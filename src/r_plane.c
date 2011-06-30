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
/*	Here is a core component: drawing the floors and ceilings,*/
/*	 while maintaining a per column clipping list only.*/
/*	Moreover, the sky areas have to be determined.*/

/*-----------------------------------------------------------------------------*/


static const char
rcsid[] = "$Id: r_plane.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";

#include <stdlib.h>

#ifdef __riscos__
#include "ROsupport.h"
#include "GameSupp.h"
#endif
#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"
#include "r_context.h"




/*static planefunction_t	floorfunc;
static planefunction_t	ceilingfunc;*/



/* Internal macros */

/* check for visplane overflow */
#define VISPLANE_OVER(p) \
  (((char*)p) >= (((char*)visplanes) + numvisplanes * VISPLANE_SIZE))

/* difference between two visplane pointers */
#define VISPLANE_DIFF(l, h) \
  ((((char*)h) - ((char*)l)) / VISPLANE_SIZE)




/* Default number of visplanes (actual number can be extended dynamically) */
#define DEFVISPLANES	64
static visplane_head_t*	visplanes = NULL;
static int		numvisplanes = 0;
static visplane_head_t*	lastvisplane = NULL;
visplane_head_t*	floorplane = NULL;
visplane_head_t*	ceilingplane = NULL;

/* ?*/
#ifdef STATIC_RESOLUTION
#define MAXOPENINGS	SCREENWIDTH*64
static dshort_t		openings[MAXOPENINGS];
#else
static int MAXOPENINGS;
static dshort_t		*openings = NULL;
#endif
dshort_t*		lastopening = NULL;



/* Clip values are the solid pixel bounding the range.*/
/*  floorclip starts out SCREENHEIGHT*/
/*  ceilingclip starts out -1*/

#ifdef STATIC_RESOLUTION
dshort_t		floorclip[SCREENWIDTH];
dshort_t		ceilingclip[SCREENWIDTH];

fixed_t			yslope[SCREENHEIGHT];
fixed_t			distscale[SCREENWIDTH];

static fixed_t		cachedheight[SCREENHEIGHT];
static fixed_t		cacheddistance[SCREENHEIGHT];
static fixed_t		cachedxstep[SCREENHEIGHT];
static fixed_t		cachedystep[SCREENHEIGHT];

/* spanstart holds the start of a plane span*/
/* initialized to 0 at start*/

static int		spanstart[SCREENHEIGHT];
/*static int		spanstop[SCREENHEIGHT];*/

#else

dshort_t		floorclip[MAXSCREENWIDTH];
dshort_t		ceilingclip[MAXSCREENWIDTH];

fixed_t			yslope[MAXSCREENHEIGHT];
fixed_t			distscale[MAXSCREENWIDTH];

static fixed_t		cachedheight[MAXSCREENHEIGHT];
static fixed_t		cacheddistance[MAXSCREENHEIGHT];
static fixed_t		cachedxstep[MAXSCREENHEIGHT];
static fixed_t		cachedystep[MAXSCREENHEIGHT];
BOOMSTATEMENT(fixed_t xoffs;)
BOOMSTATEMENT(fixed_t yoffs;)

static int		spanstart[MAXSCREENHEIGHT];
/*static int		spanstop[MAXSCREENHEIGHT];*/
#endif


/* texture mapping*/

static lighttable_t**	planezlight;
static fixed_t		planeheight;

static fixed_t		basexscale;
static fixed_t		baseyscale;




static void R_InitVisplaneArray(vistype_t *array, int width)
{
#ifdef __riscos__
  if (sizeof(vistype_t) == 1)
    memset(array, VPLANE_MAX_VALUE, width);
  else if (sizeof(vistype_t) == 2)
    GameSupp_FillMemory16(array, VPLANE_MAX_VALUE, width*2);
  else if (sizeof(vistype_t) == 4)
    GameSupp_FillMemory32(array, VPLANE_MAX_VALUE, width*4);
#else
  int i;

  for (i=0; i<width; i++)
  {
    array[i] = VPLANE_MAX_VALUE;
  }
#endif
}


/* R_InitPlanes*/
/* Only at game startup.*/

