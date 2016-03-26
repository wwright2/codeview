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
//  Filename:       alert.c
//
//  Description:    Isolates all the alert handling in one place
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  09/09/2011  Joe Halpin      1       Original
//
//  Notes: 
//
// ***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <fcntl.h>
#include <pthread.h>

#include "debug.h"
#include "trackerconf.h"
#include "common.h"
#include "portal.h"
#include "display.h"
#include "alerts.h"

extern GLYPH glyphs[256];
ALERT_DATA alertData = 
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .alertIndex = 0,
    .dispAlert = 0,
    .alertSequence = 0,
    .newAlertsOk = 1,
    .curAlert = 0,
    .curLine = 0,
    .changedAlert = 0,
    .numAlerts = 0,
    .alertBuf = { 0 },
    .alertBufPos = 0,
    .lines = { 0 },
    .linesIndex = 0,
    .alerts = { {0} },
    .tmpAlert = { 0 },
};

extern pthread_mutex_t alertReadyLock;

// --------------------------------------------------------------------------
//
void dumpLines ()
{
    DBG (__FILE__" === entering dumpLines");
    DBG (__FILE__" dumping lines: linesIndex = %d", alertData.linesIndex);
    for (int i = 0; i < alertData.linesIndex; ++i)
        DBG (__FILE__"    [%s]", alertData.lines[i]);
    DBG (__FILE__" === leaving dumpLines");
}

#if 0
// --------------------------------------------------------------------------
//
static void dumpAlert (const ALERT *a)
{
    // caller must lock alertData

    DBG (__FILE__"  === entering dumpAlert %s %d",__FILE__,__LINE__);
    DBG (__FILE__"      a->numLines = %d\n", a->numLines);

    for (int j = 0; j < a->numLines; ++j)
        DBG (__FILE__"          a->lineArray[%d] = %s\n", j, a->lineArray[j]);
    DBG (__FILE__"  === leaving dumpAlert");
}

// --------------------------------------------------------------------------
//
static void dumpAlerts ()
{
    // caller must lock alertData

    int i;
    for (i = 0; i < alertData.numAlerts; ++i)
        dumpAlert (&alertData.alerts[i]);
}
#endif

// --------------------------------------------------------------------------
//
static int doLineBreaks (char *pMatch, int matchLen)
{
    // caller must lock alertBuf

    int i = 0;
    int space = 0;
    int lineCount = 0;
    int colsUsed = 0;
    static const int maxCols = COLS_PER_LINE;

    alertData.lines[alertData.linesIndex] = 
        &alertData.alertBuf[alertData.alertBufPos];

    while ((i <= matchLen) && (lineCount < MAX_LINES))
    { 
        if ((pMatch[i] == 0))
        {
            alertData.alertBuf[alertData.alertBufPos] = 0;
            alertData.alertBufPos += 1;
            lineCount  += 1;
            alertData.linesIndex += 1;
            alertData.lines[alertData.linesIndex] = 
                &alertData.alertBuf[alertData.alertBufPos];
            space = 0;
            break;
        }
        
        if (pMatch[i] == ' ')
        {
            space = i;
        }
        
        colsUsed += glyphs[(uint8_t)pMatch[i]].width;
        if (colsUsed <= maxCols)
        {
            alertData.alertBuf[alertData.alertBufPos] = pMatch[i];
            alertData.alertBufPos += 1;
        }
        
        else
        {
            if (space > 0 && space != i)
            {
                // 
                // We saved the position of the last space, reset back to
                // there, and end this line at the space.
                //
                
                //
                // Do not set alertBufPos to space, back it up. AlertBufPos
                // accumulates acrros alerts.
                //
                
                alertData.alertBufPos -= (i - space);
                alertData.alertBuf[alertData.alertBufPos] = 0;
                alertData.alertBufPos += 1;

                lineCount  += 1;
                alertData.linesIndex += 1;
                
                alertData.lines[alertData.linesIndex] = 
                    &alertData.alertBuf[alertData.alertBufPos];
                
                i = space;
                space = 0;
                colsUsed = 0;
            }
            
            else
            {
                //
                // Didn't see a space. Terminate the line in
                // alertData.alertBuf at this point, and move on to the
                // next line.
                //
                
                alertData.alertBuf[alertData.alertBufPos] = 0;
                alertData.alertBufPos += 1;
                
                lineCount  += 1;
                alertData.linesIndex += 1;
                
                alertData.lines[alertData.linesIndex] = 
                    &alertData.alertBuf[alertData.alertBufPos];
                colsUsed = 0;
                space = 0;
            }
        }
        
        i += 1;
    }

    return lineCount;
}


