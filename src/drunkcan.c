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
#include "intarray.h"
#include "btree.h"
#include "workqueue.h"
#include "drunkcan.h"

#define MAX_EVENTS 32
#define MAX_CONN 16
#define UNIX_NAMESIZE 109
#define TBUF 4


static int init_sigfd(void);
static int init_can(const char *sock);
static int init_socket(const char *sock);

static int set_epoll(int sock, int efd, int events);
static int set_nonblock(int sock);
static int close_socket(int fd, int efd);

static void remove_fd(Node node, void *data);

static int process_conn(Node node, int efd);
static int process_in(int fd, int can, BinTree tree, int efd);
static int process_hup(int fd, BinTree tree, int efd);
static int process_can(int fd, BinTree tree, int efd);
static int process_sock(int fd, int can, BinTree tree, int efd);

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
		int e;
		e = errno;
		warn("Error while initializing CAN-socket: %s", strerror(e));
		return -1;
	}
	memset(&addr, 0, sizeof(struct sockaddr_can));

	strcpy(ifr.ifr_name, sock);
	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
		int e;
		e = errno;
		warn("ioctl error on socket %s: %s", sock, strerror(e));
		return -1;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		int e;
		e = errno;
		warn("Error while binding CAN-socket %s: %s", sock, strerror(e));
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
		int e;
		e = errno;
		warn("Error while initializing Unix-socket: %s", strerror(e));
		return -1;
	}

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, sock);
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		int e;
		e = errno;
		warn("Error while binding Unix-socket %s: %s",
		    addr.sun_path, strerror(e));
		return -1;
	}

	return s;
}

static int
set_epoll(int sock, int efd, int events)
{
	struct epoll_event ev;


	if (set_nonblock(sock) == -1) {
		int e;
		e = errno;
		warn("Error while setting nonblock: %s", strerror(e));
		return -1;
	}
	if (efd < 0) {
		if ((efd = epoll_create1(0)) == -1) {
			int e;
			e = errno;
			/* Same here */
			warn("Error while creating epoll instance: %s",
			    strerror(e));
			return -1;
		}
	}

	ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
	ev.events |= events;
	ev.data.fd = sock;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ev) == -1) {
		int e;
		e = errno;
		warn("Error while epoll_ctl: %s",
		    strerror(e));
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

static void
remove_fd(Node node, void *data)
{
	int fd;

	fd = *(int *)data;
	if (node_client(node) == fd) {
		node_set_client(node, -1);
	}
}

static int
process_conn(Node node, int efd)
{
	int conn;

	conn = accept(node_fd(node), NULL, NULL);
	set_epoll(conn, efd, 0);
	node_set_client(node, conn);
	close_socket(node_fd(node), efd);

	return conn;
}

static int
process_in(int fd, int can, BinTree tree, int efd)
{

	if (fd == can) {
		return process_can(fd, tree, efd);
	} else {
		return process_sock(fd, can, tree, efd);
	}
}

static int
process_hup(int fd, BinTree tree, int efd)
{
	close_socket(fd, efd);
	btree_apply(tree, remove_fd, &fd);

	return 0;

}

static int
process_can(int fd, BinTree tree, int efd)
{
	int len, id;
	struct can_frame fr;
	Node node;

	len = read(fd, &fr, sizeof(fr));
	if (len < 0) {
		int e;
		e = errno;
		die("can raw socket read: %s", strerror(e));
		return 1;
	}

	id = tree_protocol(tree)->get_id(fr.can_id);

	if ((node = btree_search_id(tree, id))) {
		/* TODO: Create a message queue here */
	} else {
		char sock[UNIX_NAMESIZE + 11];
		int conn;

		sprintf(sock, "%s_%d", tree_prefix(tree), id);
		conn = init_socket(sock);
		efd = set_epoll(conn, efd, 0);
		listen(conn, MAX_CONN);

		btree_insert(tree, id, conn);
	}

	return 0;
}
static int
process_sock(int fd, int can, BinTree tree, int efd)
{
	warn("Not implemented: %d, %s, %d", can, tree_prefix(tree), efd);
	return fd;
}

static int
process_event(BinTree tree, struct epoll_event ev, SockMap socks, int efd)
{
	int res;
	Node cur;
	Queue q;
	int cansock;


	cansock = sock_map_cansock(socks);
	cur = btree_search_fd(tree, ev.data.fd);

	if (cur) { /* New connection */
		process_conn(cur, efd);
		return 0;
	} else if (ev.data.fd == sfd) {
		return 1;
	}

	q = sock_map_find(socks, ev.data.fd);
	if (!q) {
		warn("Could not find proper socket queue for fd %d\n",
		     ev.data.fd);
		return -1;
	}
	if (ev.events & (EPOLLIN | EPOLLPRI)) {
		return process_in(ev.data.fd, cansock, tree, efd);
	} else if (ev.events & EPOLLOUT) {
		void *data;
		res = 0;
		while ((data = queue_peek(q))) {
			res = write(ev.data.fd, data, queue_datasize(q));
			if (res < 0) {
				switch (errno) {
				case EAGAIN:
					return 0;
				default:
					return -1;
				}
			}
			/* Check for disasters */
			if (!queue_deque(q)) {
				die("Queue %d that should have elements"
				     "failed to pop", ev.data.fd);
			}
		}
	} else if (ev.events & (EPOLLRDHUP | EPOLLHUP)) {
		return process_hup(ev.data.fd, tree, efd);
	} else {
		warn("Event that should not happen: %x\n",
		     ev.events);
	}

	return 0;
}

int
event_loop(struct drunk_config conf)
{
	int nfds, i, conn, cansock, efd, res, e;
	SockMap socks;
	BinTree tree;
	struct epoll_event events[MAX_EVENTS];


	if (!(socks = sock_map_init())) {
		warn("Failed to initialize socket map: %s", strerror(errno));
		return -1;
	}
	if (!(tree = btree_init(2048, conf.prefix))) {
		e = errno;
		warn("Failed to initialize protocol tree: %s", strerror(e));
		sock_map_destroy(socks);
		return -1;
	}

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
	res = sock_map_add_can(socks, sizeof(struct can_frame), cansock);

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

	if (listen(conn, MAX_CONN) < -1) {
		e = errno;
	}
	res = sock_map_add_can(socks, conf.prot.frame_size, cansock);
	btree_insert(tree, 0, conn); /* Make going through fds easier */

	sfd = init_sigfd();
	efd = set_epoll(sfd, efd, 0);

	for (;;) {
		nfds = epoll_wait(efd, events, MAX_EVENTS, -1);
		if (nfds < 0) {
			goto cleanup;
		}
		for (i = 0; i < nfds; i++) {
			res  = process_event(tree, events[i], socks, efd);
			if (res == 1) { /* A signal was catched */
				res = 0;
				goto cleanup;
			} else if (res < 0) {
				die("Please fix this, died on event loop\n");
			}
		}
	}

	cleanup:
	close(efd);
	err_cleanup:
	fprintf(stderr, "Cleaning up\n");
	sock_map_destroy(socks);
	btree_destroy(tree);
	return res;
}