void R_InitPlanes (void)
{
    int size;

    /* Init visplanes */
    numvisplanes = default_visplanes;
    size = numvisplanes * VISPLANE_SIZE;
    if (visplanes != NULL) Z_Free(visplanes);
    if ((visplanes = Z_MallocNoAbort(size, PU_STATIC, NULL)) == NULL)
    {
        I_Error("R_InitPlanes: can't claim memory for visplanes!");
    }
    memset(visplanes, 0, size);
    fprintf(logfile, "R_InitPlanes: claimed %d visplanes, %d bytes\n", numvisplanes, size);

    /* Init drawsegs (see r.bsp) */
    numdrawsegs = default_drawsegs;
    size = numdrawsegs * sizeof(drawseg_t);
    if (drawsegs != NULL) Z_Free(drawsegs);
    if ((drawsegs = Z_MallocNoAbort(size, PU_STATIC, NULL)) == NULL)
    {
        I_Error("R_InitPlanes: can't claim memory for drawsegs!");
    }
    memset(drawsegs, 0, size);
    fprintf(logfile, "R_InitPlanes: claimed %d drawsegs, %d bytes\n", numdrawsegs, size);
}



/* R_MapPlane*/

/* Uses global vars:*/
/*  planeheight*/
/*  ds_source*/
/*  basexscale*/
/*  baseyscale*/
/*  viewx*/
/*  viewy*/

/* BASIC PRIMITIVE*/

void
R_MapPlane
( int		y,
  int		x1,
  int		x2 )
{
    angle_t	angle;
    fixed_t	distance;
    fixed_t	length;
    unsigned	index;

#ifdef RANGECHECK
    if (x2 < x1
	|| x1<0
	|| x2>=d_ctx.viewwidth
	|| (unsigned)y>=d_ctx.viewheight)
    {
#ifdef RANGECHECK_ABORTS
	I_Error ("R_MapPlane: %i, %i at %i",x1,x2,y);
#else
        /*fprintf(logfile, "R_MapPlane: %i, %i at %i", x1,x2,y);*/
	return;
#endif
    }
#endif

    if (planeheight != cachedheight[y])
    {
	cachedheight[y] = planeheight;
	distance = cacheddistance[y] = FixedMul (planeheight, yslope[y]);
	d_ctx.ds_xstep = cachedxstep[y] = FixedMul (distance,basexscale);
	d_ctx.ds_ystep = cachedystep[y] = FixedMul (distance,baseyscale);
    }
    else
    {
	distance = cacheddistance[y];
	d_ctx.ds_xstep = cachedxstep[y];
	d_ctx.ds_ystep = cachedystep[y];
    }

    length = FixedMul (distance,distscale[x1]);
    angle = (viewangle + xtoviewangle[x1])>>ANGLETOFINESHIFT;
    d_ctx.ds_xfrac = viewx + FixedMul(finecosine[angle], length) BOOMSTATEMENT(+ xoffs);
    d_ctx.ds_yfrac = -viewy - FixedMul(finesine[angle], length) BOOMSTATEMENT(+ yoffs);

    if (fixedcolormap)
	d_ctx.ds_colormap = fixedcolormap;
    else
    {
	index = distance >> LIGHTZSHIFT;

	if (index >= MAXLIGHTZ )
	    index = MAXLIGHTZ-1;

	d_ctx.ds_colormap = planezlight[index];
    }

    d_ctx.ds_y = y;
    d_ctx.ds_x1 = x1;
    d_ctx.ds_x2 = x2;

#ifdef DEBUG_PLOTTERS
    fprintf(logfile, "i"); fflush(logfile);
#endif
    /* high or low detail*/
    spanfunc (&d_ctx);
#ifdef DEBUG_PLOTTERS
    fprintf(logfile, "j"); fflush(logfile);
#endif
}



/* R_ClearPlanes*/
/* At begining of frame.*/

void R_ClearPlanes (void)
{
    int		i;
    angle_t	angle;

    /* opening / clipping determination*/
    for (i=0 ; i<d_ctx.viewwidth ; i++)
    {
	floorclip[i] = d_ctx.viewheight;
	ceilingclip[i] = -1;      /* don't use VPLANE_MAX_VALUE */
    }

    lastvisplane = visplanes;
    lastopening = openings;

    /* texture calculation*/
    memset (cachedheight, 0, sizeof(cachedheight));

    /* left to right mapping*/
    angle = (viewangle-ANG90)>>ANGLETOFINESHIFT;

    /* scale will be unit scale at SCREENWIDTH/2 distance*/
    basexscale = FixedDiv (finecosine[angle],d_ctx.centerxfrac);
    baseyscale = -FixedDiv (finesine[angle],d_ctx.centerxfrac);
}





