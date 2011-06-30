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

/* Revision 1.3  1997/01/29 20:10*/
/* DESCRIPTION:*/
/*	Preparation of data for rendering,*/
/*	generation of lookups, caching, retrieval by name.*/

/*-----------------------------------------------------------------------------*/


static const char
rcsid[] = "$Id: r_data.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";

#include "i_system.h"
#include "z_zone.h"

#include "m_swap.h"

#include "w_wad.h"

#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"

#include "doomstat.h"
#include "r_sky.h"

#ifdef __riscos__
#include "ROsupport.h"
#endif

#ifdef LINUX
#include  <alloca.h>
#endif
#include <stdlib.h>

#include "r_data.h"


/* Graphics.*/
/* DOOM graphics for walls and sprites*/
/* is stored in vertical runs of opaque pixels (posts).*/
/* A column is composed of zero or more posts,*/
/* a patch or sprite is composed of zero or more columns.*/





/* Texture definition.*/
/* Each texture is composed of one or more patches,*/
/* with patches being lumps stored in the WAD.*/
/* The lumps are referenced by number, and patched*/
/* into the rectangular texture space using origin*/
/* and possibly other attributes.*/

/* No compact alignment on most RISC chips! */
#define MPAT_T_ORGX	0
#define MPAT_T_ORGY	2
#define MPAT_T_PATCH	4
#define MPAT_T_STPDIR	6
#define MPAT_T_CMAP	8
#define MPAT_T_SIZE	10

/* Boy, finding out those alignments really was fun! */
#define MTEX_T_NAME	0
#define MTEX_T_MASKED	8
#define MTEX_T_WIDTH	12
#define MTEX_T_HEIGHT	14
#define MTEX_T_COLDIR	16
#define MTEX_T_PCOUNT	20
#define MTEX_T_PATCHES  22
#define MTEX_T_SIZE	(MTEX_T_PATCHES + MPAT_T_SIZE)

typedef struct
{
    short	originx;
    short	originy;
    short	patch;
    short	stepdir;
    short	colormap;
} mappatch_t;


/* Texture definition.*/
/* A DOOM wall texture is a list of patches*/
/* which are to be combined in a predefined order.*/

typedef struct
{
    char		name[8];
    boolean		masked;
    short		width;
    short		height;
    void		**columndirectory;	/* OBSOLETE*/
    short		patchcount;
    mappatch_t	patches[1];
} maptexture_t;


/* A single patch from a texture definition,*/
/*  basically a rectangular area within*/
/*  the texture rectangle.*/
typedef struct
{
    /* Block origin (allways UL),*/
    /* which has allready accounted*/
    /* for the internal origin of the patch.*/
    int		originx;
    int		originy;
    int		patch;
} texpatch_t;


/* A maptexturedef_t describes a rectangular texture,*/
/*  which is composed of one or more mappatch_t structures*/
/*  that arrange graphic patches.*/
typedef struct
{
    /* Keep name for switch changing, etc.*/
    char	name[8];
    dshort_t	width;
    dshort_t	height;

    /* All the patches[patchcount]*/
    /*  are drawn back to front into the cached texture.*/
    dshort_t	patchcount;
    texpatch_t	patches[1];

} texture_t;



int		firstflat;
int		lastflat;
int		numflats;

/*static int		firstpatch;
static int		lastpatch;
static int		numpatches;*/

int		firstspritelump;
int		lastspritelump;
int		numspritelumps;

int			numtextures;
static texture_t**	textures;


static int*		texturewidthmask;
/* needed for texture pegging*/
fixed_t*		textureheight;
static int*		texturecompositesize;
static dshort_t**	texturecolumnlump;
static unsigned int**	texturecolumnofs;
static byte**		texturecomposite;

/* for global animation*/
int*		flattranslation;
int*		texturetranslation;

/* needed for pre rendering*/
fixed_t*	spritewidth;
fixed_t*	spriteoffset;
fixed_t*	spritetopoffset;

/*
 *  The colormaps read from the WAD are always bytes, so there's no need *
 *  to code it as lighttable_t.
 */
#ifdef DIYBOOM
static int	firstcolormaplump, lastcolormaplump;
int		numcolormaps;
byte**		colormaps;
byte*		fullcolormap;
#else
byte*		colormaps;
#endif



/* MAPTEXTURE_T CACHING*/
/* When a texture is first needed,*/
/*  it counts the number of composite columns*/
/*  required in the texture and allocates space*/
/*  for a column directory and any new columns.*/
/* The directory will simply point inside other patches*/
/*  if there is only one patch in a given column,*/
/*  but any columns with multiple patches*/
/*  will have new column_ts generated.*/




