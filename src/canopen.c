/* See LICENSE file for license and copyright details */
#include <stdint.h>

#include "protocol.h"

#include "canopen.h"

#define CANOPEN_BITMASK 127
#define CANOPEN_EFF_BITMASK 134217727

static int get_id(int id);

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
	conf.id = get_id;

	return conf;

}
