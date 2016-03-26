#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <sys/time.h>
#include <stdint.h>

static inline uint64_t getUSecs () {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    uint64_t now = tv.tv_sec;
    now *= 1000000;
    now += tv.tv_usec;
    return now;
}

#endif /* __HELPERS_H__ */
