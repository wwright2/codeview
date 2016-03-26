// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       winRender.c
//
//  Description:    Implementation of generic rendering functions like
//  scrolling left/right/center justification. Calls functions in a sign
//  specific file to put stuff into the output buffer.
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  03/26/2012  Joe Halpin      1
//  Notes:
//
// ***************************************************************************

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "window.h"
#include "windowData.h"
#include "graphic.h"
#include "log.h"
#include "helpers.h"

#include "textWindow.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define FLASH_DELAY 500000  /* microseconds */

//#define SCROLL_MASK (FX_SCROLL_LEFT|FX_SCROLL_RIGHT|FX_SCROLL_UP|FX_SCROLL_DOWN)
#define JUST_MASK (ALIGN_VMASK | ALIGN_HMASK)
#define EFFECTS_MASK (FX_FLASH | FX_CLEAR)

/****************************************************************************/

int isUpdateNeeded (WINDOW *win, uint64_t now) {
    WINDOW_DATA *wd = (WINDOW_DATA*)win->data;

    if (now > wd->state.nextFlashUpdate) {
        wd->state.flashState = !wd->state.flashState;
        wd->state.nextFlashUpdate = now + FLASH_DELAY;
    }

    if (now > wd->nextUpdate) {
        wd->nextUpdate += wd->updateInterval;
        wd->state.scrollNeeded = 1;
        return 1;
    }

    return 0;
}

/****************************************************************************/

static inline void clearWin (WINDOW_DATA *wd) {
    if (!wd->buf || !wd->bufsz) return;
    uint8_t *row = wd->buf;
    for (int Y = wd->dim.src.height; Y; Y--) {
        memset(row, 0, wd->dim.src.width * ((wd->depth+7)>>3));
        row += wd->dim.src.stride;
    }
}

/****************************************************************************/

#if 0
void drawDisplay (const WINDOW_DATA *wd, const uint8_t ***dst)
{
    for (int r = 0; r < wd->dp.rows; ++r)
    {
        printf("[");
        for (int c = 0; c < wd->dp.cols; ++c)
        {
            int p;
            for (p = 0; p < wd->dp.clrBytes; ++p)
            {
                if ((dst[r][c][p] > 0))
                {
                    printf ("*");
                    break;
                }
            }

            if (p == wd->dp.clrBytes)
                printf (" ");
        }
        printf ("]\n");
    }
    printf ("\n");
}
#endif

/****************************************************************************/

static roi_t drawGlyph (
    const FONT *f, 
    char c, 
    const uint8_t *color, 
    int clrBytes,
    uint8_t *drow, 
    roi_t region) 
{
    roi_t draw_size = { 0, 0, 0 };

    if ((c < f->startIndex) || (c > (f->startIndex + f->numGlyphs))) {
        LOGWARN("character %c not in font", c);
        return draw_size;
    }

    GLYPH *g = f->offsets[(int)c];
    if (g == NULL) {
        LOGWARN("Charicter '%c' does not exist in font", c);
        return draw_size;
    }

    draw_size.width = g->width;
    draw_size.height = g->height;

    if (region.height < draw_size.height)
        draw_size.height = region.height;

    for (int row = 0; row < draw_size.height; row++) 
    {
        uint8_t *dst = drow;
        for (int col = 0; col < draw_size.width; col++) 
        {
            if (!(g->data[row][col + g->startCol])) 
            {
                //for (int c = 0; c < clrBytes; ++c)
                    //*dst++ = color[c];
                *dst++ = 0;
                *dst++ = 0;
                *dst++ = 0;
                continue;
            }

            //for (int c = 0; c < clrBytes; ++c)
            //    *dst++ = color[c];

            *dst++ = color[0];
            *dst++ = color[1];
            *dst++ = color[2];

        }
        drow += region.stride;
    }
    return draw_size;
}

/****************************************************************************/

