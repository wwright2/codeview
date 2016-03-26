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
//  04/26/2011  Joe Halpin      1       Initial
//
//  Notes: 
//
// ***************************************************************************


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
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

#define MAIN
#include "font.h"
#include "display.h"
#include "mainInit.h"
#include "demoTrackerconf.h"
#include "common.h"
#include "controller.h"
#undef MAIN


// Globals
struct DispConfig
{
    char fontName[PATH_MAX];
    char cfgFile[PATH_MAX];
} dispConfig = { 
    "../fonts/Nema_5x7.so", 
    "../cfg/display.cfg" 
};

//static unsigned char version[3] = { 0x93, 0, 0 };
static unsigned char brightness[5] = { 0xa3, 25, 0, 0, 0 };
//static unsigned char bcastPacket[5] = { 0xa3, 0x20, 0, 0, 0 };
static unsigned char data[(VCOLS * 3) + 3];


static struct signData trackerData;
static unsigned char sensorData[5];

// These two are shared with network.c
int controllerFd;
int bustrackerFd;

// 
// Values from config file "controller.cfg"
//

static const char *cfgFile = "controller.cfg";
//static const char *fontName = "../fonts/Nema_5x7.so";

// ------------------------------------------------------------------------

static void die(const char *msg)
{
    logErr (msg);
    exit (1);
}

// ------------------------------------------------------------------------

