#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

static void
verr(const char *fmt, va_list ap)
{
	vfprintf(stderr, fmt, ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}
}

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verr(fmt, ap);
	va_end(ap);
}

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verr(fmt, ap);
	va_end(ap);

	exit(1);
}


ssize_t
getline(char **str, size_t *size, FILE *fp)
{
	char c, *s;
	size_t i, len;

	if (!(*str) && size == 0) {
		len = 80;
		if (!(s = calloc(len, sizeof(char)))) {
			die("calloc() failed");
		}
	} else {
		len = *size;
		s = *str;
	}
	i = 0;

	while ((c = fgetc(fp)) != EOF) {
		if (i == len) {
			len += 80;
			if (!(s = realloc(s, len))) {
				die("realloc() failed");
			}
		}
		switch (c) {
		case '\n':
			s[i] = 0;
			*str = s;
			if (size != 0) {
				*size = len;
			}
			return i;
		default:
			s[i] = c;
		}
		i++;
	}
	*str = s;
	if (size != 0) {
		*size = len;
	}
	return -1;
}
