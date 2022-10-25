/* See LICENSE file for license and copyright details */
#ifndef DRUNKCAN_WORKQUEUE_H
#define DRUNKCAN_WORKQUEUE_H

typedef struct queue *Queue;
typedef struct sock_map *SockMap;

Queue queue_init(unsigned int len, unsigned int data_size);
unsigned int queue_datasize(const Queue q);
int queue_push(Queue q, void *data);
void *queue_try_pop(Queue q);
void queue_destroy(Queue q);

SockMap sock_map_init(void);
int sock_map_add(SockMap map, Queue q, int fd);
int sock_map_add_can(SockMap map, Queue q, int fd);
int sock_map_cansock(const SockMap map);
Queue sock_map_find(SockMap map, int fd);
void sock_map_destroy(SockMap map);

#endif
