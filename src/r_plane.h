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
/*	Refresh, visplane stuff (floor, ceilings).*/

/*-----------------------------------------------------------------------------*/


#ifndef __R_PLANE__
#define __R_PLANE__


#include "r_data.h"

#ifdef __GNUG__
#pragma interface
#endif


/* Visplane related.*/
extern dshort_t*	lastopening;


typedef void (*planefunction_t) (int top, int bottom);

#ifdef STATIC_RESOLUTION
extern dshort_t		floorclip[SCREENWIDTH];
extern dshort_t		ceilingclip[SCREENWIDTH];

extern fixed_t		yslope[SCREENHEIGHT];
extern fixed_t		distscale[SCREENWIDTH];
#else
extern dshort_t		floorclip[MAXSCREENWIDTH];
extern dshort_t		ceilingclip[MAXSCREENWIDTH];

extern fixed_t		yslope[MAXSCREENHEIGHT];
extern fixed_t		distscale[MAXSCREENWIDTH];
void R_PlaneInitForResolution(void);
#endif


void R_InitPlanes (void);
void R_ClearPlanes (void);

void
R_MapPlane
( int		y,
  int		x1,
  int		x2 );

void
R_MakeSpans
( int		x,
  int		t1,
  int		b1,
  int		t2,
  int		b2 );

void R_DrawPlanes (void);

visplane_head_t*
R_FindPlane
( fixed_t	height,
  int		picnum,
  int		lightlevel
#ifdef DIYBOOM
, fixed_t	xoffs,
  fixed_t	yoffs
#endif
);

visplane_head_t*
R_CheckPlane
( visplane_head_t*	pl,
  int		start,
  int		stop );



#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
