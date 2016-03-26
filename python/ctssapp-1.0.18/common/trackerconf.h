// ***************************************************************************
//   Copyright (c) 2011 Luminator Holding, LP
//   All rights reserved.
//   Any use without the prior written consent of Luminator Holding, LP
//     is strictly prohibited.
// ***************************************************************************
//
//  Filename:   trackerconf.h
//
//  Description:
//
//  Revision History:
//  Date             Name              Ver    Remarks
//  apr 15, 2011     wwright            0
//  aug 2,  2011     wwright            1     update for CTA discovery API delivered on July27
//
//  Notes:       ...
//      UDP port numbers
//      Message types. => display controller
//      Message        => Button Monitor audio text.
//
//      process a config file
//      %s %s\n
//
//
// ***************************************************************************

/*
 * trackerconf.h
 *  - config routines
 *     ... shared data struct "signData" with busapi.py creates, bustracker reads.
 *
 */

#ifndef TRACKERCONF_H
#define TRACKERCONF_H

#define LISTEN_PORT             15545
#define BROADCAST_RX            0xffffffff
#define DEFAULT_CONF_SIZE       100
#define FILESIZE                sizeof(struct signData)

typedef enum
{
	BusArrival = 1,	 			// upto 8 arrivals sorted by arrival time. >rt1 destination 1m\nrt2 destxyz 8m\n<
	BusAlert   = 2,				// alerts >11111111111111\n222222222222222\n333333333333\n< sorted by severity
	BusError   = 3,				// error messages >my error message\n<  either no arrivals scheduled, url timeout, modem problem, etc.
	BusConfig  = 4,			    // name value pairs, separated by
                                            // newlines, name delimited by a
                                            // space. 
	BusBlankTheScreen = 5,		    // not used (?)
	NoDataMsgId = 6,
	BrightnessMsgId = 7, // pseudo-message from the network thread to api thread
	LastMessageType
}CtssMessageEnum;

//#define CTSS_MAX_MESSAGE    1024
#define CTSS_MAX_MESSAGE	1024*64			// 64k bytes is the max udp message...we hope it all fits ;)

/*
 * struct genericMessage
	char	type
 */

#define AUDIO_DATA_START 3
struct alertMessage
{
	char			type;
	char			numAlerts;
	char			ndxChange;
	char * 			data;  // upto 64k CTSS_MAX_MESSAGE
};

struct arrivalMessage
{
	char			type;
	char * 			data;  // upto 64k CTSS_MAX_MESSAGE
};

struct errorMessage
{
	char			type;
	char * 			data;  // upto 64k CTSS_MAX_MESSAGE
};




extern int      debug (int level, char *fmt, ...);

extern int      getconfigNdx(char const *key);
extern int      putconfig(char const *key, char const *value);
extern char *   getconfig(char const *key);
extern void     dumpconfig();

extern int      configinit();
extern void *   audioTts(void * work);
extern void *   busApiRx(void * work);

#endif
