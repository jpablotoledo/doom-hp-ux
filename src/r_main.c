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
/*	Rendering main loop and setup functions,*/
/*	 utility functions (BSP, geometry, trigonometry).*/
/*	See tables.c, too.*/

/*-----------------------------------------------------------------------------*/


static const char rcsid[] = "$Id: r_main.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";



#include <stdlib.h>
#include <math.h>

#ifdef __riscos__
#include "ROsupport.h"
#endif

#include "doomstat.h"
#include "doomdef.h"
#include "d_net.h"

#include "m_bbox.h"
#include "m_menu.h"

#include "r_local.h"
#include "r_sky.h"
#include "r_context.h"

#include "p_pspr.h"
#include "i_video.h"
#include "p_spec.h"
#include "z_zone.h"

#ifndef DIYINLINE
#include "inl_pside.c"
#include "inl_psub.c"
#endif





/* Fineangles in the SCREENWIDTH wide window.*/
#define FIELDOFVIEW		2048



int			viewangleoffset;

/* increment every time a check is made*/
int			validcount = 1;


const lighttable_t*	fixedcolormap;

/* just for profiling purposes*/
int			framecount;

int			sscount;
int			linecount;
int			loopcount;

fixed_t			viewx;
fixed_t			viewy;
fixed_t			viewz;

angle_t			viewangle;

fixed_t			viewcos;
fixed_t			viewsin;

player_t*		viewplayer = NULL;


/* precalculated math tables*/

angle_t			clipangle = 0;

/* The viewangletox[viewangle + FINEANGLES/4] lookup*/
/* maps the visible view angles to screen X coordinates,*/
/* flattening the arc to a flat projection plane.*/
/* There will be many angles mapped to the same X. */
int			viewangletox[FINEANGLES/2];

/* The xtoviewangleangle[] table maps a screen pixel*/
/* to the lowest viewangle that maps back to x ranges*/
/* from clipangle to -clipangle.*/
#ifdef STATIC_RESOLUTION
angle_t			xtoviewangle[SCREENWIDTH+1];
#else
angle_t			xtoviewangle[MAXSCREENWIDTH+1];
#endif


/* UNUSED.*/
/* The finetangentgent[angle+FINEANGLES/4] table*/
/* holds the fixed_t tangent values for view angles,*/
/* ranging from MININT to 0 to MAXINT.*/
/* fixed_t		finetangent[FINEANGLES/2];*/

/* fixed_t		finesine[5*FINEANGLES/4];*/
const fixed_t*		finecosine = &finesine[FINEANGLES/4];


const lighttable_t*	scalelightfixed[MAXLIGHTSCALE];
#if (defined(DIYBOOM) && (LD_PIXEL_DEPTH == 3))
static lighttable_t*	(*c_scalelight)[LIGHTLEVELS][MAXLIGHTSCALE];
static lighttable_t*	(*c_zlight)[LIGHTLEVELS][MAXLIGHTZ];
lighttable_t*		(*scalelight)[MAXLIGHTSCALE];
lighttable_t*		(*zlight)[MAXLIGHTZ];
#else
lighttable_t*		scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t*		zlight[LIGHTLEVELS][MAXLIGHTZ];
#endif

/* bumped light from gun blasts*/
int			extralight;



void (*colfunc) (draw_context_t *);
void (*basecolfunc) (draw_context_t *);
void (*fuzzcolfunc) (draw_context_t *);
void (*transcolfunc) (draw_context_t *);
void (*spanfunc) (draw_context_t *);
#if (defined(__riscos__) && (LD_PIXEL_DEPTH == 4))
void (*doublecolfunc) (draw_context_t *);
#endif




/* R_AddPointToBox*/
/* Expand a given bbox*/
/* so that it encloses a given point.*/

void
R_AddPointToBox
( int		x,
  int		y,
  fixed_t*	box )
{
    if (x< box[BOXLEFT])
	box[BOXLEFT] = x;
    if (x> box[BOXRIGHT])
	box[BOXRIGHT] = x;
    if (y< box[BOXBOTTOM])
	box[BOXBOTTOM] = y;
    if (y> box[BOXTOP])
	box[BOXTOP] = y;
}



