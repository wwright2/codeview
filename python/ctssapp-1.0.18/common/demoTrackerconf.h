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
//
//  Notes:       ...
//      process a config file
//      %s %s\n
//
// ***************************************************************************

/*
 * trackerconf.h
 *  - config routines
 *  - shared data struct "signData" with busapi.py creates, bustracker reads.
 *
 *  - size of data structure.
 *    3*(5+50+7) +50+6+8+5 = 255
 */

#ifndef TRACKERCONF_H
#define TRACKERCONF_H

#define LISTEN_PORT             15545
#define BROADCAST_RX            0xffffffff
#define DEFAULT_CONF_SIZE       100
#define FILESIZE                sizeof(struct signData)

typedef enum
{
	BusPrediction = 1,			// upto 3 buses
	BusPlusSB,					// This is BusPred. Service will follow

	LastMessageType
}CtssMessageEnum;

#define CTSS_MAX_MESSAGE    1024


struct routeDestination
{
    char        		rt[5];              /* 4+1 */
    char        		destname[50];
    char        		minutes[7];
};

struct signData
{
    char                stop[50];
    char                date[6];
    char                time[8];
    char                temp[5];

    struct routeDestination     bus[4];
};

#define BULLETIN_MAXSIZE	512
struct signDataSB
{
    char                stop[50];
    char                date[6];
    char                time[8];
    char                temp[5];

    struct routeDestination     bus[2];
    char				bulletin[BULLETIN_MAXSIZE];
};



typedef struct
{
	CtssMessageEnum		msgid;
	struct	signData	data;

}BUS_PREDICTION;

typedef struct
{
	CtssMessageEnum		msgid;
	struct signDataSB 	data;		//  If starts with \s means scroll message right to left.

}BUS_PRED_SERVICE_BULLETIN;






extern struct signData     sdata;


extern int      debug (int level, char *fmt, ...);

extern int      getconfigNdx(char const *key);
extern int      putconfig(char const *key, char const *value);
extern char *   getconfig(char const *key);
extern void     dumpconfig();

extern int      configinit();
extern void *   audioTts(void * work);
extern void *   busApiRx(void * work);

#endif
