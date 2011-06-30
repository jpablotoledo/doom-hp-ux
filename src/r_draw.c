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

/* $Log:$*/

/* DESCRIPTION:*/
/*	The actual span/column drawing functions.*/
/*	Here find the main potential for optimization,*/
/*	 e.g. inline assembly, different algorithms.*/

/*-----------------------------------------------------------------------------*/


static const char
rcsid[] = "$Id: r_draw.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";


#include "doomdef.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"
#include "r_context.h"

/* Needs access to LFB (guess what).*/
#include "v_video.h"

/* State.*/
#include "doomstat.h"

#include "i_video.h"

#ifdef __riscos__
#include "ROsupport.h"
#endif



/* Play safe */
#if (defined(DIYRESAMPLE) && (LD_PIXEL_DEPTH < 4))
#undef DIYRESAMPLE
#endif


/* All drawing to the view buffer is accomplished in this file.*/
/* The other refresh files only know about ccordinates,*/
/*  not the architecture of the frame buffer.*/
/* Conveniently, the frame buffer is a linear one,*/
/*  and we need only the base address,*/
/*  and the total size == width*height*depth/8.,*/



pixel_t*	ylookup[MAXSCREENHEIGHT];
int		columnofs[MAXSCREENWIDTH];
int		viewwindowx;
int		viewwindowy;

draw_context_t	d_ctx;

/*static byte*	viewimage;*/

/* Color tables for different players,*/
/*  translate a limited part to another*/
/*  (color ramps used for  suit colors).*/

/*static byte	translations[3][256];*/

byte*		translationtables = NULL;

/* just for profiling*/
int			dscount;

/* Factor by which to darken shadows. The higher, the darker */
#define FUZZ_DARKEN		0x20

#if (LD_PIXEL_DEPTH == 3)

#define FUZZTABLE		50
#ifdef STATIC_RESOLUTION
#define FUZZOFF	(SCREENWIDTH)
#else
#define FUZZOFF 1
#endif

int	fuzzoffset[FUZZTABLE] =
{
    FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};

#endif







#ifdef DIYBOOM
#define BOOM_COLUMN_PREPARE \
  texheight = ctx->dc_texheight; if (texheight == 0) texheight = 0x80; \
  { \
    int sign = 0; \
    if (frac < 0) {sign = 1; frac = -frac;} \
    while (frac >= (texheight << FRACBITS)) frac -= (texheight << FRACBITS); \
    if (sign == 0) frac -= (texheight << FRACBITS); \
    else frac = -frac; \
    sign = (fracstep >= 0) ? fracstep : -fracstep; \
    if (sign >= (texheight << FRACBITS)) fracstep = 0; \
    src += texheight; \
  }
#define TEXEL_ADDRESS(f) ((f)>>FRACBITS)
#define TEXEL_STEP(f,s)	f = frac + s; if (f >= 0) f -= (texheight << FRACBITS);
#else
#define BOOM_COLUMN_PREPARE
#define TEXEL_ADDRESS(f) (((f)>>FRACBITS)&127)
#define TEXEL_STEP(f,s)	f = frac + s;
#endif


#ifndef DIYARMASS

/* R_DrawColumn*/
/* Source is the top of the column to scale.*/

/* just for profiling */
/*static int		dccount;*/


/* A column is a vertical slice/span from a wall texture that,*/
/*  given the DOOM style restrictions on the view orientation,*/
/*  will always have constant z depth.*/
/* Thus a special case loop for very fast rendering can*/
/*  be used. It has also been used with Wolfenstein 3D.*/

void R_DrawColumn (draw_context_t *ctx)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;
    const byte*		src;
    BOOMSTATEMENT(int texheight;)

    count = ctx->dc_yh - ctx->dc_yl;

    /* Zero length, column does not exceed a pixel.*/
    if (count < 0)
	return;

#ifdef RANGECHECK
    if ((unsigned)(ctx->dc_x) >= ctx->scaledviewwidth
	|| ctx->dc_yl < 0
	|| ctx->dc_yh >= ctx->viewheight)
#ifdef RANGECHECK_ABORTS
	I_Error ("R_DrawColumn: %i to %i at %i", ctx->dc_yl, ctx->dc_yh, ctx->dc_x);
#else
  	return;
#endif
#endif

    /* Framebuffer destination address.*/
    /* Use ylookup LUT to avoid multiply with ScreenWidth.*/
    /* Use columnofs LUT for subwindows? */
    dest = (ctx->ylookup)[ctx->dc_yl] + (ctx->columnofs)[ctx->dc_x];
    src = ctx->dc_source;

    /* Determine scaling,*/
    /*  which is the only mapping to be done.*/
    fracstep = ctx->dc_iscale;
    frac = ctx->dc_texturemid + (ctx->dc_yl - ctx->centery)*fracstep;

    BOOM_COLUMN_PREPARE;

    /* Inner loop that does the actual texture mapping,*/
    /*  e.g. a DDA-lile scaling.*/
    /* This is as fast as it gets.*/