static roi_t drawRenderBuf (WINDOW_DATA *wd) 
{
    roi_t size = wd->dim.src;
    LOGTRACEW ("Entered drawRenderBuf (%dx%d)", size.width, size.height);

    if (wd->type != TEXT) return size;
    if (wd->depth != wd->dp.clrBytes) 
    {
        LOGTRACEW("wd->depth = %d\n", wd->dp.clrBytes);
        return (roi_t){0,0};
    }

    uint32_t max_width = 0;
    uint8_t *drow = wd->buf;
    char *ptr = wd->data;

    while (*ptr && (size.height > 0)) 
    {
        uint8_t *dst = drow;
        size.width = wd->dim.src.width;
        uint32_t row_height = 0;

        if (wd->align & (ALIGN_CENTER | ALIGN_RIGHT)) 
        {
            roi_t line_size = determineTextLineSize(ptr, wd->font, NULL);
            LOGTRACEW("Buffer size is %dx%d for line '%s'", 
		line_size.width, line_size.height, ptr);
            uint32_t rem = size.width - line_size.width;
            if (wd->align & ALIGN_CENTER)
                rem >>= 1;

            size.width -= rem;

            // I'm guessing the 3 here is for 3 byte color
            //dst += rem * 3;
            dst += rem * wd->dp.clrBytes;
        }

        for (; *ptr && (*ptr != '\n') && (size.width > 0); ptr++) 
        {
            const char *expansion = interpolate_variable((const char **)&ptr);
            if (expansion != NULL) 
            {
                for (; *expansion; expansion++) 
                {
                    LOGTRACEW("drawing expanded variable char '%c'", *expansion);
                    roi_t ds = drawGlyph(
                        wd->font, 
                        *expansion, 
                        wd->color, 
                        wd->dp.clrBytes,
                        dst, 
                        size);
                    size.width -= ds.width;
                    if (ds.height > row_height) row_height = ds.height;
                    //dst += ds.width * 3;
                    dst += wd->dp.clrBytes;
                }
                wd->effect |= FX_MARKUP;
                continue;
            }

            /* none known, draw $ */
            roi_t ds = drawGlyph(
                wd->font, 
                *ptr, 
                wd->color, 
                wd->dp.clrBytes, 
                dst, 
                size);
            size.width -= ds.width;
            if (ds.height > row_height) row_height = ds.height;
            //dst += ds.width * 3;
            dst += ds.width * wd->dp.clrBytes;
        }

        uint32_t row_width = wd->dim.src.width - size.width;
        if (max_width < row_width) max_width = row_width;

        if (!*ptr || (*ptr == '\n')) 
        {
            drow += (wd->dim.src.stride * (row_height + 1));
            size.height -= row_height;
            if (*ptr == '\n') 
            {
                ptr++;
                size.height--;
            }
        }
    }

    if (*ptr && (size.height <= 0))
        LOGTRACEW ("warning, exceeded output buffer in draw render buf!");

    roi_t draw_size = wd->dim.src;
    draw_size.width = max_width;
    draw_size.height -= size.height;

    LOGTRACEW ("<-- leaving drawRenderBuf (%dx%d)", 
	draw_size.width, draw_size.height);

    return draw_size;
}

/****************************************************************************/

void winInit (WINDOW *w)
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;

    LOGTRACEW ("in winInit: effect = %04x", wd->effect);
    clearWin (wd);
    roi_t size = drawRenderBuf (wd);
    wd->dim.src = size;

    if (wd->scroll & FX_SCROLL_VIEWPORT) return;

    if (wd->scroll & FX_SCROLL_MASK) 
    {
        if (wd->scroll & FX_SCROLL_LEFT)
            wd->offset.src.X = (wd->dim.src.width < wd->dim.dst.width) ? -wd->dim.src.width : -wd->dim.dst.width;
        else if (wd->scroll & FX_SCROLL_RIGHT)
            wd->offset.src.X = (wd->dim.src.width < wd->dim.dst.width) ? wd->dim.src.width : wd->dim.dst.width;

        if (wd->scroll & FX_SCROLL_UP)
            wd->offset.src.Y = (wd->dim.src.height < wd->dim.dst.height) ? wd->dim.src.height : wd->dim.dst.height;
        else if (wd->scroll & FX_SCROLL_DOWN)
            wd->offset.src.Y = (wd->dim.src.height < wd->dim.dst.height) ? -wd->dim.src.height : -wd->dim.dst.height;
    }

}

