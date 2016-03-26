// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       win24Funcs.c
//
//  Description:    Functions used by window.c when managing 24-bit color depth
//                  windows.
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  03/26/2012  Joe Halpin      1
//  Notes:
//
// ***************************************************************************

#ifndef WIN24FUNCS_H_INCLUDED
#define WIN24FUNCS_H_INCLUDED
#define WIN_FUNCS_HEADER

#include <stdint.h>

#include "sysGlobals.h"
#include "windowData.h"

void win24ClearColumn (
    WINDOW_DATA *wd,
    uint8_t ***dst,
    int dstCol);

void win24SetColumn (
    WINDOW_DATA *wd,
    uint8_t **src, 
    uint8_t ***dst,
    int srcCol,
    int dstCol);

void win24ClearRow (
    WINDOW_DATA *wd,
    uint8_t ***disp,
    int dstRow, int dstStart, uint32_t width);

void win24SetRow (
    WINDOW_DATA *wd,
    uint8_t **renderBuf,
    uint8_t ***disp,
    int srcRow,
    int dstRow,
    int dstStart, uint32_t width);

#endif

