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

#include "protocol.h"
#include "util.h"
#include "btree.h"
#include "canopen.h"

#define MAX_EVENTS 32
#define MAX_CONN 16
#define UNIX_NAMESIZE 109
#define TBUF 4


struct drunk_config {
	char prefix[UNIX_NAMESIZE];
	struct protocol_conf prot;
};


static int init_sigfd(void);
/* Initialize CAN-socket */
static int init_can(const char *sock);
/* Initialize an Unix domain socket */
static int init_socket(const char *sock);
/* Make a socket epollable */
static int set_epoll(int sock, int efd, int events);
static int set_nonblock(int sock);
/* The main event loop, polls frames from socket and processes them according
 * to conf */
static int event_loop(int cansock, int efd, struct drunk_config conf);
static int process_conn(struct btree_node *node, int efd);
static int process_in(int fd, int can, struct btree *tree, int efd);
static int process_hup(int fd, struct btree *tree, int efd);
static int process_can(int fd, struct btree *tree, int efd);
static int process_sock(int fd, int can, struct btree *tree, int efd);

static void print_help(void);

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
		die("Error while initializing CAN-socket: %s", strerror(e));
	}

	strcpy(ifr.ifr_name, sock);
	ioctl(s, SIOCGIFINDEX, &ifr);

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		int e;
		e = errno;
		die("Error while binding CAN-socket: %s", strerror(e));
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
		die("Error while initializing Unix-socket: %s", strerror(e));
	}

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, sock, MIN(strlen(sock) + 1, UNIX_NAMESIZE));
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		int e;
		e = errno;
		die("Error while binding Unix-socket %s: %s",
			addr.sun_path, strerror(e));
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
		die("Error while setting nonblock: %s", strerror(e));
	}
	if (efd < 0) {
		if ((efd = epoll_create1(0)) == -1) {
			int e;
			e = errno;
			die("Error while creating epoll instance: %s",
				strerror(e));
		}
	}

	ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
	ev.events |= events;
	ev.data.fd = sock;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ev) == -1) {
		int e;
		e = errno;
		die("Error while epoll_ctl: %s",
			strerror(e));
	}

	return efd;
}

static int
set_nonblock(int sock)
{
	return fcntl(sock, F_SETFD, fcntl(sock, F_GETFD, 0) | O_NONBLOCK);
}

static int
event_loop(int cansock, int efd, struct drunk_config conf)
{
	int nfds, i, conn, sfd;
	struct btree tree;
	struct btree_node *cur;
	struct epoll_event events[MAX_EVENTS], ev;

	tree = btree_init(2048);
	strcpy(tree.prefix, conf.prefix);

	conn = init_socket(conf.prefix);
	efd = set_epoll(conn, efd, 0);
	listen(conn, MAX_CONN);
	btree_insert(&tree, 0, conn); /* Make going through fds easier */

	sfd = init_sigfd();
	efd = set_epoll(sfd, efd, 0);

	for (;;) {
		nfds = epoll_wait(efd, events, MAX_EVENTS, -1);
		if (nfds < 0) {
			goto cleanup;
		}
		for (i = 0; i < nfds; i++) {
			ev = events[i];
			cur = btree_search_fd(tree, ev.data.fd);
			if (cur && cur->fd == ev.data.fd) { /* New connection */
				process_conn(cur, efd);
			} else if (ev.data.fd == sfd) {
				goto cleanup;
			} else if (ev.events & (EPOLLIN | EPOLLPRI)) {
				process_in(ev.data.fd, cansock, &tree, efd);
			} else if (ev.events & (EPOLLRDHUP | EPOLLHUP)) {
				process_hup(ev.data.fd, &tree, efd);
			} else {
				warn("Event that should not happen: %x\n",
					ev.events);
			}
		}
	}

cleanup:
	fprintf(stderr, "Cleaning up\n");
	btree_destroy(tree);
	return 0;
}

static int
process_conn(struct btree_node *node, int efd)
{
	int conn;

	conn = accept(node->fd, NULL, NULL);
	set_epoll(conn, efd, EPOLLHUP | EPOLLRDHUP);
	int_array_push(&node->clients, conn);

	return 0;
}

