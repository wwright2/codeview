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
//  Filename:       network.h
//
//  Description:    declarations for networking functions
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  05/15/11    Joe Halpin      1       Split off from main.c
//  07/28/11    Joe Halpin      1       refactoring
//
//  Notes:
//
// ***************************************************************************

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

// Number of descriptors, one for API, one for scanboard, the values of the *FD
// variables are not the file descriptors, they are bits in a bitmap.

static const int scanboardFD = 1;
static const int apiFD = 2;

typedef enum
{
    ENET_OK,
    ENET_MEM,
    ENET_INVALID,
    ENET_TIMEOUT
} NET_ERR;

typedef void (*inputCallback)(uint32_t);

// --------------------------------------------------------------------------
// Open sockets.
//
int setupSockets ();


// --------------------------------------------------------------------------
// Sends the buffer passed in as a broadcast message, using the all-ones
// broadcast address.
//
void sendBroadcast (uint8_t *buf, size_t size);


// --------------------------------------------------------------------------
// Get a UDP message from the API monitor and returns it (the message is copie
// into the buffer). The size of the message is returned in the second
// parameter (value - result)
//
uint8_t *receiveApiData (uint8_t *buf, ssize_t *size);


// --------------------------------------------------------------------------
// Gets a UDP message from the scan board. All we get from them are sensor
// readings. Returns 0 for success and -1 for error, with error data in the
// netError() and netErrorCode() functios.
//
uint8_t *receiveSensorData (size_t *size);


// --------------------------------------------------------------------------
// Calls select(), and passes back any active file descriptors. Returns 0 for
// timeout, or the number of file descriptors active as a bitmap;
//
void getEvents (inputCallback f, struct timeval *timeout);


// --------------------------------------------------------------------------
// Returns a string describing the last error. The return value points into a
// static buffer, don't try to delete it. Copy it if you need to keep it past
// one call to this function.
//
const char *netError ();


// --------------------------------------------------------------------------
// Returns a code from NET_ERR to indicate the last error
//
NET_ERR netErrorCode ();

// --------------------------------------------------------------------------
// Once getEvents() is called it just keeps going until this is called. If you
// want to exit gracefully call this to stop the select loop. It could take up
// to 5 seconds for that to happen.
//
void exitEventLoop ();

#endif
