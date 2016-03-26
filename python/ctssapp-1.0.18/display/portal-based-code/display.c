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
//  Filename:       display.c
//
//  Description:    implementation of display code
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  04/19/11    Joe Halpin      1       
//  07/28/11    Joe Halpin      2       Refactoring
//
//
// ***************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <ncurses.h>

#define DISPLAY_INCLUDE
#include "debug.h"
#include "ctaConfig.h"
#include "portal.h"
#include "portalInit.h"
#include "display.h"
#include "font.h"

extern int debugCurses;
extern GLYPH glyphs[256];

// ------------------------------------------------------------------------
// Private data
// ------------------------------------------------------------------------

static int lastErr;
static const char *lastErrMsg = "No error";
static uint8_t pixVal = 127;    // value to use when turning pixels on.

//
// There are two of these because they're double-buffered. One is made ready
// while the other is being transmitted.
//
// NOTE:
// The brightness messages must not be changed. Sansi doesn't use this the way
// we thought they did. We're changing the brightness by varying the value used
// when specifying the pixels.
//

DISP_DATA dispData[2] = 
{
    {
        .lock = PTHREAD_MUTEX_INITIALIZER,
        .brightnessMsg = { 0xa3, 32, 0, 0, 0 }
    },

    {
        .lock = PTHREAD_MUTEX_INITIALIZER,
        .brightnessMsg = { 0xa3, 32, 0, 0, 0 }
    }
};

// This is to keep gcc from whining about array index not being constant
#define INITIAL_ACTIVE_INDEX 0

int curDisplay = INITIAL_ACTIVE_INDEX;
int nextDisplay = INITIAL_ACTIVE_INDEX ^ 1;

static inline void toggleActiveDisplay()
{
    dispLockCurDisplay ();
    dispLockNextDisplay ();

    nextDisplay = curDisplay;
    curDisplay ^= 1;

    // The indexes just got swapped, unlock things in "reverse" order
    dispUnlockCurDisplay ();
    dispUnlockNextDisplay ();
}


// ------------------------------------------------------------------------
// Code
// ------------------------------------------------------------------------

static void setError (int e, const char *s)
{
    lastErr = e;
    lastErrMsg = s;
}

// -----------------------------------------------------------------

int dispGetLastErr ()
{
    return lastErr;
}

// -----------------------------------------------------------------

const char *dispGetLastErrStr ()
{
    return lastErrMsg;
}

// ------------------------------------------------------------------------

void dispInit()
{
    //pthread_mutex_lock (&dispData[nextDisplay].lock);
    //memset(dispData[nextDisplay].buf, 0, sizeof dispData[nextDisplay].buf);
    //pthread_mutex_unlock (&dispData[nextDisplay.lock));

    //if (debugCurses)
        // setup curses for sign debug output
        //initscr ();
}

// ------------------------------------------------------------------------

void dispCleanup ()
{
    //if (debugCurses)
        //endwin ();
}

// ------------------------------------------------------------------------

static const char *justText (PORTAL_JUSTIFY j)
{
    switch (j)
    {
        case P_LEFT:
            return "P_LEFT";

        case P_CENTER:
            return "P_CENTER";

        case P_RIGHT:
            return "P_RIGHT";
    }
    return "";
}

// ------------------------------------------------------------------------

static const char *getText (PORTAL *p)
{
    //
    // The calling code is responsible for locking the enclosing portal
    // list. It probably won't because this is only used for debugging portal
    // layouts. 
    //

    static char buf[MAX_STRING];
    int i;

    if (p->dataSize >= sizeof buf)
        return "too big";

    for (i = 0; i< p->dataSize; ++i)
        buf[i] = p->data[i];
    buf[i] = 0;

    return buf;
}

// ------------------------------------------------------------------------

void dumpPortal (PORTAL *p)
{
    DBG (__FILE__"    just = %s\n", justText(p->just));
    DBG (__FILE__"    id   = %d\n", p->id);
    DBG (__FILE__"    upperLeftRow = %d\n", p->ulRow);
    DBG (__FILE__"    upperLeftCol = %d\n", p->ulCol);
    DBG (__FILE__"    startCol     = %d\n", p->startCol);
    DBG (__FILE__"    width        = %d\n", p->width);
    DBG (__FILE__"    height       = %d\n", p->height);
    DBG (__FILE__"    text         = %s\n", getText (p));
    DBG (__FILE__" \n");

#if 0
    printf ("   just = %s\n", justText(p->just));
    printf ("   id   = %d\n", p->id);
    printf ("   upperLeftRow = %d\n", p->ulRow);
    printf ("   upperLeftCol = %d\n", p->ulCol);
    printf ("   startCol     = %d\n", p->startCol);
    printf ("   width        = %d\n", p->width);
    printf ("   height       = %d\n", p->height);
    printf ("   text         = %s\n", getText (p));
    printf ("\n");
#endif
}