/* Dynamic change of the number of visplanes available. */
/* Returns NULL if all OK, a pointer to the last visplane available otherwise */
static visplane_head_t *
R_IncrementVisplaneNumber(void)
{
    int		size, oldsize;
    int		wantplanes;
    char*	memory;
    visplane_head_t*	failed_value;

    failed_value = (visplane_head_t*)(((char*)visplanes) + (numvisplanes-1)*VISPLANE_SIZE);
    wantplanes = numvisplanes + DEFVISPLANES;
    if (maximum_visplanes)
    {
        if (numvisplanes >= maximum_visplanes) return failed_value;
        if (wantplanes > maximum_visplanes) wantplanes = maximum_visplanes;
    }
    size = wantplanes * VISPLANE_SIZE;

    if ((memory = (char*)Z_MallocNoAbort(size, PU_STATIC, NULL)) == NULL)
        return failed_value;

    oldsize = numvisplanes * VISPLANE_SIZE;
    memcpy(memory, visplanes, oldsize);
    memset(memory + oldsize, 0, size - oldsize);

    /* Don't forget to change all global visplane pointers! */
    lastvisplane = (visplane_head_t*)(memory + (((char*)lastvisplane) - ((char*)visplanes)));
    floorplane = (visplane_head_t*)(memory + (((char*)floorplane) - ((char*)visplanes)));
    ceilingplane = (visplane_head_t*)(memory + (((char*)ceilingplane) - ((char*)visplanes)));
    Z_Free(visplanes);
    visplanes = (visplane_head_t*)memory;
    numvisplanes = wantplanes;
    fprintf(logfile, "R_IncrementVisplaneNumber: %d visplanes, %d bytes\n", numvisplanes, size);

    return NULL;	/* All OK */
}



/* R_FindPlane*/

visplane_head_t*
R_FindPlane
( fixed_t	height,
  int		picnum,
  int		lightlevel
#ifdef DIYBOOM
, fixed_t	xoffs,
  fixed_t	yoffs
#endif
)
{
    visplane_head_t*	check;

    if (picnum == skyflatnum)
    {
	height = 0;			/* all skys map together*/
	lightlevel = 0;
    }

    for (check=visplanes; check<lastvisplane; VISPLANE_PLUS(check))
    {
	if (height == check->height
	    && picnum == check->picnum
	    && lightlevel == check->lightlevel
	    BOOMSTATEMENT(&& xoffs == check->xoffs)
	    BOOMSTATEMENT(&& yoffs == check->yoffs) )
	{
	    break;
	}
    }


    if (check < lastvisplane)
	return check;

    if (VISPLANE_OVER(lastvisplane))
    {
        visplane_head_t *last;

        if ((last = R_IncrementVisplaneNumber()) != NULL)
        {
#ifdef RANGECHECK_ABORTS
	    I_Error ("R_FindPlane: no more visplanes");
#else
            if ((maximum_visplanes == 0) || (numvisplanes < maximum_visplanes))
                fprintf(logfile, "R_FindPlane: no more visplanes\n");
            return last;
        }
#endif
    }

    check = lastvisplane;
    VISPLANE_PLUS(lastvisplane);

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = SCREENWIDTH;
    check->maxx = -1;
    BOOMSTATEMENT(check->xoffs = xoffs;)
    BOOMSTATEMENT(check->yoffs = yoffs;)

    R_InitVisplaneArray(VISPLANE_TOP(check), SCREENWIDTH);

    return check;
}



/* R_CheckPlane*/

visplane_head_t*
R_CheckPlane
( visplane_head_t*	pl,
  int		start,
  int		stop )
{
    int		intrl;
    int		intrh;
    int		unionl;
    int		unionh;
    int		x;
    visplane_head_t aux;

    if (start < pl->minx)
    {
	intrl = pl->minx;
	unionl = start;
    }
    else
    {
	unionl = pl->minx;
	intrl = start;
    }

    if (stop > pl->maxx)
    {
	intrh = pl->maxx;
	unionh = stop;
    }
    else
    {
	unionh = pl->maxx;
	intrh = stop;
    }

    for (x=intrl ; x<= intrh ; x++)
	if (VISPLANE_TOP(pl)[x] != VPLANE_MAX_VALUE)
	    break;

    if (x > intrh)
    {
	pl->minx = unionl;
	pl->maxx = unionh;

	/* use the same one*/
	return pl;
    }

    /* In case of overflow just return this one, even though it might look bad. */
    if (VISPLANE_OVER(lastvisplane))
    {
        visplane_head_t *last;

	aux.height = pl->height;
	aux.picnum = pl->picnum;
	aux.lightlevel = pl->lightlevel;
        BOOMSTATEMENT(aux.xoffs = pl->xoffs;)
	BOOMSTATEMENT(aux.yoffs = pl->yoffs;)
        if ((last = R_IncrementVisplaneNumber()) != NULL)
            return pl;

	pl = &aux;
    }

    /* make a new visplane*/
    lastvisplane->height = pl->height;
    lastvisplane->picnum = pl->picnum;
    lastvisplane->lightlevel = pl->lightlevel;
    BOOMSTATEMENT(lastvisplane->xoffs = pl->xoffs;)
    BOOMSTATEMENT(lastvisplane->yoffs = pl->yoffs;)

    pl = lastvisplane;
    VISPLANE_PLUS(lastvisplane);
    pl->minx = start;
    pl->maxx = stop;

    R_InitVisplaneArray(VISPLANE_TOP(pl), SCREENWIDTH);

    return pl;
}