/* use this in case a patch is undefined */
static struct dummy_patch_s {
  patch_t patch;
  column_t column;
} dummyPatch;	/* initialized in R_InitDummyPatch */



/* R_DrawColumnInCache*/
/* Clip and draw a column*/
/*  from a patch into a cached post.*/

static void
R_DrawColumnInCache
( const column_t*	patch,
  byte*		cache,
  int		originy,
  int		cacheheight,
  byte*		marks
)
{
    int		count;
    int		position;
    byte*	source;
    byte*	dest;

    dest = (byte *)cache + 3;

    while (patch->topdelta != 0xff)
    {
	source = (byte *)patch + 3;
	count = patch->length;
	position = originy + patch->topdelta;

	if (position < 0)
	{
	    count += position;
	    position = 0;
	}

	if (position + count > cacheheight)
	    count = cacheheight - position;

	if (count > 0)
	{
	    memcpy (cache + position, source, count);
            memset (marks + position, 0xff, count);
	}
	patch = (column_t *)(  (byte *)patch + patch->length + 4);
    }
}




/* R_GenerateComposite*/
/* Using the texture definition,*/
/*  the composite texture is created from the patches,*/
/*  and each column is cached.*/

/* Medusa fix taken from Boom sources */
static void R_GenerateComposite (int texnum)
{
    byte*		block;
    texture_t*		texture;
    texpatch_t*		patch;
    patch_t*		realpatch;
    unsigned int	patchsize;
    int			x;
    int			x1;
    int			x2;
    int			i;
    column_t*		patchcol;
    dshort_t*		collump;
    unsigned int*	colofs;
    byte*		marks;
    byte*		source;
    byte*		zmarks=NULL;

    texture = textures[texnum];

    block = Z_Malloc (texturecompositesize[texnum],
		      PU_STATIC,
		      &texturecomposite[texnum]);

    if ((marks = (byte*)malloc(texture->width * texture->height)) == NULL)
    {
        zmarks = (byte*)Z_MallocNoAbort(texture->width * texture->height, PU_STATIC, NULL);
        marks = zmarks;
    }
    if (marks == NULL)
        I_Error("R_GenerateComposite: couldn't alloc marks");

    memset(marks, 0, texture->width * texture->height);

    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    /* Composite the columns together.*/
    patch = texture->patches;

    for (i=0 , patch = texture->patches;
	 i<texture->patchcount;
	 i++, patch++)
    {
        if (patch->patch == -1)
	{
	    realpatch = &dummyPatch.patch;
	    patchsize = sizeof(dummyPatch);
	}
        else
	{
            realpatch = W_CacheLumpNum (patch->patch, PU_CACHE);
	    patchsize = (unsigned int)(lumpinfo[patch->patch].size);
	}
	x1 = patch->originx;
	x2 = x1 + SHORT(realpatch->width);

	if (x1<0)
	    x = 0;
	else
	    x = x1;

	if (x2 > texture->width)
	    x2 = texture->width;

	for ( ; x<x2 ; x++)
	{
	  /* Column has multiple patches?*/
	  if (collump[x] < 0)
	  {
            unsigned int pcoloff = LONG(realpatch->columnofs[x-x1]);;

	    if (pcoloff <= patchsize)
	    {
	      patchcol = (column_t *)((byte *)realpatch + pcoloff);
	      R_DrawColumnInCache (patchcol, block + colofs[x], patch->originy, texture->height, marks + x * texture->height);
	    }
	  }
	}
    }

    if ((source = malloc(texture->height)) == NULL)
        I_Error("R_GenerateComposite: couldn't alloc source");
    for (i=0; i<texture->width; i++)
    {
        if (collump[i] == -1)
        {
            column_t *col = (column_t*)(block + colofs[i] - 3);
            const byte* mark = marks + i * texture->height;
            int j = 0;

            memcpy(source, (byte*)col + 3, texture->height);

            while (1)
            {
                while (j < texture->height && !mark[j]) j++;
                if (j >= texture->height)
                {
                    col->topdelta = 0xff;
                    break;
                }
                col->topdelta = (byte)j;
                for (col->length=0; j<texture->height && mark[j]; j++) col->length++;
                memcpy((byte*)col + 3, source + col->topdelta, col->length);
                col = (column_t*)((byte*)col + col->length + 4);
            }
        }
    }
    free(source);
    if (zmarks == NULL) free(marks); else Z_Free(zmarks);

    /* Now that the texture has been built in column cache,*/
    /*  it is purgable from zone memory.*/
    Z_ChangeTag (block, PU_CACHE);
}