#ifdef DIYRESAMPLE
    /* No resampling if step size is >= 1.0 */
    if (abs(fracstep) < (1<<FRACBITS))
    {
        pixel_t		tx0, tx1;
        fixed_t		fraction;
        int		r0,g0,b0,r1,g1,b1;

        do
	{
	    /* Very heavy on the CPU: resample texture by 1D linear interpolation.
	      Available in true-colour modes only! */
	    tx0 = (pixel_t)(ctx->dc_colormap)[src[TEXEL_ADDRESS(frac)]];
	    TEXEL_STEP(fraction, (1<<FRACBITS));
	    tx1 = (pixel_t)(ctx->dc_colormap)[src[TEXEL_ADDRESS(fraction)]];
	    READ_RGB_PIXEL(tx0, r0, g0, b0);
	    READ_RGB_PIXEL(tx1, r1, g1, b1);
	    fraction = frac & ((1<<FRACBITS)-1);
	    r0 += ((r1 - r0)*fraction) >> FRACBITS;
	    g0 += ((g1 - g0)*fraction) >> FRACBITS;
	    b0 += ((b1 - b0)*fraction) >> FRACBITS;
	    *dest = (pixel_t)BUILD_RGB_PIXEL(r0, g0, b0);
	    dest += ctx->screenwidth;
	    TEXEL_STEP(frac, fracstep);
	}
	while (count--);
    }
    else
    {
#endif
    do
    {
	/* Re-map color indices from wall texture column*/
	/*  using a lighting/special effects LUT.*/
	*dest = (pixel_t)(ctx->dc_colormap)[src[TEXEL_ADDRESS(frac)]];
	dest += ctx->screenwidth;
	TEXEL_STEP(frac, fracstep);
    }
    while (count--);
#ifdef DIYRESAMPLE
    }
#endif
}


#ifdef DIYRESAMPLE

void R_ResampleColumn(draw_context_t *ctx, int tex, int ci, fixed_t cf)
{
  lighttable_t *dest = ctx->resampled_column;
  pixel_t tx0, tx1;
  byte *col1, *col2;
  int r0, g0, b0, r1, g1, b1;
  unsigned int height, i;

  col1 = R_GetColumn(tex, ci);
  col2 = R_GetColumn(tex, ci+1);
  height = (unsigned int)(ctx->dc_texheight);
  if (height == 0) height = 128;
#ifdef DIYBOOM
  if (height > 256) height = 256;
#else
  if (height > 128) height = 128;
#endif
  for (i=0; i<height; i++)
  {
    tx0 = (pixel_t)(ctx->dc_colormap)[(unsigned char)(*col1++)];
    tx1 = (pixel_t)(ctx->dc_colormap)[(unsigned char)(*col2++)];
    READ_RGB_PIXEL(tx0, r0, g0, b0);
    READ_RGB_PIXEL(tx1, r1, g1, b1);
    /* Sun compiler being bloody stupid... */
    r0 += ((int)((r1 - r0)*cf)) >> FRACBITS;
    g0 += ((int)((g1 - g0)*cf)) >> FRACBITS;
    b0 += ((int)((b1 - b0)*cf)) >> FRACBITS;
    *dest++ = (lighttable_t)BUILD_RGB_PIXEL(r0, g0, b0);
  }
#ifndef DIYBOOM
  if (height < 128)
  {
    *dest++ = *(ctx->resampled_column);
  }
#endif
}


void R_ResampleThingColumn(draw_context_t *ctx, const byte *col1, const byte *col2, int cf)
{
  lighttable_t *dest = ctx->resampled_column;
  pixel_t tx0, tx1;
  int r0, g0, b0, r1, g1, b1;
  unsigned int height, i;

  height = (unsigned int)(ctx->dc_texheight);
  for (i=0; i<height; i++)
  {
    tx0 = (pixel_t)(ctx->dc_colormap)[(unsigned char)(*col1++)];
    tx1 = (pixel_t)(ctx->dc_colormap)[(unsigned char)(*col2++)];
    READ_RGB_PIXEL(tx0, r0, g0, b0);
    READ_RGB_PIXEL(tx1, r1, g1, b1);
    /* Sun compiler being bloody stupid... */
    r0 += ((int)((r1 - r0)*cf)) >> FRACBITS;
    g0 += ((int)((g1 - g0)*cf)) >> FRACBITS;
    b0 += ((int)((b1 - b0)*cf)) >> FRACBITS;
    *dest++ = (lighttable_t)BUILD_RGB_PIXEL(r0, g0, b0);
  }
}


void R_ResampleTranslatedThingColumn(draw_context_t *ctx, const byte *col1, const byte *col2, int cf)
{
  lighttable_t *dest = ctx->resampled_column;
  pixel_t tx0, tx1;
  int r0, g0, b0, r1, g1, b1;
  unsigned int height, i;
  const unsigned char *translation = ctx->dc_translation;

  height = (unsigned int)(ctx->dc_texheight);
  for (i=0; i<height; i++)
  {
    tx0 = (pixel_t)(ctx->dc_colormap)[translation[(unsigned char)(*col1++)]];
    tx1 = (pixel_t)(ctx->dc_colormap)[translation[(unsigned char)(*col2++)]];
    READ_RGB_PIXEL(tx0, r0, g0, b0);
    READ_RGB_PIXEL(tx1, r1, g1, b1);
    /* Sun compiler being bloody stupid... */
    r0 += ((int)((r1 - r0)*cf)) >> FRACBITS;
    g0 += ((int)((g1 - g0)*cf)) >> FRACBITS;
    b0 += ((int)((b1 - b0)*cf)) >> FRACBITS;
    *dest++ = (lighttable_t)BUILD_RGB_PIXEL(r0, g0, b0);
  }
}



/* Special function: draws a resampled column which is already translated
   to screen colours. */
