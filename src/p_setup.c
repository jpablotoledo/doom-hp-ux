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
/*	Do all the WAD I/O, get map description,*/
/*	set up initial state and misc. LUTs.*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: p_setup.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";


#ifdef __riscos__
#include "ROsupport.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <limits.h>

#include "z_zone.h"

#include "m_swap.h"
#include "m_bbox.h"
#include "m_argv.h"

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "doomstat.h"

#include "p_setup.h"




/* MAP related Lookup tables.*/
/* Store VERTEXES, LINEDEFS, SIDEDEFS, etc.*/

int		numvertexes;
vertex_t*	vertexes;

int		numsegs;
seg_t*		segs;

int		numsectors;
sector_t*	sectors;

int		numsubsectors;
subsector_t*	subsectors;

int		numnodes;
node_t*		nodes;

int		numlines;
line_t*		lines;

int		numsides;
side_t*		sides;


/* BLOCKMAP*/
/* Created from axis aligned bounding box*/
/* of the map, a rectangular array of*/
/* blocks of size ...*/
/* Used to speed up collision detection*/
/* by spatial subdivision in 2D.*/

/* Blockmap size.*/
int		bmapwidth;
int		bmapheight;	/* size in mapblocks*/
blockmap_t*	blockmap;	/* short for doom, long for vanilla doom */
/* offsets in blockmap are from here*/
blockmap_t*	blockmaplump;
/* origin of block map*/
fixed_t		bmaporgx;
fixed_t		bmaporgy;
/* for thing chains*/
mobj_t**	blocklinks;


/* REJECT*/
/* For fast sight rejection.*/
/* Speeds up enemy AI by skipping detailed*/
/*  LineOf Sight calculation.*/
/* Without special effect, this could be*/
/*  used as a PVS lookup as well.*/

byte*		rejectmatrix;


/* Maintain single and multi player starting spots.*/

mapthing_t	deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t*	deathmatch_p = NULL;
mapthing_t	playerstarts[MAXPLAYERS];



/* For more efficient doomednum lookup */
denum_t		den_lookup[NUMMOBJTYPES];
int		den_number=0;




/* P_LoadVertexes*/

static void P_LoadVertexes (int lump)
{
    byte*		data;
    int			i;
    byte*		ml;
    vertex_t*		li;

    /* Determine number of lumps:*/
    /*  total lump length / vertex record length.*/
    numvertexes = W_LumpLength (lump) / MVRT_T_SIZE;

    /* Allocate zone memory for buffer.*/
    vertexes = Z_Malloc (numvertexes*sizeof(vertex_t),PU_LEVEL,0);

    /* Load data into cache.*/
    data = W_CacheLumpNum (lump,PU_STATIC);

    li = vertexes;
    ml = data;
    /* Copy and convert vertex coordinates,*/
    /* internal representation as fixed.*/
    for (i=0 ; i<numvertexes ; i++, li++, ml += MVRT_T_SIZE)
    {
	li->x = READ_UA_SHORT(ml + MVRT_T_X) << FRACBITS;	/* shift also handles sign */
	li->y = READ_UA_SHORT(ml + MVRT_T_Y) << FRACBITS;
    }
    /* Free buffer memory.*/
    Z_Free (data);
}




/* P_LoadSegs*/

static void P_LoadSegs (int lump)
{
    byte*		data;
    int			i;
    byte*		ml;
    seg_t*		li;
    line_t*		ldef;
    int			linedef;
    int			side;
    byte*		vertchanged;

    numsegs = W_LumpLength (lump) / MSEG_T_SIZE;
    segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);
    memset (segs, 0, numsegs*sizeof(seg_t));
    data = W_CacheLumpNum (lump,PU_STATIC);

    i = (numvertexes+7)>>3;
    if ((vertchanged = Z_MallocNoAbort(i, PU_LEVEL, 0)) != NULL)
        memset(vertchanged, 0, i);

    li = segs;
    ml = data;
    for (i=0 ; i<numsegs ; i++, li++, ml += MSEG_T_SIZE)
    {
        int	ptp_angle;
        int	delta_angle;

        li->v1 = &vertexes[READ_UA_SHORT(ml + MSEG_T_V1)];
        li->v2 = &vertexes[READ_UA_SHORT(ml + MSEG_T_V2)];
        li->angle = READ_UA_SHORT(ml + MSEG_T_ANG) << 16;

        /* firelines fix -- taken from Boom source */
        ptp_angle = R_PointToAngle2(li->v1->x,li->v1->y,li->v2->x,li->v2->y);
        delta_angle = (abs(ptp_angle-li->angle)>>ANGLETOFINESHIFT)*360/8192;

        if ((delta_angle != 0) && (vertchanged != NULL))
        {
            int dis, dx, dy, vnum1, vnum2;

            dx = (li->v1->x - li->v2->x)>>FRACBITS;
            dy = (li->v1->y - li->v2->y)>>FRACBITS;
            dis = ((int) sqrt(dx*dx + dy*dy))<<FRACBITS;
            dx = finecosine[li->angle>>ANGLETOFINESHIFT];
            dy = finesine[li->angle>>ANGLETOFINESHIFT];
            vnum1 = li->v1 - vertexes;
            vnum2 = li->v2 - vertexes;
            if ((vnum2 > vnum1) && ((vertchanged[vnum2>>3] & (1 << (vnum2&7))) == 0))
            {
                li->v2->x = li->v1->x + FixedMul(dis,dx);
                li->v2->y = li->v1->y + FixedMul(dis,dy);
                vertchanged[vnum2>>3] |= 1 << (vnum2&7);
            }
            else if ((vertchanged[vnum1>>3] & (1 << (vnum1&7))) == 0)
            {
                li->v1->x = li->v2->x - FixedMul(dis,dx);
                li->v1->y = li->v2->y - FixedMul(dis,dy);
                vertchanged[vnum1>>3] |= 1 << (vnum1&7);
            }
        }

        li->offset = READ_UA_SHORT(ml + MSEG_T_OFF) << 16;
        linedef = READ_UA_SHORT(ml + MSEG_T_LDEF);
        ldef = &lines[linedef];	/* Assume linedef, sidedef unsigned */
        li->linedef = ldef;
        side = READ_UA_SHORT(ml + MSEG_T_SIDE);
	li->sidedef = &sides[ldef->sidenum[side]];
	li->frontsector = sides[ldef->sidenum[side]].sector;

	if ((ldef-> flags & ML_TWOSIDED) BOOMSTATEMENT(&& (ldef->sidenum[side^1]!=-1)))
	    li->backsector = sides[ldef->sidenum[side^1]].sector;
	else
	    li->backsector = 0;
    }

    if (vertchanged != NULL)
        Z_Free(vertchanged);

    Z_Free (data);
}