/* R_GenerateLookup*/

static void R_GenerateLookup (int texnum)
{
    texture_t*		texture;
    texpatch_t*		patch;
    patch_t*		realpatch;
    int			x;
    int			x1;
    int			x2;
    int			i;
    dshort_t*		collump;
    unsigned int*	colofs;
    int			csize;
    struct {dushort_t patches, posts;} *patchcount;
    unsigned int	lumpsize;

    texture = textures[texnum];

    /* Composited texture not created yet.*/
    texturecomposite[texnum] = 0;

    texturecompositesize[texnum] = 0;
    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    /* Now count the number of columns*/
    /*  that are covered by more than one patch.*/
    /* Fill in the lump / offset, so columns*/
    /*  with only a single patch are all done.*/
    patchcount = malloc(sizeof(*patchcount) * texture->width);

    if (patchcount == NULL)
	I_Error("R_GenerateLookup: Couldn't claim memory for patchcount");

    memset (patchcount, 0, sizeof(*patchcount) * texture->width);

    patch = texture->patches;

    for (i=0 , patch = texture->patches;
	 i<texture->patchcount;
	 i++, patch++)
    {
        if (patch->patch == -1)
        {
	    realpatch = &dummyPatch.patch;
	    lumpsize = sizeof(dummyPatch);
	}
	else
	{
	    realpatch = W_CacheLumpNum (patch->patch, PU_CACHE);
            lumpsize = (unsigned int)lumpinfo[patch->patch].size;
        }
	
	x1 = patch->originx;
	x2 = x1 + SHORT(realpatch->width);

	if (x1 < 0)
	    x = 0;
	else
	    x = x1;

	if (x2 > texture->width)
	    x2 = texture->width;

	for ( ; x<x2 ; x++)
	{
            /* to fix Medusa bug */
            const column_t *col;
            unsigned int ofs = LONG(realpatch->columnofs[x-x1]);

            /* safety */
            if (ofs < lumpsize)
            {
                col = (column_t*)((byte*)realpatch + LONG(realpatch->columnofs[x-x1]));
                for (; col->topdelta != 0xff; patchcount[x].posts++)
                    col = (column_t*)((byte*)col + col->length + 4);
                patchcount[x].patches++;
            }
	    collump[x] = patch->patch;
	    colofs[x] = LONG(realpatch->columnofs[x-x1])+3;
	}
    }

    texturecomposite[texnum] = 0;

    for (x=0, csize=0; x<texture->width ; x++)
    {
        int	pcnt;

        pcnt = patchcount[x].patches;
	if (!pcnt)
	{
	    fprintf (logfile, "R_GenerateLookup: column without a patch (%s)\n",
		    texture->name);
	    free (patchcount);
	    return;
	}
	/* I_Error ("R_GenerateLookup: column without a patch");*/

	if (pcnt > 1)
	{
	    /* Use the cached block.*/
	    collump[x] = -1;
            colofs[x] = csize + 3;
            csize += 4*patchcount[x].posts + 1;
            /* changed texturecolumnofs, texture may safely be > 64k now */

	    csize += texture->height;
	}
	texturecompositesize[texnum] = csize;
    }
    free (patchcount);
}





/* R_GetColumn*/

BOOMSTATEMENT(static byte dummyColumn[256];)

byte *R_GetColumn(int tex, int col)
{
    int		lump;
    dushort_t	ofs;

    col &= texturewidthmask[tex];
    lump = texturecolumnlump[tex][col];
    ofs = texturecolumnofs[tex][col];

    if (lump > 0)
    {
        /* FIXME Boom (for now; find the real reason later) */
        BOOMSTATEMENT(if (ofs >= lumpinfo[lump].size) return dummyColumn;)
	return (byte *)W_CacheLumpNum(lump,PU_CACHE)+ofs;
    }

    if (!texturecomposite[tex])
	R_GenerateComposite (tex);

    return texturecomposite[tex] + ofs;
}


int R_TextureWidthMask(int tex)
{
  return texturewidthmask[tex];
}





/* R_InitTextures*/
/* Initializes the texture list*/
/*  with the textures from the world map.*/