void R_DrawResampledColumn(draw_context_t *ctx)
{
    int			count;
    pixel_t*		dest;
    lighttable_t*	src;
    fixed_t		frac;
    fixed_t		fracstep;
    lighttable_t	tx0, tx1;
    fixed_t		fraction;
    int			r0,g0,b0,r1,g1,b1;
    BOOMSTATEMENT(int texheight;)

    count = ctx->dc_yh - ctx->dc_yl;
    src = ctx->resampled_column;

    /* Zero length, column does not exceed a pixel.*/
    if (count < 0)
	return;

#ifdef RANGECHECK
    if ((unsigned)(ctx->dc_x) >= ctx->scaledviewwidth
	|| ctx->dc_yl < 0
	|| ctx->dc_yh >= ctx->viewheight)
#ifdef RANGECHECK_ABORTS
	I_Error ("R_DrawResampledColumn: %i to %i at %i", ctx->dc_yl, ctx->dc_yh, ctx->dc_x);
#else
  	return;
#endif
#endif

    /* See R_DrawColumn */
    dest = (ctx->ylookup)[ctx->dc_yl] + (ctx->columnofs)[ctx->dc_x];

    fracstep = ctx->dc_iscale;
    frac = ctx->dc_texturemid + (ctx->dc_yl - ctx->centery)*fracstep;

    BOOM_COLUMN_PREPARE;

    do
    {
	/* The column is alread in pixel_t format! */
	tx0 = src[TEXEL_ADDRESS(frac)];
	TEXEL_STEP(fraction, (1<<FRACBITS));
	tx1 = src[TEXEL_ADDRESS(fraction)];
	READ_RGB_PIXEL(tx0, r0, g0, b0);
	READ_RGB_PIXEL(tx1, r1, g1, b1);
	fraction = frac & ((1<<FRACBITS)-1);
	r0 += ((r1 - r0)*fraction) >> FRACBITS;
	g0 += ((g1 - g0)*fraction) >> FRACBITS;
	b0 += ((b1 - b0)*fraction) >> FRACBITS;
	*dest = (pixel_t)BUILD_RGB_PIXEL(r0, g0, b0);
	dest += ctx->screenwidth;
	TEXEL_STEP(frac, fracstep);
    }
    while (count--);
}


/* Special function: draws a resampled column which is already translated
   to screen colours. */
void R_DrawResampledTranslucentColumn(draw_context_t *ctx)
{
    int			count;
    pixel_t*		dest;
    lighttable_t*	src;
    fixed_t		frac;
    fixed_t		fracstep;
    lighttable_t	tx0, tx1;
    fixed_t		fraction;
    int			r0,g0,b0,r1,g1,b1;
    BOOMSTATEMENT(int texheight;)

    count = ctx->dc_yh - ctx->dc_yl;
    src = ctx->resampled_column;

    /* Zero length, column does not exceed a pixel.*/
    if (count < 0)
	return;

#ifdef RANGECHECK
    if ((unsigned)(ctx->dc_x) >= ctx->scaledviewwidth
	|| ctx->dc_yl < 0
	|| ctx->dc_yh >= ctx->viewheight)
#ifdef RANGECHECK_ABORTS
	I_Error ("R_DrawResampledColumn: %i to %i at %i", ctx->dc_yl, ctx->dc_yh, ctx->dc_x);
#else
  	return;
#endif
#endif

    /* See R_DrawColumn */
    dest = (ctx->ylookup)[ctx->dc_yl] + (ctx->columnofs)[ctx->dc_x];

    fracstep = ctx->dc_iscale;
    frac = ctx->dc_texturemid + (ctx->dc_yl - ctx->centery)*fracstep;

    BOOM_COLUMN_PREPARE;

    do
    {
	/* The column is alread in pixel_t format! */
	tx0 = src[TEXEL_ADDRESS(frac)];
	TEXEL_STEP(fraction, (1<<FRACBITS));
	tx1 = src[TEXEL_ADDRESS(fraction)];
	READ_RGB_PIXEL(tx0, r0, g0, b0);
	READ_RGB_PIXEL(tx1, r1, g1, b1);
	fraction = frac & ((1<<FRACBITS)-1);
	r0 += ((r1 - r0)*fraction) >> FRACBITS;
	g0 += ((g1 - g0)*fraction) >> FRACBITS;
	b0 += ((b1 - b0)*fraction) >> FRACBITS;
	tx1 = *dest;
	READ_RGB_PIXEL(tx1, r1, g1, b1);
	r0 = (r0 + r1) >> 1;
	g0 = (g0 + g1) >> 1;
	b0 = (b0 + b1) >> 1;
	*dest = (pixel_t)BUILD_RGB_PIXEL(r0, g0, b0);
	dest += ctx->screenwidth;
	TEXEL_STEP(frac, fracstep);
    }
    while (count--);
}
#endif



#if ((LD_PIXEL_DEPTH > 3) && defined(DIYTRANSPARENCY))
void R_DrawColumnTranslucent (draw_context_t *ctx)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;
    int			r0,g0,b0,r1,g1,b1;
    pixel_t		tx0, tx1;
    const byte*		src;
    BOOMSTATEMENT(int texheight;)

    count = ctx->dc_yh - ctx->dc_yl;

    /* Zero length, column does not exceed a pixel.*/
    if (count < 0)
	return;

#ifdef RANGECHECK
    if ((unsigned)(ctx->dc_x) >= ctx->scaledviewwidth
	|| ctx->dc_yl < 0
	|| ctx->dc_yh >= ctx->viewheight)
#ifdef RANGECHECK_ABORTS
	I_Error ("R_DrawColumn: %i to %i at %i", ctx->dc_yl, ctx->dc_yh, ctx->dc_x);
#else
  	return;
#endif
#endif

    dest = (ctx->ylookup)[ctx->dc_yl] + (ctx->columnofs)[ctx->dc_x];
    src = ctx->dc_source;

    fracstep = ctx->dc_iscale;
    frac = ctx->dc_texturemid + (ctx->dc_yl - ctx->centery)*fracstep;

    BOOM_COLUMN_PREPARE;

