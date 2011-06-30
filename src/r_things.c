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
/*	Refresh of things, i.e. objects represented by sprites.*/

/*-----------------------------------------------------------------------------*/


static const char
rcsid[] = "$Id: r_things.c,v 1.5 1997/02/03 16:47:56 b1 Exp $";


#include <stdio.h>
#include <stdlib.h>


#include "doomdef.h"
#include "m_swap.h"

#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"
#include "r_draw.h"
#include "r_context.h"
#include "st_stuff.h"

#include "doomstat.h"


#ifdef __riscos__
#include "ROsupport.h"
#endif



/* Define to use a faster method for R_DrawSprite() */
#define SORT_DRAWSEGS



#define MINZ				(FRACUNIT*4)
#define BASEYCENTER			100

/*void R_DrawColumn (void);*/
/*void R_DrawFuzzColumn (void);*/

typedef void (*rdraw_fn)(draw_context_t*);


typedef struct
{
    int		x1;
    int		x2;

    int		column;
    int		topclip;
    int		bottomclip;

} maskdraw_t;




/* Sprite rotation 0 is facing the viewer,*/
/*  rotation 1 is one angle turn CLOCKWISE around the axis.*/
/* This is not the same as the angle,*/
/*  which increases counter clockwise (protractor).*/
/* There was a lot of stuff grabbed wrong, so I changed it...*/

fixed_t		pspritescale;
fixed_t		pspriteiscale;

static lighttable_t**	spritelights;

/* constant arrays*/
/*  used for psprite clipping and initializing clipping*/
#ifdef STATIC_RESOLUTION
dshort_t	negonearray[SCREENWIDTH];
dshort_t	screenheightarray[SCREENWIDTH];
#else
dshort_t	negonearray[MAXSCREENWIDTH];
dshort_t	screenheightarray[MAXSCREENWIDTH];
#endif



/* INITIALIZATION FUNCTIONS*/


/* variables used to look up*/
/*  and range check thing_t sprites patches*/
spritedef_t*	sprites = NULL;
int		numsprites = 0;

static spriteframe_t	sprtemp[29];
static int		maxframe;
static const char*	spritename;





/* R_InstallSpriteLump*/
/* Local function for R_InitSprites.*/

static void
R_InstallSpriteLump
( int		lump,
  unsigned	frame,
  unsigned	rotation,
  boolean	flipped )
{
    int		r;
    char	isl_error[256];

    if (frame >= 29 || rotation > 8)
    {
	char	lumpname[9];

	strncpy(lumpname, lumpinfo[lump].name, 8); lumpname[8] = 0;
	fprintf(logfile, "R_InstallSpriteLump: Bad frame characters in lump %s\n", lumpname);
	return;
    }

    if ((int)frame > maxframe)
	maxframe = frame;

    if (rotation == 0)
    {
	/* the lump should be used for all rotations*/
	if (((int)sprtemp[frame].rotate) == 0)
	{
	    sprintf(isl_error, "R_InitSprites: Sprite %s frame %c has multip rot=0 lump", spritename, 'A'+frame);
	    /*I_Error(isl_error);*/
	    /*fprintf(logfile, "%s\n", isl_error);*/
	    return;
	}

	if (((int)sprtemp[frame].rotate) == 1)
	{
	    sprintf(isl_error, "R_InitSprites: Sprite %s frame %c has rotations and a rot=0 lump", spritename, 'A'+frame);
	    /*I_Error(isl_error);*/
	    /*fprintf(logfile, "%s\n", isl_error);*/
	    return;
	}

	sprtemp[frame].rotate = false;
	for (r=0 ; r<8 ; r++)
	{
	    sprtemp[frame].lump[r] = lump - firstspritelump;
	    sprtemp[frame].flip[r] = (byte)flipped;
	}
	return;
    }

    /* the lump is only used for one rotation*/
    if (((int)sprtemp[frame].rotate) == 0)
    {
	sprintf(isl_error, "R_InitSprites: Sprite %s frame %c has rotations and a rot=0 lump", spritename, 'A'+frame);
	/*I_Error(isl_error);*/
	/*fprintf(logfile, "%s\n", isl_error);*/
	return;
    }

    /*sprtemp[frame].rotate = true;*/

    /* make 0 based*/
    rotation--;
    if (sprtemp[frame].lump[rotation] != -1)
    {
	sprintf(isl_error, "R_InitSprites: Sprite %s : %c : %c has two lumps mapped to it", spritename, 'A'+frame, '1'+rotation);
	/*I_Error(isl_error);*/
	/*fprintf(logfile, "%s\n", isl_error);*/
	return; /* don't report error */
    }

    sprtemp[frame].rotate = true;

    sprtemp[frame].lump[rotation] = lump - firstspritelump;
    sprtemp[frame].flip[rotation] = (byte)flipped;
}





/* R_InitSpriteDefs*/
/* Pass a null terminated list of sprite names*/
/*  (4 chars exactly) to be used.*/
/* Builds the sprite rotation matrixes to account*/
/*  for horizontally flipped sprites.*/
/* Will report an error if the lumps are inconsistant. */
/* Only called at startup.*/

/* Sprite lump names are 4 characters for the actor,*/
/*  a letter for the frame, and a number for the rotation.*/
/* A sprite that is flippable will have an additional*/
/*  letter/number appended.*/
/* The rotation character can be 0 to signify no rotations.*/

