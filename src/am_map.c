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

/* DESCRIPTION:  the automap code*/

/*-----------------------------------------------------------------------------*/

static const char rcsid[] = "$Id: am_map.c,v 1.4 1997/02/03 21:24:33 b1 Exp $";

#include <stdio.h>


#ifdef __riscos__
#include "ROsupport.h"
#include "GameSupp.h"
#endif
#include "z_zone.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"
#include "g_game.h"
#include "d_main.h"

#include "m_cheat.h"
#include "i_system.h"

/* Needs access to LFB.*/
#include "v_video.h"
#include "i_video.h"

/* State.*/
#include "doomstat.h"
#include "r_state.h"
#include "r_context.h"

/* Data.*/
#include "dstrings.h"

#include "am_map.h"


#define mapcolor_back 247 /* map background */
#define mapcolor_grid 104 /* grid lines colour */
#define mapcolor_wall 181 /* normal 1s wall colour (23) */
#define mapcolor_fchg  64 /* line at floor height change colour */
#define mapcolor_cchg 162 /* line at ceiling height change colour */
#define mapcolor_clsd 152 /* line at sector with floor=ceiling colour */
#define mapcolor_rkey 175 /* red key colour */
#define mapcolor_bkey 204 /* blue key colour */
#define mapcolor_ykey 231 /* yellow key colour */
#define mapcolor_rdor 175 /* red door colour  (diff from keys to allow option) */
#define mapcolor_bdor 204 /* blue door colour (of enabling one but not other ) */
#define mapcolor_ydor 231 /* yellow door colour */
#define mapcolor_tele 119 /* teleporter line colour */
#define mapcolor_secr 252 /* secret sector boundary colour */
#define mapcolor_exit	 0 /* jff 4/23/98 add exit line colour */
#define mapcolor_unsn 104 /* computer map unseen line colour */
#define mapcolor_flat  88 /* line with no floor/ceiling changes */
/* Boom defaults for the above are:
 * 247, 104, 23, 55, 215, 208, (175, 204, 231)*2, 119, 252, 0, 104, 88
 */
#define mapcolor_sprt 112 /* general sprite colour */
#define mapcolor_hair 208 /* crosshair colour */
#define mapcolor_sngl 208 /* single player arrow colour */
const unsigned char mapcolor_plyr[4] = { 112, 88, 64, 176 };
  /* colours for player arrows in multiplayer */

/* Extra automap colours */
#define mapcolor_monster 216
#define mapcolor_ammo	  200
#define mapcolor_body	  190
#define mapcolor_dead	  191
#define mapcolor_bonus	  64
#define mapcolor_special 250
#define mapcolor_pillar  76
#define mapcolor_tree	  76
#define mapcolor_light	  249
#define mapcolor_blood	  189
#define mapcolor_skull	  188


/* drawing stuff*/
#define	FB		0

#define AM_NUMMARKPOINTS 10

/* scale on entry*/
#define INITSCALEMTOF (.2*FRACUNIT)
/* how much the automap moves window per tic in frame-buffer coordinates*/
/* moves 140 pixels in 1 second*/
#define F_PANINC	4
/* how much zoom-in per tic*/
/* goes to 2x in 1 second*/
#define M_ZOOMIN	((int) (1.02*FRACUNIT))
/* how much zoom-out per tic*/
/* pulls out to 0.5x in 1 second*/
#define M_ZOOMOUT	((int) (FRACUNIT/1.02))

#define MM(v) ((v)[(int)mixmap])

/* translates between frame-buffer and map distances*/
#define FTOM(x) FixedMul(((x)<<16),MM(scale_ftom))
#define MTOF(x) (FixedMul((x),MM(scale_mtof))>>16)
/* translates between frame-buffer and map coordinates*/
#define CXMTOF(x)  (f_x + MTOF((x)-MM(m_x)))
#define CYMTOF(y)  (f_y + (f_h - MTOF((y)-MM(m_y))))

/* the following is crap*/
#define LINE_NEVERSEE ML_DONTDRAW

typedef struct
{
    int x, y;
} fpoint_t;

typedef struct
{
    fpoint_t a, b;
} fline_t;

typedef struct
{
    fixed_t		x,y;
} mpoint_t;

typedef struct
{
    mpoint_t a, b;
} mline_t;

typedef struct
{
    fixed_t slp, islp;
} islope_t;

typedef struct
{
  int numlines;
  const mline_t *shape;
} mshape_t;


/* The vector graphics for the automap.*/
/*  A line drawing of the player pointing right,*/
/*   starting from the middle.*/

