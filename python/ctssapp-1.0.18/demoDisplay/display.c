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
//  Filename:       display.h
//
//  Description:    Public interface for display code
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  04/19/11    Joe Halpin      1       
//
//  Notes: The screen buffer definition is bigger than the sign
//  display because the scanboard apparently has been defined with a
//  larger display, and that's not going to change. We still only use
//  the upper-left 192x32 pixel area.
//
// ***************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "display.h"
#include "font.h"


// ------------------------------------------------------------------------
// Private data
// ------------------------------------------------------------------------

//
// the display buffer is way bigger than the real display. The
// scanboard firmware was apparently written for these screen
// dimensions and it's not going to change, so we need to send them
// all the buffer they want. We must only count on the upper left
// signWidth/signHeight pixels being used.
//

static uint8_t dispBuf[VROWS][VCOLS][3];

//
// Each of these structs defines a region of the screen within which
// something will be displayed.
//

static PORTAL  portalBuf[MAX_PORTALS];  // memory for portal structs
static PORTAL *portals[MAX_PORTALS];    // list of portal structs in use
static const char *lastErrMsg = "";
static int lastErr = 0;

//
// These are working buffers. They are sized to handle the longest headsign
// that we know about at this point, plus some additional room. It's hard to be
// exact because the font we're using is proprotional, and the strings can
// change. 
//
// These buffers are used to pre-render strings which are intended to be
// scrolled. The pre-rendered strings are then copied by columns into the
// display buffer to achieve scrolling.
//

#define LONGEST_STRING 32
#define AVG_GLYPH_WIDTH 7       // this is a little more than average...
#define NORMAL_LINE_HEIGHT 8

uint8_t line0Buf[NORMAL_LINE_HEIGHT][LONGEST_STRING * AVG_GLYPH_WIDTH][3];
uint8_t line1Buf[NORMAL_LINE_HEIGHT][LONGEST_STRING * AVG_GLYPH_WIDTH][3];
uint8_t line2Buf[NORMAL_LINE_HEIGHT][LONGEST_STRING * AVG_GLYPH_WIDTH][3];
uint8_t line3Buf[NORMAL_LINE_HEIGHT][LONGEST_STRING * AVG_GLYPH_WIDTH][3];


// ------------------------------------------------------------------------
// Code
// ------------------------------------------------------------------------

static void setError (int e, const char *s)
{
    lastErr = e;
    lastErrMsg = s;
}

// ------------------------------------------------------------------------

static PORTAL *getPortal()
{
    PORTAL *tmp = 0;

    for (int i = 0; i < MAX_PORTALS; ++i)
    {
        if (portals[i] == 0)
        {
            tmp = &portalBuf[i];
            memset(tmp, 0, sizeof *tmp);
            tmp->id = i;
            portals[i] = &portalBuf[i];
            break;
        }
    }
    return tmp;
}

// ------------------------------------------------------------------------

static void copyText (
    PORTAL *p, 
    const char *data, 
    int length)
{
    //
    // While copying the text check to be sure it will fit into the number of
    // columns alloted to the portal. Break it at a letter boundary if not.
    //

    if (length > MAX_STRING)
        length = MAX_STRING;

    int colsAvailable = p->width; // columns in the portal
    int colsUsed      = 0;

    p->content.text.size = (size_t) length;

    for(int i = 0; i < length; ++i)
    {
        int chCols = glyphs[(int)data[i]].width;

        if (colsUsed + chCols > colsAvailable)
        {
            p->content.text.size = i;
            break;
        }

        colsUsed += chCols;
        p->content.text.data[i] = data[i];
    }
}

// ------------------------------------------------------------------------