#ifdef DIYRESAMPLE
    /* No resampling if step size is >= 1.0 */
    if (abs(fracstep) < (1<<FRACBITS))
    {
        fixed_t		fraction;

	do
	{
	    /* Very heavy on the CPU: resample texture by 1D linear interpolation.
	      Available in true-colour modes only! */
	    tx0 = (pixel_t)(ctx->dc_colormap)[src[TEXEL_ADDRESS(frac)]];
	    TEXEL_STEP(fraction, (1<<FRACBITS));
	    tx1 = (pixel_t)(ctx->dc_colormap)[src[TEXEL_ADDRESS(fraction)]];
	    READ_RGB_PIXEL(tx0, r0, g0, b0);
	    READ_RGB_PIXEL(tx1, r1, g1, b1);
	    fraction = frac & ((1<<FRACBITS)-1);
	    r0 += ((r1 - r0)*fraction) >> FRACBITS;
	    g0 += ((g1 - g0)*fraction) >> FRACBITS;
	    b0 += ((b1 - b0)*fraction) >> FRACBITS;
	    tx1 = *dest;
	    READ_RGB_PIXEL(tx1, r1, g1, b1);
	    r0 = (r0 + r1) >> 1;
	    g0 = (g0 + g1) >> 1;
	    b0 = (b0 + b1) >> 1;
	    *dest = (pixel_t)BUILD_RGB_PIXEL(r0, g0, b0);
	    dest += ctx->screenwidth;
	    TEXEL_STEP(frac, fracstep);
	}
	while (count--);
    }
    else
    {
#endif
    do
    {
	/* Re-map color indices from wall texture column*/
	/*  using a lighting/special effects LUT.*/
	tx0 = (pixel_t)(ctx->dc_colormap)[src[TEXEL_ADDRESS(frac)]];
	tx1 = *dest;
	READ_RGB_PIXEL(tx0, r0, g0, b0);
	READ_RGB_PIXEL(tx1, r1, g1, b1);
	r0 = (r0 + r1) >> 1;
	g0 = (g0 + g1) >> 1;
	b0 = (b0 + b1) >> 1;
	*dest = (pixel_t)BUILD_RGB_PIXEL(r0, g0, b0);
	dest += ctx->screenwidth;
	TEXEL_STEP(frac, fracstep);
    }
    while (count--);
#ifdef DIYRESAMPLE
    }
#endif
}
#endif



void R_DrawColumnLow (draw_context_t *ctx)
{
    int			count;
    pixel_t*		dest;
    pixel_t*		dest2;
    fixed_t		frac;
    fixed_t		fracstep;
    const byte*		src;
    BOOMSTATEMENT(int texheight;)

    count = ctx->dc_yh - ctx->dc_yl;

    /* Zero length.*/
    if (count < 0)
	return;

#ifdef RANGECHECK
    if ((unsigned)(ctx->dc_x) >= ctx->scaledviewwidth
	|| ctx->dc_yl < 0
	|| ctx->dc_yh >= ctx->viewheight)
    {
#ifdef RANGECHECK_ABORTS
	I_Error ("R_DrawColumn: %i to %i at %i", ctx->dc_yl, ctx->dc_yh, ctx->dc_x);
#else
	return;
#endif
    }
    /*	dccount++; */
#endif

    /* Blocky mode, need to multiply by 2.*/
    ctx->dc_x <<= 1;

    dest = (ctx->ylookup)[ctx->dc_yl] + (ctx->columnofs)[ctx->dc_x];
    dest2 = (ctx->ylookup)[ctx->dc_yl] + (ctx->columnofs)[ctx->dc_x+1];
    src = ctx->dc_source;

    fracstep = ctx->dc_iscale;
    frac = ctx->dc_texturemid + (ctx->dc_yl - ctx->centery)*fracstep;

    BOOM_COLUMN_PREPARE;

    do
    {
	/* Hack. Does not work corretly.*/
	*dest2 = *dest = (pixel_t)(ctx->dc_colormap)[src[TEXEL_ADDRESS(frac)]];
	dest += ctx->screenwidth;
	dest2 += ctx->screenwidth;
	TEXEL_STEP(frac, fracstep);

    } while (count--);
}



/* Spectre/Invisibility.*/

/* Framebuffer postprocessing.*/
/* Creates a fuzzy image by copying pixels*/
/*  from adjacent ones to left and right.*/
/* Used with an all black colormap, this*/
/*  could create the SHADOW effect,*/
/*  i.e. spectres and invisible players.*/

void R_DrawFuzzColumn (draw_context_t *ctx)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;
#if (LD_PIXEL_DEPTH > 3)
    int			red, green, blue;
#endif

    /* Adjust borders. Low... */
    if (!ctx->dc_yl)
	ctx->dc_yl = 1;

    /* .. and high.*/
    if (ctx->dc_yh == ctx->viewheight-1)
	ctx->dc_yh = ctx->viewheight - 2;

    count = ctx->dc_yh - ctx->dc_yl;

    /* Zero length.*/
    if (count < 0)
	return;


#ifdef RANGECHECK
    if ((unsigned)(ctx->dc_x) >= ctx->scaledviewwidth
	|| ctx->dc_yl < 0 || ctx->dc_yh >= ctx->viewheight)
    {
#ifdef RANGECHECK_ABORTS
	I_Error ("R_DrawFuzzColumn: %i to %i at %i", ctx->dc_yl, ctx->dc_yh, ctx->dc_x);
#else
	return;
#endif
    }