static void R_InitSpriteDefs (const char** namelist)
{
    const char** check;
    int		i;
    int		l;
    int		intname;
    int		frame;
    int		rotation;
    int		start;
    int		end;

    /* count the number of sprite names*/
    check = namelist;
    while (*check != NULL)
	check++;

    numsprites = check-namelist;

    if (!numsprites)
	return;

    sprites = Z_Malloc(numsprites *sizeof(*sprites), PU_STATIC, NULL);
    start = firstspritelump-1;
    end = lastspritelump+1;

    /* scan all the lump names for each of the names,*/
    /*  noting the highest frame letter.*/
    /* Just compare 4 characters as ints*/
    for (i=0 ; i<numsprites ; i++)
    {
	spritename = namelist[i];
	memset (sprtemp,-1, sizeof(sprtemp));

	maxframe = -1;
	intname = *(int *)namelist[i];

	/* scan the lumps,*/
	/*  filling in the frames for whatever is found*/
	for (l=end-1 ; l>start ; l--)
	{
	    if (*(int *)lumpinfo[l].name == intname)
	    {
		frame = lumpinfo[l].name[4] - 'A';
		rotation = lumpinfo[l].name[5] - '0';

		/*
		 *  sprites in patchlumps are now handled by updating the lumpinfo of
		 *  the original lumps (R_InitSpriteLumps). This works because it
		 *  ensures for all sprite firstspritelump <= sprlump <= lastspritelump.
		 */
		R_InstallSpriteLump (l, frame, rotation, false);

		if (lumpinfo[l].name[6] != 0)
		{
		    frame = lumpinfo[l].name[6] - 'A';
		    rotation = lumpinfo[l].name[7] - '0';
		    R_InstallSpriteLump (l, frame, rotation, true);
		}
	    }
	}

	/* check the frames that were found for completeness*/
	if (maxframe == -1)
	{
	    sprites[i].numframes = 0;
	    continue;
	}

	maxframe++;

	for (frame = 0 ; frame < maxframe ; frame++)
	{
	    switch ((int)sprtemp[frame].rotate)
	    {
	      case 0:
		/* only the first rotation is needed*/
		break;

	      case 1:
	        {
		    /* must have all 8 frames*/
		    for (rotation=0 ; rotation<8 ; rotation++)
		        if (sprtemp[frame].lump[rotation] == -1)
			    break;

                    if (rotation<8)
		    {
		        int j;

			fprintf(logfile, "R_InitSprites: Sprite %s frame %c "
			        "is missing rotations, fixing...", namelist[i], frame+'A');
			if (rotation == 0)
			{
			    for (rotation=0; rotation<8; rotation++)
			    {
			        if (sprtemp[frame].lump[rotation] != -1)
			            break;
			    }
			    if (rotation >= 8)
			    {
			        fprintf(logfile, " no rotations found!\n");
			    }
			    else
			    {
				for (j=rotation-1; j>=0; j--)
				{
				    sprtemp[frame].lump[j] = sprtemp[frame].lump[rotation];
				    fprintf(logfile, " [%d=%d]", j, rotation);
				}
			    }
			}
			else
			{
			    rotation--;
			}
			/* now rotation points to the first valid rotation */
			for (j=rotation+1; j<8; j++)
			{
			    if (sprtemp[frame].lump[j] == -1)
			    {
			        sprtemp[frame].lump[j] = sprtemp[frame].lump[rotation];
				fprintf(logfile, " [%d=%d]", j, rotation);
			    }
			    else
			        rotation = j;
			}
			fprintf(logfile, "\n");
		    }
                }
		break;

	      default:
		/* no rotations were found for that frame at all*/
		I_Error ("R_InitSprites: No patches found "
			 "for %s frame %c", namelist[i], frame+'A');
		break;
	    }
	}

	/* allocate space for the frames present and copy sprtemp to it*/
	sprites[i].numframes = maxframe;
	sprites[i].spriteframes =
	    Z_Malloc (maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
	memcpy (sprites[i].spriteframes, sprtemp, maxframe*sizeof(spriteframe_t));
    }

}





/* GAME FUNCTIONS*/

#define DEFAULT_VISSPRITES	64

vissprite_t*	vissprites=NULL;
vissprite_t*	vissprite_p=NULL;
static int	maxvissprites=0;
/*static int	newvissprite;*/


#ifdef SORT_DRAWSEGS
typedef struct sorted_min_s {
  dshort_t	local;
  dshort_t	global;
} sorted_min_t;

static int	numsortsegs=0;
static int	incsortsegs=256;

static dshort_t*	sortedsegs;
static dshort_t*	sortednext;
static sorted_min_t*	sortedmins;
static dshort_t*	dsegstore = NULL;

/*
 *  bucketsize is (1 << bucketshift) pixels
 *  This is the biggest problem under RISC OS machines due to the pathetic memory
 *  bandwidth. On less restrictive architectures you can't go far wrong setting
 *  this parameter to 0, but changing it on RISC OS can easily backfire. Make the
 *  buckets too small and you'll have to search through more drawsegs to find the
 *  ones intersecting the sprites, make them too big and you'll put too much stress
 *  on the cache and performance will drop again. This factor was found optimum by
 *  timing various heavy-duty demos.
 */
#ifdef __riscos__
static const int	bucketshift = 3;
#else
static const int	bucketshift = 0;
#endif
#endif



/* R_InitSprites*/
/* Called at program start.*/

void R_InitSprites (const char** namelist)
{
    int		i;

    for (i=0 ; i<SCREENWIDTH ; i++)
    {
	negonearray[i] = -1;
    }
    maxvissprites = default_vissprites;
    if ((vissprites = (vissprite_t*)Z_MallocNoAbort(maxvissprites * sizeof(vissprite_t), PU_STATIC, NULL)) == NULL)
    {
        I_Error("Unable to claim array for vissprites!");
    }
    fprintf(logfile, "R_InitSprites: claimed %d vissprites, %d bytes\n", maxvissprites, maxvissprites * sizeof(vissprite_t));
    
#ifdef SORT_DRAWSEGS
    i = (SCREENWIDTH + (1 << bucketshift) - 1) >> bucketshift;
    sortedsegs = (dshort_t*)Z_MallocNoAbort(i * sizeof(dshort_t), PU_STATIC, NULL);
    sortednext = (dshort_t*)Z_MallocNoAbort(i * sizeof(dshort_t), PU_STATIC, NULL);
    sortedmins = (sorted_min_t*)Z_MallocNoAbort(i * sizeof(sorted_min_t), PU_STATIC, NULL);
    if ((sortedsegs == NULL) || (sortednext == NULL) || (sortedmins == NULL))
    {
        I_Error("Unable to claim arrays for sorted drawsegs!");
    }
    fprintf(logfile, "R_InitSprites: claimed %d bytes drawseg bucket\n", SCREENWIDTH * (2 * sizeof(dshort_t) + sizeof(sorted_min_t)));
#endif

    R_InitSpriteDefs (namelist);
}




/* R_ClearSprites*/
/* Called at frame start.*/

void R_ClearSprites (void)
{
    vissprite_p = vissprites;
}



/* R_NewVisSprite*/

static vissprite_t	overflowsprite;

static vissprite_t* R_NewVisSprite (void)
{
    if (vissprite_p >= vissprites + maxvissprites)
    {
      int wantSprites;
      vissprite_t *newSprites;

      wantSprites = maxvissprites + DEFAULT_VISSPRITES;
      if (maximum_vissprites)
      {
          if (maxvissprites >= maximum_vissprites) return &overflowsprite;
          if (wantSprites > maximum_vissprites) wantSprites = maximum_vissprites;
      }
      if ((newSprites = (vissprite_t*)Z_MallocNoAbort(wantSprites * sizeof(vissprite_t), PU_STATIC, NULL)) == NULL)
        return &overflowsprite;
      memcpy(newSprites, vissprites, maxvissprites * sizeof(vissprite_t));
      vissprite_p = newSprites + (vissprite_p - vissprites);
      Z_Free(vissprites);
      vissprites = newSprites;
      maxvissprites = wantSprites;
      fprintf(logfile, "R_NewVisSprite: claimed %d vissprites, %d bytes\n", maxvissprites, maxvissprites * sizeof(vissprite_t));
    }

    vissprite_p++;
    return vissprite_p-1;
}




/* R_DrawMaskedColumn*/
/* Used for sprites and masked mid textures.*/
/* Masked means: partly transparent, i.e. stored*/
/*  in posts/runs of opaque pixels.*/

void R_DrawMaskedColumn (draw_context_t *ctx, const column_t* column)
{
    int		topscreen;
    int 	bottomscreen;
    fixed_t	basetexturemid;

#ifdef DIYARMASS
    if (colfunc == R_DrawColumn)
    {
#ifdef DEBUG_PLOTTERS
        fprintf(logfile, "e"); fflush(logfile);
#endif
        Rarm_DrawMaskedColumn(ctx, column);
#ifdef DEBUG_PLOTTERS
        fprintf(logfile, "f"); fflush(logfile);
#endif
        return;
    }
    else if (colfunc == R_DrawColumnLow)
    {
        Rarm_DrawMaskedColumnLow(ctx, column);
        return;
    }
    else
#if ((LD_PIXEL_DEPTH > 3) && defined(DIYTRANSPARENCY))
    if (colfunc == (rdraw_fn)Rarm_DrawMaskedColumnTranslucent)
    {
#ifdef DEBUG_PLOTTERS
        fprintf(logfile, "m"); fflush(logfile);
#endif
        Rarm_DrawMaskedColumnTranslucent(ctx, column);
#ifdef DEBUG_PLOTTERS
        fprintf(logfile, "n"); fflush(logfile);
#endif
        return;
    }
    /*else if (colfunc == (rdraw_fn)Rarm_DrawMaskedColumnLowTranslucent)
    {
        Rarm_DrawMaskedColumnLowTranslucent(ctx, column);
        return;
    }*/
    else
#endif
    {
#endif
    basetexturemid = ctx->dc_texturemid;

    for ( ; column->topdelta != 0xff ; )
    {
	/* calculate unclipped screen coordinates*/
	/*  for post*/
	topscreen = ctx->sprtopscreen + ctx->spryscale*column->topdelta;
	bottomscreen = topscreen + ctx->spryscale*column->length;

	ctx->dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
	ctx->dc_yh = (bottomscreen-1)>>FRACBITS;

	if (ctx->dc_yh >= (ctx->floorclip)[ctx->dc_x])
	    ctx->dc_yh = (ctx->floorclip)[ctx->dc_x]-1;
	if (ctx->dc_yl <= (ctx->ceilingclip)[ctx->dc_x])
	    ctx->dc_yl = (ctx->ceilingclip)[ctx->dc_x]+1;

	if (ctx->dc_yl <= ctx->dc_yh)
	{
	    ctx->dc_source = (byte *)column + 3;
	    ctx->dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);
	    /* ctx->dc_source = (byte *)column + 3 - column->topdelta;*/

	    ctx->dc_texheight = (int)(column->length);

	    /* Drawn by either R_DrawColumn*/
	    /*  or (SHADOW) R_DrawFuzzColumn.*/
	    if ((ctx->dc_texturemid + (ctx->dc_yl-ctx->centery)*ctx->dc_iscale) < 0) ctx->dc_yl++;
#ifdef DEBUG_PLOTTERS
	    fprintf(logfile, "k"); fflush(logfile);
#endif
	    colfunc (ctx);
#ifdef DEBUG_PLOTTERS
            fprintf(logfile, "l"); fflush(logfile);
#endif
	}
	column = (const column_t *)( (const byte *)column + column->length + 4);
    }

    ctx->dc_texturemid = basetexturemid;
#ifdef DIYARMASS
    }
#endif
}




