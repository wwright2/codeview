#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/stat.h>
#include <pthread.h>

#include <syslog.h>
#include <stdarg.h>

#include "font.h"
#include "window.h"
#include "winRender.h"
#include "winTypes.h"
#include "log.h"

WINDOW *w;

#define ROWS  32                // real number of rows
#define COLS  192               // real number of columns
#define VROWS 48                // what the scanboard wants
#define VCOLS 224               // what the scanboard wants


// this has to be defined per application. The size of the matrix may change,
// but presumably the color depth will not.

typedef uint8_t PIXEL[3];
PIXEL **disp;
DISPLAY_PROFILE bigDP    = { 16, 192, 3, 24 };
DISPLAY_PROFILE smallDP  = { 16, 128, 3, 24 };
DISPLAY_PROFILE *dp      = &bigDP;
const char *resourceDir  = "";

size_t rsize = 0;

static const short scanboardPort  = 17478; //0x4446;
static const short controllerPort = 17479; //0x4447;
static const short bustrackerPort = 15545; 
static const int MAX_FDs = 2;
uint8_t brightnessMsg[] = { 0xa3, 32, 0, 0, 0 };

static int ctrlFd;
static int trackerFd;

static struct sockaddr_in bcastAddr;
static struct sockaddr_in controllerAddr;
//static struct sockaddr_in scanboardAddr;
static struct sockaddr_in bustrackerAddr;

static int runEventLoop = 0;

// Number of descriptors, one for API, one for scanboard, the values of the *FD
// variables are not the file descriptors, they are bits in a bitmap.

//static const char *errStr;

// --------------------------------------------------------------------------

PIXEL **allocDisplayBuf (DISPLAY_PROFILE *dp)
{
    if (dp->clrBytes != sizeof (PIXEL))
        LOGFATAL ("Mismatch between dp->clrBytes (%d) and sizeof (PIXEL) (%d)",
            dp->clrBytes, sizeof (PIXEL));

    rsize = (VROWS * (VCOLS * sizeof (PIXEL)));
    PIXEL *buf    = malloc (rsize);
    PIXEL **rows  = malloc (VROWS * sizeof (PIXEL*));

    for (int r = 0; r < VROWS; ++r)
        rows[r] = &buf[r * dp->cols];

    return rows;
}

void freeDisplayBuf (PIXEL **displayBuf)
{
    free (displayBuf[0]);
    free (displayBuf);
}

static const char *alpha = 
" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}";


// --------------------------------------------------------------------------
// Open sockets. Returns 0 for success and -1 for error
//