static void R_InitTextures (void)
{
    byte*               mtexture;
    byte*               mpatch;
    texture_t*		texture;
    texpatch_t*		patch;

    int			i;
    int			j;

    int*		maptex;
    int*		maptex2;
    int*		maptex1;

    char		name[9];
    char*		names;
    char*		name_p;

    int*		patchlookup;

    int			totalwidth;
    int			nummappatches;
    int			offset;
    int			maxoff;
    int			maxoff2;
    int			numtextures1;
    int			numtextures2;

    int*		directory;

    int			temp1;
    int			temp2;
    int			temp3;


    /* Load the patch names from pnames.lmp.*/
    name[8] = 0;
    temp1 = W_GetNumForName("PNAMES");
    names = W_CacheLumpNum(temp1, PU_STATIC);
    nummappatches = LONG ( *((int *)names) );
    temp2 = (lumpinfo[temp1].size - 4) / 8;
    if (nummappatches != temp2)
    {
      fprintf(logfile, "R_InitTextures: PNAMES lump size %d, should be %d\n", nummappatches, temp2);
      fflush(logfile);
      if (nummappatches > temp2)
        nummappatches = temp2;
    }

    name_p = names+4;

    if ((patchlookup = malloc (nummappatches*sizeof(*patchlookup))) == NULL)
	I_Error("R_InitTextures: Couldn't claim memory for patchlookup");

    for (i=0 ; i<nummappatches ; i++)
    {
	strncpy (name,name_p+i*8, 8);
	patchlookup[i] = W_CheckNumForName2(name, ns_sprites);
    }
    Z_Free (names);

    /* Load the map texture definitions from textures.lmp.*/
    /* The data is contained in one or two lumps,*/
    /*  TEXTURE1 for shareware, plus TEXTURE2 for commercial.*/
    maptex = maptex1 = W_CacheLumpName ("TEXTURE1", PU_STATIC);
    numtextures1 = LONG(*maptex);
    maxoff = W_LumpLength (W_GetNumForName ("TEXTURE1"));
    directory = maptex+1;

    if (W_CheckNumForName ("TEXTURE2") != -1)
    {
	maptex2 = W_CacheLumpName ("TEXTURE2", PU_STATIC);
	numtextures2 = LONG(*maptex2);
	maxoff2 = W_LumpLength (W_GetNumForName ("TEXTURE2"));
    }
    else
    {
	maptex2 = NULL;
	numtextures2 = 0;
	maxoff2 = 0;
    }
    numtextures = numtextures1 + numtextures2;

    textures = Z_Malloc (numtextures*sizeof(texture_t*), PU_STATIC, NULL);
    texturecolumnlump = Z_Malloc (numtextures*sizeof(dshort_t*), PU_STATIC, NULL);
    texturecolumnofs = Z_Malloc (numtextures*sizeof(unsigned int*), PU_STATIC, NULL);
    texturecomposite = Z_Malloc (numtextures*sizeof(byte*), PU_STATIC, NULL);
    texturecompositesize = Z_Malloc (numtextures*sizeof(int), PU_STATIC, NULL);
    texturewidthmask = Z_Malloc (numtextures*sizeof(int), PU_STATIC, NULL);
    textureheight = Z_Malloc (numtextures*sizeof(fixed_t), PU_STATIC, NULL);

    totalwidth = 0;

    /*	Really complex printing shit...*/
    temp1 = W_GetNumForName ("S_START");  /* P_???????*/
    temp2 = W_GetNumForName ("S_END") - 1;
    temp3 = ((temp2-temp1+63)/64) + ((numtextures+63)/64);
    printf("[");
#ifndef __riscos__
    for (i = 0; i < temp3; i++)
	printf(" ");
    printf("         ]");
    for (i = 0; i < temp3; i++)
	printf("\x8");
    printf("\x8\x8\x8\x8\x8\x8\x8\x8\x8\x8");
#endif

    for (i=0 ; i<numtextures ; i++, directory++)
    {
	if (!(i&63))
	    printf (".");

	if (i == numtextures1)
	{
	    /* Start looking in second texture file.*/
	    maptex = maptex2;
	    maxoff = maxoff2;
	    directory = maptex+1;
	}

	offset = LONG(*directory);

	if (offset > maxoff)
	    I_Error ("R_InitTextures: bad texture directory");

	mtexture = (byte*) maptex + offset;

	texture = textures[i] =
	    Z_Malloc (sizeof(texture_t)
	    	      + sizeof(texpatch_t)*(READ_UA_SHORT(mtexture+MTEX_T_PCOUNT)-1),
                      PU_STATIC, NULL);

	/* texture->width/height/patchcount are short, so no sign extend necessary */
	texture->width = READ_UA_SHORT(mtexture + MTEX_T_WIDTH);
	texture->height = READ_UA_SHORT(mtexture + MTEX_T_HEIGHT);
	texture->patchcount = READ_UA_SHORT(mtexture + MTEX_T_PCOUNT);

	memcpy (texture->name, mtexture + MTEX_T_NAME, sizeof(texture->name));
	patch = &texture->patches[0];

	mpatch = mtexture + MTEX_T_PATCHES;
	for (j=0 ; j<texture->patchcount ; j++, mpatch += MPAT_T_SIZE, patch++)
	{
	    unsigned short pnr;
	    patch->originx = READ_UA_SHORT(mpatch + MPAT_T_ORGX) << 16;
	    patch->originy = READ_UA_SHORT(mpatch + MPAT_T_ORGY) << 16;
	    patch->patch = -1;
	    pnr = READ_UA_SHORT(mpatch + MPAT_T_PATCH);
	    if (pnr >= nummappatches)
	    {
	        char tname[9];
		strncpy(tname, texture->name, 8);
		tname[8] = 0;
	        fprintf(logfile, "Illegal patch number %d in texture %s col %d/%d\n", pnr, tname, j, texture->patchcount);
		fflush(logfile);
	    }
	    else
	    {
	        patch->patch = patchlookup[pnr];
	    }
	    /* These are signed and copied to int!!! */
	    patch->originx >>= 16; patch->originy >>= 16;
	    if (patch->patch == -1)
	    {
		fprintf(logfile, "R_InitTextures: Missing patch in texture %s\n", texture->name);
		fflush(logfile);
	    }
	}
	texturecolumnlump[i] = Z_Malloc (texture->width*sizeof(dshort_t), PU_STATIC, NULL);
	texturecolumnofs[i] = Z_Malloc (texture->width*sizeof(unsigned int), PU_STATIC, NULL);
        /* init for safety */
        memset(texturecolumnlump[i], 0, texture->width * sizeof(dshort_t));
        memset(texturecolumnofs[i], 0, texture->width * sizeof(unsigned int));

	j = 1;
	while (j*2 <= texture->width)
	    j<<=1;

	texturewidthmask[i] = j-1;
	textureheight[i] = texture->height<<FRACBITS;

	totalwidth += texture->width;
    }

    Z_Free (maptex1);
    if (maptex2)
	Z_Free (maptex2);

    /* Precalculate whatever possible.	*/
    for (i=0 ; i<numtextures ; i++)
	R_GenerateLookup (i);

    /* Create translation table for global animation.*/
    texturetranslation = Z_Malloc ((numtextures+1)*sizeof(int), PU_STATIC, NULL);

    for (i=0 ; i<numtextures ; i++)
	texturetranslation[i] = i;

    free (patchlookup);
}