int
R_PointOnSegSide
( fixed_t	x,
  fixed_t	y,
  const seg_t*	line )
{
    fixed_t	lx;
    fixed_t	ly;
    fixed_t	ldx;
    fixed_t	ldy;
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;

    lx = line->v1->x;
    ly = line->v1->y;

    ldx = line->v2->x - lx;
    ldy = line->v2->y - ly;

    if (!ldx)
    {
	if (x <= lx)
	    return ldy > 0;

	return ldy < 0;
    }
    if (!ldy)
    {
	if (y <= ly)
	    return ldx < 0;

	return ldx > 0;
    }

    dx = (x - lx);
    dy = (y - ly);

    /* Try to quickly decide by looking at sign bits.*/
    if ( (ldy ^ ldx ^ dx ^ dy)&0x80000000 )
    {
	if  ( (ldy ^ dx) & 0x80000000 )
	{
	    /* (left is negative)*/
	    return 1;
	}
	return 0;
    }

    left = FixedMul ( ldy>>FRACBITS , dx );
    right = FixedMul ( dy , ldx>>FRACBITS );

    if (right < left)
    {
	/* front side*/
	return 0;
    }
    /* back side*/
    return 1;
}



/* R_PointToAngle*/
/* To get a global angle from cartesian coordinates,*/
/*  the coordinates are flipped until they are in*/
/*  the first octant of the coordinate system, then*/
/*  the y (<=x) is scaled and divided by x to get a*/
/*  tangent (slope) value which is looked up in the*/
/*  tantoangle[] table.*/






angle_t
R_PointToAngle
( fixed_t	x,
  fixed_t	y )
{
    x -= viewx;
    y -= viewy;

    if ( (!x) && (!y) )
	return 0;

    if (x>= 0)
    {
	/* x >=0*/
	if (y>= 0)
	{
	    /* y>= 0*/

	    if (x>y)
	    {
		/* octant 0*/
		return tantoangle[ SlopeDiv(y,x)];
	    }
	    else
	    {
		/* octant 1*/
		return ANG90-1-tantoangle[ SlopeDiv(x,y)];
	    }
	}
	else
	{
	    /* y<0*/
	    y = -y;

	    if (x>y)
	    {
		/* octant 8*/
		return -tantoangle[SlopeDiv(y,x)];
	    }
	    else
	    {
		/* octant 7*/
		return ANG270+tantoangle[ SlopeDiv(x,y)];
	    }
	}
    }
    else
    {
	/* x<0*/
	x = -x;

	if (y>= 0)
	{
	    /* y>= 0*/
	    if (x>y)
	    {
		/* octant 3*/
		return ANG180-1-tantoangle[ SlopeDiv(y,x)];
	    }
	    else
	    {
		/* octant 2*/
		return ANG90+ tantoangle[ SlopeDiv(x,y)];
	    }
	}
	else
	{
	    /* y<0*/
	    y = -y;

	    if (x>y)
	    {
		/* octant 4*/
		return ANG180+tantoangle[ SlopeDiv(y,x)];
	    }
	    else
	    {
		 /* octant 5*/
		return ANG270-1-tantoangle[ SlopeDiv(x,y)];
	    }
	}
    }
    return 0;
}


angle_t
R_PointToAngle2
( fixed_t	x1,
  fixed_t	y1,
  fixed_t	x2,
  fixed_t	y2 )
{
    viewx = x1;
    viewy = y1;

    return R_PointToAngle (x2, y2);
}


fixed_t
R_PointToDist
( fixed_t	x,
  fixed_t	y )
{
    int		angle;
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	temp;
    fixed_t	dist;

    dx = abs(x - viewx);
    dy = abs(y - viewy);

    if (dy>dx)
    {
	temp = dx;
	dx = dy;
	dy = temp;
    }

    angle = (tantoangle[ FixedDiv(dy,dx)>>DBITS ]+ANG90) >> ANGLETOFINESHIFT;

    /* use as cosine*/
    dist = FixedDiv (dx, finesine[angle] );

    return dist;
}





