/* Emacs style mode select   -*- C++ -*- */
/*-----------------------------------------------------------------------------*/

/* $Id:$*/

/* Copyright (C) 1993-1996 by id Software, Inc.*/

/* This source is available for distribution and/or modification*/
/* only under the terms of the DOOM Source Code License as*/
/* published by id Software. All rights reserved.*/

/* The source is distributed in the hope that it will be useful,*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of*/
/* FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License*/
/* for more details.*/

/* DESCRIPTION:*/
/*	System specific interface stuff.*/

/*-----------------------------------------------------------------------------*/


#ifndef __I_VIDEO__
#define __I_VIDEO__


#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif


#ifdef __riscos__
#define INVAL_STATUS		1
#define INVAL_BACKGRND		2

pixel_t *I_GetDisplayedScreen(void);
pixel_t *I_GetSpareScreen(void);
void I_InvalidateArea(unsigned int flags);
void I_ReadDisplayedScreen(pixel_t *scr);
void I_DisplayFrameBuffer(void *adr);

/* configure variables (m_misc.c) */
extern int HardwareGamma;
extern int BufferedKeyboard;
extern int InstHandlerFlags;
/* handler flags */
#define IHFlag_Exit	1
#define IHFlag_Abort	2
#define IHFlag_StFix	4
#endif
extern int MinLightLevel;


/* To get the lighttable_t definition */
#include "r_defs.h"

#if (LD_PIXEL_DEPTH == 3)
extern byte*			translated_colourmaps;
#else
extern lighttable_t		translated_colourmaps[];

/*
 *  Highly machine-specific. Experiment for yourself. These macros were tested with
 *  the following machine configurations:
 *  32bpp display: Linux PC (x86)
 *  16bpp display: Linux Notebook (x86)
 *  32bpp little endian: PC, local. 32bpp big endian: Sun Ultra 1, remote.
 *  16bpp little endian: Notebook, local. 16bpp big endian: Sun Ultra 1, remote.
 *  Everything is working fine with that setup.
 */
#define BUILD_RGB_PIX15(r,g,b) (((r) >> 3) | (((g) & 0xf8) << 2) | (((b) & 0xf8) << 7))
#define BUILD_RGB_PIX16(r,g,b) (((r) >> 3) | (((g) & 0xfc) << 3) | (((b) & 0xf8) << 8))
#define BUILD_RGB_PIX32(r,g,b) ((r) | ((g) << 8) | ((b) << 16))
#define BUILD_BGR_PIX15(r,g,b) (((b) >> 3) | (((g) & 0xf8) << 2) | (((b) & 0xf8) << 7))
#define BUILD_BGR_PIX16(r,g,b) (((b) >> 3) | (((g) & 0xfc) << 3) | (((r) & 0xf8) << 8))
#define BUILD_BGR_PIX32(r,g,b) ((b) | ((g) << 8) | ((r) << 16))
#define READ_RGB_PIX15(v,r,g,b) r=((v)<<3)&0xf8; g=((v)>>2)&0xf8; b=((v)>>7)&0xf8;
#define READ_RGB_PIX16(v,r,g,b) r=((v)<<3)&0xf8; g=((v)>>3)&0xfc; b=((v)>>8)&0xf8;
#define READ_RGB_PIX32(v,r,g,b) r=(v)&0xff; g=((v)>>8)&0xff; b=((v)>>16)&0xff;
#define READ_BGR_PIX15(v,r,g,b) b=((v)<<3)&0xf8; g=((v)>>2)&0xf8; r=((v)>>7)&0xf8;
#define READ_BGR_PIX16(v,r,g,b) b=((v)<<3)&0xf8; g=((v)>>3)&0xfc; r=((v)>>8)&0xf8;
#define READ_BGR_PIX32(v,r,g,b) b=(v)&0xff; g=((v)>>8)&0xff; r=((v)>>16)&0xff;

