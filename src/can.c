/* See LICENSE file for license and copyright details */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <linux/can.h>

#include "can.h"
#include "util.h"

struct can_queue
can_queue_init(void)
{
	struct can_queue q;
	int e;

	q.size = 10;
	q.len = 0;

	q.q = malloc(sizeof(*q.q) * q.size);

	if (!q.q) {
		e = errno;
		die("Error while malloc: %s", strerror(e));
	}

	return q;
}

struct can_frame
can_queue_pop(struct can_queue *q)
{
	struct can_frame cf;
	cf.can_id = 0;

	if (q->len > 0) {
		cf = q->q[0];
		q->q = memcpy(q->q, &(q->q[1]), (q->len - 1) * sizeof(cf));
		if (!q->q) {
			int e;
			e = errno;
			die("Error while malloc: %s", strerror(e));
		}
	}

	return cf;
}

int
can_queue_push(struct can_queue *q, struct can_frame cf)
{
	if (q->size < q->len*2) {
		q->size *= 2;
		q->size += 5;
		q->q = realloc(q->q, q->size * sizeof(cf));
		if (!q->q) {
			int e;
			e = errno;
			die("Error while realloc: %s", strerror(e));
		}
	}
	q->q = memcpy(&(q->q[1]), q->q, q->len * sizeof(cf));
	if (!q->q) {
		int e;
		e = errno;
		die("memcpy: %s", strerror(e));
	}
	q->q[0] = cf;
	return 0;
}

void
can_queue_destroy(struct can_queue q)
{
	q.size = 0;
	q.len = 0;
	free(q.q);
}
