/* See LICENSE file for license and copyright details */
#include <stdint.h>
#include <stddef.h>

#include <linux/can.h>

#include "protocol.h"
#include "canopen.h"

#define CANOPEN_BITMASK 127
#define CANOPEN_EFF_BITMASK 134217727

static int cleanup(void *state);
static int get_id(int id);
static int validate_can(struct can_frame fr, void *state);
static int validate_sock(void *data, size_t size, void *state);
static int process_can(struct can_frame fr, void *buf, void *state);
static int process_sock(struct can_frame *fr, void *buf, size_t size, int id,
		    void *state);

static int
cleanup(void *state)
{
	(void) state;

	return 0;
}

static int
get_id(int id)
{
	if (id > 2047) {
		return id & CANOPEN_EFF_BITMASK;
	} else {
		return id & CANOPEN_BITMASK;
	}

}

static int
validate_can(struct can_frame fr, void *state)
{
	(void) fr;
	(void) state;

	return 1;
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

	conf.frame_size = sizeof(struct canopen_frame);
	conf.state = NULL;
	conf.protocol = CANOPEN;
	conf.get_id = get_id;
	conf.cleanup = cleanup;
	conf.validate_can = validate_can;
	conf.validate_sock = validate_sock;
	conf.process_can = process_can;
	conf.process_sock = process_sock;

	return conf;

}
