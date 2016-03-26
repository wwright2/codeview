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
//  Description:    contains network routines, messages, etc
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  5/15/11     Joe Halpin      1       Split off from main.c
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

#include "common.h"

const short scanboardPort  = 17478; //0x4446;
const short controllerPort = 17479; //0x4447;
const short bustrackerPort = 15545; 

extern int controllerFd;
extern int bustrackerFd;

struct sockaddr_in bcastAddr;
struct sockaddr_in controllerAddr;
struct sockaddr_in scanboardAddr;
struct sockaddr_in bustrackerAddr;

// ------------------------------------------------------------------------

void setupSockets()
{
    size_t size = sizeof controllerAddr;

    // --------------------------------------------------------------------
    // Setup the controller socket. This is used to receive sensor data from
    // the scanboard as well as to send display data to it.
    //
    if((controllerFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)	// listen
        logErr ("socket() failed controller");

    memset(&controllerAddr, 0, sizeof controllerAddr);
    controllerAddr.sin_family = AF_INET;
    controllerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    controllerAddr.sin_port = htons(controllerPort);

    if(bind(controllerFd, (struct sockaddr*)&controllerAddr, size) < 0)
        logErr ("binding controllerFd failed");

    int bcast = 1;
    int bcastSize = sizeof bcast;
    if(setsockopt(controllerFd, SOL_SOCKET, SO_BROADCAST, &bcast, bcastSize)<0)
        logErr ("setsockopt(SO_BROADCAST) failed");
    
    // --------------------------------------------------------------------
    // Setup a socket to catch API updates from the API monitor task. 
    //
    if ((bustrackerFd = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
        logErr ("socket() failed for bustracker");

    memset (&bustrackerAddr, 0, sizeof bustrackerAddr);
    bustrackerAddr.sin_family = AF_INET;
    bustrackerAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    bustrackerAddr.sin_port = htons (bustrackerPort);

    if(bind(bustrackerFd, (struct sockaddr*)&bustrackerAddr, size) < 0)
        logErr ("binding bustrackerFd failed");

    // --------------------------------------------------------------------
    // Setup a broadcast address structure. We use this initially to initialize
    // the protocol.
    //
    memset(&bcastAddr, 0, sizeof bcastAddr);
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_port = htons(scanboardPort);
    if(inet_pton(AF_INET, "255.255.255.255", &bcastAddr.sin_addr) != 1)
        logErr ("inet_pton() failed");
}

// ------------------------------------------------------------------------

void sendBroadcast(uint8_t *buf, size_t size)
{
    int ret = sendto (
        controllerFd, 
        buf, 
        size, 
        0, 
        (struct sockaddr*)&bcastAddr,
        sizeof bcastAddr);

    if(ret == -1)
        logErr ("broadcast send failed: %s", strerror (errno));
}

// ------------------------------------------------------------------------

uint8_t *receiveApiData (uint8_t *buf, size_t size)
{
    memset (&bustrackerAddr, 0, sizeof bustrackerAddr);
    socklen_t len = sizeof bustrackerAddr;
    //printf ("calling recvfrom()\n");
    if(recvfrom(
            bustrackerFd, 
            buf,
            size,
            0,
            (struct sockaddr*)&bustrackerAddr,
            &len) < 0)
    {
        logErr ("receiving sensor data");
        return 0;
    }

    return buf;
}

// ------------------------------------------------------------------------

uint8_t *receiveSensorData (uint8_t *buf, size_t size)
{
    memset (&scanboardAddr, 0, sizeof scanboardAddr);
    socklen_t len = sizeof scanboardAddr;
    if(recvfrom(
            controllerFd, 
            buf,
            size,
            0,
            (struct sockaddr*)&scanboardAddr,
            &len) < 0)
    {
        logErr ("Error receiving sensor data: %s", strerror (errno));
        return 0;
    }

    return buf;
}

// ------------------------------------------------------------------------
// Set the bits in the read mask for the sockets we're using. The main routine
// uses this for it's event loop.
//
void setFdSet (fd_set *readSet, int *maxFd)
{
    FD_ZERO (readSet);
    FD_SET  (controllerFd, readSet);
    FD_SET  (bustrackerFd, readSet);
    *maxFd = (controllerFd > bustrackerFd) ? controllerFd : bustrackerFd;
}


