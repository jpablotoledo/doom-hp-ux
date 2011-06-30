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
/*	All the clipping: columns, horizontal spans, sky columns.*/

/*-----------------------------------------------------------------------------*/


static const char
rcsid[] = "$Id: r_segs.c,v 1.3 1997/01/29 20:10:19 b1 Exp $";





#include <stdlib.h>

#include "i_system.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_context.h"
#include "r_sky.h"
#include "z_zone.h"



#if (defined(DIYRESAMPLE) && (LD_PIXEL_DEPTH < 4))
#undef DIYRESAMPLE
#endif

#if (defined(__riscos__) && (LD_PIXEL_DEPTH == 4) && !defined(DIYRESAMPLE))
#define USE_DOUBLE_COLUMNS
#endif


#ifdef DIYRESAMPLE

#ifdef DEBUG_PLOTTERS
#define CALL_RESAMPLE_COLUMN(texture) \
  if (d_ctx.dc_iscale >= (1<<FRACBITS)) \
  { \
    d_ctx.dc_source = R_GetColumn(texture, texturecolumn); \
    fprintf(logfile, "k"); fflush(logfile); \
    colfunc(&d_ctx); \
    fprintf(logfile, "l"); fflush(logfile); \
  } \
  else \
  { \
    R_ResampleColumn(&d_ctx, texture, texturecolumn, d_ctx.resample_texfrac); \
    fprintf(logfile, "g"); fflush(logfile); \
    R_DrawResampledColumn(&d_ctx); \
    fprintf(logfile, "h"); fflush(logfile); \
  }
#else
#define CALL_RESAMPLE_COLUMN(texture) \
  if (d_ctx.dc_iscale >= (1<<FRACBITS)) \
  { \
    d_ctx.dc_source = R_GetColumn(texture, texturecolumn); colfunc(&d_ctx); \
  } \
  else \
  { \
    R_ResampleColumn(&d_ctx, texture, texturecolumn, d_ctx.resample_texfrac); \
    R_DrawResampledColumn(&d_ctx); \
  }

#endif
#endif


/* The inverse division as a macro to make changing it easy */
#ifdef USE_FLOATING_POINT
#define INVERSE_DIVISION(den)	(fixed_t)((double)(0xffffffffu) / ((unsigned)(den)))
#else
#define INVERSE_DIVISION(den)	0xffffffffu / ((unsigned)(den))
#endif



/* Packed all global boolean vars into one flag-integer */
static unsigned int	textureflags;

/* True if any of the segs textures might be visible.*/
#define TFLAG_SEGTEXTURED	1
#define TFLAG_MASKEDTEXTURE	2
/* False if the back side is the same plane.*/
#define TFLAG_MARKFLOOR		64
#define TFLAG_MARKCEILING	128


/* OPTIMIZE: closed two sided lines as single sided*/
static int	toptexture;
static int	bottomtexture;
static int	midtexture;


angle_t		rw_normalangle;
/* angle to line origin*/
int		rw_angle1;


/* regular wall*/

int		rw_x;
int		rw_stopx;
fixed_t		rw_distance;
static angle_t	rw_centerangle;
static fixed_t	rw_offset;
static fixed_t	rw_scale;
static fixed_t	rw_scalestep;
static fixed_t	rw_midtexturemid;
static fixed_t	rw_toptexturemid;
static fixed_t	rw_bottomtexturemid;

static int	worldtop;
static int	worldbottom;
static int	worldhigh;
static int	worldlow;

static fixed_t	pixhigh;
static fixed_t	pixlow;
static fixed_t	pixhighstep;
static fixed_t	pixlowstep;

static fixed_t	topfrac;
static fixed_t	topstep;

static fixed_t	bottomfrac;
static fixed_t	bottomstep;


const lighttable_t**	walllights = NULL;

dshort_t*	maskedtexturecol = NULL;





#ifdef DIYRESAMPLETHINGS
typedef struct twin_segment_s {
  twin_context_t twin;
  int texnum;
} twin_segment_t;

static const column_t *twin_segment_read_column(twin_context_t *ctx, int num)
{
  twin_segment_t *tseg = (twin_segment_t*)ctx;
  return (const column_t*)(((const byte*)R_GetColumn(tseg->texnum, num)) - 3);
}

static int **twinSegments = NULL;

static int *R_EnsureTwinSegment(int number)
{
  if ((number < 0) || (number >= numtextures))
    return NULL;

  if (twinSegments == NULL)
  {
    int i;
    twinSegments = (int**)Z_MallocNoAbort(numtextures*sizeof(int*), PU_STATIC, NULL);
    if (twinSegments == NULL)
      return NULL;
    for (i=0; i<numtextures; i++)
      twinSegments[i] = NULL;
  }
  if (twinSegments[number] == NULL)
  {
    twin_segment_t twinCtx;
    twinCtx.twin.width = R_TextureWidthMask(number) + 1;
    twinCtx.twin.modulo = 1;
    twinCtx.twin.getcolumn = twin_segment_read_column;
    twinCtx.texnum = number;

    /*printf("Make seg %d...\n", number); fflush(stdout);*/
    R_CreateMaskedTwinColumns(&twinCtx.twin, twinSegments + number);
    /*printf("OK.\n"); fflush(stdout);*/
  }
  return twinSegments[number];
}



static fixed_t *segmentFractions = NULL;
static fixed_t *segmentDivTable = NULL;

/*
 *  Try to interpolate halfway usable column fractions from the segment columns
 *  Amazingly enough this works really well ;-).
 */
