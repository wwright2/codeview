#ifndef DEBUG_H
#define DEBUG_H

#include <time.h>

enum 
{
    WD_API,
    WD_RENDER,
    WD_SCANBOARD,
    WD_MAX
};

extern int allDebug;
extern time_t wdTimers[3];

#define DBG(...) { if (allDebug == 1) logErr (__VA_ARGS__); }

#endif
