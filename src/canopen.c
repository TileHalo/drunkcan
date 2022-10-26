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

static int
cleanup(void *state)
{
	(void) state;

	return 0;
}

static int
get_id(int id)
{
	if (id > CANOPEN_BITMASK) {
		return id & CANOPEN_EFF_BITMASK;
	} else {
		return id & CANOPEN_BITMASK;
	}

}

struct protocol_conf
canopen_protocol(void)
{
	struct protocol_conf conf;

	conf.frame_size = sizeof(struct canopen_frame);
	conf.protocol = CANOPEN;
	conf.get_id = get_id;
	conf.cleanup = cleanup;

	return conf;

}