/* P_LoadSubsectors*/

static void P_LoadSubsectors (int lump)
{
    byte*		data;
    int			i;
    byte*		ms;
    subsector_t*	ss;

    numsubsectors = W_LumpLength (lump) / MSUB_T_SIZE;
    subsectors = Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
    data = W_CacheLumpNum (lump,PU_STATIC);
    memset (subsectors,0, numsubsectors*sizeof(subsector_t));
    ss = subsectors;

    ms = data;
    for (i=0 ; i<numsubsectors ; i++, ss++, ms += MSUB_T_SIZE)
    {
        ss->numlines = READ_UA_SHORT(ms + MSUB_T_NSEGS);	/* destination is short */
        ss->firstline = READ_UA_SHORT(ms + MSUB_T_FSEG);
    }

    Z_Free (data);
}




/* P_LoadSectors*/

static void P_LoadSectors (int lump)
{
    byte*		data;
    int			i;
    byte*               ms;
    sector_t*		ss;

    numsectors = W_LumpLength (lump) / MSEC_T_SIZE;
    sectors = Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);
    memset (sectors, 0, numsectors*sizeof(sector_t));
    data = W_CacheLumpNum (lump,PU_STATIC);

    ss = sectors;
    ms = data;
    for (i=0 ; i<numsectors ; i++, ss++, ms += MSEC_T_SIZE)
    {
        ss->floorheight = READ_UA_SHORT(ms + MSEC_T_FHEIGHT) << FRACBITS;
        ss->ceilingheight = READ_UA_SHORT(ms + MSEC_T_CHEIGHT) << FRACBITS;
        ss->floorpic = R_FlatNumForName((char*)(ms + MSEC_T_FPIC)); /* those are all short */
        ss->ceilingpic = R_FlatNumForName((char*)(ms + MSEC_T_CPIC));
        ss->lightlevel = READ_UA_SHORT(ms + MSEC_T_LIGHT);
        ss->special = READ_UA_SHORT(ms + MSEC_T_SPECIAL);
        ss->tag = READ_UA_SHORT(ms + MSEC_T_TAG);
        ss->thinglist = NULL;
#ifdef DIYBOOM
        ss->touching_thinglist = NULL;
        ss->nextsec = -1;
        ss->prevsec = -1;
        ss->floor_xoffs = 0;
        ss->floor_yoffs = 0;
        ss->ceiling_xoffs = 0;
        ss->ceiling_yoffs = 0;
        ss->heightsec = -1;
        ss->floorlightsec = -1;
        ss->ceilinglightsec = -1;
        ss->bottommap = 0;
        ss->midmap = 0;
        ss->topmap = 0;
#endif
    }

    Z_Free (data);
}



/* P_LoadNodes*/

static void P_LoadNodes (int lump)
{
    byte*	data;
    int		i;
    int		j;
    int		k;
    byte*       mn;
    node_t*	no;

    numnodes = W_LumpLength (lump) / MNOD_T_SIZE;
    nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);
    data = W_CacheLumpNum (lump,PU_STATIC);

    no = nodes;
    mn = data;
    for (i=0 ; i<numnodes ; i++, no++, mn += MNOD_T_SIZE)
    {
        no->x = READ_UA_SHORT(mn + MNOD_T_X) << FRACBITS;
        no->y = READ_UA_SHORT(mn + MNOD_T_Y) << FRACBITS;
        no->dx = READ_UA_SHORT(mn + MNOD_T_DX) << FRACBITS;
        no->dy = READ_UA_SHORT(mn + MNOD_T_DY) << FRACBITS;
        for (j=0 ; j<2 ; j++)
        {
            /* children is ushort; need mask in case dshort_t is int */
            no->children[j] = READ_UA_SHORT(mn + MNOD_T_CHILD + j*2) & 0xffff;
            for (k=0 ; k<4 ; k++)
                no->bbox[j][k] = READ_UA_SHORT(mn + MNOD_T_BBOX + (j*4 + k)*2) << FRACBITS;
        }
    }

    Z_Free (data);
}



/* P_LoadThings*/

