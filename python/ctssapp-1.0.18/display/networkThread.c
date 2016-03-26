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
//  Filename:       networkThread.c
//
//  Description:    networking functions
//
//  Revision History:
//  Date        Name            Ver     Remarks
//
//  Notes:
//
// ***************************************************************************

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include <getopt.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "networkThread.h"
#include "log.h"
#include "sharedData.h"
#include "trackerconf.h"

static const short scanboardPort  = 17478; //0x4446;
static const short controllerPort = 17479; //0x4447;
static const short bustrackerPort = 15545; 
static const int MAX_FDs = 2;
static char udpMsg[65535]; // big enough for max sized UDP message.

int ctrlFd = -1;
int trackerFd = -1;
int dbgFd = -1;

static struct sockaddr_in bcastAddr;
static struct sockaddr_in controllerAddr;
static struct sockaddr_in scanboardAddr;
static struct sockaddr_in bustrackerAddr;

void hexdump (const char *s, size_t sz);

#define DBG_FIFO "/var/run/display/debug"

// Number of descriptors, one for API, one for scanboard, the values of the *FD
// variables are not the file descriptors, they are bits in a bitmap.

//static const char *errStr;

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

    // --------------------------------------------------------------------
    // Setup a FIFO for changing the debug value at run-time. 
    //
    unlink (DBG_FIFO);
    if (mkfifo (DBG_FIFO, 0755) == -1)
	LOGERR ("Could not create %s: %s", DBG_FIFO, strerror (errno));
    else
    {
	if ((dbgFd = open (DBG_FIFO, O_RDWR)) == -1)
	    LOGERR ("Cold not open %s: %s", DBG_FIFO, strerror (errno));
    }

    return 0;
}


// --------------------------------------------------------------------------
// Sends the buffer passed in as a broadcast message, using the all-ones
// broadcast address.
//
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
    {
        LOGERR ("broadcast send failed: %s", strerror (errno));
	exit (1);
    }
}

// --------------------------------------------------------------------------
// Get a UDP message from the API monitor and returns it. The size of the
// message is returned in the second parameter (value - result)
//
uint8_t *receiveApiData (uint8_t *buf, ssize_t *size)
{
    memset (&bustrackerAddr, 0, sizeof bustrackerAddr);
    socklen_t len = sizeof bustrackerAddr;

    if((*size = recvfrom(
            trackerFd, 
            buf,
            *size,
            0,
            (struct sockaddr*)&bustrackerAddr,
            &len)) < 0)
    {
        LOGERR ("receiving sensor data");
        return 0;
    }

    buf[*size] = 0;
    return buf;
}


// --------------------------------------------------------------------------
// Gets a UDP message from the scan board. All we get from them are sensor
// readings. Returns 0 for success and -1 for error, with error data in the
// netError() and netErrorCode() functios.
//
uint8_t *receiveSensorData (size_t *size)
{
    static uint8_t buf[5];

    memset (&scanboardAddr, 0, sizeof scanboardAddr);
    socklen_t len = sizeof scanboardAddr;
    if(recvfrom(
            ctrlFd, 
            buf,
            sizeof buf,
            0,
            (struct sockaddr*)&scanboardAddr,
            &len) < 0)
    {
        LOGERR ("Error receiving sensor data: %s", strerror (errno));
        return 0;
    }

    *size = sizeof buf;
    return buf;
}


// --------------------------------------------------------------------------
// Returns a string describing the last error. The return value points into a
// static buffer, don't try to delete it. Copy it if you need to keep it past
// one call to this function.
//
const char *netError ()
{
    return "Not implented";
}


// --------------------------------------------------------------------------
//
void addToWorkQueue (CTA_LIST *item)
{
    int ptErr = 0;

    LOCK (workQueue.lock, ptErr);
    if (ptErr) die ("Error locking mutex");

    ctalistAddBack (&workQueue.head, item);
    //pthread_cond_signal (&workQueue.cond);

    UNLOCK (workQueue.lock, ptErr);
    if (ptErr) die ("Error unlocking mutex");
}

// ------------------------------------------------------------------------
//
#define BR_MSGSIZE 7
void setBrightness (int b)
{
    // this needs to construct a pseudo message and put it on the api thread
    // work queue. The message consists of an id and an integer.
    //
    // The msg buffer and item must be allocated because the apiThread will
    // free them when it's done with the message.

    char *msg = malloc (BR_MSGSIZE);
    CTA_LIST *item = malloc (sizeof (CTA_LIST));
    if ((msg == 0) || (item == 0))
    {
        LOGERR ("Can't allocate work queue item in setBrightness");
	printf ("Can't allocate work queue item in setBrightness\n");
        free (msg);
        free (item);
        return;
    }

    snprintf (msg, BR_MSGSIZE, "%c %d", BrightnessMsgId, b);
    ctalistInit (item);
    item->data = (void*) msg;
    addToWorkQueue (item);
}

