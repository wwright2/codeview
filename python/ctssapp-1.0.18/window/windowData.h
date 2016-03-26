// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       windowData.h
//
//  Description:    declarations for window data structure
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  03/28/2012  Joe Halpin      1
//  Notes:
//
// ***************************************************************************


#ifndef WINDOWDATA_H_INCLUDED
#define WINDOWDATA_H_INCLUDED

//#include "sysGlobals.h"
#include "font.h"
#include "winEnums.h"
#include "winTypes.h"
 
typedef struct
{
    int srcStart;               // start column of source string/graphic
    int srcEnd;                 // ending column of source string/graphic
    int srcLen;                 // number of columns
    int dstStart;               // start column of destination
    int dstEnd;                 // end column of destination
    int dstLen;                 // number of columns in destination
    int delayTime;              // how long to delay between shifts
    int dstOffset;              // keeps track of where to start in dst
    int srcOffset;              // keeps track of where to start in src
    int dstCol;
    int srcCol;
    int dstRow;
    int srcRow;
    int flashState;
    unsigned long long nextFlashUpdate;
    int scrollNeeded;
} EFFECT_STATE;

typedef struct 
{
    uint32_t rowbytes;           // total number of bytes in a row
    uint32_t imgHeight;          // number of rows
    uint32_t imgWidth;           // number of columns
    uint8_t channels;           // number of channels
    uint8_t bitDepth;           // number of bits in each channel
    uint8_t clrType;            // Do we need this?
    uint8_t data[];             // the image bytes
} __attribute__ ((packed)) GRAPHIC_DATA;

typedef struct
{
    GRAPHIC_DATA *g;
    int rows;
    uint8_t *rowPtrs[];
} GRAPHIC_IMG;

// Made color a 6 byte array to make supporting more than one color depth.

typedef struct WINDOW_DATA 
{
    DISPLAY_PROFILE dp;
    struct {
        pos_t src;
        pos_t dst;
    } offset;
    struct {
        roi_t src;
        roi_t dst;
    } dim;
    uint32_t depth;	      // color depth in bits
    const FONT *font;	      // default font to use
    uint8_t align;
    uint32_t scroll; 
    uint32_t effect;	      // effect for the window
    EFFECT_STATE state;	      // variables used when stepping through effect
    WIN_DATA_TYPE type;	      // type of data (graphic or text)
    char      *data;	      // the text or graphic to display
    size_t    dataLen;	      // length of valid data within the buffer.
    roi_t     winSize;        // size of the actual rendered data
    uint8_t   color[6];	      // color used to render characters
    uint8_t  *buf;	      // buffer to render the window output into
    uint32_t  bufsz;
    int       windowId;
    int       priority;
    int       updateInterval;
    uint64_t nextUpdate;
    GRAPHIC_IMG *img;

} WINDOW_DATA;

#endif
