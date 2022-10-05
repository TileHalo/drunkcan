# See LICS file for license details
VERSION = 0.1

_NAME = drunkcan
NAME = $(_NAME)

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# flags
_CPPFLAGS = -D_DEFAULT_SOURCE
_CFLAGS = -std=c99 -pedantic -Wall -Wextra
_LDFLAGS =-I/usr/include
_LDLIBS = 
ifneq "$(and $(TST),$(DBUG))" ""
CPPFLAGS = $(_CPPFLAGS)
CFLAGS = $(_CFLAGS) -0s
LDFLAGS = $(_LDFLAGS) -s
LDLIBS = $(_LDLIBS)
else ifdef TST
CPPFLAGS = $(_CPPFLAGS) -g
LDLIBS = $(_LDLIBS) -lcheck
LDFLAGS = $(_LDFLAGS)
LDLIBS = $(_LDLIBS)
NAME = $(_NAME)-test
else
CPPFLAGS = $(_CPPFLAGS) -g
CFLAGS = $(_CFLAGS)
LDFLAGS = $(_LDFLAGS)
LDLIBS = $(_LDLIBS)
endif

CC = gcc
LD = gcc
