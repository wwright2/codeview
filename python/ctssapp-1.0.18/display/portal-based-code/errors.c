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
//  Filename:       error.c
//
//  Description:    Isolates all the error handling in one place
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  09/09/2011  Joe Halpin      1       Original
//
//  Notes: 
//
//  Yes, I copied this straight from alerts.c and changed the names
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
#include "errors.h"
#include "debug.h"

extern GLYPH glyphs[256];
ERROR_DATA errorData = 
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .errorIndex = 0,
    .dispError = 0,
    .errorSequence = 0,
    .newErrorsOk = 1,
    .curError = 0,
    .curLine = 0,
    .errorBuf = { 0 },
    .errorBufPos = 0,
    .lines = { 0 },
    .linesIndex = 0,
    .errors = { {0} },
    .tmpError = { 0 },
};

extern pthread_mutex_t errorReadyLock;

// --------------------------------------------------------------------------
//
void dumpErrorLines ()
{
    DBG (__FILE__" === entering dumpErrorLines");
    DBG (__FILE__" dumping lines: linesIndex = %d", errorData.linesIndex);
    for (int i = 0; i < errorData.linesIndex; ++i)
        DBG (__FILE__"    [%s]", errorData.lines[i]);
    DBG (__FILE__" === leaving dumpErrorLines");
}

#if 0
// --------------------------------------------------------------------------
//
static void dumpError (const ERROR *a)
{
    // caller must lock errorData

    DBG (__FILE__"  === entering dumpError %s %d",__FILE__,__LINE__);
    DBG (__FILE__"      a->numLines = %d\n", a->numLines);

    for (int j = 0; j < a->numLines; ++j)
        DBG (__FILE__"          a->lineArray[%d] = %s\n", j, a->lineArray[j]);
    DBG (__FILE__"  === leaving dumpError");
}
#endif

#if 0
// --------------------------------------------------------------------------
//
static void dumpErrors ()
{
    // caller must lock errorData

    dumpError (&errorData.errors[0]);
}
#endif

// --------------------------------------------------------------------------
//
static int doLineBreaks (char *pMatch, int matchLen)
{
    // caller must lock errorBuf

    int i = 0;
    int space = 0;
    int lineCount = 0;
    int colsUsed = 0;
    static const int maxCols = COLS_PER_LINE;

    errorData.lines[errorData.linesIndex] = 
        &errorData.errorBuf[errorData.errorBufPos];

    while ((i <= matchLen) && (lineCount < MAX_LINES))
    { 
        if ((pMatch[i] == 0))
        {
            errorData.errorBuf[errorData.errorBufPos] = 0;
            errorData.errorBufPos += 1;
            lineCount  += 1;
            errorData.linesIndex += 1;
            errorData.lines[errorData.linesIndex] = 
                &errorData.errorBuf[errorData.errorBufPos];
            space = 0;
            break;
        }
        
        if (pMatch[i] == ' ')
        {
            space = i;
        }
        
        colsUsed += glyphs[(uint8_t)pMatch[i]].width;
        if (colsUsed < maxCols)
        {
            errorData.errorBuf[errorData.errorBufPos] = pMatch[i];
            errorData.errorBufPos += 1;
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
                // Do not set errorBufPos to space, back it up. ErrorBufPos
                // accumulates acrros errors.
                //
                
                errorData.errorBufPos -= (i - space);
                errorData.errorBuf[errorData.errorBufPos] = 0;
                errorData.errorBufPos += 1;

                lineCount  += 1;
                errorData.linesIndex += 1;
                
                errorData.lines[errorData.linesIndex] = 
                    &errorData.errorBuf[errorData.errorBufPos];
                
                i = space;
                space = 0;
                colsUsed = 0;
            }
            
            else
            {
                //
                // Didn't see a space. Terminate the line in
                // errorData.errorBuf at this point, and move on to the
                // next line.
                //
                
                errorData.errorBuf[errorData.errorBufPos] = 0;
                errorData.errorBufPos += 1;
                
                lineCount  += 1;
                errorData.linesIndex += 1;
                
                errorData.lines[errorData.linesIndex] = 
                    &errorData.errorBuf[errorData.errorBufPos];
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
void getError (const char *pNew, char *pTmp, int *size)
{
    // caller must lock error buf

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
void errorSetData (const uint8_t *newData, size_t len)
{
    const char *pNew = (const char *)&newData[3];
    char       *pTmp = errorData.tmpError;

    if (errorData.newErrorsOk == 0)
    {
        //
        // Don't put new errors into the system while errors are currently
        // being shown.
        //

        return;
    }

    errorData.errorIndex   = 0;
    errorData.linesIndex   = 0;
    errorData.errorBufPos  = 0;
    
    lockErrorData();

    memset (errorData.lines, 0, sizeof errorData.lines);
    memset (errorData.errorBuf, 0, sizeof errorData.errorBuf);
    memset (errorData.tmpError, 0, sizeof errorData.tmpError);
    memset (errorData.errors, 0, sizeof errorData.errors);

    int size = 0;
    
    if (errorData.errorIndex >= MAX_ERRORS)
    {
        return;
    }
    
    errorData.errors[errorData.errorIndex].lineArray = 
        &errorData.lines[errorData.linesIndex];
    
    // Copy the error from input buffer to temp buffer
    pTmp = errorData.tmpError;
    getError (pNew, pTmp, &size);
    if (size == 0)
    {
        return;
    }

    // copy the error from the temp buf into errorData.errorBuf, splitting
    // it up into errorData.lines along the way.
    int lc;
    if ((lc = doLineBreaks (pTmp, size)) == 0)
    {
        return;
    }

    //dumpErrorLines ();
    //dumpErrors ();

    //
    // Mark the error list active and the arrival list inactive. We show one or
    // the other.
    //

    dispLockErrorList ();
    errorList.changed = 1;
    errorList.active  = 1;
    dispUnlockErrorList ();

    dispLockArrivalList ();
    arrivalList.changed = 0;
    arrivalList.active  = 0;
    dispUnlockArrivalList ();

    unlockErrorData();
}