/* R_InitFlats*/

static void R_InitFlats (void)
{
    int		i;

    firstflat = W_GetNumForName ("F_START") + 1;
    lastflat = W_GetNumForName ("F_END") - 1;
    numflats = lastflat - firstflat + 1;

    /* Modified flats? Then update lumpinfo */
    if (modifiedgame)
    {
	lumpinfo_t	*actual;
	lumpinfo_t	*physical;

	for (i=0; i<numflats; i++)
	{
	    physical = lumpinfo + (firstflat+i);
	    actual = lumpinfo + W_GetNumForName2(physical->name,ns_flats);
	    memcpy(physical, actual, sizeof(lumpinfo_t));
	}
    }

    /* Create translation table for global animation.*/
    if ((flattranslation = malloc((numflats+1)*sizeof(int))) == NULL)
	flattranslation = Z_Malloc ((numflats+1)*sizeof(int), PU_STATIC, NULL);

    for (i=0 ; i<numflats ; i++)
	flattranslation[i] = i;
}



/* R_InitSpriteLumps*/
/* Finds the width and hoffset of all sprites in the wad,*/
/*  so the sprite does not need to be cached completely*/
/*  just for having the header info ready during rendering.*/

static void R_InitSpriteLumps (void)
{
    int		i;
    patch_t	*patch;

    firstspritelump = W_GetNumForName ("S_START") + 1;
    lastspritelump = W_GetNumForName ("S_END") - 1;

    numspritelumps = lastspritelump - firstspritelump + 1;
    /* Try to put these into normal memory */
    if ((spritewidth = malloc(numspritelumps*sizeof(fixed_t))) == NULL)
	spritewidth = Z_Malloc(numspritelumps*sizeof(fixed_t), PU_STATIC, NULL);
    if ((spriteoffset = malloc(numspritelumps*sizeof(fixed_t))) == NULL)
	spriteoffset = Z_Malloc(numspritelumps*sizeof(fixed_t), PU_STATIC, NULL);
    if ((spritetopoffset = malloc(numspritelumps*sizeof(fixed_t))) == NULL)
	spritetopoffset = Z_Malloc(numspritelumps*sizeof(fixed_t), PU_STATIC, NULL);

    for (i=0 ; i< numspritelumps ; i++)
    {
	if (!(i&63))
	    printf (".");

	if (modifiedgame)
	{
	    int		actual_lump;
	    lumpinfo_t	*physical;

	    /* This seems to be the only approach that works for modified sprites */
	    physical = lumpinfo + (firstspritelump+i);
	    actual_lump = W_GetNumForName2(physical->name, ns_sprites);
	    memcpy(physical, lumpinfo + actual_lump, sizeof(lumpinfo_t));
	    patch = W_CacheLumpNum (actual_lump, PU_CACHE);
	}
	else
	    patch = W_CacheLumpNum (firstspritelump+i, PU_CACHE);

	spritewidth[i] = SHORT(patch->width)<<FRACBITS;
	spriteoffset[i] = SHORT(patch->leftoffset)<<FRACBITS;
	spritetopoffset[i] = SHORT(patch->topoffset)<<FRACBITS;
    }
}




