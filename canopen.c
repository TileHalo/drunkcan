#include "protocol.h"

#include "canopen.h"


struct protocol_conf
canopen_protocol()
{
	struct protocol_conf conf;

	conf.idmask = CANOPEN_BITMASK;
	conf.idemask = CANOPEN_EFF_BITMASK;

	conf.protocol = CANOPEN;

	return conf;

}
