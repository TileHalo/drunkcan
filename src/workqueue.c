/* See LICENSE file for license and copyright details */
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <linux/can.h>

#include "workqueue.h"

struct qdata {
	struct qdata *next;
	void *data;
};

struct queue {
	int size; /* size in char *:s */
	int i;
	size_t data_size;

	struct qdata *head, *tail;

};

struct sock_map_node {
	int fd;
	Queue q;
	struct sock_map_node *left, *right;
};

struct sock_map {
	int can;
	int size;
	int i;
	struct sock_map_node *list;
};

Queue
queue_init(unsigned int len, unsigned int data_size)
{
	struct queue *q;

	if (!(q = malloc(sizeof(*q)))) {
		return NULL;
	}

	q->size = len;
	q->data_size = data_size;
	q->i = 0;
	q->head = NULL;
	q->tail = NULL;


	return q;
}

unsigned int
queue_datasize(const Queue q)
{
	return q->data_size;
}

int
queue_enque(Queue q, void *data)
{
	if (!q->tail) {
		q->head = malloc(sizeof(struct qdata));
		q->tail = q->head;
	} else {
		q->tail->next = malloc(sizeof(struct qdata));
		q->tail = q->tail->next;
	}
	if (!q->tail) {
		return -1;
	}

	q->tail->data = malloc(q->data_size);
	if (!q->tail->data) {
		return -1;
	}
	memcpy(q->tail->data, data, q->data_size);
	q->tail->next = NULL;

	q->i++;

	return q->i;
}

void *
queue_deque(Queue q)
{
	struct qdata *old;
	void *ret;

	if (!(old = q->head)) {
		return NULL;
	}
	ret = old->data;
	q->head = old->next;
	free(old);
	q->i--;

	return ret;
}
void *
queue_peek(Queue q)
{
	return q->head ? q->head->data : NULL;
}

void
queue_destroy(Queue q)
{
	void *data;

	while ((data = queue_deque(q))) {
		free(data);
	}
	free(q);

}

SockMap
sock_map_init(void)
{
	SockMap map;

	if (!(map = malloc(sizeof(struct sock_map)))) {
		return NULL;
	}

	map->size = 20;
	map->i = 0;
	map->list = malloc(sizeof(struct sock_map_node) * map->size);
	if (!map->list) {
		free(map);
		return NULL;
	}

	return map;
}

int
sock_map_add(SockMap map, unsigned int data_size, int fd)
{

	if (map->i == map->size) {
		/* TODO: Do the reallocation trick here */
	}

	map->list[map->i].fd = fd;
	map->list[map->i].q = queue_init(20, data_size);
	map->list[map->i].left = NULL;
	map->list[map->i].right = NULL;
	if (!map->list[map->i].q) {
		return -1;
	}



	return 0;
}

int
sock_map_add_can(SockMap map, unsigned int data_size, int fd)
{
	map->can = fd;

	return sock_map_add(map, data_size, fd);
}

int
sock_map_cansock(const SockMap map)
{
	return map->can;
}

Queue
sock_map_find(SockMap map, int fd)
{
	int i;

	for (i = 0; i < map->i; i++) {
		if (map->list[i].fd == fd) {
			return map->list[i].q;
		}
	}
	return NULL;
}

void
sock_map_destroy(SockMap map)
{
	int i;

	for (i = 0; i < map->i; i++) {
		queue_destroy(map->list[i].q);
	}

	free(map->list);
	free(map);
}
