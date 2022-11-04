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
#include "config.h"

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
	const char *prot;
	char prefix[11];
	int opt, err;

	strcpy(sock, "can0");
	memset(conf.prefix, 0, UNIX_NAMESIZE);
	strcpy(conf.prefix, "/tmp/");
	memset(prefix, 0, 11);

	prot = protocols[0].prefix;
	conf.prot = protocols[0].getconf();
	while ((opt = getopt(argc, argv, "d:s:p:x:ht")) != -1) {
		switch(opt) {
		case 's':
			strcpy(sock, optarg);
			break;
		case 'p': /* Protocol */
			struct config *p;
			for (p = protocols; p < (&protocols)[1]; p++) {
				if (strcmp(p->name, optarg) == 0) {
					prot = p->prefix;
					conf.prot = p->getconf();
					break;
				}
			}
			die("Unknown protocol");
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
	strcat(conf.prefix, prot);



	err = event_loop(conf);
	err = conf.prot.cleanup(conf.prot.state) < 0 ? -1 : err;
	return err;
}
