# See LICS file for license details
VERSION = 0.1

_NAME = drunkcan
NAME = $(_NAME)

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# flags
CFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -g -std=c99  -pedantic -Wall -Wextra
LDFLAGS =-I/usr/include
LDLIBS =

CC = gcc
LD = gcc
