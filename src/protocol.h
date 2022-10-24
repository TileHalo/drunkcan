/* See LICENSE file for license and copyright details */

#ifndef DRUNKCAN_PROTOCOL_H
#define DRUNKCAN_PROTOCOL_H
enum protocol{
	CANOPEN = 1,
};

struct protocol_conf {
	enum protocol protocol;
	int (*id)(int);
};
#endif
