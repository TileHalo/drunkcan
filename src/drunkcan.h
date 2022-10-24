/* See LICENSE file for copyright and license details. */
#ifndef DRUNKCAN_H
#define DRUNKCAN_H

#define MAX_EVENTS 32
#define MAX_CONN 16
#define UNIX_NAMESIZE 109
#define TBUF 4
struct drunk_config {
	char prefix[UNIX_NAMESIZE];
	char *sock;
	struct protocol_conf prot;
};

int event_loop(struct drunk_config conf);
#endif
