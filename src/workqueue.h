/* See LICENSE file for license and copyright details */
#ifndef DRUNKCAN_WORKQUEUE_H
#define DRUNKCAN_WORKQUEUE_H

typedef struct queue *Queue;
typedef struct socketmap *SocketMap;

Queue queue_init(unsigned int len, unsigned int data_size);
unsigned int queue_datasize(const Queue q);
int queue_listen(Queue q);
int queue_id(Queue q);
int queue_enque(Queue q, void *data);
void *queue_deque(Queue q);
void *queue_peek(Queue q); /* Return first without popping */
void queue_destroy(Queue q);

SocketMap socketmap_init(int size);
Queue socketmap_add(SocketMap map, unsigned int data_size, int fd, int id);
Queue socketmap_add_can(SocketMap map, unsigned int data_size, int fd);
int socketmap_cansock(const SocketMap map);
int socketmap_enable_write(SocketMap map, int fd);
int socketmap_set_listen(SocketMap map, int fd, int listen);
int socketmap_update_fd(SocketMap map, int fd, int new);
int socketmap_flush(SocketMap map);
Queue socketmap_find(SocketMap map, int fd);
int socketmap_find_id(SocketMap map, int fd);
Queue socketmap_find_by_id(SocketMap map, int id);
void socketmap_destroy(SocketMap map);

void socketmap_set_protocol(SocketMap map, struct protocol_conf conf);
struct protocol_conf *socketmap_protocol(SocketMap map);
char *socketmap_prefix(SocketMap map);
int socketmap_set_prefix(SocketMap map, const char *prefix);

#endif