#if 0
/* Hack... found the cause for overwriting spritewidth array in p_spec.c */
static void R_ConsistencyViolation(int lump)
{
    patch_t *patch;

    fprintf(logfile, "Must adjust spritelump %d\n", lump);
    patch = W_CacheLumpNum(firstspritelump+lump, PU_CACHE);
    spritewidth[lump] = SHORT(patch->width) << FRACBITS;
}
#endif



#ifdef DIYRESAMPLETHINGS

/*
 *  Exported symbols for drawing resampled columns
 */

int *R_CreateMaskedTwinColumns(twin_context_t *ctx, void *store)
{
  static byte lastColumn[256], lastMask[32];

  int *twinCols;
  const column_t *column;
  const byte *basePtr;
  int basesize, size, totalsize, minoff, i, width;
 
  width = ctx->width;
  /*printf("Make twin, width %d...\n", width); fflush(stdout);*/
  basesize = width*sizeof(int);
  /*printf("basesize %d\n", basesize);*/

  size = basesize; minoff = 255;
  for (i=0; i<width; i++)
  {
    column = ctx->getcolumn(ctx, i);
    basePtr = (const byte*)column;
    while (column->topdelta != 0xff)
    {
      if (minoff > (int)(column->topdelta))
	minoff = (int)(column->topdelta);

      column = (const column_t *)(((const byte *)column) + column->length + 4);
    }
    /* need to reserve space for the final topdelta as well! */
    size += (((const byte*)column) - basePtr) + 1;
  }
  totalsize = (size + 3) & ~3;
  /*printf("size %d\n", size);*/

  twinCols = (int*)Z_MallocNoAbort(totalsize, PU_CACHE, store);

  if (twinCols != NULL)
  {
    column_t *destcol;
    const byte *source;
    byte *dest;
    int j, pos, innerWidth = width;

    size = basesize;
    if (ctx->modulo == 0) innerWidth--;

    for (i=0; i<innerWidth; i++)
    {
      int clen;

      /*printf("%d/%d\n", i, innerWidth); fflush(stdout);*/
      memset(lastMask, 0, 32);
      column = ctx->getcolumn(ctx, i+1);
      while (column->topdelta != 0xff)
      {
	source = ((const byte*)column) + 3;
	pos = (int)(column->topdelta) - minoff;
	clen = (int)(column->length);
	if (pos + clen >= 256)
	  clen = 256 - pos;
	for (j=0; j<clen; j++, pos++)
	{
	  lastColumn[pos] = source[j];
	  lastMask[pos>>3] |= 1<<(pos&7);
	}
	/*if ((pos < 0) || (pos >= 256)) printf("POS %d\n", pos); fflush(stdout);*/
	column = (const column_t*)(((const byte*)column) + column->length + 4);
      }

      /*printf("Check1\n"); fflush(stdout);*/
      column = ctx->getcolumn(ctx, i);
      basePtr = (const byte*)column;
      twinCols[i] = size;
      destcol = (column_t*)(((byte*)twinCols) + size);
      /*printf("size %d\n", size);*/
      while (column->topdelta != 0xff)
      {
	destcol->topdelta = column->topdelta;
	destcol->length = column->length;

	source = ((const byte*)column) + 3;
	dest = ((byte*)destcol) + 3;
	pos = (int)(column->topdelta) - minoff;
	clen = (int)(column->length);
	if (pos + clen >= 256)
	  clen = 256 - pos;
	for (j=0; j<clen; j++, pos++)
	  dest[j] = ((lastMask[pos>>3] & (1<<(pos&7))) == 0) ? source[j] : lastColumn[pos];

	destcol = (column_t*)(((byte*)destcol) + column->length + 4);
	column = (const column_t*)(((const byte*)column) + column->length + 4);
      }
      destcol->topdelta = 0xff;
      size += (((const byte*)column) - basePtr) + 1;
      /*printf("Check2\n"); fflush(stdout);*/
    }
    if (ctx->modulo == 0)
    {
      /* final column is a verbatim copy */
      column = ctx->getcolumn(ctx, width-1);
      basePtr = (const byte*)column;
      twinCols[width-1] = size;
      destcol = (column_t*)(((byte*)twinCols) + size);
      while (column->topdelta != 0xff)
      {
	destcol->topdelta = column->topdelta;
	destcol->length = column->length;
	source = ((const byte*)column) + 3;
	dest = ((byte*)destcol) + 3;
	for (j=0; j<(int)(column->length); j++)
	  dest[j] = source[j];
	destcol = (column_t*)(((byte*)destcol) + column->length + 4);
	column = (const column_t*)(((const byte*)column) + column->length + 4);
      }
      destcol->topdelta = 0xff;
      size += (((const byte*)column) - basePtr) + 1;
      /*printf("OK %d/%d/%d\n", (size+3)&~3, totalsize, ((byte*)destcol)-((byte*)twinCols)); fflush(stdout);*/
    }
  }
  return twinCols;
}