#endif


    /* Does not work with blocky mode.*/
    dest = (ctx->ylookup)[ctx->dc_yl] + (ctx->columnofs)[ctx->dc_x];

    /* Looks familiar.*/
    fracstep = ctx->dc_iscale;
    frac = ctx->dc_texturemid + (ctx->dc_yl - ctx->centery)*fracstep;

    /* Looks like an attempt at dithering,*/
    /*  using the colormap #6 (of 0-31, a bit*/
    /*  brighter than average).*/
    do
    {
	/* Lookup framebuffer, and retrieve*/
	/*  a pixel that is either one column*/
	/*  left or right of the current one.*/
	/* Add index from colormap to index.*/
#if (LD_PIXEL_DEPTH == 3)
#ifdef DIYBOOM
        *dest = (pixel_t)fullcolormap[6*256+dest[(ctx->fuzzoffset)[ctx->fuzzpos]]];
#else
	*dest = (pixel_t)colormaps[6*256+dest[(ctx->fuzzoffset)[ctx->fuzzpos]]];
#endif
	/* Clamp table lookup index.*/
	if (++(ctx->fuzzpos) == FUZZTABLE)
	    ctx->fuzzpos = 0;
#else
	READ_RGB_PIXEL(*dest, red, green, blue);
	red -= FUZZ_DARKEN; if (red < 0) red = 0;
	green -= FUZZ_DARKEN; if (green < 0) green = 0;
	blue -= FUZZ_DARKEN; if (blue < 0) blue = 0;
	*dest = BUILD_RGB_PIXEL(red, green, blue);
#endif
	dest += ctx->screenwidth;

	frac += fracstep;
    } while (count--);
}





/* R_DrawTranslatedColumn*/
/* Used to draw player sprites*/
/*  with the green colorramp mapped to others.*/
/* Could be used with different translation*/
/*  tables, e.g. the lighter colored version*/
/*  of the BaronOfHell, the HellKnight, uses*/
/*  identical sprites, kinda brightened up.*/

void R_DrawTranslatedColumn (draw_context_t *ctx)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;

    count = ctx->dc_yh - ctx->dc_yl;

    if (count < 0)
	return;

#ifdef RANGECHECK
    if ((unsigned)(ctx->dc_x) >= ctx->scaledviewwidth
	|| ctx->dc_yl < 0
	|| ctx->dc_yh >= ctx->viewheight)
    {
#ifdef RANGECHECK_ABORTS
	I_Error ( "R_DrawColumn: %i to %i at %i", ctx->dc_yl, ctx->dc_yh, ctx->dc_x);
#else
	return;
#endif
    }

#endif


    /* FIXME. As above.*/
    dest = (ctx->ylookup)[ctx->dc_yl] + (ctx->columnofs)[ctx->dc_x];

    /* Looks familiar.*/
    fracstep = ctx->dc_iscale;
    frac = ctx->dc_texturemid + (ctx->dc_yl - ctx->centery)*fracstep;

    /* Here we do an additional index re-mapping.*/
#ifdef DIYRESAMPLE
    if (abs(fracstep) < (1<<FRACBITS))
    {
        pixel_t		tx0, tx1;
        fixed_t		fraction;
        int		r0,g0,b0,r1,g1,b1;

	do
	{
	    tx0 = (pixel_t)(ctx->dc_colormap)[(ctx->dc_translation)[(ctx->dc_source)[frac>>FRACBITS]]];
	    tx1 = (pixel_t)(ctx->dc_colormap)[(ctx->dc_translation)[(ctx->dc_source)[(frac>>FRACBITS)+1]]];
	    READ_RGB_PIXEL(tx0, r0, g0, b0);
	    READ_RGB_PIXEL(tx1, r1, g1, b1);
	    fraction = frac & ((1<<FRACBITS)-1);
	    r0 += ((r1 - r0)*fraction) >> FRACBITS;
	    g0 += ((g1 - g0)*fraction) >> FRACBITS;
	    b0 += ((b1 - b0)*fraction) >> FRACBITS;
	    *dest = (pixel_t)BUILD_RGB_PIXEL(r0, g0, b0);
	    dest += ctx->screenwidth;
	    frac += fracstep;
	}
	while (count--);
    }
    else
    {
#endif
    do
    {
	/* Translation tables are used*/
	/*  to map certain colorramps to other ones,*/
	/*  used with PLAY sprites.*/
	/* Thus the "green" ramp of the player 0 sprite*/
	/*  is mapped to gray, red, black/indigo. */
	*dest = (pixel_t)(ctx->dc_colormap)[(ctx->dc_translation)[(ctx->dc_source)[frac>>FRACBITS]]];
	dest += ctx->screenwidth;
	frac += fracstep;
    }
    while (count--);
#ifdef DIYRESAMPLE
    }
#endif
}





/* R_DrawSpan */
/* With DOOM style restrictions on view orientation,*/
/*  the floors and ceilings consist of horizontal slices*/
/*  or spans with constant z depth.*/
/* However, rotation around the world z axis is possible,*/
/*  thus this mapping, while simpler and faster than*/
/*  perspective correct texture mapping, has to traverse*/
/*  the texture at an angle in all but a few cases.*/
/* In consequence, flats are not stored by column (like walls),*/
/*  and the inner loop has to step in texture space u and v.*/

