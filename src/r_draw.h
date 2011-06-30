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


#ifndef __R_DRAW__
#define __R_DRAW__


#ifdef __GNUG__
#pragma interface
#endif



/* The span blitting interface.*/
/* Hook in assembler or system specific BLT*/
/*  here.*/
void 	R_DrawColumn (struct draw_context_s *ctx);
void 	R_DrawColumnLow (struct draw_context_s *ctx);
void    R_DrawColumnTranslucent(struct draw_context_s *ctx);
void    R_DrawColumnLowTranslucent(struct draw_context_s *ctx);
#ifdef DIYRESAMPLE
void    R_ResampleColumn(struct draw_context_s *ctx, int tex, int ci, int cf);
void    R_ResampleThingColumn(struct draw_context_s *ctx, const byte *col1, const byte *col2, int cf);
void    R_ResampleTranslatedThingColumn(struct draw_context_s *ctx, const byte *col1, const byte *col2, int cf);
void    R_DrawResampledColumn(struct draw_context_s *ctx);
void    R_DrawResampledTranslucentColumn(struct draw_context_s *ctx);
#endif

/* The Spectre/Invisibility effect.*/
void 	R_DrawFuzzColumn (struct draw_context_s *ctx);
void 	R_DrawFuzzColumnLow (struct draw_context_s *ctx);

/* Draw with color translation tables,*/
/*  for player sprite rendering,*/
/*  Green/Red/Blue/Indigo shirts.*/
void	R_DrawTranslatedColumn (struct draw_context_s *ctx);
void	R_DrawTranslatedColumnLow (struct draw_context_s *ctx);
void    R_DrawTranslatedColumnTranslucent(struct draw_context_s *ctx);
void    R_DrawTranslatedColumnLowTranslucent(struct draw_context_s *ctx);

void
R_VideoErase
( unsigned	ofs,
  int		count );

extern byte*		translationtables;


/* Span blitting for rows, floor/ceiling.*/
/* No Sepctre effect needed.*/
void 	R_DrawSpan (struct draw_context_s *ctx);

/* Low resolution mode, 160x200?*/
void 	R_DrawSpanLow (struct draw_context_s *ctx);


void
R_InitBuffer
( int		width,
  int		height );

#ifdef __riscos__
void	R_UpdateBuffer(int height);
#endif


/* Initialize color translation tables,*/
/*  for player rendering etc.*/
void	R_InitTranslationTables (void);



/* Rendering function.*/
void R_FillBackScreen (void);

/* If the view size is not full screen, draws a border around it.*/
void R_DrawViewBorder (void);


#ifndef STATIC_RESOLUTION
void R_DrawInitForResolution(void);
#endif

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