void R_DrawMaskedTwinColumn(draw_context_t *ctx, const twin_draw_t *tw, const column_t *col1, const column_t *col2, int frac)
{
  int		topscreen;
  int 	        bottomscreen;
  fixed_t	basetexturemid = d_ctx.dc_texturemid;;

  /*printf("("); fflush(stdout);*/
  while (col1->topdelta != 0xff)
  {
    /* calculate unclipped screen coordinates*/
    /*  for post*/
    topscreen = ctx->sprtopscreen + ctx->spryscale*col1->topdelta;
    bottomscreen = topscreen + ctx->spryscale*col1->length;

    ctx->dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
    ctx->dc_yh = (bottomscreen-1)>>FRACBITS;

    if (ctx->dc_yh >= (ctx->floorclip)[ctx->dc_x])
      ctx->dc_yh = (ctx->floorclip)[ctx->dc_x]-1;
    if (ctx->dc_yl <= (ctx->ceilingclip)[ctx->dc_x])
      ctx->dc_yl = (ctx->ceilingclip)[ctx->dc_x]+1;

    if (ctx->dc_yl <= ctx->dc_yh)
    {
      fixed_t cf = frac & ((1<<FRACBITS)-1);

      ctx->dc_texturemid = basetexturemid - (col1->topdelta<<FRACBITS);
      if ((ctx->dc_texturemid + (ctx->dc_yl-ctx->centery)*ctx->dc_iscale) < 0) (ctx->dc_yl)++;
	      
      ctx->dc_texheight = (int)(col1->length);

      tw->resamplefunc(ctx, (const byte*)col1 + 3, (const byte*)col2 + 3, cf);

      /* Duplicate the last texel to have enough samples for the interpolation */
      ctx->resampled_column[ctx->dc_texheight] = ctx->resampled_column[ctx->dc_texheight-1];

      /* Pretend the texture is 1 higher (-> duplicated texel) to avoid undesired wraparound */
      (ctx->dc_texheight)++;

      tw->drawresfunc(ctx);
    }
    col2 = (column_t*)((byte*)col2 + col1->length + 4);
    col1 = (column_t*)((byte*)col1 + col1->length + 4);
  }
  /*printf(")"); fflush(stdout);*/
  ctx->dc_texturemid = basetexturemid;
}



/*
 *  Local code for column resampling
 */
typedef struct twin_sprite_s {
  twin_context_t twin;
  const patch_t *patch;
} twin_sprite_t;


static const column_t *twin_patch_read_column(twin_context_t *ctx, int num)
{
  twin_sprite_t *tspr = (twin_sprite_t*)ctx;
  return (const column_t*)(((const byte*)(tspr->patch)) + LONG(tspr->patch->columnofs[num]));
}

static int **twinPatches = NULL;

static int *R_EnsureTwinPatch(int number, const patch_t *patch)
{
  if ((number < 0) || (number >= numspritelumps))
    return NULL;

  if (twinPatches == NULL)
  {
    int i;
    /*printf("Twin lookup %d\n", numspritelumps);*/
    twinPatches = (int**)Z_MallocNoAbort(numspritelumps * sizeof(int*), PU_STATIC, NULL);
    if (twinPatches == NULL)
      return NULL;
    for (i=0; i<numspritelumps; i++)
      twinPatches[i] = NULL;
  }
  if (twinPatches[number] == NULL)
  {
    twin_sprite_t twinCtx;
    twinCtx.twin.width = (int)SHORT(patch->width);
    twinCtx.twin.modulo = 0;
    twinCtx.twin.getcolumn = twin_patch_read_column;
    twinCtx.patch = patch;

    /*printf("Make %d, %p...\n", number, patch); fflush(stdout);*/
    R_CreateMaskedTwinColumns(&twinCtx.twin, twinPatches + number);
    /*printf("OK.\n"); fflush(stdout);*/
  }
  return twinPatches[number];
}
#endif


/* R_DrawVisSprite*/
/*  mfloorclip and mceilingclip should also be set.*/

static void
R_DrawVisSprite
( vissprite_t*		vis,
  int			x1,
  int			x2 )
{
    column_t*		column;
    int			texturecolumn;
    fixed_t		frac;
    patch_t*		patch;
    void		(*oldcolfunc)(draw_context_t*) = colfunc;
#if ((LD_PIXEL_DEPTH > 3) && defined(DIYTRANSPARENCY))
    int			isTransparent = 0;
#endif

    patch = W_CacheLumpNum (vis->patch+firstspritelump, PU_CACHE);
    d_ctx.dc_colormap = vis->colormap;

    if (!d_ctx.dc_colormap)
    {
	/* NULL colormap = shadow draw*/
	colfunc = fuzzcolfunc;
    }
    else if (vis->mobjflags & MF_TRANSLATION)
    {
	colfunc = R_DrawTranslatedColumn;
	d_ctx.dc_translation = translationtables - 256 +
	    ( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) );
    }
#if ((LD_PIXEL_DEPTH > 3) && defined(DIYTRANSPARENCY))
    else if ((EnableTransparency != 0) && ((vis->translucent) BOOMSTATEMENT(|| (vis->mobjflags & MF_TRANSLUCENT)) ))
    {
      isTransparent = 1;
#ifdef DIYARMASS
      if (colfunc==R_DrawColumn)
        colfunc=(rdraw_fn)Rarm_DrawMaskedColumnTranslucent;
      /*else if (colfunc==R_DrawColumnLow)
        colfunc=(rdraw_fn)Rarm_DrawMaskedColumnLowTranslucent;*/
#else
      if (colfunc == R_DrawColumn)
        colfunc = R_DrawColumnTranslucent;
      /*else if (colfunc == R_DrawColumnLow)
        colfunc = R_DrawColumnLowTranslucent;*/
#endif
    }
#endif

    d_ctx.dc_iscale = abs(vis->xiscale)>>d_ctx.detailshift;
    d_ctx.dc_texturemid = vis->texturemid;
    frac = vis->startfrac;
    d_ctx.spryscale = vis->scale;
    d_ctx.sprtopscreen = d_ctx.centeryfrac
                       - FixedMul(d_ctx.dc_texturemid,d_ctx.spryscale);

