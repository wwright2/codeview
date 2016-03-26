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
//  Filename:       mainInit.h
//
//  Description:    static initialization of arrays, etc. Just to keep the
//                  clutter in main.c down a bit.
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  05/02/11    Joe Halpin      1       Initial
//  Notes: 
//
// ***************************************************************************

#ifndef mainInit_h
#define mainInit_h

#ifndef MAIN
#   error This should only be included by main.c
#endif

//
// This is a mapping of the brightness levels we get from the
// scanboard to the level we set in our brightness message. The
// scanboard sends us a value between 0 - 255, and we set a
// corresponding value between 1 - 32.
//
// Divide the sensor range 0 - 255 into 32 sections. Assign a brightness value
// to all the sensor values in each section.
//

static unsigned char brightnessLevels[] = {
    1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,
    9,9,9,9,9,9,9,9,
    10,10,10,10,10,10,10,10,
    11,11,11,11,11,11,11,11,
    12,12,12,12,12,12,12,12,
    13,13,13,13,13,13,13,13,
    14,14,14,14,14,14,14,14,
    15,15,15,15,15,15,15,15,
    16,16,16,16,16,16,16,16,
    17,17,17,17,17,17,17,17,
    18,18,18,18,18,18,18,18,
    19,19,19,19,19,19,19,19,
    20,20,20,20,20,20,20,20,
    21,21,21,21,21,21,21,21,
    22,22,22,22,22,22,22,22,
    23,23,23,23,23,23,23,23,
    24,24,24,24,24,24,24,24,
    25,25,25,25,25,25,25,25,
    26,26,26,26,26,26,26,26,
    27,27,27,27,27,27,27,27,
    28,28,28,28,28,28,28,28,
    29,29,29,29,29,29,29,29,
    30,30,30,30,30,30,30,30,
    31,31,31,31,31,31,31,31,
    32,32,32,32,32,32,32,32
};


typedef struct
{
    PORTAL *p;
    PORTAL_JUSTIFY j;
    int r;
    int c;
    int w;
    int h;
    PORTAL_TYPE t;
    const void *data;
    FONT *f;
    size_t len;
} PORTAL_CONFIG;

typedef enum
{
    BUS1_NUM,
    BUS1_ROUTE,
    BUS1_DUE,
    BUS2_NUM,
    BUS2_ROUTE,
    BUS2_DUE,
    BUS3_NUM,
    BUS3_ROUTE,
    BUS3_DUE,
    BUS4_NUM,
    BUS4_ROUTE,
    BUS4_DUE,
    NUM_PORTALS
} PORTAL_IDX;

#define BUS_COL 0
#define BUS_WIDTH 18

#define ROUTE_COL 20
#define ROUTE_WIDTH 150

#define DUE_COL 172
#define DUE_WIDTH 20


static PORTAL *phead = 0;
static PORTAL *ptail = 0;
static PORTAL_CONFIG portalList[NUM_PORTALS] = 
{
    // BUS1_NUM,
    {
        .r = 0,
        .c = BUS_COL,
        .w = BUS_WIDTH,
        .h = 8,
        .j = P_RIGHT,
        .t = PORTAL_TEXT, 
    },

    // BUS1_ROUTE,
    {
        .r = 0,
        .c = ROUTE_COL,
        .w = ROUTE_WIDTH,
        .h = 8,
        .j = P_LEFT,
        .t = PORTAL_TEXT, 
    },

    // BUS1_DUE,
    {
        .r = 0,
        .c = DUE_COL,
        .w = DUE_WIDTH,
        .h = 8,
        .j = P_RIGHT,
        .t = PORTAL_TEXT, 
    },

    // BUS2_NUM,
    {
        .r = 8,
        .c = BUS_COL,
        .w = BUS_WIDTH,
        .h = 8,
        .j = P_RIGHT,
        .t = PORTAL_TEXT, 
    },

    // BUS2_ROUTE,
    {
        .r = 8,
        .c = ROUTE_COL,
        .w = ROUTE_WIDTH,
        .h = 8,
        .j = P_LEFT,
        .t = PORTAL_TEXT, 
    },

    // BUS2_DUE,
    {
        .r = 8,
        .c = DUE_COL,
        .w = DUE_WIDTH,
        .h = 8,
        .j = P_RIGHT,
        .t = PORTAL_TEXT, 
    },

    // BUS3_NUM,
    {
        .r = 16,
        .c = BUS_COL,
        .w = BUS_WIDTH,
        .h = 8,
        .j = P_RIGHT,
        .t = PORTAL_TEXT, 
    },

    // BUS3_ROUTE,
    {
        .r = 16,
        .c = ROUTE_COL,
        .w = ROUTE_WIDTH,
        .h = 8,
        .j = P_LEFT,
        .t = PORTAL_TEXT, 
    },

    // BUS3_DUE,
    {
        .r = 16,
        .c = DUE_COL,
        .w = DUE_WIDTH,
        .h = 8,
        .j = P_RIGHT,
        .t = PORTAL_TEXT, 
    },

    // BUS4_NUM,
    {
        .r = 24,
        .c = BUS_COL,
        .w = BUS_WIDTH,
        .h = 8,
        .j = P_RIGHT,
        .t = PORTAL_TEXT, 
    },

    // BUS4_ROUTE,
    {
        .r = 24,
        .c = ROUTE_COL,
        .w = ROUTE_WIDTH,
        .h = 8,
        .j = P_LEFT,
        .t = PORTAL_TEXT, 
    },

    // BUS4_DUE,
    {
        .r = 24,
        .c = DUE_COL,
        .w = DUE_WIDTH,
        .h = 8,
        .j = P_RIGHT,
        .t = PORTAL_TEXT, 
    },
};

static const char *testStrings[] = 
{
    // bus 1 data
    "",
    "",
    "",
    "",

    // bus 2 data
    "",
    "                SIGN  INITIALIZING",
    "",
    "",

    // bus 3 data
    "",
    "",
    "",
    "",
    
    // bus 4 data
    "",
    "",
    "",
};

#endif
