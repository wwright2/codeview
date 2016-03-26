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
#include "winEnums.h"
#include "log.h"

#include "ctaList.h"
#include "sharedData.h"
//#include "apiThread.h"

WINDOW *w;
//WINDOW *listHead = 0;
//WINDOW *listTail = 0;

CTA_LIST listHead = CTALIST_INIT (listHead);
FONT *fnt;


#define ROWS  32                // real number of rows
#define COLS  192               // real number of columns (128 - short sign)
#define VROWS 48                // what the scanboard wants
#define VCOLS 224               // what the scanboard wants


// this has to be defined per application. The size of the matrix may change,
// but presumably the color depth will not.

PIXEL **disp[2];
DISPLAY_PROFILE bigDP    = { ROWS, 192, sizeof (PIXEL), 24 };
DISPLAY_PROFILE smallDP  = { ROWS, 128, sizeof (PIXEL), 24 };
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

// Number of descriptors, one for API, one for scanboard, the values of the *FD
// variables are not the file descriptors, they are bits in a bitmap.

//static const char *errStr;

#if 0
typedef struct
{
    int busCol;
    int busWidth;
    int routeCol;
    int routeWidth;
    int dueCol;
    int dueWidth;
} FIELD_DATA;

FIELD_DATA fieldData[] =
{
    { 0, 28, 30, 141, 172, 20 },
    { 0, 28, 30, 80,  108, 20 },
};
#endif

typedef struct
{
    int busCol;
    int busWidth;
    int busAlign;

    int rteCol;
    int rteWidth;
    int rteAlign;

    int dueCol;
    int dueWidth;
    int dueAlign;
} FIELD_DATA;

FIELD_DATA fieldData[] =
{
    { 0, 28, ALIGN_LEFT, 30, 141, ALIGN_LEFT, 172, 20, ALIGN_RIGHT },
    { 0, 28, ALIGN_LEFT, 30, 80,  ALIGN_LEFT, 108, 20, ALIGN_RIGHT },
};

FIELD_DATA *fd;

void fatal (const char *msg)
{
    fprintf (stderr, "%s\n", msg);
    exit (1);
}

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


//static const char *alpha = 
//" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}";


// --------------------------------------------------------------------------
// Open sockets. Returns 0 for success and -1 for error
//

#ifdef ON_BOARD
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
    uint8_t data[(VCOLS*3) + 3] = { 0 };

    memset (data, 0, sizeof data);

    for(int r = 0; r < VROWS; ++r)
    {
        const uint8_t *dispData = (uint8_t*)disp[r];
        
        data[0] = 0xb3;
        data[1] = 0;
        data[2] = r;
        memcpy (&data[3], dispData, dp->cols * dp->clrBytes);

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
            if (dst[r][c][0] > 0)
                printf ("*");

            else
                printf (" ");
        }
        
        printf ("]\n");
    }
    printf ("\n");
}


uint8_t clr[] = { 255, 0, 0 };

int mkWin (
    int row, 
    int col, 
    int width, 
    int align, 
    int eff, 
    int scroll, 
    const char *txt, 
    int pri)
{
    w = newWindow (
        dp,
        row,		   // base row
        col,		   // base column
        width,             // length of window in columns
        8,		   // height of window in rows
        0,                 // graphic offset
        0,                 // graphic offset
        TEXT,		   // text or graphic
        3,		   // color depth - 24 bit supported initially
        clr,               // color to use
        fnt,		   // default font
        align,		   // alignment
        eff,               // effect
        scroll,            // scroll
        txt,               // data to display
        strlen (txt),	   // length of data
        0,		   // ms
        1,		   // id of window.
        pri,		   // determines where window is in order
        -1);		   // time to live in seconds, or -1
    
    if (w == 0)
    {
        printf ("Could not create window: %s\n", winLastErrMsg ());
	fflush (stdout);
        return -1;
    }

    CTA_LIST *entry = malloc (sizeof (CTA_LIST));
    //w->userData = line;
    entry->data = w;

    ctalistAddBack (&listHead, entry);

    dumpList (&listHead);
    return 0;
}