/* R_InitPointToAngle*/

static void R_InitPointToAngle (void)
{
    /* UNUSED - now getting from tables.c*/
#if 0
    int	i;
    long	t;
    float	f;

/* slope (tangent) to angle lookup*/

    for (i=0 ; i<=SLOPERANGE ; i++)
    {
	f = atan( (float)i/SLOPERANGE )/(3.141592657*2);
	t = 0xffffffff*f;
	tantoangle[i] = t;
    }
#endif
}



/* R_ScaleFromGlobalAngle*/
/* Returns the texture mapping scale*/
/*  for the current line (horizontal span)*/
/*  at the given angle.*/
/* rw_distance must be calculated first.*/

fixed_t R_ScaleFromGlobalAngle (angle_t visangle)
{
    fixed_t		scale;
    int			anglea;
    int			angleb;
    int			sinea;
    int			sineb;
    fixed_t		num;
    int			den;

    /* UNUSED*/
#if 0
{
    fixed_t		dist;
    fixed_t		z;
    fixed_t		sinv;
    fixed_t		cosv;

    sinv = finesine[(visangle-rw_normalangle)>>ANGLETOFINESHIFT];
    dist = FixedDiv (rw_distance, sinv);
    cosv = finecosine[(viewangle-visangle)>>ANGLETOFINESHIFT];
    z = abs(FixedMul (dist, cosv));
    scale = FixedDiv(d_ctx.projection, z);
    return scale;
}
#endif

    anglea = ANG90 + (visangle-viewangle);
    angleb = ANG90 + (visangle-rw_normalangle);

    /* both sines are allways positive*/
    sinea = finesine[anglea>>ANGLETOFINESHIFT];
    sineb = finesine[angleb>>ANGLETOFINESHIFT];
    num = FixedMul(d_ctx.projection,sineb) << d_ctx.detailshift;
    den = FixedMul(rw_distance,sinea);

    if (den > num>>16)
    {
	scale = FixedDiv (num, den);

	if (scale > 64*FRACUNIT)
	    scale = 64*FRACUNIT;
	else if (scale < 256)
	    scale = 256;
    }
    else
	scale = 64*FRACUNIT;

    return scale;
}




/* R_InitTables*/

static void R_InitTables (void)
{
    /* UNUSED: now getting from tables.c*/
#if 0
    int		i;
    float	a;
    float	fv;
    int		t;

    /* viewangle tangent table*/
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
	a = (i-FINEANGLES/4+0.5)*PI*2/FINEANGLES;
	fv = FRACUNIT*tan (a);
	t = fv;
	finetangent[i] = t;
    }

    /* finesine table*/
    for (i=0 ; i<5*FINEANGLES/4 ; i++)
    {
	/* OPTIMIZE: mirror...*/
	a = (i+0.5)*PI*2/FINEANGLES;
	t = FRACUNIT*sin (a);
	finesine[i] = t;
    }
#endif
}




/* R_InitTextureMapping*/

