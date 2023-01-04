#ifndef LOG_H
#define LOG_H

#include <errno.h>
#include <stdio.h>

// https://cmake.org/pipermail/cmake/2013-January/053117.html
#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define LOG(colour, format, ...) fprintf(stderr, "\e[1;%im[%s:%i]\e[0m " format "\n", colour, __FILENAME__, __LINE__, ##__VA_ARGS__)

#define NOTE(format, ...) LOG(33, format, ##__VA_ARGS__)

#define ERR(format, ...) LOG(31, format, ##__VA_ARGS__)
#define ERR_PTR(str) ERR("%s", str);
#define ERR_ERRNO(format, ...) ERR(format ": %s", ##__VA_ARGS__, strerror(errno));

#ifdef ENABLE_DEBUG
#define DEBUG(format, ...) LOG(1, format, ##__VA_ARGS__)
#else
#define DEBUG(...) printf("hello")
#endif

#endif