int main(int argc, char *argv[])
{
    int cols = 128;
    const char *fontName = "../../fonts/Nema_5x7C.so";
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

#ifdef ON_BOARD
    if (setupSockets () == -1)
    {
        printf ("Can't setup network\n");
        return -1;
    }

    brightnessMsg[1] = b;
#endif

    disp[0] = allocDisplayBuf (dp);
    if (disp[0] == 0)
    {
        printf ("Can't allocate display buffer\n");
        exit (1);
    }

    struct stat sb;
    if (stat (fontName, &sb) == -1)
    {
        printf ("Can't stat %s\n", fontName);
        exit (1);
    }

    printf ("loading font %s\n", fontName);
    fnt = fontLoad (fontName);
    printf ("fnt = %p\n", fnt);

    if (fnt == 0)
    {
	printf ("Can't find font file\n");
        exit (1);
    }

    //uint8_t clr[3] = { 255, 0, 0 };

    // --------------------------------------------------------------------
    //

#if 0
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
#endif
    

    //sendBroadcast ((uint8_t*)" ", 1);
    
    fnt = fnt;
    const FIELD_DATA *fd;

    if (dp->cols > 128)
        fd = &fieldData[0];
    else
        fd = &fieldData[1];

#if 0
    // ---------------- page 1 - two on-line entries ----------------------
    mkWin (4, fd->busCol, fd->busWidth, ALIGN_LEFT, "15", 1);
    mkWin (4, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "47th  Red Line", 1);
    mkWin (4, fd->dueCol, fd->dueWidth, ALIGN_RIGHT, "7", 1);
    
    mkWin (20, fd->busCol, fd->busWidth, ALIGN_LEFT, "15", 3);
    mkWin (20, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "47th  Red Line", 3);
    mkWin (20, fd->dueCol, fd->dueWidth, ALIGN_RIGHT, "27", 3);
    
    // ---------------- page 2 two-line and one-line -----------------------
    mkWin (4, fd->busCol, fd->busWidth, ALIGN_LEFT, "J7", 1);
    mkWin (0, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "Washington", 1);
    mkWin (4, fd->dueCol, fd->dueWidth, ALIGN_RIGHT, "DUE", 1);
    mkWin (8, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "/Jefferson", 2);
    
    mkWin (20, fd->busCol, fd->busWidth, ALIGN_LEFT, "15", 4);
    mkWin (20, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "47th Red Line", 4);
    mkWin (20, fd->dueCol, fd->dueWidth, ALIGN_RIGHT, "7", 4);
    
    // ---------------- page 3 one one-line, one two-line ------------------
    mkWin (4, fd->busCol, fd->busWidth, ALIGN_LEFT, "15", 1);
    mkWin (4, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "47th Red Line", 1);
    mkWin (4, fd->dueCol, fd->dueWidth, ALIGN_RIGHT, "7", 1);

    mkWin (20, fd->busCol, fd->busWidth, ALIGN_LEFT, "J7", 3);
    mkWin (16, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "Washington", 3);
    mkWin (20, fd->dueCol, fd->dueWidth, ALIGN_RIGHT, "DUE", 3);
    mkWin (24, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "/Jefferson", 4);
    
    // ---------------- page 4 two two-line entries ------------------------
    mkWin (4, fd->busCol, fd->busWidth, ALIGN_LEFT, "J7", 1);
    mkWin (0, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "Washington", 1);
    mkWin (4, fd->dueCol, fd->dueWidth, ALIGN_RIGHT, "DUE", 1);
    mkWin (8, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "/Jefferson", 2);
    
    mkWin (20, fd->busCol, fd->busWidth, ALIGN_LEFT, "J7", 3);
    mkWin (16, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "Washington", 3);
    mkWin (20, fd->dueCol, fd->dueWidth, ALIGN_RIGHT, "7", 3);
    mkWin (24, fd->rteCol, fd->rteWidth, ALIGN_LEFT, "/Jefferson", 4);
#endif

    if (mkWin (0, 0, 192, 0, 0, 0, "This is the bottom window", 1) == -1)
	fatal ("Can't make first window");
    if (mkWin (0, 0, 80, 0, 0, FX_SCROLL_LEFT, " Top Win ", 2) == -1)
	fatal ("Can't make second window");

    CTA_LIST *tmp = listHead.next;
    printf ("dumpList:\n");
    do
    {
        printf ("   node = %p (%s)\n", tmp, winGetText ((WINDOW*)tmp->data));
        tmp = tmp->next;
    } while (tmp != &listHead);

    //    int l1, l2;

    //l1 = 1;

    CTA_LIST *entry = &listHead;
    for (;;)
    {
        memset (disp[0][0], 0, rsize);

        for (;;)
        {
            entry = entry->next;
            if (entry == &listHead)
		break;

	    WINDOW *w = (WINDOW*)entry->data;
            winPrep (w);
	    printf ("%s\n", winGetText (w));
            winDraw (w, (uint8_t ***)disp[0]);

#if 0            
            entry = entry->next;
            if (entry == &listHead)
                entry = entry->next;
            
            w = entry->data;
            l2 = w->userData;

	    if (l2 < l1)
            {
		printf ("------------------------------------------\n");
                l1 = 1;
                break;
            }

            l1 = l2;
#endif
        }

#ifdef ON_BOARD
        updateScanboard ((PIXEL**)disp[0], 0);
        setBrightness ();
        sleep (10);
#else
        //system ("clear");
        printf ("%c[H%c[J\n", 033, 033);
        drawDisplay (dp, (const PIXEL**)disp[0]);
        usleep (5000);
#endif
    }
}
