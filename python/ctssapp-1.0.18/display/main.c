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
//  Filename:       main.c
//
//  Description:    main routine for the display manager
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  07/28/2011  Joe Halpin      1       Refactored from previous
//  09/28/2012  Joe Halpin      2       Changes to support more than one
//                                      display size
//  Notes: 
//
// ***************************************************************************

#define _POSIX_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include "log.h"
#include "common.h"
#include "trackerconf.h"
#include "winTypes.h"
#include "sharedData.h"
#include "ctaList.h"
#include "networkThread.h"
#include "apiThread.h"
#include "renderThread.h"
#include "scannerThread.h"
#include "font.h"
#include "sharedData.h"

PIXEL **disp;
DISPLAY_PROFILE bigDP    = { 32, 192, 3, 24 };
DISPLAY_PROFILE smallDP  = { 32, 128, 3, 24 };
DISPLAY_PROFILE *dp      = &smallDP;

const char *resourceDir  = "";
void *updateScanboard (void *arg);
size_t rsize = 0;

#define RUNDIR "/var/run/display"
#define LOCKFILE RUNDIR"/display.lck"
#define WD_TIMEOUT 120 

// ------------------------------------------------------------------------
//
int watchdog (char *msg)
{
    int bad = 0;
    time_t now = time(0);

    for (int i = 0; i < WD_MAX; ++i)
    {
        if (now - wdTimers[i] > WD_TIMEOUT)
        {
            char num[128];
            sprintf (num, "%d ", i);
            bad = 1;
            strcat (msg, num);
        }
    }

    if (bad)
        return 0;

    return 1;
}

// ------------------------------------------------------------------------
//
void setLockFile ()
{
    struct stat sb;
    int lockFd;
    struct flock lck = 
    {
        .l_type   = F_WRLCK,
        .l_whence = SEEK_SET,
	.l_start  = 0,
        .l_len    = 0,
    };

    // setup run directory
    if (stat (RUNDIR, &sb) == -1)
    {
	system ("mkdir -p "RUNDIR);
	if (stat (RUNDIR, &sb) == -1)
	    LOGERR ("Can't create %s", RUNDIR);
	else
	{
	    lockFd = open (LOCKFILE, O_WRONLY|O_CREAT, 0644);
	    if (lockFd == -1)
		LOGERR ("Can't open %s: %s\n", LOCKFILE, strerror (errno));
	    
	    else if (fcntl (lockFd, F_SETLK, &lck) == -1)
	    {
		LOGERR ("Run lock held by an existing process, exiting");
		exit (1);
	    }
	}
    }
}

// ------------------------------------------------------------------------
//
void signalHandler (int sig)
{
    runEventLoop = 0;
}

// ------------------------------------------------------------------------
//
static void setSignalHandling ()
{
    struct sigaction sa;
    sa.sa_handler = signalHandler;

    sigaction (SIGINT, &sa, 0);
    sigaction (SIGTERM, &sa, 0);
}


// ------------------------------------------------------------------------
//
static void sendInitBroadcast ()
{
    sendBroadcast ((uint8_t*) " ", 1);
}

// ------------------------------------------------------------------------

void mainLoop ()
{
    static char msg[2048]= "watchdog timer(s) expired: ";
    static const char *fname = "/home/cta/wdErr.txt";
    static const char *backup = "/home/cta/wdErr.txt.bak";
    static const int maxSize = 524288; // 512K
    struct stat sb;
    time_t start = time (0);

    for (;;)
    {
        if (! runEventLoop) 
        {
            LOGERR ("display main loop exiting");
            break;
        }

        time_t now = time(0);
        if (now > start + WD_TIMEOUT)
        {
            // Check to see if the log file is getting too big and back it up
            // if so.
            if (stat (fname, &sb) == 0)
            {
                int ret = 0;
                if (sb.st_size > maxSize)
                    ret = rename (fname, backup);
                if (ret != 0)
                {
                    LOGERR ("ERROR: resetting %s, trying to remove it",fname);
                    if (unlink (fname) != 0)
                        LOGERR ("ERROR: cannot remove %s: %s",
                            fname,strerror (errno));
                }
            }

            //
            // Check watchdog timers for each thread. If one of or
            // more doesn't update its timestamp in a minute, restart
            // the process.
            //
            
            if (! watchdog (msg))
            {
                time_t t = time(0);

                LOGERR ("main.c ERROR: %s", msg);
                FILE *fp = fopen (fname, "a+");
                if (fp != 0)
                {
                    fprintf (fp, "%s - %s", msg, ctime (&t));
                    fclose (fp);
                }
                
                LOGERR ("display exiting");
                exit (1);
            }
        
            start = time (0);
        }

        sleep (60);
    }
}

// ------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int tmpDebug = DBG_BIT | ERR_BIT | WARN_BIT;

    struct option longOptions[] = {
        { "cols", required_argument, 0, 0 },
        { "font", required_argument, 0, 0 },
        { 0,      0 }
    };

    setLockFile ();
	    
    // Default to small sign
    int cols = 128;
    uint8_t brightness = 150;

    for (;;)
    {
        int optionIndex = 0;

        int c = getopt_long (argc, argv, "b:d:", longOptions, &optionIndex);
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            if (strcmp (longOptions[optionIndex].name, "cols") == 0)
                cols = atoi (optarg);
            else if (strcmp (longOptions[optionIndex].name, "font") == 0)
                ctaConfig.fontName = optarg;
            break;
	case 'b':
	    brightness = (uint8_t)atoi (optarg);
	    break;
	case 'd':
	    tmpDebug = strtoul (optarg, 0, 0);
	    break;
        default:
            LOGFATAL ("Invalid argument");
            break;
        }
    }

    if (cols > 128)
        dp = &bigDP;

    // Override the default debug bits by what's in the environment
    const char *p = getenv ("DEBUG");
    if (p) 
	logMask = strtoul (p, 0, 0);
    else
	logMask = tmpDebug;

    initLog ("display");
    LOGDBG ("main: debug set to 0x%02x", logMask);

    frameBuffer.brightnessMsg[1] = brightness;
    initSharedData (dp);
    setSignalHandling ();
    
    LOGTRACE ("getting font");
    FONT *f = fontLoad(ctaConfig.fontName);
    if (f == 0) LOGFATAL ("Could not load font %s", ctaConfig.fontName);

    if (setupSockets() == -1)
	LOGFATAL ("Cannot setup sockets");

    sendInitBroadcast ();
    
    ctaConfig.f = f;
    ctaConfig.dp = dp;

    pthread_t networkThread;
    if (pthread_create (&networkThread,0,networkThreadLoop, 0) != 0)
    {
        LOGERR ("Failed to create networkThread: %s", strerror (errno));
        exit (1);
    }

    pthread_t apiThread;
    if (pthread_create (&apiThread, 0, apiThreadLoop, 0) != 0)
    {
        LOGERR ("Failed to create apiThread: %s", strerror (errno));
        exit (1);
    }

    pthread_t renderThread;
    if (pthread_create (&renderThread, 0, renderThreadLoop, 0) != 0)
    {
        LOGERR ("Failed to create renderThread: %s", strerror (errno));
        exit (1);
    }

    pthread_t scanThread;
    if (pthread_create (&scanThread, 0, scanThreadLoop, 0) != 0)
    {
        LOGERR ("Failed to create scanThread: %s", strerror (errno));
        exit (1);
    }

    mainLoop ();

}
