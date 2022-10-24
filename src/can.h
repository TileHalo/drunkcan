/* See LICENSE file for license and copyright details */

#ifndef DRUNKCAN_CAN_H
#define DRUNKCAN_CAN_H

struct can_queue {
	int size;
	int len;
	struct can_frame *q;
};

struct can_queue can_queue_init(void);
struct can_frame can_queue_pop(struct can_queue *q);
int can_queue_push(struct can_queue *q, struct can_frame cf);
void can_queue_destroy(struct can_queue q);

#endif