static void R_InitTextureMapping (void)
{
    int			i;
    int			x;
    int			t;
    fixed_t		focallength;

    /* Use tangent table to generate viewangletox:*/
    /*  viewangletox will give the next greatest x*/
    /*  after the view angle.*/

    /* Calc focallength*/
    /*  so FIELDOFVIEW angles covers SCREENWIDTH.*/
    focallength = FixedDiv (d_ctx.centerxfrac,
			    finetangent[FINEANGLES/4+FIELDOFVIEW/2] );

    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
	if (finetangent[i] > FRACUNIT*2)
	    t = -1;
	else if (finetangent[i] < -FRACUNIT*2)
	    t = d_ctx.viewwidth+1;
	else
	{
	    t = FixedMul (finetangent[i], focallength);
	    t = (d_ctx.centerxfrac - t+FRACUNIT-1)>>FRACBITS;

	    if (t < -1)
		t = -1;
	    else if (t>d_ctx.viewwidth+1)
		t = d_ctx.viewwidth+1;
	}
	viewangletox[i] = t;
    }

    /* Scan viewangletox[] to generate xtoviewangle[]:*/
    /*  xtoviewangle will give the smallest view angle*/
    /*  that maps to x.	*/
    for (x=0;x<=d_ctx.viewwidth;x++)
    {
	i = 0;
	while (viewangletox[i]>x)
	    i++;
	xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }

    /* Take out the fencepost cases from viewangletox.*/
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
	t = FixedMul (finetangent[i], focallength);
	t = d_ctx.centerx - t;

	if (viewangletox[i] == -1)
	    viewangletox[i] = 0;
	else if (viewangletox[i] == d_ctx.viewwidth+1)
	    viewangletox[i]  = d_ctx.viewwidth;
    }

    clipangle = xtoviewangle[0];
}




/* R_InitLightTables*/
/* Only inits the zlight table,*/
/*  because the scalelight table changes with view size.*/

#define DISTMAP		2

