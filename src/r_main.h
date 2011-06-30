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


#ifndef __R_MAIN__
#define __R_MAIN__

#include "d_player.h"
#include "r_data.h"

#ifdef __GNUG__
#pragma interface
#endif



/* POV related.*/

extern fixed_t		viewcos;
extern fixed_t		viewsin;
extern int		viewangleoffset;

extern int		viewwindowx;
extern int		viewwindowy;

extern fixed_t		projection;

extern int		validcount;

extern int		linecount;
extern int		loopcount;



/* Number of diminishing brightness levels.*/
/* There a 0-31, i.e. 32 LUT in the COLORMAP lump.*/
/* Allow more for 32bpp modes */
#if (LD_PIXEL_DEPTH == 3)
#define NUMCOLORMAPS		32
#elif (LD_PIXEL_DEPTH == 4)
#define NUMCOLORMAPS		32
#elif (LD_PIXEL_DEPTH == 5)
#define NUMCOLORMAPS		128
#endif


/* Lighting LUT.*/
/* Used for z-depth cuing per column/row,*/
/*  and other lighting effects (sector ambient, flash).*/


/* Lighting constants.*/
/* Now why not 32 levels here?*/
#if (LD_PIXEL_DEPTH == 3)
#define LIGHTLEVELS	        16
#define LIGHTSEGSHIFT	         4
#else
#define LIGHTLEVELS		32
#define LIGHTSEGSHIFT	         3
#endif

#define MAXLIGHTSCALE		48
#define LIGHTSCALESHIFT		12
#define MAXLIGHTZ	       (4*NUMCOLORMAPS)
#if   (NUMCOLORMAPS == 32)
#define LIGHTZSHIFT		20
#elif (NUMCOLORMAPS == 64)
#define LIGHTZSHIFT		19
#elif (NUMCOLORMAPS == 128)
#define LIGHTZSHIFT		18
#elif (NUMCOLORMAPS == 256)
#define LIGHTZSHIFT		17
#else
#error "Numcolormaps must be one of 32, 64, 128 or 256!"
#endif

extern const lighttable_t*	scalelightfixed[MAXLIGHTSCALE];
#if (defined(DIYBOOM) && (LD_PIXEL_DEPTH == 3))
/* code this as offsets into the currently active colourmap */
extern lighttable_t*	(*scalelight)[MAXLIGHTSCALE];
extern lighttable_t*	(*zlight)[MAXLIGHTZ];
#else
extern lighttable_t*	scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern lighttable_t*	zlight[LIGHTLEVELS][MAXLIGHTZ];
#endif

extern int		extralight;
extern const lighttable_t* fixedcolormap;


/* Blocky/low detail mode.*/
/*B remove this?*/
/*  0 = high, 1 = low*/
extern boolean		setsizeneeded;


/* Function pointers to switch refresh/drawing functions.*/
/* Used to select shadow mode etc.*/

struct draw_context_s;

extern void		(*colfunc) (struct draw_context_s *);
extern void		(*basecolfunc) (struct draw_context_s *);
extern void		(*fuzzcolfunc) (struct draw_context_s *);
/* No shadow effects on floors.*/
extern void		(*spanfunc) (struct draw_context_s *);
#if (defined(__riscos__) && (LD_PIXEL_DEPTH == 4))
extern void		(*doublecolfunc) (struct draw_context_s *);
#endif



/* Utility functions.*/
int
R_PointOnSegSide
( fixed_t	x,
  fixed_t	y,
  const seg_t*	line );

angle_t
R_PointToAngle
( fixed_t	x,
  fixed_t	y );

angle_t
R_PointToAngle2
( fixed_t	x1,
  fixed_t	y1,
  fixed_t	x2,
  fixed_t	y2 );

fixed_t
R_PointToDist
( fixed_t	x,
  fixed_t	y );


fixed_t R_ScaleFromGlobalAngle (angle_t visangle);

void
R_AddPointToBox
( int		x,
  int		y,
  fixed_t*	box );

#ifndef DIYINLINE
/* for inlining these functions are static */
int
R_PointOnSide
( fixed_t	x,
  fixed_t	y,
  const node_t*	node );

subsector_t*
R_PointInSubsector
( fixed_t	x,
  fixed_t	y );
#endif




/* REFRESH - the actual rendering functions.*/


/* Called by G_Drawer.*/
void R_RenderPlayerView (player_t *player);

/* Called by startup code.*/
void R_Init (void);

/* Called by M_Responder.*/
void R_SetViewSize (int blocks, int detail);

void R_ExecuteSetViewSize (void);

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
