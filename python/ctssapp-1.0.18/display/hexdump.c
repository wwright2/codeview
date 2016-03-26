// ***************************************************************************
//  ... Copyright (c) 2008 Luminator Mark IV
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator Mark IV
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       eipUtils.c
//
//  Description:    Functions used by more than one file
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  06/30/2010  Joe Halpin      0       Original
//
//  Notes:
// ***************************************************************************

// System include files
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
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

#ifdef MAIN

void die (const char *msg)
{
    perror (msg);
    exit (1);
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        fprintf(stderr,"Syntax: hexdump <file>");
        return 1;
    }

    struct stat sb;
    if (stat (argv[1], &sb) == -1)
        die ("Couldn't stat file");

    int fd = open (argv[1], O_RDONLY);
    if (fd == -1)
        die ("open");

    char *buf = malloc (sb.st_size + 1);
    if (buf == 0)
        die ("malloc");

    if (read (fd, buf, sb.st_size) != sb.st_size)
        die ("read");
    
    buf[sb.st_size] = 0;
    hexdump (buf, sb.st_size);

    free (buf);
    close (fd);
    return 0;
}
#endif
