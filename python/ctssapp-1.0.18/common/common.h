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
//  Filename:       common.h
//
//  Description:    Interface definitions for the config file code
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  05/04/2011  Joe Halpin      1       Original
//  Notes: 
//
// This version of config allows only one configuration file at a time. Reads
// name/value pairs consecutively from the file.
// ***************************************************************************
#ifndef config_h
#define config_h

#include "log.h"

#define WATCHDOG_DIR "/tmp/run/ctss/watchdog/"

// ---------------------------------------------------------------------
// Sets a process up to be monitored by the watchdog process. This creates a
// file in watchdogDir and gets a lock on it. Don't do anything like close all
// file handles, or the system will reboot.
//
// The file is named for the program that created it, and contains the current
// pid of that process.
//
int registerWithWatchdog ();

// ---------------------------------------------------------------------
// Tell the watchdog monitor process we're still alive.
//
void updateWatchdog ();

// ---------------------------------------------------------------------
// Makes a process a daemon. This doesn't create a pidfile or do anything
// other than turning the calling process into a daemon (setsid(), setup
// std descriptors).
void becomeDaemon ();

// ---------------------------------------------------------------------
// Opens a config file and loads it into memory. The format of the file is
// name=value (no white space around '='). Value extends to the end of the
// line. Comments after the value are taken as part of the value.
//

int cfgOpen (const char *cfgFile);

// ---------------------------------------------------------------------
// Gets the next name/value pair in the file.
//

int cfgGetNext (const char **name, const char **const val);

// ---------------------------------------------------------------------
// Save the existing configuration to the same filename that was used
// to open it. A backup file is created. If the save fails the existing
// file is not changed.
//

void cfgClose ();

// ------------------------------------------------------------------------
// Logs an error to syslog
//
//void initLog (const char *pname);
//void LOGERR (const char *fmt, ...);

#endif
