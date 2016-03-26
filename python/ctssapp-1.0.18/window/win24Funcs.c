// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       win24Func.c
//
//  Description:    Implementations.
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  03/26/2012  Joe Halpin      1
//  Notes:
//
// ***************************************************************************

#include <string.h>
#include <stdint.h>

#include "win24Funcs.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

// ---------------------------------------------------------------------------
// This clears a column in the destination buffer. The dst parameter points to
// the beginning of the display buffer, so we have to index into that to get
// our window position.
//
void win24ClearColumn (
    WINDOW_DATA *wd,
    uint8_t ***dst,
    int dstCol)
{
    for (int row = 0; row < wd->height; ++row)
    {
        dst[wd->ulRow + row][dstCol + wd->ulCol][0] = 0;
        dst[wd->ulRow + row][dstCol + wd->ulCol][1] = 0;
        dst[wd->ulRow + row][dstCol + wd->ulCol][2] = 0;
    }
}

// ---------------------------------------------------------------------------
// This fills a column in the destination buffer with the contents of the
// corresponding column in the source buffer. The c parameter is the column
// number in the source buffer. It must be added to the ulCol when addressing
// the destination buffer because the pointer to destination (dst) is the
// absolute start of the display buffer.
//
void win24SetColumn (
    WINDOW_DATA *wd,
    uint8_t **src, 
    DisplayBuf dst,
    int srcCol,
    int dstCol)
{
    for (int row = 0; row < wd->height; ++row)
    {
        
        if ((row < wd->font->height) && (src[row][srcCol] != 0))
        {
            dst[wd->ulRow + row][dstCol + wd->ulCol][0] = wd->color[0];
            dst[wd->ulRow + row][dstCol + wd->ulCol][1] = wd->color[1];
            dst[wd->ulRow + row][dstCol + wd->ulCol][2] = wd->color[2];
        }
        else
        {
            dst[wd->ulRow + row][dstCol + wd->ulCol][0] = 0;
            dst[wd->ulRow + row][dstCol + wd->ulCol][1] = 0;
            dst[wd->ulRow + row][dstCol + wd->ulCol][2] = 0;
        }
    }
}

// ---------------------------------------------------------------------------
//
void win24SetRow (
    WINDOW_DATA *wd,
    uint8_t **renderBuf,
    DisplayBuf disp,
    int srcRow,
    int dstRow,
    int dstStart,
    uint32_t width
) {
    int row = dstRow + wd->ulRow;

    for (int c = 0; (c < wd->winSize.width) && (c < width); ++c)
    {
        int col = c + dstStart + wd->ulCol;

        //printf ("row = %d col = %d dstStart = %d\n", row,col,dstStart);
	//printf ("%c", renderBuf[srcRow][c] ? '*' : ' ');

        if ((renderBuf[srcRow][c]) && (srcRow < wd->font->height))
        {
            disp[row][col][0] = wd->color[0];
            disp[row][col][1] = wd->color[1];
            disp[row][col][2] = wd->color[2];
        }

        else
        {
            disp[row][col][0] = 0;
            disp[row][col][1] = 0;
            disp[row][col][2] = 0;
        }
    }
    //printf ("\n");
}

// ---------------------------------------------------------------------------
//
void win24ClearRow (
    WINDOW_DATA *wd,
    DisplayBuf disp,
    int dstRow,
    int dstStart,
    uint32_t width
) {
    int row = dstRow + wd->ulRow;

    for (int c = 0; (c < wd->winSize.width) && (c < width); ++c)
    {
        int col = c + dstStart + wd->ulCol;

        //printf ("row = %d col = %d\n", row, col);
        disp[row][col][0] = 0;
        disp[row][col][1] = 0;
        disp[row][col][2] = 0;
    }
}

