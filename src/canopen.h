/* See LICENSE file for license and copyright details */
#ifndef DRUNKCAN_CANOPEN_H
#define DRUNKCAN_CANOPEN_H

#define CANOPEN_MAX_DATA 256


enum canopen_command {
	CANOPEN_NMT = 0x00,
	CANOPEN_FAIL = 0x01,
	CANOPEN_SYNC_OR_EMERGENCY = 0x08,
	CANOPEN_TPDO1 = 0x18,
	CANOPEN_RPDO1 = 0x20,
	CANOPEN_TPDO2 = 0x28,
	CANOPEN_RPDO2 = 0x30,
	CANOPEN_TPDO3 = 0x38,
	CANOPEN_RPDO3 = 0x40,
	CANOPEN_TPDO4 = 0x48,
	CANOPEN_RPDO4 = 0x50,
	CANOPEN_TSDO = 0x58,
	CANOPEN_RSDO = 0x60,
	CANOPEN_NMTnm = 0x70
};


struct canopen_frame {
	short init;
	short cmd;
	union {
		short can_id;
		uint64_t eff_id : 40;
	} nodeid;
	short __cc;
	short len;
	short data[CANOPEN_MAX_DATA];
} __attribute__((__packed__));

struct protocol_conf canopen_protocol(void);

#endif