#ifdef DIYRESAMPLETHINGS
    if ((vis->colormap != NULL) && (SHORT(patch->width) > 1) && (abs(vis->xiscale) < (1<<FRACBITS)))
    {
      int *twinCols = R_EnsureTwinPatch(vis->patch, patch);

      if (twinCols != NULL)
      {
	twin_draw_t drawTwin;
	column_t *col2;

	drawTwin.drawresfunc = R_DrawResampledColumn;
	drawTwin.resamplefunc = ((vis->mobjflags & MF_TRANSLATION) == 0) ?
	  R_ResampleThingColumn : R_ResampleTranslatedThingColumn;
#ifdef DIYTRANSPARENCY
	if (isTransparent)
	  drawTwin.drawresfunc = R_DrawResampledTranslucentColumn;
#endif
	/*printf("resampled, width %d\n", SHORT(patch->width));*/
	for (d_ctx.dc_x=vis->x1; d_ctx.dc_x<=vis->x2; d_ctx.dc_x++, frac += vis->xiscale)
	{
	  /*printf("[%d]", d_ctx.dc_x); fflush(stdout);*/
	  texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
	  if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
	    continue;
#endif
	  /*printf("col %d, ofs1 %d\n", texturecolumn, patch->columnofs[texturecolumn]); fflush(stdout);
	    printf("ofs2 %d\n", twinCols[texturecolumn]); fflush(stdout);*/
	  column = (column_t*)(((byte*)patch) + LONG(patch->columnofs[texturecolumn]));
	  col2 = (column_t*)(((byte*)twinCols) + twinCols[texturecolumn]);

	  R_DrawMaskedTwinColumn(&d_ctx, &drawTwin, column, col2, frac);
	}
	colfunc = oldcolfunc;
	return;
      }
    }
#endif

    for (d_ctx.dc_x=vis->x1 ; d_ctx.dc_x<=vis->x2 ; d_ctx.dc_x++, frac += vis->xiscale)
    {
	texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
	if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
#ifdef RANGECHECK_ABORTS
	    I_Error ("R_DrawSpriteRange: bad texturecolumn");
#else
	    continue;
#endif
#endif
	column = (column_t *) ((byte *)patch +
			       LONG(patch->columnofs[texturecolumn]));
	R_DrawMaskedColumn (&d_ctx, column);
    }
    colfunc = oldcolfunc;
}




/* R_ProjectSprite*/
/* Generates a vissprite for a thing*/
/*  if it might be visible.*/

static void R_ProjectSprite (mobj_t* thing)
{
    fixed_t		tr_x;
    fixed_t		tr_y;

    fixed_t		gxt;
    fixed_t		gyt;
    fixed_t		gzt;

    fixed_t		tx;
    fixed_t		tz;

    fixed_t		xscale;

    int			x1;
    int			x2;

    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int			lump;
    spritenum_t         sprite;

    unsigned		rot;
    boolean		flip;

    int			index;
    BOOMSTATEMENT(int heightsec;)

    vissprite_t*	vis;

    angle_t		ang;
    fixed_t		iscale;

    /* transform the origin point*/
    tr_x = thing->x - viewx;
    tr_y = thing->y - viewy;

    gxt = FixedMul(tr_x,viewcos);
    gyt = -FixedMul(tr_y,viewsin);

    tz = gxt-gyt;

    /* thing is behind view plane?*/
    if (tz < MINZ)
	return;

    xscale = FixedDiv(d_ctx.projection, tz);

    gxt = -FixedMul(tr_x,viewsin);
    gyt = FixedMul(tr_y,viewcos);
    tx = -(gyt+gxt);

    /* too far off the side?*/
    if (abs(tx)>(tz<<2))
	return;

    sprite=(unsigned)thing->sprite;

    /* decide which patch to use for sprite relative to player*/
#ifdef RANGECHECK
    if ((unsigned)sprite >= numsprites)
#ifdef RANGECHECK_ABORTS
	I_Error ("R_ProjectSprite: invalid sprite number %i ", sprite);
#else
	return;
#endif
#endif
    sprdef = &sprites[sprite];
#ifdef RANGECHECK
    if ( (thing->frame&FF_FRAMEMASK) >= sprdef->numframes )
#ifdef RANGECHECK_ABORTS
	I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ", sprite, thing->frame);
#else
	return;
#endif
#endif
    sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
	/* choose a different rotation based on player view*/
	ang = R_PointToAngle (thing->x, thing->y);
	rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
	lump = sprframe->lump[rot];
	flip = (boolean)sprframe->flip[rot];
    }
    else
    {
	/* use single rotation for all views*/
	lump = sprframe->lump[0];
	flip = (boolean)sprframe->flip[0];
    }

    /* calculate edges of the shape*/
    tx -= spriteoffset[lump];
    x1 = (d_ctx.centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS;

    /* off the right side?*/
    if (x1 > d_ctx.viewwidth)
	return;

    /*if (spritewidth[lump] < (1<<FRACBITS)) R_ConsistencyViolation(lump);*/

    tx +=  spritewidth[lump];
    x2 = ((d_ctx.centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS) - 1;

    /* off the left side*/
    if (x2 < 0)
	return;

    gzt = thing->z + spritetopoffset[lump];

    /* killough 4/9/98: clip things which are out of view due to height */
    if (thing->z > viewz + FixedDiv(d_ctx.centeryfrac, xscale) ||
        gzt < viewz - FixedDiv(d_ctx.centeryfrac-d_ctx.viewheight, xscale))
      return;

#ifdef DIYBOOM
    /* killough 3/27/98: exclude things totally separated
     * from the viewer, by either water or fake ceilings
     * killough 4/11/98: improve sprite clipping for underwater/fake ceilings
     */

    heightsec = thing->subsector->sector->heightsec;

    if (heightsec != -1) /* only clip things which are in special sectors */
    {
      int phs = viewplayer->mo->subsector->sector->heightsec;
      if (phs != -1 && viewz < sectors[phs].floorheight ?
         thing->z >= sectors[heightsec].floorheight :
         gzt < sectors[heightsec].floorheight)
       return;
      if (phs != -1 && viewz > sectors[phs].ceilingheight ?
         gzt < sectors[heightsec].ceilingheight &&
         viewz >= sectors[heightsec].ceilingheight :
         thing->z >= sectors[heightsec].ceilingheight)
      return;
    }
#endif

    /* store information in a vissprite*/
    vis = R_NewVisSprite ();
    BOOMSTATEMENT(vis->heightsec = heightsec;)
    vis->mobjflags = thing->flags;
    vis->scale = xscale<<d_ctx.detailshift;
    vis->gx = thing->x;
    vis->gy = thing->y;
    vis->gz = thing->z;
    vis->gzt = gzt;
    vis->texturemid = vis->gzt - viewz;
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= d_ctx.viewwidth ? d_ctx.viewwidth-1 : x2;
#if ((LD_PIXEL_DEPTH > 3) && defined(DIYTRANSPARENCY))
    vis->translucent = STATE_TRANSPARENT((thing->state - states));
#endif
    iscale = FixedDiv (FRACUNIT, xscale);

    if (flip)
    {
	vis->startfrac = spritewidth[lump]-1;
	vis->xiscale = -iscale;
    }
    else
    {
	vis->startfrac = 0;
	vis->xiscale = iscale;
    }

    if (vis->x1 > x1)
	vis->startfrac += vis->xiscale*(vis->x1-x1);
    vis->patch = lump;

    /* get light level*/
    if (thing->flags & MF_SHADOW)
    {
	/* shadow draw*/
	vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
	/* fixed map*/
	vis->colormap = fixedcolormap;
    }
    else if (thing->frame & FF_FULLBRIGHT)
    {
	/* full bright*/
#if (LD_PIXEL_DEPTH == 3)
#ifdef DIYBOOM
        vis->colormap = fullcolormap;
#else
	vis->colormap = colormaps;
#endif
#else
	vis->colormap = translated_colourmaps;
#endif
    }

    else
    {
	/* diminished light*/
	index = xscale>>(LIGHTSCALESHIFT-d_ctx.detailshift);

	if (index >= MAXLIGHTSCALE)
	    index = MAXLIGHTSCALE-1;

	vis->colormap = spritelights[index];
    }
}





/* R_AddSprites*/
/* During BSP traversal, this adds sprites by sector.*/

void R_AddSprites (sector_t* sec)
{
    mobj_t*		thing;
    int			lightnum;

    /* BSP is traversed by subsector.*/
    /* A sector might have been split into several*/
    /*  subsectors during BSP building.*/
    /* Thus we check whether its already added.*/
    if (sec->validcount == validcount)
	return;

    /* Well, now it will be done.*/
    sec->validcount = validcount;

    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (lightnum < 0) lightnum = 0;
    if (lightnum >= LIGHTLEVELS) lightnum = LIGHTLEVELS-1;

    spritelights = scalelight[lightnum];

    /* Handle all things in sector.*/
    for (thing = sec->thinglist ; thing ; thing = thing->snext)
	R_ProjectSprite (thing);
}



/* R_DrawPSprite*/

static void R_DrawPSprite (const pspdef_t* psp)
{
    fixed_t		tx;
    int			x1;
    int			x2;
    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int			lump;
    boolean		flip;
    vissprite_t*	vis;
    vissprite_t		avis;
    spritenum_t         sprite;

    sprite = psp->state->sprite;
    /* decide which patch to use*/
#ifdef RANGECHECK
    if ((unsigned)sprite >= numsprites)
#ifdef RANGECHECK_ABORTS
	I_Error ("R_ProjectSprite: invalid sprite number %i ", sprite);
#else
	return;
#endif
#endif
    sprdef = &sprites[sprite];
#ifdef RANGECHECK
    if ( (psp->state->frame & FF_FRAMEMASK)  >= sprdef->numframes)
#ifdef RANGECHECK_ABORTS
	I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ",
		 sprite, psp->state->frame);
#else
	return;
#endif
#endif
    sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

    lump = sprframe->lump[0];
    flip = (boolean)sprframe->flip[0];

    /* calculate edges of the shape*/
    tx = psp->sx-160*FRACUNIT;

    tx -= spriteoffset[lump];
    x1 = (d_ctx.centerxfrac + FixedMul (tx,pspritescale) ) >>FRACBITS;

    /* off the right side*/
    if (x1 > d_ctx.viewwidth)
	return;

    /*if (spritewidth[lump] < (1<<FRACBITS)) R_ConsistencyViolation(lump);*/

    tx +=  spritewidth[lump];
    x2 = ((d_ctx.centerxfrac + FixedMul (tx, pspritescale) ) >>FRACBITS) - 1;

    /* off the left side*/
    if (x2 < 0)
	return;

    /* store information in a vissprite*/
    vis = &avis;
    vis->mobjflags = 0;
    vis->texturemid = (BASEYCENTER<<FRACBITS)+FRACUNIT/2-(psp->sy-spritetopoffset[lump]);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= d_ctx.viewwidth ? d_ctx.viewwidth-1 : x2;
    vis->scale = pspritescale<<d_ctx.detailshift;
#if ((LD_PIXEL_DEPTH > 3) && defined(DIYTRANSPARENCY))
    vis->translucent = STATE_TRANSPARENT(psp->state - states);
#endif

    if (flip)
    {
	vis->xiscale = -pspriteiscale;
	vis->startfrac = spritewidth[lump]-1;
    }
    else
    {
	vis->xiscale = pspriteiscale;
	vis->startfrac = 0;
    }

    if (vis->x1 > x1)
	vis->startfrac += vis->xiscale*(vis->x1-x1);

    vis->patch = lump;

    if (viewplayer->powers[pw_invisibility] > 4*32
	|| viewplayer->powers[pw_invisibility] & 8)
    {
	/* shadow draw*/
	vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
	/* fixed color*/
	vis->colormap = fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
	/* full bright*/
#if (LD_PIXEL_DEPTH == 3)
#ifdef DIYBOOM
        vis->colormap = fullcolormap;
#else
	vis->colormap = colormaps;
#endif
#else
	vis->colormap = translated_colourmaps;
#endif
    }
    else
    {
	/* local light*/
	vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }

    R_DrawVisSprite (vis, vis->x1, vis->x2);
}