static void copyGraphic (PORTAL *p, const uint8_t *data, int length)
{
    if (length > MAX_GRAPHIC)
        length = MAX_GRAPHIC;

    p->content.graphic.size = (size_t) length;
    for (int i = 0; i < length; ++i)
        p->content.graphic.data[i] = data[i];
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

void dispClear ()
{
    memset(dispBuf, 0, sizeof dispBuf);
}

// ------------------------------------------------------------------------

void dispInit()
{
    // This can be called more than once, so don't assume that default
    // initialization is in effect.
    memset(dispBuf, 0, sizeof dispBuf);
    memset(portalBuf, 0, sizeof portalBuf);
    memset(portals, 0, sizeof portals);
}

// ------------------------------------------------------------------------

void dispDelete()
{
    memset (portalBuf, 0, sizeof portalBuf);
    memset (portals, 0, sizeof portals);
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

static const char *portalType (PORTAL_TYPE t)
{
    switch (t)
    {
        case PORTAL_TEXT:
            return "TEXT";
        case PORTAL_GRAPHIC:
            return "GRAPHIC";
    }
    return "";
}

// ------------------------------------------------------------------------

static const char *getText (PORTAL *p)
{
    static char buf[1024];
    int i;

    if (p->content.text.size >= sizeof buf)
        return "too big";

    for (i = 0; i< p->content.text.size; ++i)
        buf[i] = p->content.text.data[i];
    buf[i] = 0;
    return buf;
}

// ------------------------------------------------------------------------

void dumpPortal (PORTAL *p)
{
    printf ("   just = %s\n", justText(p->just));
    printf ("   type = %s\n", portalType (p->type));
    printf ("   id   = %d\n", p->id);
    printf ("   upperLeftRow = %d\n", p->upperLeftRow);
    printf ("   upperLeftCol = %d\n", p->upperLeftCol);
    printf ("   startCol     = %d\n", p->startCol);
    printf ("   width        = %d\n", p->width);
    printf ("   height       = %d\n", p->height);
    printf ("   text         = %s\n", getText (p));
    printf ("\n");
}

// ------------------------------------------------------------------------

PORTAL *dispPortalAdd(
    PORTAL_JUSTIFY just,      // How to justify the text
    int ulRow,                // upper left row where portal starts
    int ulCol,                // upper left column where portal starts
    int width,                // width of portal in columns
    int height,               // height of portal in rows
    PORTAL_TYPE type,         // TEXT or GRAPHIC
    const void *data,         // text string or bitmap data. not 0 terminated
    FONT *font,               // null for graphic data
    size_t length,            // length of data
    int scroll,               // boolean 1/0 = scroll/no scroll
    int scrollDelay)          // delay in ms between column shifts
{
    PORTAL *tmp = getPortal();

    if(tmp == 0)
    {
        setError (EDISP_RANGE, "Out of portals in portalAdd()");
        return 0;
    }

    tmp->just = just;
    tmp->type = type;
    tmp->upperLeftRow = ulRow;
    tmp->upperLeftCol = ulCol;
    tmp->width = width;
    tmp->height = height;
    tmp->scroll = scroll;
    tmp->scrollDelay = scrollDelay;
    tmp->content.text.font = font;

    if (type == PORTAL_TEXT)
        copyText (tmp, data, length);
    else if (type == PORTAL_GRAPHIC)
        copyGraphic (tmp, data, length);
    else
    {
        setError (EDISP_INVAL, "Invalid parameter passed to portalAdd()");
        portals[tmp->id] = 0;
        return 0;
    }

    return tmp;
}

// ------------------------------------------------------------------------

int dispSetPortalText (PORTAL *p, const char *t)
{
    if ((p == 0) || (t == 0))
    {
        setError (EDISP_INVAL, "Null pointer passed to dispSetPortalText()");
        return -1;
    }

    p->content.text.size = strlen (t);
    if (p->content.text.size > sizeof p->content.text.data)
        p->content.text.size = sizeof p->content.text.data;

    memset (p->content.text.data, 0, sizeof p->content.text.data);
    for (int i = 0; i < p->content.text.size; ++i)
    {
        copyText (p, t, p->content.text.size);
        //p->content.text.data[i] = t[i];
        //putchar (p->content.text.data[i]);
    }
    putchar ('\n');

    return 0;
}

// ------------------------------------------------------------------------

int dispPortalDel (PORTAL *p)
{
    int id = p->id;

    if ((id < 0) || (id >= (sizeof portals / sizeof portals[0])))
    {
        setError (EDISP_RANGE, "invalid id in portalDel()");
        return -1;
    }

    portals[id] = 0;
    return 0;
}

// ------------------------------------------------------------------------

static void drawGlyph (
    const GLYPH *g, int *startCol, const PORTAL *p, int maxCol)
{
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
        
        int row = p->upperLeftRow + r;
        uint8_t byte = g->data[r];

        for (col = *startCol; 
             (col < *startCol + g->width) && (col < maxCol); 
             ++col)
        {
            if (byte & 0x80)
            {
                dispBuf[row][col][0] = 255;
                //putchar ((dispBuf[row][col][0]) ? '*' : ' ');
            }
            else
            {
                dispBuf[row][col][0] = 0;
                //putchar ((dispBuf[row][col][0]) ? '*' : ' ');
            }
            byte <<= 1;
        }
        //putchar ('\n');
    }

    *startCol = col;
}

// ------------------------------------------------------------------------

static void setJustification (PORTAL *p, GLYPH *g, int gsize)
{
    //
    // Adjust the start point per the justification setting.
    //

    int glyphCols  = 0;
    int portalCols = p->width;
    for (int i = 0; i < gsize; ++i)
        glyphCols += g[i].width;

    p->startCol = p->upperLeftCol;
    switch (p->just)
    {
        case P_LEFT:
            break;

        case P_RIGHT:
            if (portalCols > glyphCols)
                p->startCol = (portalCols - glyphCols) + p->upperLeftCol;
            break;
            
        case P_CENTER:
            if (portalCols > glyphCols)
            {
                int tmp = portalCols - glyphCols;
                if (tmp %2 != 0)
                    ++tmp;
                p->startCol = (tmp / 2) + p->upperLeftCol;
            }
            break;
    }
}

// ------------------------------------------------------------------------

static int scrollPortal (PORTAL *p)
{
    return 0;
}

// ------------------------------------------------------------------------

int dispDrawPortal (PORTAL *p)
{
    if (p->type != PORTAL_TEXT)
    {
        setError (EDISP_INVAL, "Invalid portal type in dispDrawPortal");
        return -1;
    }

    if (p->scroll == 1)
        return scrollPortal (p);

    GLYPH g[p->content.text.size];

    //
    // Cycle through the letters in the text to be displayed. We have
    // to resolve justification before rendering the portal. Collect
    // all the glyphs, add up their widths, and move the start points
    // to the appropriate column.
    //

    for (int i = 0; i < p->content.text.size; ++i)
    {
        // get the next letter to render
        char letter = p->content.text.data[i];

        // get the glyph for that letter
        if (fntGetGlyph (p->content.text.font, &g[i], letter) != 0)
        {
            setError (EDISP_FONTERR, "Error getting glyph in dispDrawPortal");
            return -1;
        }
    }

    setJustification (p, g, sizeof g / sizeof g[0]);

    //
    // Draw the portal
    //

    int col = p->startCol;
    int maxCol = p->startCol + p->width;
    for (int i = 0; i < p->content.text.size; ++i)
        drawGlyph (&g[i], &col, p, maxCol);

    //dumpPortal (p);
    return 0;
}

// ------------------------------------------------------------------------

const uint8_t *dispGetDisplayRow (int r)
{
    if (r < 0 || r > VROWS)
        return 0;

    return (uint8_t *)dispBuf[r];
}

// ------------------------------------------------------------------------

void dispDumpDisplay ()
{
    for (int r = 0; r < ROWS; ++r)
    {
        for (int c = 0; c < COLS; ++c)
        {
            putchar ((dispBuf[r][c][0]) ? '*' : ' ');
        }
        putchar ('\n');
    }
}

// ------------------------------------------------------------------------

static int testNum;
static int rowNum;
static int colNum;
static const int rowTest = 0;
static const int colTest = 1;
static const int numTest = 2;

void initPOST ()
{
    memset (dispBuf, 0, sizeof dispBuf);
    testNum = 0;
    rowNum  = 0;
    colNum  = 0;
}

// ------------------------------------------------------------------------

int setNextPOSTDisplay ()
{
    if (testNum == rowTest)
    {
        for (int i = 0; i < COLS; ++i)
        {
            dispBuf[rowNum][i][1] = 255;
        }

        rowNum++;
        if (rowNum == ROWS)
        {
            rowNum = 0;
            colNum = 0;
            testNum++;
            sleep (3); // Pause between tests
            goto done;
        }

        return 1;
    }

    else if (testNum == colTest)
    {
        for (int i = 0; i < ROWS; ++i)
        {
            dispBuf[i][colNum][1] = 0;
        }

        colNum++;
        if (colNum == COLS)
        {
            rowNum = 0;
            colNum = 0;
            testNum++;
            sleep (3); // Pause between tests
            goto done;
        }

        return 1;
    }

    else if (testNum > numTest)
        testNum = 0;

 done:
    return 0;
}
