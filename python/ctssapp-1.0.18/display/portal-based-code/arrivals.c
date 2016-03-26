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
//  Filename:       arrivals.c
//
//  Description:    Routines for handling API messages from bustracker
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  08/01/11    Joe Halpin      1       original
//
//  Notes:
//
// These routines are called by the main thread to setup portals with the
// information in the API messages.
// ***************************************************************************

#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <errno.h>
#include <string.h>

#include "debug.h"
#include "portal.h"
#include "display.h"
#include "arrivals.h"

//
// This is used to parse each line of arrival data. It finds the three brace
// expressions corresponding to bus number, route name, and due time, and
// returns them in the last three parameters. These parameters must point to
// buffers of sufficient size to hold the results.
//

static const char *regex = 
    "^(.{1,4})[ ]+(.+)[ ]+(...?)$";
    //"^(...)[ ]+(.+)[ ]+([[:digit:]]{1,2}m|Due)$";
static regex_t re;
static regmatch_t match[5];
static char err[1024];
static int arrivalsInitDone = 0;
static const int matchSize = sizeof match / sizeof match[0];

// ---------------------------------------------------------------------------
// Compile the regular expression here so we don't have to do it every time.
//
int arrivalsInit ()
{
    int ret;

    memset (&re, 0, sizeof re);
    memset (&match, 0, sizeof match);
    if ((ret = regcomp (&re, regex, REG_EXTENDED | REG_NEWLINE)) < 0)
    {
        regerror (ret, &re, err, 1024);
        return 1;
    }

    arrivalsInitDone = 1;
    return 0;
}

// ---------------------------------------------------------------------------
// Parse the next line of arrival text. The arrival data is given to us as one
// long string of lines delimited by newlines. The regex will match the
// elements of one line, and stop when it sees the newline. Return a pointer to
// the byte following the newline (which will be the next line, or a null to
// indicate we're done).
//
// Using snprintf() vs strncpy() because the latter isn't guaranteed to null
// terminate the buffer.
//

void copyMatch (const char *s, const regmatch_t *m, char *buf, int *tot)
{
    int start = m->rm_so;
    int end   = m->rm_eo;
    int len   = end - start;

    if (start == -1 || end == -1)
    {
        buf[0] = 0;
    }
    else
    {
        *tot += len + 1;
        snprintf (buf, end - start + 1, &s[start]);
    }
}

// ---------------------------------------------------------------------------

static int parseArrival (
    const char *buf, char *bus, char *route, char *time, int *tot)
{
    int numParsed = 0;
    int ret;

    if (! arrivalsInitDone)
    {
        return 0;
    }

    *tot = 0;

    memset (match, -1, sizeof match);
    ret = regexec (&re, buf, sizeof match / sizeof match[0], &match[0], 0);
    if (ret < 0)
    {
        regerror (ret, &re, err, 1024);
        logErr ("Error executing regex:%s", err);
        return 0;
    }

    int tmp = 0;
    int len = 0;
    copyMatch (buf, &match[1], bus, &tmp);
    if (tmp > 0)
    {
        numParsed += 1;
        len += tmp;
        tmp = 0;
    }

    copyMatch (buf, &match[2], route, &tmp);
    if (tmp > 0)
    {
        numParsed += 1;
        len += tmp;
        tmp = 0;
    }

    copyMatch (buf, &match[3], time, &tmp);
    if (tmp > 0)
    {
        numParsed += 1;
        len += tmp;
        tmp = 0;
    }

    if (len > 0)
        *tot += len;

    return numParsed;
}

// ---------------------------------------------------------------------------
// Parse the arrival data and add it to the arrival portals. There are a max of
// 4 pages per set, and a max of two pages of alerts. The alert page(s) get
// copied to the remaining portals so that we either have one page repeated
// four times, or two pages repeated once in the set.
//
// This is called by main.c when it gets ARRIVALS data from bustracker.
//

static char tmp[ARR_PAGES_PER_SET * ARR_PORTALS_PER_PAGE][MAX_STRING];

void dupLine (int arrPtl, int tmpPtl)
{
    strcpy (arrivalList.p[arrPtl].data,   tmp[tmpPtl]);
    strcpy (arrivalList.p[arrPtl+1].data, tmp[tmpPtl+1]);
    strcpy (arrivalList.p[arrPtl+2].data, tmp[tmpPtl+2]);
}