static void R_CalcSegmentFractions(int from, int to)
{
  if (segmentFractions == NULL)
  {
    segmentFractions =
      (fixed_t*)Z_MallocNoAbort((SCREENWIDTH+16) * sizeof(fixed_t), PU_STATIC, NULL);
    if (segmentFractions == NULL)
      return;
  }
  if (segmentDivTable == NULL)
  {
    int i;
    segmentDivTable =
      (fixed_t*)Z_MallocNoAbort(SCREENWIDTH * sizeof(fixed_t), PU_STATIC, NULL);
    if (segmentDivTable == NULL)
      return;
    segmentDivTable[0] = 0;
    for (i=1; i<SCREENWIDTH; i++)
      segmentDivTable[i] = FRACUNIT / i;
  }

  if (from == to)
  {
    segmentFractions[from] = 0;
  }
  else
  {
    while (from <= to)
    {
      fixed_t frac, step;
      int last = from++;
      while ((from <= to) && (maskedtexturecol[from-1] == maskedtexturecol[from])) from++;
      for (frac=0, step=segmentDivTable[from-last]; last<from; last++, frac+=step)
	segmentFractions[last] = frac;
    }
  }
}
#endif



/* don't emulate transparency in 8bpp modes, sorry. */
#if (defined(DIYBOOM) && (LD_PIXEL_DEPTH == 3))
void R_DrawDummyColumn(draw_context_t *ctx)
{
}
#endif



/* R_RenderMaskedSegRange*/

void
R_RenderMaskedSegRange
( const drawseg_t* ds,
  int		x1,
  int		x2 )
{
    unsigned	index;
    column_t*	col;
    int		lightnum;
    int		texnum;
#ifdef DIYBOOM
    void	(*oldcolfunc)(draw_context_t *);
#endif
#ifdef DIYRESAMPLETHINGS
    twin_draw_t drawTwin;
    int*        twinCols=NULL;
    int         texcolmask;
#endif

    /* Calculate light table.*/
    /* Use different light tables*/
    /*   for horizontal / vertical / diagonal. Diagonal?*/
    /* OPTIMIZE: get rid of LIGHTSEGSHIFT globally*/
    curline = ds->curline;

    frontsector = curline->frontsector;
    backsector = curline->backsector;
    index = curline->sidedef->midtexture;
    texnum = (index < numtextures) ? texturetranslation[index] : index;

    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (curline->v1->y == curline->v2->y)
	lightnum--;
    else if (curline->v1->x == curline->v2->x)
	lightnum++;

    /* changed from original code */
    if (lightnum < 0)
        lightnum = 0;
    else if (lightnum >= LIGHTLEVELS)
        lightnum = LIGHTLEVELS-1;

    walllights = (const lighttable_t**) scalelight[lightnum];

    maskedtexturecol = ds->maskedtexturecol;

    rw_scalestep = ds->scalestep;
    d_ctx.spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
    d_ctx.floorclip = ds->sprbottomclip;
    d_ctx.ceilingclip = ds->sprtopclip;

    /* find positioning*/
    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
	d_ctx.dc_texturemid = frontsector->floorheight > backsector->floorheight
	    ? frontsector->floorheight : backsector->floorheight;
	d_ctx.dc_texturemid = d_ctx.dc_texturemid + textureheight[texnum] - viewz;
    }
    else
    {
	d_ctx.dc_texturemid = frontsector->ceilingheight<backsector->ceilingheight
	    ? frontsector->ceilingheight : backsector->ceilingheight;
	d_ctx.dc_texturemid = d_ctx.dc_texturemid - viewz;
    }
    d_ctx.dc_texturemid += curline->sidedef->rowoffset;

    if (fixedcolormap)
	d_ctx.dc_colormap = fixedcolormap;

#ifdef DIYRESAMPLETHINGS
    /*printf("scale %d\n", ds->scale1);*/
    texcolmask = R_TextureWidthMask(texnum);
    if ((texcolmask != 0) && ((abs(d_ctx.spryscale) > FRACUNIT) || (abs(d_ctx.spryscale + (x2-x1+1)*rw_scalestep) > FRACUNIT)))
    {
      twinCols = R_EnsureTwinSegment(texnum);
      if (twinCols != NULL)
      {
        drawTwin.resamplefunc = R_ResampleThingColumn;
        drawTwin.drawresfunc = R_DrawResampledColumn;
#ifdef DIYBOOM
	if (curline->linedef->tranlump >= 0)
	  drawTwin.drawresfunc = R_DrawResampledTranslucentColumn;
#endif
	R_CalcSegmentFractions(x1, x2);

	if (segmentFractions == NULL)
	  twinCols = NULL;
      }
    }
#endif

#ifdef DIYBOOM
    oldcolfunc = colfunc;
    colfunc = R_DrawColumn;
    if (curline->linedef->tranlump >= 0)
    {
#if (LD_PIXEL_DEPTH == 3)
        colfunc = R_DrawDummyColumn;
#else
        colfunc = R_DrawColumnTranslucent;
#endif
    }
#endif

    /* draw the columns*/
    for (d_ctx.dc_x = x1 ; d_ctx.dc_x <= x2 ; d_ctx.dc_x++)
    {
      /* calculate lighting*/
      if (maskedtexturecol[d_ctx.dc_x] != MAXDSHORT)
      {
        if (!fixedcolormap)
        {
          index = d_ctx.spryscale>>LIGHTSCALESHIFT;
          if (index >=  MAXLIGHTSCALE )
              index = MAXLIGHTSCALE-1;
          d_ctx.dc_colormap = walllights[index];
        }

        d_ctx.sprtopscreen = d_ctx.centeryfrac - FixedMul(d_ctx.dc_texturemid, d_ctx.spryscale);
        d_ctx.dc_iscale = INVERSE_DIVISION(d_ctx.spryscale);

#ifdef DIYRESAMPLETHINGS
        if ((twinCols != NULL) && (d_ctx.dc_iscale < FRACUNIT))
        {
          column_t *c2;
          int colnum = maskedtexturecol[d_ctx.dc_x];

	  /* draw the texture*/
	  col = (column_t *)((byte *)R_GetColumn(texnum, colnum) - 3);
	  c2 = (column_t*)(((byte*)twinCols) + twinCols[colnum & texcolmask]);

	  R_DrawMaskedTwinColumn(&d_ctx, &drawTwin, col, c2, segmentFractions[d_ctx.dc_x]);
        }
        else
#endif
        {
	  /* draw the texture*/
	  col = (column_t *)((byte *)R_GetColumn(texnum,maskedtexturecol[d_ctx.dc_x]) - 3);

	  R_DrawMaskedColumn (&d_ctx, col);
	}
	maskedtexturecol[d_ctx.dc_x] = MAXDSHORT;
      }
      d_ctx.spryscale += rw_scalestep;
    }