/* R_InitColormaps*/

static void R_InitColormaps (void)
{
    int lump;

#ifdef DIYBOOM
    firstcolormaplump = W_GetNumForName("C_START");
    lastcolormaplump  = W_GetNumForName("C_END");
    numcolormaps = lastcolormaplump - firstcolormaplump;
    colormaps = Z_Malloc(sizeof(*colormaps) * numcolormaps, PU_STATIC, NULL);
    colormaps[0] = W_CacheLumpNum(W_GetNumForName("COLORMAP"), PU_STATIC);
    for (lump=1; lump<numcolormaps; lump++)
      colormaps[lump] = W_CacheLumpNum(lump + firstcolormaplump, PU_STATIC);
    /* Just to make sure this is initialized */
    fullcolormap = colormaps[0];
#else
    int	length;

    /* Load in the light tables, */
    /*  256 byte align tables.*/
    lump = W_GetNumForName("COLORMAP");
    length = W_LumpLength (lump) + 255;
    colormaps = Z_Malloc (length, PU_STATIC, NULL);
    colormaps = (byte *)( ((int)colormaps + 255)&~0xff);
    W_ReadLump (lump,colormaps);
#endif
}


#ifdef DIYBOOM
int R_ColormapNumForName(const char *name)
{
  int lump=0;

  if (strncasecmp(name, "COLORMAP", 8))
  {
    if ((lump = (W_CheckNumForName)(name, ns_colormaps)) != -1)
      lump -= firstcolormaplump;
  }
  return lump;
}
#endif



static void R_InitDummyPatch(void)
{
    unsigned int i, off;
    patch_t *patch = &dummyPatch.patch;
    column_t *column = &dummyPatch.column;
    
    /* the dummy patch must pretend to be in external endianness! */
    patch->width = SHORT(1);
    patch->height = SHORT(1);
    patch->leftoffset = SHORT(0);
    patch->topoffset = SHORT(0);
    off = (byte*)(&dummyPatch.column) - (byte*)(&dummyPatch.patch);
    for (i=0; i<8; i++)
        dummyPatch.patch.columnofs[i] = LONG(off);

    column->topdelta = 0xff;
    column->length = 0x00;
    
    /*printf("Size: %d\n", sizeof(dummyPatch));
    for (i=0; i<sizeof(dummyPatch); i++)
      printf("%2d: %2x\n", i, ((byte*)&dummyPatch)[i]);*/
}


/* R_InitData*/
/* Locates all the lumps*/
/*  that will be used by all views*/
/* Must be called after W_Init.*/

void R_InitData (void)
{
    R_InitDummyPatch();
    R_InitTextures ();
    printf ("\nInitTextures");
    R_InitFlats ();
    printf ("\nInitFlats");
    R_InitSpriteLumps ();
    printf ("\nInitSprites");
    R_InitColormaps ();
    printf ("\nInitColormaps");
    BOOMSTATEMENT(memset(dummyColumn, 0xff, 256);)	/* fixme Boom */

    /*printf("Texture dump:\n");
    {
      int i;
      char namet[9];

      namet[8] = 0;
      for (i=0; i<numtextures; i++)
      {
        memcpy(namet, textures[i]->name, 8); printf("<%s>\n", namet);
      }
    }*/
}




/* R_FlatNumForName*/
/* Retrieval, get a flat number for a flat name.*/

