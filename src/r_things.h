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
/*	Rendering of moving objects, sprites.*/

/*-----------------------------------------------------------------------------*/


#ifndef __R_THINGS__
#define __R_THINGS__


#ifdef __GNUG__
#pragma interface
#endif

extern vissprite_t*	vissprites;
extern vissprite_t*	vissprite_p;
extern vissprite_t	vsprsortedhead;

/* Constant arrays used for psprite clipping*/
/*  and initializing clipping.*/
#ifdef STATIC_RESOLUTION
extern dshort_t		negonearray[SCREENWIDTH];
extern dshort_t		screenheightarray[SCREENWIDTH];
#else
extern dshort_t		negonearray[MAXSCREENWIDTH];
extern dshort_t		screenheightarray[MAXSCREENWIDTH];
#endif

extern fixed_t		pspritescale;
extern fixed_t		pspriteiscale;


struct draw_context_s;

void R_DrawMaskedColumn (struct draw_context_s *ctx, const column_t* column);
void R_DrawMaskedColumnTranslucent (struct draw_context_s *ctx, const column_t *column);


void R_SortVisSprites (void);

void R_AddSprites (sector_t* sec);
void R_AddPSprites (void);
void R_InitSprites (const char** namelist);
void R_ClearSprites (void);
void R_DrawMasked (void);

#ifdef DIYRESAMPLETHINGS
typedef struct twin_context_s {
  int width;
  int modulo;
  const column_t *(*getcolumn)(struct twin_context_s *ctx, int num);
} twin_context_t;

extern int *R_CreateMaskedTwinColumns(twin_context_t *ctx, void *store);


typedef struct twin_draw_s {
  void (*resamplefunc)(struct draw_context_s*, const byte *, const byte *, int cf);
  void (*drawresfunc)(struct draw_context_s*);
} twin_draw_t;

extern void R_DrawMaskedTwinColumn(struct draw_context_s *ctx, const twin_draw_t *tw, const column_t *col1, const column_t *col2, int frac);

#endif

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
