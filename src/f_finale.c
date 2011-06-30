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
/*	Game completion, final screen animation.*/

/*-----------------------------------------------------------------------------*/


static const char
rcsid[] = "$Id: f_finale.c,v 1.5 1997/02/03 21:26:34 b1 Exp $";

#include <ctype.h>

/* Functions.*/
#include "d_main.h"
#include "i_system.h"
#include "m_swap.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"
#include "hu_stuff.h"

/* Data.*/
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"
#include "r_state.h"
#include "r_context.h"

#include "i_video.h"

/* ?*/
/*#include "doomstat.h"*/
/*#include "r_local.h"*/
#include "f_finale.h"

/* Stage of animation:*/
/*  0 = text, 1 = art screen, 2 = character cast*/
static int	finalestage;

static int	finalecount;

#define	TEXTSPEED	3
#define	TEXTWAIT	250


/* New for DeHackEd: lumps used in finale */
#define FLUMP_FLOOR4_8	0
#define FLUMP_SFLR6_1	1
#define FLUMP_MFLR8_4	2
#define FLUMP_MFLR8_3	3
#define FLUMP_SLIME16	4
#define FLUMP_RROCK14	5
#define FLUMP_RROCK07	6
#define FLUMP_RROCK17	7
#define FLUMP_RROCK13	8
#define FLUMP_RROCK19	9
#define FLUMP_F_SKY1	10
#define FLUMP_BOSSBACK	11
#define FLUMP_PFUB2	12
#define FLUMP_PFUB1	13
#define FLUMP_END0	14
#define FLUMP_CREDIT	15
#define FLUMP_HELP2	16
#define FLUMP_VICTORY2	17
#define FLUMP_ENDPIC	18

char *finale_lumps[FLUMP_NUMBER] = {
  "FLOOR4_8",
  "SFLR6_1",
  "MFLR8_4",
  "MFLR8_3",
  "SLIME16",
  "RROCK14",
  "RROCK07",
  "RROCK17",
  "RROCK13",
  "RROCK19",
  "F_SKY1",
  "BOSSBACK",
  "PFUB2",
  "PFUB1",
  "END0",
  "CREDIT",
  "HELP2",
  "VICTORY2",
  "ENDPIC"
};


char*	e1text = E1TEXT;
char*	e2text = E2TEXT;
char*	e3text = E3TEXT;
char*	e4text = E4TEXT;

char*	c1text = C1TEXT;
char*	c2text = C2TEXT;
char*	c3text = C3TEXT;
char*	c4text = C4TEXT;
char*	c5text = C5TEXT;
char*	c6text = C6TEXT;

char*	p1text = P1TEXT;
char*	p2text = P2TEXT;
char*	p3text = P3TEXT;
char*	p4text = P4TEXT;
char*	p5text = P5TEXT;
char*	p6text = P6TEXT;

char*	t1text = T1TEXT;
char*	t2text = T2TEXT;
char*	t3text = T3TEXT;
char*	t4text = T4TEXT;
char*	t5text = T5TEXT;
char*	t6text = T6TEXT;

char*	finaletext;
char*	finaleflat;

static void	F_StartCast (void);
static void	F_CastTicker (void);
static boolean	F_CastResponder (const event_t *ev);
static void	F_CastDrawer (void);



#ifdef STATIC_RESOLUTION
#define FINALE_ORGX	((SCREENWIDTH - 320) >> 1)
#define FINALE_ORGY	((SCREENHEIGHT - 200) >> 1)
#else
static int FINALE_ORGX;
static int FINALE_ORGY;
#endif


/* F_StartFinale*/

