#ifndef __ENUMS_H__
#define __ENUMS_H__

#define ALIGN_NONE   0x00
#define ALIGN_MASK   0x0F
#define ALIGN_HMASK  0x03
#define ALIGN_LEFT   0x00
#define ALIGN_RIGHT  0x01
#define ALIGN_CENTER 0x02

#define ALIGN_VMASK  0x0C
#define ALIGN_TOP    0x00
#define ALIGN_BOTTOM 0x04
#define ALIGN_MIDDLE 0x08

#define FX_NONE         0x00
#define FX_SCROLL_COUNT_MASK 0x00FF
#define FX_SCROLL_MASK  0xFF00
#define FX_SCROLL_HMASK 0x0300
#define FX_SCROLL_VMASK 0x0C00
#define FX_SCROLL_LEFT  0x0100
#define FX_SCROLL_RIGHT 0x0200
#define FX_SCROLL_UP    0x0400
#define FX_SCROLL_DOWN  0x0800

#define FX_SCROLL_VIEWPORT 0x1000
#define FX_SCROLL_COUNTER 0x2000


#define FX_FLASH        0x01
#define FX_CLEAR        0x02
#define FX_MARKUP       0x04

#define WIN_OK 0
//#define WIN_DESTROY -10
#define WIN_SCROLL_DONE -11

typedef enum
{
    WIN_TTL_INVAL,
    WIN_TTL_OK,
    WIN_TTL_NOT_YET,
    WIN_TTL_EXCEEDED,
} WIN_TTL_STATE;

typedef enum
{
    TEXT,
    GRAPHIC,
    MARKUPTEXT
} WIN_DATA_TYPE;

#endif
