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
//  Filename:       render.c
//
//  Description:    implementation of rendering thread
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  08/05/2011  Joe Halpin      1       initial
//
//
// ***************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>

#include "debug.h"
#include "portal.h"
#include "display.h"
#include "font.h"
#include "render.h"
#include "errors.h"

// ------------------------------------------------------------------------
// Private data
// ------------------------------------------------------------------------

//static int lastErr;
//static const char *lastErrMsg = "No error";
static PORTAL_LIST_TYPE curListType = ARRIVAL_LIST;

typedef enum
{
    UNINIT,
    NO_PAGES,
    OUT_OF_PAGES,
    PAGE_DONE,
} PAGE_STATE;

const char *pageStateStr (PAGE_STATE s)
{
    switch (s)
    {
    case UNINIT:
        return "UNINIT";

    case NO_PAGES:
        return "NO_PAGES";

    case OUT_OF_PAGES:
        return "OUT_OF_PAGES";

    case PAGE_DONE:
        return "PAGE_DONE";
    }

    return "unknown";
}

// ------------------------------------------------------------------------
// Render the next page in the list being passed in. Update the state variables
// in the list structure.
//
static PAGE_STATE renderArrivals (PORTAL_LIST *pl)
{
    PAGE_STATE ret = UNINIT;
    static int pages  = 0;

    dispLockArrivalList();

    if (arrivalList.validPages == 0)
    {
        ret =  NO_PAGES;
    }

    if (ret == UNINIT)
    {
        //
        // Make sure there are enough portals left. Adjust for 0 based vs 1
        // based numbers.
        //

        int numPortals = pl->numPages * pl->portalsPerPage;
        if ((numPortals - pl->curPortal + 1) < pl->portalsPerPage)
        {
            pl->curPortal = 0;
        }

        dispLockNextDisplay();
        memset (dispData[nextDisplay].buf,0,sizeof dispData[nextDisplay].buf);
        
        PORTAL *p = &arrivalList.p[pl->curPortal];
        for (int i = 0; i < pl->portalsPerPage; ++i)
        {
            dispDrawPortal (p);
            pl->curPortal += 1;
            ++p;
        }

        if (pl->curPortal == pl->portalsPerPage * pl->numPages - 1)
        {
            pl->curPortal = 0;
            pages = 0;
            ret = OUT_OF_PAGES;
        }

        if (ret == UNINIT)
        {
            pages += 1;
            pl->curPage += 1;
            
            if (pages == 4)
            {
                pages = 0;
                arrivalList.curPage = 0;
                ret = OUT_OF_PAGES;
            }
        }

        dispUnlockNextDisplay();
    }
    
    dispUnlockArrivalList();

    if (ret == UNINIT)
        ret = PAGE_DONE;

    return ret;
}

// ------------------------------------------------------------------------
//
static void renderPage (PORTAL_LIST *pl)
{
    //
    // Caller must lock alertList
    //

    int dispLine;
    dispLockNextDisplay ();
    
    pl->curPortal = 0;
    memset (dispData[nextDisplay].buf, 0, sizeof dispData[nextDisplay].buf);
    for (dispLine = 0; dispLine < 4; ++dispLine)
    {
        dispDrawPortal (&alertList.p[dispLine]);
    }
    
    dispUnlockNextDisplay ();
}


// ------------------------------------------------------------------------
//
void dumpAlertData ()
{
    ALERT *alert = &alertData.alerts[alertData.dispAlert];

    DBG (__FILE__" alert = alertData.alerts[%d]", alertData.dispAlert);
    DBG (__FILE__"    alertData.alertIndex   = %d",alertData.alertIndex);
    DBG (__FILE__"    alertData.dispAlert    = %d",alertData.dispAlert);
    DBG (__FILE__"    alertData.changedAlert = %d",alertData.changedAlert);
    DBG (__FILE__"    alertData.numAlerts    = %d",alertData.numAlerts);
    DBG (__FILE__"    alertData.alertSequence= %d",alertData.alertSequence);
    DBG (__FILE__"    alertData.newAlertsOk   = %d", alertData.newAlertsOk);
    DBG (__FILE__"    alert->numLines = %d", alert->numLines);
    DBG (__FILE__"    alert->dispIndex = %d", alert->dispIndex);
    DBG (__FILE__"    alert->maxIndex = %d", alert->maxIndex);

    if (alert->lineArray != 0)
    {
        DBG (__FILE__"    alert->lineArray[alert->dispIndex] = [%s]", 
            alert->lineArray[alert->dispIndex]);
    }
    else
        DBG (__FILE__"    alert->lineArray is null");
}


// ------------------------------------------------------------------------
//
static PAGE_STATE renderAlert ()
{
    PAGE_STATE ret = UNINIT;

    dispLockAlertList ();
    lockAlertData ();

    //dumpAlertData ();

    if (alertData.numAlerts == 0)
    {
        ret = NO_PAGES;
    }
        
    else
    {
        ALERT *alert = &alertData.alerts[alertData.dispAlert];
        alertData.newAlertsOk = 0;
        
        //
        // Copy up to four lines into the alert portals.
        //
        
        int line;
        for (line = 0; line < 4; ++line)
        {
            if ((alert->dispIndex <= alert->maxIndex) && (alert->lineArray))
            {
                strcpy (
                    alertList.p[line].data, 
                    alert->lineArray[alert->dispIndex]);
                
                alertList.p[line].dataSize = 
                    strlen (alert->lineArray[alert->dispIndex]);
                
                alert->dispIndex += 1;
            }
            
            else
            {
                alertList.p[line].data[0] = 0;
                alertList.p[line].dataSize = 0;
            }
        }
        
        //
        // Now draw the pixels in the display buffer
        //
        
        PORTAL_LIST *pl = &alertList;
        renderPage (pl);
        
        //
        // Update bookkeeping. If there are more lines in the current alert, do
        // nothing, otherwise setup for next alert
        //
        
        if (alert->dispIndex > alert->maxIndex)
        {
            // reset this before moving to the next one.
            alert->dispIndex = 0;

            alertData.dispAlert += 1;
            alertData.alertSequence += 1;
            
            if (alertData.alertSequence == 2)
            {
                alertData.newAlertsOk = 1;
                alertData.alertSequence = 0;
                ret = OUT_OF_PAGES;
            }
            
            if (alertData.dispAlert == alertData.numAlerts)
                alertData.dispAlert = 0;
        }
    }

    //dumpAlertData ();

    unlockAlertData ();
    dispUnlockAlertList ();

    if (ret == UNINIT)
        ret = PAGE_DONE;

    return ret;
}