#define R ((8*PLAYERRADIUS)/7)
static const mline_t player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, /* -----*/
    { { R, 0 }, { R-R/2, R/4 } },  /* ----->*/
    { { R, 0 }, { R-R/2, -R/4 } },
    { { -R+R/8, 0 }, { -R-R/8, R/4 } }, /* >---->*/
    { { -R+R/8, 0 }, { -R-R/8, -R/4 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, /* >>--->*/
    { { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))

#define R ((8*PLAYERRADIUS)/7)
static const mline_t cheat_player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, /* -----*/
    { { R, 0 }, { R-R/2, R/6 } },  /* ----->*/
    { { R, 0 }, { R-R/2, -R/6 } },
    { { -R+R/8, 0 }, { -R-R/8, R/6 } }, /* >----->*/
    { { -R+R/8, 0 }, { -R-R/8, -R/6 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, /* >>----->*/
    { { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
    { { -R/2, 0 }, { -R/2, -R/6 } }, /* >>-d--->*/
    { { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
    { { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
    { { -R/6, 0 }, { -R/6, -R/6 } }, /* >>-dd-->*/
    { { -R/6, -R/6 }, { 0, -R/6 } },
    { { 0, -R/6 }, { 0, R/4 } },
    { { R/6, R/4 }, { R/6, -R/7 } }, /* >>-ddt->*/
    { { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
    { { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t triangle_guy[] = {
    { { (fixed_t)(-.867*R), (fixed_t)(-.5*R) }, { (fixed_t)(.867*R), (fixed_t)(-.5*R) } },
    { { (fixed_t)(.867*R), (fixed_t)(-.5*R) } , { (fixed_t)0, (fixed_t)R } },
    { { (fixed_t)0, (fixed_t)R }, { (fixed_t)(-.867*R), (fixed_t)(-.5*R) } }
};
#undef R
#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t thintriangle_guy[] = {
    { { (fixed_t)(-.5*R), (fixed_t)(-.7*R) }, { (fixed_t)R, (fixed_t)0 } },
    { { (fixed_t)R, (fixed_t)0 }, { (fixed_t)(-.5*R), (fixed_t)(.7*R) } },
    { { (fixed_t)(-.5*R), (fixed_t)(.7*R) }, { (fixed_t)(-.5*R), (fixed_t)(-.7*R) } }
};
#undef R
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t pentacle_guy[] = {
    { { (fixed_t)0, (fixed_t)R }, { (fixed_t)(.588*R), (fixed_t)(-.809*R) } },
    { { (fixed_t)(.588*R), (fixed_t)(-.809*R) }, { (fixed_t)(-.951*R), (fixed_t)(.309*R) }  },
    { { (fixed_t)(-.951*R), (fixed_t)(.309*R) }, { (fixed_t)(.951*R), (fixed_t)(.309*R) } },
    { { (fixed_t)(.951*R), (fixed_t)(.309*R) }, { (fixed_t)(-.588*R), (fixed_t)(-.809*R) } },
    { { (fixed_t)(-.588*R), (fixed_t)(-.809*R) }, { (fixed_t)0, (fixed_t)R }  }
};
#undef R
#define NUMPENTACLEGUYLINES (sizeof(pentacle_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t hexagon_guy[] = {
    { { (fixed_t)(.5*R), (fixed_t)(.866*R) }, { (fixed_t)R, (fixed_t)0 } },
    { { (fixed_t)R, (fixed_t)0 }, { (fixed_t)(.5*R), (fixed_t)(-.866*R) } },
    { { (fixed_t)(.5*R), (fixed_t)(-.866*R) }, { (fixed_t)(-.5*R), (fixed_t)(-.866*R) } },
    { { (fixed_t)(-.5*R), (fixed_t)(-.866*R) }, { (fixed_t)(-R), (fixed_t)0 } },
    { { (fixed_t)(-R), (fixed_t)0 }, { (fixed_t)(-.5*R), (fixed_t)(.866*R) } },
    { { (fixed_t)(-.5*R), (fixed_t)(.866*R) }, { (fixed_t)(.5*R), (fixed_t)(.866*R) } }
};
#undef R
#define NUMHEXAGONGUYLINES (sizeof(hexagon_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t keyshaped_guy[] = {
    { { (fixed_t)(-.6*R), (fixed_t)(.4*R) }, { (fixed_t)(-.3*R), (fixed_t)(.2*R) } },
    { { (fixed_t)(-.3*R), (fixed_t)(.2*R) }, { (fixed_t)(-.3*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(-.3*R), (fixed_t)(-.2*R) }, { (fixed_t)(-.6*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(-.6*R), (fixed_t)(-.4*R) }, { (fixed_t)(-.9*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(-.9*R), (fixed_t)(-.2*R) }, { (fixed_t)(-.9*R), (fixed_t)(.2*R) } },
    { { (fixed_t)(-.9*R), (fixed_t)(.2*R) }, { (fixed_t)(-.6*R), (fixed_t)(.4*R) } },
    { { (fixed_t)(-.3*R), (fixed_t)(0*R) }, { (fixed_t)(.9*R), (fixed_t)(0*R) } },
    { { (fixed_t)(.9*R), (fixed_t)(0*R) }, { (fixed_t)(.9*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(.9*R), (fixed_t)(-.4*R) }, { (fixed_t)(.6*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(.6*R), (fixed_t)(-.2*R) }, { (fixed_t)(.3*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(.3*R), (fixed_t)(-.4*R) }, { (fixed_t)(.3*R), (fixed_t)(0*R) } },
};
#undef R
#define NUMKEYSHAPEDGUYLINES (sizeof(keyshaped_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t skullkeyshaped_guy[] = {
    { { (fixed_t)(.6*R), (fixed_t)(.4*R) }, { (fixed_t)(.3*R), (fixed_t)(.1*R) } },
    { { (fixed_t)(.3*R), (fixed_t)(.1*R) }, { (fixed_t)(.4*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(.4*R), (fixed_t)(-.2*R) }, { (fixed_t)(.4*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(.4*R), (fixed_t)(-.4*R) }, { (fixed_t)(.8*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(.8*R), (fixed_t)(-.4*R) }, { (fixed_t)(.8*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(.8*R), (fixed_t)(-.2*R) }, { (fixed_t)(.9*R), (fixed_t)(.1*R) } },
    { { (fixed_t)(.9*R), (fixed_t)(.1*R) }, { (fixed_t)(.6*R), (fixed_t)(.4*R) } },
    { { (fixed_t)(1.0/3*R), (fixed_t)0 }, { (fixed_t)(-.9*R), (fixed_t)0 } },
    { { (fixed_t)(-.9*R), (fixed_t)0 }, { (fixed_t)(-.9*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(-.9*R), (fixed_t)(-.4*R) }, { (fixed_t)(-.6*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(-.6*R), (fixed_t)(-.2*R) }, { (fixed_t)(-.3*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(-.3*R), (fixed_t)(-.4*R) }, { (fixed_t)(-.3*R), (fixed_t)0 } },
};
#undef R
#define NUMSKULLKEYSHAPEDGUYLINES (sizeof(skullkeyshaped_guy)/sizeof(mline_t))

static const mshape_t thintriangle_shape = { NUMTHINTRIANGLEGUYLINES, thintriangle_guy };
static const mshape_t pentacle_shape = { NUMPENTACLEGUYLINES, pentacle_guy };
static const mshape_t hexagon_shape = { NUMHEXAGONGUYLINES, hexagon_guy };
static const mshape_t key_shape = { NUMKEYSHAPEDGUYLINES, keyshaped_guy };
static const mshape_t skullkey_shape = { NUMSKULLKEYSHAPEDGUYLINES, skullkeyshaped_guy };


static int	grid = 0;

static int	leveljuststarted = 1;	/* kluge until AM_LevelInit() is called*/

boolean		automapactive = false;
boolean		mixmap = false;
#ifdef STATIC_RESOLUTION
static int	finit_width = SCREENWIDTH;
static int	finit_height = SCREENHEIGHT - SBARHEIGHT;
#else
static int	finit_width;
static int	finit_height;
#endif

/* location of window on screen*/
static int	f_x;
static int	f_y;

/* size of window on screen*/
static int	f_w;
static int	f_h;

static int	lightlev;		/* used for funky strobing effect*/
static pixel_t*	fb;			/* pseudo-frame buffer*/
static int	amclock;

static mpoint_t m_paninc; /* how far the window pans each tic (map coords)*/
static fixed_t 	mtof_zoommul; /* how far the window zooms in each tic (map coords)*/
static fixed_t 	ftom_zoommul; /* how far the window zooms in each tic (fb coords)*/

static fixed_t 	m_x[2], m_y[2];   /* LL x,y where the window is on the map (map coords)*/
static fixed_t 	m_x2[2], m_y2[2]; /* UR x,y where the window is on the map (map coords)*/


/* width/height of window on map (map coords)*/

static fixed_t 	m_w[2];
static fixed_t	m_h[2];

/* based on level size*/
static fixed_t 	min_x;
static fixed_t	min_y;
static fixed_t 	max_x;
static fixed_t  max_y;

static fixed_t 	max_w; /* max_x-min_x,*/
static fixed_t  max_h; /* max_y-min_y*/

/* based on player size*/
static fixed_t 	min_w;
static fixed_t  min_h;


static fixed_t 	min_scale_mtof; /* used to tell when to stop zooming out*/
static fixed_t 	max_scale_mtof; /* used to tell when to stop zooming in*/

/* old stuff for recovery later*/
static fixed_t old_m_w[2], old_m_h[2];
static fixed_t old_m_x[2], old_m_y[2];

/* old location used by the Follower routine*/
static mpoint_t f_oldloc;

/* used by MTOF to scale from map-to-frame-buffer coords*/
static fixed_t scale_mtof[2] = { INITSCALEMTOF, INITSCALEMTOF };
/* used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)*/
static fixed_t scale_ftom[2];

static player_t *plr; /* the player represented by an arrow*/

static patch_t *marknums[10]; /* numbers used for marking by the automap*/
static mpoint_t markpoints[AM_NUMMARKPOINTS]; /* where the points are*/
static int markpointnum = 0; /* next point to be assigned*/

static int followplayer = 1; /* specifies whether to follow the player around*/

static boolean stopped = true;

/*extern byte screens[][SCREENWIDTH*SCREENHEIGHT];*/

int ddt_cheating;

/* map keys */
int mapkey_pandown;
int mapkey_panup;
int mapkey_panright;
int mapkey_panleft;
int mapkey_zoomin;
int mapkey_zoomout;
int mapkey_activate;
int mapkey_altmap;
int mapkey_gobig;
int mapkey_follow;
int mapkey_grid;
int mapkey_mark;
int mapkey_clearmark;

boolean altkey;


void
V_MarkRect
( int	x,
  int	y,
  int	width,
  int	height );

/* Calculates the slope and slope according to the x-axis of a line*/
/* segment in map coordinates (with the upright y-axis n' all) so*/
/* that it can be used with the brain-dead drawing stuff.*/
/*
static void
AM_getIslope
( const mline_t* ml,
  islope_t*	is )
{
    int dx, dy;

    dy = ml->a.y - ml->b.y;
    dx = ml->b.x - ml->a.x;
    if (!dy) is->islp = (dx<0?-MAXINT:MAXINT);
    else is->islp = FixedDiv(dx, dy);
    if (!dx) is->slp = (dy<0?-MAXINT:MAXINT);
    else is->slp = FixedDiv(dy, dx);

}
*/



static void AM_activateNewScale(void)
{
    MM(m_x) += MM(m_w)/2;
    MM(m_y) += MM(m_h)/2;
    if (mixmap) {
	m_w[1] = FTOM(f_w/2);
	m_h[1] = FTOM(f_h/2);
    } else {
	m_w[0] = FTOM(f_w);
	m_h[0] = FTOM(f_h);
    }
    MM(m_x) -= MM(m_w)/2;
    MM(m_y) -= MM(m_h)/2;
    MM(m_x2) = MM(m_x) + MM(m_w);
    MM(m_y2) = MM(m_y) + MM(m_h);
}




static void AM_saveScaleAndLoc(void)
{
    MM(old_m_x) = MM(m_x);
    MM(old_m_y) = MM(m_y);
    MM(old_m_w) = MM(m_w);
    MM(old_m_h) = MM(m_h);
}




static void AM_restoreScaleAndLoc(void)
{

    MM(m_w) = MM(old_m_w);
    MM(m_h) = MM(old_m_h);
    if (!followplayer)
    {
	MM(m_x) = MM(old_m_x);
	MM(m_y) = MM(old_m_y);
    } else {
	MM(m_x) = plr->mo->x - MM(m_w)/2;
	MM(m_y) = plr->mo->y - MM(m_h)/2;
    }
    MM(m_x2) = MM(m_x) + MM(m_w);
    MM(m_y2) = MM(m_y) + MM(m_h);

    /* Change the scaling multipliers*/
    MM(scale_mtof) = FixedDiv(f_w<<(FRACBITS-(int)mixmap), MM(m_w));
    MM(scale_ftom) = FixedDiv(FRACUNIT, MM(scale_mtof));
}


/* adds a marker at the current location*/

static void AM_addMark(void)
{
    markpoints[markpointnum].x = MM(m_x) + MM(m_w)/2;
    markpoints[markpointnum].y = MM(m_y) + MM(m_h)/2;
    markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;

}


/* Determines bounding box of all vertices,*/
/* sets global variables controlling zoom range.*/

static void AM_findMinMaxBoundaries(void)
{
    int i;
    fixed_t a;
    fixed_t b;

    min_x = min_y =  MAXINT;
    max_x = max_y = -MAXINT;

    for (i=0;i<numvertexes;i++)
    {
	if (vertexes[i].x < min_x)
	    min_x = vertexes[i].x;
	else if (vertexes[i].x > max_x)
	    max_x = vertexes[i].x;

	if (vertexes[i].y < min_y)
	    min_y = vertexes[i].y;
	else if (vertexes[i].y > max_y)
	    max_y = vertexes[i].y;
    }

    max_w = max_x - min_x;
    max_h = max_y - min_y;

    min_w = 2*PLAYERRADIUS; /* const? never changed?*/
    min_h = 2*PLAYERRADIUS;

    a = FixedDiv(f_w<<FRACBITS, max_w);
    b = FixedDiv(f_h<<FRACBITS, max_h);

    min_scale_mtof = a < b ? a : b;
    max_scale_mtof = FixedDiv(f_h<<FRACBITS, 2*PLAYERRADIUS);

}





static void AM_changeWindowLoc(void)
{
    if (m_paninc.x || m_paninc.y)
    {
	followplayer = 0;
	f_oldloc.x = MAXINT;
    }

    MM(m_x) += m_paninc.x;
    MM(m_y) += m_paninc.y;

    if (MM(m_x) + MM(m_w)/2 > max_x)
	MM(m_x) = max_x - MM(m_w)/2;
    else if (MM(m_x) + MM(m_w)/2 < min_x)
	MM(m_x) = min_x - MM(m_w)/2;

    if (MM(m_y) + MM(m_h)/2 > max_y)
	MM(m_y) = max_y - MM(m_h)/2;
    else if (MM(m_y) + MM(m_h)/2 < min_y)
	MM(m_y) = min_y - MM(m_h)/2;

    MM(m_x2) = MM(m_x) + MM(m_w);
    MM(m_y2) = MM(m_y) + MM(m_h);
}





static void AM_initVariables(void)
{
    int pnum;
    static event_t st_notify = { ev_keyup, AM_MSGENTERED };

    automapactive = !mixmap;
    fb = screens[0];

    f_oldloc.x = MAXINT;
    amclock = 0;
    lightlev = 0;

    m_paninc.x = m_paninc.y = 0;
    ftom_zoommul = FRACUNIT;
    mtof_zoommul = FRACUNIT;

    m_w[1] = (m_w[0] = FTOM(f_w)) / 2;
    m_h[1] = (m_h[0] = FTOM(f_h)) / 2;

    /* find player to center on initially*/
    if (!playeringame[pnum = consoleplayer])
	for (pnum=0;pnum<MAXPLAYERS;pnum++)
	    if (playeringame[pnum])
		break;

    plr = &players[pnum];
    m_x[0] = plr->mo->x - m_w[0]/2;
    m_y[0] = plr->mo->y - m_h[0]/2;
    m_x[1] = plr->mo->x - m_w[1]/2;
    m_y[1] = plr->mo->y - m_h[1]/2;
    AM_changeWindowLoc();

    /* for saving & restoring*/
    old_m_x[0] = m_x[0];
    old_m_y[0] = m_y[0];
    old_m_x[1] = m_x[1];
    old_m_y[1] = m_y[1];
#if (LD_PIXEL_DEPTH > 3)
    old_m_w[0] = FTOM(f_w);
    old_m_h[0] = FTOM(f_h);
#else
    old_m_w[0] = m_w[0];
    old_m_h[0] = m_h[0];
#endif
    old_m_w[1] = old_m_w[0]/2;
    old_m_h[1] = old_m_h[0]/2;

    /* inform the status bar of the change*/
    ST_Responder(&st_notify);

}




static void AM_loadPics(void)
{
    int i;
    char namebuf[9];

    for (i=0;i<10;i++)
    {
	sprintf(namebuf, "AMMNUM%d", i);
	marknums[i] = W_CacheLumpName(namebuf, PU_STATIC);
    }

}

static void AM_unloadPics(void)
{
    int i;

    for (i=0;i<10;i++)
	Z_ChangeTag(marknums[i], PU_CACHE);

}

static void AM_clearMarks(void)
{
    int i;

    for (i=0;i<AM_NUMMARKPOINTS;i++)
	markpoints[i].x = -1; /* means empty*/
    markpointnum = 0;
}


/* should be called at the start of every level*/
/* right now, i figure it out myself*/

static void AM_LevelInit(void)
{
    leveljuststarted = 0;

    f_x = f_y = 0;
    f_w = finit_width;
    f_h = finit_height;

    AM_clearMarks();

    AM_findMinMaxBoundaries();
    scale_mtof[0] = scale_mtof[1] = FixedDiv(min_scale_mtof, (int) (0.7*FRACUNIT));
    if (scale_mtof[0] > max_scale_mtof)
	scale_mtof[0] = scale_mtof[1] = min_scale_mtof;
    scale_ftom[0] = scale_ftom[1] = FixedDiv(FRACUNIT, scale_mtof[0]);
}







void AM_Stop (void)
{
    static event_t st_notify = { 0, ev_keyup, AM_MSGEXITED };

    AM_unloadPics();
    automapactive = false;
    mixmap = false;
    ST_Responder(&st_notify);
    stopped = true;
}




void AM_Start_Local (boolean mixed)
{
    static int lastlevel = -1, lastepisode = -1;

    if (!stopped) AM_Stop();
    stopped = false;
    if (lastlevel != gamemap || lastepisode != gameepisode)
    {
	AM_LevelInit();
	lastlevel = gamemap;
	lastepisode = gameepisode;
    }
    mixmap = mixed;
    AM_initVariables();
    AM_loadPics();
}


void AM_Start (void)
{
    AM_Start_Local (false);
}

/* set the window scale to the maximum size*/

static void AM_minOutWindowScale(void)
{
    MM(scale_mtof) = min_scale_mtof;
    MM(scale_ftom) = FixedDiv(FRACUNIT, MM(scale_mtof));
    AM_activateNewScale();
}


/* set the window scale to the minimum size*/

static void AM_maxOutWindowScale(void)
{
    MM(scale_mtof) = max_scale_mtof;
    MM(scale_ftom) = FixedDiv(FRACUNIT, MM(scale_mtof));
    AM_activateNewScale();
}



/* Check whether the key is a map key */
boolean
AM_IsMapKey( int key )
{
    return ((key == mapkey_pandown) ||
            (key == mapkey_panup) ||
	    (key == mapkey_panright) ||
	    (key == mapkey_panleft) ||
            (key == mapkey_zoomin) ||
	    (key == mapkey_zoomout) ||
	    (key == mapkey_activate) ||
	    (key == mapkey_altmap) ||
	    (key == mapkey_gobig) ||
	    (key == mapkey_follow) ||
	    (key == mapkey_grid) ||
	    (key == mapkey_mark) ||
	    (key == mapkey_clearmark));
}


/* Handle events (user inputs) in automap mode*/

boolean
AM_Responder
( const event_t*	ev )
{

    int rc;
    static int cheatstate=0;
    static int bigstate[2]={0,0};
    static char buffer[20];

    rc = false;

    /* check this first; don't claim map keypresses in normal play mode! */
    if (!automapactive && !mixmap)
    {
	if (ev->type == ev_keydown && ev->data1 == mapkey_activate)
	{
	    AM_Start_Local (altkey);
	    viewactive = false;
	    rc = true;
	}
	else if (ev->data1 == mapkey_altmap)
	{
	    altkey = (ev->type == ev_keydown);
	}
    }

    else if (ev->type == ev_keydown)
    {
	rc = true;
	if (ev->data1 == mapkey_altmap)
	{
	    altkey = true;
	}
	else if (ev->data1 == mapkey_panright)
	{
	    if (!followplayer && (!mixmap || altkey)) m_paninc.x = FTOM(F_PANINC);
	    else rc = false;
	}
	else if (ev->data1 == mapkey_panleft)
	{
	    if (!followplayer && (!mixmap || altkey)) m_paninc.x = -FTOM(F_PANINC);
	    else rc = false;
	}
	else if (ev->data1 == mapkey_panup)
	{
	    if (!followplayer && (!mixmap || altkey)) m_paninc.y = FTOM(F_PANINC);
	    else rc = false;
	}
	else if (ev->data1 == mapkey_pandown)
	{
	    if (!followplayer && (!mixmap || altkey)) m_paninc.y = -FTOM(F_PANINC);
	    else rc = false;
	}
	else if (ev->data1 == mapkey_zoomout)
	{
	    if (mixmap && !altkey) rc = false; else
	    {
	        mtof_zoommul = M_ZOOMOUT;
	        ftom_zoommul = M_ZOOMIN;
            }
	}
	else if (ev->data1 == mapkey_zoomin)
	{
	    if (mixmap && !altkey) rc = false; else
	    {
	        mtof_zoommul = M_ZOOMIN;
	        ftom_zoommul = M_ZOOMOUT;
	    }
	}
	else if (ev->data1 == mapkey_activate)
	{
	    MM(bigstate) = 0;	/* it's turned on before the switch */
	    viewactive = true;
	    AM_Stop ();
	}
	else if (ev->data1 == mapkey_gobig)
	{
	    if (mixmap && !altkey) rc = false; else
	    {
	        MM(bigstate) = !MM(bigstate);
	        if (MM(bigstate))
	        {
		    AM_saveScaleAndLoc();
		    AM_minOutWindowScale();
	        }
	        else AM_restoreScaleAndLoc();
	    }
	}
	else if (ev->data1 == mapkey_follow)
	{
	    if (mixmap && !altkey) rc = false; else
	    {
	        followplayer = !followplayer;
	        f_oldloc.x = MAXINT;
	        plr->message = followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF;
	    }
	}
	else if (ev->data1 == mapkey_grid)
	{
	    if (mixmap && !altkey) rc = false; else
	    {
	        grid = !grid;
	        plr->message = grid ? AMSTR_GRIDON : AMSTR_GRIDOFF;
	    }
	}
	else if (ev->data1 == mapkey_mark)
	{
	    if (mixmap && !altkey) rc = false; else
	    {
	        sprintf(buffer, "%s %d", AMSTR_MARKEDSPOT, markpointnum);
	        plr->message = buffer;
	        AM_addMark();
	    }
	}
	else if (ev->data1 == mapkey_clearmark)
	{
	    if (mixmap && !altkey) rc = false; else
	    {
	        AM_clearMarks();
	        plr->message = AMSTR_MARKSCLEARED;
	    }
	}
	else
	{
	    cheatstate=0;
	    rc = false;
	}
    }

    else if (ev->type == ev_keyup)
    {
	rc = false;
	if (ev->data1 == mapkey_altmap)
	{
            altkey = false;
	}
	else if (ev->data1 == mapkey_panright)
	{
	    if (!followplayer && m_paninc.x > 0) m_paninc.x = 0;
	}
	else if (ev->data1 == mapkey_panleft)
	{
	    if (!followplayer && m_paninc.x < 0) m_paninc.x = 0;
	}
	else if (ev->data1 == mapkey_panup)
	{
	    if (!followplayer && m_paninc.y > 0) m_paninc.y = 0;
	}
	else if (ev->data1 == mapkey_pandown)
	{
	    if (!followplayer && m_paninc.y < 0) m_paninc.y = 0;
	}
	else if ((ev->data1 == mapkey_zoomin) || (ev->data1 == mapkey_zoomout))
	{
	    mtof_zoommul = FRACUNIT;
	    ftom_zoommul = FRACUNIT;
	}
    }

    return rc;

}



/* Zooming*/

static void AM_changeWindowScale(void)
{

    /* Change the scaling multipliers*/
    MM(scale_mtof) = FixedMul(MM(scale_mtof), mtof_zoommul);
    MM(scale_ftom) = FixedDiv(FRACUNIT, MM(scale_mtof));

    if (MM(scale_mtof) < min_scale_mtof)
	AM_minOutWindowScale();
    else if (MM(scale_mtof) > max_scale_mtof)
	AM_maxOutWindowScale();
    else
	AM_activateNewScale();
}





static void AM_doFollowPlayer(void)
{
    if (f_oldloc.x != plr->mo->x || f_oldloc.y != plr->mo->y)
    {
	MM(m_x) = FTOM(MTOF(plr->mo->x)) - MM(m_w)/2;
	MM(m_y) = FTOM(MTOF(plr->mo->y)) - MM(m_h)/2;
	MM(m_x2) = MM(m_x) + MM(m_w);
	MM(m_y2) = MM(m_y) + MM(m_h);
	f_oldloc.x = plr->mo->x;
	f_oldloc.y = plr->mo->y;

	/*  MM(m_x) = FTOM(MTOF(plr->mo->x - MM(m_w)/2));*/
	/*  MM(m_y) = FTOM(MTOF(plr->mo->y - MM(m_h)/2));*/
	/*  MM(m_x) = plr->mo->x - MM(m_w)/2;*/
	/*  MM(m_y) = plr->mo->y - MM(m_h)/2;*/

    }

}



/* Updates on Game Tick*/

void AM_Ticker (void)
{

    if (!automapactive && !mixmap)
	return;

    amclock++;

    if (followplayer)
	AM_doFollowPlayer();

    /* Change the zoom if necessary*/
    if (ftom_zoommul != FRACUNIT)
	AM_changeWindowScale();

    /* Change x,y location*/
    if (m_paninc.x || m_paninc.y)
	AM_changeWindowLoc();

}



/* Clear automap frame buffer.*/

static void AM_clearFB(int color)
{
#if (LD_PIXEL_DEPTH > 3)
    color = d_ctx.static_colourmap[color];
#endif
#if (LD_PIXEL_DEPTH == 3)
#ifdef __riscos__
    if (translated_colourmaps != NULL) color = translated_colourmaps[color];
#endif
    color |= (color << 8);
#endif
#if (LD_PIXEL_DEPTH <= 4)
    color |= (color << 16);
#endif
#ifdef __riscos__
    GameSupp_FillMemory32(fb, color, f_w*f_h*sizeof(pixel_t));
#else
    memset(fb, color, f_w*f_h*sizeof(pixel_t));
#endif
}



/* Automap clipping of lines.*/

/* Based on Cohen-Sutherland clipping algorithm but with a slightly*/
/* faster reject and precalculated slopes.  If the speed is needed,*/
/* use a hash algorithm to handle  the common cases.*/

static boolean
AM_clipMline
( const mline_t* ml,
  fline_t*	fl )
{
    enum
    {
	LEFT	=1,
	RIGHT	=2,
	BOTTOM	=4,
	TOP	=8
    };

    register int	outcode1 = 0;
    register int	outcode2 = 0;
    register int	outside;

    fpoint_t	tmp;
    int		dx;
    int		dy;
    int		fw;


#define DOOUTCODE(oc, mx, my) \
    (oc) = 0; \
    if ((my) < (mixmap ? f_h/2 : 0)) (oc) |= TOP; \
    else if ((my) >= f_h) (oc) |= BOTTOM; \
    if ((mx) < 0) (oc) |= LEFT; \
    else if ((mx) >= fw) (oc) |= RIGHT;


    /* do trivial rejects and outcodes*/
    if (ml->a.y > MM(m_y2))
	outcode1 = TOP;
    else if (ml->a.y < MM(m_y))
	outcode1 = BOTTOM;

    if (ml->b.y > MM(m_y2))
	outcode2 = TOP;
    else if (ml->b.y < MM(m_y))
	outcode2 = BOTTOM;

    if (outcode1 & outcode2)
	return false; /* trivially outside*/

    if (ml->a.x < MM(m_x))
	outcode1 |= LEFT;
    else if (ml->a.x > MM(m_x2))
	outcode1 |= RIGHT;

    if (ml->b.x < MM(m_x))
	outcode2 |= LEFT;
    else if (ml->b.x > MM(m_x2))
	outcode2 |= RIGHT;

    if (outcode1 & outcode2)
	return false; /* trivially outside*/

    /* transform to frame-buffer coordinates.*/
    fl->a.x = CXMTOF(ml->a.x);
    fl->a.y = CYMTOF(ml->a.y);
    fl->b.x = CXMTOF(ml->b.x);
    fl->b.y = CYMTOF(ml->b.y);

    fw = mixmap ? f_w/2 : f_w;

    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    DOOUTCODE(outcode2, fl->b.x, fl->b.y);

    if (outcode1 & outcode2)
	return false;

    while (outcode1 | outcode2)
    {
	/* may be partially inside box*/
	/* find an outside point*/
	if (outcode1)
	    outside = outcode1;
	else
	    outside = outcode2;

	/* clip to each side*/
	if (outside & TOP)
	{
	  dy = fl->a.y - fl->b.y;
	  dx = fl->b.x - fl->a.x;
	  if (mixmap) {
	    tmp.x = fl->a.x + (dx*(fl->a.y-f_h/2))/dy;
	    tmp.y = f_h/2;
	  } else {
	    tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
	    tmp.y = 0;
	  }
	}
	else if (outside & BOTTOM)
	{
	    dy = fl->a.y - fl->b.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.x = fl->a.x + (dx*(fl->a.y-f_h))/dy;
	    tmp.y = f_h-1;
	}
	else if (outside & RIGHT)
	{
	    dy = fl->b.y - fl->a.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.y = fl->a.y + (dy*(fw-1 - fl->a.x))/dx;
	    tmp.x = fw-1;
	}
	else if (outside & LEFT)
	{
	    dy = fl->b.y - fl->a.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
	    tmp.x = 0;
	}

	if (outside == outcode1)
	{
	    fl->a = tmp;
	    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	}
	else
	{
	    fl->b = tmp;
	    DOOUTCODE(outcode2, fl->b.x, fl->b.y);
	}

	if (outcode1 & outcode2)
	    return false; /* trivially outside*/
    }

    return true;
}
#undef DOOUTCODE



/* Classic Bresenham w/ whatever optimizations needed for speed*/

static void
AM_drawFline
( const fline_t* fl,
  int		color )
{
    register int x;
    register int y;
    register int dx;
    register int dy;
    register int sx;
    register int sy;
    register int ax;
    register int ay;
    register int d;
#if (LD_PIXEL_DEPTH > 3)
    int cr, cg, cb;
#endif
    static int fuck = 0;

#if (defined(__riscos__) && (LD_PIXEL_DEPTH == 3))
    if (translated_colourmaps != NULL) color = translated_colourmaps[color];
#endif
#if (LD_PIXEL_DEPTH > 3)
    color = (int)(d_ctx.static_colourmap[color]);
#endif

    /* For debugging only*/
    if (      fl->a.x < 0 || fl->a.x >= f_w
	   || fl->a.y < 0 || fl->a.y >= f_h
	   || fl->b.x < 0 || fl->b.x >= f_w
	   || fl->b.y < 0 || fl->b.y >= f_h)
    {
	fprintf(logfile, "fuck %d \r", fuck++);
	return;
    }

#define PUTDOT(xx,yy,cc) fb[(yy)*f_w+(xx)]=(cc)

    dx = fl->b.x - fl->a.x;
    ax = 2 * (dx<0 ? -dx : dx);
    sx = dx<0 ? -1 : 1;

    dy = fl->b.y - fl->a.y;
    ay = 2 * (dy<0 ? -dy : dy);
    sy = dy<0 ? -1 : 1;

#if (LD_PIXEL_DEPTH > 3)
    x = fl->a.x * 256;
    y = fl->a.y * 256;

    READ_RGB_PIXEL(color, cr, cg, cb);

    if (ax > ay)
    {
	d = ax ? (sy * ay * 256 / ax) : 0;
	while (1)
	{
	  pixel_t *pixel = &fb[(y>>8)*f_w+(x>>8)];
	  int yy = y & 0xFF;
	  if (yy)
	  {
	    int r, g, b;
	    int yy2 = 256 - yy;

	    READ_RGB_PIXEL(*pixel, r, g, b);
	    r = (r * yy + cr * yy2) >> 8;
	    g = (g * yy + cg * yy2) >> 8;
	    b = (b * yy + cb * yy2) >> 8;
	    *pixel = BUILD_RGB_PIXEL(r, g, b);
	    if (yy)
	    {
	      pixel += SCREENWIDTH;
	      READ_RGB_PIXEL(*pixel, r, g, b);
	      r = (r * yy2 + cr * yy) >> 8;
	      g = (g * yy2 + cg * yy) >> 8;
	      b = (b * yy2 + cb * yy) >> 8;
	      *pixel = BUILD_RGB_PIXEL(r, g, b);
	    }
	  }
	  else
	    *pixel = color;
	  if (x == fl->b.x * 256) return;
	  x += sx * 256;
	  y += d;
	}
    }
    else
    {
	d = ay ? (sx * ax * 256 / ay) : 0;
	while (1)
	{
	  pixel_t *pixel = &fb[(y>>8)*f_w+(x>>8)];
	  int xx = x & 0xFF;
	  if (xx)
	  {
	    int r, g, b;
	    int xx2 = 256 - xx;

	    READ_RGB_PIXEL(*pixel, r, g, b);
	    r = (r * xx + cr * xx2) >> 8;
	    g = (g * xx + cg * xx2) >> 8;
	    b = (b * xx + cb * xx2) >> 8;
	    *pixel = BUILD_RGB_PIXEL(r, g, b);
	    if (xx)
	    {
	      pixel ++;
	      READ_RGB_PIXEL(*pixel, r, g, b);
	      r = (r * xx2 + cr * xx) >> 8;
	      g = (g * xx2 + cg * xx) >> 8;
	      b = (b * xx2 + cb * xx) >> 8;
	      *pixel = BUILD_RGB_PIXEL(r, g, b);
	    }
	  }
	  else
	    *pixel = color;
	  if (y == fl->b.y * 256) return;
	  y += sy * 256;
	  x += d;
	}
    }
#else
    x = fl->a.x;
    y = fl->a.y;

    if (ax > ay)
    {
	d = ay - ax/2;
	while (1)
	{
	    PUTDOT(x,y,color);
	    if (x == fl->b.x) return;
	    if (d>=0)
	    {
		y += sy;
		d -= ax;
	    }
	    x += sx;
	    d += ay;
	}
    }
    else
    {
	d = ax - ay/2;
	while (1)
	{
	    PUTDOT(x, y, color);
	    if (y == fl->b.y) return;
	    if (d >= 0)
	    {
		x += sx;
		d -= ay;
	    }
	    y += sy;
	    d += ax;
	}
    }
#endif
}



/* Clip lines, draw visible part sof lines.*/

static void
AM_drawMline
( const mline_t* ml,
  int		color )
{
    static fline_t fl;

    if (color==-1)  /* jff 4/3/98 allow not drawing any sort of line */
      return;	    /* by setting its color to -1 */
    if (color==247) /* jff 4/3/98 if color is 247 (xparent), use black */
      color=0;

    if (AM_clipMline(ml, &fl))
	AM_drawFline(&fl, color); /* draws it on frame buffer using fb coords*/
}




/* Draws flat (floor/ceiling tile) aligned grid lines.*/

static void AM_drawGrid(int color)
{
    fixed_t x, y;
    fixed_t start, end;
    mline_t ml;

    /* Figure out start of vertical gridlines*/
    start = MM(m_x);
    if ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS))
	start += (MAPBLOCKUNITS<<FRACBITS)
	    - ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS));
    end = MM(m_x) + MM(m_w);

    /* draw vertical gridlines*/
    ml.a.y = MM(m_y);
    ml.b.y = MM(m_y)+MM(m_h);
    for (x=start; x<end; x+=(MAPBLOCKUNITS<<FRACBITS))
    {
	ml.a.x = x;
	ml.b.x = x;
	AM_drawMline(&ml, color);
    }

    /* Figure out start of horizontal gridlines*/
    start = MM(m_y);
    if ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS))
	start += (MAPBLOCKUNITS<<FRACBITS)
	    - ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS));
    end = MM(m_y) + MM(m_h);

    /* draw horizontal gridlines*/
    ml.a.x = MM(m_x);
    ml.b.x = MM(m_x) + MM(m_w);
    for (y=start; y<end; y+=(MAPBLOCKUNITS<<FRACBITS))
    {
	ml.a.y = y;
	ml.b.y = y;
	AM_drawMline(&ml, color);
    }

}



/* AM_DoorColor() */

/* Returns the 'color' or key needed for a door linedef type */

/* Passed the type of linedef, returns: */
/*   -1 if not a keyed door */
/*    0 if a red key required */
/*    1 if a blue key required */
/*    2 if a yellow key required */
/*    3 if a multiple keys required */

/* jff 4/3/98 add routine to get color of generalized keyed door */

int AM_DoorColor(int type)
{
#ifdef DIYBOOM
  if (GenLockedBase <= type && type< GenDoorBase)
  {
    type -= GenLockedBase;
    type = (type & LockedKey) >> LockedKeyShift;
    if (!type || type==7)
      return 3;  /*any or all keys */
    else return (type-1)%3;
  }
#endif
  switch (type)  /* closed keyed door */
  {
    case 26: case 32: case 99: case 133:
      /*bluekey*/
      return 1;
    case 27: case 34: case 136: case 137:
      /*yellowkey*/
      return 2;
    case 28: case 33: case 134: case 135:
      /*redkey*/
      return 0;
  }
  return -1;	 /*not a keyed door */
}

/* Determines visible lines, draws them.*/
/* This is LineDef based, not LineSeg based.*/

static void AM_drawWalls(void)
{
  int i;
  static mline_t l;

  /* draw the unclipped visible portions of all lines */
  for (i=0;i<numlines;i++)
  {
    l.a.x = lines[i].v1->x;
    l.a.y = lines[i].v1->y;
    l.b.x = lines[i].v2->x;
    l.b.y = lines[i].v2->y;
    /* if line has been seen or IDDT has been used */
    if (ddt_cheating || (lines[i].flags & ML_MAPPED))
    {
      if ((lines[i].flags & ML_DONTDRAW) && !ddt_cheating)
	continue;
      if (!lines[i].backsector)
      {
	if /*jff 4/23/98 add exit lines to automap */
	(
	  mapcolor_exit &&
	  (
	    lines[i].special==11 ||
	    lines[i].special==52 ||
	    lines[i].special==197 ||
	    lines[i].special==51  ||
	    lines[i].special==124 ||
	    lines[i].special==198
	  )
	)
	AM_drawMline(&l, mapcolor_exit); /* exit line */
#ifdef DIYBOOM
	/* jff 1/10/98 add new color for 1S secret sector boundary */
	else if (mapcolor_secr && /*jff 4/3/98 0 is disable */
	    (
	     (/* ** ignore map_secret_after for now **
	      map_secret_after &&
	      P_WasSecret(lines[i].frontsector) &&
	      !P_IsSecret(lines[i].frontsector)
	     )
	     ||
	     (
	      !map_secret_after && */
	      P_WasSecret(lines[i].frontsector)
	     )
	    )
	  )
	  AM_drawMline(&l, mapcolor_secr); /* line bounding secret sector */
#endif
	else				   /*jff 2/16/98 fixed bug */
	  AM_drawMline(&l, mapcolor_wall); /* special was cleared */
      }
      else
      {
	/* jff 1/10/98 add color change for all teleporter types */
	if
	(
	    mapcolor_tele && !(lines[i].flags & ML_SECRET) &&
	    (lines[i].special == 39 || lines[i].special == 97 ||
	    lines[i].special == 125 || lines[i].special == 126)
	)
	{ /* teleporters */
	  AM_drawMline(&l, mapcolor_tele);
	}
	else if /*jff 4/23/98 add exit lines to automap */
	(
	  mapcolor_exit &&
	  (
	    lines[i].special==11 ||
	    lines[i].special==52 ||
	    lines[i].special==197 ||
	    lines[i].special==51  ||
	    lines[i].special==124 ||
	    lines[i].special==198
	  )
	)
	  AM_drawMline(&l, mapcolor_exit); /* exit line */
	else if /*jff 1/5/98 this clause implements showing keyed doors */
	(
	  (mapcolor_bdor || mapcolor_ydor || mapcolor_rdor) &&
	  ((lines[i].special >=26 && lines[i].special <=28) ||
	  (lines[i].special >=32 && lines[i].special <=34) ||
	  (lines[i].special >=133 && lines[i].special <=137) ||
	  lines[i].special == 99
	  BOOMSTATEMENT(|| (lines[i].special>=GenLockedBase && lines[i].special<GenDoorBase))
	  )
	)
	{
	  if ((lines[i].backsector->floorheight==lines[i].backsector->ceilingheight) ||
	      (lines[i].frontsector->floorheight==lines[i].frontsector->ceilingheight))
	  {
	    switch (AM_DoorColor(lines[i].special)) /* closed keyed door */
	    {
	      case 1:
		/*bluekey*/
		AM_drawMline(&l,
		  mapcolor_bdor? mapcolor_bdor : mapcolor_cchg);
		break;
	      case 2:
		/*yellowkey*/
		AM_drawMline(&l,
		  mapcolor_ydor? mapcolor_ydor : mapcolor_cchg);
		break;
	      case 0:
		/*redkey*/
		AM_drawMline(&l,
		  mapcolor_rdor? mapcolor_rdor : mapcolor_cchg);
		break;
	      case 3:
		/*any or all*/
		AM_drawMline(&l,
		  mapcolor_clsd? mapcolor_clsd : mapcolor_cchg);
		break;
	    }
	  }
	  else AM_drawMline(&l, mapcolor_cchg); /* open keyed door */
	}
	else if (lines[i].flags & ML_SECRET)	/* secret door */
	{
	  AM_drawMline(&l, mapcolor_wall);	/* wall color */
	}
	else if
	(
	    mapcolor_clsd &&
	    !(lines[i].flags & ML_SECRET) &&	/* non-secret closed door */
	    ((lines[i].backsector->floorheight==lines[i].backsector->ceilingheight) ||
	    (lines[i].frontsector->floorheight==lines[i].frontsector->ceilingheight))
	)
	{
	  AM_drawMline(&l, mapcolor_clsd);	/* non-secret closed door */
	}
#ifdef DIYBOOM
	/*jff 1/6/98 show secret sector 2S lines */
	else if
	(
	    mapcolor_secr && /*jff 2/16/98 fixed bug */
	    (			 /* special was cleared after getting it */
#if 0
	      /* ** ignore map_secret_after for now ** */
	      (map_secret_after &&
	       (
		(P_WasSecret(lines[i].frontsector)
		 && !P_IsSecret(lines[i].frontsector)) ||
		(P_WasSecret(lines[i].backsector)
		 && !P_IsSecret(lines[i].backsector))
	       )
	      )
	      ||  /*jff 3/9/98 add logic to not show secret til after entered */
#endif
	      (	  /* if map_secret_after is true */
		/*!map_secret_after &&*/
		 (P_WasSecret(lines[i].frontsector) ||
		  P_WasSecret(lines[i].backsector))
	      )
	    )
	)
	{
	  AM_drawMline(&l, mapcolor_secr); /* line bounding secret sector */
	} /*jff 1/6/98 end secret sector line change */
#endif
	else if (lines[i].backsector->floorheight !=
		  lines[i].frontsector->floorheight)
	{
	  AM_drawMline(&l, mapcolor_fchg); /* floor level change */
	}
	else if (lines[i].backsector->ceilingheight !=
		  lines[i].frontsector->ceilingheight)
	{
	  AM_drawMline(&l, mapcolor_cchg); /* ceiling level change */
	}
	else if (mapcolor_flat && ddt_cheating)
	{
	  AM_drawMline(&l, mapcolor_flat); /*2S lines that appear only in IDDT */
	}
      }
    } /* now draw the lines only visible because the player has computermap */
    else if (plr->powers[pw_allmap]) /* computermap visible lines */
    {
      if (!(lines[i].flags & ML_DONTDRAW)) /* invisible flag lines do not show */
      {
	if
	(
	  mapcolor_flat
	  ||
	  !lines[i].backsector
	  ||
	  lines[i].backsector->floorheight
	  != lines[i].frontsector->floorheight
	  ||
	  lines[i].backsector->ceilingheight
	  != lines[i].frontsector->ceilingheight
	)
	  AM_drawMline(&l, mapcolor_unsn);
      }
    }
  }
}



/* Rotation in 2D.*/
/* Used to rotate player arrow line character.*/

static void
AM_rotate
( fixed_t*	x,
  fixed_t*	y,
  angle_t	a )
{
    fixed_t tmpx;

    tmpx =
	FixedMul(*x,finecosine[a>>ANGLETOFINESHIFT])
	- FixedMul(*y,finesine[a>>ANGLETOFINESHIFT]);

    *y	 =
	FixedMul(*x,finesine[a>>ANGLETOFINESHIFT])
	+ FixedMul(*y,finecosine[a>>ANGLETOFINESHIFT]);

    *x = tmpx;
}

static void
AM_drawLineCharacter
( const mline_t* lineguy,
  int		lineguylines,
  fixed_t	scale,
  angle_t	angle,
  int		color,
  fixed_t	x,
  fixed_t	y )
{
    int		i;
    mline_t	l;

    for (i=0;i<lineguylines;i++)
    {
	l.a.x = lineguy[i].a.x;
	l.a.y = lineguy[i].a.y;

	if (scale)
	{
	    l.a.x = FixedMul(scale, l.a.x);
	    l.a.y = FixedMul(scale, l.a.y);
	}

	if (angle)
	    AM_rotate(&l.a.x, &l.a.y, angle);

	l.a.x += x;
	l.a.y += y;

	l.b.x = lineguy[i].b.x;
	l.b.y = lineguy[i].b.y;

	if (scale)
	{
	    l.b.x = FixedMul(scale, l.b.x);
	    l.b.y = FixedMul(scale, l.b.y);
	}

	if (angle)
	    AM_rotate(&l.b.x, &l.b.y, angle);

	l.b.x += x;
	l.b.y += y;

	AM_drawMline(&l, color);
    }
}

static void AM_drawPlayers(void)
{
    int		i;
    player_t*	p;
    int		their_color = -1;
    int		color;

    if (!netgame)
    {
	if (ddt_cheating)
	    AM_drawLineCharacter
		(cheat_player_arrow, NUMCHEATPLYRLINES, 0,
		 plr->mo->angle, mapcolor_sngl, plr->mo->x, plr->mo->y);
	else
	    AM_drawLineCharacter
		(player_arrow, NUMPLYRLINES, 0, plr->mo->angle,
		 mapcolor_sngl, plr->mo->x, plr->mo->y);
	return;
    }

    for (i=0;i<MAXPLAYERS;i++)
    {
	their_color++;
	p = &players[i];

	if ( (deathmatch && !singledemo) && p != plr)
	    continue;

	if (!playeringame[i])
	    continue;

	if (p->powers[pw_invisibility])
	    color = 246; /* *close* to black*/
	else
	    color = mapcolor_plyr[their_color]; /*jff 1/6/98 use default color */

	AM_drawLineCharacter
	    (player_arrow, NUMPLYRLINES, 0, p->mo->angle,
	     color, p->mo->x, p->mo->y);
    }

}

static void
AM_drawThings (void)
{
    int		i;
    mobj_t*	t;

    for (i=0;i<numsectors;i++)
    {
	t = sectors[i].thinglist;
	while (t)
	{
	    AM_drawLineCharacter
		(thintriangle_guy, NUMTHINTRIANGLEGUYLINES,
		 16<<FRACBITS, t->angle, mapcolor_sprt, t->x, t->y);
	    t = t->snext;
	}
    }
}

static void
AM_drawThingsDifferently (void)
{
  int i;
  for (i = 0; i < numsectors; i++)
  {
    const mobj_t *t = sectors[i].thinglist;
    while (t)
    {
      const mshape_t *shape = &thintriangle_shape;
      int colour = mapcolor_sprt;
      int angle = 0;

      if (t->info)
      switch (t -> sprite)  /* These are identified by sprite number in p_inter.c */
      {
        /* Weapons */
        case SPR_BFUG:
        case SPR_MGUN:
        case SPR_CSAW:
        case SPR_LAUN:
        case SPR_PLAS:
        case SPR_SHOT:
        case SPR_SGN2:
          break;

        /* Ammunition */
        case SPR_CLIP:
        case SPR_AMMO:
        case SPR_ROCK:
        case SPR_BROK:
        case SPR_CELL:
        case SPR_CELP:
        case SPR_SHEL:
        case SPR_SBOX:
        case SPR_BPAK:
          if (t->info->flags & MF_SPECIAL)
          {
            colour = mapcolor_ammo;
            shape = &pentacle_shape;
          }
          break;

        /* Health, armour */
        case SPR_ARM1:
        case SPR_ARM2:
        case SPR_BON1:
        case SPR_BON2:
        case SPR_STIM:
        case SPR_MEDI:
        case SPR_BEXP:
          if (t->info->flags & MF_SPECIAL)
          {
            colour = mapcolor_bonus;
            shape = &pentacle_shape;
          }
          break;

        /* Special objects */
        case SPR_SOUL:
        case SPR_MEGA:
        case SPR_PINV:
        case SPR_PSTR:
        case SPR_PINS:
        case SPR_SUIT:
        case SPR_PMAP:
        case SPR_PVIS:
          if (t->info->flags & MF_SPECIAL)
          {
            colour = mapcolor_special;
            shape = &pentacle_shape;
          }
          break;

        /* Key cards */
        case SPR_BKEY:
          if (t->info->flags & MF_SPECIAL)
          {
            colour = mapcolor_bkey;
            shape = &key_shape;
          }
          break;

        case SPR_YKEY:
          if (t->info->flags & MF_SPECIAL)
          {
            colour = mapcolor_ykey;
            shape = &key_shape;
          }
          break;

        case SPR_RKEY:
          if (t->info->flags & MF_SPECIAL)
          {
            colour = mapcolor_rkey;
            shape = &key_shape;
          }
          break;

        /* Skull keys */
        case SPR_BSKU:
          if (t->info->flags & MF_SPECIAL)
          {
            colour = mapcolor_bkey;
            shape = &skullkey_shape;
          }
          break;

        case SPR_YSKU:
          if (t->info->flags & MF_SPECIAL)
          {
            colour = mapcolor_ykey;
            shape = &skullkey_shape;
          }
          break;

        case SPR_RSKU:
          if (t->info->flags & MF_SPECIAL)
          {
            colour = mapcolor_rkey;
            shape = &skullkey_shape;
          }
          break;

        default:                           /* Not known by its sprite */
          switch (t->info->doomednum)
          {
            /* Monsters */
            case 7: case 9: case 16: case 58: case 64: case 65: case 66:
            case 67: case 68: case 69: case 71: case 84: case 3001: case 3002:
            case 3003: case 3004: case 3005: case 3006:
            /* Boss brain, Commander Keen */
            case 88: case 72:
              if (t->health > 0)
              {
                colour = mapcolor_monster;
                angle = t->angle;
              }
              else
              {
                colour = mapcolor_dead;
                shape = &hexagon_shape;
                 }
              break;
#if 0
            /* Weapons */
            case 82: case 2001: case 2002: case 2003: case 2004: case 2005:
            case 2006: case 2035:
              break;
            /* Ammunition */
            case 8: case 17: case 2007: case 2008: case 2010: case 2046:
            case 2047: case 2048: case 2049:
              colour = mapcolor_ammo;
              shape = &pentacle_shape;
              break;
            /* Health, armour */
            case 2011: case 2012: case 2014: case 2015: case 2018: case 2019:
              colour = mapcolor_bonus;
              shape = &pentacle_shape;
              break;
            /* Special objects */
            case 83: case 2013: case 2022: case 2023: case 2024: case 2025:
            case 2026: case 2045:
              colour = mapcolor_special;
              shape = &pentacle_shape;
              break;
            /* Key cards */
            case 5 : colour = mapcolor_bkey; shape = &key_shape; break;
            case 6 : colour = mapcolor_ykey; shape = &key_shape; break;
            case 13: colour = mapcolor_rkey; shape = &key_shape; break;
            /* Skull keys */
            case 40: colour = mapcolor_bkey; shape = &skullkey_shape; break;
            case 39: colour = mapcolor_ykey; shape = &skullkey_shape; break;
            case 38: colour = mapcolor_rkey; shape = &skullkey_shape; break;
#endif
            /* Pillars */
            case 30: case 31: case 32: case 33: case 36: case 37:
              colour = mapcolor_pillar;
              shape = &hexagon_shape;
              break;
            /* Trees */
            case 41: case 43: case 47: case 48: case 54:
              colour = mapcolor_tree;
              shape = &hexagon_shape;
              break;
            /* Lighting */
            case 34: case 35: case 44: case 45: case 46: case 55: case 56:
            case 57: case 70: case 85: case 86: case 2028:
              colour = mapcolor_light;
              shape = &hexagon_shape;
              break;
            /* Bodies */
            case 25: case 26: case 49: case 50: case 51: case 52: case 53:
            case 59: case 60: case 61: case 62: case 63: case 73: case 74:
            case 75: case 76: case 77: case 78:
              colour = mapcolor_body;
              shape = &hexagon_shape;
              break;
            /* Dead players & monsters */
            case 10: case 12: case 15: case 18: case 19: case 20: case 21:
            case 22: case 23:
              colour = mapcolor_dead;
              shape = &hexagon_shape;
              break;
            /* Blood */
            case 24: case 79: case 80: case 81:
              colour = mapcolor_blood;
              shape = &hexagon_shape;
              break;
            /* Skulls */
            case 27: case 28: case 29: case 42:
              colour = mapcolor_skull;
              shape = &hexagon_shape;
              break;
            default:
              angle = t->angle;
          }
        }
        AM_drawLineCharacter (shape->shape, shape->numlines, 16<<FRACBITS,
                   angle, colour, t->x, t->y);
        t = t->snext;
    }
  }
}

static void AM_drawMarks(void)
{
  int i;
  for (i=0;i<markpointnum;i++) /* killough 2/22/98: remove automap mark limit */
    if (markpoints[i].x != -1)
    {
      int w = 5;
      int h = 6;
      int fx = CXMTOF(markpoints[i].x);
      int fy = CYMTOF(markpoints[i].y);
      int j = i;

      do
      {
	int d = j % 10;
	if (d==1)	    /* killough 2/22/98: less spacing for '1' */
	  fx++;

	if (fx >= f_x && fx < f_w - w && fy >= f_y && fy < f_h - h)
	  V_DrawPatch(fx, fy, FB, marknums[d]);
	fx -= w-1;	    /* killough 2/22/98: 1 space backwards */
	j /= 10;
      }
      while (j>0);
    }
}

static void AM_drawCrosshair(int color)
{
#if (defined(__riscos__) && (LD_PIXEL_DEPTH == 3))
    if (translated_colourmaps != NULL) color = translated_colourmaps[color];
#endif
#if (LD_PIXEL_DEPTH > 3)
    color = (int)(d_ctx.static_colourmap[color]);
#endif
    if (mixmap)
      fb[f_w*f_h*3/4 + f_w/4] = color;
    else
      fb[(f_w*(f_h+1))/2] = color; /* single point for now*/
}

void AM_Drawer (void)
{
    if (!automapactive && !mixmap) return;
#ifdef __riscos__
    fb = screens[0];
#endif
    if (!mixmap) {
      AM_clearFB(mapcolor_back); /*jff 1/5/98 background default color */
      if (grid)
	AM_drawGrid(mapcolor_grid); /*jff 1/7/98 grid default color */
    }
    AM_drawWalls();
    AM_drawPlayers();
    if (ddt_cheating==2)
	AM_drawThings();
    else if (ddt_cheating==3)
	AM_drawThingsDifferently();
    AM_drawCrosshair(mapcolor_hair); /*jff 1/7/98 default crosshair color */

    AM_drawMarks();

    V_MarkRect(f_x, f_y, f_w, f_h);

}


#ifndef STATIC_RESOLUTION
void AM_initForResolution(void)
{
  finit_width = SCREENWIDTH;
  finit_height = SCREENHEIGHT - SBARHEIGHT;
}
#endif
