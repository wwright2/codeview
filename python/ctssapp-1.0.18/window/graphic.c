// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       graphic.c
//
//  Description:    Drawing routines for graphics
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  05/10/2012  Joe Halpin      1
//  Notes:
//
// ***************************************************************************

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <png.h>
#include <zlib.h>
#include <string.h>
#include <pthread.h>

#include "windowData.h"

#include "log.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

extern char resourceDir[];

pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct _graphic_s {
    struct _graphic_s *next;
    pthread_mutex_t lock;
    uint32_t refs;
    uint32_t nameLen;
    char *name; 
    GRAPHIC_IMG *image;
} graphic_t;

static graphic_t *graphics = NULL;

char pathbuf[1024] = { '\0' };
char *path_fname_ref = NULL;
uint32_t buf_fname_sz = 0;


GRAPHIC_IMG *load_png_image(const char *inFile) {
    char header[8];
    png_bytepp rows;

    GRAPHIC_IMG *image = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;

    /* open file and test for it being a png */
    FILE *fpIn = fopen(inFile, "rb");
    if (!fpIn) {
        LOGWARN ("Failed to open graphic file '%s': %s", inFile, strerror(errno));
        return NULL;
    }

    fread(header, 1, 8, fpIn);
    if (png_sig_cmp((uint8_t*)header, 0, 8)) {
        LOGWARN ("Graphic file '%s' is not a valid png file", inFile);
        fclose(fpIn);
        return NULL;
    }

    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        LOGWARN ("Could not allocate png_ptr: %s", strerror(errno));
        fclose(fpIn);
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        LOGWARN ("Could not create png_infop", 0);
        goto png_destroy_handles;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        LOGWARN ("Longjmp to png catch!", 0);
        goto png_destroy_handles;
    }

    png_init_io(png_ptr, fpIn);
    png_set_sig_bytes(png_ptr, 8);
    png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_SHIFT | PNG_TRANSFORM_GRAY_TO_RGB, 0);
    rows = png_get_rows (png_ptr, info_ptr);

    uint32_t rowbytes    = png_get_rowbytes (png_ptr, info_ptr);
    uint32_t img_height  = png_get_image_height (png_ptr, info_ptr);

    image = malloc (sizeof(GRAPHIC_IMG) + (sizeof(void *) * img_height));
    image->g = malloc(sizeof(GRAPHIC_DATA) + (rowbytes * img_height));

    image->g->rowbytes   = rowbytes;
    image->g->channels   = png_get_channels (png_ptr, info_ptr);
    image->g->imgWidth   = png_get_image_width (png_ptr, info_ptr);
    image->g->imgHeight  = img_height;
    image->g->bitDepth   = png_get_bit_depth (png_ptr, info_ptr);
    image->g->clrType    = png_get_color_type (png_ptr, info_ptr);

    LOGWARN ("Loaded image '%s', details: \n\trow bytes: %u, channels: %u, size: %ux%u depth: %u color type: %u", inFile, image->g->rowbytes, image->g->channels, image->g->imgWidth, image->g->imgHeight, image->g->bitDepth, image->g->clrType);

    uint8_t *ptr = image->g->data;

    LOGDBG ("Copying image data to %p", ptr);

    for (int i = 0; i < img_height; i++) {
        memcpy(ptr, rows[i], rowbytes);
        image->rowPtrs[i] = ptr;
        ptr += rowbytes;
    }

  png_destroy_handles:
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fpIn);

    return image;
}


GRAPHIC_IMG *newGraphicImg(const char *name, uint32_t nameLen) {
    
    pthread_mutex_lock(&list_lock);

    /* check cache */
    graphic_t *graph = graphics;
    for (; graph; graph = graph->next)
        if ((graph->nameLen == nameLen) && !strncmp(name, graph->name, nameLen))
            break;

    if (graph) {
        /* return from cache */
        pthread_mutex_lock(&graph->lock);
        graph->refs++;
        pthread_mutex_unlock(&graph->lock);
        pthread_mutex_unlock(&list_lock);
        return graph->image;
    }


    /* ensure pathbuf is setup with resourceDir and path_fname_ref points to the end */
    if (pathbuf[0] == '\0') {
        uint32_t len = strlen(resourceDir);
        strncpy(pathbuf, resourceDir, 1024);
        if (pathbuf[len-1] != '/') {
            pathbuf[len] = '/';
            pathbuf[len+1] = '\0';
            len++;
        }
        path_fname_ref = pathbuf + len;
        buf_fname_sz = 1024 - len;
    }

    if (nameLen > buf_fname_sz) {
        LOGERR ("Pathbuf overflow for '%s'", name);
        pthread_mutex_unlock(&list_lock);
        return NULL;
    }

    /* copy in file name to end of pathbuf and load png file */
    strncpy(path_fname_ref, name, buf_fname_sz);

    GRAPHIC_IMG *image = load_png_image(pathbuf);

    if (image == NULL) {
        LOGWARN ("Failed to load png file.", 0);
        pthread_mutex_unlock(&list_lock);
        return NULL;
    }

    /* allocate new cached graphic link */
    graph = malloc(sizeof(graphic_t));
    pthread_mutex_init(&graph->lock, NULL);
    graph->refs = 1;
    graph->nameLen = nameLen;
    graph->name = strdup(name);
    graph->image = image;

    graph->next = graphics;
    graphics = graph;

    pthread_mutex_unlock(&list_lock);

    return graph->image;
}


void delGraphicImg (GRAPHIC_IMG *img) {
    pthread_mutex_lock(&list_lock);

    graphic_t *last = NULL, *graph = graphics;
    for (; graph; last = graph, graph = graph->next)
        if (graph->image == img) break;

    if (!graph) {
        /* this graphic is not referenced in the cache??? */
        LOGWARN ("Called to delGraphicImg(%p), but this GRAPHIC_IMG is *not* referenced in the cache!", img);
        free(img->g);
        free(img);
        pthread_mutex_unlock(&list_lock);
        return;
    }

    pthread_mutex_lock(&graph->lock);

    graph->refs--;

    if (!graph->refs) {
        /* no more references.... lets not destroy it here though, we've got the memory and this is hardly a cache purge */
        LOGWARN ("References to graphic '%s' have dropped to zero.  May be purged later", graph->name);
    }

    pthread_mutex_unlock(&graph->lock);

    pthread_mutex_unlock(&list_lock);

    return;
}

void imgDraw (GRAPHIC_IMG *img, WINDOW_DATA *wd, uint8_t ***dst_buf) {
    //int srcBytes = img->g->rowbytes;
    //int dstBytes = wd->dim.dst.width * COLOR_DEPTH;

    int h = wd->dim.dst.height;
    int w = wd->dim.dst.width;

    int oX = wd->offset.src.X;
    int oY = wd->offset.src.Y;

    if (h > (img->g->imgHeight - oY)) h = img->g->imgHeight - oY;
    if (w > (img->g->imgWidth - oX)) w = img->g->imgWidth - oX;

    int sX = wd->offset.dst.X;
    int sY = wd->offset.dst.Y;

    int eX = sX + w;
    int eY = sY + h;

    if (eY > wd->dp.rows) eY = wd->dp.rows;
    if (eX > wd->dp.cols) eX = wd->dp.cols;

    for (; sY < eY; oY++, sY++) {
        uint8_t *src = img->rowPtrs[oY]; src += oX + oX + oX;
        uint8_t *dst = (uint8_t*)&(dst_buf[sY][sX][0]);
        for (int x = w; x; x--) {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
        }
    }
}