// ---------------------------------------------------------------------------
//
void dumpArrivals ()
{
    int i = 0;
    int page = 0;
    int numPtls = arrivalList.numPages * arrivalList.portalsPerPage;

    while (i < numPtls)
    {
        for (int line = 0; line < 4; ++line)
        {
            DBG (__FILE__
                " dumpArrivals: page %d, line %d: %s %-30s %s %s %d",
                page,
                line,
                arrivalList.p[i].data,
                arrivalList.p[i+1].data,
                arrivalList.p[i+2].data,
                __FILE__,__LINE__);
            i += 3;
        }
        page += 1;
    }
}

// ---------------------------------------------------------------------------
//
void dupPage (int fromPage, int toPage)
{
    int fromPtl = fromPage * 12;
    int toPtl   = toPage * 12;

    // line 0
    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    // line 1
    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    // line 2
    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    // line 3
    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;

    strcpy (arrivalList.p[toPtl].data,  tmp[fromPtl++]);
    arrivalList.p[toPtl].dataSize = strlen(arrivalList.p[toPtl].data);
    toPtl += 1;
}

static int setupTmpPortals (const uint8_t *buf, size_t len, int *ptl)
{
    // caller must lock stuff

    const char *b = (const char *)buf;
    int page      = 0;
    int line      = 0;
    int maxPortal = ARR_PAGES_PER_SET * ARR_PORTALS_PER_PAGE;
    int total     = 0;

    //
    // Fill the local arrays and figure out what we're going to do before
    // getting the lock.
    //

    memset (tmp, 0, sizeof tmp);
    for (int i = 0; i < arrivalList.portalsPerPage * arrivalList.numPages; ++i)
    {
        // erase any arrivals already here.
        arrivalList.p[i].data[0] = 0;
        arrivalList.p[i].dataSize = 0;
    }
    
    *ptl = 0;
    while (*ptl < maxPortal)
    {
        int lineSize = 0;
        int ret = 0;

        while (b && *b && *b == ' ') // skip leading whitespace 
            b += 1;

        ret = parseArrival (
            b,
            tmp[*ptl],
            tmp[*ptl + 1],
            tmp[*ptl + 2],
            &lineSize);

        if (ret != 3)
        {
            break;
        }

        else
        {
            *ptl += 3;
            line += 1;
        }
            
        b += lineSize;
        total += lineSize;
        if (total >= len)
            break;
    }

    if (*ptl == 0)
    {
        page = 0;
    }

    else if (line < 5)
    {
        page = 1;
    }
    else if (line < 9)
    {
        page = 2;
    }
    else if (line < 13)
    {
        page = 3;
    }
    else
    {
        page = 4;
    }
    
    return page;
}

void arrivalSetData (const uint8_t *buf, size_t len)
{
    int page      = 0;
    int maxPortal = ARR_PAGES_PER_SET * ARR_PORTALS_PER_PAGE;
    int arrPtl    = 0;
    int ptl       = 0;

    dispLockArrivalList ();

    arrivalList.validPages = 0;
    page = setupTmpPortals (buf, len, &ptl);

    if (page > 0)
    {
        // zero out all existing portals
        for (arrPtl = 0; arrPtl < maxPortal; ++arrPtl)
        {
            arrivalList.p[arrPtl].data[0] = 0;
            arrivalList.p[arrPtl].dataSize = 0;
        }
        
        arrPtl = 0;
        while (arrPtl < ptl)
        {
            // strcpy is safe here because snprintf() was used to copy strings
            // into the tmp buffers. It's also faster than snprintf().

            strcpy (arrivalList.p[arrPtl].data, tmp[arrPtl]);
            arrivalList.p[arrPtl].dataSize = 
                strlen(arrivalList.p[arrPtl].data);

            arrPtl += 1;
        }
        
        if (page == 1)
        {
            //
            // Need to copy page 0 onto pages 1, 2 and 3. Page one starts at
            // portal 12, page two starts at portal 24, page three starts at
            // portal 36
            //
            
            dupPage (0, 1);
            dupPage (0, 2);
            dupPage (0, 3);
        }
        
        else if (page == 2)
        {
            dupPage (0, 2);
            dupPage (1, 3);
        }
        
        else if (page == 3)
        {
            dupPage (0, 3);
        }
        
        arrivalList.validPages = 4;
        arrivalList.curPage = 0;
        arrivalList.curPortal = 0;
        arrivalList.changed = 1;
        arrivalList.active  = 1;
        dispUnlockArrivalList ();
        
        //
        // Now set the error list to inactive, in case we had been displaying
        // an error due to no arrivals.
        //
        
        dispLockErrorList ();
        errorList.changed = 0;
        errorList.active  = 0;
        dispUnlockErrorList ();
    }
}