/****************************************************************************/

WIN_TTL_STATE winPrep (WINDOW *w)
{
    uint64_t now = getUSecs ();

    if ((w->ttl > 0) && (w->ttl < now))
        return WIN_TTL_EXCEEDED;

    //
    // This is causing grief with flashing. I'm not sure why but the time the
    // text is visible is barely noticable, and it's blank for the rest of the
    // time. It needs to update every time through. Call isUpdateNeeded() to
    // set the on/off flag for the flash function, but always return 1.
    //

    isUpdateNeeded(w, now);
    w->updateNeeded = 1;

    return WIN_TTL_OK;
}

/****************************************************************************/

static void copyRegion (
    WINDOW_DATA *wd,
    uint8_t *drow, 
    roi_t droi, 
    uint8_t *srow, 
    roi_t sroi) 
{
    LOGTRACEW ("Copying region of size %dx%d from %p (stride %u) to %p "
	"(%dx%d, stride %u)", 
	sroi.width, sroi.height, srow, sroi.stride, drow, droi.width, 
	droi.height, droi.stride);

    if (droi.width  < sroi.width)  sroi.width  = droi.width;
    if (droi.height < sroi.height) sroi.height = droi.height;

    LOGTRACEW("--> Adjusted copy region: %dx%d", sroi.width, sroi.height);

    if (!sroi.height || !sroi.width) 
    {
        LOGTRACEW ("!!! Nothing to copy", 0);
        return;
    }

    for (; sroi.height; sroi.height--) 
    {
        uint8_t *src = srow;
        uint8_t *dst = drow;
        for (uint32_t W = sroi.width; W; W--) 
        {
            for (int c = 0; c < wd->dp.clrBytes; ++c)
                *dst++ = *src++;
        }

        drow += droi.stride;
        srow += sroi.stride;
    }
    return;
}

/******************************************************************************/

static inline pos_t determineAlignment (WINDOW_DATA *wd) 
{
    /* ADJUST FOR ALIGNMENT */
    roi_t roi = wd->dim.src;
        LOGTRACEW ("determineAlignment: roi = %dx%dx%d\n", roi.width,roi.height,roi.stride);
    
    pos_t pos = (pos_t){ 0, 0 };
    roi_t wsize = wd->dim.dst;
    LOGTRACEW ("Determining alignment buf %dx%d, dst: %dx%d", 
	roi.width, roi.height, wsize.width, wsize.height);
    uint32_t align = wd->align;
    uint32_t scroll = wd->scroll;
    
    pos.X -= wd->offset.src.X;
    pos.Y -= wd->offset.src.Y;
    
    if (!(align & ALIGN_MASK)) return pos;
    
    if (align & ALIGN_HMASK) do {
        if ((scroll & FX_SCROLL_HMASK) && (roi.width > wsize.width)) break;
        if (align & ALIGN_LEFT) break;

        int32_t extra = wsize.width;
        extra -= roi.width;
        if (!extra) break;

        if (align & ALIGN_CENTER)
            extra >>= 1;

        pos.X += extra;

    } while (0);

    if (align & ALIGN_VMASK) do {
        if ((scroll & FX_SCROLL_VMASK) && (roi.height > wsize.height)) break;
        if (align & ALIGN_TOP) break;

        int32_t extra = wsize.height;
        extra -= roi.height;
        if (!extra) break;

        if (align & ALIGN_MIDDLE)
            extra >>= 1;

        pos.Y += extra;

    } while (0);

    /* END OF ALIGNMENT ADJUSTMENT */

    return pos;
}

/****************************************************************************/

