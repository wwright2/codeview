// ***************************************************************************
//  ... Copyright (c) 2008 Luminator Mark IV
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator Mark IV
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:         vmbMsgStructs.h
//
//  Description:      Define message formats
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  04/05/2012  Joe Halpin      1
//  Notes:
//
//*****************************************************************************
#ifndef INCL_vmbmsgsh
#define INCL_vmbmsgsh

#include "sysGlobals.h"
#include "logmsg.h"

// ---------------------------------------------------------------------------
// VMB message identifiers

typedef enum
{
    VMB_MSG_ID_PLACE_HOLDER_ONE = 1,        //  1 NOT USED
    VMB_END_MSG_ID,
} VMB_MSG_ID;

// ---------------------------------------------------------------------------
// Messages to vmbController on the socket normally used for SOAP. The
// sysCheck.sh script also sends messages to this socket.
//
// Don't add anything to the front or middle of any enumeration, if something
// new is needed, add as the last valid entry to the appropriate enum.
//
typedef enum
{
    SOAP_MSG_ID_transaction,
    SOAP_MSG_ID_postText,
    SOAP_MSG_ID_playAudio,
    SOAP_MSG_ID_clearDisplay,
    SOAP_MSG_ID_setGraphic,
    ADMIN_MSG_ID_lowPowerOn,
    ADMIN_MSG_ID_lowPowerOff,
    SOAP_MSG_ID_END,
} __attribute__ ((packed)) SOAP_MSG_IDS;

// ---------------------------------------------------------------------------
// Messages exchanged between vmbController and internal processes
//
#define MSG_DELIM " "

typedef enum
{
    VMB_MSG_ID_createWindow = SOAP_MSG_ID_END,
    VMB_MSG_ID_setWinContent,
    VMB_MSG_ID_setText,
    VMB_MSG_ID_setGraphic,
    VMB_MSG_ID_displayContent,
    VMB_MSG_ID_getStatus,
    VMB_MSG_ID_status,
    VMB_MSG_ID_playSynchronized,
    VMB_MSG_ID_audioDone,
    VMB_MSG_ID_statusRsp,
    VMB_MSG_ID_setSensorOutputs,
    VMB_MSG_ID_sbGetStatus,		// diag -> sensor board
    VMB_MSG_ID_sbSensorRsp,		// diag <- sensor board
    VMB_MSG_ID_sbSetOutputs,		// diag -> sensor board
    VMB_MSG_ID_clearDisplay,
    VMB_MSG_ID_lowPower,
} __attribute__ ((packed)) VMB_MSG_IDS;

// ---------------------------------------------------------------------------
// VMB messages to/from SOAP server
//
// These messages all use one pretty much undefined structure, because the
// content size isn't really predictable. The message id is used to branch to a
// routine that knows what the content of the message should be, and how to
// unpack it. 
//
// See the descriptions of message content below.
//

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Msg ID 1 - Display text. Fields are delimited by spaces
//
// msgid pri ttl scollType winEffect fgColor font x y height width
//

// ---------------------------------------------------------------------------
// Messages between vmbController and pretty much anyone else
//

//
// There is one status query message regardless of which process is being
// queried. The dest field gives either the specific destination (from the
// *_PROC_ID macros in sysGlobals.h), or 0 for a broadcast. 
// 
typedef struct
{
    uint8_t  msgId;
    uint8_t  dest;
} __attribute__ ((packed)) VMB_MSG_getStatus;

//
// All status responses use the same message. They are differentiated by the
// sender field. The value of the sender field indicates the format of the data
// portion of the message.
//
typedef struct 
{
    uint8_t  msgId;
    uint8_t  success;
    uint8_t  sender;
    uint8_t  data[1];
} __attribute__ ((packed)) VMB_MSG_statusRsp;

// ---------------------------------------------------------------------------
// Message from vmbController to render process to display text
// This will only work if sender and receiver are on the same machine, and have
// been built with the same compiler/version. Padding might throw things
// off otherwise, although as it stands now the short int looks to be aligned
// ok. 
//
// The text field should be expanded when allocating the struct, so that it
// will hold the whole text string to be displayed.
//

typedef struct {
    uint8_t  msgId;
    uint16_t pri;
    uint16_t ttl;
    uint8_t  justification;
    uint32_t  effect;
    uint32_t  scroll;
    uint32_t  x;
    uint32_t  y;
    uint32_t  width;
    uint32_t  height;
} __attribute__ ((packed)) VMB_MSG_winHeader;


typedef struct {
    VMB_MSG_winHeader header;
    uint8_t  fg[3];
    uint8_t  font;
    char     text[1];
} __attribute__ ((packed)) VMB_MSG_setText;

typedef struct {
    uint8_t msgId;
} __attribute__ ((packed)) VMB_MSG_clearDisplay;

typedef struct {
    VMB_MSG_winHeader header;
    uint32_t offsetX;   /* X offset within graphic to start drawing */
    uint32_t offsetY;   /* Y offset within graphic to start drawing */
    char     fname[1];
} __attribute__ ((packed)) VMB_MSG_setGraphic;

typedef struct {
    uint8_t msgId;
    uint8_t onoff;
} __attribute__ ((packed)) VMB_MSG_lowPower;

// ---------------------------------------------------------------------------
// Messages between vmbController and the diag process
//

// These are control bits for the strobe and general purpose
// output. They should be bitwise or'd together.
typedef enum
{
    SENSOR_strobeOn  = 0x01,
    SENSOR_strobeOff = 0x00,
    SENSOR_GPon      = 0x02,
    SENSOR_GPoff     = 0x00,
} STROBE_BITS;

typedef struct
{
    uint8_t msgId;
    uint8_t ctrlByte;
} __attribute__ ((packed)) VMB_MSG_setSensorOutputs;

// ---------------------------------------------------------------------------
// Messages between diag process and the sensor board.
// 

typedef struct
{
    uint8_t  start;
    uint16_t length;
    uint16_t addr;
    uint8_t  sysId;
    uint8_t  msgId;
    uint8_t  checksum;
} __attribute__ ((packed)) VMB_MSG_sbGetStatus;

#define SB_GET_STATUS_INIT { 0x7e, 0x06, 0x00, 0x05, 0x15, 0xe0 }

typedef struct
{
    uint8_t  start;
    uint16_t length;
    uint16_t addr;
    uint8_t  sysId;
    uint8_t  msgId;
    uint16_t brightness1;
    uint16_t brightness2;
    uint8_t  temp;
    uint8_t  genInput;
    uint8_t  checksum;
} __attribute__ ((packed)) VMB_MSG_sbSensorRsp;

typedef struct 
{
    uint8_t  start;
    uint16_t length;
    uint16_t addr;
    uint8_t  sysId;
    uint8_t  msgId;
    uint8_t  ctrlByte;
    uint8_t  checksum;
} __attribute__ ((packed)) VMB_MSG_sbSetOutputs;

#define STROBE_BIT          0x01
#define GP_BIT              0x10
#define SB_SET_OUTPUTS_INIT { 0x7e, 0x00, 0x06, 0x23, 0x00, 0x00 }

#endif