static void R_InitLightTables (void)
{
    int		i;
    int		j;
    int		level;
    int		startmap;
    int		scale;

#if (defined(DIYBOOM) && (LD_PIXEL_DEPTH == 3))
    int		t;

    c_zlight = Z_Malloc(sizeof(*c_zlight) * numcolormaps, PU_STATIC, NULL);
    c_scalelight = Z_Malloc(sizeof(*c_scalelight) * numcolormaps, PU_STATIC, NULL);
#endif

    /* Calculate the light levels to use*/
    /*  for each level / distance combination.*/
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
	startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
	for (j=0 ; j<MAXLIGHTZ ; j++)
	{
	    scale = FixedDiv ((SCREENWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
	    scale >>= LIGHTSCALESHIFT;
	    /* If NUMCOLORMAPS gets bigger we also have to adapt this factor */
	    level = startmap - (scale*NUMCOLORMAPS)/(32*DISTMAP);

	    if (level < 0)
		level = 0;

	    if (level >= NUMCOLORMAPS)
		level = NUMCOLORMAPS-1;
#if (defined(DIYBOOM) && (LD_PIXEL_DEPTH == 3))
            for (t=0; t<numcolormaps; t++)
                c_zlight[t][i][j] = colormaps[t] + level*256;
#else
# if (LD_PIXEL_DEPTH == 3)
	    zlight[i][j] = colormaps + level*256;
# else
	    zlight[i][j] = translated_colourmaps + level*256;
# endif
#endif
	}
    }
}




/* R_SetViewSize*/
/* Do not really change anything here,*/
/*  because it might be in the middle of a refresh.*/
/* The change will take effect next refresh.*/

boolean		setsizeneeded;
static int	setblocks;
static int	setdetail;


void
R_SetViewSize
( int		blocks,
  int		detail )
{
    setsizeneeded = true;
    setblocks = blocks;
    setdetail = detail;
}



/* R_ExecuteSetViewSize*/

void R_ExecuteSetViewSize (void)
{
    fixed_t	cosadj;
    fixed_t	dy;
    int		i;
    int		j;
    int		level;
    int		startmap;

    setsizeneeded = false;

    if (setblocks == 11)
    {
	d_ctx.scaledviewwidth = SCREENWIDTH;
	d_ctx.viewheight = SCREENHEIGHT;
    }
    else
    {
	d_ctx.scaledviewwidth = ((setblocks*SCREENWIDTH)/10)&~7;
	d_ctx.viewheight = ((setblocks*(SCREENHEIGHT-32))/10)&~7;
    }

    d_ctx.detailshift = setdetail;
    d_ctx.viewwidth = d_ctx.scaledviewwidth >> d_ctx.detailshift;

    d_ctx.centery = d_ctx.viewheight/2;
    d_ctx.centerx = d_ctx.viewwidth/2;
    d_ctx.centerxfrac = d_ctx.centerx<<FRACBITS;
    d_ctx.centeryfrac = d_ctx.centery<<FRACBITS;
    d_ctx.projection = d_ctx.centerxfrac;

    if (!d_ctx.detailshift)
    {
	colfunc = basecolfunc = R_DrawColumn;
	fuzzcolfunc = R_DrawFuzzColumn;
	transcolfunc = R_DrawTranslatedColumn;
	spanfunc = R_DrawSpan;
#if (defined(__riscos__) && (LD_PIXEL_DEPTH == 4))
# ifdef DIYRESAMPLE
        doublecolfunc = NULL;
# else
	doublecolfunc = R_DrawDoubleColumn;
# endif
#endif
    }
    else
    {
	colfunc = basecolfunc = R_DrawColumnLow;
	fuzzcolfunc = R_DrawFuzzColumn;
	transcolfunc = R_DrawTranslatedColumn;
	spanfunc = R_DrawSpanLow;
#if (defined(__riscos__) && (LD_PIXEL_DEPTH == 4))
	doublecolfunc = NULL;
#endif
    }

    R_InitBuffer (d_ctx.scaledviewwidth, d_ctx.viewheight);

    R_InitTextureMapping ();

    /* psprite scales*/
    /*pspritescale = (FRACUNIT*d_ctx.viewwidth)/SCREENWIDTH;*/
    /*pspriteiscale = (FRACUNIT*SCREENWIDTH)/d_ctx.viewwidth;*/

    /* Try to fix for different aspect ratios, otherwise the sky looks really weird */
    /* in non-320:200 modes. Uncomment the above lines and remove the following */
    /* 10 lines to get the original behaviour. */
    pspritescale = (FRACUNIT*d_ctx.viewwidth)/320;
    pspriteiscale = (FRACUNIT*d_ctx.viewheight)/200;
    if (pspritescale >= pspriteiscale)
    {
	pspriteiscale = (FRACUNIT*320)/d_ctx.viewwidth;
    }
    else
    {
	pspritescale = pspriteiscale;
	pspriteiscale = (FRACUNIT*200)/d_ctx.viewheight;
    }

    /* thing clipping*/
    for (i=0 ; i<d_ctx.viewwidth ; i++)
	screenheightarray[i] = d_ctx.viewheight;

    /* planes*/
    for (i=0 ; i<d_ctx.viewheight ; i++)
    {
	dy = ((i-d_ctx.viewheight/2)<<FRACBITS)+FRACUNIT/2;
	dy = abs(dy);
	yslope[i] = FixedDiv ( (d_ctx.viewwidth << d_ctx.detailshift)/2*FRACUNIT, dy);
    }

    for (i=0 ; i<d_ctx.viewwidth ; i++)
    {
	cosadj = abs(finecosine[xtoviewangle[i]>>ANGLETOFINESHIFT]);
	distscale[i] = FixedDiv (FRACUNIT,cosadj);
    }

    /* Calculate the light levels to use*/
    /*  for each level / scale combination.*/
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
	startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
	for (j=0 ; j<MAXLIGHTSCALE ; j++)
	{
#if (defined(DIYBOOM) && (LD_PIXEL_DEPTH == 3))
	    int		t;
#endif

	    /* Also take a higher number of colourmaps into account here */
	    level = startmap - (j*SCREENWIDTH*NUMCOLORMAPS)/(32*DISTMAP*(d_ctx.viewwidth << d_ctx.detailshift));

	    if (level < 0)
		level = 0;

	    if (level >= NUMCOLORMAPS)
		level = NUMCOLORMAPS-1;

#if (defined(DIYBOOM) && (LD_PIXEL_DEPTH == 3))
            for (t=0; t<numcolormaps; t++)
                c_scalelight[t][i][j] = colormaps[t] + level * 256;
#else
# if (LD_PIXEL_DEPTH == 3)
	    scalelight[i][j] = colormaps + level*256;
# else
	    scalelight[i][j] = translated_colourmaps + level*256;
# endif
#endif
	}
    }

    /* Set the weapon position */
    P_SetPosPsprites(d_ctx.detailshift);
}