void F_StartFinale (void)
{
    gameaction = ga_nothing;
    gamestate = GS_FINALE;
    viewactive = false;
    automapactive = mixmap = false;

    /* Okay - IWAD dependend stuff.*/
    /* This has been changed severly, and*/
    /*  some stuff might have changed in the process.*/
    switch ( gamemode )
    {

      /* DOOM 1 - E1, E3 or E4, but each nine missions*/
      case shareware:
      case registered:
      case retail:
      {
	S_ChangeMusic(mus_victor, true);

	switch (gameepisode)
	{
	  case 1:
	    finaleflat = finale_lumps[FLUMP_FLOOR4_8];
	    finaletext = e1text;
	    break;
	  case 2:
	    finaleflat = finale_lumps[FLUMP_SFLR6_1];
	    finaletext = e2text;
	    break;
	  case 3:
	    finaleflat = finale_lumps[FLUMP_MFLR8_4];
	    finaletext = e3text;
	    break;
	  case 4:
	    finaleflat = finale_lumps[FLUMP_MFLR8_3];
	    finaletext = e4text;
	    break;
	  default:
	    /* Ouch.*/
	    break;
	}
	break;
      }

      /* DOOM II and missions packs with E1, M34*/
      case commercial:
      {
	  S_ChangeMusic(mus_read_m, true);

	  switch (gamemap)
	  {
	    case 6:
	      finaleflat = finale_lumps[FLUMP_SLIME16];
	      finaletext = c1text;
	      break;
	    case 11:
	      finaleflat = finale_lumps[FLUMP_RROCK14];
	      finaletext = c2text;
	      break;
	    case 20:
	      finaleflat = finale_lumps[FLUMP_RROCK07];
	      finaletext = c3text;
	      break;
	    case 30:
	      finaleflat = finale_lumps[FLUMP_RROCK17];
	      finaletext = c4text;
	      break;
	    case 15:
	      finaleflat = finale_lumps[FLUMP_RROCK13];
	      finaletext = c5text;
	      break;
	    case 31:
	      finaleflat = finale_lumps[FLUMP_RROCK19];
	      finaletext = c6text;
	      break;
	    default:
	      /* Ouch.*/
	      break;
	  }
	  break;
      }


      /* Indeterminate.*/
      default:
	S_ChangeMusic(mus_read_m, true);
	finaleflat = finale_lumps[FLUMP_F_SKY1]; /* Not used anywhere else.*/
	finaletext = c1text;  /* FIXME - other text, music?*/
	break;
    }

    finalestage = 0;
    finalecount = 0;

}



boolean F_Responder (const event_t *event)
{
    if (finalestage == 2)
	return F_CastResponder (event);

    return false;
}



/* F_Ticker*/

void F_Ticker (void)
{
    int		i;

    /* check for skipping*/
    if ( (gamemode == commercial)
      && ( finalecount > 50) )
    {
      /* go on to the next level*/
      for (i=0 ; i<MAXPLAYERS ; i++)
	if (players[i].cmd.buttons)
	  break;

      if (i < MAXPLAYERS)
      {
	if (gamemap == 30)
	  F_StartCast ();
	else
	  gameaction = ga_worlddone;
      }
    }

    /* advance animation*/
    finalecount++;

    if (finalestage == 2)
    {
	F_CastTicker ();
	return;
    }

    if ( gamemode == commercial)
	return;

    if (!finalestage && finalecount>strlen (finaletext)*TEXTSPEED + TEXTWAIT)
    {
	finalecount = 0;
	finalestage = 1;
	wipegamestate = -1;		/* force a wipe*/
	I_ClearFrameBuffers();
	if (gameepisode == 3)
	    S_StartMusic (mus_bunny);
    }
}




/* F_TextWrite*/


static void F_TextWrite (void)
{
#if (defined(__riscos__) || (LD_PIXEL_DEPTH > 3))
    const byte*	srcchunk;
#endif
    const byte*	src;
    pixel_t*	dest;

    int		x,y,w;
    int		count;
    const char*	ch;
    int		c;
    int		cx;
    int		cy;

    /* erase the entire screen to a tiled background*/
    src = W_CacheLumpName2 ( finaleflat, ns_flats , PU_CACHE);
    dest = screens[0];
#if (LD_PIXEL_DEPTH == 3)
#ifdef __riscos__
    if (translated_colourmaps != NULL)
    {
	for (y=0 ; y<SCREENHEIGHT ; y++)
	{
	    srcchunk = src + ((y&63)<<6);
	    for (x=0 ; x<SCREENWIDTH ; x++)
	    {
		*dest++ = translated_colourmaps[(unsigned char)(srcchunk[(x&63)])];
	    }
	}
    }
    else
    {
#endif
    for (y=0 ; y<SCREENHEIGHT ; y++)
    {
	for (x=0 ; x<SCREENWIDTH/64 ; x++)
	{
	    memcpy (dest, src+((y&63)<<6), 64);
	    dest += 64;
	}
	if (SCREENWIDTH&63)
	{
	    memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
	    dest += (SCREENWIDTH&63);
	}
    }
#ifdef __riscos__
    }
#endif

#else
    for (y=0 ; y<SCREENHEIGHT ; y++)
    {
	srcchunk = src + ((y&63)<<6);
	for (x=0 ; x <SCREENWIDTH ; x++)
	{
	    *dest++ = (pixel_t)d_ctx.static_colourmap[(unsigned char)(srcchunk[(x&63)])];
	}
    }
#endif

    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);

    /* draw some of the text onto the screen*/
    cx = 10;
    cy = 10;
    ch = finaletext;

    count = (finalecount - 10)/TEXTSPEED;
    if (count < 0)
	count = 0;
    for ( ; count ; count-- )
    {
	c = *ch++;
	if (!c)
	    break;
	if (c == '\n')
	{
	    cx = 10;
	    cy += 11;
	    continue;
	}

	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}

	w = SHORT (hu_font[c]->width);
	if (cx+w+FINALE_ORGX > SCREENWIDTH)
	    break;
	V_DrawPatch(FINALE_ORGX + cx, FINALE_ORGY + cy, 0, hu_font[c]);
	cx+=w;
    }

}


