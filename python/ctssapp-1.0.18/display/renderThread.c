#define _BSD_SOURCE
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "log.h"
#include "winTypes.h"
#include "sharedData.h"
#include "ctaList.h"
#include "apiThread.h"
#include "window.h"

typedef enum
{
    RENDER_START,
    RENDER_ARRIVAL,
    RENDER_ERROR,
} RENDER_MSG;

void drawDisplay (const PIXEL **dst)
{
    DISPLAY_PROFILE *dp = ctaConfig.dp;

    printf ("drawDisplay: rows = %d cols = %d\n", dp->rows, dp->cols);
    for (int r = 0; r < dp->rows; ++r)
    {
        printf("%4d [", r);
        for (int c = 0; c < dp->cols; ++c)
        {
            if (dst[r][c][0] > 0)
                printf ("*");

            else
                printf (" ");
        }

        printf ("]\n");
    }
    printf ("\n");
}

// ---------------------------------------------------------------------------
//
static void processWindows ()
{
    // windowList is already locked by renderThreadLoop ()
    static int pagesToShow = 4;
    static int pagesShown  = 1;

    LOGTRACE (" ======= processWidows starting");

    if ((pagesToShow == pagesShown) && windowList.newData)
    {
	windowList.active ^= 1;                // swap active/inactive list
	windowList.curWin = windowList.lst[windowList.active]->next; 
	windowList.newData = 0;
	pagesShown = 0;
    }

    //
    // Once we've shown the required number of pages, reshow them
    // until there's something new to show (windowList.newData == 1).
    //

    pagesShown = (pagesShown == pagesToShow) ? pagesShown : pagesShown + 1;

    int r = ctaConfig.dp->rows;
    int c = ctaConfig.dp->cols;
    int b = ctaConfig.dp->clrBytes;
    int ptErr = 0;

    //
    // Show a blank page to make display blink.
    //
    // It's safe to access this without a lock because this function
    // is the only thing that changes it. This function is also the
    // only thing that writes to the inactive buffer.
    //
    int inactive = frameBuffer.active ^ 1;
    memset ((uint8_t*)frameBuffer.fb[inactive][0], 0, r * c * b);

    // Change the active index    
    LOCK (frameBuffer.lock, ptErr);
    if (ptErr) die ("Error locking mutex");

    frameBuffer.active ^= 1;

    UNLOCK (frameBuffer.lock, ptErr);
    if (ptErr) die ("Error unlocking mutex");

    usleep (ctaConfig.blinkLen * 1000);

    //
    // Now setup the next real page. Again, writing to the inactive
    // buffer doesn't need a lock.
    // 

    CTA_LIST *head = windowList.lst[windowList.active];
    if (ctalistIsEmpty (head))
    {
	return;
    }

    inactive = frameBuffer.active ^ 1;
    memset ((uint8_t*)frameBuffer.fb[inactive][0], 0, r * c * b);

    int line = 1;
    for (;;)
    {
        // Don't try to use the head pointer, it has no data
	if (windowList.curWin == head)
	    windowList.curWin = windowList.curWin->next;

	WINDOW *curWin = (WINDOW*)windowList.curWin->data;
	if ((curWin->userData < line))
	{
            LOGTRACE ("curwin->userData < line (%d < %d)",
                curWin->userData, line);
	    //
	    // The windows are put on the list with the line they
	    // belong to. When we get back to line 1, we're done with
	    // this page.
	    //
	    break;
	}

        LOGTRACE ("drawing line %d: [%s]",
            curWin->userData, winGetText (curWin));

	winPrep (curWin);
	winDraw (curWin, (uint8_t***)frameBuffer.fb[inactive]);
	line = curWin->userData;
	windowList.curWin = windowList.curWin->next;
    }

    LOCK (frameBuffer.lock, ptErr);
    if (ptErr) die ("Error locking mutex");

    frameBuffer.active ^= 1;

    UNLOCK (frameBuffer.lock, ptErr);
    if (ptErr) die ("Error unlocking mutex");

    //drawDisplay ((const PIXEL **)frameBuffer.fb[frameBuffer.active]);
}

// ---------------------------------------------------------------------------
//
void *renderThreadLoop (void *data)
{
    wdTimers[WD_RENDER] = time (0);
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);
    int ptErr = 0;

    for (;;)
    {
	LOCK (windowList.lock, ptErr);
	if (ptErr) die ("Error locking mutex");

        processWindows ();

	UNLOCK (windowList.lock, ptErr);
	if (ptErr) die ("Error unlocking mutex");

        wdTimers[WD_RENDER] = time (0);
        sleep (ctaConfig.arrLen);
    }
}

