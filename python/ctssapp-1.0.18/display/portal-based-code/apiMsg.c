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
//  Filename:       apiMsg.c
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
#include <errno.h>
#include <string.h>

#include "debug.h"
#include "portal.h"
#include "display.h"
#include "apiMsg.h"
#include "arrivals.h"
#include "alerts.h"
#include "network.h"

void handleInput (uint32_t bits);

// ---------------------------------------------------------------------------
// 
//
void setAlertData (const uint8_t *buf, size_t len)
{
    alertSetData (buf, len);
}


// ---------------------------------------------------------------------------
// 
//
void setArrivalData (const uint8_t *buf, size_t len)
{
    arrivalSetData (buf, len);
}


// ---------------------------------------------------------------------------
// Copy the next alert from the buffer into alert, and return a pointer to the
// following alert, or 0 if there are no more. 
//
const char *getNextAlert (const char *alrtStr, char *alert)
{
    return 0;
}

// ---------------------------------------------------------------------------
// Setup an error message. We get this if there is an error getting
// arrivals. In this case set the arrival list to inactive and set the message
// into the error page.
//
// The errText variable is shared with the rendering stuff. Don't change it
// unless you have a lock on the errorList structure.
//
// This uses snprintf() instead of strncpy() because the latter is braindead
// and might not null terminate the buffer.
//
static char errText[ERR_PORTALS_PER_PAGE * ERR_PAGES_PER_SET * MAX_STRING];
void setErrorData (const uint8_t *buf, size_t len)
{
    dispLockErrorList ();
    snprintf (errText, sizeof errText, "%s", buf);
    errorList.lineBuf = errText;
    errorList.changed = 1;
    errorList.active  = 1;
    dispUnlockErrorList ();

    dispLockArrivalList ();
    DBG ("   =======> setting arrivalList.validPages to 0");
    arrivalList.validPages = 0;
    arrivalList.changed = 0;
    arrivalList.active  = 0;
    dispUnlockArrivalList ();
}


//
// This looks for messages on either the scanboard socket or the API
// socket. When it gets activity it sets a flag for the active socket(s)
// and invokes handleInput with the bitmask.
//

void *apiThreadLoop (void *dummy)
{
    DBG ("calling getEvents");
    getEvents (handleInput, 0);
    dispCleanup ();
    return 0;
}