// ------------------------------------------------------------------------
// The calling code is responsible for locking the enclosing portal list
//
int dispSetPortalText (PORTAL *p, const char *t)
{
    if ((p == 0) || (t == 0))
    {
        setError (EDISP_INVAL, "Null pointer passed to dispSetPortalText()");
        return -1;
    }

    p->dataSize = strlen (t);
    if (p->dataSize > sizeof p->data)
        p->dataSize = sizeof p->data;

    int colsAvail = p->width;
    int colsUsed  = 0;
    int curWidth  = 0;
    int i         = 0;

    memset (p->data, 0, sizeof p->data);
    while (i < p->dataSize)
    {
        //
        // Don't set any more text than will fit into the number of columns
        // available in the portal. Break it off on whole word boundaries. That
        // means we have to look one word ahead when figuring out what will
        // fit.
        //

        while (t[i] == ' ')
        {
            curWidth = glyphs[(int) ' '].width;
            if (colsUsed + curWidth > colsAvail)
            {
                p->dataSize = i;
                break;
            }
            colsUsed += curWidth;
            p->data[i] = t[i];
            ++i;
        }
            
        while (t[i] != ' ')
        {
            curWidth = glyphs[(int)t[i]].width;
            if (colsUsed + curWidth > colsAvail)
            {
                p->dataSize = i;
                break;
            }
            colsUsed += curWidth;
            p->data[i] = t[i];
            ++i;
        }
    }

    return 0;
}

// ------------------------------------------------------------------------
// This assumes that the inactive buffer is ALREADY LOCKED
//
static void drawGlyph (
    const GLYPH *g, int *startCol, const PORTAL *p, int maxCol)
{
    //
    // Caller is responsible for locking...
    //

    int col;

    // for each byte in the glyph bitmap
    for (int r = 0; r < g->height; ++r)
    {
        //
        // DANGER WILL ROBINSON!
        //
        // This assumes that no glyph width will be greater than 8 bits.
        // This needs to be made smarter eventually. Until then let's
        // be sure our glyphs are small enough.
        //
        
        int row = p->ulRow + r;
        uint8_t byte = g->data[r];

        for (col = *startCol; 
             (col < *startCol + g->width) && (col < maxCol); 
             ++col)
        {
            if (byte & 0x80)
            {
                dispData[nextDisplay].buf[row][col][0] = pixVal;
            }
            else
            {
                dispData[nextDisplay].buf[row][col][0] = 0;
            }
            byte <<= 1;
        }
    }

    *startCol = col;
}

// ------------------------------------------------------------------------

static void setJustification (PORTAL *p, GLYPH *g[], int gsize)
{
    //
    // Adjust the start point per the justification setting.
    //

    int glyphCols  = 0;
    int portalCols = p->width;
    for (int i = 0; i < gsize; ++i)
        glyphCols += g[i]->width;

    p->startCol = p->ulCol;
    switch (p->just)
    {
        case P_LEFT:
            break;

        case P_RIGHT:
            if (portalCols >= glyphCols)
            {
                p->startCol = (portalCols - glyphCols) + p->ulCol;
            }
            break;
            
        case P_CENTER:
            if (portalCols > glyphCols)
            {
                int tmp = portalCols - glyphCols;
                if (tmp %2 != 0)
                    ++tmp;
                p->startCol = (tmp / 2) + p->ulCol;
            }
            break;
    }
}

// ------------------------------------------------------------------------

void dumpGlyph (GLYPH *g)
{
    printf ("glyph = \n");
    printf ("   width  = %d\n", g->width);
    printf ("   height = %d\n", g->height);
    for (int i = 0; i < g->height; ++i)
        printf ("   %02x ", g->data[i]);
    printf ("\n\n");
}

// ------------------------------------------------------------------------
// The calling code is responsible for locking the enclosing portal list before
// calling this.
//

int dispDrawPortal (PORTAL *p)
{
    //
    // the caller is responsible for locking before calling this
    //

    DBG ("=== dispDrawPortal: drawing [%s]",p->data);

    GLYPH *g[p->dataSize];

    //
    // Cycle through the letters in the text to be displayed. We have
    // to resolve justification before rendering the portal. Collect
    // all the glyphs, add up their widths, and move the start points
    // to the appropriate column.
    //

    for (int i = 0; i < p->dataSize; ++i)
    {
        // get the glyph for the next letter to render
        g[i] = &glyphs[(int) p->data[i]];
    }

    setJustification (p, g, sizeof g / sizeof g[0]);

    //
    // Draw the portal
    //

    int col = p->startCol;
    int maxCol = p->startCol + p->width;

    for (int i = 0; i < p->dataSize; ++i)
        drawGlyph (g[i], &col, p, maxCol);

    //dumpPortal (p);
    return 0;
}

