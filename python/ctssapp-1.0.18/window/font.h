// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       font.h
//
//  Description:    Defines the structure for a font as needed by the window
//                  library. 
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  03/16/2012  Joe Halpin      1 
//  Notes:
//
//  Calling code must supply a font structure that compiles with this type when
// calling the window constructor. If it's not compatible, bad things will
// happen. 
//
// The glyphs in a font are accessed through the offsets array. The character
// code for the desired glyph is used to index into offsets[], and the pointer
// at that location gives the start of data for that glyph.
//
// This means that the glyphTable doesn't need to have 256 entries. If only a
// subset of ASCII is implemented in the font (say, 32 - 126), the other
// entries in offsets can point to some null glyph object. The rendering code
// will check for this.
// ***************************************************************************

#ifndef FONT_H_INCLUDED
#define FONT_H_INCLUDED

#include <stdint.h>

typedef enum
{
    BY_ROW,
    BY_COL,
} FONT_ORIENTATION;

typedef struct
{
    int      height;
    int      width; 
    int      startCol;
    uint8_t **data;
} GLYPH;

typedef struct 
{
    //
    // Keep track of the memory that was allocated for the glyphs so it can be
    // easily deallocated if needed. Probably we'll keep a font around forever
    // once it's here, but ...
    //
    uint8_t **rows;
    GLYPH    *glyphs;
    GLYPH   **offsets;
} FONT_MEM;

typedef struct
{
    GLYPH     *glyphTable;
    int        height;
    int        maxWidth;
    int        vpitch;
    int        startIndex;
    int        numGlyphs;
    GLYPH    **offsets;
    FONT_MEM   fontMem;
} FONT;

FONT *fontLoad (const char *fntlib);
void freeFont (FONT *f);

#endif

