#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "sysGlobals.h"

#include "debug.h"

typedef struct __bmp_header_s {
    uint16_t ident;     /* BM */
    uint32_t file_size; /* size of file in bytes */
    uint16_t resv[2];   /* reserved */
    uint32_t offset;    /* offset of pixel data */
} __attribute__ ((packed)) bmp_header_t;

#if 0
typedef struct __bmp_dib_s {  /* BITMAPCOREHEADER  */
    uint32_t header_size;   /* the size of this header (12 bytes) */
    uint16_t width;         /* the bitmap width in pixels. */
    uint16_t height;        /* the bitmap height in pixels. */
    uint16_t planes;        /* the number of color planes; 1 is the only legal value */
    uint16_t bpp;           /* the number of bits per pixel. Typical values are 1, 4, 8 and 24 */
} __attribute__ ((packed)) bmp_dib_t;
#endif 

typedef struct __bmp_dib_s {  /* BITMAPINFOHEADER */
    uint32_t header_size; /* size of the DIB header (40 bytes) */
    int32_t  width;
    int32_t  height;
    uint16_t planes;    /* number of color planes (must be 1) */
    uint16_t bpp;       /* bits per pixel */
    uint32_t compression; /* compression type used (0 = uncompressed) */
    uint32_t image_size; /* size in bytes of IMAGE DATA (not file size) */
    int32_t  hres;      /* horizontal resolution (pixels per meter) */
    int32_t  vres;      /* vertical resolution (pixels per meter) */
    uint32_t npalette;  /* colors in palette (0, we're using truecolor) */
    uint32_t icolors;   /* number of important colors (0, usually ignored) */
} __attribute__ ((packed)) bmp_dib_t;


typedef struct __bmp_file_s {
    bmp_header_t header;
    bmp_dib_t dib;
    uint8_t pixels[];
} __attribute__ ((packed)) bmp_file_t;


static bmp_file_t *bmp = NULL;
static int bmpfd = 0;
static uint32_t filesize = 0;
static uint8_t rowpad = 0;
static uint32_t rowsize = 0;


void init_bmp (const char *name, uint32_t width, uint32_t height) {

    rowsize = (width * 3);
    rowpad = 4 - (rowsize & 3);
    if (rowpad == 4) rowpad = 0;
    rowsize += rowpad;  /* each row is padded to 4 bytes in size */

    filesize = (sizeof(bmp_file_t) + (rowsize * height));

    bmpfd = open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (bmpfd == -1) {
        __show_error("Unable to open bmp file '%s': %s", name, strerror(errno));
        return;
    }

    /* make sure the file is at least filesize bytes */
    lseek(bmpfd, (off_t)(filesize-1), SEEK_SET);
    write(bmpfd, (const void*)"\0", 1);

    bmp = (bmp_file_t*) mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, bmpfd, (off_t)0);
    if (bmp == MAP_FAILED) {
        __show_error("Unable to mmap bmpfile '%s': %s", name, strerror(errno));
        close(bmpfd);
        bmpfd = -1;
        bmp = NULL;
        return;
    }

    /* populate the header and dib data... */

    bmp->header.ident = *((uint16_t*)"BM");
    bmp->header.file_size = filesize;
    bmp->header.resv[0] = 0;
    bmp->header.resv[1] = 0;
    bmp->header.offset = sizeof(bmp_file_t);

    bmp->dib.header_size = sizeof(bmp_dib_t);
    bmp->dib.width = width;
    bmp->dib.height = height;
    bmp->dib.planes = 1;
    bmp->dib.bpp = 24;
    bmp->dib.compression = 0;
    bmp->dib.image_size = (rowsize * height);
    bmp->dib.hres = 100;
    bmp->dib.vres = 100;
    bmp->dib.npalette= 0;
    bmp->dib.icolors = 0;

    __show_notice("Opened '%s' for bmp output and mmapped file to address %p, filesize: %u, image size: %u", name, bmp, filesize, bmp->dib.image_size);

    return;
}

void write_bmp (DisplayBuf *buf) {
    if (bmp == NULL) {
        /* opening failed, but we don't want to nessessarily make it critical */
        return;
    }

//    __debug_verbose("Writing display buffer contents into bmp file at %p (rows: %hu, cols: %hu, pad: %hhu", bmp, bmp->dib.height, bmp->dib.width, rowpad);

    uint8_t *dst = &(bmp->pixels[0]);

    for (int y = bmp->dib.height - 1; y >= 0; y--) {
        uint8_t *src = &((*buf)[y][0][0]);
        for (int x = 0; x < bmp->dib.width; x++) {
            *dst++ = src[2];
            *dst++ = src[1];
            *dst++ = src[0];
            src += 3;
        }
        dst += rowpad;
    }

    return;
}


