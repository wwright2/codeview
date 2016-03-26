#ifndef __TYPES_H__
#define __TYPES_H__

typedef struct 
{
    int rows;
    int cols;
    int clrBytes;
    int clrBits;
} DISPLAY_PROFILE;

typedef struct WINDOW {
    struct WINDOW *next;        // list pointer
    struct WINDOW *prev;        // list pointer
    int priority;               // defines stacking order
    int id;                     // window id
    int updateNeeded;           // Is an animation due for an update
    int winChanged;             // has the window content changed
    uint64_t ttl;               // expiry time (epoch microseconds)
    uint32_t userData;	    	// App specific window identification, etc
    void *data;                 // none of your business (ridiculous)
} WINDOW;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
} roi_t;

typedef struct {
    int32_t X;
    int32_t Y;
} pos_t;

typedef struct {
    pos_t pos;
    roi_t roi;
} fsize_t;

#endif
