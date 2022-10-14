#ifndef CANOPEN_H
#define CANOPEN_H

#define CANOPEN_BITMASK 0x07F;
#define CANOPEN_EFF_BITMASK 0x07FFFFFF;

enum canopen_command {
	CANOPEN_NMT = 0x000,
	CANOPEN_FAIL = 0x001,
	CANOPEN_SYNC_OR_EMERGENCY = 0x080,
	CANOPEN_TPDO1 = 0x180,
	CANOPEN_RPDO1 = 0x200,
	CANOPEN_TPDO2 = 0x280,
	CANOPEN_RPDO2 = 0x300,
	CANOPEN_TPDO3 = 0x380,
	CANOPEN_RPDO3 = 0x400,
	CANOPEN_TPDO4 = 0x480,
	CANOPEN_RPDO4 = 0x500,
	CANOPEN_TSDO = 0x580,
	CANOPEN_RSDO = 0x600,
	CANOPEN_NMTnm = 0x700
};


struct canopen_frame {
	enum canopen_command cmd;
	union {
		unsigned short id;
		unsigned int eid;
	} node_id;
	short rtr;

};

struct protocol_conf canopen_protocol();

#endif
