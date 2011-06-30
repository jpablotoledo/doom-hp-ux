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
/*  all external data is defined here*/
/*  most of the data is loaded into different structures at run time*/
/*  some internal structures shared by many modules are here*/

/*-----------------------------------------------------------------------------*/

#ifndef __DOOMDATA__
#define __DOOMDATA__

/* The most basic types we use, portability.*/
#include "doomtype.h"

/* Some global defines, that configure the game.*/
#include "doomdef.h"



/* The offsets for badly aligned textures */
#define MVRT_T_X	0
#define MVRT_T_Y	2
#define MVRT_T_SIZE	4

#define MSDF_T_TEXOFF	0
#define MSDF_T_ROWOFF	2
#define MSDF_T_TOPTEX	4
#define MSDF_T_BOTTEX	12
#define MSDF_T_MIDTEX	20
#define MSDF_T_SECTOR	28
#define MSDF_T_SIZE	30

#define MLDF_T_V1	0
#define MLDF_T_V2	2
#define MLDF_T_FLAGS	4
#define MLDF_T_SPECIAL	6
#define MLDF_T_TAG	8
#define MLDF_T_SIDENUM	10
#define MLDF_T_SIZE	14

#define MSEC_T_FHEIGHT	0
#define MSEC_T_CHEIGHT	2
#define MSEC_T_FPIC	4
#define MSEC_T_CPIC	12
#define MSEC_T_LIGHT	20
#define MSEC_T_SPECIAL	22
#define MSEC_T_TAG	24
#define MSEC_T_SIZE	26

#define MSUB_T_NSEGS	0
#define MSUB_T_FSEG	2
#define MSUB_T_SIZE	4

#define MSEG_T_V1	0
#define MSEG_T_V2	2
#define MSEG_T_ANG	4
#define MSEG_T_LDEF	6
#define MSEG_T_SIDE	8
#define MSEG_T_OFF	10
#define MSEG_T_SIZE	12

#define MNOD_T_X	0
#define MNOD_T_Y	2
#define MNOD_T_DX	4
#define MNOD_T_DY	6
#define MNOD_T_BBOX	8
#define MNOD_T_CHILD	24
#define MNOD_T_SIZE	28

#define MTHG_T_X	0
#define MTHG_T_Y	2
#define MTHG_T_ANGLE	4
#define MTHG_T_TYPE	6
#define MTHG_T_OPTIONS	8
#define MTHG_T_SIZE	10



/* Map level types.*/
/* The following data structures define the persistent format*/
/* used in the lumps of the WAD files.*/


/* Lump order in a map WAD: each map needs a couple of lumps*/
/* to provide a complete scene geometry description.*/
enum
{
  ML_LABEL,		/* A separator, name, ExMx or MAPxx*/
  ML_THINGS,		/* Monsters, items..*/
  ML_LINEDEFS,		/* LineDefs, from editing*/
  ML_SIDEDEFS,		/* SideDefs, from editing*/
  ML_VERTEXES,		/* Vertices, edited and BSP splits generated*/
  ML_SEGS,		/* LineSegs, from LineDefs split by BSP*/
  ML_SSECTORS,		/* SubSectors, list of LineSegs*/
  ML_NODES,		/* BSP nodes*/
  ML_SECTORS,		/* Sectors, from editing*/
  ML_REJECT,		/* LUT, sector-sector visibility	*/
  ML_BLOCKMAP		/* LUT, motion clipping, walls/grid element*/
};


/* A single Vertex.*/
typedef struct
{
  short		x;
  short		y;
} mapvertex_t;


/* A SideDef, defining the visual appearance of a wall,*/
/* by setting textures and offsets.*/
typedef struct
{
  short		textureoffset;
  short		rowoffset;
  char		toptexture[8];
  char		bottomtexture[8];
  char		midtexture[8];
  /* Front sector, towards viewer.*/
  short		sector;
} mapsidedef_t;



/* A LineDef, as used for editing, and as input*/
/* to the BSP builder.*/
typedef struct
{
  short		v1;
  short		v2;
  short		flags;
  short		special;
  short		tag;
  /* sidenum[1] will be -1 if one sided*/
  short		sidenum[2];
} maplinedef_t;



/* LineDef attributes.*/


/* Solid, is an obstacle.*/
#define ML_BLOCKING		1

/* Blocks monsters only.*/
#define ML_BLOCKMONSTERS	2

/* Backside will not be present at all*/
/*  if not two sided.*/
#define ML_TWOSIDED		4

/* If a texture is pegged, the texture will have*/
/* the end exposed to air held constant at the*/
/* top or bottom of the texture (stairs or pulled*/
/* down things) and will move with a height change*/
/* of one of the neighbor sectors.*/
/* Unpegged textures allways have the first row of*/
/* the texture at the top pixel of the line for both*/
/* top and bottom textures (use next to windows).*/

/* upper texture unpegged*/
#define ML_DONTPEGTOP		8

/* lower texture unpegged*/
#define ML_DONTPEGBOTTOM	16

/* In AutoMap: don't map as two sided: IT'S A SECRET!*/
#define ML_SECRET		32

/* Sound rendering: don't let sound cross two of these.*/
#define ML_SOUNDBLOCK		64

/* Don't draw on the automap at all.*/
#define ML_DONTDRAW		128

/* Set if already seen, thus drawn in automap.*/
#define ML_MAPPED		256

/* jff 3/21/98 Set if line absorbs use by player
 * allow multiple push/switch triggers to be used on one push */
#define ML_PASSUSE      512



/* Sector definition, from editing.*/
typedef	struct
{
  short		floorheight;
  short		ceilingheight;
  char		floorpic[8];
  char		ceilingpic[8];
  short		lightlevel;
  short		special;
  short		tag;
} mapsector_t;

/* SubSector, as generated by BSP.*/
typedef struct
{
  short		numsegs;
  /* Index of first one, segs are stored sequentially.*/
  short		firstseg;
} mapsubsector_t;


/* LineSeg, generated by splitting LineDefs*/
/* using partition lines selected by BSP builder.*/
typedef struct
{
  short		v1;
  short		v2;
  short		angle;
  short		linedef;
  short		side;
  short		offset;
} mapseg_t;



/* BSP node structure.*/

/* Indicate a leaf.*/
#define	NF_SUBSECTOR	0x8000

typedef struct
{
  /* Partition line from (x,y) to x+dx,y+dy)*/
  short		x;
  short		y;
  short		dx;
  short		dy;

  /* Bounding box for each child,*/
  /* clip against view frustum.*/
  short		bbox[2][4];

  /* If NF_SUBSECTOR its a subsector,*/
  /* else it's a node of another subtree.*/
  unsigned short	children[2];

} mapnode_t;




/* Thing definition, position, orientation and type,*/
/* plus skill/visibility flags and attributes.*/
typedef struct
{
    short		x;
    short		y;
    short		angle;
    short		type;
    short		options;
} mapthing_t;





#endif			/* __DOOMDATA__*/
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/

