# See LICS file for license details
VERSION = 0.1

NAME = drunkcan

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# flags
CPPFLAGS =
CFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -std=c99 -pedantic -Wall -Wextra
LDFLAGS =-I/usr/include
LDLIBS =

ifeq ($(TEST)$(DEBUG),)
	CFLAGS += -O2
else ifeq ($(TEST),)
	NAME = drunkcan-debug
	CFLAGS += -g
else
	CFLAGS += -g
	LDLIBS += -lcheck
	NAME = drunkcan-test
endif

CC = cc
LD = cc
