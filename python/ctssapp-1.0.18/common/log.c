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
 *****************************************************************************/ // ***************************************************************************
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

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <log.h>

uint32_t logMask = WARN_BIT | DBG_BIT | TRACE_BIT | ERR_BIT;

// ---------------------------------------------------------------------------
//
void initLog (const char *pname)
{
    openlog (pname, LOG_CONS | LOG_PID, LOG_DAEMON);
}

// ---------------------------------------------------------------------------
//
void LOGFATAL (const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vsyslog (LOG_EMERG, fmt, ap);
    va_end (ap);

    exit (1);
}

// ---------------------------------------------------------------------------
//
void LOGWARN (const char *fmt, ...)
{
    if (!(logMask & WARN_BIT))
	return;

    va_list ap;

    va_start (ap, fmt);
    vsyslog (LOG_WARNING, fmt, ap);
    va_end (ap);
}

// ---------------------------------------------------------------------------
//
void LOGDBG (const char *fmt, ...)
{
    if (!(logMask & DBG_BIT))
	return;

    va_list ap;

    va_start (ap, fmt);
    vsyslog (LOG_DEBUG, fmt, ap);
    va_end (ap);
}

// ---------------------------------------------------------------------------
//
void LOGTRACE (const char *fmt, ...)
{
    if (!(logMask & TRACE_BIT))
	return;

    va_list ap;

    va_start (ap, fmt);
    vsyslog (LOG_INFO, fmt, ap);
    va_end (ap);
}

// ---------------------------------------------------------------------------
// Trace the window stuff. That generates so much output it's worth its own
// function. 
//
void LOGTRACEW (const char *fmt, ...)
{
    if (!(logMask & 0x02))
	return;

    va_list ap;

    va_start (ap, fmt);
    vsyslog (LOG_INFO, fmt, ap);
    va_end (ap);
}

// ---------------------------------------------------------------------------
//
void LOGERR (const char *fmt, ...)
{
    if (!(logMask & ERR_BIT))
	return;

    va_list ap;

    va_start (ap, fmt);
    vsyslog (LOG_ERR, fmt, ap);
    va_end (ap);
}

// ---------------------------------------------------------------------------
//
void logErr (const char *fmt, ...)
{
    if (!(logMask & ERR_BIT))
	return;

    va_list ap;

    va_start (ap, fmt);
    vsyslog (LOG_ERR, fmt, ap);
    va_end (ap);
}

