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
/*      Refresh/rendering module, shared data struct definitions.*/

/*-----------------------------------------------------------------------------*/


#ifndef __R_DEFS__
#define __R_DEFS__


/* Screenwidth.*/
#include "doomdef.h"

/* Some more or less basic data types*/
/* we depend on.*/
#include "m_fixed.h"

/* We rely on the thinker data struct*/
/* to handle sound origins in sectors.*/
#include "d_think.h"
/* SECTORS do store MObjs anyway.*/
#include "p_mobj.h"



#ifdef __GNUG__
#pragma interface
#endif



/* Silhouette, needed for clipping Segs (mainly)*/
/* and sprites representing things.*/
#define SIL_NONE		0
#define SIL_BOTTOM		1
#define SIL_TOP			2
#define SIL_BOTH		3

/* Dynamic drawsegs, only default value needed */
#define DEFDRAWSEGS		256


/* Moves status bar height here... */
#define SBARHEIGHT		32





/* INTERNAL MAP TYPES*/
/*  used by play and refresh*/



/* Your plain vanilla vertex.*/
/* Note: transformed values not buffered locally,*/
/*  like some DOOM-alikes ("wt", "WebView") did.*/

typedef struct
{
    fixed_t	x;
    fixed_t	y;

} vertex_t;


/* Forward of LineDefs, for Sectors.*/
struct line_s;

/* Each sector has a degenmobj_t in its center*/
/*  for sound origin purposes.*/
/* I suppose this does not handle sound from*/
/*  moving objects (doppler), because*/
/*  position is prolly just buffered, not*/
/*  updated.*/
typedef struct
{
    thinker_t		thinker;	/* not used for anything*/
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;

} degenmobj_t;


/* The SECTORS record, at runtime.*/
/* Stores things/mobjs.*/

typedef	struct
{
    fixed_t	floorheight;
    fixed_t	ceilingheight;
    dshort_t	floorpic;
    dshort_t	ceilingpic;
    dshort_t	lightlevel;
    dshort_t	special;
    dshort_t	tag;

    /* 0 = untraversed, 1,2 = sndlines -1*/
    int		soundtraversed;

    /* thing that made a sound (or null)*/
    mobj_t*	soundtarget;

    /* mapblock bounding box for height changes*/
    int		blockbox[4];

    /* origin for any sounds played by the sector*/
    degenmobj_t	soundorg;

    /* if == validcount, already checked*/
    int		validcount;

    /* list of mobjs in sector*/
    mobj_t*	thinglist;

    int			linecount;
    struct line_s**	lines;	/* [linecount] size*/
#ifdef DIYBOOM
    dshort_t	oldspecial;
    int		nexttag;
    int		firsttag;
    void*	floordata;
    void*	ceilingdata;
    void*	lightingdata;
    int		stairlock;
    int		prevsec;
    int		nextsec;
    fixed_t	floor_xoffs;
    fixed_t	floor_yoffs;
    fixed_t	ceiling_xoffs;
    fixed_t	ceiling_yoffs;
    int		heightsec;
    int		floorlightsec;
    int		ceilinglightsec;
    int		bottommap;
    int		midmap;
    int		topmap;
    struct msecnode_s *touching_thinglist;
#else
    /* thinker_t for reversable actions*/
    void*	specialdata;
#endif
} sector_t;





/* The SideDef.*/


typedef struct
{
    /* add this to the calculated texture column*/
    fixed_t	textureoffset;

    /* add this to the calculated texture top*/
    fixed_t	rowoffset;

    /* Texture indices.*/
    /* We do not maintain names here. */
    dshort_t	toptexture;
    dshort_t	bottomtexture;
    dshort_t	midtexture;

    /* Sector the SideDef is facing.*/
    sector_t*	sector;

#ifdef DIYBOOM
    int		special;
#endif
} side_t;




/* Move clipping aid for LineDefs.*/

typedef enum
{
    ST_HORIZONTAL,
    ST_VERTICAL,
    ST_POSITIVE,
    ST_NEGATIVE

} slopetype_t;