// --------------------------------------------------------------------------
// Scan through pNew looking for a line feed. Copy characters up to that line
// feed into pTmp.
//
void getAlert (const char *pNew, char *pTmp, int *size)
{
    // caller must lock alert buf

    int count = 0;
    char *pc  = pTmp; // leave pTmp alone so we can print it out.

    while (pNew && *pNew && (*pNew != 0) && (*pNew != '\n'))
    {
        *pc  = *pNew;
        pc  += 1;
        
        pNew  += 1;
        count += 1;
    }

    *pc = 0;
    *size = count;
}


// --------------------------------------------------------------------------
//
void alertSetData (const uint8_t *newData, size_t len)
{
    const char *pNew = (const char *)&newData[3];
    char       *pTmp = alertData.tmpAlert;

    if (alertData.newAlertsOk == 0)
    {
        //
        // Don't put new alerts into the system while alerts are currently
        // being shown.
        //

        return;
    }

    alertData.alertIndex   = 0;
    alertData.linesIndex   = 0;
    alertData.numAlerts    = newData[1];
    alertData.changedAlert = newData[2];
    alertData.alertBufPos  = 0;
    
    lockAlertData();

    memset (alertData.lines, 0, sizeof alertData.lines);
    memset (alertData.alertBuf, 0, sizeof alertData.alertBuf);
    memset (alertData.tmpAlert, 0, sizeof alertData.tmpAlert);
    memset (alertData.alerts, 0, sizeof alertData.alerts);

    if (alertData.numAlerts == 0)
    {
        unlockAlertData ();
        return;
    }

    for (;;)
    {
        int size = 0;

        if (alertData.alertIndex >= MAX_ALERTS)
        {
            break;
        }

        alertData.alerts[alertData.alertIndex].lineArray = 
            &alertData.lines[alertData.linesIndex];

        // Copy next alert from input buffer to temp buffer
        pTmp = alertData.tmpAlert;
        getAlert (pNew, pTmp, &size);
        if (size == 0)
        {
            break;
        }

        // copy the alert from the temp buf into alertData.alertBuf, splitting
        // it up into alertData.lines along the way.
        int lc;
        if ((lc = doLineBreaks (pTmp, size)) == 0)
        {
            break;
        }

        alertData.alerts[alertData.alertIndex].numLines = lc;
        alertData.alerts[alertData.alertIndex].maxIndex = lc - 1;

        pNew += size + 1;
        alertData.alertIndex += 1;

    }

    //
    // If changed alert > 0 then an alert has been added. This is presumed to
    // have a priority such that it should take precidence over the following
    // ones. So if we're currently displaying an alert past the point where the
    // new one was inserted, set dispAlert to the new alert so we'll back up
    // and show it first.
    //

    alertData.numAlerts = alertData.alertIndex;
    if (alertData.numAlerts >= alertData.changedAlert && 
        alertData.dispAlert > alertData.changedAlert)
        alertData.dispAlert = alertData.changedAlert;

    alertList.changed = 1;
    alertList.active  = 1;

    //dumpLines ();
    //dumpAlerts ();

    unlockAlertData();
}

