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
#ifndef display_h
#define display_h

#include <stdint.h>
#include "font.h"

#define ROWS  32                // real number of rows
#define COLS  192               // real number of columns
#define VROWS 48                // what the scanboard thinks
#define VCOLS 224               // what the scanboard thinks

typedef enum
{
    PORTAL_TEXT,
    PORTAL_GRAPHIC
} PORTAL_TYPE;

typedef enum
{
    EDISP_RANGE,
    EDISP_INVAL,
    EDISP_FONTERR,
} DISP_ERROR_TYPE;

typedef enum
{
    MAX_PORTALS = 20,        // arbitrary, don't want too many or too few
    MAX_STRING  = COLS,      // This is too long in reality
    MAX_GRAPHIC = COLS * ROWS,
} PORTAL_LIMITS;

typedef struct
{
    const FONT *font;           // The font used to render the text
    char data[MAX_STRING];      // The text string to be rendered
    size_t size;                // length of the text string
} TEXT_DESCRIPTOR;

typedef struct
{
    uint8_t data[MAX_GRAPHIC];  // pointer to bitmap of graphic.
    size_t size;                // size of graphic
} GRAPHIC_DESCRIPTOR;

typedef enum
{
    P_LEFT,
    P_CENTER,
    P_RIGHT,
} PORTAL_JUSTIFY;

typedef struct portal
{
    struct portal *next;
    PORTAL_JUSTIFY just;
    PORTAL_TYPE type;
    int id;
    int upperLeftRow;
    int upperLeftCol;
    int startCol;               // after justification 
    int width;                  // in columns
    int height;                 // in rows
    int scroll;
    int scrollDelay;
    union
    {
        TEXT_DESCRIPTOR text;
        GRAPHIC_DESCRIPTOR graphic;
    } content;
} PORTAL;


// ------------------------------------------------------------------
// Initialize the display buffer and associated structures. Return
// 0 for ok and -1 for error.
//
void dispInit();

// ------------------------------------------------------------------
// Call this before drawing portals for a new display. It just 
// clears the display buffer so there won't be anything left over.
//
void dispClear ();

// ------------------------------------------------------------------
// Deallocate the display buffer and associated structures.
//
void dispDelete();

// ------------------------------------------------------------------
// Add a portal to the list of portals on the display. Portals are
// drawn in the order in which they're defined. Portals defined later
// override portals defined earlier, so if one portal overlaps
// another, the one define last wins. This returns a pointer to the
// portal, or 0 on error.
//

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
    int scrollDelay);         // delay in ms between column shifts

// ------------------------------------------------------------------
// Deallocate an existing portal
//
int dispPortalDel (PORTAL *p);

// ------------------------------------------------------------------
// Puts the content of a portal into the display buffer at the spot
// specified by the portal struct.
//
int dispDrawPortal (PORTAL *p);

// ------------------------------------------------------------------
// Returns a const pointer to the indicated row of the display buffer.
// This array is VCOLS * 3 bytes wide.
//
const uint8_t *dispGetDisplayRow (int r);

// ------------------------------------------------------------------
// Returns a string identifying the last error, or "" if no error
//
const char *dispGetLastErrStr();

// ------------------------------------------------------------------
// Changes the text being displayed in a portal
int dispSetPortalText (PORTAL *p, const char *t);

// ------------------------------------------------------------------
// Returns an integer identifying the last error, or 0 if no error
//
int dispGetLastErr();

void dispDumpDisplay ();
#endif
