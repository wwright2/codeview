// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       winRender.h
//
//  Description:    Functions used by window.c for rendering stuff.
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  03/26/2012  Joe Halpin      1
//  Notes:
//
// ***************************************************************************

#ifndef WINRENDER_H_INCLUDED
#define WINRENDER_H_INCLUDED
#define WIN_FUNCS_HEADER

#include <stdint.h>

#include "sysGlobals.h"
#include "windowData.h"

int winSetData (
    WINDOW *w,
    WIN_DATA_TYPE type,
    uint8_t align,
    uint32_t effect,
    uint8_t color[3],
    void *data,
    int size,
    int ttl);

//typedef uint8_t w24Pixel[3];

WIN_TTL_STATE winPrep (WINDOW *);
void winInit (WINDOW *w);

#endif