/* Draws the actual span.*/
void R_DrawSpan (draw_context_t *ctx)
{
    fixed_t		xfrac;
    fixed_t		yfrac;
    pixel_t*		dest;
    int			count;
    int			spot;

#ifdef RANGECHECK
    if (ctx->ds_x2 < ctx->ds_x1
	|| ctx->ds_x1 < 0
	|| ctx->ds_x2 >= ctx->scaledviewwidth
	|| (unsigned)(ctx->ds_y) >= ctx->viewheight)
    {
#ifdef RANGECHECK_ABORTS
	I_Error( "R_DrawSpan: %i to %i at %i", ctx->ds_x1, ctx->ds_x2, ctx->ds_y);
#else
	return;
#endif
    }
/*	dscount++; */
#endif

    xfrac = ctx->ds_xfrac;
    yfrac = ctx->ds_yfrac;

    dest = (ctx->ylookup)[ctx->ds_y] + (ctx->columnofs)[ctx->ds_x1];

    /* We do not check for zero spans here?*/
    count = ctx->ds_x2 - ctx->ds_x1;

#ifdef DIYRESAMPLE
    if ((abs(ctx->ds_xstep) < (1<<FRACBITS)) && (abs(ctx->ds_ystep) < (1<<FRACBITS)))
    {
        pixel_t		tx0, tx1;
        fixed_t		fraction;
        int		r0,g0,b0,r1,g1,b1,r2,g2,b2;

        do
	{
	    /* Extremely heavy on the CPU: bilinear interpolation... */
	    /* First interpolate the two horizontal segments */
	    fraction = xfrac & ((1<<FRACBITS)-1);
	    spot = ((yfrac>>(FRACBITS-6)) & (63 << 6));
	    tx0 = (pixel_t)(ctx->ds_colormap)[(ctx->ds_source)[spot + ((xfrac>>FRACBITS)&63)]];
	    tx1 = (pixel_t)(ctx->ds_colormap)[(ctx->ds_source)[spot + (((xfrac>>FRACBITS)+1)&63)]];
	    READ_RGB_PIXEL(tx0, r0, g0, b0);
	    READ_RGB_PIXEL(tx1, r1, g1, b1);
	    r0 += ((r1 - r0)*fraction) >> FRACBITS;
	    g0 += ((g1 - g0)*fraction) >> FRACBITS;
	    b0 += ((b1 - b0)*fraction) >> FRACBITS;
	    spot = (((yfrac + (1<<FRACBITS))>>(FRACBITS-6)) & (63 << 6));
	    tx0 = (pixel_t)(ctx->ds_colormap)[(ctx->ds_source)[spot + ((xfrac>>FRACBITS)&63)]];
	    tx1 = (pixel_t)(ctx->ds_colormap)[(ctx->ds_source)[spot + (((xfrac>>FRACBITS)+1)&63)]];
	    READ_RGB_PIXEL(tx0, r1, g1, b1);
	    READ_RGB_PIXEL(tx1, r2, g2, b2);
	    r1 += ((r2 - r1)*fraction) >> FRACBITS;
	    g1 += ((g2 - g1)*fraction) >> FRACBITS;
	    b1 += ((b2 - b1)*fraction) >> FRACBITS;
	    /* Now interpolate the two colour vectors vertically */
	    fraction = yfrac & ((1<<FRACBITS)-1);
	    r0 += ((r1 - r0)*fraction) >> FRACBITS;
	    g0 += ((g1 - g0)*fraction) >> FRACBITS;
	    b0 += ((b1 - b0)*fraction) >> FRACBITS;
	    *dest++ = (pixel_t)BUILD_RGB_PIXEL(r0, g0, b0);
	    xfrac += ctx->ds_xstep; yfrac += ctx->ds_ystep;
	}
	while (count--);
    }
    else
    {
#endif
    do
    {
	/* Current texture index in u,v.*/
	spot = ((yfrac>>(FRACBITS-6))&(63<<6)) + ((xfrac>>FRACBITS)&63);
	/* Lookup pixel from flat texture tile,*/
	/*  re-index using light/colormap.*/
	*dest++ = (pixel_t)(ctx->ds_colormap)[(ctx->ds_source)[spot]];
	/* Next step in u,v.*/
	xfrac += ctx->ds_xstep;
	yfrac += ctx->ds_ystep;
    }
    while (count--);
#ifdef DIYRESAMPLE
    }
#endif
}






/* Again..*/
void R_DrawSpanLow (draw_context_t *ctx)
{
    fixed_t		xfrac;
    fixed_t		yfrac;
    pixel_t*		dest;
    int			count;
    int			spot;

#ifdef RANGECHECK
    if (ctx->ds_x2 < ctx->ds_x1
	|| ctx->ds_x1 < 0
	|| ctx->ds_x2 >= ctx->scaledviewwidth
	|| (unsigned)(ctx->ds_y) >= ctx->viewheight)
    {
#ifdef RANGECHECK_ABORTS
	I_Error( "R_DrawSpan: %i to %i at %i", ctx->ds_x1, ctx->ds_x2, ctx->ds_y);
#else
	return;
#endif
    }
/*	dscount++; */
#endif

    xfrac = ctx->ds_xfrac;
    yfrac = ctx->ds_yfrac;

    /* Blocky mode, need to multiply by 2.*/
    ctx->ds_x1 <<= 1;
    ctx->ds_x2 <<= 1;

    dest = (ctx->ylookup)[ctx->ds_y] + (ctx->columnofs)[ctx->ds_x1];


    count = ctx->ds_x2 - ctx->ds_x1;
    do
    {
	spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
	/* Lowres/blocky mode does it twice,*/
	/*  while scale is adjusted appropriately.*/
	*dest++ = (pixel_t)(ctx->ds_colormap)[(ctx->ds_source)[spot]];
	*dest++ = (pixel_t)(ctx->ds_colormap)[(ctx->ds_source)[spot]];

	xfrac += ctx->ds_xstep;
	yfrac += ctx->ds_ystep;

    } while (count--);
}

#else	/* !DIYARMASS */

