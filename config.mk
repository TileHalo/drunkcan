# See LICS file for license details
VERSION = 0.1

NAME = drunkcan

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# flags
CFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -std=c99  -pedantic -Wall -Wextra
LDFLAGS =-I/usr/include
LDLIBS =

ifdef $(DEBUG)
	NAME = drunkcan-debug
	CFLAGS += -g
endif

ifdef $(TEST)
	NAME = drunkcan-test
	CFLAGS += -g
	LDLIBS += -lcheck
endif

ifeq ($(TEST)$(DEBUG),)
	CFLAGS += -Os
endif

CC = gcc
LD = gcc