/* Final DOOM 2 animation*/
/* Casting by id Software.*/
/*   in order of appearance*/

castinfo_t	castorder[] = {
    {CC_ZOMBIE, MT_POSSESSED},
    {CC_SHOTGUN, MT_SHOTGUY},
    {CC_HEAVY, MT_CHAINGUY},
    {CC_IMP, MT_TROOP},
    {CC_DEMON, MT_SERGEANT},
    {CC_LOST, MT_SKULL},
    {CC_CACO, MT_HEAD},
    {CC_HELL, MT_KNIGHT},
    {CC_BARON, MT_BRUISER},
    {CC_ARACH, MT_BABY},
    {CC_PAIN, MT_PAIN},
    {CC_REVEN, MT_UNDEAD},
    {CC_MANCU, MT_FATSO},
    {CC_ARCH, MT_VILE},
    {CC_SPIDER, MT_SPIDER},
    {CC_CYBER, MT_CYBORG},
    {CC_HERO, MT_PLAYER},
    {NULL,0}
};

static int	castnum;
static int	casttics;
static state_t*	caststate;
static boolean	castdeath;
static int	castframes;
static int	castonmelee;
static boolean	castattacking;



/* F_StartCast*/

static void F_StartCast (void)
{
    wipegamestate = -1;		/* force a screen wipe*/
    I_ClearFrameBuffers();
    castnum = 0;
    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    casttics = caststate->tics;
    castdeath = false;
    finalestage = 2;
    castframes = 0;
    castonmelee = 0;
    castattacking = false;
    S_ChangeMusic(mus_evil, true);
}



/* F_CastTicker*/