/* R_DrawPlayerSprites*/

static void R_DrawPlayerSprites (void)
{
    int		i;
    int		lightnum;
    pspdef_t*	psp;

    /* get light level*/
    lightnum =
	(viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT)
	+extralight;

    if (lightnum < 0) lightnum = 0;
    if (lightnum >= LIGHTLEVELS) lightnum = LIGHTLEVELS-1;

    spritelights = scalelight[lightnum];

    /* clip to screen bounds*/
    d_ctx.floorclip = screenheightarray;
    d_ctx.ceilingclip = negonearray;

    /* add all active psprites*/
    for (i=0, psp=viewplayer->psprites;
	 i<NUMPSPRITES;
	 i++,psp++)
    {
	if (psp->state)
	    R_DrawPSprite (psp);
    }
}





/* R_SortVisSprites*/

vissprite_t	vsprsortedhead;


void R_SortVisSprites (void)
{
    int			i;
    int			count;
    vissprite_t*	ds;
    vissprite_t*	best = NULL;
    vissprite_t		unsorted;
    fixed_t		bestscale;

    count = vissprite_p - vissprites;

    unsorted.next = unsorted.prev = &unsorted;

    if (!count)
	return;

    for (ds=vissprites ; ds<vissprite_p ; ds++)
    {
	ds->next = ds+1;
	ds->prev = ds-1;
    }

    vissprites[0].prev = &unsorted;
    unsorted.next = &vissprites[0];
    (vissprite_p-1)->next = &unsorted;
    unsorted.prev = vissprite_p-1;

    /* pull the vissprites out by scale*/
    /*best = 0;		// shut up the compiler warning*/
    vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;
    for (i=0 ; i<count ; i++)
    {
	bestscale = MAXINT;
	for (ds=unsorted.next ; ds!= &unsorted ; ds=ds->next)
	{
	    if (ds->scale < bestscale)
	    {
		bestscale = ds->scale;
		best = ds;
	    }
	}
	best->next->prev = best->prev;
	best->prev->next = best->next;
	best->next = &vsprsortedhead;
	best->prev = vsprsortedhead.prev;
	vsprsortedhead.prev->next = best;
	vsprsortedhead.prev = best;
    }
}




/* R_DrawSprite*/

/*
 *  Moved these arrays out here in order to avoid having to create them on the
 *  stack or even worse on the heap every time R_DrawSprite is called. Since
 *  R_DrawSprite doesn't appear to be reentrant there shouldn't be any problems
 *  as a result of this.
 */
#ifdef STATIC_RESOLUTION
static dshort_t	clipbot[SCREENWIDTH];
static dshort_t	cliptop[SCREENWIDTH];
#else
static dshort_t	clipbot[MAXSCREENWIDTH];
static dshort_t	cliptop[MAXSCREENWIDTH];
#endif

/* R_DrawSprite components shared by sorted and unsorted variants */
#define DRAW_SPRITE_VARS \
    drawseg_t*		ds; \
    int			x; \
    int			r1; \
    int			r2; \
    fixed_t		scale; \
    fixed_t		lowscale; \
    int			silhouette;