static void P_LoadThings (int lump)
{
    byte*		data;
    int			i;
    byte*		mt;
    mapthing_t		newthing;
    int			numthings;
    boolean		spawn;

    data = W_CacheLumpNum (lump,PU_STATIC);
    numthings = W_LumpLength (lump) / MTHG_T_SIZE;

    mt = data;
    for (i=0 ; i<numthings ; i++, mt += MTHG_T_SIZE)
    {
	/* All mapthing_t members are short ==> no sign extend */
	newthing.x = READ_UA_SHORT(mt + MTHG_T_X);
	newthing.y = READ_UA_SHORT(mt + MTHG_T_Y);
	newthing.angle = READ_UA_SHORT(mt + MTHG_T_ANGLE);
	newthing.type = READ_UA_SHORT(mt + MTHG_T_TYPE);
	newthing.options = READ_UA_SHORT(mt + MTHG_T_OPTIONS);

	spawn = true;
	/* Do not spawn cool, new monsters if !commercial*/
	if ( gamemode != commercial)
	{
	    switch(newthing.type)
	    {
	      case 68:	/* Arachnotron*/
	      case 64:	/* Archvile*/
	      case 88:	/* Boss Brain*/
	      case 89:	/* Boss Shooter*/
	      case 69:	/* Hell Knight*/
	      case 67:	/* Mancubus*/
	      case 71:	/* Pain Elemental*/
	      case 65:	/* Former Human Commando*/
	      case 66:	/* Revenant*/
	      case 84:	/* Wolf SS*/
		spawn = false;
		break;
	    }
	}
	if (spawn == false)
	    continue;

	P_SpawnMapThing(&newthing);
    }

    Z_Free (data);
}



/* P_LoadLineDefs*/
/* Also counts secret lines for intermissions.*/

static void P_LoadLineDefs (int lump)
{
    byte*		data;
    int			i;
    byte*               mld;
    line_t*		ld;
    vertex_t*		v1;
    vertex_t*		v2;
    dshort_t		vnum1, vnum2;

    numlines = W_LumpLength (lump) / MLDF_T_SIZE;
    lines = Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);
    memset (lines, 0, numlines*sizeof(line_t));
    data = W_CacheLumpNum (lump,PU_STATIC);

    ld = lines;
    mld = data;
    for (i=0 ; i<numlines ; i++, mld += MLDF_T_SIZE, ld++)
    {
        ld->flags = READ_UA_SHORT(mld + MLDF_T_FLAGS);	/* Those 3 are short */
        ld->special = READ_UA_SHORT(mld + MLDF_T_SPECIAL);
        ld->tag = READ_UA_SHORT(mld + MLDF_T_TAG);
	vnum1 = READ_UA_SHORT(mld + MLDF_T_V1);
	vnum2 = READ_UA_SHORT(mld + MLDF_T_V2);

	if ((vnum1 >= numvertexes) || (vnum1 < 0) || (vnum2 >= numvertexes) || (vnum2 < 0))
	{
	    vnum1 = 0; vnum2 = 0;
	}
	v1 = ld->v1 = vertexes + vnum1; v2 = ld->v2 = vertexes + vnum2;

	ld->dx = v2->x - v1->x;
	ld->dy = v2->y - v1->y;

        BOOMSTATEMENT(ld->tranlump = -1;)

	if (!ld->dx)
	    ld->slopetype = ST_VERTICAL;
	else if (!ld->dy)
	    ld->slopetype = ST_HORIZONTAL;
	else
	{
	    if (FixedDiv (ld->dy , ld->dx) > 0)
		ld->slopetype = ST_POSITIVE;
	    else
		ld->slopetype = ST_NEGATIVE;
	}

	if (v1->x < v2->x)
	{
	    ld->bbox[BOXLEFT] = v1->x;
	    ld->bbox[BOXRIGHT] = v2->x;
	}
	else
	{
	    ld->bbox[BOXLEFT] = v2->x;
	    ld->bbox[BOXRIGHT] = v1->x;
	}

	if (v1->y < v2->y)
	{
	    ld->bbox[BOXBOTTOM] = v1->y;
	    ld->bbox[BOXTOP] = v2->y;
	}
	else
	{
	    ld->bbox[BOXBOTTOM] = v2->y;
	    ld->bbox[BOXTOP] = v1->y;
	}

	ld->sidenum[0] = READ_UA_SHORT(mld + MLDF_T_SIDENUM);	/* Those 2 are short */
	ld->sidenum[1] = READ_UA_SHORT(mld + MLDF_T_SIDENUM + 2);

#ifdef DIYBOOM
        if ((ld->sidenum[0] >= 0) && (ld->sidenum[0] < numsides) && (ld->special))
            sides[*ld->sidenum].special = ld->special;
#else
	/* if (ld->sidenum[0] != -1) */
	if ((ld->sidenum[0] >= 0) && (ld->sidenum[1] < numsides))
	    ld->frontsector = sides[ld->sidenum[0]].sector;
	else
	    ld->frontsector = 0;

	/* if (ld->sidenum[1] != -1) */
	if ((ld->sidenum[1] >= 0) && (ld->sidenum[1] < numsides))
	    ld->backsector = sides[ld->sidenum[1]].sector;
	else
	    ld->backsector = 0;
#endif
    }

    Z_Free (data);
}