static void F_CastTicker (void)
{
    int		st;
    int		sfx;

    if (--casttics > 0)
	return;			/* not time to change state yet*/

    if (caststate->tics == -1 || caststate->nextstate == S_NULL
        || (castdeath && castframes == 12))
    {
	/* switch from deathstate to next monster*/
	castnum++;
	castdeath = false;
	if (castorder[castnum].name == NULL)
	    castnum = 0;
	if (mobjinfo[castorder[castnum].type].seesound)
	    S_StartSound (NULL, mobjinfo[castorder[castnum].type].seesound);
	caststate = &states[mobjinfo[castorder[castnum].type].seestate];
	castframes = 0;
    }
    else
    {
	/* just advance to next state in animation*/
	if (caststate == &states[S_PLAY_ATK1])
	    goto stopattack;	/* Oh, gross hack!*/
	st = caststate->nextstate;
	caststate = &states[st];
	castframes++;

	/* sound hacks....*/
	switch (st)
	{
	  case S_PLAY_ATK1:	sfx = sfx_dshtgn; break;
	  case S_POSS_ATK2:	sfx = sfx_pistol; break;
	  case S_SPOS_ATK2:	sfx = sfx_shotgn; break;
	  case S_VILE_ATK2:	sfx = sfx_vilatk; break;
	  case S_SKEL_FIST2:	sfx = sfx_skeswg; break;
	  case S_SKEL_FIST4:	sfx = sfx_skepch; break;
	  case S_SKEL_MISS2:	sfx = sfx_skeatk; break;
	  case S_FATT_ATK8:
	  case S_FATT_ATK5:
	  case S_FATT_ATK2:	sfx = sfx_firsht; break;
	  case S_CPOS_ATK2:
	  case S_CPOS_ATK3:
	  case S_CPOS_ATK4:	sfx = sfx_shotgn; break;
	  case S_TROO_ATK3:	sfx = sfx_claw; break;
	  case S_SARG_ATK2:	sfx = sfx_sgtatk; break;
	  case S_BOSS_ATK2:
	  case S_BOS2_ATK2:
	  case S_HEAD_ATK2:	sfx = sfx_firsht; break;
	  case S_SKULL_ATK2:	sfx = sfx_sklatk; break;
	  case S_SPID_ATK2:
	  case S_SPID_ATK3:	sfx = sfx_shotgn; break;
	  case S_BSPI_ATK2:	sfx = sfx_plasma; break;
	  case S_CYBER_ATK2:
	  case S_CYBER_ATK4:
	  case S_CYBER_ATK6:	sfx = sfx_rlaunc; break;
	  case S_PAIN_ATK3:	sfx = sfx_sklatk; break;
	  default: sfx = 0; break;
	}

	if (sfx)
	    S_StartSound (NULL, sfx);
    }

    if (castframes == 12)
    {
	/* go into attack frame*/
	castattacking = true;
	if (castonmelee)
	    caststate=&states[mobjinfo[castorder[castnum].type].meleestate];
	else
	    caststate=&states[mobjinfo[castorder[castnum].type].missilestate];
	castonmelee ^= 1;
	if (caststate == &states[S_NULL])
	{
	    if (castonmelee)
		caststate=
		    &states[mobjinfo[castorder[castnum].type].meleestate];
	    else
		caststate=
		    &states[mobjinfo[castorder[castnum].type].missilestate];
	}
    }

    if (castattacking)
    {
	if (castframes == 24
	    ||	caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
	{
	  stopattack:
	    castattacking = false;
	    castframes = 0;
	    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
	}
    }

    casttics = caststate->tics;
    if (casttics == -1)
	casttics = 15;
}



/* F_CastResponder*/


static boolean F_CastResponder (const event_t* ev)
{
    if (ev->type != ev_keydown)
	return false;

    if (castdeath)
	return true;			/* already in dying frames*/

    /* go into death frame*/
    castdeath = true;
    caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
    casttics = caststate->tics;
    castframes = 0;
    castattacking = false;
    if (mobjinfo[castorder[castnum].type].deathsound)
	S_StartSound (NULL, mobjinfo[castorder[castnum].type].deathsound);

    return true;
}


static void F_CastPrint (const char* text)
{
    const char*	ch;
    int		c;
    int		cx;
    int		w;
    int		width;

    /* find width*/
    ch = text;
    width = 0;

    while (ch)
    {
	c = *ch++;
	if (!c)
	    break;
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    width += 4;
	    continue;
	}

	w = SHORT (hu_font[c]->width);
	width += w;
    }

    /* draw it*/
    cx = 160-width/2;
    ch = text;
    while (ch)
    {
	c = *ch++;
	if (!c)
	    break;
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}

	w = SHORT (hu_font[c]->width);
	V_DrawPatch(FINALE_ORGX + cx, FINALE_ORGY + 180, 0, hu_font[c]);
	cx+=w;
    }

}



/* F_CastDrawer*/

static void F_CastDrawer (void)
{
    const spritedef_t*	sprdef;
    const spriteframe_t* sprframe;
    int			lump;
    boolean		flip;
    const patch_t*	patch;

    /* erase the entire screen to a background*/
    V_DrawPatch (FINALE_ORGX,FINALE_ORGY,0, W_CacheLumpName (finale_lumps[FLUMP_BOSSBACK], PU_CACHE));

    F_CastPrint (castorder[castnum].name);

    /* draw the current frame in the middle of the screen*/
    sprdef = &sprites[caststate->sprite];
    sprframe = &sprdef->spriteframes[ caststate->frame & FF_FRAMEMASK];
    lump = sprframe->lump[0];
    flip = (boolean)sprframe->flip[0];

    patch = W_CacheLumpNum (lump+firstspritelump, PU_CACHE);
    if (flip)
	V_DrawPatchFlipped (FINALE_ORGX+160,FINALE_ORGY+170,0,patch);
    else
	V_DrawPatch (FINALE_ORGX+160,FINALE_ORGY+170,0,patch);
}