#define DRAW_SPRITE_BODY \
	/* determine if the drawseg obscures the sprite*/ \
	if (ds->x1 > spr->x2 \
	    || ds->x2 < spr->x1 \
	    || (!ds->silhouette \
		&& !ds->maskedtexturecol) ) \
	{ \
	    /* does not cover sprite*/ \
	    continue; \
	} \
	r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1; \
	r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2; \
	if (ds->scale1 > ds->scale2) \
	{ \
	    lowscale = ds->scale2; \
	    scale = ds->scale1; \
	} \
	else \
	{ \
	    lowscale = ds->scale1; \
	    scale = ds->scale2; \
	} \
	if (scale < spr->scale \
	    || ( lowscale < spr->scale \
		 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline) ) ) \
	{ \
	    /* masked mid texture?*/ \
	    if (ds->maskedtexturecol) \
		R_RenderMaskedSegRange (ds, r1, r2); \
	    /* seg is behind sprite*/ \
	    continue; \
	} \
	/* clip this piece of the sprite*/ \
	silhouette = ds->silhouette; \
	if (spr->gz >= ds->bsilheight) \
	    silhouette &= ~SIL_BOTTOM; \
	if (spr->gzt <= ds->tsilheight) \
	    silhouette &= ~SIL_TOP;


#ifdef DIYBOOM
# define DRAW_SPRITE_END_BOOM \
    /* killough 3/27/98: */ \
    /* Clip the sprite against deep water and/or fake ceilings. */\
    /* killough 4/9/98: optimize by adding mh */\
    /* killough 4/11/98: improve sprite clipping for underwater/fake ceilings */\
    if (spr->heightsec != -1)  /* only things in specially marked sectors */\
    { \
      fixed_t h,mh; \
      int phs = viewplayer->mo->subsector->sector->heightsec; \
      if ((mh = sectors[spr->heightsec].floorheight) > spr->gz && \
          (h = d_ctx.centeryfrac - FixedMul(mh-=viewz, spr->scale)) >= 0 && \
          (h >>= FRACBITS) < d_ctx.viewheight) \
      { \
        if (mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight)) \
        { /* clip bottom */\
          for (x=spr->x1 ; x<=spr->x2 ; x++) \
            if (clipbot[x] == -2 || h < clipbot[x]) \
              clipbot[x] = h; \
        } \
        else /* clip top */\
          for (x=spr->x1 ; x<=spr->x2 ; x++) \
            if (cliptop[x] == -2 || h > cliptop[x]) \
              cliptop[x] = h; \
      } \
      if ((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt && \
          (h = d_ctx.centeryfrac - FixedMul(mh-viewz, spr->scale)) >= 0 && \
          (h >>= FRACBITS) < d_ctx.viewheight) \
      { \
        if (phs != -1 && viewz >= sectors[phs].ceilingheight) \
        { /* clip bottom */\
          for (x=spr->x1 ; x<=spr->x2 ; x++) \
            if (clipbot[x] == -2 || h < clipbot[x])\
              clipbot[x] = h; \
        } \
        else /* clip top */\
          for (x=spr->x1 ; x<=spr->x2 ; x++) \
            if (cliptop[x] == -2 || h > cliptop[x]) \
              cliptop[x] = h; \
      } \
    } \
    /* killough 3/27/98: end special clipping for deep water / fake ceilings */
#else
# define DRAW_SPRITE_END_BOOM
#endif

#define DRAW_SPRITE_END \
    DRAW_SPRITE_END_BOOM \
    d_ctx.floorclip = clipbot; \
    d_ctx.ceilingclip = cliptop; \
    R_DrawVisSprite (spr, spr->x1, spr->x2);



/* Use sorted drawsegs approach? */
#ifdef SORT_DRAWSEGS

/* The weight of the bucket sort phase (for choosing the optimum strategy) */
#define BUCKET_WEIGHT(segs,width) (segs + (width>>bucketshift))


static int	drawsortstats = 1;
static dshort_t dsegnum_top[MAXSCREENWIDTH];
static dshort_t dsegnum_bot[MAXSCREENWIDTH];


static void R_BucketSortDrawSegs(void)
{
    int		i;
    int		nsegs;
    int		maxwidth;

    nsegs = (ds_p - drawsegs);

    /* somewhat crude, sets everything to -1 */
    maxwidth = (SCREENWIDTH + (1 << bucketshift) - 1) >> bucketshift;
    memset(sortedsegs, 0xff, maxwidth * sizeof(dshort_t));

    for (i=0; i<nsegs; i++)
    {
        int x;

        x = drawsegs[i].x2 >> bucketshift;
        /* if the right border is off screen we can ignore it entirely */
        if ((x >= 0) && (drawsegs[i].x1 < SCREENWIDTH))
        {
            int minx = drawsegs[i].x1;
            int oldnum;

            if (x >= maxwidth)
                x = maxwidth-1;

            oldnum = sortedsegs[x];
            if (oldnum < 0)
            {
                sortedmins[x].local = minx;
            }
            else
            {
                if (sortedmins[x].local > minx)
                    sortedmins[x].local = minx;
            }
            dsegstore[i] = oldnum;
            sortedsegs[x] = i;
        }
    }
    for (i=maxwidth-1; i>=0; i--)
    {
        sortednext[i] = MAXDSHORT;
        if (sortedsegs[i] != -1) break;
        /*sortedmins[i].global = MAXDSHORT;*/
    }
    if (i >= 0)
    {
        int minglobal;
        int next;

        minglobal = sortedmins[i].local;
        sortedmins[i].global = minglobal;
        next = i;
        while (--i >= 0)
        {
            sortednext[i] = next;

            if (sortedsegs[i] >= 0)
            {
                if (sortedmins[i].local < minglobal)
                    minglobal = sortedmins[i].local;

                sortedmins[i].global = minglobal;
                next = i;
            }
            /*sortedmins[i].global = minglobal;*/
        }
    }
#if 0
    for (i=0; i<maxwidth; i++)
    {
        fprintf(logfile, "%3d: %d %d %d %d\n", i, sortedsegs[i], sortednext[i], sortedmins[i].local, sortedmins[i].global);
        if (sortedsegs[i] >= 0)
        {
            int j;

            j = sortedsegs[i];
            while (j >= 0)
            {
                fprintf(logfile, "\t%d\n", j);
                j = dsegstore[j];
            }
        }
    }
#endif
}




