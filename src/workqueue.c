/* See LICENSE file for license and copyright details */
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <linux/can.h>

#include "workqueue.h"


struct queue {
	int size; /* size in char *:s */
	int i;
	size_t data_size;

	void *data;
	void *head, *tail;

};

struct sock_map_node {
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
	if (!(q->data = malloc(q->size * q->data_size))) {
		free(q);
		return NULL;
	}
	q->head = NULL;
	q->tail = q->head;


	return q;
}

unsigned int
queue_datasize(const Queue q)
{
	return q->data_size;
}

int
queue_push(Queue q, void *data)
{
	if (q->i == q->size)
		return -1; /* Not implemented yet */

	/* Extremely illegal, but I do not know any better */
	q->tail = (char *)q->tail + q->data_size;
	memcpy(q->tail, data, q->data_size);

	q->i++;

	return q->i;
}

void *
queue_try_pop(Queue q)
{
	void *ret;

	if (!(ret = malloc(q->data_size)) || q->i == 0) {
		return NULL;
	}

	memcpy(ret, q->head, q->data_size);
	q->head = (char *)q->head + q->data_size;
	q->i--;

	return ret;
}

void
queue_destroy(Queue q)
{

	free(q->data);
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
sock_map_add(SockMap map, Queue q, int fd)
{


	return 0;
}

int
sock_map_add_can(SockMap map, Queue q, int fd)
{
	map->can = fd;

	return sock_map_add(map, q, fd);
}

int
sock_map_cansock(const SockMap map)
{
	return map->can;
}

Queue
sock_map_find(SockMap map, int fd)
{
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
