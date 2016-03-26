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
//  Filename:       config.h
//
//  Description:    declarations for config stuff
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  07/28/11    Joe Halpin      1       original
//
//  Notes:
//
// ***************************************************************************

#ifndef CONFIG_H
#define CONFIG_H

#include <syslog.h>
#include <stdarg.h>
#include <sys/time.h>

typedef struct
{
    int blinkLen;            // 100-500ms how long to display the blank page
    int arrLen;              // how log to show arrivals (seconds)
    int alrtLen;             // how long to show alerts (seconds)
    int brightnessFloor;     // lowest value we'll set the LEDs to.
    int brightnessWin;       // high/low water mark for changes
} CTA_CONFIG;

extern CTA_CONFIG ctaConfig;

void setConfig (const char *data, int len);
#endif
