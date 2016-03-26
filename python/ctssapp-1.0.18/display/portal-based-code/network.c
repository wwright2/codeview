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
//  Filename:       network.c
//
//  Description:    networking functions
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  07/28/11    Joe Halpin      1       refactoring
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

#include "debug.h"
#include "common.h"
#include "network.h"

static const short scanboardPort  = 17478; //0x4446;
static const short controllerPort = 17479; //0x4447;
static const short bustrackerPort = 15545; 
static const int MAX_FDs = 2;

static int ctrlFd;
static int trackerFd;

static struct sockaddr_in bcastAddr;
static struct sockaddr_in controllerAddr;
static struct sockaddr_in scanboardAddr;
static struct sockaddr_in bustrackerAddr;

static int runEventLoop = 0;

// Number of descriptors, one for API, one for scanboard, the values of the *FD
// variables are not the file descriptors, they are bits in a bitmap.

//static const char *errStr;

#if 0
// ------------------------------------------------------------------------

static void logErr (const char *fmt, ...)
{
    int priority = LOG_CONS | LOG_PID;
    va_list ap;

    va_start (ap, fmt);
    vsyslog (priority, fmt, ap);
    va_end (ap);
}
#endif

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
        logErr ("socket() failed controller: %s", strerror (errno));
        return -1;
    }

    memset(&controllerAddr, 0, sizeof controllerAddr);
    controllerAddr.sin_family = AF_INET;
    controllerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    controllerAddr.sin_port = htons(controllerPort);

    if(bind(ctrlFd, (struct sockaddr*)&controllerAddr, size) < 0)
    {
        logErr ("binding ctrlFd failed: %s", strerror (errno));
        return -1;
    }

    int bcast = 1;
    int bcastSize = sizeof bcast;
    if(setsockopt(ctrlFd, SOL_SOCKET, SO_BROADCAST, &bcast, bcastSize)<0)
    {
        logErr ("setsockopt(SO_BROADCAST) failed: %s", strerror (errno));
        return -1;
    }
    
    // --------------------------------------------------------------------
    // Setup a socket to catch API updates from the API monitor task. 
    //
    if ((trackerFd = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        logErr ("socket() failed for bustracker: %s", strerror (errno));
        return -1;
    }

    memset (&bustrackerAddr, 0, sizeof bustrackerAddr);
    bustrackerAddr.sin_family = AF_INET;
    bustrackerAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    bustrackerAddr.sin_port = htons (bustrackerPort);

    if(bind(trackerFd, (struct sockaddr*)&bustrackerAddr, size) < 0)
    {
        logErr ("binding trackerFd failed: %s", strerror (errno));
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
        logErr ("inet_pton() failed: %s", strerror (errno));
        return -1;
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
        logErr ("broadcast send failed: %s", strerror (errno));
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
        logErr ("receiving sensor data");
        return 0;
    }

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
        logErr ("Error receiving sensor data: %s", strerror (errno));
        return 0;
    }

    *size = sizeof buf;
    return buf;
}


// --------------------------------------------------------------------------
// Calls select(), and passes back any active file descriptors to the callback
// routine. Logs timeouts and errors.
//
void getEvents (inputCallback f, struct timeval *timeout)
{
    fd_set readSet;
    uint32_t bits;
    int maxFd = (ctrlFd > trackerFd) ? ctrlFd + 1 : trackerFd + 1;

    runEventLoop = 1;

    for (;;)
    {
        if (runEventLoop == 0)
            break;

        struct timeval tv = { 5, 0 };

        FD_ZERO (&readSet);
        FD_SET (ctrlFd, &readSet);
        FD_SET (trackerFd, &readSet);
        
        if (timeout != 0)
        {
            tv.tv_sec = timeout->tv_sec;
            tv.tv_usec = timeout->tv_usec;
        }

        //logErr ("getEvents: ctrlFd = %d, trackerFd = %d", ctrlFd, trackerFd);
        //logErr ("getEvents: scanboardFD = %d, apiFD = %d",scanboardFD,apiFD);

        errno = 0;
        int ret = select (maxFd, &readSet, 0, 0, &tv);
        switch(ret)
        {
        case -1:
            if (errno == EINTR)
                continue;
            else
                logErr ("Error: in select: %s\n", strerror (errno));
            break;

        case 0:
            logErr ("Error: timeout waiting for sensor data\n");
            break;

        default:
            bits = 0;
            if (FD_ISSET (ctrlFd, &readSet))
            {
                DBG ("getEvents: got sensor data\n");
                bits |= scanboardFD;
            }

            if (FD_ISSET (trackerFd, &readSet))
            {
                DBG ("getEvents: got api data\n");
                bits |= apiFD;
            }

            f (bits);           // invoke callback to let the main routine
                                // handle this
            break;
        }
    }
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
// Returns a code from NET_ERR to indicate the last error
//
NET_ERR netErrorCode ()
{
    return -1;
}

void exitEventLoop ()
{
    runEventLoop = 0;
}
