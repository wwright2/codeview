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
//  Filename:       portal.h
//
//  Description:    declares portal stuff
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  07/28/2011  Joe Halpin      1       refactored from previous stuff
//
//  Notes: 
//
//  A portal is the same thing as a window, I just got used to calling it a
// portal because that's what the previous incarnation of this project did. It
// defines one area of the screen and holds information about how it's supposed
// to be rendered.
//
// This requires --std=c99 because of the flexible array member in the
// PORTAL_LIST struct.
// ***************************************************************************

#ifndef PORTAL_H
#define PORTAL_H

#include <pthread.h>
#include <stdint.h>
#include <sys/select.h>

#include "font.h"

#define MAX_STRING 1024         // longest text string we'll deal with

typedef struct
{
    const FONT *font;           // The font used to render the text
    char data[MAX_STRING];      // The text string to be rendered
    size_t size;                // length of the text string
} TEXT_DESCRIPTOR;

typedef enum
{
    P_LEFT,
    P_CENTER,
    P_RIGHT,
} PORTAL_JUSTIFY;

typedef struct portal
{
    PORTAL_JUSTIFY just;
    int id;
    int ulRow;                  // upper left row
    int ulCol;                  // upper left column
    int startCol;               // after justification 
    int width;                  // in columns
    int height;                 // in rows
    const FONT *font;
    char data[MAX_STRING];
    size_t dataSize;
} PORTAL;

//
// This defines a list of portals which constitutes a set of pages to be
// displayed on the sign. 
//

typedef struct 
{
    // Both rendering thread and api thread access this
    pthread_mutex_t lock;       // keeps things from happening at once

    // These are maintained by api message handling thread
    int portalsPerPage;         // how many portals in a page
    int numPages;               // how many pages this list can hold
    int validPages;             // how many pages it actually holds
    int changed;                // set when the api handler modifies the list
    int active;                 // whether the rendering code should use
    struct timeval pauseTime;   // how long to wait till rendering next page
    const char *lineBuf;        // for error and alerts, text goes here.

    // These are maintained by the rendering thread
    int pagesShown;             // Tells when the current block is over
    int curPage;                // which page is to be rendered next (0 based)
    int curPortal;              // which portal is to be rendered (0 based)

    // Both rendering thread and api thread access this
    PORTAL p[];
} PORTAL_LIST;

extern PORTAL_LIST arrivalList;
extern PORTAL_LIST alertList;
extern PORTAL_LIST blankList;
extern PORTAL_LIST errorList;

#endif
