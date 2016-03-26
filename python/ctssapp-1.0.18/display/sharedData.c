//
// This initializes the shared data structures. These structures live in this
// module as well.
//

#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "log.h"
#include "sharedData.h"

time_t wdTimers[WD_MAX] = {0};
volatile sig_atomic_t runEventLoop = 1;

// ---------------------------------------------------------------------------
// These are defaults which may be overridden on startup after reading a config
// file. No lock needed because it's not changed after threads start.
//
CTA_CONFIG ctaConfig = 
{
    .blinkLen = 400,            // time to show blank page when blinking (ms)
    .arrLen   = 8,              // time to show arrival pages
    .alrtLen  = 10,             // time to show alert pages
    .brightnessFloor = 70,      // lowest value we'll set the LEDs to.
    .brightnessWin = 4,         // high/low water mark for changes
    .fontName = "../fonts/Nema_5x7C.so", // default font
};


// ---------------------------------------------------------------------------
// For passing data from main thread to api thread
//
API_WORK_QUEUE workQueue =
{
    .head = { &workQueue.head, &workQueue.head },
    .lock = PTHREAD_MUTEX_INITIALIZER,
    //.cond = PTHREAD_COND_INITIALIZER,
};


// ---------------------------------------------------------------------------
// For passing data from api thread to render thread
//
static CTA_LIST arrivalActiveList   = CTALIST_INIT (arrivalActiveList);
static CTA_LIST arrivalInactiveList = CTALIST_INIT (arrivalInactiveList);
static CTA_LIST pageStartList       = CTALIST_INIT (pageStartList);
static CTA_LIST curPageList         = CTALIST_INIT (curPageList);

WINDOW_LIST windowList = 
{
    .lst       = { &arrivalActiveList, &arrivalInactiveList },
    .curWin    = &arrivalActiveList,
    .active    = 0,
    .newData   = 0,
    .color     = { 255, 0, 0 },
    .lock      = PTHREAD_MUTEX_INITIALIZER,
};


// ---------------------------------------------------------------------------
//
FRAME_BUFFER frameBuffer =
{
    .active = 0,
    .brightnessMsg = { 0xa3, 32, 0, 0, 0 },
    .color = { 255, 0, 0 },
    .lock = PTHREAD_MUTEX_INITIALIZER,
};


// ---------------------------------------------------------------------------
//  The sign doesn't actually use all this memory, but the scan board requires
//  it. It wants VROWS worth of data even though there are only 16 rows on the
//  matrix. 
//
static PIXEL **allocDisplayBuf (DISPLAY_PROFILE *dp)
{
    if (dp->clrBytes != sizeof (PIXEL))
        LOGFATAL ("Mismatch between dp->clrBytes (%d) and sizeof (PIXEL) (%d)",
            dp->clrBytes, sizeof (PIXEL));

    size_t rsize = (VROWS * (VCOLS * sizeof (PIXEL)));
    PIXEL *buf    = calloc (1, rsize);
    PIXEL **rows  = calloc (1, (VROWS * sizeof (PIXEL*)));

    for (int r = 0; r < VROWS; ++r)
        rows[r] = &buf[r * dp->cols];

    return rows;
}

// ---------------------------------------------------------------------------
// 

int initSharedData (DISPLAY_PROFILE *dp)
{
    // Allocate the frame buffers. Everything but this is initialized
    // statically above.

    frameBuffer.fb[0] = allocDisplayBuf (dp);
    frameBuffer.fb[1] = allocDisplayBuf (dp);
    if ((frameBuffer.fb[0] == 0) || (frameBuffer.fb[1] == 0))
    {
        LOGERR ("Can't allocate frame buffers");
        printf ("Can't allocate frame buffers: %s", strerror (errno));
        return -1;
    }

    //
    // These get set during provisioning, and are stored in
    // /etc/sysconfig.sh, which is sourced by start-ctss.
    //

    const char *p;
    if ((p = getenv ("BLANKTIME")) != 0) ctaConfig.blinkLen = atoi (p);
    if ((p = getenv ("ARR_LEN"))   != 0) ctaConfig.arrLen = atoi (p);
    if ((p = getenv ("ALRT_LEN"))  != 0) ctaConfig.alrtLen = atoi (p);
    if ((p = getenv ("BR_FLOOR"))  != 0) ctaConfig.brightnessFloor = atoi (p);
    if ((p = getenv ("BR_WIN"))    != 0) ctaConfig.brightnessWin = atoi (p);

    LOGDBG ("ctaConfig.blinkLen = %d", ctaConfig.blinkLen);
    LOGDBG ("ctaConfig.arrLen = %d", ctaConfig.arrLen);
    LOGDBG ("ctaConfig.alrtLen = %d", ctaConfig.alrtLen);
    LOGDBG ("ctaConfig.brightnessFloor = %d", ctaConfig.brightnessFloor);
    LOGDBG ("ctaConfig.brightnessWin = %d", ctaConfig.brightnessWin);

    return 0;
}

