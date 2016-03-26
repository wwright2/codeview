#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "font.h"
#include "winTypes.h"
#include "helpers.h"

#include "log.h"

#include "textWindow.h"

#define EXP_STR_TIME "88:88:88 WW"
#define EXP_STR_DATE "88/88/8888"

static const char *interpolate_date();
static const char *interpolate_time();
static inline const char *get_expansion_string (const char **ptrref);

//
// TODO: try to avoid the assumption that there is enough buffer space
// left to check for these expansions.
//
const char *interpolate_variable(const char **ptrref) {
    const char *ptr = *ptrref;
    if (*ptr != '$') return NULL;

    if ((ptr[1] == 't') && (ptr[2] == 'i') && (ptr[3] == 'm') && (ptr[4] == 'e')) {
        *ptrref += 5;
        return interpolate_time();
    }

    if ((ptr[1] == 'd') && (ptr[2] == 'a') && (ptr[3] == 't') && (ptr[4] == 'e')) {
        *ptrref += 5;
        return interpolate_date();
    }

    return NULL;
}

static inline const char *get_expansion_string (const char **ptrref) {
    const char *ptr = *ptrref; 
    if (*ptr != '$') return NULL;

    if ((ptr[1] == 't') && (ptr[2] == 'i') && (ptr[3] == 'm') && (ptr[4] == 'e')) {
        *ptrref += 5;
        return EXP_STR_TIME;
    }

    if ((ptr[1] == 'd') && (ptr[2] == 'a') && (ptr[3] == 't') && (ptr[4] == 'e')) {
        *ptrref += 5;
        return EXP_STR_DATE;
    }

    return NULL;
}

static inline const char *interpolate_time () {
    static char buf[12];
    time_t t = time(NULL);
    struct tm *tmp = localtime(&t);
    const char *td = "am";
    
    if (tmp->tm_hour > 12) {
        tmp->tm_hour -= 12;
        td = "pm";
    } else if (!tmp->tm_hour) 
        tmp->tm_hour = 12;
    
    snprintf(buf,12, "%02u:%02u:%02u %s", ((tmp->tm_hour > 12) ? (tmp->tm_hour - 12) : tmp->tm_hour), tmp->tm_min, tmp->tm_sec, td);

    return buf;
}

static const char *interpolate_date() {
    static char buf[11];
    time_t t = time(NULL);
    struct tm *tmp = localtime(&t);
    
    snprintf(buf, 11, "%02u/%02u/%04u", tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_year + 1900);

    return buf;
}

roi_t determineTextLineSize (const char *c, const FONT *f, const char **line_end) {
    roi_t roi = { 0, 0, 0 };
    GLYPH *g;

    LOGTRACEW ("determining line size for the first line of '%s'", c);

    for (; *c && (*c != '\n'); c++) {
        const char *expansion = get_expansion_string(&c);
        if (expansion) {
            roi_t tr = determineTextLineSize(expansion, f, NULL);
            roi.width += tr.width;
            c--;
            continue;
        }

        if ((*c < f->startIndex) || (*c > (f->startIndex + f->numGlyphs)))
            continue;

        int idx = *c;
        if ((g = f->offsets[idx]) == NULL)
            continue;

        LOGTRACEW ("### char '%c': (font %p, idx: %d, g: %p) width: %u", *c, f, idx, g, g->width);
        roi.width += g->width;
        if (roi.height < g->height) roi.height = g->height;
    }

    if (line_end != NULL)
        *line_end = c;

    LOGTRACEW ("<-- line size is %ux%u", roi.width, roi.height); 

    return roi;
}

roi_t determineTextBufferSize(const char *data, const FONT *f) {
    roi_t roi = { 0, 0, 0 };
    const char *line_end = NULL;

    LOGTRACEW ("determining buffer size needed for '%s'", data);

    while (*data) {
        roi_t ll = determineTextLineSize(data, f, &line_end);
        if (ll.width > roi.width) roi.width = ll.width;
        if (roi.height) roi.height++;
        roi.height += ll.height;
        data = line_end;
        if (*data) data++;
    }
    LOGTRACEW ("<-- text buffer size is %dx%d", roi.width, roi.height);

    return roi;
}

uint32_t allocTextData (const char *data, char **dataBuf, uint8_t **bufptr, roi_t size, const DISPLAY_PROFILE *dp) {

    int dataSize = size.width * size.height * dp->clrBytes;
    *bufptr = calloc(1, dataSize);
    if (bufptr == 0)
	return -1;

    *dataBuf = strdup(data);
    if (dataBuf == 0)
    {
	free (bufptr);
	return (size_t) -1;
    }

    return dataSize;
}