/* R_Init*/


void R_Init (void)
{
    R_InitData ();
    printf ("\nR_InitData");
    R_InitPointToAngle ();
    printf ("\nR_InitPointToAngle");
    R_InitTables ();
    /* viewwidth / viewheight / detailLevel are set by the defaults*/
    printf ("\nR_InitTables");

    R_SetViewSize (screenblocks, detailLevel);
    R_InitPlanes ();
    printf ("\nR_InitPlanes");
    R_InitLightTables ();
    printf ("\nR_InitLightTables");
    R_InitSkyMap ();
    printf ("\nR_InitSkyMap");
    R_InitTranslationTables ();
    printf ("\nR_InitTranslationsTables");

    framecount = 0;
}




#ifdef DIYBOOM
static int last_colourmap = 0;
#endif

/* R_SetupFrame*/

static void R_SetupFrame (player_t* player)
{
    int		i;
    BOOMSTATEMENT(int cm=0;)

    viewplayer = player;
    viewx = player->mo->x;
    viewy = player->mo->y;
    viewangle = player->mo->angle + viewangleoffset;
    /* Gotta scale that light up according to the number of lightlevels, too... */
    extralight = (player->extralight) << (4 - LIGHTSEGSHIFT);

    viewz = player->viewz;

    viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
    viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

#ifdef DIYBOOM
    if (player->mo->subsector->sector->heightsec != -1)
    {
        const sector_t *s = player->mo->subsector->sector->heightsec + sectors;
        cm = (viewz < s->floorheight) ? s->bottommap : (viewz > s->ceilingheight) ? s->topmap : s->midmap;
        if ((cm < 0) || (cm >= numcolormaps))
          cm = 0;
    }
    fullcolormap = colormaps[cm];
#if (LD_PIXEL_DEPTH == 3)
    zlight = c_zlight[cm];
    scalelight = c_scalelight[cm];
#endif
    if (cm != last_colourmap)
    {
        last_colourmap = cm;
        I_SetPalette(NULL);
    }
#endif

    sscount = 0;

    if (player->fixedcolormap)
    {
	fixedcolormap =
#if (LD_PIXEL_DEPTH == 3)
#ifdef DIYBOOM
            fullcolormap
#else
	    colormaps
#endif
#else
	    translated_colourmaps
#endif
	    + player->fixedcolormap*256;

	walllights = scalelightfixed;

	for (i=0 ; i<MAXLIGHTSCALE ; i++)
	    scalelightfixed[i] = fixedcolormap;
    }
    else
	fixedcolormap = 0;

    framecount++;
    validcount++;
#ifdef __riscos__
    R_UpdateBuffer(d_ctx.viewheight);
#endif
}



/* R_RenderView*/

#ifdef DEBUG
static int SB_gametics = 0;
#endif

void R_RenderPlayerView (player_t* player)
{
#ifdef DEBUG
    fprintf(logfile, "("); fflush(logfile);
#endif

    R_SetupFrame (player);

    /* Clear buffers.*/
    R_ClearClipSegs ();
    R_ClearDrawSegs ();
    R_ClearPlanes ();
    R_ClearSprites ();

    /* check for new console commands.*/
    NetUpdate ();

    /* The head node is the last node output.*/
    R_RenderBSPNode (numnodes-1);

    /* Check for new console commands.*/
    NetUpdate ();

    R_DrawPlanes ();

    /* Check for new console commands.*/
    NetUpdate ();

    R_DrawMasked ();

    /* Check for new console commands.*/
    NetUpdate ();

#ifdef DEBUG
    fprintf(logfile, ")"); fflush(logfile);

    if (gametic - SB_gametics > 4200)
    {
      fclose(logfile); logfile = fopen("Doom:log", "w");
      SB_gametics = gametic;
    }
#endif
}