#ifdef DIYRESAMPLE
void R_ResampleColumn(draw_context_t *ctx, int tex, int ci, int cf)
{
#ifdef DEBUG_PLOTTERS
  fprintf(logfile, "c"); fflush(logfile);
#endif
  Rarm_ResampleColumn(ctx, R_GetColumn(tex, ci), R_GetColumn(tex, ci+1), cf);
#ifdef DEBUG_PLOTTERS
  fprintf(logfile, "d"); fflush(logfile);
#endif
}
#endif

#endif







/* R_InitTranslationTables*/
/* Creates the translation tables to map*/
/*  the green color ramp to gray, brown, red.*/
/* Assumes a given structure of the PLAYPAL.*/
/* Could be read from a lump instead.*/

void R_InitTranslationTables (void)
{
    int		i;

    translationtables = Z_Malloc (256*3+255, PU_STATIC, 0);
    translationtables = (byte *)(( (int)translationtables + 255 )& ~255);

    /* translate just the 16 green colors*/
    for (i=0 ; i<256 ; i++)
    {
	if (i >= 0x70 && i<= 0x7f)
	{
	    /* map green ramp to gray, brown, red*/
	    translationtables[i] = 0x60 + (i&0xf);
	    translationtables [i+256] = 0x40 + (i&0xf);
	    translationtables [i+512] = 0x20 + (i&0xf);
	}
	else
	{
	    /* Keep all other colors as is.*/
	    translationtables[i] = translationtables[i+256]
		= translationtables[i+512] = i;
	}
    }
}





/* R_InitBuffer */
/* Creats lookup tables that avoid*/
/*  multiplies and other hazzles*/
/*  for getting the framebuffer address*/
/*  of a pixel to draw.*/

void
R_InitBuffer
( int		width,
  int		height )
{
    int		i;

    /* Handle resize,*/
    /*  e.g. smaller view windows*/
    /*  with border and/or status bar.*/
    viewwindowx = (SCREENWIDTH-width) >> 1;
    d_ctx.columnofs = columnofs;
    d_ctx.ylookup = ylookup;
    d_ctx.screenwidth = SCREENWIDTH;
    d_ctx.screenheight = SCREENHEIGHT;
#if (LD_PIXEL_DEPTH == 3)
    d_ctx.fuzzoffset = fuzzoffset;
    d_ctx.fuzzpos = 0;
    d_ctx.fuzztable = FUZZTABLE;
#endif
#ifdef DIYRESAMPLE
    /* Reserve a little extra space for the resampled column */
    if ((d_ctx.resampled_column = (lighttable_t*)Z_Malloc(280*sizeof(lighttable_t), PU_STATIC, NULL)) == NULL)
      I_Error("I_InitBuffer: Couldn't claim memory for resampled column!\n");
#endif

    /* Column offset. For windows. */
    /* (on RISC OS plotters are in assembler, so it's convenient to scale by pixel size) */
    for (i=0 ; i<width ; i++)
#ifdef DIYARMASS
	(d_ctx.columnofs)[i] = (viewwindowx + i)*sizeof(pixel_t);
#else
	(d_ctx.columnofs)[i] = viewwindowx + i;
#endif

    /* Same with base row offset.*/
    if (width == SCREENWIDTH)
	viewwindowy = 0;
    else
	viewwindowy = (SCREENHEIGHT-SBARHEIGHT-height) >> 1;

    /* Precalculate all row offsets.*/
    for (i=0 ; i<height ; i++)
	(d_ctx.ylookup)[i] = screens[0] + (i+viewwindowy)*SCREENWIDTH;
}


void R_UpdateBuffer(int height)
{
    int		i;
    int		offset;

    offset = viewwindowy*SCREENWIDTH;
    for (i=0 ; i<height ; i++)
    {
	(d_ctx.ylookup)[i] = screens[0] + offset;
	offset += SCREENWIDTH;
    }
}




/* R_FillBackScreen*/
/* Fills the back screen with a pattern*/
/*  for variable screen sizes*/
/* Also draws a beveled edge.*/

