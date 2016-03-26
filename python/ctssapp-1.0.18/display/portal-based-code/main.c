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
//  Filename:       main.c
//
//  Description:    main routine for the display manager
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  07/28/2011  Joe Halpin      1       Refactored from previous
//
//  Notes: 
//
// ***************************************************************************

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

#include "debug.h"
#include "common.h"
#include "ctaConfig.h"
#include "portal.h"
#include "display.h"
#include "network.h"
#include "font.h"
#include "apiMsg.h"
#include "arrivals.h"
#include "render.h"
#include "trackerconf.h"


// Turns on all debug messages, not just real errors, if DEBUG is set in the
// program environment.
int allDebug = 0;

// tells the program to keep going or stop
static volatile sig_atomic_t exitMainLoop = 0;

// Not using a config file for now, so this is the config
struct DispConfig
{
    char fontName[1024];
    char cfgFile[1024];
} dispConfig = {
    "../fonts/Nema_5x7.so",
    "../cfg/display.cfg"
};

int debugCurses = 0;
int alertReady  = 0;
pthread_mutex_t alertReadyLock = PTHREAD_MUTEX_INITIALIZER;

time_t wdTimers[3];

int watchdog (char *msg)
{
    int bad = 0;
    time_t now = time(0);

    for (int i = 0; i < WD_MAX; ++i)
    {
        if (now - wdTimers[i] > 60)
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

static void die(const char *msg)
{
    logErr (msg);
    exit (1);
}

// ------------------------------------------------------------------------
// This runs in a separate thread, and does nothing but send updates to the
// scanboard. Changes to the displayed data are made by the rest of the
// program. 
//
void *updateScanboard (void *arg)
{
    uint8_t data[(VCOLS * 3) + 3];

    for (;;)
    {
        wdTimers[WD_SCANBOARD] = time (0);

        if (exitMainLoop)
            break;

        dispLockCurDisplay_q ();

        for(int r = 0; r < VROWS; ++r)
        {
            const uint8_t *dispData = dispGetDisplayRow (r);
            if (dispData == 0)
            {
                logErr ("Err = %d: %s: %s %d",
                    dispGetLastErr(), dispGetLastErrStr(),__FILE__,__LINE__);
                continue;
            }

            data[0] = 0xb3;
            data[1] = 0;
            data[2] = r;
            memcpy (&data[3], dispData, VCOLS * 3);

            sendBroadcast (data, sizeof data);
            struct timeval tv = { 0, 4000 };
            select (0, 0, 0, 0, &tv);
        }

        dispUnlockCurDisplay_q ();

        //
        // Send brightness settings with every display update
        //

        size_t msgSize;
        uint8_t *brightnessMsg = getBrightnessMsg (&msgSize);
        sendBroadcast (brightnessMsg, msgSize);

        // Wait a bit before doing it again.
        struct timeval tv = { 0, 30000 };
        select (0, 0, 0, 0, &tv);
    }

    return 0;
}

// ------------------------------------------------------------------------

static void sendInitBroadcast ()
{
    //
    // Not sure why we do this, it doesn't seem to make any difference if we do
    // or not.
    //
    sendBroadcast ((uint8_t*) " ", 1);
}

// ------------------------------------------------------------------------

static void setBrightnessLevel (int b)
{
    //
    // Use a sliding window to keep from oscillating in corner cases. Set the
    // high and low water marks depending on the current brightness level from
    // the sensor data.
    //

    static int hw = -9999;
    static int lw = -9999;
    int wSize = ctaConfig.brightnessWin;

    if ((b > hw) || (b < lw))
    {
        hw = b + wSize;
        lw = b - wSize;
        dispSetBrightness (b);
    }
}

// ------------------------------------------------------------------------

static void handleSensorData()
{
    size_t  dataLen;
    uint8_t *sensorData = receiveSensorData (&dataLen);
    if (sensorData == 0)
    {
        logErr ("Error receiving sensor data: %s %d",__FILE__,__LINE__);
        return;
    }

    DBG ("handleSensorData: got brightness = %d", sensorData[1]);

    // set brightness level on sign to correspond with the ambient
    // brightness. 
    setBrightnessLevel (sensorData[1]);

    //
    // Get the current temperature readings and compare them with the last. If
    // they changed, update the file in /home/cta so the status.xml file will
    // reflect reality.
    //

    static uint8_t firstTime = 1;
    static uint8_t lastIntTemp = 0;
    static uint8_t lastOutTemp = 0;
    const char *intTempFile = "/home/cta/intTemperature.tmp";
    const char *realIntFile = "/home/cta/intTemperature.txt";
    const char *outTempFile = "/home/cta/outTemperature.tmp";
    const char *realOutFile = "/home/cta/outTemperature.txt";
    uint8_t intTempC = sensorData[3];
    uint8_t outTempC = sensorData[2];

    DBG ("--- intTempC = %d", intTempC);
    DBG ("--- outTempC = %d", outTempC);

    if ((intTempC > lastIntTemp + 2) || 
        (intTempC < lastIntTemp - 2) || 
        (firstTime == 1))
    {
        lastIntTemp = intTempC;

        FILE *fp = fopen (intTempFile, "w");
        if (! fp)
            logErr ("Could not open %s: %s", intTempFile, strerror (errno));
        else
        {
            fprintf (fp, "%dC", intTempC);
            fclose (fp);
            rename (intTempFile, realIntFile);
        }
    } 

    if ((outTempC > lastOutTemp + 2) || 
        (outTempC < lastOutTemp - 2) ||
        (firstTime == 1))
    {
        lastOutTemp = outTempC;

        FILE *fp = fopen (outTempFile, "w");
        if (! fp)
            logErr ("Could not open %s: %s", outTempFile, strerror (errno));
        else
        {
            fprintf (fp, "%dC", outTempC);
            fclose (fp);
            rename (outTempFile, realOutFile);
        }
    } 
    
    firstTime = 0;
}


// ------------------------------------------------------------------------

static void handleApiData (uint32_t activeFiles)
{
    static uint8_t trackerData[65536];
    ssize_t len = sizeof trackerData;

    if (receiveApiData ((uint8_t *)trackerData, &len) == 0)
        return;

    //
    // trackerData is a set of arrival strings, delimited by newlines. Pass a
    // pointer to the begining to the parser, along with the next three portals
    // to be setup. The parser will set up the three portals with the arrival
    // string, and return a pointer to the next arrival (one past the
    // delimiting newline) or null.
    //
    // Don't worry about what's being displayed here.
    //

    trackerData[len] = 0;
    switch (trackerData[0])
    {
    case BusArrival:
        DBG ("handleAPIData: got BusArrival");
        // Remove the message type when sending to the processing function.
        setArrivalData (&trackerData[1], len - 1);
        break;

    case BusAlert:
        DBG ("handleAPIData: got BusAlert");
        // Do not remove message metadata for this one.
        alertSetData (trackerData, len);
        break;

    case BusError:
        DBG ("handleAPIData: got BusError");
        setErrorData (&trackerData[1], len - 1);
        break;

    case BusConfig:
        DBG ("handleAPIData: got BusConfig");
        setConfig ((const char *)&trackerData[1], len - 1);
        break;

    default:
        logErr ("Got unknown message type: %d", trackerData[0]);
        break;
    }
}

// ------------------------------------------------------------------------

void handleInput (uint32_t bits)
{
    wdTimers[WD_API] = time(0);

    if (bits & scanboardFD)
        handleSensorData ();
   
    if (bits & apiFD)
        handleApiData (bits);
}

// ------------------------------------------------------------------------

void doPOST ()
{
}

// ------------------------------------------------------------------------

void readCmdLine (int argc, char *argv[])
{
    int opt;
    while ((opt = getopt (argc, argv, "db:w:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            debugCurses = 1;
            break;
        case 'b':
            ctaConfig.brightnessFloor = atoi(optarg);
            DBG ("main.c set brightnessFloor to %d", 
                ctaConfig.brightnessFloor);
            break;
        case 'w':
            ctaConfig.brightnessWin = atoi(optarg);
            DBG ("main.c set brightnessWin to %d", 
                ctaConfig.brightnessWin);
            break;
        default:
            DBG ("Usage: display [-d]");
            printf ("Usage: display [-d]\n");
            exit (1);
        }
    }
}

// ------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    pthread_t displayThread;
    pthread_t renderingThread;
    pthread_t apiThread;

    if (getenv ("DEBUG"))
        allDebug = 1;

    readCmdLine (argc, argv);
    initLog ("display");
    dispInit ();

    DBG ("getting font");
    FONT *f = fntGetFont(dispConfig.fontName);
    if (f == 0)
        die (fntGetLastErrStr ());

    DBG ("calling doPOST()");
    doPOST ();
    DBG ("calling setupSockets");
    setupSockets();
    DBG ("sending initBroadcast");
    sendInitBroadcast ();

    DBG ("checking apiInit");
    if (arrivalsInit () < 0)
    {
        logErr ("Could not initialze API handling code");
        exit (1);
    }

    DBG ("starting displayThread");
    if (pthread_create (&displayThread, 0, updateScanboard, 0) < 0)
    {
        logErr ("Could not start display thread: %s\n", strerror (errno));
        exit (1);
    }

    DBG ("starting rendering thread");
    if (pthread_create (&renderingThread, 0, renderThreadLoop, 0) < 0)
    {
        logErr ("Could not start rendering thread: %s\n", strerror (errno));
        exit (1);
    }

    DBG ("starting api thread");
    if (pthread_create (&apiThread, 0, apiThreadLoop, 0) < 0)
    {
        logErr ("Could not start api thread: %s\n", strerror (errno));
        exit (1);
    }

    //
    // Start the loop to check watchdog timers for each thread. If one of or
    // more doesn't update its timestamp in a minute, restart the process.
    //

    for (int i = 0; i < WD_MAX; ++i)
        wdTimers[i] = time(0);

    for (;;)
    {
        static char msg[2048]= "watchdog timer(s) expired: ";
        static const char *fname = "/home/cta/wdErr.txt";
        static const char *backup = "/home/cta/wdErr.txt.bak";
        static const int maxSize = 524288; // 512K
        struct stat sb;


        // Check to see if the log file is getting too big and back it up
        // if so.
        if (stat (fname, &sb) == 0)
        {
            int ret = 0;
            if (sb.st_size > maxSize)
                ret = rename (fname, backup);
            if (ret != 0)
            {
                logErr ("ERROR: resetting %s, trying to remove it",fname);
                if (unlink (fname) != 0)
                    logErr ("ERROR: cannot remove %s: %s",
                        fname,strerror (errno));
            }
        }

        if (! watchdog (msg))
        {
            time_t t = time(0);

            logErr ("main.c ERROR: %s", msg);
            FILE *fp = fopen (fname, "a+");
            if (fp != 0)
            {
                fprintf (fp, "%s - %s", msg, ctime (&t));
                fclose (fp);
            }

            logErr ("display exiting");
            exit (1);
        }

        sleep (60);
    }
}