static int
process_in(int fd, int can, struct btree *tree, int efd)
{

	if (fd == can) {
		return process_can(fd, tree, efd);
	} else {
		return process_sock(fd, can, tree, efd);
	}
}

static int
process_hup(int fd, struct btree *tree, int efd)
{
	int i;
	epoll_ctl(efd, EPOLL_CTL_DEL,
	fd, NULL);
	close(fd);
	for (i = 0; i < tree->size; i++) {
		int_array_remove(&tree->tree[i].clients, fd);
	}

	return 0;

}

static int
process_can(int fd, struct btree *tree, int efd)
{
	int len, i, id;
	struct can_frame fr;
	struct btree_node *node;

	len = read(fd, &fr, sizeof(fr));
	if (len < 0) {
		int e;
		e = errno;
		die("can raw socket read: %s", strerror(e));
		return 1;
	}

	fprintf(stderr, "id: %d dlc: %X data:",
		fr.can_id & tree->protocol.idmask, fr.can_dlc);
	for (i = 0; i < fr.can_dlc; i++) {
		fprintf(stderr, "%X", fr.data[i]);
	}
	fprintf(stderr, "\n");

	id = fr.can_id & tree->protocol.idmask;

	if ((node = btree_search_id(*tree, id))) {
		for (i = 0; i < (int)node->clients.i; i++) {
			write(node->clients.data[i], fr.data, fr.can_dlc);
		}
	} else {
		char sock[UNIX_NAMESIZE + 11];
		int conn;

		sprintf(sock, "%s_%d", tree->prefix, id);
		conn = init_socket(sock);
		efd = set_epoll(conn, efd, 0);
		listen(conn, MAX_CONN);

		btree_insert(tree, id, conn);
	}

	return 0;
}
static int
process_sock(int fd, int can, struct btree *tree, int efd)
{
	warn("Not implemented: %d, %s, %d", can, tree->prefix, efd);
	return fd;
}


static void
print_help(void)
{
fprintf(stderr,
	"This is the help for Drunkcan. Please consult man page for detailed "
	"information.\n"
	"Options are:\n"
	"    -h: print this help text\n"
	"    -s SOCKET: define the SocketCAN socket used. Default: can0\n"
	"    -p PROTOCOL: define the protocol used. Default: canopen\n"
	"    -x PREFIX: define the prefix used. Default: \n"
	"    -d DIRECTORY: define the directory used. Default: /tmp/ \n"
	);
}

int
main(int argc, char **argv)
{
	struct drunk_config conf;
	char sock[IF_NAMESIZE];
	char prefix[11];
	int opt, can, efd, err;

	strcpy(sock, "can0");
	memset(conf.prefix, 0, UNIX_NAMESIZE);
	strcpy(conf.prefix, "/tmp/");
	conf.prot = canopen_protocol();
	memset(prefix, 0, 11);

	while ((opt = getopt(argc, argv, "d:s:p:x:h")) != -1) {
		switch(opt) {
		case 's':
			strncpy(sock, optarg, IF_NAMESIZE);
			break;
		case 'p': /* Protocol */
			strncpy(sock, optarg, IF_NAMESIZE);
			break;
		case 'd':
			strcpy(conf.prefix, optarg);
			break;
		case 'x': /* Prefix */
			strncpy(prefix, optarg, 10);
			break;
		case 'h':
			print_help();
			return 0;
		case '?':
			printf("unknown option: %c\n", opt);
			print_help();
			return EXIT_FAILURE;
		}
	}

	strcat(conf.prefix, prefix);
	strcat(conf.prefix, sock);
	switch (conf.prot.protocol) { /* TODO: Handle the getopt */
	case CANOPEN:
		strcat(conf.prefix, "_CANopen");
		break;
	default: /* Should not reach */
		die("Unknown protocol: %d\n", conf.prot);
		break;
	}

	can = init_can(sock);
	efd = set_epoll(can, -1, 0);


	err = event_loop(can, efd, conf);
	close(efd);
	return err;
}
