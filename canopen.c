#include "protocol.h"

#include "canopen.h"


struct protocol_conf
canopen_protocol()
{
	struct protocol_conf conf;

	conf.protocol = CANOPEN;

	return conf;

}
