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
//  Filename:       config.c
//
//  Description:    config stuff
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  09/07/2011  Joe Halpin      1       Original
//
//  Notes: 
//
// ***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "debug.h"
#include "ctaConfig.h"
#include "common.h"
#include "display.h"

//
// This is the configuration CTA sends us. It gets refreshed every 8 hours or
// so. 
//

CTA_CONFIG ctaConfig = 
{
    .blinkLen = 500,            // time to show blank page when blinking
    .arrLen   = 8,              // time to show arrival pages
    .alrtLen  = 10,             // time to show alert pages
    .brightnessFloor = 75,      // lowest value we'll set the LEDs to.
    .brightnessWin = 4,         // high/low water mark for changes
};


// ------------------------------------------------------------------------
//
void setConfigVal (const char *name, const char *val)
{
    if (strncmp (name, "blank_time", 10) == 0)
    {
        ctaConfig.blinkLen = atoi (val);
        dispSetBlankTime (&ctaConfig);
    }

    else if (strncmp (name, "arrival_display_sec", 19) == 0)
    {
        ctaConfig.arrLen = atoi (val);
        dispSetArrivalTime (&ctaConfig);
    }

    else if (strncmp (name, "alert_display_sec", 17) == 0)
    {
        ctaConfig.alrtLen = atoi (val);
        dispSetAlertTime (&ctaConfig);
    }

    else if (strncmp (name, "brightness_floor", 16) == 0)
    {
        ctaConfig.brightnessFloor = atoi (val);
    }

    else if (strncmp (name, "brightness_window", 17) == 0)
    {
        ctaConfig.brightnessWin = atoi (val);
    }
}

// ------------------------------------------------------------------------
//
static int getValues (const char *buf, char *name, char *val)
{
    int srcPos = 0;
    int dstPos = 0;

    while (buf[srcPos] && buf[srcPos] != ' ' && buf[srcPos] != '\n')
    {
        name[dstPos] = buf[srcPos];
        dstPos += 1;
        srcPos += 1;
    }
    name[dstPos] = 0;

    while (buf[srcPos] && buf[srcPos] == ' ' && buf[srcPos] != '\n')
        srcPos += 1;

    dstPos = 0;
    while (buf[srcPos] && buf[srcPos] != '\n')
    {
        val[dstPos] = buf[srcPos];
        dstPos += 1;
        srcPos += 1;
    }
    val[dstPos] = 0;

    return srcPos;
}

// ------------------------------------------------------------------------
// This updates the working parameters for sign display when we get a config
// message from CTA.
//
static char nameBuf[1024];
static char valBuf[2048];

void setConfig (const char *msg, int len)
{
    int tmpLen = 0;

    while ((tmpLen = getValues (msg, nameBuf, valBuf)) > 0)
    {
        setConfigVal (nameBuf, valBuf);
        msg += tmpLen + 1;
    }

    return;
}