/* F_DrawPatchCol*/

static void
F_DrawPatchCol
( int		x,
  const patch_t* patch,
  int		col )
{
    const column_t* column;
    const byte*	source;
    pixel_t*	dest;
    pixel_t*	desttop;
    int		count;

    column = (const column_t *)((const byte *)patch + LONG(patch->columnofs[col]));
    desttop = screens[0] + FINALE_ORGY*SCREENWIDTH + FINALE_ORGX + x;

    /* step through the posts in a column*/
    while (column->topdelta != 0xff )
    {
	source = (const byte *)column + 3;
	dest = desttop + column->topdelta*SCREENWIDTH;
	count = column->length;

	while (count--)
	{
#if (LD_PIXEL_DEPTH == 3)
	    *dest = *source++;
#else
	    *dest = (pixel_t)d_ctx.static_colourmap[*source++];
#endif
	    dest += SCREENWIDTH;
	}
	column = (const column_t *)( (const byte *)column + column->length + 4 );
    }
}



/* F_BunnyScroll*/

static void F_BunnyScroll (void)
{
    int		scrolled;
    int		x;
    const patch_t*	p1;
    const patch_t*	p2;
    char	name[10];
    int		stage;
    static int	laststage;

    p1 = W_CacheLumpName (finale_lumps[FLUMP_PFUB2], PU_LEVEL);
    p2 = W_CacheLumpName (finale_lumps[FLUMP_PFUB1], PU_LEVEL);

    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);

    scrolled = 320 - (finalecount-230)/2;
    if (scrolled > 320)
	scrolled = 320;
    if (scrolled < 0)
	scrolled = 0;

    for ( x=0 ; x<320 ; x++)
    {
	if (x+scrolled < 320)
	    F_DrawPatchCol (x, p1, x+scrolled);
	else
	    F_DrawPatchCol (x, p2, x+scrolled - 320);
    }

    if (finalecount < 1130)
	return;
    if (finalecount < 1180)
    {
	V_DrawPatch (FINALE_ORGX+(320-13*8)/2,
		     FINALE_ORGY+(200-8*8)/2,0, W_CacheLumpName (finale_lumps[FLUMP_END0],PU_CACHE));
	laststage = 0;
	return;
    }

    stage = (finalecount-1180) / 5;
    if (stage > 6)
	stage = 6;
    if (stage > laststage)
    {
	S_StartSound (NULL, sfx_pistol);
	laststage = stage;
    }

    sprintf (name,"END%i",stage);
    V_DrawPatch (FINALE_ORGX+(320-13*8)/2,
		 FINALE_ORGY+(200-8*8)/2,0, W_CacheLumpName (name,PU_CACHE));
}



/* F_Drawer*/

void F_Drawer (void)
{
    if (finalestage == 2)
    {
	F_CastDrawer ();
	return;
    }

    if (!finalestage)
	F_TextWrite ();
    else
    {
	switch (gameepisode)
	{
	  case 1:
	    if ( gamemode == retail )
	      V_DrawPatch (FINALE_ORGX,FINALE_ORGY,0,
			 W_CacheLumpName(finale_lumps[FLUMP_CREDIT],PU_CACHE));
	    else
	      V_DrawPatch (FINALE_ORGX,FINALE_ORGY,0,
			 W_CacheLumpName(finale_lumps[FLUMP_HELP2],PU_CACHE));
	    break;
	  case 2:
	    V_DrawPatch(FINALE_ORGX,FINALE_ORGY,0,
			W_CacheLumpName(finale_lumps[FLUMP_VICTORY2],PU_CACHE));
	    break;
	  case 3:
	    F_BunnyScroll ();
	    break;
	  case 4:
	    V_DrawPatch (FINALE_ORGX,FINALE_ORGY,0,
			 W_CacheLumpName(finale_lumps[FLUMP_ENDPIC],PU_CACHE));
	    break;
	}
    }

}



#ifndef STATIC_RESOLUTION
void F_initForResolution(void)
{
  FINALE_ORGX = (SCREENWIDTH - 320) >> 1;
  FINALE_ORGY = (SCREENHEIGHT - 200) >> 1;
}
#endif