// ------------------------------------------------------------------------
//
static PAGE_STATE renderError ()
{
    PAGE_STATE ret = UNINIT;

    dispLockErrorList ();
    lockErrorData ();

    //dumpErrorData ();

    if (errorData.numErrors == 0)
    {
        ret = NO_PAGES;
    }
        
    else
    {
        ERROR *error = &errorData.errors[errorData.dispError];
        errorData.newErrorsOk = 0;
        
        //
        // Copy up to four lines into the error portals.
        //
        
        int line;
        for (line = 0; line < 4; ++line)
        {
            if ((error->dispIndex <= error->maxIndex) && (error->lineArray))
            {
                strcpy (
                    errorList.p[line].data, 
                    error->lineArray[error->dispIndex]);
                
                errorList.p[line].dataSize = 
                    strlen (error->lineArray[error->dispIndex]);
                
                error->dispIndex += 1;
            }
            
            else
            {
                errorList.p[line].data[0] = 0;
                errorList.p[line].dataSize = 0;
            }
        }
        
        //
        // Now draw the pixels in the display buffer
        //
        
        PORTAL_LIST *pl = &errorList;
        renderPage (pl);
        
        //
        // Update bookkeeping. If there are more lines in the current error, do
        // nothing, otherwise setup for next error
        //
        
        if (error->dispIndex > error->maxIndex)
        {
            // reset this before moving to the next one.
            error->dispIndex = 0;

            errorData.dispError += 1;
            errorData.errorSequence += 1;
            
            if (errorData.errorSequence == 2)
            {
                errorData.newErrorsOk = 1;
                errorData.errorSequence = 0;
                ret = OUT_OF_PAGES;
            }
            
            if (errorData.dispError == errorData.numErrors)
                errorData.dispError = 0;
        }
    }

    //dumpErrorData ();

    unlockErrorData ();
    dispUnlockErrorList ();

    if (ret == UNINIT)
        ret = PAGE_DONE;

    return ret;
}

// ------------------------------------------------------------------------
//
static PAGE_STATE renderBlank ()
{
    dispLockNextDisplay();
    memset (dispData[nextDisplay].buf,0,sizeof dispData[nextDisplay].buf);
    dispUnlockNextDisplay ();
    return OUT_OF_PAGES;
}

// ------------------------------------------------------------------------
//
void *renderThreadLoop (void *arg)
{
    PORTAL_LIST_TYPE ret;
    struct timeval tv;

    for (;;)
    {
        wdTimers[WD_RENDER] = time (0);

        //
        // Check the PORTAL_LIST objects to see which one(s) have valid data,
        // and figure out which takes priority.
        //
        
        //curListType = checkList (curListType);
        switch (curListType)
        {
        case ARRIVAL_LIST:
            ret = renderArrivals (&arrivalList);

            if (ret == OUT_OF_PAGES)
            {
                curListType = ALERT_LIST;
                tv  = arrivalList.pauseTime;
            }

            else if (ret == NO_PAGES)
            {
                curListType = ALERT_LIST;
                tv.tv_sec = 0;
                tv.tv_usec = 0;
            }

            else // PAGE_DONE
            {
                curListType = BLANK_LIST;
                tv  = arrivalList.pauseTime;
            }
            break;

        case ALERT_LIST:
            ret = renderAlert ();

            if (ret == OUT_OF_PAGES)
            {
                curListType = BLANK_LIST;
                tv = alertList.pauseTime;
            }

            else if (ret == NO_PAGES)
            {
                curListType = BLANK_LIST;
                tv.tv_sec = 0;
                tv.tv_usec = 0;
            }

            else // PAGE_DONE
            {
                curListType = ALERT_LIST;
                tv = alertList.pauseTime;
            }
            break;

        case ERROR_LIST:
            ret = renderError (&errorList);
            if (ret == OUT_OF_PAGES)
            {
                curListType = BLANK_LIST;
                tv = errorList.pauseTime;
            }

            else if (ret == NO_PAGES)
            {
                curListType = BLANK_LIST;
                tv.tv_sec = 0;
                tv.tv_usec = 0;
            }

            else // PAGE_DONE
            {
                curListType = ARRIVAL_LIST;
                tv = errorList.pauseTime;
            }
            break;

            break;

        case BLANK_LIST:
            ret = renderBlank ();
            tv = blankList.pauseTime;
            curListType = ARRIVAL_LIST;
            break;

        default:
            break;
        }

        dispSwitchDisplays ();
        select (0, 0, 0, 0, &tv);
    }

    return 0;
}

int renderIsBlank ()
{
    if (curListType == BLANK_LIST)
        return 1;
    else
        return 0;
}
