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
//  Filename:       main.c
//
//  Description:    Driver to convert BitfontCreator output .c file
//                  to .FNT format
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  04/20/11    Joe Halpin      1       
//
//  Notes: 
//
//  This has to be kept in sync with font.c in the controller code.
//
// ***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "tgtFont.h"

void die (const char *msg)
{
    perror (msg);
    exit (1);
}

int main()
{
    FILE *fp;
    FILE *tmp;
    char fname[41];
    uint16_t dtLength = htons (dataTableLength);
    uint16_t otLength = htons (offsetTableLength);
    uint16_t itLength = htons (indexTableLength);
    uint16_t wtLength = htons (widthTableLength);

    memset (fname, 0, sizeof fname);
    if (strlen (fontName) > sizeof fname)
        printf ("%s larger than allowed field length\n",fontName);

    snprintf (fname, sizeof fname, "%s.FNT", fontName);

    if ((fp = fopen (fname, "w")) == 0)
        die ("Error opening output file");

    fputc ('F', fp);
    fputc ('N', fp);
    fputc ('T', fp);
    fputc ('0', fp);
    fputc ('0', fp);

    printf ("fname pos = %ld\n", ftell(fp));
    fwrite (fname, sizeof fname, 1, fp);
    printf ("fontHeight pos = %ld\n", ftell(fp));
    fwrite (&fontHeight, sizeof fontHeight, 1, fp);
    printf ("fontWidth pos = %ld\n", ftell(fp));
    fwrite (&fontWidth, sizeof fontWidth, 1, fp);
    printf ("variable pos = %ld\n", ftell(fp));
    fwrite (&variable, sizeof variable, 1, fp);
    printf ("dtLength pos = %ld\n", ftell(fp));
    fwrite (&dtLength, 2, 1, fp);
    printf ("itLength pos = %ld\n", ftell(fp));
    fwrite (&itLength, 2, 1, fp);
    printf ("otLength pos = %ld\n", ftell(fp));
    fwrite (&otLength, 2, 1, fp);
    printf ("wtLength pos = %ld\n", ftell(fp));
    fwrite (&wtLength, 2, 1, fp);

    printf ("data_table pos = %ld\n", ftell(fp));
    fwrite (data_table, dataTableLength, 1, fp);
    tmp = fopen ("data_table", "w");
    fwrite (data_table, dataTableLength, 1, tmp);
    fclose (tmp);

    printf ("index_table pos = %ld\n", ftell(fp));
    fwrite (index_table, indexTableLength, 1, fp);
    tmp = fopen ("index_table", "w");
    fwrite (index_table, indexTableLength, 1, tmp);
    fclose (tmp);

    printf ("offset_table pos = %ld\n", ftell(fp));
    fwrite (offset_table, offsetTableLength, 1, fp);
    tmp = fopen ("offset_table", "w");
    fwrite (offset_table, offsetTableLength, 1, tmp);
    fclose (tmp);

    printf ("width_table pos = %ld\n", ftell(fp));
    fwrite (width_table, widthTableLength, 1, fp);
    tmp = fopen ("width_table", "w");
    fwrite (width_table, widthTableLength, 1, tmp);
    fclose (tmp);

    fclose (fp);

    printf ("index_table length  = %u\n", sizeof index_table);
    printf ("offset_table length = %u\n", sizeof offset_table);

    char testf[1024];
    sprintf (testf, "%s.output", fontName);

    fp = fopen (testf, "w");
    fwrite (index_table, indexTableLength, 1, fp);
    fclose (fp);
}