/* Sprite-drawing code to use if sprites have been sorted */
static void R_DrawSpriteSorted (vissprite_t* spr)
{
    DRAW_SPRITE_VARS
    int 		nsegs;	/* new */
    dshort_t		dsegnum;
    int			firstcol;
    int			maxwidth;
    /*int		numscans = 0;*/

    for (x = spr->x1 ; x<=spr->x2 ; x++)
	dsegnum_top[x] = dsegnum_bot[x] = -1;

    /* New code for improved speed */
    nsegs = (ds_p - drawsegs);
    if (nsegs > numsortsegs) nsegs = numsortsegs;
    /* FIXME -- is spr->x1 confined to screen? */
    firstcol = spr->x1 >> bucketshift;

    maxwidth = (SCREENWIDTH + (1 << bucketshift) - 1) >> bucketshift;
    /* Scan drawsegs from end to start for obscuring segs.*/
    /* The first drawseg that has a greater scale */
    /* is the clip seg.*/
    while (firstcol < maxwidth)
    {
        /*fprintf(logfile, "col %d\n", firstcol);*/
        if (sortedsegs[firstcol] < 0)
        {
            firstcol = sortednext[firstcol];
            if (firstcol >= maxwidth)
                break;
        }
        /* there are no more segs from to the right of the current position which
           can intersect? */
        if (sortedmins[firstcol].global > spr->x2)
            break;

        /* there are no segs at this position which can intersect? */
        if (sortedmins[firstcol].local <= spr->x2)
        {
            for (dsegnum=sortedsegs[firstcol]; dsegnum>=0; dsegnum=dsegstore[dsegnum])
            {
                /*fprintf(logfile, "idx %d\n", firstidx);*/
	        ds = drawsegs + dsegnum;
	        drawsortstats++;
	        /*numscans++;*/
	        /* from here the same code as before */

                DRAW_SPRITE_BODY

	        if (silhouette == 1)
	        {
	            /* bottom sil */
	            for (x=r1 ; x<=r2 ; x++)
	                if (dsegnum_bot[x] < dsegnum)
		        {
		            clipbot[x] = ds->sprbottomclip[x];
		            dsegnum_bot[x] = dsegnum;
		        }
	        }
	        else if (silhouette == 2)
	        {
	            /* top sil */
	            for (x=r1 ; x<=r2 ; x++)
	                if (dsegnum_top[x] < dsegnum)
		        {
		            cliptop[x] = ds->sprtopclip[x];
		            dsegnum_top[x] = dsegnum;
		        }
	        }
	        else if (silhouette == 3)
	        {
	            /* both */
	            for (x=r1 ; x<=r2 ; x++)
	            {
	                if (dsegnum_bot[x] < dsegnum)
		        {
		            clipbot[x] = ds->sprbottomclip[x];
		            dsegnum_bot[x] = dsegnum;
		        }
		        if (dsegnum_top[x] < dsegnum)
		        {
		            cliptop[x] = ds->sprtopclip[x];
		            dsegnum_top[x] = dsegnum;
		        }
	            }
		}
	    }
	}
	firstcol++;
    }
    /*printf("scans %d | %d\n", numscans, nsegs); new, debugging only */

    /* all clipping has been performed, so draw the sprite*/

    /* check for unclipped columns*/
    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
	if (dsegnum_bot[x] == -1)
	    clipbot[x] = d_ctx.viewheight;
	if (dsegnum_top[x] == -1)
	    cliptop[x] = -1;
    }

    /*for (x = spr->x1; x <= spr->x2; x++)
    {
        fprintf(logfile, "%d:%d ", clipbot[x], cliptop[x]);
    }
    fprintf(logfile, "\n"); fflush(logfile);*/

    DRAW_SPRITE_END
}

#endif


/* Old sprite-drawing approach for unsorted sprites */
static void R_DrawSprite (vissprite_t* spr)
{
    DRAW_SPRITE_VARS

    for (x = spr->x1 ; x<=spr->x2 ; x++)
	clipbot[x] = cliptop[x] = -2;

    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
    {
        DRAW_SPRITE_BODY

	if (silhouette == 1)
	{
	    /* bottom sil*/
	    for (x=r1 ; x<=r2 ; x++)
		if (clipbot[x] == -2)
		    clipbot[x] = ds->sprbottomclip[x];
	}
	else if (silhouette == 2)
	{
	    /* top sil*/
	    for (x=r1 ; x<=r2 ; x++)
		if (cliptop[x] == -2)
		    cliptop[x] = ds->sprtopclip[x];
	}
	else if (silhouette == 3)
	{
	    /* both*/
	    for (x=r1 ; x<=r2 ; x++)
	    {
		if (clipbot[x] == -2)
		    clipbot[x] = ds->sprbottomclip[x];
		if (cliptop[x] == -2)
		    cliptop[x] = ds->sprtopclip[x];
	    }
	}
    }
    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
	if (clipbot[x] == -2)
	    clipbot[x] = d_ctx.viewheight;

	if (cliptop[x] == -2)
	    cliptop[x] = -1;
    }

    DRAW_SPRITE_END
}



/* R_DrawMasked*/

void R_DrawMasked (void)
{
    vissprite_t*	spr;
    drawseg_t*		ds;
#ifdef SORT_DRAWSEGS
    int			nsegs;
    int			nsprites;
    int			dobucket;
#endif

    R_SortVisSprites ();

#ifdef SORT_DRAWSEGS
    nsegs = ds_p - drawsegs;
    nsprites = vissprite_p - vissprites;

    /*fprintf(logfile, "bucket: %d, normal: %d\n", BUCKET_WEIGHT(nsegs,SCREENWIDTH) + nsprites * drawsortstats, nsprites * nsegs);*/

    /* approximate when it's cheaper to use the old, primitive method */
    dobucket = (BUCKET_WEIGHT(nsegs,SCREENWIDTH) < nsprites * (nsegs - drawsortstats));
    if (dobucket)
    {
        /* Somewhat crude code to make sure it keeps going even if memory is very tight */
        if (nsegs > numsortsegs)
        {
            int wantSize = numsortsegs;

            while (wantSize < nsegs) wantSize += incsortsegs;

            if (dsegstore != NULL) Z_Free(dsegstore);
            if ((dsegstore = (dshort_t*)Z_MallocNoAbort(wantSize * sizeof(dshort_t), PU_STATIC, NULL)) != NULL)
            {
                numsortsegs = wantSize;
                fprintf(logfile, "R_DrawMasked: grew sorted drawsegs to %d bytes\n", wantSize * sizeof(*dsegstore));
            }
            else
            {
                fprintf(logfile, "Unable to claim drawseg sorting buffers (%d)!\n", nsegs);
                dobucket = 0;
            }
        }

        if (dsegstore != NULL)
        {
            /* drawsortstats = total number of bucket accesses */
            drawsortstats = 0;

            R_BucketSortDrawSegs();

            if (vissprite_p > vissprites)
            {
                for (spr=vsprsortedhead.next ; spr != &vsprsortedhead ; spr=spr->next)
                {
                    R_DrawSpriteSorted(spr);
                }
            }

            /* drawsortstats = average number of bucket accesses per sprite */
            drawsortstats /= nsprites;
            /*fprintf(logfile, "accesses: %d\n", drawsortstats);*/
        }
    }
    if (!dobucket)
    {
        /* age the data so we will retry the buckets some time later */
        drawsortstats--;
#endif
        if (vissprite_p > vissprites)
        {
            /* draw all vissprites back to front*/
	    for (spr = vsprsortedhead.next ;
	         spr != &vsprsortedhead ;
	         spr=spr->next)
	    {
	        R_DrawSprite (spr);
	    }
        }
#ifdef SORT_DRAWSEGS
    }
#endif

    /* render any remaining masked mid textures*/
    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
	if (ds->maskedtexturecol)
	    R_RenderMaskedSegRange (ds, ds->x1, ds->x2);

    /* draw the psprites on top of everything*/
    /*  but does not draw on side views*/
    if (!viewangleoffset)
	R_DrawPlayerSprites ();
}
