/* See LICENSE file for copyright and license details. */
#ifndef DRUNKCAN_UTIL_H
#define DRUNKCAN_UTIL_H

#define LEN(a) (sizeof (a)/sizeof(*a))

void warn(const char *, ...);
void die(const char *, ...);
ssize_t getline(char **, size_t *, FILE *);
#endif
