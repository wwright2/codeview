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
//  Filename:       alert.h
//
//  Description:    Declarations
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  09/26/2011  Joe Halpin      1       Original
//
//  Notes:
//
// ***************************************************************************

#ifndef ALERTS_H
#define ALERTS_H

#include <pthread.h>

#include "trackerconf.h"

#define MAX_ALERTS 25
#define MAX_LINES  25
#define COLS_PER_LINE 192

typedef struct 
{
    char **lineArray;           // pointer to lines in the alert
    int numLines;               // how many lines in the alert
    int dispIndex;              // line currently being rendered
    int maxIndex;               // last valid index in lineArray
} ALERT;

typedef struct
{
    pthread_mutex_t lock;

    int alertIndex;             // alert being parsed from alerts array
    int dispAlert;              // alert being rendered

    int changedAlert;           // index of alert which changed
    int numAlerts;              // number of alerts.
    int alertSequence;          // which alert in the sequence is being shown
    int newAlertsOk;            // should the API thread accept new alerts yet

    //
    // The alertBuf contains the alert data after being broken up into alerts
    // and lines.
    //
    char alertBuf[CTSS_MAX_MESSAGE]; // the alert data from busapi
    int alertBufPos;                 // index into alertBuf while parsing

    //
    // The lines array contains pointers to the beginning of each line in
    // alertBuf. These pointers are referenced in the ALERT structs to find the
    // lines belonging to each alert.
    //
    char *lines[MAX_ALERTS * MAX_LINES]; // pointers to line arrays
    int linesIndex;                      // index used while parsing
    int curLine;                         // index into alerts - for parsing

    // 
    //
    // This is the distillation of all the parsing done on the data. Each
    // element in the array contains the information needed to access an alert,
    // and the lines to be displayed.
    //
    ALERT alerts[MAX_ALERTS];
    int curAlert;               // index into alerts - for parsing

    char tmpAlert[CTSS_MAX_MESSAGE];
} ALERT_DATA;


//
// Preserve state between rendering calls.
//

#ifdef THR_DEBUG
#define lockAlertData() \
{ \
    logErr ("locking alertData.lock (%p) %s %d", \
        &alertData.lock, __FILE__,__LINE__);     \
    pthread_mutex_lock (&alertData.lock); \
}


#define unlockAlertData() \
{ \
    logErr ("unlocking alertData.lock (%p) %s %d", \
        &alertData.lock, __FILE__,__LINE__);         \
    pthread_mutex_unlock (&alertData.lock); \
}

#else

#define lockAlertData() \
{ \
    pthread_mutex_lock (&alertData.lock); \
}


#define unlockAlertData() \
{ \
    pthread_mutex_unlock (&alertData.lock); \
}

#endif


extern ALERT_DATA alertData;    

#endif
