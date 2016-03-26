/***************************************************************************/ //  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       window.c
//
//  Description:    Implementation of window.
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  03/16/2012  Joe Halpin      1
//  Notes:
//
// ***************************************************************************


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <assert.h>

#include "font.h"
#include "window.h"
#include "windowData.h"
#include "winRender.h"
#include "win24Funcs.h"
#include "graphic.h"
#include "log.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "helpers.h"
#include "textWindow.h"
#include "winTypes.h"

//
// Error reporting stuff
//
static const char *errMsg = "";
static int errCode = 0;

#define ERR_OK()    { errMsg = ""; errCode = 0; }
#define ERR_ALLOC() { MsgLog(PRI_HIGH,"Memory allocation failed: %s %d",\
            __FILE__, __LINE__); abort();}
#define ERR_FATAL() { MsgLog(PRI_HIGH,"Fatal windowing error: %s %d", \
            __FILE__, __LINE__); abort();}
#define ERR_INVAL() { MsgLog(PRI_HIGH,"Invalid arg: %s %d", \
            __FILE__, __LINE__); abort (); }

const char * winLastErrMsg () { return errMsg; }
int winLastErrCode () { return errCode; }

#define freeIdListSize() (sizeof freeIdList / sizeof freeIdList[0])

// --------------------------------------------------------------------------
// 
void dumpWin (WINDOW *w)
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;

    printf ("\n ======================\ndumpWin:\n");
    printf ("userData       = %u\n", w->userData);
    printf ("dp.rows        = %d\n", wd->dp.rows);
    printf ("dp.cols        = %d\n", wd->dp.cols);
    printf ("dp.clrBytes    = %d\n", wd->dp.clrBytes);
    printf ("dp.clrBits     = %d\n", wd->dp.clrBits);
    printf ("offset.src.X   = %d\n", wd->offset.src.X);
    printf ("offset.src.Y   = %d\n", wd->offset.src.Y);
    printf ("offset.dst.X   = %d\n", wd->offset.dst.X);
    printf ("offset.dst.Y   = %d\n", wd->offset.dst.Y);
    printf ("dim.src.width  = %d\n", wd->dim.src.width);
    printf ("dim.src.height = %d\n", wd->dim.src.height);
    printf ("dim.src.stride = %d\n", wd->dim.src.stride);
    printf ("dim.dst.width  = %d\n", wd->dim.dst.width);
    printf ("dim.dst.height = %d\n", wd->dim.dst.height);
    printf ("dim.dst.stride = %d\n", wd->dim.dst.stride);
    printf ("depth          = %d\n", wd->depth);
    printf ("font           = %p\n", wd->font);
    printf ("align          = %u\n", wd->align);
    printf ("effect         = %u\n", wd->align);
    printf ("data           = %s\n", wd->data);
    printf ("dataLen        = %d\n", wd->dataLen);
    printf ("winSize.width  = %d\n", wd->winSize.width);
    printf ("winSize.height = %d\n", wd->winSize.height);
    printf ("winSize.stride = %d\n", wd->winSize.stride);
    printf ("buf            = %p\n", wd->buf);
    printf ("bufsz          = %d\n", wd->bufsz);
    printf ("windowId       = %d\n", wd->windowId);
    printf ("priority       = %d\n", wd->priority);
}

// --------------------------------------------------------------------------
// If an update is needed before redrawing, like a shift needs to be made for a
// scrolling effect, this tells the window when to do it.
//
static inline void setUpdate (WINDOW_DATA *wd, int interval, uint64_t now)
{
    wd->updateInterval = interval;
    wd->updateInterval *= 1000;
    wd->state.nextFlashUpdate = now;

    LOGTRACEW ("interval = %d usec", wd->updateInterval);
    wd->nextUpdate = now;       // start it off right away, it will get reset
}

// --------------------------------------------------------------------------
// Set the time to live as a number of microseconds from now, as an absolute
// time. 
static inline void setTTL (WINDOW *win, int ttl, uint64_t now) {

    if (ttl < 1) {
        win->ttl = 0;
        return;
    }

    win->ttl = ttl;
    win->ttl *= 1000000;
    win->ttl += now;

    return;

}


