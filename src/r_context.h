/*
 *  File containing the exact definition of the draw context structure.
 *  Must only be included by files which access d_ctx or need to dereference
 *  a pointer. This was done to minimize dependencies.
 */

#ifndef __R_DRAW_CONTEXT__
#define __R_DRAW_CONTEXT__

#ifdef __GNUG__
#pragma interface
#endif

#include "doomtype.h"
#include "r_defs.h"


/* the draw context structure in its full glory */

typedef struct draw_context_s {

  int			viewwidth;
  int			viewheight;
  int			screenwidth;
  int			screenheight;
  int			scaledviewwidth;
  int			detailshift;

  /* misc */
  int			centerx;
  int			centery;
  fixed_t		centerxfrac;
  fixed_t		centeryfrac;
  fixed_t		projection;

  /* column drawing interface */
  const lighttable_t*	dc_colormap;
  int			dc_x;
  int			dc_yl;
  int			dc_yh;
  fixed_t		dc_iscale;
  fixed_t		dc_texturemid;
  int			dc_texheight;
  const byte*		dc_source;
  const byte*		dc_translation;
  int			fuzztable;
  int			fuzzpos;
  int*			fuzzoffset;
  const lighttable_t*	fuzz_colourmap;
  lighttable_t*		resampled_column;
  fixed_t               resample_texfrac;

  /* span drawing interface */
  const lighttable_t*	ds_colormap;
  int			ds_y;
  int			ds_x1;
  int			ds_x2;
  fixed_t		ds_xfrac;
  fixed_t		ds_yfrac;
  fixed_t		ds_xstep;
  fixed_t		ds_ystep;
  const byte*		ds_source;

  /* sprites */
  int			sprtopscreen;
  int			spryscale;
  dshort_t*		floorclip;	/* was mfloorclip */
  dshort_t*		ceilingclip;	/* was mceilingclip */

  /* only needed for RISC OS 16bpp, but to keep the structure constant
     we always define it */
  int			dc_dbl_yl;
  int			dc_dbl_yh;
  const byte*		dc_dbl_source;
  fixed_t		dc_dbl_iscale;

  void*			trans_colmaps;	/* just for export */
  lighttable_t*		static_colourmap;

  pixel_t**		ylookup;
  int*			columnofs;

  /* colourmap translation */
  const int*		lightmultipliers;
  byte*			trans_cmap_work;
  int			num_colourmaps;

  /* for debugging on RISC OS */
  byte*			frame_buffer_start;
  byte*			frame_buffer_end;
  byte*			end_of_slot;

} draw_context_t;


extern draw_context_t d_ctx;

#endif