#ifdef DIYBOOM
static void P_LoadLineDefs2(void)
{
    int		i;
    line_t*	ld;

    for (i=0, ld=lines; i<numlines; i++, ld++)
    {
        ld->frontsector = ((ld->sidenum[0] >= 0) && (ld->sidenum[0] < numsides))
                        ? sides[ld->sidenum[0]].sector : 0;
        ld->backsector  = ((ld->sidenum[1] >= 0) && (ld->sidenum[1] < numsides))
                        ? sides[ld->sidenum[1]].sector : 0;

        switch (ld->special)
        {
            int		lump, j;

            case 260:
              lump = sides[*ld->sidenum].special;
              if (!ld->tag)
                ld->tranlump = lump;
              else
                for (j=0; j<numlines; j++)
                  if (lines[j].tag == ld->tag)
                    lines[j].tranlump = lump;
              break;
            default:
              break;
        }
    }
}
#endif



/* P_LoadSideDefs*/

static void P_LoadSideDefs (int lump)
{
#ifndef DIYBOOM
    byte*		data;
    int			i;
    byte*               msd;
    side_t*		sd;
#endif

    numsides = W_LumpLength (lump) / MSDF_T_SIZE;
    sides = Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);
    memset (sides, 0, numsides*sizeof(side_t));
#ifndef DIYBOOM
    data = W_CacheLumpNum (lump,PU_STATIC);

    sd = sides;
    msd = data;
    for (i=0 ; i<numsides ; i++, msd += MSDF_T_SIZE, sd++)
    {
        sd->textureoffset = READ_UA_SHORT(msd + MSDF_T_TEXOFF) << FRACBITS;
        sd->rowoffset = READ_UA_SHORT(msd + MSDF_T_ROWOFF) << FRACBITS;
        /* next 3 are short */
        sd->toptexture = R_TextureNumForName((char*)(msd + MSDF_T_TOPTEX));
        sd->bottomtexture = R_TextureNumForName((char*)(msd + MSDF_T_BOTTEX));
        sd->midtexture = R_TextureNumForName((char*)(msd + MSDF_T_MIDTEX));
        sd->sector = &sectors[READ_UA_SHORT(msd + MSDF_T_SECTOR)];
    }

    Z_Free (data);
#endif
}

#ifdef DIYBOOM

static void P_LoadSideDefs2(int lump)
{
    byte*	data;
    byte*	msd;
    int		i;
    side_t*	sd;

    data = W_CacheLumpNum(lump, PU_STATIC);

    for (i=0, msd=data, sd=sides; i<numsides; i++, msd += MSDF_T_SIZE, sd++)
    {
        sector_t*	sec;

        sd->textureoffset = READ_UA_SHORT(msd + MSDF_T_TEXOFF) << FRACBITS;
        sd->rowoffset = READ_UA_SHORT(msd + MSDF_T_ROWOFF) << FRACBITS;
        sd->sector = &sectors[READ_UA_SHORT(msd + MSDF_T_SECTOR)];
        sec = sd->sector;

        switch (sd->special)
        {
            char *texname;

            case 242:	/* variable colourmap */
              texname = (char*)msd + MSDF_T_BOTTEX;
              if ((sec->bottommap = R_ColormapNumForName(texname)) < 0)
                sec->bottommap = 0, sd->bottomtexture = R_TextureNumForName(texname);
              else
                sd->bottomtexture = 0;

              texname = (char*)msd + MSDF_T_MIDTEX;
              if ((sec->midmap = R_ColormapNumForName(texname)) < 0)
                sec->midmap = 0, sd->midtexture = R_TextureNumForName(texname);
              else
                sd->midtexture = 0;

              texname = (char*)msd + MSDF_T_TOPTEX;
              if ((sec->topmap = R_ColormapNumForName(texname)) < 0)
                sec->topmap = 0, sd->toptexture = R_TextureNumForName(texname);
              else
                sd->toptexture = 0;

              break;

            case 260:	/* translucency */
              texname = (char*)msd + MSDF_T_MIDTEX;
              if (strncasecmp("TRANMAP", texname, 7) != 0)
              {
                if (((sd->special = W_CheckNumForName(texname)) < 0) || (W_LumpLength(sd->special != 65536)))
                  sd->special = 0, sd->midtexture = R_TextureNumForName(texname);
                else
                  sd->special++, sd->midtexture = 0;
              }
              else
              {
                sd->special = 0; sd->midtexture = 0;
              }
              sd->toptexture = R_TextureNumForName((char*)msd + MSDF_T_TOPTEX);
              sd->bottomtexture = R_TextureNumForName((char*)msd + MSDF_T_BOTTEX);
              break;

            default:
              sd->midtexture = R_TextureNumForName((char*)msd + MSDF_T_MIDTEX);
              sd->toptexture = R_TextureNumForName((char*)msd + MSDF_T_TOPTEX);
              sd->bottomtexture = R_TextureNumForName((char*)msd + MSDF_T_BOTTEX);
              break;
        }
    }
    Z_Free(data);
}



/*
 *  Boom-code for handling larger blockmaps (long rather than short)
 */
#define blkshift 7               /* places to shift rel position for cell num */
#define blkmask ((1<<blkshift)-1)/* mask for rel position within cell */
#define blkmargin 0              /* size guardband around map used */

typedef struct linelist_t        /* type used to list lines in each block */
{
  long num;
  struct linelist_t *next;
} linelist_t;


/*
 *  Subroutine to add a line number to a block list
 *  It simply returns if the line is already in the block
 *  (changed malloc/free to Z_Malloc/Z_Free to avoid problems on RISC OS)
 */

