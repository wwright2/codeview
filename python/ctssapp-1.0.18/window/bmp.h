#ifndef __BMP_H__
#define __BMP_H__

#include "sysGlobals.h"

void write_bmp (DisplayBuf *buf);
void init_bmp (const char *name, uint32_t width, uint32_t height);

#endif /* __BMP_H__ */