int setupSockets ()
{
    size_t size = sizeof controllerAddr;

    // --------------------------------------------------------------------
    // Setup the controller socket. This is used to receive sensor data from
    // the scanboard as well as to send display data to it.
    //
    if((ctrlFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)	// listen
    {
        LOGERR ("socket() failed controller: %s", strerror (errno));
        return -1;
    }

    memset(&controllerAddr, 0, sizeof controllerAddr);
    controllerAddr.sin_family = AF_INET;
    controllerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    controllerAddr.sin_port = htons(controllerPort);

    if(bind(ctrlFd, (struct sockaddr*)&controllerAddr, size) < 0)
    {
        LOGERR ("binding ctrlFd failed: %s", strerror (errno));
        return -1;
    }

    int bcast = 1;
    int bcastSize = sizeof bcast;
    if(setsockopt(ctrlFd, SOL_SOCKET, SO_BROADCAST, &bcast, bcastSize)<0)
    {
        LOGERR ("setsockopt(SO_BROADCAST) failed: %s", strerror (errno));
        return -1;
    }
    
    // --------------------------------------------------------------------
    // Setup a socket to catch API updates from the API monitor task. 
    //
    if ((trackerFd = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        LOGERR ("socket() failed for bustracker: %s", strerror (errno));
        return -1;
    }

    memset (&bustrackerAddr, 0, sizeof bustrackerAddr);
    bustrackerAddr.sin_family = AF_INET;
    bustrackerAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    bustrackerAddr.sin_port = htons (bustrackerPort);

    if(bind(trackerFd, (struct sockaddr*)&bustrackerAddr, size) < 0)
    {
        LOGERR ("binding trackerFd failed: %s", strerror (errno));
        return -1;
    }

    // --------------------------------------------------------------------
    // Setup a broadcast address structure. We use this initially to initialize
    // the protocol.
    //
    memset(&bcastAddr, 0, sizeof bcastAddr);
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_port = htons(scanboardPort);
    if(inet_pton(AF_INET, "255.255.255.255", &bcastAddr.sin_addr) != 1)
    {
        LOGERR ("inet_pton() failed: %s", strerror (errno));
        return -1;
    }

    return 0;
}


// --------------------------------------------------------------------------
// Sends the buffer passed in as a broadcast message, using the all-ones
// broadcast address.
//
#ifdef ON_BOARD
// Don't enable this unless it's going to run away from the company LAN.
void sendBroadcast (uint8_t *buf, size_t size)
{
    int ret = sendto (
        ctrlFd, 
        buf, 
        size, 
        0, 
        (struct sockaddr*)&bcastAddr,
        sizeof bcastAddr);

    if(ret == -1)
        LOGERR ("broadcast send failed: %s", strerror (errno));
}

void setBrightness ()
{
    //
    // Send brightness settings with every display update
    //
    
    sendBroadcast (brightnessMsg, sizeof brightnessMsg);
    struct timeval tv = { 0, 500 };
    select (0, 0, 0, 0, &tv);
}

void updateScanboard (PIXEL **disp, int side)
{
    uint8_t data[(VCOLS*3) + 3];

    for(int r = 0; r < VROWS; ++r)
    {
        const uint8_t *dispData = (uint8_t*)disp[r];
        
        data[0] = 0xb3;
        data[1] = 0;
        data[2] = r;
        //memcpy (&data[3], dispData, dp->cols * dp->clrBytes);
        memcpy (&data[3], dispData, VCOLS * 3);

        //data[0] = 0xb3;
        //data[1] = 0;
        //data[2] = r;
        //memcpy (&data[3], dispData, dp->cols * dp->clrBytes);

        sendBroadcast (data, (dp->cols * dp->clrBytes * 3) + 3);
        struct timeval tv = { 0, 400 };
        select (0, 0, 0, 0, &tv);
    }
}
#endif

void getEvents (struct timeval *timeout)
{
    fd_set readSet;
    int maxFd = ctrlFd + 1;

    runEventLoop = 1;

    for (;;)
    {
        if (runEventLoop == 0)
            break;

        struct timeval tv = { 5, 0 };

        FD_ZERO (&readSet);
        FD_SET (ctrlFd, &readSet);

        if (timeout != 0)
        {
            tv.tv_sec = timeout->tv_sec;
            tv.tv_usec = timeout->tv_usec;
        }

        errno = 0;
        int ret = select (maxFd, &readSet, 0, 0, &tv);
        switch(ret)
        {
        case -1:
            if (errno == EINTR)
                continue;
            else
                LOGERR ("Error: in select: %s\n", strerror (errno));
            break;

        case 0:
            break;

        default:
            if (FD_ISSET (ctrlFd, &readSet))
            {
                printf ("getEvents: got sensor data\n");
                printf ("   %d %d %d %d %d\n",
                    brightnessMsg[0],
                    brightnessMsg[1],
                    brightnessMsg[2],
                    brightnessMsg[3],
                    brightnessMsg[4]);
            }

            break;
        }
    }
}

uint8_t *getImg (const char *fname, int *size)
{
   struct stat sb;
   if (stat (fname, &sb) == -1)
   {
       printf ("Could not stat [%s]: %s\n", fname, strerror (errno));
       exit (0);
   }

   int fd = open (fname, O_RDONLY);
   if (fd == -1)
   {
       printf ("Could not open [%s]: %s\n", fname, strerror (errno));
       exit (0);
   }

   uint8_t *gbuf = malloc (sb.st_size);
   if (gbuf == 0)
   {
       printf ("Could not malloc memory\n");
       exit (0);
   }

   if (read (fd, gbuf, sb.st_size) == -1)
   {
       printf ("Error reading image file: [%s]\n", strerror (errno));
       exit (1);
   }

   *size = sb.st_size;
   return gbuf;
}

void drawDisplay (const DISPLAY_PROFILE *dp, const PIXEL **dst)
{
    for (int r = 0; r < dp->rows; ++r)
    {
        printf("[");
        for (int c = 0; c < dp->cols; ++c)
        {
            int p;
            for (p = 0; p < dp->clrBytes; ++p)
            {
                if ((dst[r][c][p] > 0))
                {
                    printf ("*");
                    break;
                }
            }

            if (p == dp->clrBytes)
                printf (" ");
        }
        printf ("]\n");
    }
    printf ("\n");
}

int main(int argc, char *argv[])
{
    int cols = 192;
    const char *fontName = "/ctss/fonts/Nema_5x7C.so";
    uint8_t b = 32;

    struct option longOptions[] =
    {
        { "cols", required_argument, 0, 0 },
        { 0,      0 }
    };

    for (;;)
    {
        int optionIndex = 0;
        
        int c = getopt_long (argc, argv, "f:b:", longOptions, &optionIndex);
        if (c == -1)
            break;
        
        switch (c)
        {
        case 0:
            if (strncmp (longOptions[optionIndex].name, "cols", 4) == 0)
                cols = atoi (optarg);
            break;
        case 'f':
            fontName = optarg;
            break;
        case 'b':
            b = atoi (optarg);
            break;
        default:
            LOGFATAL ("Invalid arguments to display");
            break;
        }
    }

    if (cols < 192)
        dp = &smallDP;

    brightnessMsg[1] = b;

    disp = allocDisplayBuf (dp);
    if (disp == 0)
    {
        printf ("Can't allocate display buffer\n");
        exit (1);
    }

    WINDOW *listHead0 = 0;

    FONT *fnt;

    struct stat sb;
    if (stat (fontName, &sb) == -1)
    {
        printf ("Can't stat %s\n", fontName);
        exit (1);
    }

    fnt = fontLoad (fontName);
    printf ("fnt = %p\n", fnt);

    if (fnt == 0)
    {
	printf ("Can't find font file\n");
        exit (1);
    }

    uint8_t clr[3] = { 255, 0, 0 };

    // --------------------------------------------------------------------
    //

    FONT *f = fnt;

    w = newWindow (
        dp,
        0,		   // base row
        0,		   // base column
        dp->cols,           // length of window in columns
        8,		   // height of window in rows
        0,                 // graphic offset
        0,                 // graphic offset
        TEXT,		   // text or graphic
        3,		   // color depth - 24 bit supported initially
        &clr[0],           // color to use
        f,		   // default font
        0,                 // alignment
        0,                 // effect
        FX_SCROLL_LEFT,
        alpha,             // data to display
        strlen (alpha),	   // length of data
        0,		   // ms
        1,		   // id of window.
        1,		   // determines where window is in order
        -1);		   // time to live in seconds, or -1

    if (w == 0)
    {
        printf ("Could not create window: %s\n", winLastErrMsg ());
        return -1;
    }

   listHead0 = w;

    if (setupSockets () == -1)
    {
        printf ("Can't setup network\n");
        return -1;
    }

    for (;;)
    {
        memset (disp[0], 0, rsize);
        int active = 0;

        // ------- side 0
        WINDOW *cur = listHead0;
        while (cur != 0)
        {
            winPrep (cur);
            active += cur->updateNeeded;
            cur = cur->next;
        }

        if (active > 0)
        {
            cur = listHead0;
            while (cur != 0)
            {
                //winDraw (cur, (uint8_t***)disp);
                winDraw (cur, (uint8_t***)disp);
                cur = cur->next;
            }
        }

#ifdef ON_BOARD
        updateScanboard (disp, 0);
        setBrightness ();
#else
	system ("clear");
	drawDisplay (dp, (const PIXEL **)disp);
	usleep (90000);
#endif
    }
}
