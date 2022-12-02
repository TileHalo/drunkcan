/* See LICENSE file for license and copyright details */
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <linux/can.h>
#include <sys/time.h>

#include "protocol.h"
#include "workqueue.h"
#include "canopen.h"

enum canopen_status {
	BOOTUP = 0x00,
	STOPPED = 0x04,
	OPERATIONAL = 0x05,
	PREOPERATIONAL = 0x7f
};

enum functions {
	NMT = 0x000,
	FAILSAFE = 0x001,
	SYNC = 0x080,
	EMERGENCY = 0x080,
	TIMESTAMP = 0x100,
	TPDO1 = 0x180,
	RPDO1 = 0x200,
	TPDO2 = 0x280,
	RPDO2 = 0x300,
	TPDO3 = 0x380,
	RPDO3 = 0x400,
	TPDO4 = 0x480,
	RPDO4 = 0x500,
	TSDO = 0x580,
	RSDO = 0x600,
	HEARTBEAT = 0x700,
	TLSS = 0x7E4,
	RLSS = 0x7E5,
};

struct canopen_dev {
	int id;
	enum canopen_status status;
	int heartbeat; /* in ms */
	unsigned int vendorid;
	unsigned int productcode;
	unsigned short subindex;
	unsigned int revision_number;
	unsigned int serial_number;
	struct canopen_dev *left;
	struct canopen_dev *right;
};

struct canopen_state {
	enum canopen_status status;
	enum canopen_status desired;
	long synctime;
	long startup;
	Queue queue;
	unsigned short syncid;
	struct canopen_dev *root;
};

struct cobid {
	int fn;
	int nodeid;
};

static struct cobid get_cobid(int id);
static long get_time(void);

static int cleanup(void *state);
static int give_queue(void *q, void *state);
static int update(void *state);
static int get_id(int id);
static int validate_can(struct can_frame fr, void *state);
static int validate_sock(void *data, size_t size, void *state);
static int process_can(struct can_frame fr, void *buf, void *state);
static int process_sock(struct can_frame *fr, void *buf, size_t size, int id,
		    void *state);

static struct can_frame nmt[] = {
	{ .can_id = NMT, { .len =  1 }, .data =  {0x01} }, /* start */
	{ .can_id = NMT, { .len =  1 }, .data =  {0x02} }, /* stop */
	{ .can_id = NMT, { .len =  1 }, .data =  {0x80} }, /* preop */
	{ .can_id = NMT, { .len =  1 }, .data =  {0x81} }, /* reset */
	{ .can_id = NMT, { .len =  1 }, .data =  {0x82} }, /* reset_comm */
};

static struct can_frame sync = { .can_id = SYNC, { .len =  0 }};
// static struct can_frame timestamp = { .can_id = TIMESTAMP, { .len =  6 }};


static struct cobid
get_cobid(int id)
{
	struct cobid cobid;
	int fncodes;

	fncodes = (id & CAN_EFF_FLAG) ? CAN_EFF_ID_BITS : CAN_SFF_ID_BITS;
	fncodes -= 4;

	cobid.fn = id >> fncodes;
	cobid.nodeid = id & ((1 << fncodes) - 1);

	return cobid;
}

static long
get_time(void)
{
	struct timeval  tv;

	gettimeofday(&tv, NULL);
	return round((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000);
}

static int
cleanup(void *state)
{

	free(state);
	return 0;
}

static int
give_queue(void *q, void *state)
{
	struct canopen_state *st;
	if (!(st = (struct canopen_state *)state)) {
		return -1;
	}
	st->queue = (Queue)q;
	return 0;
}

static int
update(void *state)
{
	struct canopen_state *st;
	long elapsed;

	if (!(st = (struct canopen_state *)state)) {
		return -1;
	}

	if (st->status == st->desired) {
		goto nodestatus;
	}
	switch (st->desired) {
		case OPERATIONAL:
			queue_enque(st->queue, &nmt[0]);
			break;
		case STOPPED:
			queue_enque(st->queue, &nmt[1]);
			break;
		case PREOPERATIONAL:
			queue_enque(st->queue, &nmt[2]);
			break;
		default:
			goto nodestatus;
	}
	st->status = st->desired;
	nodestatus:

	/* Sync */
	elapsed = get_time() - st->startup;
	if (elapsed >= st->synctime) {
		st->startup += elapsed;
		queue_enque(st->queue, &sync);
	}

	return 0;
}

static int
get_id(int id)
{
	struct cobid cobid;

	cobid = get_cobid(id);

	return cobid.nodeid;
}

static int
validate_can(struct can_frame fr, void *state)
{
	(void) state;

	struct cobid cobid;

	cobid = get_cobid(fr.can_id);

	switch (cobid.fn) {
	case TPDO1:
	case TPDO2:
	case TPDO3:
	case TPDO4:
	case TSDO:
	case EMERGENCY:
	case FAILSAFE:
		break;
	default:
		return -1;

	}

	return cobid.nodeid;
}

static int
validate_sock(void *data, size_t size, void *state)
{
	(void) data;
	(void) size;
	(void) state;

	return 1;
}

static int
process_can(struct can_frame fr, void *buf, void *state)
{
	(void) state;
	(void) buf;

	buf = &fr;

	return 0;

}

static int
process_sock(struct can_frame *fr, void *buf, size_t size, int id, void *state)
{
	(void) state;
	(void) buf;
	(void) fr;
	(void) size;
	(void) id;

	return 0;
}

struct protocol_conf
canopen_protocol(void)
{
	struct protocol_conf conf;
	struct canopen_state *state;


	if ((state = malloc(sizeof(*state)))) {
		state->status = BOOTUP;
		state->desired = OPERATIONAL;
		state->synctime = 1000;
		state->startup = get_time();
		state->root = NULL;
	}
	conf.frame_size = sizeof(struct canopen_frame);
	conf.state = state;
	conf.update = update;
	conf.protocol = CANOPEN;
	conf.get_id = get_id;
	conf.cleanup = cleanup;
	conf.give_queue = give_queue;
	conf.validate_can = validate_can;
	conf.validate_sock = validate_sock;
	conf.process_can = process_can;
	conf.process_sock = process_sock;

	return conf;

}
