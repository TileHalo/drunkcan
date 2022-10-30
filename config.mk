# See LICENSE file for license details
VERSION = 0.1

NAME = drunkcan

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# flags
CPPFLAGS =
CFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -std=c99 -pedantic -Wall -Wextra
LDFLAGS =
LDLIBS =

CHECKTOOL = valgrind
CHECKFLAGS = --leak-check=full --track-origins=yes -s

ifeq ($(DEBUG),)
	CFLAGS += -O2
else
	NAME = drunkcan-debug
	CFLAGS += -g -DDEBUG
endif

CC = cc
LD = cc
