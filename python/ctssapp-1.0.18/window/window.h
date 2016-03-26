// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       window.h
//
//  Description:    Declaration of window structures
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  03/16/2012  Joe Halpin      1
//  Notes:
//
// This uses object style C structs which are intended to be opaque to calling
// code. The inline function definitions are used to access the "member"
// functions of the window.
//
// The intention is to eventually have windows specialized for different color
// depths, row-based vs column-based fonts, and maybe some other stuff. Not all
// of that is implemented here though... Tick Tock
//
// A window is defined as a region of the entire display, given by the upper
// left x and y of the sign where the window should start, and the width in
// columns. For example, ulRow = 4, ulCol = 26, width = 27 puts the window at
// those coordinates of the sign display.
//
// When calling the winDraw() function, the second parameter is the output of
// gettimeofday() at the time of the call. This is used by the various window
// objects in the layout to determine if it's time to modify output for an
// effect. So, for example if a layout consists of three windows, one of which
// scrolls up every 10ms, one scrolls left every 15ms and the other is
// static. passing this in will cause the first to advance the scroll when the
// parameter shows it's been 10ms or more since last update, the second will do
// the same after 15ms, etc.
//
// ***************************************************************************


#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <sys/types.h>
#include <stdint.h>
#include <sys/time.h>

#include "font.h"
#include "winEnums.h"
#include "windowData.h"
#include "log.h"

// --------------------------------------------------------------------------
// Use the following functions to access window operations. Please do not try
// to access the data member directly. If you do, don't call me to fix it.
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Construct a window object
WINDOW *newWindow (
    const DISPLAY_PROFILE *dp,
    uint32_t startRow,	       // base row
    uint32_t startCol,	       // base column
    uint32_t width,	       // length of window in columns
    uint32_t height,	       // height of window in rows
    uint32_t offsetX,	       // offset into source buffer (graphics)
    uint32_t offsetY,
    WIN_DATA_TYPE type,	      // text or graphic
    uint8_t depth,	      // color depth - 24 bit supported initially
    const uint8_t *color,     // color to use
    const FONT *f,	      // default font 
    uint8_t align,	      // if none given LEFT is default
    uint32_t effect,
    uint32_t scroll,
    const char *data,        // data to display
    size_t dataLen,	     // length of data
    int updateInterval,	     // period between updates when scrolling etc
    int windowId,	     // id of window.
    int priority,	     // determines where window is in stacking order
    int ttl);		     // time to live in seconds, or -1

// --------------------------------------------------------------------------
// deallocate a window object. Do NOT just call free() on the pointer. There
// are opaque data structures involved, and the constructor may not have used
// malloc() to allocate the struct in the first place.
//
void delWindow (WINDOW *w);

// --------------------------------------------------------------------------
// Put the text/graphic for a window into the external buffer specified.
// The dst parameter refers to the start of the entire display buffer. The
// window needs to know where to draw itself within this.
WIN_TTL_STATE winPrep (WINDOW *w);

//int winDraw (WINDOW *w, PIXEL **dst);
int winDraw (WINDOW *w, uint8_t ***dst);

// --------------------------------------------------------------------------
// 
const char *winLastErrMsg ();
int winLastErrCode ();

uint64_t winGetUpdateTime (WINDOW *w);
int  winGetEffectDelay (WINDOW *w);
void winSetEffectDelay (WINDOW *w, int delay);
int  winSetText (WINDOW *w, const char *data);
int  winDrawAt (WINDOW *w, int row);
int  winGetRows (WINDOW *w);
int  winGetRow (WINDOW *w);
const char *winGetText (WINDOW *w);

#endif
