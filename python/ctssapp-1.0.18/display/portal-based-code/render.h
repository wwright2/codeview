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
//  Filename:       render.h
//
//  Description:    public interface for rendering thread
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  08/05/2011  Joe Halpin      1       initial
//
//
// ***************************************************************************

#ifndef RENDER_H
#define RENDER_H

typedef enum
{
    STARTUP,
    ARRIVAL_LIST,
    ALERT_LIST,
    ERROR_LIST,
    BLANK_LIST,
} PORTAL_LIST_TYPE;


// ------------------------------------------------------------------------
// This is the event loop for the rendering thread. The rendering thread takes
// portal data from whichever portal list is up for rendering next, and turns
// the portal text into bits in the inactive display buffer.
//
// It waits on a condition variable for a timer to expire. The timer is set for
// the length of time the current page should be displayed. By default a page
// of arrivals should be shown for 8 seconds, a page of alerts for 10 seconds,
// and a blank page for 100-500ms. These times are specified in the global
// configuration. 
//

void *renderThreadLoop (void *arg);

// ------------------------------------------------------------------------
// This is called by the API thread to let the renderer know what the next
// message type to be displayed is. Normally it will be either arrival or blank
// page. If an error is received however, it should be displayed instead of
// arrivals (we only get errors because arrivals are not available.
//
// Because we're not doing Alerts at this point, this should only be setting
// Arrivals, or Errors. When an error is received, it will be played until this
// is set to arrival again.
// 
// The code in render.c will automatically switch between arrivals and alerts,
// when alerts get here.
//

void renderSetError (int onoff);

#endif