typedef struct line_s
{
    /* Vertices, from v1 to v2.*/
    vertex_t*	v1;
    vertex_t*	v2;

    /* Precalculated v2 - v1 for side checking.*/
    fixed_t	dx;
    fixed_t	dy;

    /* Animation related.*/
    dshort_t	flags;
    dshort_t	special;
    dshort_t	tag;

    /* Visual appearance: SideDefs.*/
    /*  sidenum[1] will be -1 if one sided*/
    dshort_t	sidenum[2];

    /* Neat. Another bounding box, for the extent*/
    /*  of the LineDef.*/
    fixed_t	bbox[4];

    /* To aid move clipping.*/
    slopetype_t	slopetype;

    /* Front and back sector.*/
    /* Note: redundant? Can be retrieved from SideDefs.*/
    sector_t*	frontsector;
    sector_t*	backsector;

    /* if == validcount, already checked*/
    int		validcount;

    /* thinker_t for reversable actions*/
    void*	specialdata;

#ifdef DIYBOOM
    int		tranlump;
    int		firsttag;
    int		nexttag;
#endif
} line_t;





/* A SubSector.*/
/* References a Sector.*/
/* Basically, this is a list of LineSegs,*/
/*  indicating the visible walls that define*/
/*  (all or some) sides of a convex BSP leaf.*/

typedef struct subsector_s
{
    sector_t*	sector;
    dshort_t	numlines;
    dshort_t	firstline;

} subsector_t;



#ifdef DIYBOOM
typedef struct msecnode_s
{
    sector_t*		m_sector;
    struct mobj_s*	m_thing;
    struct msecnode_s*	m_tprev;
    struct msecnode_s*	m_tnext;
    struct msecnode_s*	m_sprev;
    struct msecnode_s*	m_snext;
    boolean		visited;
} msecnode_t;
#endif


/* The LineSeg.*/

typedef struct
{
    vertex_t*	v1;
    vertex_t*	v2;

    fixed_t	offset;

    angle_t	angle;

    side_t*	sidedef;
    line_t*	linedef;

    /* Sector references.*/
    /* Could be retrieved from linedef, too.*/
    /* backsector is NULL for one sided lines*/
    sector_t*	frontsector;
    sector_t*	backsector;

} seg_t;




/* BSP node.*/

typedef struct
{
    /* Partition line.*/
    fixed_t	x;
    fixed_t	y;
    fixed_t	dx;
    fixed_t	dy;

    /* Bounding box for each child.*/
    fixed_t	bbox[2][4];

    /* If NF_SUBSECTOR its a subsector.*/
    dushort_t children[2];

} node_t;




/* posts are runs of non masked source pixels*/
typedef struct
{
    byte		topdelta;	/* -1 is the last post in a column*/
    byte		length; 	/* length data bytes follows*/
} post_t;

/* column_t is a list of 0 or more post_t, (byte)-1 terminated*/
typedef post_t	column_t;



/* PC direct to screen pointers*/
/*B UNUSED - keep till detailshift in r_draw.c resolved*/
/*extern byte*	destview;*/
/*extern byte*	destscreen;*/






/* OTHER TYPES*/


/* This could be wider for >8 bit display.*/
/* Indeed, true color support is posibble*/
/*  precalculating 24bpp lightmap/colormap LUT.*/
/*  from darkening PLAYPAL to all black.*/
/* Could even us emore than 32 levels.*/
#if (LD_PIXEL_DEPTH == 3)
typedef byte	lighttable_t;
#else
typedef long	lighttable_t;
#endif





/* ?*/

typedef struct drawseg_s
{
    seg_t*		curline;
    int			x1;
    int			x2;

    fixed_t		scale1;
    fixed_t		scale2;
    fixed_t		scalestep;

    /* 0=none, 1=bottom, 2=top, 3=both*/
    int			silhouette;

    /* do not clip sprites above this*/
    fixed_t		bsilheight;

    /* do not clip sprites below this*/
    fixed_t		tsilheight;

    /* Pointers to lists for sprite clipping,*/
    /*  all three adjusted so [x1] is first value.*/
    dshort_t*		sprtopclip;
    dshort_t*		sprbottomclip;
    dshort_t*		maskedtexturecol;

} drawseg_t;



