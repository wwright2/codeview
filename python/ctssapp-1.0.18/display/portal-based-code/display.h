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
//  07/28/11    Joe Halpin      2       Refactoring
//
//  Notes: The screen buffer definition is bigger than the sign
//  display because the scanboard apparently has been defined with a
//  larger display, and that's not going to change. We still only use
//  the upper-left 192x32 pixel area.
//
// ***************************************************************************
#ifndef display_h
#define display_h

#include <pthread.h>
#include <stdint.h>
#include "common.h"
#include "font.h"
#include "alerts.h"
#include "portal.h"
#include "ctaConfig.h"

#define DISP_ROWS  32           // real number of rows
#define DISP_COLS  192          // real number of columns
#define VROWS 48                // what the scanboard thinks
#define VCOLS 224               // what the scanboard thinks

//
// There are two of these in the display code, double buffering so one can be
// prepared while another one is being displayed.
//

typedef struct
{
    pthread_mutex_t lock;
    uint8_t buf[VROWS][VCOLS][3];
    uint8_t active;
    uint8_t brightnessMsg[5];
} DISP_DATA;

typedef enum
{
    EDISP_RANGE,
    EDISP_INVAL,
    EDISP_FONTERR,
} DISP_ERROR_TYPE;

#define LINES_PER_PAGE        4

#define ARR_PORTALS_PER_PAGE  12
#define ARR_PORTALS_PER_LINE  3
#define ARR_PAGES_PER_SET     4

#define ALRT_PORTALS_PER_PAGE 4
#define ALRT_PAGES_PER_SET    4

#define ERR_PORTALS_PER_PAGE  4
#define ERR_PAGES_PER_SET     1

#define BUS_COL 0
#define BUS_WIDTH 28

#define ROUTE_COL 30
#define ROUTE_WIDTH 141

#define DUE_COL 172
#define DUE_WIDTH 20


extern GLYPH glyphs[256];
extern DISP_DATA *activeDisplay;

extern pthread_mutex_t alertReadyLock;

// ------------------------------------------------------------------
// Initialize the display buffer and associated structures. Return
// 0 for ok and -1 for error. Just call this once in main()
//
void dispInit();
void dispCleanup ();

// ------------------------------------------------------------------
// Call this before drawing portals for a new display. It just 
// clears the display buffer so there won't be anything left over.
//
void dispClear ();

// ------------------------------------------------------------------
// Clears the display buffer and associated structures.
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
    const void *data,         // text string or bitmap data. not 0 terminated
    FONT *font,               // null for graphic data
    size_t length);           // length of data

// ------------------------------------------------------------------
// Deallocate an existing portal
//
int dispPortalDel (PORTAL *p);

// ------------------------------------------------------------------
// Renders the portal group onto the display. This makes the display buffer
// ready to send to the scan board. 

// ------------------------------------------------------------------
// Puts the content of a portal into the display buffer at the spot
// specified by the portal struct.
//
int dispDrawPortal (PORTAL *p);

// ------------------------------------------------------------------
// Returns a const pointer to the indicated row of the display buffer.
// This array is DISP_COLS * 3 bytes wide.
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

// ------------------------------------------------------------------
//
uint8_t *getBrightnessMsg (size_t *size);

// ------------------------------------------------------------------
// Tells the display code what value to send in the brightness message. This is
// called by the main loop to set the value. The display thread calls
// getBrightnessMsg() when sending the message. 
//
void dispSetBrightness (int b);

// ------------------------------------------------------------------
//
// ------------------------------------------------------------------------

uint8_t *getBrightnessMsg (size_t *size);

// ------------------------------------------------------------------------

void dispSetBrightness (int b);

// ------------------------------------------------------------------------

void dispSwitchDisplays ();

// ------------------------------------------------------------------------
// Called by ctaConfig.c to set new values
//
void dispSetBlankTime (CTA_CONFIG *ctaConfig);
void dispSetArrivalTime (CTA_CONFIG *ctaConfig);
void dispSetAlertTime (CTA_CONFIG *ctaConfig);

// ------------------------------------------------------------------
// This tells the display thread how long to pause once it gets done sending a
// screen full of data, before trying to send the next one.
//
size_t getPageDelay ();

// ------------------------------------------------------------------
// debug
//
void dispDumpDisplay ();
void dispDumpNextDisplay ();

// ------------------------------------------------------------------
//

void dispSwitchDisplays ();



// ------------------------------------------------------------------------

extern int curDisplay;
extern int nextDisplay;
extern DISP_DATA dispData[2];

// ------------------------------------------------------------------------

#define dispLockCurDisplay_q() \
{ \
    pthread_mutex_lock (&dispData[curDisplay].lock); \
}

// ------------------------------------------------------------------------

// quiet versions of lock/unlock macros
#define dispUnlockCurDisplay_q() \
{ \
    pthread_mutex_unlock (&dispData[curDisplay].lock); \
}


// ------------------------------------------------------------------------

