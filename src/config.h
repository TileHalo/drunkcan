/* See LICENSE file for license and copyright information */

struct config {
	const char *name;
	const char *prefix;

	struct protocol_conf (*getconf)(void);
};

static const struct config protocols[] = {
	{ "canopen", "_CANopen", canopen_protocol}
};