static void AddBlockLine
(
  linelist_t **lists,
  int *count,
  int *done,
  int blockno,
  long lineno
)
{
  linelist_t *l;

  if (done[blockno])
    return;

  l = (linelist_t*)Z_Malloc(sizeof(linelist_t), PU_STATIC, NULL);
  l->num = lineno;
  l->next = lists[blockno];
  lists[blockno] = l;
  count[blockno]++;
  done[blockno] = 1;
}


/*
 *  Actually construct the blockmap lump from the level data
 *
 *  This finds the intersection of each linedef with the column and
 *  row lines at the left and bottom of each blockmap cell. It then
 *  adds the line to all block lists touching the intersection.
 */

void P_CreateBlockMap()
{
  int xorg,yorg;                 /*  blockmap origin (lower left) */
  int nrows,ncols;               /*  blockmap dimensions */
  linelist_t **blocklists=NULL;  /*  array of pointers to lists of lines */
  int *blockcount=NULL;          /*  array of counters of line lists */
  int *blockdone=NULL;           /*  array keeping track of blocks/line */
  int NBlocks;                   /*  number of cells = nrows*ncols */
  long linetotal=0;              /*  total length of all blocklists */
  int i,j;
  int map_minx=INT_MAX;          /*  init for map limits search */
  int map_miny=INT_MAX;
  int map_maxx=INT_MIN;
  int map_maxy=INT_MIN;

  /*  scan for map limits, which the blockmap must enclose */

  for (i=0;i<numvertexes;i++)
  {
    fixed_t t;

    if ((t=vertexes[i].x) < map_minx)
      map_minx = t;
    else if (t > map_maxx)
      map_maxx = t;
    if ((t=vertexes[i].y) < map_miny)
      map_miny = t;
    else if (t > map_maxy)
      map_maxy = t;
  }
  map_minx >>= FRACBITS;    /*  work in map coords, not fixed_t */
  map_maxx >>= FRACBITS;
  map_miny >>= FRACBITS;
  map_maxy >>= FRACBITS;

  /*  set up blockmap area to enclose level plus margin */

  xorg = map_minx-blkmargin;
  yorg = map_miny-blkmargin;
  ncols = (map_maxx+blkmargin-xorg+1+blkmask)>>blkshift;  /* jff 10/12/98 */
  nrows = (map_maxy+blkmargin-yorg+1+blkmask)>>blkshift;  /* +1 needed for */
  NBlocks = ncols*nrows;                                  /* map exactly 1 cell */

  /*  create the array of pointers on NBlocks to blocklists */
  /*  also create an array of linelist counts on NBlocks */
  /*  finally make an array in which we can mark blocks done per line */

  blocklists = (linelist_t**)Z_Malloc(NBlocks*sizeof(linelist_t *), PU_STATIC, NULL);
  memset(blocklists,0,NBlocks*sizeof(linelist_t *));
  blockcount = (int*)Z_Malloc(NBlocks*sizeof(int), PU_STATIC, NULL);
  memset(blockcount,0,NBlocks*sizeof(int));
  blockdone = (int*)Z_Malloc(NBlocks*sizeof(int), PU_STATIC, NULL);

  /*  initialize each blocklist, and enter the trailing -1 in all blocklists */
  /*  note the linked list of lines grows backwards */

  for (i=0;i<NBlocks;i++)
  {
    blocklists[i] = (linelist_t*)Z_Malloc(sizeof(linelist_t), PU_STATIC, NULL);
    blocklists[i]->num = -1;
    blocklists[i]->next = NULL;
    blockcount[i]++;
  }

  /*  For each linedef in the wad, determine all blockmap blocks it touches, */
  /*  and add the linedef number to the blocklists for those blocks */

  for (i=0;i<numlines;i++)
  {
    int x1 = lines[i].v1->x>>FRACBITS;         /*  lines[i] map coords */
    int y1 = lines[i].v1->y>>FRACBITS;
    int x2 = lines[i].v2->x>>FRACBITS;
    int y2 = lines[i].v2->y>>FRACBITS;
    int dx = x2-x1;
    int dy = y2-y1;
    int vert = !dx;                            /*  lines[i] slopetype */
    int horiz = !dy;
    int spos = (dx^dy) > 0;
    int sneg = (dx^dy) < 0;
    int bx,by;                                 /*  block cell coords */
    int minx = x1>x2? x2 : x1;                 /*  extremal lines[i] coords */
    int maxx = x1>x2? x1 : x2;
    int miny = y1>y2? y2 : y1;
    int maxy = y1>y2? y1 : y2;

    /*  no blocks done for this linedef yet */

    memset(blockdone,0,NBlocks*sizeof(int));

    /*  The line always belongs to the blocks containing its endpoints */

    bx = (x1-xorg)>>blkshift;
    by = (y1-yorg)>>blkshift;
    AddBlockLine(blocklists,blockcount,blockdone,by*ncols+bx,i);
    bx = (x2-xorg)>>blkshift;
    by = (y2-yorg)>>blkshift;
    AddBlockLine(blocklists,blockcount,blockdone,by*ncols+bx,i);


    /*  For each column, see where the line along its left edge, which */
    /*  it contains, intersects the Linedef i. Add i to each corresponding */
    /*  blocklist. */

    if (!vert)    /*  don't interesect vertical lines with columns */
    {
      for (j=0;j<ncols;j++)
      {
        /*  intersection of Linedef with x=xorg+(j<<blkshift) */
        /*  (y-y1)*dx = dy*(x-x1) */
        /*  y = dy*(x-x1)+y1*dx; */

        int x = xorg+(j<<blkshift);       /*  (x,y) is intersection */
        int y = (dy*(x-x1))/dx+y1;
        int yb = (y-yorg)>>blkshift;      /*  block row number */
        int yp = (y-yorg)&blkmask;        /*  y position within block */

        if (yb<0 || yb>nrows-1)     /*  outside blockmap, continue */
          continue;

        if (x<minx || x>maxx)       /*  line doesn't touch column */
          continue;

        /*  The cell that contains the intersection point is always added */

        AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j,i);

        /*  if the intersection is at a corner it depends on the slope */
        /*  (and whether the line extends past the intersection) which */
        /*  blocks are hit */

        if (yp==0)        /*  intersection at a corner */
        {
          if (sneg)       /*    \ - blocks x,y-, x-,y */
          {
            if (yb>0 && miny<y)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(yb-1)+j,i);
            if (j>0 && minx<x)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j-1,i);
          }
          else if (spos)  /*    / - block x-,y- */
          {
            if (yb>0 && j>0 && minx<x)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(yb-1)+j-1,i);
          }
          else if (horiz) /*    - - block x-,y */
          {
            if (j>0 && minx<x)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j-1,i);
          }
        }
        else if (j>0 && minx<x) /*  else not at corner: x-,y */
          AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j-1,i);
      }
    }

    /*  For each row, see where the line along its bottom edge, which */
    /*  it contains, intersects the Linedef i. Add i to all the corresponding */
    /*  blocklists. */

    if (!horiz)
    {
      for (j=0;j<nrows;j++)
      {
        /*  intersection of Linedef with y=yorg+(j<<blkshift) */
        /*  (x,y) on Linedef i satisfies: (y-y1)*dx = dy*(x-x1) */
        /*  x = dx*(y-y1)/dy+x1; */

        int y = yorg+(j<<blkshift);       /*  (x,y) is intersection */
        int x = (dx*(y-y1))/dy+x1;
        int xb = (x-xorg)>>blkshift;      /*  block column number */
        int xp = (x-xorg)&blkmask;        /*  x position within block */

        if (xb<0 || xb>ncols-1)   /*  outside blockmap, continue */
          continue;

        if (y<miny || y>maxy)     /*  line doesn't touch row */
          continue;

        /*  The cell that contains the intersection point is always added */

        AddBlockLine(blocklists,blockcount,blockdone,ncols*j+xb,i);

        /*  if the intersection is at a corner it depends on the slope */
        /*  (and whether the line extends past the intersection) which */
        /*  blocks are hit */

        if (xp==0)        /*  intersection at a corner */
        {
          if (sneg)       /*    \ - blocks x,y-, x-,y */
          {
            if (j>0 && miny<y)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(j-1)+xb,i);
            if (xb>0 && minx<x)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*j+xb-1,i);
          }
          else if (vert)  /*    | - block x,y- */
          {
            if (j>0 && miny<y)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(j-1)+xb,i);
          }
          else if (spos)  /*    / - block x-,y- */
          {
            if (xb>0 && j>0 && miny<y)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(j-1)+xb-1,i);
          }
        }
        else if (j>0 && miny<y) /*  else not on a corner: x,y- */
          AddBlockLine(blocklists,blockcount,blockdone,ncols*(j-1)+xb,i);
      }
    }
  }

  /*  Add initial 0 to all blocklists */
  /*  count the total number of lines (and 0's and -1's) */

  memset(blockdone,0,NBlocks*sizeof(int));
  for (i=0,linetotal=0;i<NBlocks;i++)
  {
    AddBlockLine(blocklists,blockcount,blockdone,i,0);
    linetotal += blockcount[i];
  }

  /*  Create the blockmap lump */

  blockmaplump = Z_Malloc(sizeof(*blockmaplump) * (4+NBlocks+linetotal),
                          PU_LEVEL, 0);
  /*  blockmap header */

  blockmaplump[0] = bmaporgx = xorg << FRACBITS;
  blockmaplump[1] = bmaporgy = yorg << FRACBITS;
  blockmaplump[2] = bmapwidth  = ncols;
  blockmaplump[3] = bmapheight = nrows;

  /*  offsets to lists and block lists */

  for (i=0;i<NBlocks;i++)
  {
    linelist_t *bl = blocklists[i];
    long offs = blockmaplump[4+i] =   /*  set offset to block's list */
      (i? blockmaplump[4+i-1] : 4+NBlocks) + (i? blockcount[i-1] : 0);

    /*  add the lines in each block's list to the blockmaplump */
    /*  delete each list node as we go */

    while (bl)
    {
      linelist_t *tmp = bl->next;
      blockmaplump[offs++] = bl->num;
      Z_Free(bl);
      bl = tmp;
    }
  }

  /*  free all temporary storage */

  Z_Free (blocklists);
  Z_Free (blockcount);
  Z_Free (blockdone);
}