void R_FillBackScreen (void)
{
#if (defined(__riscos__) || (LD_PIXEL_DEPTH > 3))
    byte*	srcchunk;
#endif
    byte*	src;
    pixel_t*	dest;
    int		x;
    int		y;
    patch_t*	patch;
    int		y_high;

    /* DOOM border patch.*/
    char	name1[] = "FLOOR7_2";

    /* DOOM II border patch.*/
    char	name2[] = "GRNROCK";

    char*	name;

    if ((d_ctx.scaledviewwidth == SCREENWIDTH) && (SCREENWIDTH == 320))
	return;

    if ( gamemode == commercial)
	name = name2;
    else
	name = name1;

    src = W_CacheLumpName2 (name, ns_flats, PU_CACHE);
    dest = screens[1];

    /*
     *  For higher resolutions we fill the bottom strip too because the status bar
     *  doesn't completely cover it.
     */
#ifdef STATIC_RESOLUTION
#if (SCREENWIDTH <= 320)
    y_high = SCREENHEIGHT - SBARHEIGHT;
#else
    y_high = SCREENHEIGHT;
#endif
#else
    if (SCREENWIDTH <= 320)
	y_high = SCREENHEIGHT - SBARHEIGHT;
    else
	y_high = SCREENHEIGHT;
#endif

#if (LD_PIXEL_DEPTH == 3)
#ifdef __riscos__
    if (translated_colourmaps != NULL)
    {
	for (y=0 ; y<y_high ; y++)
	{
	    srcchunk = src + ((y&63)<<6);
	    for (x=0 ; x<SCREENWIDTH ; x++)
	    {
		*dest++ = translated_colourmaps[(unsigned char)(srcchunk[(x&63)])];
	    }
	    /* Since we're going byte-wise anyway the right-padding isn't needed. */
	}
    }
    else
    {
#endif
    for (y=0 ; y<y_high ; y++)
    {
	for (x=0 ; x<SCREENWIDTH/64 ; x++)
	{
	    memcpy (dest, src+((y&63)<<6), 64);
	    dest += 64;
	}

	if (SCREENWIDTH&63)
	{
	    memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
	    dest += (SCREENWIDTH&63);
	}
    }
#ifdef __riscos__
    }
#endif

#else
    for (y=0 ; y<y_high ; y++)
    {
	srcchunk = src + ((y&63)<<6);
	for (x=0 ; x<SCREENWIDTH ; x++)
	{
	    *dest++ = (pixel_t)translated_colourmaps[(unsigned char)(srcchunk[(x&63)])];
	}
    }
#endif

    patch = W_CacheLumpName ("brdr_t",PU_CACHE);

    for (x=0 ; x<d_ctx.scaledviewwidth ; x+=8)
	V_DrawPatch (viewwindowx+x,viewwindowy-8,1,patch);
    patch = W_CacheLumpName ("brdr_b",PU_CACHE);

    for (x=0 ; x<d_ctx.scaledviewwidth ; x+=8)
	V_DrawPatch (viewwindowx+x,viewwindowy+d_ctx.viewheight,1,patch);
    patch = W_CacheLumpName ("brdr_l",PU_CACHE);

    for (y=0 ; y<d_ctx.viewheight ; y+=8)
	V_DrawPatch (viewwindowx-8,viewwindowy+y,1,patch);
    patch = W_CacheLumpName ("brdr_r",PU_CACHE);

    for (y=0 ; y<d_ctx.viewheight ; y+=8)
	V_DrawPatch (viewwindowx+d_ctx.scaledviewwidth,viewwindowy+y,1,patch);


    /* Draw beveled edge. */
    V_DrawPatch (viewwindowx-8,
		 viewwindowy-8,
		 1,
		 W_CacheLumpName ("brdr_tl",PU_CACHE));

    V_DrawPatch (viewwindowx+d_ctx.scaledviewwidth,
		 viewwindowy-8,
		 1,
		 W_CacheLumpName ("brdr_tr",PU_CACHE));

    V_DrawPatch (viewwindowx-8,
		 viewwindowy+d_ctx.viewheight,
		 1,
		 W_CacheLumpName ("brdr_bl",PU_CACHE));

    V_DrawPatch (viewwindowx+d_ctx.scaledviewwidth,
		 viewwindowy+d_ctx.viewheight,
		 1,
		 W_CacheLumpName ("brdr_br",PU_CACHE));
}



/* Copy a screen buffer.*/

void
R_VideoErase
( unsigned	ofs,
  int		count )
{
  /* LFB copy.*/
  /* This might not be a good idea if memcpy*/
  /*  is not optiomal, e.g. byte by byte on*/
  /*  a 32bit CPU, as GNU GCC/Linux libc did*/
  /*  at one point.*/
  memcpy (screens[0]+ofs, screens[1]+ofs, count*sizeof(pixel_t));
}



/* R_DrawViewBorder*/
/* Draws the border around the view*/
/*  for different size windows?*/

void
V_MarkRect
( int		x,
  int		y,
  int		width,
  int		height );

void R_DrawViewBorder (void)
{
    int		top;
    int		side;
#ifndef __riscos__
    int		ofs;
    int		i;
#endif

    if (d_ctx.scaledviewwidth == SCREENWIDTH)
	return;

    top = ((SCREENHEIGHT-SBARHEIGHT)-d_ctx.viewheight)/2;
    side = (SCREENWIDTH-d_ctx.scaledviewwidth)/2;
#ifdef __riscos__
    /*
     *  The values assigned to viewheight and viewwidth are multiples of 8 (see
     *  R_ExecuteSetViewSize (r_main.c). SCREENWIDTH / HEIGHT should be multiples
     *  of 8 too, so we always copy whole words.
     *  This function has a different name for each pixel depth to make sure you
     *  can't link plotters for a different pixel depth with the rest of the code.
     */
#ifdef DEBUG_PLOTTERS
    fprintf(logfile, "a"); fflush(logfile);
#endif
    Rarm_DrawViewBorder(screens[0], screens[1], top, side, SCREENWIDTH, d_ctx.viewheight);
#ifdef DEBUG_PLOTTERS
    fprintf(logfile, "b"); fflush(logfile);
#endif

#else
    /* copy top and one line of left side */
    R_VideoErase (0, top*SCREENWIDTH+side);

    /* copy one line of right side and bottom */
    ofs = (d_ctx.viewheight+top)*SCREENWIDTH-side;
    R_VideoErase (ofs, top*SCREENWIDTH+side);

    /* copy sides using wraparound */
    ofs = top*SCREENWIDTH + SCREENWIDTH-side;
    side <<= 1;

    for (i=1 ; i<d_ctx.viewheight ; i++)
    {
	R_VideoErase (ofs, side);
	ofs += SCREENWIDTH;
    }
#endif
    /* ? */
    V_MarkRect (0,0,SCREENWIDTH, SCREENHEIGHT-SBARHEIGHT);
}



#ifndef STATIC_RESOLUTION
void R_DrawInitForResolution(void)
{
#if (LD_PIXEL_DEPTH == 3)
  int i;

  for (i=0; i<FUZZTABLE; i++)
  {
    /* use local copy, not draw context pointer! (not initialized yet) */
    if (fuzzoffset[i] < 0)
      (fuzzoffset)[i] = -SCREENWIDTH;
    else
      (fuzzoffset)[i] = SCREENWIDTH;
  }
#endif
}
#endif