// ------------------------------------------------------------------------

const uint8_t *dispGetDisplayRow (int r)
{
    //
    // The caller is responsible for locking the active buffer before calling
    // this.
    //

    if (r < 0 || r > VROWS)
    {
        setError (r, "Row out of range");
        return 0;
    }

    return (uint8_t *)dispData[curDisplay].buf[r];
}

// ------------------------------------------------------------------------

void dispDumpDisplay ()
{
    for (int r = 0; r < DISP_ROWS; ++r)
    {
        for (int c = 0; c < DISP_COLS; ++c)
        {
            //mvprintw (r, 0, "%c", 
            //    (dispData[curDisplay].buf[r][c][0]) ? '*' : ' ');
            printf ("%c", (dispData[curDisplay].buf[r][c][0]) ? '*' : ' ');
        }
        //printf ("\n");
    }
    //printf ("\n");
}

// ------------------------------------------------------------------------

void dispDumpNextDisplay ()
{
    for (int r = 0; r < DISP_ROWS; ++r)
    {
        for (int c = 0; c < DISP_COLS; ++c)
        {
            //mvprintw (r, 0, "%c", 
            //    (dispData[nextDisplay].buf[r][c][0]) ? '*' : ' ');
            printf ("%c", (dispData[nextDisplay].buf[r][c][0]) ? '*' : ' ');
        }
        printf ("\n");
    }
    printf ("\n");
}

// ------------------------------------------------------------------------

uint8_t *getBrightnessMsg (size_t *size)
{
    *size = sizeof dispData[curDisplay].brightnessMsg;
    return dispData[curDisplay].brightnessMsg;
}

// ------------------------------------------------------------------------

void dispSetBrightness (int b)
{
    if (b < ctaConfig.brightnessFloor)
        b = ctaConfig.brightnessFloor;

    pixVal = b;
    DBG ("brightness: pixVal now == %d", pixVal);

    //
    // Store the new setting in /home/cta so it can be put in the next
    // status report.
    //
    
    const char *tempFile = "/home/cta/brightness.tmp";
    const char *realFile = "/home/cta/brightness.txt";
    
    FILE *fp = fopen (tempFile, "w");
    if (! fp)
        logErr ("Could not open %s: %s %d: %s", 
            tempFile,__FILE__,__LINE__, strerror (errno));
    else
    {
        fprintf (fp, "%d", pixVal);
        fclose (fp);
        rename (tempFile, realFile);
    }
}

// ------------------------------------------------------------------------

void dispSwitchDisplays ()
{
    toggleActiveDisplay ();
}

// ------------------------------------------------------------------------

void dispSetCTAConfig (const CTA_CONFIG *ctaConfig)
{
    //
    // This should only get called about once every 8 hours.
    //

    dispLockArrivalList ();
    arrivalList.pauseTime.tv_sec = ctaConfig->arrLen;
    dispUnlockArrivalList ();

    dispLockAlertList ();
    alertList.pauseTime.tv_sec = ctaConfig->alrtLen;
    dispUnlockAlertList ();

    dispLockBlankList ();
    blankList.pauseTime.tv_sec = ctaConfig->blinkLen;
    dispUnlockBlankList ();
}

// ------------------------------------------------------------------------

void dispSetBlankTime (CTA_CONFIG *ctaConfig)
{
    // Bug fix
    // no lock set because 
    // 1) it only happens once every 8 hours
    // 2) I'm afraid of deadlocks and don't have the time to work it out.

    blankList.pauseTime.tv_sec = 0;
    blankList.pauseTime.tv_usec = ctaConfig->blinkLen * 1000;
}

// ------------------------------------------------------------------------

void dispSetArrivalTime (CTA_CONFIG *ctaConfig)
{
    // Bug fix
    // no lock set because 
    // 1) it only happens once every 8 hours
    // 2) I'm afraid of deadlocks and don't have the time to work it out.

    arrivalList.pauseTime.tv_sec = ctaConfig->arrLen;
    arrivalList.pauseTime.tv_usec = 0;
}

// ------------------------------------------------------------------------

void dispSetAlertTime (CTA_CONFIG *ctaConfig)
{
    // Bug fix
    // no lock set because 
    // 1) it only happens once every 8 hours
    // 2) I'm afraid of deadlocks and don't have the time to work it out.

    alertList.pauseTime.tv_sec = ctaConfig->alrtLen;
    alertList.pauseTime.tv_usec = 0;
}
