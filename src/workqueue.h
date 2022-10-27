/* See LICENSE file for license and copyright details */
#ifndef DRUNKCAN_WORKQUEUE_H
#define DRUNKCAN_WORKQUEUE_H

typedef struct queue *Queue;
typedef struct socketmap *SocketMap;

Queue queue_init(unsigned int len, unsigned int data_size);
unsigned int queue_datasize(const Queue q);
int queue_enque(Queue q, void *data);
void *queue_deque(Queue q);
void *queue_peek(Queue q); /* Return first without popping */
void queue_destroy(Queue q);

SocketMap socketmap_init(int size);
Queue socketmap_add(SocketMap map, unsigned int data_size, int fd);
Queue socketmap_add_can(SocketMap map, unsigned int data_size, int fd);
int socketmap_cansock(const SocketMap map);
int socketmap_enable_write(SocketMap map, int fd);
int socketmap_flush(SocketMap map);
Queue socketmap_find(SocketMap map, int fd);
void socketmap_destroy(SocketMap map);

#endif