// --------------------------------------------------------------------------
//
int checkDimensions (
    const DISPLAY_PROFILE *dp,
    uint32_t *width,
    uint32_t *startCol,
    uint32_t *height,
    uint32_t *startRow,
    const FONT *f)
{
    LOGTRACEW ("in checkDimensions:", 0);
    LOGTRACEW ("width = %d height = %d startRow = %d startCol = %d dp->cols = %d",
        *width, *height, *startRow, *startCol, dp->cols);

    if ((*width) > dp->cols) {
        LOGWARN ("window won't fit in screen, (width %d) resizing to MAX %d)",
            *width, dp->cols);
        (*width) = dp->cols;
    }

    if ((*height) > dp->rows) {
        LOGWARN ("window won't fit in screen (height %d) resizing to MAX %d)",
            *height, dp->rows);
        (*height) = dp->rows;
    }

    int wend = (*width) + (*startCol);
    if (wend > dp->cols) {
        int extra = wend - dp->cols;
        LOGWARN ("Window exceed display size  (*width: %d + X: %d > MAX: %d)",
            extra, *width, *startCol, dp->cols);
        if (extra > *startCol) {
            extra -= *startCol;
            (*startCol) = 0;
            (*width) -= extra;
        } else
            (*startCol) -= extra;
        LOGWARN ("--> Resized window to %d,%d %d,%d", *startCol,
            *startRow, *width, *height);
    }

    int hend = (*height) + (*startRow);
    if (hend > dp->rows) {
        int extra = hend - dp->rows;
        LOGWARN ("Window exceed display size by %u (height: %d + Y: %d > MAX: %d)", extra, *height, *startRow, dp->rows);
        if (extra > (*startRow)) {
            extra -= (*startRow);
            (*startRow) = 0;
            (*height) -= extra;
        } else
            (*startRow) -= extra;

        LOGWARN ("--> Resized window to %d,%d %d,%d", *startCol,
            *startRow, *width, *height);
    }

    if (f) {
        LOGTRACEW ("f->height = %d", f->height);
        if ((*height) < f->height) {
            LOGWARN ("Font is too big for window", 0);
            return -1;
        }
    }

    return 0;
}


// --------------------------------------------------------------------------
//
WINDOW *newWindow (
    const DISPLAY_PROFILE *dp, // describes dimensions of the display buf
    uint32_t startRow,	      // base row
    uint32_t startCol,	      // base column
    uint32_t width,	      // length of window in columns
    uint32_t height,	      // height of window in rows
    uint32_t offsetX,	      // offset into src buffer (graphics)
    uint32_t offsetY,         // ditto
    WIN_DATA_TYPE type,
    uint8_t depth,	      // color depth - 24 bit supported initially
    const uint8_t *color,     // color to use
    const FONT *f,	      // default font 
    uint8_t align,	      // if none given LEFT is default
    uint32_t effect,
    uint32_t scroll,
    const char *data,	      // data to display. This isn't the real type
    size_t dataLen,	      // length of data
    int updateInterval,	      // interval in ms between updates
    int windowId,	      // id of window.
    int priority,	      // determines where window is in stacking order
    int ttl)		      // time to live in seconds, or -1
{
    GRAPHIC_IMG *img = NULL;
    char *dataBuf = NULL;
    roi_t roi = { 0, 0, 0 };
    size_t rsize = 0;
    uint8_t *frameBuf = NULL;

    LOGTRACEW ("in newWindow()", 0);

    //
    // Do checks to make sure the color depth is supported, and the window will
    // fit inside the matrix.
    //

    if (depth != dp->clrBytes)
    {
        errMsg = "Invalid color";
	LOGERR("Invalid color depth passed to %s %d", __FILE__, __LINE__);
	return NULL;
    }

    uint64_t now = getUSecs();

    if ((startCol >= dp->cols) || (startRow >= dp->rows)) 
    {
        errMsg = "Invalid dimensions";
	LOGERR ("Window too large for display");
        return NULL;
    }

    if (type == TEXT) 
    {
        if (data == NULL) 
	{
            data = "";
            dataLen = 0;
        }

        roi = determineTextBufferSize(data, f);
        LOGTRACEW ("roi = %dx%dx%d\n", roi.width,roi.height,roi.stride);

        rsize = allocTextData (data, &dataBuf, &frameBuf, roi, dp);
	if ((rsize == (size_t) -1) || (frameBuf == NULL))
	{
            errMsg = "Allocation error";
	    LOGERR ("Allocation error for region %d x %d", 
		roi.width, roi.height);
	    return NULL;
        }

        LOGTRACEW("Allocated %dx%d for render buf", roi.width, roi.height);
    } 

    else if (type == GRAPHIC) 
    {
        LOGTRACEW ("creating new graphic image", 0);

        img = newGraphicImg (data, dataLen);
        if (img == NULL) 
	{
            errMsg = "Failed to load graphics";
            LOGERR ("Failed to load graphics image '%s'", data);
            return NULL;
        }

        roi.width  = img->g->imgWidth;
        roi.height = img->g->imgHeight;
        roi.stride = img->g->rowbytes;
        frameBuf   = img->g->data;

        rsize = 0;
        dataLen = 0;
    } 

    else 
    {
        errMsg = "Invalid window type";
        LOGERR ("Attempted to create an unknown window type %u", type);
        return NULL;
    }

    LOGTRACEW ("calloc WINDOW", 0);
    WINDOW *newWin = calloc(1, sizeof (WINDOW));
    if (newWin == NULL) 
    {
        errMsg = "Allocation error";
        LOGERR ("calloc failed");
        return NULL;
    }

    LOGTRACEW ("calloc WINDOW_DATA", 0);
    WINDOW_DATA *wd = calloc(1, sizeof (WINDOW_DATA));
    if (wd == 0) 
    {
        errMsg = "Allocation error";
        free (newWin);
	LOGERR ("calloc failed");
	return NULL;
    }

    wd->dp = *dp;
    wd->offset.dst.X = startCol;
    wd->offset.dst.Y = startRow;
    uint32_t end = startCol + width;

    if (end >= dp->cols) 
    {
        uint32_t diff = end - dp->cols;
        if (diff > width) width = 0;
        else width -= diff;
    }

    end = startRow + height;
    if (end >= dp->rows) 
    {
        uint32_t diff = end - dp->rows;
        if (diff > height) height = 0;
        else height -= diff;
    }

    wd->dim.dst.width  = width;
    wd->dim.dst.height = height;
    wd->dim.dst.stride = (dp->cols * dp->clrBytes);

    wd->offset.src.X = offsetX;
    wd->offset.src.Y = offsetY;
    roi.stride = roi.width * dp->clrBytes;
        LOGTRACEW ("roi = %dx%dx%d\n", roi.width,roi.height,roi.stride);
    wd->dim.src = roi;
        LOGTRACEW ("wd->dim.src = %dx%dx%d\n", roi.width,roi.height,roi.stride);

    wd->depth      = depth;
    wd->align      = align;
    wd->effect     = effect;
    if (scroll & FX_SCROLL_COUNT_MASK)
        scroll |= FX_SCROLL_COUNTER;
    wd->scroll     = scroll;

    wd->font       = f;
    wd->type       = type;

    wd->buf        = frameBuf;
    wd->bufsz      = rsize;

    wd->img        = img;
    wd->data       = dataBuf;
    wd->dataLen    = dataLen;

    wd->windowId   = windowId;
    wd->priority   = priority;

    for (int clr = 0; clr < dp->clrBytes; ++clr)
        wd->color[clr] = color[clr];

    LOGTRACEW ("calling setUpdate", 0);
    setUpdate (wd, updateInterval, now);
    setTTL(newWin, ttl, now);

    newWin->next = newWin->prev = NULL;
    newWin->data = wd;
    newWin->id = windowId;
    winInit (newWin);

    errMsg = "";
    LOGTRACEW ("leaving newWindow", 0);
    return newWin;
};

