/******************************************************************************
 *****                                                                    *****
 *****                 Copyright (c) 2011 Luminator USA                   *****
 *****                      All Rights Reserved                           *****
 *****    THIS IS UNPUBLISHED CONFIDENTIAL AND PROPRIETARY SOURCE CODE    *****
 *****                        OF Luminator USA                            *****
 *****       ANY UNAUTHORIZED USE, REPRODUCTION, OR TRANSFER OF THIS      *****
 *****                  PROGRAM IS STRICTLY PROHIBITED                    *****
 *****          The copyright notice above does not evidence any          *****
 *****         actual or intended publication of such source code         *****
 *****                                                                    *****
 *****************************************************************************/
// ***************************************************************************
//
//  Filename:       log.c
//
//  Description:    Logging function that can be shared. Logs to syslog
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  05/15/11    Joe Halpin      1       Initial
//
//  Notes: 
//
// ***************************************************************************

#define _BSD_SOURCE
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <pthread.h>

static char buf[2048];
pthread_mutex_t logLock = PTHREAD_MUTEX_INITIALIZER;

void initLog (const char *pname)
{
    pthread_mutex_lock (&logLock);

    openlog (pname, LOG_CONS | LOG_PID, LOG_DAEMON);

    pthread_mutex_unlock (&logLock);
}


void logErr (const char *fmt, ...)
{
    pthread_mutex_lock (&logLock);

    int priority = LOG_CONS | LOG_PID;
    va_list ap;

    va_start (ap, fmt);
    vsnprintf (buf, sizeof buf, fmt, ap);
    syslog (priority, buf);
    //vsyslog (priority, fmt, ap);
    va_end (ap);

    pthread_mutex_unlock (&logLock);
}