#ifdef DIYBOOM
    colfunc = oldcolfunc;
#endif
}





#ifdef DEBUG_PLOTTERS
#ifdef USE_DOUBLE_COLUMNS
#define CALL_COLFUNC(l,h,s)	if (doubleOK) {d_ctx.dc_dbl_yl = l; d_ctx.dc_dbl_yh = h; d_ctx.dc_dbl_source = s; doublecolfunc(&d_ctx);} else {fprintf(logfile, "k"); fflush(logfile); colfunc(&d_ctx); fprintf(logfile, "l"); fflush(logfile);}
#else
#define CALL_COLFUNC(l,h,s)	{fprintf(logfile, "k"); fflush(logfile); colfunc(&d_ctx); fprintf(logfile, "l"); fflush(logfile);}
#endif
#else
#ifdef USE_DOUBLE_COLUMNS
#define CALL_COLFUNC(l,h,s)	if (doubleOK) {d_ctx.dc_dbl_yl = l; d_ctx.dc_dbl_yh = h; d_ctx.dc_dbl_source = s; doublecolfunc(&d_ctx);} else {colfunc(&d_ctx);}
#else
#define CALL_COLFUNC(l,h,s)	colfunc(&d_ctx)
#endif
#endif

/* R_RenderSegLoop*/
/* Draws zero, one, or two textures (and possibly a masked*/
/*  texture) for walls.*/
/* Can draw or mark the starting pixel of floor and ceiling*/
/*  textures.*/
/* CALLED: CORE LOOPING ROUTINE.*/

#define HEIGHTBITS		12
#define HEIGHTUNIT		(1<<HEIGHTBITS)

