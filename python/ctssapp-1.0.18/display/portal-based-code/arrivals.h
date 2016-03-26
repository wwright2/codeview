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
//  Filename:       arrivalMsg.h
//
//  Description:    declarations for handling ARRIVAL messages from bustracker
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  08/04/11    Joe Halpin      1       original
//
//  Notes:
//
// ***************************************************************************

#ifndef ARRIVALMSG_H
#define ARRIVALMSG_H

#include <sys/types.h>
#include <stdint.h>
#include <regex.h>
#include <pthread.h>

// ---------------------------------------------------------------------------
// Compile the regular expression here so we don't have to do it every time.
//
int arrivalsInit ();

// ---------------------------------------------------------------------------
// Parse the arrival data and add it to the arrival portals. There are a max of
// 4 pages per set, and a max of two pages of alerts. The alert page(s) get
// copied to the remaining portals so that we either have one page repeated
// four times, or two pages repeated once in the set.
//
void arrivalSetData (const uint8_t *buf, size_t len);

#endif