#ifdef THR_DEBUG
#define dispLockCurDisplay() \
{ \
    logErr ("dispLockCurDisplay: locking %d %s %d", \
        curDisplay, __FILE__, __LINE__); \
    pthread_mutex_lock (&dispData[curDisplay].lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockCurDisplay() \
{ \
    logErr ("dispUnlockCurDisplay: unlocking %d %s %d", \
        curDisplay, __FILE__, __LINE__); \
    pthread_mutex_unlock (&dispData[curDisplay].lock); \
}

// ------------------------------------------------------------------------

#define dispLockNextDisplay() \
{ \
    logErr ("dispLockNextDisplay: locking %d %s %d", \
        nextDisplay, __FILE__, __LINE__); \
    pthread_mutex_lock (&dispData[nextDisplay].lock);\
}

// ------------------------------------------------------------------------

#define dispUnlockNextDisplay() \
{ \
    logErr ("dispUnlockNextDisplay: unlocking %d, %s, %d", \
        nextDisplay, __FILE__, __LINE__); \
    pthread_mutex_unlock (&dispData[nextDisplay].lock); \
}

// ------------------------------------------------------------------------

#define dispLockArrivalList()\
{ \
    logErr ("locking arrival list: %s %d",__FILE__,__LINE__);   \
    pthread_mutex_lock (&arrivalList.lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockArrivalList()\
{ \
    logErr ("unlocking arrivalList %s %d",__FILE__,__LINE__);   \
    pthread_mutex_unlock (&arrivalList.lock); \
}

// ------------------------------------------------------------------------

#define dispLockAlertList() \
{ \
    logErr ("locking alert list: %s %d",__FILE__,__LINE__);     \
    pthread_mutex_lock (&alertList.lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockAlertList() \
{ \
    logErr ("unlocking alertList: %s %d",__FILE__,__LINE__);    \
    pthread_mutex_unlock (&alertList.lock); \
}

// ------------------------------------------------------------------------

#define dispLockErrorList() \
{ \
    logErr ("locking error list: %s %d",__FILE__,__LINE__);     \
    pthread_mutex_lock (&errorList.lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockErrorList() \
{ \
    logErr ("unlocking errorList: %s %d",__FILE__,__LINE__);    \
    pthread_mutex_unlock (&errorList.lock); \
}

// ------------------------------------------------------------------------

#define dispLockBlankList() \
{ \
    logErr ("locking blank list: %s %d",__FILE__,__LINE__);     \
    pthread_mutex_lock (&blankList.lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockBlankList() \
{ \
    logErr ("unlocking blankList: %s %d",__FILE__,__LINE__);    \
    pthread_mutex_unlock (&blankList.lock); \
}

// ------------------------------------------------------------------------

#define dispLockAlertReady()                  \
{                                                                 \
    logErr ("locking alertReadyLock: %s %d",__FILE__,__LINE__);   \
    pthread_mutex_lock (&alertReadyLock);                         \
}

// ------------------------------------------------------------------------

#define dispUnlockAlertReady() \
{ \
    logErr ("unlocking alertReadyLock: %s %d",__FILE__,__LINE__);    \
    pthread_mutex_unlock (&alertReadyLock); \
}

#else
// ------------------------------------------------------------------------

#define dispLockCurDisplay() \
{ \
    pthread_mutex_lock (&dispData[curDisplay].lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockCurDisplay() \
{ \
    pthread_mutex_unlock (&dispData[curDisplay].lock); \
}

// ------------------------------------------------------------------------

#define dispLockNextDisplay() \
{ \
    pthread_mutex_lock (&dispData[nextDisplay].lock);\
}

// ------------------------------------------------------------------------

#define dispUnlockNextDisplay() \
{ \
    pthread_mutex_unlock (&dispData[nextDisplay].lock); \
}

// ------------------------------------------------------------------------

#define dispLockArrivalList()\
{ \
    pthread_mutex_lock (&arrivalList.lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockArrivalList()\
{ \
    pthread_mutex_unlock (&arrivalList.lock); \
}

// ------------------------------------------------------------------------

#define dispLockAlertList() \
{ \
    pthread_mutex_lock (&alertList.lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockAlertList() \
{ \
    pthread_mutex_unlock (&alertList.lock); \
}

// ------------------------------------------------------------------------

#define dispLockErrorList() \
{ \
    pthread_mutex_lock (&errorList.lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockErrorList() \
{ \
    pthread_mutex_unlock (&errorList.lock); \
}

// ------------------------------------------------------------------------

#define dispLockBlankList() \
{ \
    pthread_mutex_lock (&blankList.lock); \
}

// ------------------------------------------------------------------------

#define dispUnlockBlankList() \
{ \
    pthread_mutex_unlock (&blankList.lock); \
}

// ------------------------------------------------------------------------

#define dispLockAlertReady() \
{ \
    pthread_mutex_lock (&alertReadyLock); \
}

// ------------------------------------------------------------------------

#define dispUnlockAlertReady() \
{ \
    pthread_mutex_unlock (&alertReadyLock); \
}

#endif

void alertSetData (const uint8_t *s, size_t len);

#endif