void R_RenderSegLoop (void)
{
    angle_t		angle;
    unsigned		index;
    int			yl;
    int			yh;
    int			midb=0, midt=0;
    fixed_t		texturecolumn = 0;
    int			top;
    int			bottom;
    int			help;
    int			midtexheight=0, toptexheight=0, bottexheight=0;
#ifdef USE_DOUBLE_COLUMNS
    int			rw_x2;
    /* mid, top and bottom textures are rendered in one go, therefore we must memorize the */
    /* corrdinates and textures for each kind individually. */
    byte*		dbl_midsrc = NULL;
    byte*		dbl_topsrc = NULL;
    byte*		dbl_botsrc = NULL;
    int			dbl_midl=0, dbl_midh=0;
    int			dbl_topl=0, dbl_toph=0;
    int			dbl_botl=0, dbl_both=0;
    boolean		doubleOK, doublePossible;
#endif

    /*texturecolumn = 0;				// shut up compiler warning*/

    if (midtexture)
      midtexheight = textureheight[midtexture] >> FRACBITS;
    else
    {
      if (toptexture)
        toptexheight = textureheight[toptexture] >> FRACBITS;
      if (bottomtexture)
        bottexheight = textureheight[bottomtexture] >> FRACBITS;
    }

#ifdef USE_DOUBLE_COLUMNS
    doublePossible = ((doublecolfunc != NULL) && ((textureflags & TFLAG_MASKEDTEXTURE) == 0));
#ifdef DIYBOOM
    if (midtexture)
      doublePossible = doublePossible && ((midtexheight == 128) || (midtexheight == 0));
    else
    {
      if (toptexture)
        doublePossible = doublePossible && ((toptexheight == 128) || (toptexheight == 0));
      if (bottomtexture)
        doublePossible = doublePossible && ((bottexheight == 128) || (bottexheight == 0));
    }
#endif
#endif

    while (rw_x < rw_stopx)
    {

#ifdef USE_DOUBLE_COLUMNS
	doubleOK = false;
	/* Set up variables for double column plotting if necessary */
	if ((doublePossible) && ((rw_x & 1) == 0) && (rw_stopx - rw_x >= 2))
	{
	    rw_x2 = rw_x + 1;

	    /*
	     *  If any textures (mid / top / bottom) fall out of range we have to revert
	     *  to single column plotting. Therefore in this case doubleOK is set to false.
	     */
	    doubleOK = true;

	    /*
	     *  This is basically a copy of the code below...
	     *  ONLY: if we have to revert to single column plotting we must NOT mark
	     *  anything. Therefore rearrange the whole thing to first see whether
	     *  everything's OK and only then do the marking...
	     */
	    yl = (topfrac+topstep+HEIGHTUNIT-1) >> HEIGHTBITS;

	    if (yl < ceilingclip[rw_x2]+1)
		yl = ceilingclip[rw_x2]+1;

	    yh = (bottomfrac+bottomstep) >> HEIGHTBITS;

	    if (yh >= floorclip[rw_x2])
		yh = floorclip[rw_x2]-1;

	    if (!midtexture)
	    {
		/*
		 *  REALLY nice: this must set ceilingclip because bottomtexture uses
		 *  ceilingclip. Only we can't set ceilingclip, so use another variable.
		 */
		help = ceilingclip[rw_x2];

		if (toptexture)
	    	{
		    dbl_toph = (pixhigh + pixhighstep) >> HEIGHTBITS;

		    if (dbl_toph >= floorclip[rw_x2])
			dbl_toph = floorclip[rw_x2]-1;

		    if (dbl_toph >= yl)
			help = dbl_toph;
		    else
		    {
			help = yl-1;
			doubleOK = false;
		    }
		}
		else
		{
		    if ((textureflags & TFLAG_MARKCEILING) != 0)
			help = yl-1;
		}

		if (bottomtexture)
		{
		    dbl_botl = (pixlow + pixlowstep + HEIGHTUNIT - 1) >> HEIGHTBITS;

		    if (dbl_botl <= help)
			dbl_botl = help+1;

		    if (dbl_botl > yh)
		    {
			doubleOK = false;
		    }
		}
	    }

	    /*
	     *  Now mark stuff IF everything went OK... otherwise stuff will be marked
	     *  on the next single column plot anyway.
	     */
	    if (doubleOK)
	    {
		if ((textureflags & TFLAG_SEGTEXTURED) != 0)
		{
		    angle = (rw_centerangle + xtoviewangle[rw_x2]) >> ANGLETOFINESHIFT;
		    texturecolumn = rw_offset-FixedMul(finetangent[angle], rw_distance);
		    texturecolumn >>= FRACBITS;
		    /* Double columns use same colourmap */
		    d_ctx.dc_dbl_iscale = INVERSE_DIVISION(rw_scale + rw_scalestep);
		}

		if ((textureflags & TFLAG_MARKCEILING) != 0)
		{
		    top = ceilingclip[rw_x2]+1;
		    bottom = yl-1;

		    if (bottom >= floorclip[rw_x2])
			bottom = floorclip[rw_x2]-1;

		    if (top <= bottom)
		    {
			VISPLANE_TOP(ceilingplane)[rw_x2] = top;
			VISPLANE_BOTTOM(ceilingplane)[rw_x2] = bottom;
		    }
		}

		if ((textureflags & TFLAG_MARKFLOOR) != 0)
		{
	            top = yh + 1;
	            bottom = floorclip[rw_x2]-1;

		    if (top <= ceilingclip[rw_x2])
			top = ceilingclip[rw_x2]+1;

		    if (top <= bottom)
		    {
			VISPLANE_TOP(floorplane)[rw_x2] = top;
			VISPLANE_BOTTOM(floorplane)[rw_x2] = bottom;
		    }
		}

		if (midtexture)
		{
		    dbl_midl = yl;
		    dbl_midh = yh;
		    dbl_midsrc = R_GetColumn(midtexture, texturecolumn);
		    ceilingclip[rw_x2] = d_ctx.viewheight;
		    floorclip[rw_x2] = -1;
		}
		else
		{
		    if (toptexture)
		    {
			dbl_topl = yl;
			dbl_topsrc = R_GetColumn(toptexture, texturecolumn);
			ceilingclip[rw_x2] = dbl_toph;
		    }
		    else
		    {
			if ((textureflags & TFLAG_MARKCEILING) != 0)
			    ceilingclip[rw_x2] = yl-1;
		    }

		    if (bottomtexture)
		    {
			dbl_both = yh;
			dbl_botsrc = R_GetColumn(bottomtexture, texturecolumn);
			floorclip[rw_x2] = dbl_botl;
		    }
		    else
		    {
			if ((textureflags & TFLAG_MARKFLOOR) != 0)
			    floorclip[rw_x2] = yh+1;
		    }
		}
	    }
	}
#endif

	/* mark floor / ceiling areas*/
	yl = (topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

	/* no space above wall?*/
	if (yl < ceilingclip[rw_x]+1)
	    yl = ceilingclip[rw_x]+1;

	if ((textureflags & TFLAG_MARKCEILING) != 0)
	{
	    top = ceilingclip[rw_x]+1;
	    bottom = yl-1;

	    if (bottom >= floorclip[rw_x])
		bottom = floorclip[rw_x]-1;

	    if (top <= bottom)
	    {
		VISPLANE_TOP(ceilingplane)[rw_x] = top;
		VISPLANE_BOTTOM(ceilingplane)[rw_x] = bottom;
	    }
	}

	yh = bottomfrac>>HEIGHTBITS;

	if (yh >= floorclip[rw_x])
	    yh = floorclip[rw_x]-1;

	if ((textureflags & TFLAG_MARKFLOOR) != 0)
	{
	    top = yh+1;
	    bottom = floorclip[rw_x]-1;
	    if (top <= ceilingclip[rw_x])
		top = ceilingclip[rw_x]+1;
	    if (top <= bottom)
	    {
		VISPLANE_TOP(floorplane)[rw_x] = top;
		VISPLANE_BOTTOM(floorplane)[rw_x] = bottom;
	    }
	}

	/*
	 *  Move this up here to avoid the division whenever possible.
	 *  help == 0 <==> nothing to do. Means a tiny overhead in simple scenes,
	 *  but should gain substantial speed in complex ones. help is a bitfield.
	 *  bit0: calculate texturecolumn, bit1: colourmap, bit2: dc_iscale.
	 */
#ifdef USE_DOUBLE_COLUMNS
	/* If col2 OK we always have to calculate the colormap */
	help = (doubleOK) ? 2 : 0;
#else
	help = 0;
#endif
	if (midtexture)
	{
	    ceilingclip[rw_x] = d_ctx.viewheight;
	    floorclip[rw_x] = -1;
	    help = 7;
	}
	else
	{
	    if (toptexture)
	    {
		/* top wall*/
		midt = pixhigh >> HEIGHTBITS;
#ifdef USE_DOUBLE_COLUMNS
		pixhigh += (doubleOK) ? 2*pixhighstep : pixhighstep;
#else
		pixhigh += pixhighstep;
#endif
		if (midt >= floorclip[rw_x])
		    midt = floorclip[rw_x]-1;

		if (midt >= yl)
		{
		    ceilingclip[rw_x] = midt;
		    help = 7;
		}
		else
		    ceilingclip[rw_x] = yl-1;
	    }
	    else
	    {
		if ((textureflags & TFLAG_MARKCEILING) != 0)
		    ceilingclip[rw_x] = yl-1;
	    }

	    if (bottomtexture)
	    {
		/* bottom wall*/
		midb = (pixlow+HEIGHTUNIT-1) >> HEIGHTBITS;
#ifdef USE_DOUBLE_COLUMNS
		pixlow += (doubleOK) ? 2*pixlowstep : pixlowstep;
#else
		pixlow += pixlowstep;
#endif
		/* no space above wall?*/
		if (midb <= ceilingclip[rw_x])
		    midb = ceilingclip[rw_x]+1;

		if (midb <= yh)
		{
		    floorclip[rw_x] = midb;
		    help = 7;
		}
		else
		    floorclip[rw_x] = yh+1;
	    }
	    else
	    {
	        if ((textureflags & TFLAG_MARKFLOOR) != 0)
		    floorclip[rw_x] = yh+1;
	    }

	    if ((textureflags & TFLAG_MASKEDTEXTURE) != 0)
	    {
		help |= 1;
	    }
	}

	if (help != 0)
	{
	    /* texturecolumn and lighting are independent of wall tiers*/
	    if ((textureflags & TFLAG_SEGTEXTURED) != 0)
	    {
		d_ctx.dc_x = rw_x;
		if ((help & 1) != 0)
		{
		    /* calculate texture offset*/
		    angle = (rw_centerangle + xtoviewangle[rw_x])>>ANGLETOFINESHIFT;
		    texturecolumn = rw_offset-FixedMul(finetangent[angle],rw_distance);
#ifdef DIYRESAMPLE
		    d_ctx.resample_texfrac = texturecolumn & ((1<<FRACBITS)-1);
#endif
		    texturecolumn >>= FRACBITS;
		}
		if ((help & 2) != 0)
		{
		    /* calculate lighting*/
		    index = rw_scale>>LIGHTSCALESHIFT;

		    if (index >=  MAXLIGHTSCALE )
			index = MAXLIGHTSCALE-1;

		    d_ctx.dc_colormap = walllights[index];
		}
		if ((help & 4) != 0)
		{
		    d_ctx.dc_iscale = INVERSE_DIVISION(rw_scale);
		}
	    }

	    /* draw the wall tiers*/
	    if (midtexture)
	    {
		/* single sided line*/
		d_ctx.dc_yl = yl;
		d_ctx.dc_yh = yh;
		d_ctx.dc_texturemid = rw_midtexturemid;
		d_ctx.dc_texheight = midtexheight;
#ifdef DIYRESAMPLE
		CALL_RESAMPLE_COLUMN(midtexture);
#else
		d_ctx.dc_source = R_GetColumn(midtexture,texturecolumn);
		CALL_COLFUNC(dbl_midl, dbl_midh, dbl_midsrc);
#endif
	    }
	    else
	    {
		/* two sided line*/
		if (toptexture)
		{
		    if (midt >= yl)
		    {
			d_ctx.dc_yl = yl;
			d_ctx.dc_yh = midt;
			d_ctx.dc_texturemid = rw_toptexturemid;
			d_ctx.dc_texheight = toptexheight;
#ifdef DIYRESAMPLE
			CALL_RESAMPLE_COLUMN(toptexture);
#else
			d_ctx.dc_source = R_GetColumn(toptexture,texturecolumn);
			CALL_COLFUNC(dbl_topl, dbl_toph, dbl_topsrc);
#endif
		    }
#ifdef USE_DOUBLE_COLUMNS
		    /* Make sure that col2 is plotted if col2 is legal but col1 isn't */
		    else if (doubleOK)
		    {
			d_ctx.dc_yl = dbl_topl;
			d_ctx.dc_yh = dbl_toph;
			d_ctx.dc_texturemid = rw_toptexturemid;
			d_ctx.dc_source = dbl_topsrc;
			help = d_ctx.dc_iscale; d_ctx.dc_iscale = d_ctx.dc_dbl_iscale; d_ctx.dc_x++;
#ifdef DEBUG_PLOTTERS
                        fprintf(logfile, "k"); fflush(logfile);
#endif
			colfunc(&d_ctx);
#ifdef DEBUG_PLOTTERS
                        fprintf(logfile, "l"); fflush(logfile);
#endif
			d_ctx.dc_iscale = help; d_ctx.dc_x--;
		    }
#endif
		}

		if (bottomtexture)
		{
		    if (midb <= yh)
		    {
			d_ctx.dc_yl = midb;
			d_ctx.dc_yh = yh;
			d_ctx.dc_texturemid = rw_bottomtexturemid;
			d_ctx.dc_texheight = bottexheight;
#ifdef DIYRESAMPLE
			CALL_RESAMPLE_COLUMN(bottomtexture);
#else
			d_ctx.dc_source = R_GetColumn(bottomtexture,texturecolumn);
			CALL_COLFUNC(dbl_botl, dbl_both, dbl_botsrc);
#endif
		    }
#ifdef USE_DOUBLE_COLUMNS
		    else if (doubleOK)
		    {
			d_ctx.dc_yl = dbl_botl;
			d_ctx.dc_yh = dbl_both;
			d_ctx.dc_texturemid = rw_bottomtexturemid;
			d_ctx.dc_source = dbl_botsrc;
			help = d_ctx.dc_iscale; d_ctx.dc_iscale = d_ctx.dc_dbl_iscale; d_ctx.dc_x++;
#ifdef DEBUG_PLOTTERS
			fprintf(logfile, "k"); fflush(logfile);
#endif
			colfunc(&d_ctx);
#ifdef DEBUG_PLOTTERS
                        fprintf(logfile, "l"); fflush(logfile);
#endif
			d_ctx.dc_iscale = help; d_ctx.dc_x--;
		    }
#endif
		}
	    }

	    if ((textureflags & TFLAG_MASKEDTEXTURE) != 0)
	    {
		/* save texturecol*/
		/*  for backdrawing of masked mid texture*/
		maskedtexturecol[rw_x] = texturecolumn;
	    }
	}
#ifdef USE_DOUBLE_COLUMNS
	if (doubleOK)
	{
	    rw_scale += 2*rw_scalestep;
	    topfrac += 2*topstep;
	    bottomfrac += 2*bottomstep;
	    rw_x += 2;
	}
	else
	{
#endif
	rw_scale += rw_scalestep;
	topfrac += topstep;
	bottomfrac += bottomstep;
	rw_x++;
#ifdef USE_DOUBLE_COLUMNS
	}
#endif
    }
}





/* Increment the number of drawsegs. */
/* returns NULL if all OK, otherwise a pointer to the last valid drawseg. */
static drawseg_t *
R_IncrementDrawsegNumber(void)
{
    int		size;
    int		wantsegs;
    drawseg_t*	memory;

    wantsegs = numdrawsegs + DEFDRAWSEGS;
    if (maximum_drawsegs)
    {
        if (numdrawsegs >= maximum_drawsegs) return (drawsegs + (numdrawsegs-1));
        if (wantsegs > maximum_drawsegs) wantsegs = maximum_drawsegs;
    }
    size = wantsegs * sizeof(drawseg_t);

    if ((memory = (drawseg_t*)Z_MallocNoAbort(size, PU_STATIC, NULL)) == NULL)
        return (drawsegs + (numdrawsegs-1));

    memcpy(memory, drawsegs, numdrawsegs * sizeof(drawseg_t));
    memset(memory + numdrawsegs, 0, (wantsegs - numdrawsegs) * sizeof(drawseg_t));

    /* Don't forget to change all global drawseg pointers */
    ds_p = memory + (ds_p - drawsegs);
    Z_Free(drawsegs);
    drawsegs = memory;
    numdrawsegs = wantsegs;
    fprintf(logfile, "R_IncrementDrawsegNumber: %d drawsegs, %d bytes\n", numdrawsegs, size);

    return NULL;
}



/* R_StoreWallRange*/
/* A wall segment will be drawn*/
/*  between start and stop pixels (inclusive).*/

void
R_StoreWallRange
( int	start,
  int	stop )
{
    fixed_t		hyp;
    fixed_t		sineval;
    angle_t		distangle, offsetangle;
    fixed_t		vtop;
    int			lightnum;

    /* don't overflow and crash */
    if (ds_p - drawsegs >= numdrawsegs)
    {
        if (R_IncrementDrawsegNumber() != NULL) return;
    }

#ifdef RANGECHECK
    if (start >= d_ctx.viewwidth || start > stop)
    {
#ifdef RANGECHECK_ABORTS
	I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#else
	/*fprintf(stderr, "R_RenderWallRange: %i to %i\n", start, stop);*/
	return;
#endif
    }
#endif

    textureflags = 0;

    sidedef = curline->sidedef;
    linedef = curline->linedef;

    /* mark the segment as visible for auto map*/
    linedef->flags |= ML_MAPPED;

    /* calculate rw_distance for scale calculation*/
    rw_normalangle = curline->angle + ANG90;
    offsetangle = abs(rw_normalangle-rw_angle1);

    if (offsetangle > ANG90)
	offsetangle = ANG90;

    distangle = ANG90 - offsetangle;
#ifdef DIYBOOM
    hyp = (viewx==curline->v1->x && viewy==curline->v1->y)
         ? 0 : R_PointToDist (curline->v1->x, curline->v1->y);
#else
    hyp = R_PointToDist (curline->v1->x, curline->v1->y);
#endif
    sineval = finesine[distangle>>ANGLETOFINESHIFT];
    rw_distance = FixedMul (hyp, sineval);


    ds_p->x1 = rw_x = start;
    ds_p->x2 = stop;
    ds_p->curline = curline;
    rw_stopx = stop+1;

    /* calculate scale at both ends and step*/
    ds_p->scale1 = rw_scale =
	R_ScaleFromGlobalAngle (viewangle + xtoviewangle[start]);

    if (stop > start )
    {
	ds_p->scale2 = R_ScaleFromGlobalAngle (viewangle + xtoviewangle[stop]);
	ds_p->scalestep = rw_scalestep =
	    (ds_p->scale2 - rw_scale) / (stop-start);
    }
    else
    {
	/* UNUSED: try to fix the stretched line bug*/
#if 0
	if (rw_distance < FRACUNIT/2)
	{
	    fixed_t		trx,try;
	    fixed_t		gxt,gyt;

	    trx = curline->v1->x - viewx;
	    try = curline->v1->y - viewy;

	    gxt = FixedMul(trx,viewcos);
	    gyt = -FixedMul(try,viewsin);
	    ds_p->scale1 = FixedDiv(d_ctx.projection, gxt-gyt)<<detailshift;
	}
#endif
	ds_p->scale2 = ds_p->scale1;
    }

    /* calculate texture boundaries*/
    /*  and decide if floor / ceiling marks are needed*/
    worldtop = frontsector->ceilingheight - viewz;
    worldbottom = frontsector->floorheight - viewz;

    midtexture = toptexture = bottomtexture = 0;
    ds_p->maskedtexturecol = NULL;

    if (!backsector)
    {
	/* single sided line*/
	midtexture = (sidedef->midtexture < numtextures) ? texturetranslation[sidedef->midtexture] : sidedef->midtexture;
	/* a single sided line is terminal, so it must mark ends*/
	textureflags |= (TFLAG_MARKFLOOR | TFLAG_MARKCEILING);
	if (linedef->flags & ML_DONTPEGBOTTOM)
	{
	    vtop = frontsector->floorheight +
		textureheight[sidedef->midtexture];
	    /* bottom of texture at bottom*/
	    rw_midtexturemid = vtop - viewz;
	}
	else
	{
	    /* top of texture at top*/
	    rw_midtexturemid = worldtop;
	}
	rw_midtexturemid += sidedef->rowoffset;
#if 0
#ifdef DIYBOOM
       {      /* killough 3/27/98: reduce offset */
         fixed_t h;
         h = textureheight[sidedef->midtexture];
         if (h & (h-FRACUNIT))
           rw_midtexturemid %= h;
       }
#endif
#endif
	ds_p->silhouette = SIL_BOTH;
	ds_p->sprtopclip = screenheightarray;
	ds_p->sprbottomclip = negonearray;
	ds_p->bsilheight = MAXINT;
	ds_p->tsilheight = MININT;
    }
    else
    {
	/* two sided line*/
	ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
	ds_p->silhouette = 0;

	if (frontsector->floorheight > backsector->floorheight)
	{
	    ds_p->silhouette = SIL_BOTTOM;
	    ds_p->bsilheight = frontsector->floorheight;
	}
	else if (backsector->floorheight > viewz)
	{
	    ds_p->silhouette = SIL_BOTTOM;
	    ds_p->bsilheight = MAXINT;
	    /* ds_p->sprbottomclip = negonearray;*/
	}

	if (frontsector->ceilingheight < backsector->ceilingheight)
	{
	    ds_p->silhouette |= SIL_TOP;
	    ds_p->tsilheight = frontsector->ceilingheight;
	}
	else if (backsector->ceilingheight < viewz)
	{
	    ds_p->silhouette |= SIL_TOP;
	    ds_p->tsilheight = MININT;
	    /* ds_p->sprtopclip = screenheightarray;*/
	}

	{
#ifdef DIYBOOM
          extern int doorclosed; /* killough 1/17/98, 2/8/98, 4/7/98 */
# define DOORCLOSED doorclosed
#else
# define DOORCLOSED 0
#endif
          if (DOORCLOSED || backsector->ceilingheight <= frontsector->floorheight)
          {
	    ds_p->sprbottomclip = negonearray;
	    ds_p->bsilheight = MAXINT;
	    ds_p->silhouette |= SIL_BOTTOM;
	  }

	  if (DOORCLOSED || backsector->floorheight >= frontsector->ceilingheight)
	  {
	    ds_p->sprtopclip = screenheightarray;
	    ds_p->tsilheight = MININT;
	    ds_p->silhouette |= SIL_TOP;
	  }
        }

	worldhigh = backsector->ceilingheight - viewz;
	worldlow = backsector->floorheight - viewz;

	/* hack to allow height changes in outdoor areas*/
	if (frontsector->ceilingpic == skyflatnum
	    && backsector->ceilingpic == skyflatnum)
	{
	    worldtop = worldhigh;
	}


	if (worldlow != worldbottom
	    || backsector->floorpic != frontsector->floorpic
	    || backsector->lightlevel != frontsector->lightlevel
#ifdef DIYBOOM
            /* killough 3/7/98: Add checks for (x,y) offsets */
            || backsector->floor_xoffs != frontsector->floor_xoffs
            || backsector->floor_yoffs != frontsector->floor_yoffs
            /* killough 4/15/98: prevent 2s normals */
            /* from bleeding through deep water */
            || frontsector->heightsec != -1
            /* killough 4/17/98: draw floors if different light levels */
            || backsector->floorlightsec != frontsector->floorlightsec
#endif
           )
	{
	    textureflags |= TFLAG_MARKFLOOR;
	}
	else
	{
	    /* same plane on both sides*/
	    textureflags &= ~TFLAG_MARKFLOOR;
	}


	if (worldhigh != worldtop
	    || backsector->ceilingpic != frontsector->ceilingpic
	    || backsector->lightlevel != frontsector->lightlevel
#ifdef DIYBOOM
            || backsector->lightlevel != frontsector->lightlevel
            /* killough 3/7/98: Add checks for (x,y) offsets */
            || backsector->ceiling_xoffs != frontsector->ceiling_xoffs
            || backsector->ceiling_yoffs != frontsector->ceiling_yoffs
            /* killough 4/15/98: prevent 2s normals */
            /* from bleeding through fake ceilings */
            || (frontsector->heightsec != -1 &&
                frontsector->ceilingpic!=skyflatnum)
            /* killough 4/17/98: draw ceilings if different light levels */
            || backsector->ceilinglightsec != frontsector->ceilinglightsec
#endif
          )
	{
	    textureflags |= TFLAG_MARKCEILING;
	}
	else
	{
	    /* same plane on both sides*/
	    textureflags &= ~TFLAG_MARKCEILING;
	}

	if (backsector->ceilingheight <= frontsector->floorheight
	    || backsector->floorheight >= frontsector->ceilingheight)
	{
	    /* closed door*/
	    textureflags |= (TFLAG_MARKFLOOR | TFLAG_MARKCEILING);
	}


	if (worldhigh < worldtop)
	{
	    /* top texture*/
	    toptexture = (sidedef->toptexture < numtextures) ? texturetranslation[sidedef->toptexture] : sidedef->toptexture;
	    if (linedef->flags & ML_DONTPEGTOP)
	    {
		/* top of texture at top*/
		rw_toptexturemid = worldtop;
	    }
	    else
	    {
		vtop =
		    backsector->ceilingheight
		    + textureheight[sidedef->toptexture];

		/* bottom of texture*/
		rw_toptexturemid = vtop - viewz;
	    }
	}
	if (worldlow > worldbottom)
	{
	    /* bottom texture*/
	    bottomtexture = (sidedef->bottomtexture < numtextures) ? texturetranslation[sidedef->bottomtexture] : sidedef->bottomtexture;

	    if (linedef->flags & ML_DONTPEGBOTTOM )
	    {
		/* bottom of texture at bottom*/
		/* top of texture at top*/
		rw_bottomtexturemid = worldtop;
	    }
	    else	/* top of texture at top*/
		rw_bottomtexturemid = worldlow;
	}
	rw_toptexturemid += sidedef->rowoffset;
#if 0
#ifdef DIYBOOM
        /* killough 3/27/98: reduce offset */
        {
          fixed_t h;
          h = textureheight[sidedef->toptexture];
          if (h & (h-FRACUNIT))
            rw_toptexturemid %= h;
        }
#endif
#endif
	rw_bottomtexturemid += sidedef->rowoffset;
#if 0
#ifdef DIYBOOM
       /* killough 3/27/98: reduce offset */
       {
         fixed_t h;
         h = textureheight[sidedef->bottomtexture];
         if (h & (h-FRACUNIT))
           rw_bottomtexturemid %= h;
       }
#endif
#endif
	/* allocate space for masked texture tables*/
	if (sidedef->midtexture)
	{
	    /* masked midtexture*/
	    textureflags |= TFLAG_MASKEDTEXTURE;
	    ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
	    lastopening += rw_stopx - rw_x;
	}
    }

    /* calculate rw_offset (only needed for textured lines)*/
    if ((midtexture | toptexture | bottomtexture | (textureflags & TFLAG_MASKEDTEXTURE)) != 0)
    {
	textureflags |= TFLAG_SEGTEXTURED;

	offsetangle = rw_normalangle-rw_angle1;

	if (offsetangle > ANG180)
	    offsetangle = -offsetangle;

	if (offsetangle > ANG90)
	    offsetangle = ANG90;

	sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
	rw_offset = FixedMul (hyp, sineval);

	if (rw_normalangle-rw_angle1 < ANG180)
	    rw_offset = -rw_offset;

	rw_offset += sidedef->textureoffset + curline->offset;
	rw_centerangle = ANG90 + viewangle - rw_normalangle;

	/* calculate light table*/
	/*  use different light tables*/
	/*  for horizontal / vertical / diagonal*/
	/* OPTIMIZE: get rid of LIGHTSEGSHIFT globally*/
	if (!fixedcolormap)
	{
	    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;

	    if (curline->v1->y == curline->v2->y)
		lightnum--;
	    else if (curline->v1->x == curline->v2->x)
		lightnum++;

	    if (lightnum < 0)
	        lightnum = 0;
	    else if (lightnum >= LIGHTLEVELS)
	        lightnum = LIGHTLEVELS-1;

	    walllights = (const lighttable_t**) scalelight[lightnum];
	}
    }

    /* if a floor / ceiling plane is on the wrong side*/
    /*  of the view plane, it is definitely invisible*/
    /*  and doesn't need to be marked.*/

#ifdef DIYBOOM
    if (frontsector->heightsec == -1)
    {
      if (frontsector->floorheight >= viewz)
	/* above view plane*/
	textureflags &= ~TFLAG_MARKFLOOR;

      if (frontsector->ceilingheight <= viewz
         && frontsector->ceilingpic != skyflatnum)
	/* below view plane*/
	textureflags &= ~TFLAG_MARKCEILING;
    }
#else
    if (frontsector->floorheight >= viewz)
    {
        /* above view plane*/
        textureflags &= ~TFLAG_MARKFLOOR;
    }

    if (frontsector->ceilingheight <= viewz
        && frontsector->ceilingpic != skyflatnum)
    {
        /* below view plane*/
        textureflags &= ~TFLAG_MARKCEILING;
    }
#endif


    /* calculate incremental stepping values for texture edges*/
    worldtop >>= 4;
    worldbottom >>= 4;

    topstep = -FixedMul (rw_scalestep, worldtop);
    topfrac = (d_ctx.centeryfrac>>4) - FixedMul (worldtop, rw_scale);

    bottomstep = -FixedMul (rw_scalestep,worldbottom);
    bottomfrac = (d_ctx.centeryfrac>>4) - FixedMul (worldbottom, rw_scale);

    if (backsector)
    {
	worldhigh >>= 4;
	worldlow >>= 4;

	if (worldhigh < worldtop)
	{
	    pixhigh = (d_ctx.centeryfrac>>4) - FixedMul (worldhigh, rw_scale);
	    pixhighstep = -FixedMul (rw_scalestep,worldhigh);
	}

	if (worldlow > worldbottom)
	{
	    pixlow = (d_ctx.centeryfrac>>4) - FixedMul (worldlow, rw_scale);
	    pixlowstep = -FixedMul (rw_scalestep,worldlow);
	}
    }

    /* render it*/
    if ((textureflags & TFLAG_MARKCEILING) != 0)
    {
      if (ceilingplane)        /* killough 4/11/98: add NULL ptr checks */
        ceilingplane = R_CheckPlane (ceilingplane, rw_x, rw_stopx-1);
      else
        textureflags &= ~TFLAG_MARKCEILING;
    }

    if ((textureflags & TFLAG_MARKFLOOR) != 0)
    {
      if (floorplane)  /* killough 4/11/98: add NULL ptr checks */
        floorplane = R_CheckPlane (floorplane, rw_x, rw_stopx-1);
      else
        textureflags &= ~TFLAG_MARKFLOOR;
    }

    R_RenderSegLoop ();


    /* save sprite clipping info*/
    if ( ((ds_p->silhouette & SIL_TOP) || ((textureflags & TFLAG_MASKEDTEXTURE) != 0))
	 && !ds_p->sprtopclip)
    {
	memcpy (lastopening, ceilingclip+start, (rw_stopx-start)*sizeof(dshort_t));
	ds_p->sprtopclip = lastopening - start;
	lastopening += rw_stopx - start;
    }

    if ( ((ds_p->silhouette & SIL_BOTTOM) || ((textureflags & TFLAG_MASKEDTEXTURE) != 0))
	 && !ds_p->sprbottomclip)
    {
	memcpy (lastopening, floorclip+start, (rw_stopx-start)*sizeof(dshort_t));
	ds_p->sprbottomclip = lastopening - start;
	lastopening += rw_stopx - start;
    }

    if (((textureflags & TFLAG_MASKEDTEXTURE) != 0) && !(ds_p->silhouette&SIL_TOP))
    {
	ds_p->silhouette |= SIL_TOP;
	ds_p->tsilheight = MININT;
    }
    if (((textureflags & TFLAG_MASKEDTEXTURE) != 0) && !(ds_p->silhouette&SIL_BOTTOM))
    {
	ds_p->silhouette |= SIL_BOTTOM;
	ds_p->bsilheight = MAXINT;
    }
    ds_p++;
}

