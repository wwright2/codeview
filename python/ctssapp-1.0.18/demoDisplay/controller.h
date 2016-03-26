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
//  Filename:       controller.h
//
//  Description:    declaration of shared stuff
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  05/15/11    Joe Halpin      1       Initial
//
//  Notes: 
//
// ***************************************************************************

#ifndef controller_h
#define controller_h

#include <sys/select.h>

#include "display.h"

void dumpPortal (PORTAL *p);
void sendDisplayData();
void sendBroadcast (uint8_t *buf, size_t size);
void setFdSet (fd_set *readSet, int *maxFd);
void setupSockets();
uint8_t *receiveSensorData (uint8_t *buf, size_t size);
uint8_t *receiveApiData (uint8_t *buf, size_t size);
void initPOST ();
int setNextPOSTDisplay ();

#endif
