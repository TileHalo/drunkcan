#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <linux/can.h>

#include "util.h"
#include "protocol.h"
#include "canopen.h"


#include "drunkcan.h"

static void print_help(void);

static void
print_help(void)
{
fprintf(stderr,
	"This is the help for Drunkcan. Please consult man page for detailed "
	"information.\n"
	"Options are:\n"
	"    -h: print this help text\n"
	"    -s SOCKET: define the SocketCAN socket used. Default: can0\n"
	"    -p PROTOCOL: define the protocol used. Default: canopen\n"
	"    -x PREFIX: define the prefix used. Default: \n"
	"    -d DIRECTORY: define the directory used. Default: /tmp/ \n"
	"    -t Runs tests. Only applicable when made with DEBUG=1  \n"
	);
}


int
main(int argc, char **argv)
{
	struct drunk_config conf;
	char sock[IF_NAMESIZE];
	char prefix[11];
	int opt, err;

	strcpy(sock, "can0");
	memset(conf.prefix, 0, UNIX_NAMESIZE);
	strcpy(conf.prefix, "/tmp/");
	memset(prefix, 0, 11);

	conf.prot.protocol = CANOPEN;
	while ((opt = getopt(argc, argv, "d:s:p:x:ht")) != -1) {
		switch(opt) {
		case 's':
			strcpy(sock, optarg);
			break;
		case 'p': /* Protocol */
			if (strcasecmp("canopen", optarg) == 0) {
				conf.prot.protocol = CANOPEN;
			} else {
				die("Protocol %s not supported", optarg);
			}
			break;
		case 'd':
			strcpy(conf.prefix, optarg);
			break;
		case 'x': /* Prefix */
			strncpy(prefix, optarg, 10);
			break;
		case 'h':
			print_help();
			return 0;
		case '?':
			printf("unknown option: %c\n", opt);
			print_help();
			return EXIT_FAILURE;
                }
	}

	strcat(conf.prefix, prefix);
	strcat(conf.prefix, sock);
	conf.sock = sock;
	switch (conf.prot.protocol) { /* TODO: Handle the getopt */
	case CANOPEN:
		strcat(conf.prefix, "_CANopen");
		conf.prot = canopen_protocol();
		break;
	default: /* Should not reach */
		die("Unknown protocol: %d\n", conf.prot);
		break;
	}



	err = event_loop(conf);
	err = conf.prot.cleanup(conf.prot.protocol_state) < 0 ? -1 : err;
	return err;
}
