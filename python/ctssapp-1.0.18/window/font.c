// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       font.c
//
//  Description:    given an array of font data, and other required data, this
//                  creates a font with pre-rendered glyphs.
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  03/21/2012  Joe Halpin      1
//  Notes:
//
// ***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "font.h"


// ---------------------------------------------------------------------------
// This takes an array of font data, and creates a font object with a
// pre-rendered glyph table.
//
// NOTE: this assumes the bytes in the font data are in column first order.
//
// NOTE: There is no function to free memory, because fonts are never deleted
// once they have been instantiated.
//

FONT *newFont (
    const char *name,             // font name
    const uint8_t *src,           // font data
    const int *widthTable,        // width of each glyph
    int height,                   // how many rows
    int maxWidth,                 // how many columns (max)
    int vpitch,                   // how many bytes per column/row
    int numGlyphs,                // how many glyphs are defined in font data
    int startIndex)               // what character code is the first in data
{
    //
    // Allocate a buffer big enough for everything, then point to the start of
    // each row with the rows array.
    //

    //printf ("height=%d maxWidth=%d vpitch=%d numGlyphs=%d startIndex=%d\n",
    //	height,maxWidth,vpitch,numGlyphs,startIndex);

    int rowSize  = maxWidth * numGlyphs;
    int dataSize = height * rowSize;

    //printf ("rowSize  = %d\n", rowSize);
    //printf ("dataSize = %d\n", dataSize);

    uint8_t *data = malloc (dataSize);
    uint8_t **rows = malloc (height * sizeof (uint8_t *));

    if (data == 0 || rows == 0)
    {
	printf ("allocation error in newFont\n");
	exit (1);
    }

    memset (data, 0, dataSize);
    memset (rows, 0, height * sizeof (uint8_t*));

    for (int i = 0; i < height; ++i)
        rows[i] = &data[i * rowSize];

    //
    // Allocate an array of GLYPH objects, one for each character code in the
    // font. Use the offsets array to tell whether a particular character code
    // is implemented, and if so, get a pointer to the glyph.
    //
    // This assumes that all the character codes are contiguous.
    //

    GLYPH *glyphs   = malloc (numGlyphs * sizeof (GLYPH));
    GLYPH **offsets = malloc (256 * sizeof (GLYPH*));

    if (glyphs == 0 || offsets == 0)
    {
	printf ("Allocation error in newFont\n");
	exit (1);
    }

    memset (glyphs,  0, (numGlyphs * sizeof (GLYPH)));
    memset (offsets, 0, (256 * sizeof (GLYPH**)));

    //uint8_t colBits = (vpitch * 8) - ((vpitch*8) - height);
    //uint8_t padBits = (vpitch * 8) - colBits;

    int srcByte   = 0;
    int col       = 0;
    uint8_t byte  = 0;

    for (int g = 0; g < numGlyphs; ++g)
    {
        int startCol = col;
        int idx = g + startIndex;

       if (idx < startIndex || idx > startIndex + numGlyphs)
            continue;

        glyphs[g].data     = rows;
        glyphs[g].startCol = startCol;
        glyphs[g].height   = height;
        glyphs[g].width    = widthTable[g]; // number of bytes in glyph
        offsets[idx]       = &glyphs[g];

	int gcols = glyphs[g].width;

        for (int c = startCol; c < startCol + gcols; ++c)
        {
            //uint8_t colBit = 0;
            int     r      = height - 1;

	    //
	    // At the start of each column we may have to skip some padding
	    // bits if the height is less than the number of bits in vpitch
	    // bytes.
	    //
	    
	    int pad = (vpitch * 8) - height;

            for (int vpByte = 0; vpByte < vpitch; ++vpByte)
            {
		byte = src[srcByte++];
		byte <<= pad;

		for (int i=pad; i < 8; ++i)
		{
		    //printf ("%c", byte & 0x80 ? '*' : ' ');
		    rows[r][c] = byte & 0x80 ? 1 : 0;
		    byte <<= 1;
		    --r;
		}
		pad = 0;
	    }
	    //printf ("\n");
        }
	//printf ("\n");

        col += gcols ;
    }

    FONT *f = malloc (sizeof (FONT));
    f->startIndex = startIndex;
    f->glyphTable = glyphs;
    f->height = height;
    f->maxWidth = maxWidth;
    f->vpitch = vpitch;
    f->numGlyphs = numGlyphs;
    f->offsets = offsets;
    f->fontMem.rows = rows;
    f->fontMem.glyphs = glyphs;
    f->fontMem.offsets = offsets;

#if 0
    int startCol = 0;
    for (int g = 0; g < numGlyphs; ++g)
    {
        GLYPH *glyph = &f->glyphTable[g];

	printf ("glyph %d:\n", g);
	printf ("   startCol = %d\n", glyph->startCol);
	printf ("   height   = %d\n", glyph->height);
	printf ("   width    = %d\n", glyph->width);
	
        for (int r = 0; r < height; ++r)
        {
	    printf ("[");
            for (int c = 0; c < glyph->width; ++c)
            {
                printf ("%c", glyph->data[r][c+startCol] ? '*' : ' ');
            }
            printf ("]\n");
        }
        printf ("\n");
        startCol += glyph->width;
    }
#endif

    return f;
}

