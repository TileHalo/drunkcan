/* See LICENSE file for copyright and license details. */
#ifndef DRUNKCAN_UTIL_H
#define DRUNKCAN_UTIL_H

#define LEN(a) (sizeof (a)/sizeof(*a))

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

void warn(const char *, ...);
void die(const char *, ...);
ssize_t getline(char **, size_t *, FILE *);
#endif
