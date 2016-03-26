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
//  Filename:       config.c
//
//  Description:    Implementation of the config file api
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  05/04/2011  Joe Halpin      1       Original
//  Notes: 
//
// See the header file for comments on usage.
// ***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "common.h"

static FILE *fp;
static char *buf;
static char *p;

// ---------------------------------------------------------------------

int cfgOpen (const char *cfgFile)
{
    struct stat sb;

    if ((fp = fopen (cfgFile, "r")) == 0)
    {
        LOGERR ("Error reading %s: %s", cfgFile, strerror (errno));
        return 0;
    }


    if (stat (cfgFile, &sb) == -1)
    {
        LOGERR ("Cannot stat %s", cfgFile);
        return 0;
    }

    if ((buf = malloc (sb.st_size + 1)) == 0)
    {
        LOGERR ("Malloc error: %s", strerror (errno));
        return 0;
    }

    if (fread (buf, sb.st_size, 1, fp) != 1)
    {
        LOGERR ("Error reading %s: %s", cfgFile, strerror (errno));
        return 0;
    }

    buf[sb.st_size] = 0;
    p = buf;
    return 1;
}

// ---------------------------------------------------------------------

static inline void skipWhite ()
{
    while ((*p != 0) && (isspace (*p)))
        ++p;

    //printf ("skipWhite: *p = [%c]\n", *p);
}

// ---------------------------------------------------------------------

static inline void findEOL ()
{
    while ((*p != 0) && (*p != '\n'))
        ++p;

    //printf ("findEOL: *p = [%c]\n", *p);
}

// ---------------------------------------------------------------------

static inline int skipComment ()
{
    if ((*p != 0) && (*p == '#'))
    {
        findEOL ();
        //printf ("skipComment1: *p = [%c]\n", *p);
        return 1;
    }

    //printf ("skipComment2: *p = [%c]\n", *p);
    return 0;
}

// ---------------------------------------------------------------------

static inline void findEqual ()
{
    while ((*p != 0) && (*p != '='))
        ++p;

    //printf ("findEqual: *p = [%c]\n", *p);
}

// ---------------------------------------------------------------------

static const char *getName ()
{
    char *n;

    skipWhite ();
    while (skipComment ())
        skipWhite ();

    if (*p == 0)
        return 0;

    n = p;

    findEqual();
    if (*p == '=')
    {
        *p = 0;
        p++;
    }

    return n;
}

// ---------------------------------------------------------------------

static const char *getValue ()
{
    char *n;

    skipComment ();

    if (*p == 0)
        return 0;


    if (*p == '\n')
    {
        p = 0;
        ++p;
    }

    n = p;
    findEOL ();

    if (*p != 0)
    {
        *p = 0;
        ++p;
    }
    
    return n;
}

// ---------------------------------------------------------------------

int cfgGetNext (const char **name, const char **const val)
{
    *name = getName ();
    *val  = getValue ();

    if (*name == 0 || *val == 0)
        return 0;

    return 1;
}

// ---------------------------------------------------------------------

void cfgClose ()
{
    if (fp != 0)
    {
        fclose (fp);
        fp = 0;
        free (buf);
        buf = 0;
    }
}

#ifdef TEST

int main(int argc, const char *argv[])
{
    const char *fname = "test.cfg";
    const char *name;
    const char *value = "";

    if (argc > 1)
        fname = argv[1];

    if (cfgOpen (fname) == 0)
    {
        printf ("Could not open %s\n", fname);
        return 1;
    }

    while (cfgGetNext (&name, &value))
        printf ("%s = %s\n", name, value);

    cfgClose ();
}
#endif