void P_LoadBlockMap (int lump)
{
  long count;

  if (M_CheckParm("-blockmap") || (count = W_LumpLength(lump)/2) >= 0x10000)
    P_CreateBlockMap();
  else
    {
      long i;
      short *wadblockmaplump = W_CacheLumpNum (lump, PU_LEVEL);
      blockmaplump = Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, 0);

      /*  killough 3/1/98: Expand wad blockmap into larger internal one, */
      /*  by treating all offsets except -1 as unsigned and zero-extending */
      /*  them. This potentially doubles the size of blockmaps allowed, */
      /*  because Doom originally considered the offsets as always signed. */

      blockmaplump[0] = SHORT(wadblockmaplump[0]);
      blockmaplump[1] = SHORT(wadblockmaplump[1]);
      blockmaplump[2] = (long)(SHORT(wadblockmaplump[2])) & 0xffff;
      blockmaplump[3] = (long)(SHORT(wadblockmaplump[3])) & 0xffff;

      for (i=4 ; i<count ; i++)
        {
          short t = SHORT(wadblockmaplump[i]);          /*  killough 3/1/98 */
          blockmaplump[i] = t == -1 ? -1l : (long) t & 0xffff;
        }

      Z_Free(wadblockmaplump);

      bmaporgx = blockmaplump[0]<<FRACBITS;
      bmaporgy = blockmaplump[1]<<FRACBITS;
      bmapwidth = blockmaplump[2];
      bmapheight = blockmaplump[3];
    }

  /*  clear out mobj chains */
  count = sizeof(*blocklinks)* bmapwidth*bmapheight;
  blocklinks = Z_Malloc (count,PU_LEVEL, 0);
  memset (blocklinks, 0, count);
  blockmap = blockmaplump+4;
}


