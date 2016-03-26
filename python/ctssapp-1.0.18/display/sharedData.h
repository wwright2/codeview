#ifndef SHARED_DATA
#define SHARED_DATA

#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <signal.h>

#include "ctaList.h"
#include "winTypes.h"
#include "font.h"

// ---------------------------------------------------------------------------
// PIXEL defines the number of bytes per pixel. 
//
// For this phase of CTSS, there are two possible sign sizes, as shown
// below. The DISPLAY_PROFILE struct is used to drive rendering based on the
// matrix size as determined at runtime.
//
// The windowing code doesn't (and shouldn't) know about this typedef.
//

typedef uint8_t PIXEL[3];

//
// The scanboard wants the data to be 224*3 bytes per row, and 48 rows. The
// signs don't have that many, but it has to be that way for the scanboard to
// take it. Just render the rows and columns needed starting at 0,0, and the
// scan board will be happy.
//
#define VROWS 48
#define VCOLS 224

// ---------------------------------------------------------------------------
// This is generic config data, it's set once on program startup, and then left
// alone. 
//
typedef struct 
{
    int blinkLen;            // 100-500ms how long to display the blank page
    int arrLen;              // how log to show arrivals (seconds)
    int alrtLen;             // how long to show alerts (seconds)
    int brightnessFloor;     // lowest value we'll set the LEDs to.
    int brightnessWin;       // high/low water mark for changes
    const char *fontName;
    FONT *f;
    DISPLAY_PROFILE *dp;
} CTA_CONFIG;

// ---------------------------------------------------------------------------
// Shared between render thread and scanner thread. This is sized the way it is
// because the scan board wants VROWS rows of data, although we can send the
// actual number of columns.
//
typedef struct
{
    PIXEL **fb[2];
    int active;
    uint8_t brightnessMsg[5];
    uint8_t color[3];
    pthread_mutex_t lock;
} FRAME_BUFFER;

// ---------------------------------------------------------------------------
// main thread puts messages into the queue, and api thread takes them
// out. Main thread allocates memory for the queue entry, and api thread
// deallocates it when done.
//
typedef struct
{
    CTA_LIST head;
    pthread_mutex_t lock;
    //pthread_cond_t  cond;
} API_WORK_QUEUE;

// ---------------------------------------------------------------------------
// Render thread sets up the new window list. The newData struct is set to tell
// the render thread there's new data to display. When the render thread is at
// a good point to switch, it swaps the pointers, and starts using the new
// active window list.
//
typedef struct
{
    CTA_LIST *lst[2];         // active and inactive list
    CTA_LIST *curWin;         // used by renderThread
    int active;               // index for current active list
    int newData;              // Flag used when swapping active & standby
    uint8_t color[3];         // Used to set brightness of pixels
    pthread_mutex_t lock;     // This is shared between api & rendering threads
} WINDOW_LIST;

// ---------------------------------------------------------------------------
// There is an application watchdog which restarts the program in the event
// that threads become deadlocked. The following are used for that purpose.
//
typedef enum
{
    WD_NETWORK,
    WD_API,
    WD_RENDER,
    WD_SCAN,
    WD_MAX,
} WD_INDEX;

extern time_t wdTimers[WD_MAX];
extern API_WORK_QUEUE workQueue;
extern WINDOW_LIST windowList;
extern FRAME_BUFFER frameBuffer;
extern CTA_CONFIG ctaConfig;
extern int exitMainLoop;
extern DISPLAY_PROFILE *dp;
extern volatile sig_atomic_t runEventLoop;
int initSharedData ();

#endif