/* Patches.*/
/* A patch holds one or more columns.*/
/* Patches are used for sprites and all masked pictures,*/
/* and we compose textures from the TEXTURE1/2 lists*/
/* of patches.*/
typedef struct
{
    short		width;		/* bounding box size */
    short		height;
    short		leftoffset;	/* pixels to the left of origin */
    short		topoffset;	/* pixels below the origin */
    int			columnofs[8];	/* only [width] used*/
    /* the [0] is &columnofs[width] */
} patch_t;







/* A vissprite_t is a thing*/
/*  that will be drawn during a refresh.*/
/* I.e. a sprite object that is partly visible.*/
typedef struct vissprite_s
{
    /* Doubly linked list.*/
    struct vissprite_s*	prev;
    struct vissprite_s*	next;

    int			x1;
    int			x2;

    /* for line side calculation*/
    fixed_t		gx;
    fixed_t		gy;

    /* global bottom / top for silhouette clipping*/
    fixed_t		gz;
    fixed_t		gzt;

    /* horizontal position of x1*/
    fixed_t		startfrac;

    fixed_t		scale;

    /* negative if flipped*/
    fixed_t		xiscale;

    fixed_t		texturemid;
    int			patch;

    /* for color translation and shadow draw,*/
    /*  maxbright frames as well*/
    const lighttable_t*	colormap;

    int			mobjflags;
#if ((LD_PIXEL_DEPTH > 3) && defined(DIYTRANSPARENCY))
    int			translucent;
#endif
#ifdef DIYBOOM
    int			heightsec;
#endif
} vissprite_t;



/* Sprites are patches with a special naming convention*/
/*  so they can be recognized by R_InitSprites.*/
/* The base name is NNNNFx or NNNNFxFx, with*/
/*  x indicating the rotation, x = 0, 1-7.*/
/* The sprite and frame specified by a thing_t*/
/*  is range checked at run time.*/
/* A sprite is a patch_t that is assumed to represent*/
/*  a three dimensional object and may have multiple*/
/*  rotations pre drawn.*/
/* Horizontal flipping is used to save space,*/
/*  thus NNNNF2F5 defines a mirrored patch.*/
/* Some sprites will only have one picture used*/
/* for all views: NNNNF0*/

typedef struct
{
    /* If false use 0 for any position.*/
    /* Note: as eight entries are available,*/
    /*  we might as well insert the same name eight times.*/
    boolean	rotate;

    /* Lump to use for view angles 0-7.*/
    dshort_t	lump[8];

    /* Flip bit (1 = flip) to use for view angles 0-7.*/
    byte	flip[8];

} spriteframe_t;




/* A sprite definition:*/
/*  a number of animation frames.*/

typedef struct
{
    int			numframes;
    spriteframe_t*	spriteframes;

} spritedef_t;




/* visplanes in the original sense are no more */

#ifdef STATIC_RESOLUTION
#if (SCREENHEIGHT >= 256)
typedef dushort_t	vistype_t;
#define VPLANE_MAX_VALUE	0x4000
#else
typedef byte		vistype_t;
#define VPLANE_MAX_VALUE	0xff
#endif
#else
typedef dushort_t	vistype_t;
#define VPLANE_MAX_VALUE	0x4000
#endif

typedef struct
{
    fixed_t		height;
    int			picnum;
    int			lightlevel;
    int			minx;
    int			maxx;
#ifdef DIYBOOM
    fixed_t		xoffs;
    fixed_t		yoffs;
#endif
    /* This will then be followed by the arrays of the correct size for this res */
} visplane_head_t;

/* Macros for handling new, dynamic visplanes */
/* (looks much more complicated, but the generated code should be more or less identical) */

/* The size of a visplane */
#define VISPLANE_SIZE \
  (sizeof(visplane_head_t) + (2*SCREENWIDTH+4)*sizeof(vistype_t))

/* progress a visplane_head_t* */
#define VISPLANE_PLUS(p) \
  p = (visplane_head_t*)(((char*)(p)) + VISPLANE_SIZE)

/* Return a pointer to the top part of a visplane */
#define VISPLANE_TOP(p) \
  ((vistype_t*)(((char*)(p)) + sizeof(visplane_head_t) + sizeof(vistype_t)))
/* Return a pointer to the bottom part of a visplane */
#define VISPLANE_BOTTOM(p) \
  ((vistype_t*)(((char*)(p)) + sizeof(visplane_head_t) + (SCREENWIDTH + 3)*sizeof(vistype_t)))

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
