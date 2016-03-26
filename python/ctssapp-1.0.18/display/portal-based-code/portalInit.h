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
//  Filename:       portalInit.h
//
//  Description:    static initialization of the portal structures
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  07/25/11    Joe Halpin      1       I don't like this either
//
//  Notes: 
//
//  This should probably be in the C file for the sake of information hiding,
//  but there's just too much crap here and I don't want it in the C file.
// ***************************************************************************

#ifndef PORTALINIT_H
#define PORTALINIT_H

#ifndef DISPLAY_INCLUDE
#  error This must only be included by display.c
#endif

#include "display.h"

PORTAL_LIST arrivalList =
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .portalsPerPage = 12,
    .numPages = ARR_PAGES_PER_SET,
    .validPages = 0,
    .pauseTime = { 8, 0 },
    .curPage = 0,

    .p =
    {
        // ------------------------- PAGE 1 ------------------------------
        // ------- line 1
        {
            .ulRow = 0,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 0,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 0,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 2
        {
            .ulRow = 8,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 8,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 8,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 3
        {
            .ulRow = 16,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 16,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 16,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 4
        {
            .ulRow = 24,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 24,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 24,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------------------------- PAGE 2 ------------------------------
        // ------- line 1
        {
            .ulRow = 0,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 0,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },
        
        {
            .ulRow = 0,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },
        
        // ------- line 2
        {
            .ulRow = 8,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 8,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 8,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 3
        {
            .ulRow = 16,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 16,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 16,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 4
        {
            .ulRow = 24,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 24,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 24,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------------------------- PAGE 3 ------------------------------
        // ------- line 1
        {
            .ulRow = 0,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 0,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 0,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 2
        {
            .ulRow = 8,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 8,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 8,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 3
        {
            .ulRow = 16,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 16,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 16,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 4
        {
            .ulRow = 24,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 24,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 24,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },


        // ------------------------- PAGE 4 ------------------------------
        // ------- line 1
        {
            .ulRow = 0,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 0,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 0,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 2
        {
            .ulRow = 8,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 8,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 8,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 3
        {
            .ulRow = 16,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 16,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 16,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },

        // ------- line 4
        {
            .ulRow = 24,
            .ulCol = BUS_COL,
            .width = BUS_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 24,
            .ulCol = ROUTE_COL,
            .width = ROUTE_WIDTH,
            .just  = P_LEFT,
        },

        {
            .ulRow = 24,
            .ulCol = DUE_COL,
            .width = DUE_WIDTH,
            .just  = P_RIGHT,
        },
    },
};


PORTAL_LIST alertList = 
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .portalsPerPage = 4,
    .numPages = ALRT_PAGES_PER_SET,
    .validPages = 0,
    .pauseTime = { 10, 0 },
    .curPage = 1,
    .p =
    {

        // ------------------------- PAGE 1 ------------------------------
        {
            .ulRow = 0,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 8,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 16,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 24,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },

        // ------------------------- PAGE 2 ------------------------------
        {
            .ulRow = 0,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 8,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 16,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 24,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },

        // ------------------------- PAGE 3 ------------------------------
        {
            .ulRow = 0,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 8,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 16,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 24,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },

        // ------------------------- PAGE 4 ------------------------------
        {
            .ulRow = 0,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 8,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 16,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
            
        {
            .ulRow = 24,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
    },
};


PORTAL_LIST errorList = 
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .portalsPerPage = 4,
    .numPages = ALRT_PAGES_PER_SET,
    .validPages = 0,
    .pauseTime = { 8, 0 },
    .curPage = 1,
    .p =
    {

        // ------------------------- PAGE 1 ------------------------------
        {
            .ulRow = 0,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 8,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 16,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 24,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },

        // ------------------------- PAGE 2 ------------------------------
        {
            .ulRow = 0,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 8,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 16,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 24,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },

        // ------------------------- PAGE 3 ------------------------------
        {
            .ulRow = 0,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 8,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 16,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 24,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },

        // ------------------------- PAGE 4 ------------------------------
        {
            .ulRow = 0,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 8,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 16,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
            
        {
            .ulRow = 24,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_CENTER,
        },
    },
};


PORTAL_LIST blankList =
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .portalsPerPage = 4,
    .numPages = 1,
    .validPages = 0,
    .pauseTime = { 0, 200000 },
    .curPage = 1,
    .p =
    {
        {
            .ulRow = 0,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
        
        {
            .ulRow = 8,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
        
        {
            .ulRow = 16,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
        
        {
            .ulRow = 24,
            .ulCol = 0,
            .width = DISP_COLS,
            .just  = P_LEFT,
        },
    },
};


#endif
