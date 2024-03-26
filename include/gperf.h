#ifndef GPERF_H
#define GPERF_H

#include <stdint.h>
#include <string.h>

// gperf single integer value
struct gsiv_t {
	char *name;
	long long value;
};
typedef struct gsiv_t gsiv_t;

#endif