#else	/* DIYBOOM */


/* P_LoadBlockMap (normal code) */

static void P_LoadBlockMap (int lump)
{
    int		i;
    int		count;

    blockmaplump = W_CacheLumpNum (lump,PU_LEVEL);
    blockmap = blockmaplump+4;
    count = W_LumpLength (lump)/2;

    for (i=0 ; i<count ; i++)
	blockmaplump[i] = SHORT(blockmaplump[i]);

    bmaporgx = blockmaplump[0]<<FRACBITS;
    bmaporgy = blockmaplump[1]<<FRACBITS;
#ifdef __riscos__
    /* just to be on the safe side: extend sign manually */
    bmapwidth = (blockmaplump[2] << 16); bmapwidth >>= 16;
    bmapheight = (blockmaplump[3] << 16); bmapheight >>= 16;
#else
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];
#endif

    /* clear out mobj chains*/
    count = sizeof(*blocklinks)* bmapwidth*bmapheight;
    blocklinks = Z_Malloc (count,PU_LEVEL, 0);
    memset (blocklinks, 0, count);
}

#endif




/* In case something went wrong with the memory-requirements... */
static int P_TryGroupLines(int total)
{
    line_t**		linebuffer;
    line_t**		linebase;
    sector_t*		sector;
    fixed_t		bbox[4];
    int			block;
    int			i, j;
    line_t*		li;

    /* build line tables for each sector	*/
    linebase = Z_Malloc (total*sizeof(line_t*), PU_LEVEL, 0);
    linebuffer = linebase;

    sector = sectors;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
	M_ClearBox (bbox);
	sector->lines = linebuffer;
	li = lines;
	for (j=0 ; j<numlines ; j++, li++)
	{
	    if ((li->frontsector == sector) || ((li->backsector == sector) && (li->backsector != li->frontsector)))
	    {
	        if (linebuffer < linebase + total) *linebuffer = li;
		linebuffer++;
		M_AddToBox (bbox, li->v1->x, li->v1->y);
		M_AddToBox (bbox, li->v2->x, li->v2->y);
	    }
	}
	if (linebuffer - sector->lines != sector->linecount)
	{
	    fprintf(logfile, "P_GroupLines: miscounted by %d\n", (linebuffer - sector->lines) - sector->linecount);
	    sector->linecount = linebuffer - sector->lines;
        }
	/* set the degenmobj_t to the middle of the bounding box*/
	sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
	sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;

	/* adjust bounding box to map blocks*/
	block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block >= bmapheight ? bmapheight-1 : block;
	sector->blockbox[BOXTOP]=block;

	block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block < 0 ? 0 : block;
	sector->blockbox[BOXBOTTOM]=block;

	block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block >= bmapwidth ? bmapwidth-1 : block;
	sector->blockbox[BOXRIGHT]=block;

	block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block < 0 ? 0 : block;
	sector->blockbox[BOXLEFT]=block;
    }

    if (linebuffer > linebase + total)
    {
        Z_Free(linebase);
	return (linebuffer - linebase);
    }
    return 0;
}

/* P_GroupLines*/
/* Builds sector line lists and subsector sector numbers.*/
/* Finds block bounding boxes for sectors.*/

static void P_GroupLines (void)
{
    int			i;
    int			total;
    line_t*		li;
    subsector_t*	ss;
    seg_t*		seg;

    /* look up sector number for each subsector*/
    ss = subsectors;
    for (i=0 ; i<numsubsectors ; i++, ss++)
    {
	seg = &segs[ss->firstline];
	ss->sector = seg->sidedef->sector;
    }

    /* count number of lines in each sector*/
    li = lines;
    total = 0;
    for (i=0 ; i<numlines ; i++, li++)
    {
        if (li->frontsector != NULL)
	{
	    total++;
	    li->frontsector->linecount++;

	    if (li->backsector && li->backsector != li->frontsector)
	    {
	        li->backsector->linecount++;
	        total++;
	    }
	}
    }

    while (total != 0) total = P_TryGroupLines(total);
}



/* P_SetupLevel*/

void
P_SetupLevel
( int		episode,
  int		map,
  int		playermask,
  skill_t	skill)
{
    int		i;
    char	lumpname[9];
    int		lumpnum;

    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    wminfo.partime = 180;
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	players[i].killcount = players[i].secretcount
	    = players[i].itemcount = 0;
    }

    /* Initial height of PointOfView*/
    /* will be set by player think.*/
    players[consoleplayer].viewz = 1;

    /* Make sure all sounds are stopped before Z_FreeTags.*/
    S_Start ();