#ifdef __riscos__
#if (LD_PIXEL_DEPTH == 4)
#define BUILD_RGB_PIXEL(r,g,b) BUILD_RGB_PIX15(r,g,b)
#define READ_RGB_PIXEL(v,r,g,b) READ_RGB_PIX15(v,r,g,b)
#else
#define BUILD_RGB_PIXEL(r,g,b) BUILD_RGB_PIX32(r,g,b)
#define READ_RGB_PIXEL(v,r,g,b) READ_RGB_PIX32(v,r,g,b)
#endif
#endif

#ifdef __sun
#if (LD_PIXEL_DEPTH == 4)
#define BUILD_RGB_PIXEL(r,g,b) BUILD_RGB_PIX16(r,g,b)
#define READ_RGB_PIXEL(v,r,g,b) READ_RGB_PIX16(v,r,g,b)
#else
#define BUILD_RGB_PIXEL(r,g,b) BUILD_RGB_PIX32(r,g,b)
#define READ_RGB_PIXEL(v,r,g,b)	READ_RGB_PIX32(v,r,g,b)
#endif
#endif

#ifdef LINUX
#if (LD_PIXEL_DEPTH == 4)
#define BUILD_RGB_PIXEL(r,g,b) BUILD_BGR_PIX16(r,g,b)
#define READ_RGB_PIXEL(v,r,g,b) READ_BGR_PIX16(v,r,g,b)
#else
#define BUILD_RGB_PIXEL(r,g,b) BUILD_BGR_PIX32(r,g,b)
#define READ_RGB_PIXEL(v,r,g,b) READ_BGR_PIX32(v,r,g,b)
#endif
#endif

/*#if (LD_PIXEL_DEPTH == 4)
#ifdef __BIG_ENDIAN__
#define BUILD_RGB_PIXEL(r,g,b)	((((b)&0xf8)<<5) | (((g)&0x1c)<<12) | ((g)>>5) | ((r)&0xf8))
#define READ_RGB_PIXEL(v,r,g,b)	r=((v)&0xf8); g=(((v)&0x07)<<5) | (((v)&0xe000)>>12); b=(((v)&0x1c00)>>5);
#else
#define BUILD_RGB_PIXEL(r,g,b)	(((b)>>3) | (((g)&0xfc)<<3) | (((r)&0xf8)<<8))
#define READ_RGB_PIXEL(v,r,g,b)	r=(((v)>>8)&0xf8); g=(((v)>>3)&0xfc); b=(((v)<<3)&0xf8);
#endif
#else
#ifdef __BIG_ENDIAN__
#define BUILD_RGB_PIXEL(r,g,b)	(((b)<<24) | ((g)<<16) | ((r)<<8))
#define READ_RGB_PIXEL(v,r,g,b)	r=(((v)>>8)&0xff); g=(((v)>>16)&0xff); b=(((v)>>24)&0xff);
#else
#define BUILD_RGB_PIXEL(r,g,b)	((b) | ((g)<<8) | ((r)<<16))
#define READ_RGB_PIXEL(v,r,g,b)	r=(((v)>>16)&0xff); g=(((v)>>8)&0xff); b=((v)&0xff);
#endif
#endif*/

#endif	/* LD_PIXEL_DEPTH > 3 */

extern boolean	PaletteChanged;
void I_TranslateColourmaps(const byte *palette);


#ifdef DIYBOOM
/* plot status bar / automap / ... with original colourmap */
extern lighttable_t	translated_basemap[];
#endif



/* Called by D_DoomMain,*/
/* determines the hardware configuration*/
/* and sets up the video mode*/
void I_InitGraphics (void);


void I_ShutdownGraphics(void);

/* Takes full 8 bit values.*/
void I_SetPalette (const byte* palette);

void I_UpdateNoBlit (void);
void I_FinishUpdate (void);

/* Wait for vertical retrace or pause a bit.*/
void I_WaitVBL(int count);

void I_ReadScreen (pixel_t* scr);

void I_BeginRead (void);
void I_EndRead (void);

/* clear all frame buffers (called when a screen is displayed) */
void I_ClearFrameBuffers(void);

/* Make a screenshot */
void I_ScreenShot(void);

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
