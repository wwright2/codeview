#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>

#include "log.h"
#include "networkThread.h"
#include "winTypes.h"
#include "sharedData.h"

extern volatile sig_atomic_t runEventLoop;

// ------------------------------------------------------------------------
// This runs in a separate thread, and does nothing but send updates to the
// scanboard. Changes to the displayed data are made by the rest of the
// program. 
//
void *scanThreadLoop (void *arg)
{
    int ptErr = 0;
    DISPLAY_PROFILE *dp = ctaConfig.dp;
    uint8_t data[(dp->cols * dp->clrBytes) + 3];

    memset (data, 0, sizeof data);
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

    for (;;)
    {
        if (runEventLoop == 0)
            break;

        wdTimers[WD_SCAN] = time (0);

	LOCK (frameBuffer.lock, ptErr);
	if (ptErr) die ("Error locking mutex");

	int active = frameBuffer.active;
        for(int r = 0; r < VROWS; ++r)
        {
            const uint8_t *dispData = (uint8_t*)frameBuffer.fb[active][r];

            data[0] = 0xb3;
            data[1] = 0;
            data[2] = r;
            memcpy (&data[3], dispData,  dp->cols * dp->clrBytes);

	    sendBroadcast (data, (dp->cols * dp->clrBytes) + 3);
            struct timeval tv = { 0, 400 };
            select (0, 0, 0, 0, &tv);
        }
	
        //
        // Send brightness settings with every display update
        //

        size_t msgSize = sizeof frameBuffer.brightnessMsg;
        sendBroadcast (frameBuffer.brightnessMsg, msgSize);

	UNLOCK (frameBuffer.lock, ptErr);
	if (ptErr) die ("Error unlocking mutex");

        // Wait a bit before doing it again.
        struct timeval tv = { 0, 30000 };
        select (0, 0, 0, 0, &tv);
    }

    return 0;
}

