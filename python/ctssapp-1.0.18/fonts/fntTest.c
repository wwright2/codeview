#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <dlfcn.h>

#include <ctype.h>

void hexdump (const void *data, size_t length)
{
    const unsigned char *dptr = (const unsigned char*) data;
    unsigned char hexLine[128];
    unsigned char ascLine[128];
    unsigned char *hptr;
    unsigned char *aptr;

    size_t i = 0;
    while(i < length)
    {
        size_t j;

        hptr = hexLine;
        aptr = ascLine;

        for(j = i; j < i + 16; ++j)
        {
            if(j < length)
            {
                sprintf((char*)hptr,"%02x", (unsigned int) dptr[j]);
            }

            else
            {
                sprintf((char*)hptr,"%s", "  ");
            }

            hptr += 2;

            if((j+1) % 2 == 0)
            {
                sprintf((char*)hptr, "%s", "  ");
                hptr += 1;
            }
        }

        for(j = i; j < i + 16; ++j)
        {
            if(j < length)
            {
                *aptr = (isprint(dptr[j])) ? (char)dptr[j] : '.';
            }
            else
            {
                *aptr = ' ';
            }
            ++aptr;
        }

        *aptr = 0;
        printf("%08x  %s  %s\n",(unsigned int)i, hexLine, ascLine);

        i += 16;
    }
}





#define MAXFONTS 20

//
// NOTE: these two typedefs must be kept in sync with font.c in the
// display code
//

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


FONT *f = 0;
uint8_t *buf = 0;

void die (const char *msg)
{
    perror (msg);
    exit (1);
}


void display ()
{
    printf ("%20s %s\n", "fontName", f->fontName);
    printf ("%20s %u\n", "fontHeight", f->fontHeight);
    printf ("%20s %u\n", "fontWidth", f->fontWidth);
    printf ("%20s %u\n", "variable", (uint32_t)f->variable);
    printf ("%20s %u\n", "dataTableLength", (uint32_t)f->dataTableLength);
    printf ("%20s %u\n", "indexTableLength", (uint32_t)f->indexTableLength);
    printf ("%20s %u\n", "offsetTableLength", (uint32_t)f->offsetTableLength);
    printf ("%20s %u\n", "widthTableLength", (uint32_t)f->widthTableLength);
    printf ("%20s %p\n", "dataTable", f->dataTable);
    printf ("%20s %p\n", "indexTable", f->indexTable);
    printf ("%20s %p\n", "offsetTable", f->offsetTable);
    printf ("%20s %p\n", "widthTable", f->widthTable);

    //
    // This is demo code. It assumes that the glyph width will always
    // be <= 8. Need to expand this for production.
    //

    unsigned char letter = '0';
    printf ("\nThe letter %c\n", letter);
    if (f->offsetTableLength == 4)
    {
        //
        // If offset table length == 4, then there's only one element
        // in the array. That would be the dummy element that was put
        // there so the symbol would resolve.
        //

        printf ("in fixed width code\n");
        unsigned int offset = f->indexTable[letter] * f->fontHeight;
        const uint8_t *data = &f->dataTable[offset];
        printf ("offset = %d\n", offset);

        for (int r = 0; r < f->fontHeight; ++r)
        {
            uint8_t b = data[r];
            putchar ('[');
            for (int c = 0; c < f->fontWidth; ++c)
            {
                if (b & 0x80)
                    putchar ('*');
                else
                    putchar (' ');
                b <<= 1;
            }
            printf ("]\n");
        }
    }

    else
    {
        printf ("in variable width code\n");
        const unsigned int index  = f->indexTable[letter];
        printf ("   index = %d\n", index);
        const unsigned int offset = f->offsetTable[index];
        printf ("   offset = %d\n", offset);
        const unsigned char *data = &f->dataTable[offset];

        printf ("   data - dataTable = %d\n", (data - f->dataTable) / 8);
        printf ("   data = %02x %02x %02x %02x %02x %02x %02x %02x\n",
            data[0], data[1], data[2], data[3], 
            data[4], data[5], data[6], data[7]);

        const unsigned char w     = f->widthTable[index];

        printf ("index  = %d\n", index);
        printf ("offset = %d\n", offset);
        printf ("w      = %d\n", w);

        //  ---- FIX ME
        for (int r = 0; r < f->fontHeight; ++r)
        {
            uint8_t b = data[r];

            putchar ('[');
            for (int c = 0; c < w; ++c)
            {
                if (b & 0x80)
                    putchar ('*');
                else
                    putchar (' ');
                b <<= 1;
            }
            printf ("]\n");
        }
        
    }
}

int main (int argc, const char *argv[])
{
    char varname[1024];
    if (argc == 1)
        die ("Font file not specified");

    // open the shared object

    printf ("argv[1] = %s\n", argv[1]);
    sprintf (varname, "./%s", argv[1]);
    printf ("opening %s\n", varname);
    void *flib = dlopen (varname, RTLD_NOW);
    if (flib == 0)
        die ("ERROR in dlopen()");
    
    f = malloc (sizeof (FONT));
    if (f == 0)
        die ("Error in malloc()");
    
    memset (f, 0, sizeof *f);
    
    const char *err;
    
    dlerror();
    f->fontName = *(const char **)dlsym (flib, "fontName");
    printf ("f->fontName = %s\n", f->fontName);
    if ((err = dlerror()) != 0)
        die (err);
    
    dlerror();
    sprintf (varname, "%s_FontHeight", f->fontName);
    f->fontHeight = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    dlerror();
    sprintf (varname, "%s_FontWidth", f->fontName);
    f->fontWidth = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    dlerror();
    sprintf (varname, "%s_dataTableLength", f->fontName);
    f->dataTableLength = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    dlerror();
    sprintf (varname, "%s_indexTableLength", f->fontName);
    f->indexTableLength = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    dlerror();
    sprintf (varname, "%s_offsetTableLength", f->fontName);
    f->offsetTableLength = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    dlerror();
    sprintf (varname, "%s_widthTableLength", f->fontName);
    f->widthTableLength = *(uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    dlerror();
    sprintf (varname, "%s_data_table", f->fontName);
    f->dataTable = (const uint8_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    dlerror();
    sprintf (varname, "%s_offset_table", f->fontName);
    f->offsetTable = (const uint32_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    dlerror();
    sprintf (varname, "%s_index_table", f->fontName);
    f->indexTable = (const uint8_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    dlerror();
    sprintf (varname, "%s_width_table", f->fontName);
    f->widthTable = (const uint8_t*)dlsym (flib, varname);
    if ((err = dlerror()) != 0)
        die (err);

    if (f->fontWidth == 0)
        f->variable = 0;
    else 
        f->variable = 1;

    printf ("Printing font\n");
    display ();
}