static inline int determineScroll (WINDOW_DATA *wd) 
{
    uint32_t scroll = wd->scroll;
    int scroll_check = 0;

    int minX = -wd->dim.dst.width;
    int maxX = wd->dim.src.width;
    int minY = -wd->dim.dst.height;
    int maxY = wd->dim.src.height;

    if (!(scroll & FX_SCROLL_MASK)) return WIN_OK;

    if (scroll & FX_SCROLL_VIEWPORT) 
    {
        if (scroll & FX_SCROLL_COUNT_MASK) 
        {
            uint32_t count = scroll & FX_SCROLL_COUNT_MASK;
            count--;
            wd->scroll = (scroll & ~FX_SCROLL_COUNT_MASK) | (count & FX_SCROLL_COUNT_MASK);
            LOGTRACEW("Viewport scroll counter: %u", count);
            return WIN_OK;
        } else
            scroll = (scroll & ~FX_SCROLL_COUNT_MASK) | (3 & FX_SCROLL_COUNT_MASK);

        int xover = wd->dim.src.width - wd->dim.dst.width;
        int yover = wd->dim.src.height - wd->dim.dst.height;
        if ((xover <= 0) && (yover <= 0)) return WIN_OK;

        minX = 0;
        maxX = xover;
        minY = 0;
        maxY = yover;
    }

    if (scroll & FX_SCROLL_LEFT) 
    {
        wd->offset.src.X++;
        LOGTRACEW("scroll left (offsetX %d)", wd->offset.src.X);
        if (wd->offset.src.X >= maxX) { //(int32_t)wd->dim.src.width) {
            if (scroll & FX_SCROLL_VIEWPORT) {
                scroll &= ~FX_SCROLL_HMASK;
                scroll |= FX_SCROLL_RIGHT;
                scroll = (wd->scroll & ~FX_SCROLL_COUNT_MASK) | (32 & FX_SCROLL_COUNT_MASK);
            } 

            else 
            {
                wd->offset.src.X = minX; //-wd->dim.dst.width;
                scroll_check = 1;
            }
        }

    } 

    else if (scroll & FX_SCROLL_RIGHT) 
    {
        wd->offset.src.X--;
        LOGTRACEW("scroll right (offsetX %d)", wd->offset.src.X);
        if (wd->offset.src.X <= minX) 
        { 
            if (scroll & FX_SCROLL_VIEWPORT) 
            {
                scroll &= ~FX_SCROLL_HMASK;
                scroll |= FX_SCROLL_LEFT;
                scroll = (scroll & ~FX_SCROLL_COUNT_MASK) | (32 & FX_SCROLL_COUNT_MASK);
            } else {
                wd->offset.src.X = maxX; //wd->dim.src.width;
                scroll_check = 1;
            }
        }
    }

    if (scroll & FX_SCROLL_UP) 
    {
        wd->offset.src.Y++;
        LOGTRACEW("scroll up (offsetY %d)", wd->offset.src.Y);
        if (wd->offset.src.Y >= maxY) 
        {
            if (scroll & FX_SCROLL_VIEWPORT) 
            {
                scroll &= ~FX_SCROLL_VMASK;
                scroll |= FX_SCROLL_DOWN;
                scroll = (scroll & ~FX_SCROLL_COUNT_MASK) | (32 & FX_SCROLL_COUNT_MASK);
            } 
            
            else 
            {
                wd->offset.src.Y = minY; //-wd->dim.dst.height;
                scroll_check = 1;
            }
        }

    } 

    else if (scroll & FX_SCROLL_DOWN) 
    {
        wd->offset.src.Y--;
        LOGTRACEW("scroll down (offsetY %d)", wd->offset.src.Y);
        if (wd->offset.src.Y <= minY) 
        {
            if (scroll & FX_SCROLL_VIEWPORT) 
            {
                scroll &= ~FX_SCROLL_VMASK;
                scroll |= FX_SCROLL_UP;
                scroll = (scroll & ~FX_SCROLL_COUNT_MASK) | (32 & FX_SCROLL_COUNT_MASK);
            } 

            else 
            {
                wd->offset.src.Y = maxY; //wd->dim.src.height;
                scroll_check = 1;
            }
        }
    }

    if (scroll_check) 
    {
        if (scroll & FX_SCROLL_COUNTER) 
        {
            uint32_t count = scroll & FX_SCROLL_COUNT_MASK;
            count--;
            if (!count) 
            {
                // Don't want the window to be destroyed unconditionally. Let
                // the driver code know it's done scrolling, and let it decide
                // what to do.
                scroll &= ~(FX_SCROLL_COUNT_MASK | FX_SCROLL_COUNTER);
                return WIN_SCROLL_DONE;
#if 0
                /* window is to be destroyed */
                LOGTRACEW("Window will be destroyed, scroll count exceeded", 0);
                scroll &= ~(FX_SCROLL_COUNT_MASK | FX_SCROLL_COUNTER);
                wd->scroll = scroll;
                return WIN_DESTROY;
#endif
            }

            scroll = (scroll & ~FX_SCROLL_COUNT_MASK) | (count & FX_SCROLL_COUNT_MASK);
        }
    }

    wd->scroll = scroll;

    return WIN_OK;
}

