#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

//
// This converts the old row-based fonts to column-based fonts.
//

extern const char *fontName;
extern const unsigned int  Nema_5x7_FontWidth;
extern const unsigned int  Nema_5x7_FontHeight;
extern const unsigned int  Nema_5x7_DataLength;
extern const unsigned int  Nema_5x7_dataTableLength  ;
extern const unsigned int  Nema_5x7_offsetTableLength;
extern const unsigned int  Nema_5x7_indexTableLength ;
extern const unsigned int  Nema_5x7_widthTableLength ;
extern const unsigned char data_table[];
extern const unsigned int  offset_table[];
extern const unsigned char index_table[];
extern const unsigned char width_table[];

int main()
{
    const char *fileName = "Nema_5x7C.c";
    const char *fontName = "Nema_5x7";
    const int fontStartCode  = 0;
    const int fontNumGlyphs  = 255;
    const int fontHeight = 8;
    const int fontMaxWidth = 8;
    const int fontVpitch = 1;

    // 
    // print header and global variables.
    //

    printf (
        "// file name %s\n"
        "#include <stdint.h>\n"
        "const char *fontName = \"%s\";\n"
        "const int fontStartCode = %d;\n"
        "const int fontNumGlyphs = %d;\n"
        "const int fontHeight = %d;\n"
        "const int fontMaxWidth = %d;\n"
        "const int fontVpitch = %d;\n"
        "uint8_t fontData[] = {\n",
        fileName,
        fontName,
        fontStartCode,
        fontNumGlyphs,
        fontHeight,
        fontMaxWidth,
        fontVpitch);

    //
    // allocate the lookup tables for the output file
    //

    int widthTable[255];
    int indexTable[255];
    char charPic[8][8];

    //
    // Output the font table
    //

    for (int i = 0; i < 256; ++i)
    {
        int rowByte;
        int colByte;
        int offset = offset_table[i];
        int width  = width_table[i];
        const unsigned char *rowBytes = &data_table[offset];
        uint8_t colBytes[width];
        memset (colBytes, 0, sizeof colBytes);

        printf ("    // char code %d\n", i);

        widthTable[i] = width_table[i];
        indexTable[i] = offset_table[i];

        uint8_t mask = 0x80;
        for (colByte = 0; colByte < width; ++colByte)
        {
            for (rowByte = 8; rowByte >= 0; --rowByte)
            {
                if (rowBytes[rowByte] & mask)
                    colBytes[colByte] |= (1 << rowByte);
            }

            mask >>= 1;
        }

        for (int c = 0; c < width; ++c)
        {
            int mask = 0x01;
            for (int r = 0; r < 8; ++r)
            {
                charPic[r][c] = (colBytes[c] & mask) ? '*' : ' ';
                mask <<= 1;
            }
        }

        for (int r = 0; r < 8; ++r)
        {
            printf ("    // ");
            for (int c = 0; c < width; ++c)
            {
                printf ("%c", charPic[r][c]);
            }
            printf ("\n");
        }

        printf ("    ");
        for (int c = 0; c < width; ++c)
            printf ("0x%02x, ", colBytes[c]);
        printf ("\n\n");
    }

    printf ("};\n");

    //
    // Output the lookup tables
    //

    printf ("const int widthTable[] = {\n");
    for (int i = 0; i < fontNumGlyphs; ++i)
        printf ("    %d,\t//%d\n", widthTable[i], i);
    printf ("};\n\n");

    printf ("const int indexTable[] = {\n");
    for (int i = 0; i < fontNumGlyphs; ++i)
        printf ("    %d,\t//%d\n", indexTable[i], i);
    printf ("};\n\n");
}