#define SPAN_RANGE(x) if ((x < 0) || (x >= MAXSCREENHEIGHT)) {fprintf(logfile, "OVERFLOW %d\n", x); fflush(logfile);}

/* R_MakeSpans*/

void
R_MakeSpans
( int		x,
  int		t1,
  int		b1,
  int		t2,
  int		b2 )
{
    while (t1 < t2 && t1<=b1)
    {
	R_MapPlane (t1,spanstart[t1],x-1);
	t1++;
    }
    while (b1 > b2 && b1>=t1)
    {
	R_MapPlane (b1,spanstart[b1],x-1);
	b1--;
    }

    while (t2 < t1 && t2<=b2)
    {
	spanstart[t2] = x;
	t2++;
    }
    while (b2 > b1 && b2>=t2)
    {
	spanstart[b2] = x;
	b2--;
    }
}




/* R_DrawPlanes*/
/* At the end of each frame.*/

void R_DrawPlanes (void)
{
    visplane_head_t*	pl;
    int			light;
    int			x;
    int			stop;
    int			angle;

#ifdef RANGECHECK
#ifdef RANGECHECK_ABORTS
    if (ds_p - drawsegs >= numdrawsegs)
	I_Error ("R_DrawPlanes: drawsegs overflow (%i)",
		 ds_p - drawsegs);

    if (VISPLANE_OVER(lastvisplane))
	I_Error ("R_DrawPlanes: visplane overflow (%i)",
		 VISPLANE_DIFF(visplanes, lastvisplane));

    if (lastopening - openings >= MAXOPENINGS)
	I_Error ("R_DrawPlanes: opening overflow (%i)",
		 lastopening - openings);
#else
    if (ds_p - drawsegs >= numdrawsegs)
    {
        if ((maximum_drawsegs == 0) || (numdrawsegs < maximum_drawsegs))
	    fprintf(logfile, "R_DrawPlanes: drawsegs overflow (%i)\n", ds_p - drawsegs);
	return;
    }
    if (VISPLANE_OVER(lastvisplane))
    {
        if ((maximum_visplanes == 0) || (numvisplanes < maximum_visplanes))
	    fprintf(logfile, "R_DrawPlanes: visplane overflow (%i)\n", VISPLANE_DIFF(visplanes, lastvisplane));
	return;
    }
    if (lastopening - openings >= MAXOPENINGS)
    {
	fprintf(logfile, "R_DrawPlanes: opening overflow (%i)\n", lastopening - openings);
	return;
    }
#endif
#endif

    for (pl = visplanes ; pl < lastvisplane ; VISPLANE_PLUS(pl))
    {
	if (pl->minx > pl->maxx)
	    continue;


	/* sky flat*/
	if (pl->picnum == skyflatnum)
	{
	    d_ctx.dc_iscale = pspriteiscale>>d_ctx.detailshift;
#if (defined(__riscos__) && (LD_PIXEL_DEPTH == 4))
	    d_ctx.dc_dbl_iscale = d_ctx.dc_iscale;
#endif
	    /* Sky is allways drawn full bright,*/
	    /*  i.e. colormaps[0] is used.*/
	    /* Because of this hack, sky is not affected*/
	    /*  by INVUL inverse mapping.*/
#if (LD_PIXEL_DEPTH == 3)
#ifdef DIYBOOM
            d_ctx.dc_colormap = fullcolormap;
#else
	    d_ctx.dc_colormap = colormaps;
#endif
#else
	    d_ctx.dc_colormap = translated_colourmaps;
#endif
            d_ctx.dc_texheight = textureheight[skytexture] >> FRACBITS;
	    d_ctx.dc_texturemid = skytexturemid;
	    x = pl->minx;

	    while (x <= pl->maxx)
	    {
		d_ctx.dc_yl = VISPLANE_TOP(pl)[x];
		d_ctx.dc_yh = VISPLANE_BOTTOM(pl)[x];

		if (d_ctx.dc_yl <= d_ctx.dc_yh)
		{
		    d_ctx.dc_x = x;
#ifdef DIYRESAMPLE
                    if (d_ctx.dc_iscale < (1<<FRACBITS))
		    {
                      angle = (viewangle + xtoviewangle[x]) >> (ANGLETOSKYSHIFT - FRACBITS);
                      R_ResampleColumn(&d_ctx, skytexture, angle >> FRACBITS, angle & ((1<<FRACBITS)-1));
#ifdef DEBUG_PLOTTERS
                      fprintf(logfile, "g"); fflush(logfile);
#endif
		      R_DrawResampledColumn(&d_ctx);
#ifdef DEBUG_PLOTTERS
                      fprintf(logfile, "h"); fflush(logfile);
#endif
		    }
		    else
		    {
		      angle = (viewangle + xtoviewangle[x])>>ANGLETOSKYSHIFT;
		      d_ctx.dc_source = R_GetColumn(skytexture, angle);
#ifdef DEBUG_PLOTTERS
                      fprintf(logfile, "k"); fflush(logfile);
#endif
		      colfunc(&d_ctx);
#ifdef DEBUG_PLOTTERS
                      fprintf(logfile, "l"); fflush(logfile);
#endif
		    }
#else
		    angle = (viewangle + xtoviewangle[x])>>ANGLETOSKYSHIFT;
		    d_ctx.dc_source = R_GetColumn(skytexture, angle);

#if (defined(__riscos__) && (LD_PIXEL_DEPTH == 4))
		    /* pl->maxx is _inclusive_! Thus we have >= 2 cols if pl->maxx-x >= 1 */
		    if ((doublecolfunc != NULL) && ((x&1) == 0) && (pl->maxx - x >= 1))
		    {
			d_ctx.dc_dbl_yl = VISPLANE_TOP(pl)[x+1];
			d_ctx.dc_dbl_yh = VISPLANE_BOTTOM(pl)[x+1];

			if (d_ctx.dc_dbl_yl <= d_ctx.dc_dbl_yh)
			{
			    x++;
			    angle = (viewangle + xtoviewangle[x]) >> ANGLETOSKYSHIFT;
			    d_ctx.dc_dbl_source = R_GetColumn(skytexture, angle);

			    doublecolfunc(&d_ctx);
			}
			else
			{
			    colfunc(&d_ctx);
			}
		    }
		    else
#endif
                    {
#ifdef DEBUG_PLOTTERS
                        fprintf(logfile, "k"); fflush(logfile);
#endif
		        colfunc (&d_ctx);
#ifdef DEBUG_PLOTTERS
                        fprintf(logfile, "l"); fflush(logfile);
#endif
                    }
#endif
		}
		x++;
	    }
	    continue;
	}

	/* regular flat*/
	x = (pl->picnum < numflats) ? flattranslation[pl->picnum] : pl->picnum;
	d_ctx.ds_source = W_CacheLumpNum(firstflat + x,
				   PU_STATIC);

        BOOMSTATEMENT(xoffs = pl->xoffs;)
	BOOMSTATEMENT(yoffs = pl->yoffs;)
	planeheight = abs(pl->height-viewz);
	light = (pl->lightlevel >> LIGHTSEGSHIFT)+extralight;

	if (light >= LIGHTLEVELS)
	    light = LIGHTLEVELS-1;

	if (light < 0)
	    light = 0;

	planezlight = zlight[light];

	VISPLANE_TOP(pl)[pl->maxx+1] = VPLANE_MAX_VALUE;
	VISPLANE_TOP(pl)[pl->minx-1] = VPLANE_MAX_VALUE;

	stop = pl->maxx + 1;
	for (x=pl->minx ; x<= stop ; x++)
	{
	    R_MakeSpans(x,VISPLANE_TOP(pl)[x-1],
			VISPLANE_BOTTOM(pl)[x-1],
			VISPLANE_TOP(pl)[x],
			VISPLANE_BOTTOM(pl)[x]);
	}

	Z_ChangeTag (d_ctx.ds_source, PU_CACHE);
    }
}


#ifndef STATIC_RESOLUTION
void R_PlaneInitForResolution(void)
{
    MAXOPENINGS = 64*SCREENWIDTH;
    if (openings != NULL) free(openings);
    if ((openings = (dshort_t*)malloc(MAXOPENINGS*sizeof(dshort_t))) == NULL)
    {
        I_Error("Can't claim memory for openings!");
    }
}
#endif
