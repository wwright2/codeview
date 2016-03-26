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
//  Filename:       font.h
//
//  Description:    font definitions
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  04/20/11    Joe Halpin      1       New
//
//  Notes: 
//
//  Fonts are produced by BitfontCreator, a Windows program. This
//  program emits a .c file for each font it produces. The .c file has
//  literal control codes which cause problems with index 0 of the
//  various tables, and must be hand edited out. If this is not done
//  weird things happen.
//
//  ***************************************************************************

#ifndef font_h
#define font_h

#include <stdint.h>

// Number of fonts allowed
#define MAXFONT 20

enum
{
    EFONT_NOTFOUND,             // Can't find the font file
    EFONT_MAX,                  // no more room in font table
    EFONT_MEM,                  // out of memory
    EFONT_DLSYM,                // error resolving symbol
    EFONT_INVAL,                // invalid argument
};
    
typedef struct
{
    const char *fontName;
    uint32_t    fontHeight;
    uint32_t    fontWidth;
    uint32_t    variable;
    uint32_t    dataTableLength;
    uint32_t    indexTableLength;
    uint32_t    offsetTableLength;
    uint32_t    widthTableLength;

    const uint8_t    *dataTable;
    const uint8_t    *indexTable;
    const uint32_t   *offsetTable;
    const uint8_t    *widthTable;
} FONT;

typedef struct
{
    int width;
    int height;
    const uint8_t *data;
} GLYPH;

extern GLYPH glyphs[256];

// -----------------------------------------------------------------
// This taks an absolute or relative path name to the .so file. The 
// file is then loaded into memory and converted into a FONT struct.
// Returns a pointer to the font struct, or 0 on error
//
// The path to the font directory on the controller should be used.
// If a path isn't given, the system will try to find the .so file
// in the usual locations, and this may or may not have been added
// to ldconfig.
//
FONT *fntGetFont (const char *fname);

// -----------------------------------------------------------------
// This removes the font from the font table
// 
void fntDeleteFont (int fontID);

// -----------------------------------------------------------------
// This retrieves the bitmap for a glyph, and fills in the GLYPH
// struct with the information required to use it.
//
int fntGetGlyph (const FONT *f, GLYPH *g, char c);

// -----------------------------------------------------------------
//
int fntGetLastError ();

// -----------------------------------------------------------------
//
const char *fntGetLastErrStr ();

#endif