void dumpWin (WINDOW *w);

/****************************************************************************/

int winDraw (WINDOW *w, uint8_t ***buf) 
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;

    LOGTRACEW ("Entered winDraw (win region: %dx%d, doc size: %dx%d)", 
	wd->dim.dst.width, wd->dim.dst.height, wd->dim.src.width, 
	wd->dim.src.height);

    if ((wd->type == TEXT) && (wd->effect & FX_MARKUP))
        wd->dim.src = drawRenderBuf(wd);

    int ret = determineScroll(wd);
    if (ret != WIN_OK) return ret;

    //
    // Clear the entire window, so lower priority windows underneath don't show
    // up in the window area. Do this here so it will take effect for flashing.
    //

    uint8_t **fb = (uint8_t**)buf;
    for (int r = 0; r < wd->dim.dst.height; ++r)
    {
	memset (
	    &fb[wd->offset.dst.Y + r][wd->offset.dst.X * wd->dp.clrBytes],
	    0,
	    wd->dim.dst.width * wd->dp.clrBytes);
    }

    if ((wd->effect & FX_FLASH) && wd->state.flashState) return WIN_OK;

    /* get starting address and region of interest for destination buffer */
    //uint8_t *dst = (uint8_t*)&buf[0][0][0];
    uint8_t *dst = (uint8_t*)buf[0];
    roi_t droi = wd->dim.dst;

    if (wd->offset.dst.Y < 0)
        LOGERR ("How did offset.dst.Y become %d?", wd->offset.dst.Y);
    else
       dst += (droi.stride * wd->offset.dst.Y);

    if (wd->offset.dst.X < 0)
        LOGERR ("How did offset.dst.X become %d?", wd->offset.dst.X);
    else 
        dst += wd->offset.dst.X * wd->dp.clrBytes;

    /* determine alignment for source document */
    uint8_t *src = wd->buf;

    roi_t sroi = wd->dim.src;
    pos_t pos = determineAlignment(wd);

    LOGTRACEW("Alignment returned pos %d,%d skew, buf == %p, buf[0][0] == %p, &(buf[0][0][0]) == %p", pos.X, pos.Y, wd->buf, buf[0][0], &(buf[0][0][0]));

    if (pos.X) {
        int32_t absX = abs(pos.X);
//        sroi.width -= absX;
        if (pos.X > 0) {
            if (droi.width < absX) return WIN_OK;
            //dst += absX * 3;
            dst += absX * wd->dp.clrBytes;
            droi.width -= absX;
        } else {
            if (sroi.width < absX) return WIN_OK;
            //src += absX * 3;
            src += absX * wd->dp.clrBytes;
            sroi.width -= absX;
        }
    }

    if (pos.Y) {
        int32_t absY = abs(pos.Y);
//        sroi.height -= absY;
        if (pos.Y > 0) {
            if (droi.height < absY) return WIN_OK;
            dst += (absY * droi.stride);
            droi.height -= absY;
        } else {
            if (sroi.height < absY) return WIN_OK;
            src += (absY * sroi.stride);
            sroi.height -= absY;
        }
    }

    copyRegion(wd, dst, droi, src, sroi);
    return WIN_OK;
}

/****************************************************************************/

int winDrawAtRow (WINDOW *w, uint8_t ***buf, int row)
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;

    if ((row < 0) || (row >= wd->dp.rows))
    {
        LOGERR ("winDrawAtRow: row out of bounds: %d", row);
        return -1;
    }

    wd->offset.dst.Y = row;
    winDraw (w, buf);

    return WIN_OK;
}

