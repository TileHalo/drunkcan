/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "util.h"
#include "protocol.h"
#include "workqueue.h"
#include "drunkcan.h"

#define MAX_EVENTS 32
#define MAX_CONN 16
#define UNIX_NAMESIZE 109
#define TBUF 4
#define READBYTES 64


static int init_sigfd(void);
static int init_can(const char *sock);
static int init_socket(const char *sock);

static int set_epoll(int sock, int efd, int events);
static int set_nonblock(int sock);
static int close_socket(int fd, int efd);

static int process_conn(SocketMap map, int fd, int efd);
static int process_in(SocketMap map, int fd, int efd);
static int process_hup(SocketMap map, int fd, int efd);
static int process_can(SocketMap map, int fd, int efd);
static int process_sock(SocketMap map, int fd);

static int sfd;


static int
init_sigfd(void)
{
	sigset_t mask;
	int sfd;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
		int err;
		err = errno;
		die("Error creating signal mask: %s", strerror(err));
	}

	sfd = signalfd(-1, &mask, 0);
	if (sfd < 0) {
		int err;
		err = errno;
		die("Error creating signalfd %s", strerror(err));
	}
	return sfd;
}

static int
init_can(const char *sock)
{
	int s;
	struct sockaddr_can addr;
	struct ifreq ifr;

	s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (s < 0) {
		warn("Error while initializing CAN-socket:");
		return -1;
	}
	memset(&addr, 0, sizeof(struct sockaddr_can));

	strcpy(ifr.ifr_name, sock);
	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
		warn("ioctl error on socket %s:", sock);
		return -1;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		warn("Error while binding CAN-socket %s:", sock);
		return -1;
	}

	return s;
}
static int
init_socket(const char *sock)
{
	int s;
	struct sockaddr_un addr;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		warn("Error while initializing Unix-socket:");
		return -1;
	}

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, sock);
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
                warn("Error while binding Unix-socket %s:", addr.sun_path);
                return -1;
	}

	return s;
}

static int
set_epoll(int sock, int efd, int events)
{
	struct epoll_event ev;


	if (set_nonblock(sock) == -1) {
		warn("Error while setting nonblock:");
		return -1;
	}
	if (efd < 0) {
		if ((efd = epoll_create1(0)) == -1) {
			warn("Error while creating epoll instance:");
			return -1;
		}
	}

	ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
	ev.events |= events;
	ev.data.fd = sock;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ev) == -1) {
		warn("Error while epoll_ctl:");
		return -1;
	}

	return efd;
}

static int
set_nonblock(int sock)
{
	return fcntl(sock, F_SETFD, fcntl(sock, F_GETFD, 0) | O_NONBLOCK);
}

static int
close_socket(int fd, int efd)
{
	epoll_ctl(efd, EPOLL_CTL_DEL,
		  fd, NULL);
	return close(fd);
}

static int
process_conn(SocketMap map, int fd, int efd)
{
	int conn;

	conn = accept(fd, NULL, NULL);
	if (set_epoll(conn, efd, 0) < 0) {
		return -1;
	}
	if (socketmap_set_listen(map, fd, 0) < 0) {
		return -1;
	}
	return socketmap_update_fd(map, fd, conn);
}

static int
process_in(SocketMap map, int fd, int efd)
{

	if (fd == socketmap_cansock(map)) {
		return process_can(map, fd, efd);
	} else {
		return process_sock(map, fd);
	}
}

static int
process_hup(SocketMap map, int fd, int efd)
{
	char sock[UNIX_NAMESIZE + 11];
	int conn;
	Queue q;

	close_socket(fd, efd);

	if (fd == socketmap_cansock(map)) {
		return -1;
	}

	if (!(q = socketmap_find(map, fd))) {
		return -1;
	}
	if (queue_listen(q)) {
		return -1;
	}
	socketmap_set_listen(map, fd, 1);

	sprintf(sock, "%s_%d", socketmap_prefix(map), queue_id(q));
	conn = init_socket(sock);
	efd = set_epoll(conn, efd, 0);
	listen(conn, MAX_CONN);

	socketmap_update_fd(map, fd, conn);

	return 0;

}

