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
//  Filename:       registerWithWatchdog.c
//
//  Description:    function to manage watchdog files
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  08/09/2011  Joe Halpin      1       creation
//
//  Notes: 
//
// ***************************************************************************


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "common.h"

static const char *watchdogDir = WATCHDOG_DIR;

static int fd;
int registerWithWatchdog (const char *pname)
{
    static struct flock fl;
    struct stat sb;
    pid_t pid = getpid ();
    char fname[1024];
    char pidStr[128];

    LOGERR ("in registerWithWatchdog ()");

    //
    // Make sure the directory exists;
    //

    LOGERR ("WATCHDOG_DIR = %s", WATCHDOG_DIR);
    LOGERR ("Creating %s", WATCHDOG_DIR);
    int ret = system ("mkdir -p "WATCHDOG_DIR);
    if (ret != 0)
    {
        LOGERR ("system() failed: %s", strerror (errno));
        return -1;
    }

    if (stat (WATCHDOG_DIR, &sb) != 0)
    {
        LOGERR ("Could not stat %s: %s", WATCHDOG_DIR, strerror (errno));
        return -1;
    }

    if (strlen (pname) >= sizeof fname)
    {
        LOGERR ("Process name string too long: %s", pname);
        return -1;
    }

    snprintf (fname, sizeof fname, "%s/%s", WATCHDOG_DIR, pname);
    fd = open (fname, O_WRONLY | O_CREAT);
    if (fd == -1)
    {
        LOGERR ("Could not open %s for writing: %s", fname, strerror (errno));
        return -1;
    }

    snprintf (pidStr, sizeof pidStr, "%d", pid);
    if (write (fd, pidStr, strlen (pidStr)) < 0)
    {
        LOGERR ("Could not write pid to %s: %s", fname, strerror (errno));
        close (fd);
        return -1;
    }

    //
    // Do not close fd.
    //

    return 0;
}


void updateWatchdog ()
{
    unsigned char b = 0;
    write (fd, &b, 1);
}