int R_FlatNumForName (const char* name)
{
    int i = W_CheckNumForName2(name, ns_flats);

    if (i == -1)
    {
        char	namet[9];

        namet[8] = 0;
	memcpy (namet, name, 8);
	I_Error ("R_FlatNumForName: %s not found",namet);
    }
    return i - firstflat;
}





/* R_CheckTextureNumForName*/
/* Check whether texture is available.*/
/* Filter out NoTexture indicator.*/

int	R_CheckTextureNumForName (const char *name)
{
    int		i;

    /* "NoTexture" marker.*/
    if (name[0] == '-')
	return 0;

    for (i=0 ; i<numtextures ; i++)
	if (!strncasecmp (textures[i]->name, name, 8) )
	    return i;

    return -1;
}




/* R_TextureNumForName*/
/* Calls R_CheckTextureNumForName,*/
/*  aborts with error message.*/

int	R_TextureNumForName (const char* name)
{
    int		i;

    i = R_CheckTextureNumForName (name);

    if (i==-1)
    {
        char        namet[9];

        /* StarWars fixes */
        if (!strncasecmp(name, "COMPWERD", 8)) return R_TextureNumForName("COMPTALL");
        if (!strncasecmp(name, "GRAYBIG", 8)) return R_TextureNumForName("GRAYTALL");
        if (!strncasecmp(name, "GRAY1", 8)) return R_TextureNumForName("GRAY4");
        if (!strncasecmp(name, "METAL", 8)) return R_TextureNumForName("METAL1");
        if (!strncasecmp(name, "COMPBLUE", 8)) return R_TextureNumForName("COMPTILE");
        if (!strncasecmp(name, "LITERED", 8)) return R_TextureNumForName("REDWALL1");
        if (!strncasecmp(name, "GRAY2", 8)) return R_TextureNumForName("GRAY5");
        if (!strncasecmp(name, "CRATE1", 8)) return R_TextureNumForName("SW1STON1");
        if (!strncasecmp(name, "CRATE2", 8)) return R_TextureNumForName("SW1STON2");
        if (!strncasecmp(name, "CRATELIT", 8)) return R_TextureNumForName("SW1STONE");
        if (!strncasecmp(name, "CRATWIDE", 8)) return R_TextureNumForName("SW1STRTN");

        namet[8] = 0;
        memcpy(namet, name, 8);
	/*I_Error ("R_TextureNumForName: %s not found", namet);*/
        fprintf(logfile, "R_TextureNumForName: %s not found\n", namet); i = 0;
    }
    return i;
}





/* R_PrecacheLevel*/
/* Preloads all relevant graphics for the level.*/

/*
 *  Rewrote quite a bit of it: introduced rangechecks to avoid certain IWADs
 *  bombing out your machine, also packed the boolean arrays into bitfields and
 *  check whether pre-caching is a good idea in the first place (if you Z_Zone
 *  chunk is smaller than the total memory requirements of the level pre-cacheing
 *  only slows you down a lot).
 */
static int		flatmemory;
static int		texturememory;
static int		spritememory;

/* Size of Z_Zone, from i_system.c */

