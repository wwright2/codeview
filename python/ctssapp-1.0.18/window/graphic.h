// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       graphic.h
//
//  Description:    Declarations for drawing graphics
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  05/10/2012  Joe Halpin      1
//  Notes:
//
// ***************************************************************************

#ifndef GRAPHIC_H_INCLUDED
#define GRAPHIC_H_INCLUDED

#include <stdint.h>

#include <window.h>
#include <windowData.h>

GRAPHIC_IMG *newGraphicImg(const char *name, uint32_t nameLen);
GRAPHIC_IMG *load_png_image(const char *inFile);
void delGraphicImg (GRAPHIC_IMG *g);
void imgDraw (GRAPHIC_IMG *g, WINDOW_DATA *wd, uint8_t ***dst);

#endif
