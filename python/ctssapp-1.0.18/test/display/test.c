#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/stat.h>
#include <pthread.h>

#include <syslog.h>
#include <stdarg.h>

#include "font.h"
#include "window.h"
#include "winRender.h"
#include "winTypes.h"
#include "log.h"

static const short scanboardPort  = 17478; //0x4446;
static const short controllerPort = 17479; //0x4447;
static const short bustrackerPort = 15545; 
static const int MAX_FDs = 2;
uint8_t brightnessMsg[] = { 0xa3, 32, 0, 0, 0 };

static int ctrlFd;
static int trackerFd;

static struct sockaddr_in bcastAddr;
static struct sockaddr_in controllerAddr;
//static struct sockaddr_in scanboardAddr;
static struct sockaddr_in bustrackerAddr;


// --------------------------------------------------------------------------
// Open sockets. Returns 0 for success and -1 for error
//

int setupSockets ()
{
    size_t size = sizeof controllerAddr;

    // --------------------------------------------------------------------
    // Setup the controller socket. This is used to receive sensor data from
    // the scanboard as well as to send display data to it.
    //
    if((ctrlFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)	// listen
    {
        printf ("socket() failed controller: %s", strerror (errno));
        return -1;
    }

    memset(&controllerAddr, 0, sizeof controllerAddr);
    controllerAddr.sin_family = AF_INET;
    controllerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    controllerAddr.sin_port = htons(controllerPort);

    if(bind(ctrlFd, (struct sockaddr*)&controllerAddr, size) < 0)
    {
        printf ("binding ctrlFd failed: %s", strerror (errno));
        return -1;
    }

    int bcast = 1;
    int bcastSize = sizeof bcast;
    if(setsockopt(ctrlFd, SOL_SOCKET, SO_BROADCAST, &bcast, bcastSize)<0)
    {
        printf ("setsockopt(SO_BROADCAST) failed: %s", strerror (errno));
        return -1;
    }
    
    // --------------------------------------------------------------------
    // Setup a socket to catch API updates from the API monitor task. 
    //
    if ((trackerFd = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf ("socket() failed for bustracker: %s", strerror (errno));
        return -1;
    }

    memset (&bustrackerAddr, 0, sizeof bustrackerAddr);
    bustrackerAddr.sin_family = AF_INET;
    bustrackerAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    bustrackerAddr.sin_port = htons (bustrackerPort);

    if(bind(trackerFd, (struct sockaddr*)&bustrackerAddr, size) < 0)
    {
        printf ("binding trackerFd failed: %s", strerror (errno));
        return -1;
    }

    // --------------------------------------------------------------------
    // Setup a broadcast address structure. We use this initially to initialize
    // the protocol.
    //
    memset(&bcastAddr, 0, sizeof bcastAddr);
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_port = htons(scanboardPort);
    if(inet_pton(AF_INET, "255.255.255.255", &bcastAddr.sin_addr) != 1)
    {
        printf ("inet_pton() failed: %s", strerror (errno));
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int i;
    uint8_t brightnessMsg[] = { 0xa3, 32, 0, 0, 0 };
    char buf[192*3] = {0};
    for (i = 3; i < sizeof buf; i+= 3)
        buf[i] = 255;

    buf[0] = 0xb3;
    buf[1] = 0;
    buf[2] = 5;

    if (argc == 2)
        brightnessMsg[1] = atoi (argv[1]);

    if (setupSockets () == -1)
    {
        printf ("setupSockets failed\n");
        return 1;
    }
    for (;;)
    {
        int r;
        for (r = 0; r < 16; ++r)
        {
            buf[2] = r;
            int ret = sendto (
                ctrlFd, 
                buf, 
                sizeof buf, 
                0, 
                (struct sockaddr*)&bcastAddr,
                sizeof bcastAddr);
            
            if(ret == -1)
            {
                printf ("send data: ret == -1: %s\n", strerror (errno));
                break;
            }
            
            struct timeval tv = { 0, 4000 };
            select (0, 0, 0, 0, &tv);
        }

        int ret = sendto (
            ctrlFd, 
            brightnessMsg, 
            sizeof brightnessMsg, 
            0, 
            (struct sockaddr*)&bcastAddr,
            sizeof bcastAddr);
        
        if(ret == -1)
        {
            printf ("send brightness: ret == -1: %s\n", strerror (errno));
            break;
        }
        
        sleep (3);
    }
}