static void readCmdLine (int argc, char *const argv[])
{
    int opt;
    extern char *optarg;

    while ((opt = getopt(argc, argv, "c:f:")) != -1) 
    {
        switch (opt) 
        {
        case 'c':
            cfgFile = optarg;
            break;

        case 'f':
	    logErr("f, strncpy() optarg=%s\n",optarg);
            strncpy (dispConfig.fontName, optarg, sizeof dispConfig.fontName);
            break;

        default: /* '?' */
            fprintf(stderr, "Usage: %s [-c configFile] [-f fontName]\n",
                argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

// ------------------------------------------------------------------------

static void setupData (FONT *f)
{
    //
    // Initialize the portal list with position/length/justification etc.
    //
    // This should only be called once at startup. To change strings, just
    // change the pointer in the relevant portal structure.
    //

    PORTAL *p;

    dispClear ();

    int r = portalList[0].r;
    int c = portalList[0].c;
    int w = portalList[0].w;
    int h = portalList[0].h;
    int j = portalList[0].j;
    const char *s = testStrings[0];
    p = dispPortalAdd (
        j, r, c, w, h, PORTAL_TEXT, 
        s, f, strlen (s), 0, 0);
    if (p == 0)
        logErr (dispGetLastErrStr ());

    portalList[0].p = p;
    phead = p;
    ptail = p;
    
    for (int i = 1; i < NUM_PORTALS; ++i)
    {
        r = portalList[i].r;
        c = portalList[i].c;
        w = portalList[i].w;
        h = portalList[i].h;
        j = portalList[i].j;
        s = testStrings[i];
        p = dispPortalAdd (
            j, r, c, w, h, PORTAL_TEXT, 
            s, f, strlen (s), 0, 0);
        if (p == 0)
            logErr (dispGetLastErrStr ());
        
        portalList[i].p = p;
        ptail->next = p;
        ptail = p;
    }
}

// ------------------------------------------------------------------------

static void sendInitBroadcast ()
{
    sendBroadcast ((uint8_t*) " ", 1);
}

// ------------------------------------------------------------------------

static void sendBrightnessData ()
{
    //printf ("sending level: %d\n", brightness[1]);
    sendBroadcast (brightness, sizeof brightness);
}

// ------------------------------------------------------------------------

static void setBrightnessLevel (int b)
{
    //
    // Use a sliding window to keep from oscillating in corner cases. Set the
    // high and low water marks depending on the current brightness level from
    // the sensor data.
    //

    static int hw = 0;
    static int lw = 0;
    const int wSize = 3;        // SWAG

    //printf ("setBrightnessLevel:\n");
    //printf ("   b = %d\n", b);
    //printf ("  hw = %d\n", hw);
    //printf ("  lw = %d\n", lw);

    if ((b > hw) || (b < lw))
    {
        hw = b + wSize;
        lw = b - wSize;
        brightness[1] = brightnessLevels[sensorData[1]];
    }
}

// ------------------------------------------------------------------------

static void handleSensorData()
{
    if (receiveSensorData (sensorData, sizeof sensorData) == 0)
        logErr ("receiving sensor data");

    // set brightness level on sign to correspond with the ambient
    // brightness. 
    setBrightnessLevel (sensorData[1]);
    sendDisplayData ();
}


// ------------------------------------------------------------------------

void sendDisplay ()
{
    for(int r = 0; r < VROWS; ++r)
    {
        const uint8_t *dispData = dispGetDisplayRow (r);
        if (dispData == 0)
            logErr (dispGetLastErrStr());

        data[0] = 0xb3;
        data[1] = 0;
        data[2] = r;
        memcpy (&data[3], dispData, VCOLS * 3);

        sendBroadcast (data, sizeof data);
    }
}

// ------------------------------------------------------------------------

void sendDisplayData()
{
    dispClear ();
    PORTAL *tmp = phead;
    while (tmp != 0)
    {
        if (dispDrawPortal (tmp) == -1)
            logErr (dispGetLastErrStr ());
        tmp = tmp->next;
    }

    //dispDumpDisplay ();

    sendDisplay ();

    //
    // Need to send brightness after display data
    //

    //printf ("sending brightness data\n");
    sendBrightnessData();
}

// ------------------------------------------------------------------------

static void handleApiData ()
{
    if (receiveApiData ((uint8_t*)&trackerData, sizeof trackerData) == 0)
        return;
    
    PORTAL *p;
    p = portalList[BUS1_NUM].p;
    if (dispSetPortalText (p, trackerData.bus[0].rt) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());
    
    p = portalList[BUS1_ROUTE].p;
    if (dispSetPortalText (p, trackerData.bus[0].destname) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());

    p = portalList[BUS1_DUE].p;
    if (dispSetPortalText (p, trackerData.bus[0].minutes) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());

    p = portalList[BUS2_NUM].p;
    if (dispSetPortalText (p, trackerData.bus[1].rt) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());
    
    p = portalList[BUS2_ROUTE].p;
    if (dispSetPortalText (p, trackerData.bus[1].destname) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());

    p = portalList[BUS2_DUE].p;
    if (dispSetPortalText (p, trackerData.bus[1].minutes) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());

    p = portalList[BUS3_NUM].p;
    if (dispSetPortalText (p, trackerData.bus[2].rt) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());
    
    p = portalList[BUS3_ROUTE].p;
    if (dispSetPortalText (p, trackerData.bus[2].destname) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());

    p = portalList[BUS3_DUE].p;
    if (dispSetPortalText (p, trackerData.bus[2].minutes) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());

    p = portalList[BUS4_NUM].p;
    if (dispSetPortalText (p, trackerData.bus[3].rt) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());
    
    p = portalList[BUS4_ROUTE].p;
    if (dispSetPortalText (p, trackerData.bus[3].destname) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());

    p = portalList[BUS4_DUE].p;
    if (dispSetPortalText (p, trackerData.bus[3].minutes) == -1)
        logErr ("Error: %s\n", dispGetLastErrStr ());

    sendDisplayData ();
}

// ------------------------------------------------------------------------

static void mainLoop ()
{
    fd_set readSet;
    struct timeval tv;
    int maxFd;
    
    for (;;)
    {
        //
        // We need to receive sensor data updates, and API updates. We need to
        // send display updates as needed. For this iteration they will only be
        // needed when the API data is received.
        //

        setFdSet (&readSet, &maxFd);

        //
        // Set the timeout to five seconds. We don't need it to send anything,
        // because sending sensor data is driven by bustracker API updates. We
        // do need to check that we get sensor data every five seconds though,
        // so if the timer expires, the scanboard is late giving us data. What
        // we do about that remains to be determined.
        //

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int ret = select (maxFd + 1, &readSet, 0, 0, &tv); 
        //printf ("select returned %d\n", ret);

        switch (ret)
        {
        case -1:
            if (errno == EINTR)
                continue;
            else
                logErr ("Error: in select: %s\n", strerror (errno));
            break;

        case 0:
            // Timed out without getting sensor data. Just log it for now.
            logErr ("Error: timeout waiting for sensor data\n");
            break;

        default:
            if (FD_ISSET (controllerFd, &readSet))
            {
                //printf ("got sensor data\n");
                handleSensorData ();
            }
            if (FD_ISSET (bustrackerFd, &readSet))
            {
                //printf ("got api data\n");
                handleApiData ();
            }
            break;
        }
    }
}

// ------------------------------------------------------------------------

static void sigHandler (int sig)
{
    switch (sig)
    {
    case SIGHUP:
        syslog (LOG_EMERG, "Exiting with SIGHUP:");
        exit (1);
        break;
    case SIGINT:
        syslog (LOG_EMERG, "Exiting with SIGINT:");
        exit (1);
        break;
    case SIGCHLD:
        syslog (LOG_EMERG, "Exiting with SIGCHLD:");
        exit (1);
        break;
    case SIGQUIT:
        syslog (LOG_EMERG, "Exiting with SIGQUIT:");
        exit (1);
        break;
    case SIGPIPE:
        syslog (LOG_EMERG, "Exiting with SIGPIPE:");
        exit (1);
        break;
    case SIGALRM:
        syslog (LOG_EMERG, "Exiting with SIGALRM:");
        exit (1);
        break;
    case SIGTERM:
        syslog (LOG_EMERG, "Exiting with SIGTERM:");
        exit (1);
        break;
    }
}

// ------------------------------------------------------------------------

static void catchSignals ()
{
    struct sigaction sa;
    memset (&sa, 0, sizeof sa);
    sa.sa_handler = sigHandler;
    sigaction (SIGHUP, &sa, 0);
    sigaction (SIGINT, &sa, 0);
    sigaction (SIGCHLD, &sa, 0);
    sigaction (SIGQUIT, &sa, 0);
    sigaction (SIGPIPE, &sa, 0);
    sigaction (SIGALRM, &sa, 0);
    sigaction (SIGTERM, &sa, 0);
}

// ------------------------------------------------------------------------

void nanoPause (unsigned long n)
{
    struct timespec tSleep, tRem;

    if (n > 999999999)
    {
        logErr ("Invalid parameter in nanoPause: %d", n);
        return;
    }

    tSleep.tv_sec = 0;
    tSleep.tv_nsec = n;
    tRem.tv_sec = 0;
    tRem.tv_nsec = 0;

    for (;;)
    {
        errno = 0;
        if (nanosleep (&tSleep, &tRem) == -1)
        {
            if (errno == EINTR)
                tSleep = tRem;
            else
            {
                logErr ("Error in nanoPause: %s", strerror (errno));
                break;
            }
        }

        else
            break;
    }
}

// ------------------------------------------------------------------------

void doPOST ()
{
    initPOST ();
    while (setNextPOSTDisplay ())
    {
        sendDisplay ();
        nanoPause (2000000); // 2 ms pause between lines
    }
}

// ------------------------------------------------------------------------

void loadConfig ()
{
    const char *name;
    const char *value;

    if (! cfgOpen (dispConfig.cfgFile))
    {
        logErr ("config file not found, using defaults");
        return;
    }

    while (cfgGetNext (&name, &value))
    {
        if (strcmp (name, "fontName") == 0)
            strncpy (dispConfig.fontName, value, sizeof dispConfig.fontName);
    }
}

// ------------------------------------------------------------------------

int main(int argc, char * const argv[])
{
	char *fn;

    catchSignals ();

    dispInit ();
    readCmdLine (argc, argv);

    fn = dispConfig.fontName;

    logErr("display opening font: %s", fn);
    FONT *f = fntGetFont(fn);
    if (f == 0)
        die (fntGetLastErrStr ());
    
    doPOST ();
    setupData (f);
    setupSockets();
    sendInitBroadcast ();
    sendDisplayData ();
    mainLoop ();
}