#if 0 /* UNUSED*/
    if (debugfile)
    {
	Z_FreeTags (PU_LEVEL, MAXINT);
	Z_FileDumpHeap (debugfile);
    }
    else
#endif
	Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);


    /* UNUSED W_Profile ();*/
    P_InitThinkers ();

    /* if working with a devlopment map, reload it*/
    W_Reload ();

    /* find map name*/
    if ( gamemode == commercial)
    {
	if (map<10)
	    sprintf (lumpname,"map0%i", map);
	else
	    sprintf (lumpname,"map%i", map);
    }
    else
    {
	lumpname[0] = 'E';
	lumpname[1] = '0' + episode;
	lumpname[2] = 'M';
	lumpname[3] = '0' + map;
	lumpname[4] = 0;
    }

    if ((lumpnum = W_CheckNumForName (lumpname)) == -1)
	I_Error("Level %s not found.", lumpname);

    leveltime = 0;

    /* note: most of this ordering is important	*/
#ifndef DIYBOOM
    P_LoadBlockMap (lumpnum+ML_BLOCKMAP);
#endif
    P_LoadVertexes (lumpnum+ML_VERTEXES);
    P_LoadSectors (lumpnum+ML_SECTORS);
    P_LoadSideDefs (lumpnum+ML_SIDEDEFS);
    P_LoadLineDefs (lumpnum+ML_LINEDEFS);
    BOOMSTATEMENT(P_LoadSideDefs2(lumpnum+ML_SIDEDEFS);)
    BOOMSTATEMENT(P_LoadLineDefs2();)
    BOOMSTATEMENT(P_LoadBlockMap(lumpnum+ML_BLOCKMAP);)
    P_LoadSubsectors (lumpnum+ML_SSECTORS);
    P_LoadNodes (lumpnum+ML_NODES);
    P_LoadSegs (lumpnum+ML_SEGS);

    rejectmatrix = W_CacheLumpNum (lumpnum+ML_REJECT,PU_LEVEL);
    P_GroupLines ();

    bodyqueslot = 0;
    deathmatch_p = deathmatchstarts;
    P_LoadThings (lumpnum+ML_THINGS);

    /* if deathmatch, randomly spawn the active players*/
    if (deathmatch)
    {
	for (i=0 ; i<MAXPLAYERS ; i++)
	    if (playeringame[i])
	    {
		players[i].mo = NULL;
		G_DeathMatchSpawnPlayer (i);
	    }

    }

    /* clear special respawning que*/
    iquehead = iquetail = 0;

    /* set up world state*/
    P_SpawnSpecials ();

    /* build subsector connect matrix*/
    /*	UNUSED P_ConnectSubsectors ();*/

    /* preload graphics*/
    if (precache)
	R_PrecacheLevel ();

    /*printf ("free memory: 0x%x\n", Z_FreeMemory());*/
}




#define SWAP_DOOMEDNUM(x,y)	h=den_lookup[x].denum; den_lookup[x].denum=den_lookup[y].denum; den_lookup[y].denum=h;\
				h=den_lookup[x].num; den_lookup[x].num=den_lookup[y].num; den_lookup[y].num=h;

static void QuicksortDoomednum(int from, int to)
{
    while (from < to)
    {
 	int i, j, h;

 	j = (from+to)/2;
 	SWAP_DOOMEDNUM(from, j);
 	j=from;
 	for (i=from+1; i<=to; i++)
        {
            if (den_lookup[i].denum < den_lookup[from].denum)
            {
        	j++;
        	SWAP_DOOMEDNUM(i, j);
            }
        }
        SWAP_DOOMEDNUM(from, j);

        if ((j-from) < (to-j))
        {
            QuicksortDoomednum(from, j-1);
            from = j+1;
        }
        else
        {
            QuicksortDoomednum(j+1, to);
            to = j-1;
        }
    }
}


int P_LookupDoomednum(int doomednum)
{
    int pos, step, count;

    pos = (den_number+1)/2; step = (pos+1)/2; count = (pos<<1);

    while (count != 0)
    {
	if (den_lookup[pos].denum == doomednum) return den_lookup[pos].num;
	if (den_lookup[pos].denum < doomednum)
	{
	    pos += step; if (pos >= den_number) pos = den_number-1;
	}
	else
	{
	    pos -= step; if (pos < 0) pos = 0;
	}
	step = (step+1)/2;
	count >>= 1;
    }
    return NUMMOBJTYPES;
}


/* Prepare doomednum lookup for binary search */
void P_InitDoomednum(void)
{
    int i, j;

    for (i=0; i<NUMMOBJTYPES; i++)
    {
	den_lookup[i].denum = mobjinfo[i].doomednum;
	den_lookup[i].num = i;
    }

    QuicksortDoomednum(0, NUMMOBJTYPES-1);

    for (i=0; den_lookup[i].denum == -1; i++) ;
    for (j=0; i<NUMMOBJTYPES; i++, j++)
    {
	den_lookup[j].denum = den_lookup[i].denum;
	den_lookup[j].num = den_lookup[i].num;
    }
    den_number = j;
    /*fprintf(logfile, "Doomednum: %d\n", den_number);*/
}




/* P_Init*/

void P_Init (void)
{
    P_InitDoomednum ();
    P_InitSwitchList ();
    P_InitPicAnims ();
    P_InitIntercepts ();
    P_InitSpechits ();
    R_InitSprites ((const char**)sprnames);
}