static inline void showdlErr () 
{                    
    char buf[1024];  
    sprintf (buf, "%s\n", dlerror ()); 
    printf ("%s", buf); 
}


void freeFont (FONT *f)
{
    free (f->fontMem.rows[0]);
    free (f->fontMem.rows);
    free (f->fontMem.glyphs);
    free (f->fontMem.offsets);
    free (f);
}


FONT *fontLoad (const char *fntlib)
{
    void *handle;
    const char *fontName = 0;
    int fontStartCode = 0;
    int fontNumGlyphs = 0;
    int fontHeight = 0;
    int fontMaxWidth = 0;
    int fontVpitch = 0;
    const uint8_t *fontData = 0;
    int *widthTable = 0;
    int *indexTable = 0;
    int *tmp = 0;

    if (fntlib == 0)
    {
        printf ("no library specified\n");
        exit (1);
    }

    dlerror ();
    if ((handle = dlopen (fntlib, RTLD_NOW)) == 0)
    {
        showdlErr ();
        return 0;
    }

    dlerror ();
    if ((fontName = *(const char **)dlsym (handle, "fontName")) == 0)
    {
        showdlErr ();
        dlclose (handle);
        return 0;
    }

    if ((tmp = dlsym (handle, "fontStartCode")) == 0)
    {
        showdlErr ();
        dlclose (handle);
        return 0;
    }
    fontStartCode = *tmp;

    if ((tmp = dlsym (handle, "fontNumGlyphs")) == 0)
    {
        showdlErr ();
        dlclose (handle);
        return 0;
    }
    fontNumGlyphs = *tmp;

    if ((tmp = dlsym (handle, "fontHeight")) == 0)
    {
        showdlErr ();
        dlclose (handle);
        return 0;
    }
    fontHeight = *tmp;

    if ((tmp = dlsym (handle, "fontMaxWidth")) == 0)
    {
        showdlErr ();
        dlclose (handle);
        return 0;
    }
    fontMaxWidth = *tmp;

    if ((tmp = dlsym (handle, "fontVpitch")) == 0)
    {
        showdlErr ();
        dlclose (handle);
        return 0;
    }
    fontVpitch = *tmp;

    if ((fontData = (const uint8_t*)dlsym (handle, "fontData")) == 0)
    {
        showdlErr ();
        dlclose (handle);
        return 0;
    }

    if ((widthTable = dlsym (handle, "widthTable")) == 0)
    {
        showdlErr ();
        dlclose (handle);
        return 0;
    }

    if ((indexTable = dlsym (handle, "indexTable")) == 0)
    {
        showdlErr ();
        dlclose (handle);
        return 0;
    }

    FONT *fnt = newFont (
        fontName,
        fontData,
        widthTable,
        fontHeight,
        fontMaxWidth,
        fontVpitch,
        fontNumGlyphs,
        fontStartCode);

    dlclose (handle);
    if (fnt == 0)
    {
        printf ("Error getting font %s\n", fontName);
        return 0;
    }

    return fnt;
}
