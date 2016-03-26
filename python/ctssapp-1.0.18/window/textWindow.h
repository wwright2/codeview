#ifndef __TEXTWINDOW_H__
#define __TEXTWINDOW_H__

#include "winTypes.h"
#include "font.h"
#include "helpers.h"

roi_t determineTextLineSize(const char *data, const FONT *f, const char **endptr);
roi_t determineTextBufferSize(const char *data, const FONT *f);
uint32_t allocTextData (const char *data, char **dataBuf, uint8_t **bufptr, roi_t size, const DISPLAY_PROFILE *dp);

const char *interpolate_variable(const char **ptrref);

#endif /* __TEXT_WINDOW_H__ */