static int
process_can(SocketMap map, int fd, int efd)
{
	int id, res;
	struct can_frame fr;
	struct protocol_conf *prot;
	void *data;
	Queue q;

	prot = socketmap_protocol(map);
	data = NULL; /* Otherwise process_can complains */
	while ((res = read(fd, &fr, sizeof(fr))) > 0) {
		id = prot->get_id(fr.can_id);
		if (!(q = socketmap_find_by_id(map, id))) {
			char sock[UNIX_NAMESIZE + 11];
			int conn;

			sprintf(sock, "%s_%d", socketmap_prefix(map), id);
			conn = init_socket(sock);
			efd = set_epoll(conn, efd, 0);
			listen(conn, MAX_CONN);

			q = socketmap_add(map, prot->frame_size, conn, id);
			if (!q) {
				return -1;
			}
		}
		res = prot->process_can(fr, data, prot->state);
		if (res > 0) {
			if(queue_enque(q, data) < 0) {
				return -1;
			}
		} else if (res < 0) {
			return -1;
		}
	}

	return res == EAGAIN ? 0 : -1;
}

static int
process_sock(SocketMap map, int fd)
{
	int res, id;
	struct can_frame frame;
	struct protocol_conf *prot;
	void *data;

	if (!(data = malloc(READBYTES))) {
		warn("Malloc failed at process_sock:");
		return -1;
	}

	id = socketmap_find_id(map, fd);

	while ((res = read(fd, data, READBYTES)) > 0) {
		prot = socketmap_protocol(map);
		if (!prot->validate_sock(data, res, prot->state)) {
			continue;
		}
		res = prot->process_sock(&frame, data, res, id, prot->state);
	}

	return 0;
}

static int
process_event(struct epoll_event ev, SocketMap map, int efd)
{
	int res;
	(void) res;
	Queue q;


	q = socketmap_find(map, ev.data.fd);
	if (q && queue_listen(q)) { /* New connection */
		process_conn(map, ev.data.fd, efd);
		return 0;
	} else if (ev.data.fd == sfd) {
		return 1;
	}

	if (ev.events & EPOLLIN) {
		return process_in(map, ev.data.fd, efd);
	}
	if (ev.events & EPOLLOUT) {
		return socketmap_enable_write(map, ev.data.fd);
	}
	if (ev.events & (EPOLLRDHUP | EPOLLHUP)) {
		return process_hup(map, ev.data.fd, efd);
	} else {
		warn("Event that should not happen: %x\n",
		     ev.events);
	}

	return 0;
}

int
event_loop(struct drunk_config conf)
{
	int nfds, i, conn, cansock, efd, res;
	SocketMap map;
	struct epoll_event events[MAX_EVENTS];


	if (!(map = socketmap_init(20))) {
		warn("Failed to initialize socket map:");
		return -1;
	}
	socketmap_set_protocol(map, conf.prot);
	socketmap_set_prefix(map, conf.prefix);

	/* Set up can socket */
	cansock = init_can(conf.sock);
	if (cansock < 0) {
		res = -1;
		goto err_cleanup;
	}

	efd = set_epoll(cansock, -1, 0);
	if (efd < 0) {
		res = -1;
		goto err_cleanup;
	}

	if (!socketmap_add_can(map, sizeof(struct can_frame), cansock)) {
		res = -1;
		goto cleanup;
	}

	conn = init_socket(conf.prefix);
	if (conn < 0) {
		res = -1;
		goto cleanup;
	}

	efd = set_epoll(conn, efd, 0);

	if (efd < 0) {
		res = -1;
		goto cleanup;
	}

	if (listen(conn, MAX_CONN) < 0) {
		res = -1;
		warn("Error while listening:");
		goto cleanup;
	}
	if (!socketmap_add(map, conf.prot.frame_size, conn, -1)) {
		res = -1;
		goto cleanup;
	}
	sfd = init_sigfd();
	efd = set_epoll(sfd, efd, 0);

	for (;;) {
		nfds = epoll_wait(efd, events, MAX_EVENTS, -1);
		if (nfds < 0) {
			goto cleanup;
		}
		for (i = 0; i < nfds; i++) {
			res  = process_event(events[i], map, efd);
			if (res == 1) { /* A signal was catched */
				res = 0;
				goto cleanup;
			} else if (res < 0) {
				die("Please fix this, died on event loop\n");
			}
		}
		if (socketmap_flush(map) < 0) {
			warn("Problem during flush:");
			res = -1;
			goto cleanup;
		}
	}

	cleanup:
	close(efd);
	err_cleanup:
	fprintf(stderr, "Cleaning up\n");
	socketmap_destroy(map);
	return res;
}
