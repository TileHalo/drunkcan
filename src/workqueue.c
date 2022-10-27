/* See LICENSE file for license and copyright details */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/can.h>
#include <unistd.h>

#include "util.h"
#include "protocol.h"
#include "btree.h"
#include "workqueue.h"


struct queue {
	unsigned int i;
	size_t data_size;
	size_t size;
	unsigned int head;
	unsigned int tail;
	int id;
	int *listen;

	void *data;
};

struct socketmap_node {
	int fd;
	int id;
	int listen;
	int writable;
	struct queue q;
};

struct socketmap {
	int can;
	int size;
	int i;
	char prefix[UNIX_NAMESIZE];
	struct protocol_conf protocol;
	struct socketmap_node *list;
};

static void _queue_init(Queue q, unsigned int data_size);
static struct socketmap_node *socketmap_find_node(SocketMap map, int fd);
static struct socketmap_node *socketmap_find_node_id(SocketMap map, int id);


static void
_queue_init(Queue q, unsigned int data_size)
{
	q->data_size = data_size;
	q->i = 0;
	q->size = 10;
	q->head = 0;
	q->tail = 0;
	q->data = malloc(q->data_size * q->size);
}

static struct
socketmap_node *socketmap_find_node(SocketMap map, int fd)
{
	int i;

	for (i = 0; i < map->i; i++) {
		if (map->list[i].fd == fd) {
			return &map->list[i];
		}
	}
	return NULL;

}

static struct
socketmap_node *socketmap_find_node_id(SocketMap map, int id)
{
	int i;

	for (i = 0; i < map->i; i++) {
		if (map->list[i].id == id) {
			return &map->list[i];
		}
	}
	return NULL;

}

Queue
queue_init(unsigned int len, unsigned int data_size)
{
	(void) len;
	struct queue *q;

	if (!(q = malloc(sizeof(*q)))) {
		return NULL;
	}

	_queue_init(q, data_size);


	return q;
}

unsigned int
queue_datasize(const Queue q)
{
	return q->data_size;
}

int
queue_listen(Queue q)
{
	return *q->listen;
}

int
queue_id(Queue q)
{
	return q->id;
}

int
queue_enque(Queue q, void *data)
{
	if (q->i == q->size) {
		q->size *= 2;
		q->data = realloc(q->data, q->size * q->data_size);
	}

	q->tail = (q->head + q->i) % q->size;

	/* Very ugly, please fix */
	memcpy((char *)q->data + q->tail * q->data_size, data, q->data_size);

	return ++q->i;
}

void *
queue_deque(Queue q)
{
	void *ret;

	if (q->i == 0) {
		return NULL;
	}

	ret = (char *)q->data + q->head * q->data_size;

	q->head++;
	if (q->head == q->size) {
		q->head = 0;
	}
	q->i--;

	return ret;
}
void *
queue_peek(Queue q)
{
	return q->i > 0 ? (char *)q->data + q->head * q->data_size : NULL;
}

void
queue_destroy(Queue q)
{

	free(q->data);
	free(q);
}

SocketMap
socketmap_init(int size)
{
	SocketMap map;

	if (!(map = malloc(sizeof(*map)))) {
		return NULL;
	}

	map->size = size;
	map->i = 0;
	map->list = calloc(sizeof(struct socketmap_node), map->size);
	if (!map->list) {
		free(map);
		return NULL;
	}

	return map;
}

Queue
socketmap_add(SocketMap map, unsigned int data_size, int fd, int id)
{
	Queue q;

	if (map->i >= map->size) {
		map->size *= 2;
		map->list = realloc(map->list, sizeof(*map->list) * map->size);
		if (!map->list) {
			return NULL;
		}
	}

	map->list[map->i].fd = fd;
	map->list[map->i].id = id;
	map->list[map->i].writable = 0;
	map->list[map->i].listen = 1;
	_queue_init(&map->list[map->i].q, data_size);
	map->list[map->i].q.id = id;
	map->list[map->i].q.listen = &map->list[map->i].listen;

	q = &map->list[map->i].q;
	map->i++;

	return q;
}

Queue
socketmap_add_can(SocketMap map, unsigned int data_size, int fd)
{
	map->can = fd;

	return socketmap_add(map, data_size, fd, 0);
}

int
socketmap_cansock(const SocketMap map)
{
	return map->can;
}

int
socketmap_enable_write(SocketMap map, int fd)
{
	struct socketmap_node *n;

	if (!(n = socketmap_find_node(map, fd))) {
		return -1;
	} else if (n->listen) {
		return -1;
	}

	n->writable = 1;

	return 0;
}

int
socketmap_set_listen(SocketMap map, int fd, int listen)
{
	struct socketmap_node *n;

	if (!(n = socketmap_find_node(map, fd))) {
		return -1;
	}

	n->listen = listen;

	return 0;
}

int
socketmap_update_fd(SocketMap map, int fd, int new)
{
	struct socketmap_node *n;
	if (!(n = socketmap_find_node(map, fd))) {
		return -1;
	}
	n->fd = new;
	return 0;
}

int
socketmap_flush(SocketMap map)
{
	int i, res, fd;
	Queue q;
	void *data;

	for (i = 0; i < map->i; i++) {
		if (map->list[i].writable && !map->list[i].listen) {
			res = 0;
			fd = map->list[i].fd;
			q = &map->list[i].q;
			while ((data = queue_peek(q))) {
				res = write(fd, data, q->data_size);
				if (res < 0) {
					break;
				}
				free(queue_deque(q));
			}
			if (res < 0 && errno == EAGAIN) {
				map->list[i].writable = 0;
			} else if (res < 0) {
				warn("Problem writing to file descriptor %d:"
				     " %s\n", fd, errno);
			}
		}
	}

	return 0;
}

Queue
socketmap_find(SocketMap map, int fd)
{
	struct socketmap_node *n;

	if (!(n = socketmap_find_node(map, fd))) {
		return NULL;
	}
	return &n->q;
}

Queue
socketmap_find_by_id(SocketMap map, int id)
{
	struct socketmap_node *n;

	if (!(n = socketmap_find_node_id(map, id))) {
		return NULL;
	}
	return &n->q;
}

int
socketmap_find_id(SocketMap map, int fd)
{
	struct socketmap_node *n;

	if (!(n = socketmap_find_node_id(map, fd))) {
		return -1;
	}
	return n->id;
}

void
socketmap_destroy(SocketMap map)
{
	int i;

	for (i = 0; i < map->i; i++) {
		free(map->list[i].q.data);
	}

	free(map->list);
	free(map);
}

void
socketmap_set_protocol(SocketMap map, struct protocol_conf conf)
{
	map->protocol = conf;
}

struct protocol_conf *
socketmap_protocol(SocketMap map)
{
	return &map->protocol;
}

char *
socketmap_prefix(SocketMap map)
{
	return map->prefix;
}

int
socketmap_set_prefix(SocketMap map, const char *prefix)
{

	if (strlen(prefix) > UNIX_NAMESIZE - 1) {
		return -1;
	}
	strcpy(map->prefix, prefix);
	return 0;
}
