#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

#define TRACE_BIT  0x01
#define TRACEW_BIT 0x02
#define WARN_BIT   0x04
#define DBG_BIT    0x08
#define ERR_BIT    0x10

extern uint32_t logMask;

void initLog (const char *pname);
void LOGWARN (const char *fmt, ...);
void LOGDBG (const char *fmt, ...);
void LOGTRACE (const char *fmt, ...);  // normal trace function
void LOGTRACEW (const char *fmt, ...); // trace function for window stuff
void LOGERR (const char *fmt, ...);
void LOGFATAL(const char *fmt, ...); // no bit for this, it always logs & exits

static inline void die (const char *msg)
{
    LOGFATAL ("%s: %s", msg, strerror (errno));
}

#ifdef DEBUG_THREAD
#define LOCK(x, e)   {LOGDBG ("===> locking %s: %s %d\n",#x,__FILE__,__LINE__); e = pthread_mutex_lock (&x);}
#define UNLOCK(x, e) {LOGDBG ("<=== unlocking %s: %s %d\n",#x,__FILE__,__LINE__); e = pthread_mutex_unlock (&x);}
#else
#define LOCK(x, e)   {pthread_mutex_lock (&x);}
#define UNLOCK(x, e) {pthread_mutex_unlock (&x);}
#endif

#endif
