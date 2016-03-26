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
 *****************************************************************************/ // ***************************************************************************
//
//  Filename:       apiThread.c
//
//  Description:    Routines for handling API messages from bustracker
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  2012-09-26  Joe Halpin      1       original
//
//  Notes:
//
// This takes data from the busapi program and stores it as lists of windows
// for the renderer.
//
// It blocks on the condition variable in it's work queue. When the main thread
// sees a message from the busapi process it puts it on the work queue and
// signals the condition variable. This thread then wakes up and processes the
// message. 
//
// ***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "log.h"
#include "ctaList.h"
#include "apiThread.h"
#include "sharedData.h"
#include "trackerconf.h"
#include "winEnums.h"
#include "arrivalFuncs.h"
#include "window.h"
#include "trackerconf.h"
#include "apiThread.h"

uint8_t tmpColor[3] = { 255, 0, 0 };
//static int numWindows  = 0;
CTA_LIST winTmp = CTALIST_INIT (winTmp);

FIELD_DATA fieldData[] =
{
    { 0, 28, ALIGN_LEFT, 30, 141, ALIGN_LEFT, 172, 20, ALIGN_RIGHT },
    { 0, 28, ALIGN_LEFT, 30, 80,  ALIGN_LEFT, 108, 20, ALIGN_RIGHT },
};

FIELD_DATA *fd;

// ----------------------------------------------------------------------------
//
static void processNoDataMsg ()
{
    int rowInc = ctaConfig.f->height;
    int width = ctaConfig.dp->cols;
    int line = 1;
    int row  = 0;

    static const char *lines[] = 
    {
	" ",
	"Arrival information",
	"Temporarily Unavailable",
	" "
    };

    for (int i = 0; i < 4; ++i)
    {
	WINDOW *w = mkWin (i * rowInc, 0, width, ALIGN_CENTER, lines[i], line);
	if (w == 0) continue;
	addToStandbyList (w);
	row = (row + rowInc > 24) ? 0 : row + rowInc;
	++line;
    }

    updateTmpWindowList ();
}


// ----------------------------------------------------------------------------
//
static void processErrorMsg (char *msg)
{
    int numLines = 0;
    char *lines[MAX_ARRIVALS+1] = {0};
    
    numLines = getLines (msg, lines);
    if (numLines == 0)
    {
	return;
    }

    if (! ctalistIsEmpty (&winTmp))
	emptyTmpList ();

    doError (lines, numLines);
    updateTmpWindowList ();
}

// ----------------------------------------------------------------------------
//
static void processArrivalMsg (char *msg)
{
    int numLines = 0;
    char *lines[MAX_ARRIVALS+1] = {0};
    
    numLines = getLines (msg, lines);
    if (numLines == 0)
	return;

    if (! ctalistIsEmpty (&winTmp))
	emptyTmpList ();

    if (ctaConfig.dp->cols > 128)
        doLongSign (lines, numLines);
    else
        doShortSign (lines, numLines);

    updateTmpWindowList ();
}

// ----------------------------------------------------------------------------
//
static void processBrightnessMsg (char *msg)
{
    //
    // This message only has one value because we're only using one
    // byte of the three byte color array. It takes a long time to
    // register no matter how it's done, because the windowing code
    // sets it at creation and doesn't look at it again after that. So
    // just set the variable used when creating the windows and let it
    // happen when it does.
    //

    tmpColor[0] = atoi (&msg[1]);
}

// ----------------------------------------------------------------------------
//
static int checkNoData (char *msg)
{
    //
    // The no data message is formatted for the older system. Rather
    // than change that, just recognise what it is, and setup the
    // message we want to display
    //

    char *p = msg;
    while (*p && (*p <= 32))
	++p;

    if (*p)
    {
	if (strncmp ("Arrival  information", p, 20) == 0)
	{
	    processNoDataMsg ();
	    return 0;
	}
    }

    return -1;
}

// ----------------------------------------------------------------------------
//
static int checkErrorMsg (char *msg)
{
    //
    // The error message is formatted for the older system. Rather
    // than change that, just recognise what it is, and setup the
    // message we want to display
    //

    char *p = msg;
    while (*p && (*p <= 32))
	++p;

    if (*p)
    {
        if (strncmp ("Error: ", p, 7) == 0)
	{
	    processErrorMsg (p + 7);
	    return 0;
	}
    }

    return -1;
}

// ----------------------------------------------------------------------------
//
static void processMsg (char *msg)
{ 
    switch (msg[0])
    {
    case BusArrival:
        // These two don't come with the right message id. Check for them
        // here. 
	if (checkNoData (msg) == 0) 
	    return;

        if (checkErrorMsg (msg) == 0)
            return;
            
	LOGTRACE ("calling processArrivalMsg");
        processArrivalMsg (&msg[1]);
        break;
	
    case BusError:
	LOGTRACE ("calling processErrorMsg");
        processErrorMsg (&msg[1]);
        break;
	
    case BrightnessMsgId:
	LOGTRACE ("calling processBrightnessMsg: msg = [%s]", msg);
        processBrightnessMsg (&msg[1]);
        break;

    case NoDataMsgId:
	LOGTRACE ("calling noData msg");
	processNoDataMsg (&msg[1]);
	break;

    default:
        LOGERR ("apiThread: Invalid message id: %d", msg[0]);
        break;
    }
}

// ----------------------------------------------------------------------------
//
void *apiThreadLoop (void *data)
{
    wdTimers[WD_API] = time (0);
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);
    int ptErr = 0;

    if (arrivalsInit () == -1)
	LOGFATAL ("Could not initialize apiThread");

    if (ctaConfig.dp->cols > 128)
	fd = &fieldData[0];
    else
	fd = &fieldData[1];

    for (;;)
    {
        LOGTRACE ("apiThreadLoop: at top of thread");

	//
	// This should probably be using a condition variable to let
	// this thread sleep completely till something needed to be
	// done. However I get deadlocks when doing it that way. I
	// suspect the threads package with this version of buildroot,
	// but I can't prove it. It's so old it's not even supported
	// anymore though.
	//

	sleep (5);
	LOCK (workQueue.lock, ptErr);
	if (ptErr) die ("Error locking mutex");

        while (! ctalistIsEmpty (&workQueue.head))
	{
	    CTA_LIST *item = workQueue.head.next;
	    ctalistDel (item->prev, item->next);

	    processMsg ((char*)item->data);
	    free (item->data);
	    free (item);
	}

	UNLOCK (workQueue.lock, ptErr);
	if (ptErr) die ("Error unlocking mutex");

	wdTimers[WD_API] = time (0);
    }
}
