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
//  Filename:       error.h
//
//  Description:    Declarations
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  09/26/2011  Joe Halpin      1       Original
//
//  Notes:
//
// Yes, I copied this straight from alerts.h and changed the names. 
// ***************************************************************************

#ifndef ERRORS_H
#define ERRORS_H

#include <pthread.h>

#include "trackerconf.h"

#define MAX_ERRORS 25
#define MAX_LINES  25
#define COLS_PER_LINE 192

typedef struct 
{
    char **lineArray;           // pointer to lines in the error
    int numLines;               // how many lines in the error
    int dispIndex;              // line currently being rendered
    int maxIndex;               // last valid index in lineArray
} ERROR;

typedef struct
{
    pthread_mutex_t lock;

    int errorIndex;             // error being parsed from errors array
    int dispError;              // error being rendered

    int numErrors;              // how many errors there are (always 1 now)
    int errorSequence;          // which error in the sequence is being shown
    int newErrorsOk;            // should the API thread accept new errors yet

    //
    // The errorBuf contains the error data after being broken up into errors
    // and lines.
    //
    char errorBuf[CTSS_MAX_MESSAGE]; // the error data from busapi
    int errorBufPos;                 // index into errorBuf while parsing

    //
    // The lines array contains pointers to the beginning of each line in
    // errorBuf. These pointers are referenced in the ERROR structs to find the
    // lines belonging to each error.
    //
    char *lines[MAX_ERRORS * MAX_LINES]; // pointers to line arrays
    int linesIndex;                      // index used while parsing
    int curLine;                         // index into errors - for parsing

    // 
    //
    // This is the distillation of all the parsing done on the data. Each
    // element in the array contains the information needed to access an error,
    // and the lines to be displayed.
    //
    ERROR errors[MAX_ERRORS];
    int curError;               // index into errors - for parsing

    char tmpError[CTSS_MAX_MESSAGE];
} ERROR_DATA;


//
// Preserve state between rendering calls.
//

#ifdef THR_DEBUG
#define lockErrorData() \
{ \
    logErr ("locking errorData.lock (%p) %s %d", \
        &errorData.lock, __FILE__,__LINE__);     \
    pthread_mutex_lock (&errorData.lock); \
}


#define unlockErrorData() \
{ \
    logErr ("unlocking errorData.lock (%p) %s %d", \
        &errorData.lock, __FILE__,__LINE__);         \
    pthread_mutex_unlock (&errorData.lock); \
}

#else

#define lockErrorData() \
{ \
    pthread_mutex_lock (&errorData.lock); \
}


#define unlockErrorData() \
{ \
    pthread_mutex_unlock (&errorData.lock); \
}

#endif


extern ERROR_DATA errorData;    

#endif