// ------------------------------------------------------------------------
//
static void setBrightnessLevel (int b)
{
    //
    // Use a sliding window to keep from oscillating in corner cases. Set the
    // high and low water marks depending on the current brightness level from
    // the sensor data.
    //

    static int hw = -9999;
    static int lw = 9999;
    int wSize = ctaConfig.brightnessWin;

    if (b < ctaConfig.brightnessFloor)
	b = ctaConfig.brightnessFloor;

    if ((b > hw) || (b < lw))
    {
        hw = b + wSize;
        lw = b - wSize;
        setBrightness (b);
    }
}

// ------------------------------------------------------------------------

static void handleSensorData()
{
    size_t  dataLen;
    uint8_t *sensorData = receiveSensorData (&dataLen);
    if (sensorData == 0)
    {
        LOGERR ("Error receiving sensor data: %s %d",__FILE__,__LINE__);
        return;
    }

    // set brightness level on sign to correspond with the ambient
    // brightness. 
    setBrightnessLevel (sensorData[1]);

    //
    // Get the current temperature readings and compare them with the last. If
    // they changed, update the file in /home/cta so the status.xml file will
    // reflect reality.
    //

    static uint8_t firstTime = 1;
    static uint8_t lastIntTemp = 0;
    static uint8_t lastOutTemp = 0;
    const char *intTempFile = "/home/cta/intTemperature.tmp";
    const char *realIntFile = "/home/cta/intTemperature.txt";
    const char *outTempFile = "/home/cta/outTemperature.tmp";
    const char *realOutFile = "/home/cta/outTemperature.txt";
    uint8_t intTempC = sensorData[3];
    uint8_t outTempC = sensorData[2];

    if ((intTempC > lastIntTemp + 2) || 
        (intTempC < lastIntTemp - 2) || 
        (firstTime == 1))
    {
        lastIntTemp = intTempC;

        FILE *fp = fopen (intTempFile, "w");
        if (! fp)
            LOGERR ("Could not open %s: %s", intTempFile, strerror (errno));
        else
        {
            fprintf (fp, "%dC\n", intTempC);
            fclose (fp);
            rename (intTempFile, realIntFile);
        }
    } 

    if ((outTempC > lastOutTemp + 2) || 
        (outTempC < lastOutTemp - 2) ||
        (firstTime == 1))
    {
        lastOutTemp = outTempC;

        FILE *fp = fopen (outTempFile, "w");
        if (! fp)
            LOGERR ("Could not open %s: %s", outTempFile, strerror (errno));
        else
        {
            fprintf (fp, "%dC", outTempC);
            fclose (fp);
            rename (outTempFile, realOutFile);
        }
    } 
    
    firstTime = 0;
}


// --------------------------------------------------------------------------
// Add the incoming message to the API thread's work queue.  Memory allocated
// here is free'd by the apiThread
//
static void handleApiData ()
{
    ssize_t sz = sizeof udpMsg;
    if (receiveApiData ((uint8_t*)udpMsg, &sz) == 0)
    {
        LOGERR ("Error receiving api data: %s", strerror (errno));
        return;
    }
    
    char *dupMsg = strdup (udpMsg);
    if (dupMsg == 0)
    {
        LOGERR ("Error allocating memory for msg in handleApiData");
        return;
    }

    CTA_LIST *item = malloc (sizeof (CTA_LIST));
    if (item == 0)
    {
        LOGERR ("Error allocating memory for list item in handleApiData");
        free (dupMsg);
        return;
    }

    ctalistInit (item);
    item->data = (void*) dupMsg;
    addToWorkQueue (item);
}

// --------------------------------------------------------------------------
//
void *networkThreadLoop (void *data)
{
    fd_set readSet;
    int maxFd = (ctrlFd > trackerFd) ? ctrlFd + 1 : trackerFd + 1;
    wdTimers[WD_NETWORK] = time (0);

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, 0);
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

    for (;;)
    {
        LOGTRACE ("networkThreadLoop: at top of loop");
        if (runEventLoop == 0)
            break;
        
        struct timeval tv = { 5, 0 };

        FD_ZERO (&readSet);
        FD_SET (ctrlFd, &readSet);
        FD_SET (trackerFd, &readSet);
	maxFd = (trackerFd > ctrlFd) ? trackerFd : ctrlFd;

	if (dbgFd != -1) 
	{
	    FD_SET (dbgFd, &readSet);
	    maxFd = (dbgFd > maxFd) ? dbgFd : maxFd;
	}

	maxFd += 1;
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
            // Should have received a sensor data message
            LOGERR ("Missing sensor data message from scan board");
            break;
        
        default:
	    wdTimers[WD_NETWORK] = time (0);
            if (FD_ISSET (ctrlFd, &readSet))
            {
                LOGTRACE ("getEvents: got sensor data\n");
                handleSensorData ();
            }
        
            if (FD_ISSET (trackerFd, &readSet))
            {
                LOGTRACE ("getEvents: got api data\n");
                handleApiData ();
            }

            if (FD_ISSET (dbgFd, &readSet))
	    {
		// change the level of debug output
		if (read (dbgFd, udpMsg, sizeof udpMsg) > 0)
		    logMask = strtoul (udpMsg, 0, 0);
		printf ("logMask = %02x\n", logMask);
	    }

            break;
        }

        wdTimers[WD_NETWORK] = time (0);
    }

    return 0;
}