// --------------------------------------------------------------------------
//
void delWindow (WINDOW *w) 
{
    LOGTRACEW ("Deleting window #%d (%p)", w->id, w);
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;

    if (wd->type == TEXT) 
    {
        LOGTRACEW ("Freeing buf and window data (%p, %p)", wd->buf, wd->data);
        if (wd->bufsz) free(wd->buf);
        free(wd->data);
    } 

    else 
    {
        delGraphicImg (wd->img);
    }

    LOGTRACEW ("freeing w->data and w (%p, %p)", w->data, w);
    free (w->data);
    free (w);

    LOGTRACEW ("Leaving delWindow", 0);
}

// --------------------------------------------------------------------------
//
int winGetPriority (WINDOW *w)
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;
    return wd->priority;
}

// --------------------------------------------------------------------------
//
inline uint64_t winGetUpdateTime (WINDOW *w)
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;
    return wd->nextUpdate;
}

// --------------------------------------------------------------------------
//
int winGetEffectDelay (WINDOW *w)
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;
    return wd->state.delayTime;
}

// --------------------------------------------------------------------------
//
void winSetEffectDelay (WINDOW *w, int delay)
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;
    wd->state.delayTime = delay;
}

// --------------------------------------------------------------------------
//
const char *winGetText (WINDOW *w)
{
    return ((WINDOW_DATA*)w->data)->data;
}

// --------------------------------------------------------------------------
//
int winSetText (WINDOW *w, const char *text)
{
    return 0;
}

// --------------------------------------------------------------------------
//
int winDrawAt (WINDOW *w, int row)
{
    return 0;
}

// --------------------------------------------------------------------------
//
int winGetRows (WINDOW *w)
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;
    return wd->dim.src.height;
}

// --------------------------------------------------------------------------
//
int winGetRow (WINDOW *w)
{
    WINDOW_DATA *wd = (WINDOW_DATA*)w->data;
    return wd->offset.dst.Y;
}

