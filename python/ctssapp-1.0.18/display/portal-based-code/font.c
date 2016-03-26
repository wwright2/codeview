#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "font.h"

static FONT fontTable[MAXFONT];
static int  inuseList[MAXFONT];

static int lastErr = 0;
static const char *lastErrMsg = "";

GLYPH glyphs[256];

static int getNextFontID (const char *fname, int *new)
{
    int ret = -1;

    for (int i = 0; i < MAXFONT; ++i)
    {
        if (inuseList[i] == 1)
        {
            // Don't load it again if it's already there.
            if (strcmp (fontTable[i].fontName, fname) == 0)
            {
                ret = i;
                *new = 0;
                break;
            }
        }

        else if (inuseList[i] == 0)
        {
            inuseList[i] = 1;
            ret = i;
            *new = 1;
            break;
        }
    }

    return ret;
}

// -----------------------------------------------------------------

static void setError (int e, const char *s)
{
    lastErr = e;
    lastErrMsg = s;
}

// -----------------------------------------------------------------

int fntGetLastError ()
{
    return lastErr;
}

// -----------------------------------------------------------------

const char *fntGetLastErrStr ()
{
    return lastErrMsg;
}

// -----------------------------------------------------------------

FONT *fntGetFont (const char *fname)
{
    int new = 0;
    int id = getNextFontID (fname, &new);
    char varname[1024];
    const char *err;

    if (id == -1)
    {
        setError (EFONT_MAX, "Max fonts exceeded");
        return 0;
    }

    if (new == 0)
        return &fontTable[id];

    //
    // Not there, and we have a slot for it
    //

    FONT *f = &fontTable[id];

    // open the shared object
    void *flib = dlopen (fname, RTLD_NOW);
    if (flib == 0)
    {
        setError (EFONT_NOTFOUND, dlerror());
        return 0;
    }

    memset (f, 0, sizeof *f);
    
    dlerror();
    f->fontName = *(const char **)dlsym (flib, "fontName");
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }
    
    dlerror();
    sprintf (varname, "%s_FontHeight", f->fontName);
    f->fontHeight = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    dlerror();
    sprintf (varname, "%s_FontWidth", f->fontName);
    f->fontWidth = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    dlerror();
    sprintf (varname, "%s_dataTableLength", f->fontName);
    f->dataTableLength = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    dlerror();
    sprintf (varname, "%s_indexTableLength", f->fontName);
    f->indexTableLength = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    dlerror();
    sprintf (varname, "%s_offsetTableLength", f->fontName);
    f->offsetTableLength = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    dlerror();
    sprintf (varname, "%s_widthTableLength", f->fontName);
    f->widthTableLength = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    dlerror();
    sprintf (varname, "%s_data_table", f->fontName);
    f->dataTable = (const uint8_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    dlerror();
    sprintf (varname, "%s_offset_table", f->fontName);
    f->offsetTable = (const uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    dlerror();
    sprintf (varname, "%s_index_table", f->fontName);
    f->indexTable = (const uint8_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    dlerror();
    sprintf (varname, "%s_width_table", f->fontName);
    f->widthTable = (const uint8_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
    {
        setError (EFONT_DLSYM, err);
        return 0;
    }

    if (f->fontWidth == 0)
        f->variable = 1;
    else 
        f->variable = 0;

    //
    // Load up the glyph table once to speed things up during processing.
    //
    // This assumes that no more than 256 characters are in the table.
    //
    for (int i = 0; i < f->widthTableLength; ++i)
        fntGetGlyph (f, &glyphs[i], i);

    return f;
}

// -----------------------------------------------------------------

void fntDeleteFont (int fontID)
{
    inuseList[fontID] = 0;
}

// -----------------------------------------------------------------

int fntGetGlyph (const FONT *f, GLYPH *g, char c)
{
    //printf ("in fntGetGlyph\n");

    if (f->variable == 1)
    {
        uint32_t  index  = f->indexTable[(uint8_t)c];
        uint32_t  offset = f->offsetTable[index];
        const uint8_t  *data  = &f->dataTable[offset];

        g->width  = f->widthTable[index];
        g->height = f->fontHeight;
        g->data   = data;
        
        //printf ("   width = %d, height = %d\n", g->width, g->height);
    }

    else
    {
        unsigned int offset = f->indexTable['A'] * f->fontHeight;
        const uint8_t *data = &f->dataTable[offset];

        g->width  = f->fontWidth;
        g->height = f->fontHeight;
        g->data   = data;
        //printf ("  width = %d, height = %d\n", g->width, g->height);
    }

    return 0;
}