void R_PrecacheLevel (void)
{
    unsigned char*	flatpresent;
    unsigned char*	texturepresent;
    unsigned char*	spritepresent;

    int			i;
    int			j;
    int			k;
    int			lump;
    int			idx;

    texture_t*		texture;
    thinker_t*		th;
    spriteframe_t*	sf;

    char		memerror[] = "R_PrecacheLevel: Not enough memory";
    int			memfrom;

    if (demoplayback)
	return;

    memfrom = 0;

    /* Precache flats.*/
    if ((flatpresent = (unsigned char*)malloc((numflats+7)>>3)) == NULL)
    {
	memfrom |= 1;
	if ((flatpresent = (unsigned char*)Z_MallocNoAbort((numflats+7)>>3, PU_STATIC, NULL)) == NULL)
	    I_Error(memerror);
    }

    memset (flatpresent,0,(numflats+7)>>3);

    for (i=0 ; i<numsectors ; i++)
    {
	if ((idx = sectors[i].floorpic) < numflats)
	    flatpresent[idx>>3] |= (1 << (idx & 7));
	if ((idx = sectors[i].ceilingpic) < numflats)
	    flatpresent[idx>>3] |= (1 << (idx & 7));
    }

    flatmemory = 0;

    /* First check the total memory requirements before cacheing anything */
    for (i=0 ; i<numflats ; i++)
    {
	if ((flatpresent[i>>3] & (1<<(i&7))) != 0)
	    flatmemory += lumpinfo[firstflat + i].size;
    }

    /* Precache textures.*/
    if ((texturepresent = (unsigned char*)malloc((numtextures+7)>>3)) == NULL)
    {
	memfrom |= 2;
	if ((texturepresent = (unsigned char*)Z_MallocNoAbort((numtextures+7)>>3, PU_STATIC, NULL)) == NULL)
	    I_Error(memerror);
    }

    memset (texturepresent,0, (numtextures+7)>>3);

    for (i=0 ; i<numsides ; i++)
    {
	if ((idx = sides[i].toptexture) < numtextures)
	    texturepresent[idx>>3] |= (1 << (idx & 7));
	if ((idx = sides[i].midtexture) < numtextures)
	    texturepresent[idx>>3] |= (1 << (idx & 7));
	if ((idx = sides[i].bottomtexture) < numtextures)
	    texturepresent[idx>>3] |= (1 << (idx & 7));
    }

    /* Sky texture is always present.*/
    /* Note that F_SKY1 is the name used to*/
    /*  indicate a sky floor/ceiling as a flat,*/
    /*  while the sky texture is stored like*/
    /*  a wall texture, with an episode dependend*/
    /*  name.*/
    texturepresent[skytexture>>3] |= (1 << (skytexture & 7));

    texturememory = 0;

    for (i=0 ; i<numtextures ; i++)
    {
	if ((texturepresent[i>>3] & (1<<(i&7))) != 0)
	{
	    texture = textures[i];

	    for (j=0 ; j<texture->patchcount ; j++)
	    {
		lump = texture->patches[j].patch;
		if (lump != -1)
		    texturememory += lumpinfo[lump].size;
	    }
	}
    }

    /* Precache sprites.*/
    if ((spritepresent = (unsigned char*)malloc((numsprites+7)>>3)) == NULL)
    {
	memfrom |= 4;
	if ((spritepresent = (unsigned char*)Z_MallocNoAbort((numsprites+7)>>3, PU_STATIC, NULL)) == NULL)
	    I_Error(memerror);
    }

    memset (spritepresent,0, (numsprites+7)>>3);

    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
	if (th->function.acp1 == (actionf_p1)P_MobjThinker)
	{
	    if ((idx = ((mobj_t*)th)->sprite) < numsprites)
		spritepresent[idx>>3] |= (1 << (idx & 7));
	}
    }

    spritememory = 0;

    for (i=0 ; i<numsprites ; i++)
    {
	if ((spritepresent[i>>3] & (1<<(i&7))) != 0)
	{
	    for (j=0 ; j<sprites[i].numframes ; j++)
	    {
		sf = &sprites[i].spriteframes[j];
		for (k=0 ; k<8 ; k++)
		{
		    lump = firstspritelump + sf->lump[k];
		    spritememory += lumpinfo[lump].size;
		}
	    }
	}
    }

    fprintf(logfile, "R_PrecacheLevel: Memory requirements: %dkB...",
	(flatmemory + texturememory + spritememory + 1023) >> 10);

    /* Only cache if we have enough room for cacheing in the first place. */
    if (flatmemory + texturememory + spritememory <= (kb_used << 10))
    {
	fprintf(logfile, " cacheing.");

	for (i=0 ; i<numflats ; i++)
	{
	    if ((flatpresent[i>>3] & (1<<(i&7))) != 0)
	    {
		W_CacheLumpNum(firstflat + i, PU_CACHE);
	    }
	}

	for (i=0 ; i<numtextures ; i++)
	{
	    if ((texturepresent[i>>3] & (1<<(i&7))) != 0)
	    {
		texture = textures[i];

		for (j=0 ; j<texture->patchcount ; j++)
		{
		    lump = texture->patches[j].patch;
		    if (lump != -1)
		        W_CacheLumpNum(lump , PU_CACHE);
		}
	    }
	}

	for (i=0 ; i<numsprites ; i++)
	{
	    if ((spritepresent[i>>3] & (1<<(i&7))) != 0)
	    {
		for (j=0 ; j<sprites[i].numframes ; j++)
		{
		    sf = &sprites[i].spriteframes[j];
		    for (k=0 ; k<8 ; k++)
		    {
			lump = firstspritelump + sf->lump[k];
			W_CacheLumpNum(lump , PU_CACHE);
		    }
		}
	    }
	}
    }
    fprintf(logfile, "\n");

    if ((memfrom & 4) == 0) free (spritepresent); else Z_Free(spritepresent);
    if ((memfrom & 2) == 0) free (texturepresent); else Z_Free(texturepresent);
    if ((memfrom & 1) == 0) free (flatpresent); else Z_Free(flatpresent);
}




