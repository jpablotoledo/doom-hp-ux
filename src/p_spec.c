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
/*	Implements special effects:*/
/*	Texture animation, height or lighting changes*/
/*	 according to adjacent sectors, respective*/
/*	 utility functions, etc.*/
/*	Line Tag handling. Line and Sector triggers.*/

/*-----------------------------------------------------------------------------*/

/*
 *  BOOM extensions from the Boom source (2.02) with kind permission by Team TNT
 */

static const char
rcsid[] = "$Id: p_spec.c,v 1.6 1997/02/03 22:45:12 b1 Exp $";

#include <stdlib.h>

#include "doomdef.h"
#include "doomstat.h"

#include "i_system.h"
#include "z_zone.h"
#include "m_argv.h"
#include "m_random.h"
#include "m_swap.h"
#include "m_bbox.h"
#include "w_wad.h"

#include "r_local.h"
#include "p_local.h"
#include "p_inter.h"

#include "g_game.h"

#include "s_sound.h"

/* State.*/
#include "r_state.h"

/* Data.*/
#include "sounds.h"

#if (defined(DIYBOOM) && defined(DIYINLINE))
#include "inl_btitr.c"
#endif

#if 0
# define BOOMTRACEOUT(s) fprintf(logfile, "(%s)", s);
#else
# define BOOMTRACEOUT(s)
#endif


/* Animating textures and planes*/
/* There is another anim_t used in wi_stuff, unrelated.*/

typedef struct
{
    boolean	istexture;
    int		picnum;
    int		basepic;
    int		numpics;
    int		speed;

} anim_t;

#define ANDEF_T_ISTEX	0
#define ANDEF_T_ENDNM	1
#define ANDEF_T_STRNM	10
#define ANDEF_T_SPEED	19
#define ANDEF_T_SIZE	23




#define MAXANIMS                32


static anim_t*		lastanim = NULL;
#ifdef DIYBOOM
static anim_t*		anims=NULL;
static int		maxanims=0;

static void P_SpawnScrollers(void);
static void P_SpawnFriction(void);
static void P_SpawnPushers(void);
static void P_InitTagLists(void);

#else
anim_t		anims[MAXANIMS];

/* P_InitPicAnims*/


/* Floor/ceiling animation sequences,*/
/*  defined by first and last frame,*/
/*  i.e. the flat (64x64 tile) name to*/
/*  be used.*/
/* The full animation sequence is given*/
/*  using all the flats between the start*/
/*  and end entry, in the order found in*/
/*  the WAD file.*/

animdef_t		animdefs[] =
{
    {false,	"NUKAGE3",	"NUKAGE1",	8},
    {false,	"FWATER4",	"FWATER1",	8},
    {false,	"SWATER4",	"SWATER1", 	8},
    {false,	"LAVA4",	"LAVA1",	8},
    {false,	"BLOOD3",	"BLOOD1",	8},

    /* DOOM II flat animations.*/
    {false,	"RROCK08",	"RROCK05",	8},
    {false,	"SLIME04",	"SLIME01",	8},
    {false,	"SLIME08",	"SLIME05",	8},
    {false,	"SLIME12",	"SLIME09",	8},

    {true,	"BLODGR4",	"BLODGR1",	8},
    {true,	"SLADRIP3",	"SLADRIP1",	8},

    {true,	"BLODRIP4",	"BLODRIP1",	8},
    {true,	"FIREWALL",	"FIREWALA",	8},
    {true,	"GSTFONT3",	"GSTFONT1",	8},
    {true,	"FIRELAVA",	"FIRELAV3",	8},
    {true,	"FIREMAG3",	"FIREMAG1",	8},
    {true,	"FIREBLU2",	"FIREBLU1",	8},
    {true,	"ROCKRED3",	"ROCKRED1",	8},

    {true,	"BFALL4",	"BFALL1",	8},
    {true,	"SFALL4",	"SFALL1",	8},
    {true,	"WFALL4",	"WFALL1",	8},
    {true,	"DBRAIN4",	"DBRAIN1",	8},

    {-1}
};
#endif




/*      Animating line specials*/

#define LINEANIM_GRANULARITY	64

static dshort_t	numlinespecials;
static dshort_t	maxlinespecials;
static line_t**	linespeciallist;



void P_InitPicAnims (void)
{
    int		i;
#ifdef DIYBOOM
    byte*	animdefs;
    byte*	aptr;
    anim_t*	newAnims;

    animdefs = W_CacheLumpName("ANIMATED", PU_STATIC);

    for (i=0, aptr = animdefs; aptr[ANDEF_T_ISTEX] != 0xff; i++, aptr += ANDEF_T_SIZE) ;
    if ((newAnims = Z_MallocNoAbort((maxanims + i) * sizeof(anim_t), PU_STATIC, NULL)) == NULL)
        I_Error("P_InitPicAnims: unable to claim memory!");

    if (maxanims != 0)
    {
        memcpy(newAnims, anims, maxanims * sizeof(anim_t));
        Z_Free(anims);
    }
    anims = newAnims; maxanims += i;
    lastanim = anims;
    for (i=0, aptr = animdefs; aptr[ANDEF_T_ISTEX] != 0xff; i++, aptr += ANDEF_T_SIZE)
    {
       if (aptr[ANDEF_T_ISTEX])
       {
           if (R_CheckTextureNumForName((char*)(aptr + ANDEF_T_STRNM)) == -1) continue;

           lastanim->picnum  = R_TextureNumForName((char*)(aptr + ANDEF_T_ENDNM));
           lastanim->basepic = R_TextureNumForName((char*)(aptr + ANDEF_T_STRNM));
       }
       else
       {
           if (W_CheckNumForName2((char*)(aptr + ANDEF_T_ENDNM), ns_flats) == -1) continue;

           lastanim->picnum  = R_FlatNumForName((char*)(aptr + ANDEF_T_ENDNM));
           lastanim->basepic = R_FlatNumForName((char*)(aptr + ANDEF_T_STRNM));
       }
       lastanim->istexture = aptr[ANDEF_T_ISTEX];
       lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

       if (lastanim->numpics < 2)
           I_Error("P_InitPicAnims: bad cycle from %s to %s", aptr + ANDEF_T_STRNM, aptr + ANDEF_T_ENDNM);

       lastanim->speed = READ_UA_LONG(aptr + ANDEF_T_SPEED);
       lastanim++;
    }
    Z_ChangeTag(animdefs, PU_CACHE);
#else
    /*	Init animation*/
    lastanim = anims;
    for (i=0 ; animdefs[i].istexture != -1 ; i++)
    {
	if (animdefs[i].istexture)
	{
	    /* different episode ?*/
	    if (R_CheckTextureNumForName(animdefs[i].startname) == -1)
		continue;
	    lastanim->picnum = R_TextureNumForName (animdefs[i].endname);
	    lastanim->basepic = R_TextureNumForName (animdefs[i].startname);
	}
	else
	{
	    if (W_CheckNumForName2(animdefs[i].startname, ns_flats) == -1)
		continue;

	    lastanim->picnum = R_FlatNumForName (animdefs[i].endname);
	    lastanim->basepic = R_FlatNumForName (animdefs[i].startname);
	}

	lastanim->istexture = animdefs[i].istexture;
	lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

	if (lastanim->numpics < 2)
	    I_Error ("P_InitPicAnims: bad cycle from %s to %s",
		     animdefs[i].startname,
		     animdefs[i].endname);

	lastanim->speed = animdefs[i].speed;
	lastanim++;
    }
#endif

    /* lineanim structure */
    maxlinespecials = default_linespecials;
    if ((linespeciallist = (line_t**)Z_MallocNoAbort(maxlinespecials * sizeof(line_t*), PU_STATIC, NULL)) == NULL)
        I_Error("Couldn't claim memory for line specials");

    fprintf(logfile, "P_InitPicAnims: claimed %d linespecials, %d bytes\n", maxlinespecials, maxlinespecials * sizeof(line_t*));
}




/* UTILITIES*/





/* getSide()*/
/* Will return a side_t**/
/*  given the number of the current sector,*/
/*  the line number, and the side (0/1) that you want.*/

side_t*
getSide
( int		currentSector,
  int		line,
  int		side )
{
    return &sides[ (sectors[currentSector].lines[line])->sidenum[side] ];
}



/* getSector()*/
/* Will return a sector_t**/
/*  given the number of the current sector,*/
/*  the line number and the side (0/1) that you want.*/

sector_t*
getSector
( int		currentSector,
  int		line,
  int		side )
{
    return sides[ (sectors[currentSector].lines[line])->sidenum[side] ].sector;
}



/* twoSided()*/
/* Given the sector number and the line number,*/
/*  it will tell you whether the line is two-sided or not.*/

int
twoSided
( int	sector,
  int	line )
{
    BOOMSTATEMENT(if (compatibility))
    return (sectors[sector].lines[line])->flags & ML_TWOSIDED;
    BOOMSTATEMENT(else return ((sectors[sector].lines[line])->sidenum[1] != -1);)
}





/* getNextSector()*/
/* Return sector_t * of sector next to current.*/
/* NULL if not two-sided line*/

sector_t*
getNextSector
( const line_t*		line,
  const sector_t*	sec )
{
    BOOMSTATEMENT(if (compatibility))
    if (!(line->flags & ML_TWOSIDED))
	return NULL;

    if (line->frontsector == sec)
    {
        BOOMSTATEMENT(if ((compatibility) || (line->backsector!=sec)))
	return line->backsector;
	BOOMSTATEMENT(else return NULL;)
    }

    return line->frontsector;
}




/* P_FindLowestFloorSurrounding()*/
/* FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS*/

fixed_t	P_FindLowestFloorSurrounding(const sector_t* sec)
{
    int			i;
    line_t*		check;
    sector_t*		other;
    fixed_t		floor = sec->floorheight;

    for (i=0 ;i < sec->linecount ; i++)
    {
	check = sec->lines[i];
	other = getNextSector(check,sec);

	if (!other)
	    continue;

	if (other->floorheight < floor)
	    floor = other->floorheight;
    }
    return floor;
}




/* P_FindHighestFloorSurrounding()*/
/* FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS*/

fixed_t	P_FindHighestFloorSurrounding(const sector_t *sec)
{
    int			i;
    line_t*		check;
    sector_t*		other;
    fixed_t		floor = -500*FRACUNIT;

    BOOMSTATEMENT(if (!compatibility) floor = -32000*FRACUNIT;)

    for (i=0 ;i < sec->linecount ; i++)
    {
	check = sec->lines[i];
	other = getNextSector(check,sec);

	if (!other)
	    continue;

	if (other->floorheight > floor)
	    floor = other->floorheight;
    }
    return floor;
}




/* P_FindNextHighestFloor*/
/* FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS*/
/* (= lowest of all those higher than the current one) */
/* Removed the fixed array hack. */

fixed_t
P_FindNextHighestFloor
( const sector_t*	sec,
  int			currentheight )
{
    int			i;
    int			h;
    int			min;
    line_t*		check;
    sector_t*		other;
    fixed_t		height = currentheight;

    min = MAXINT;
    for (i=0, h=0;i < sec->linecount ; i++)
    {
	check = sec->lines[i];
	other = getNextSector(check,sec);

	if (!other)
	    continue;

	if (other->floorheight > height)
	{
	    h++;
	    if (other->floorheight < min) min = other->floorheight;
	}
    }

    if (h == 0)
        return currentheight;

    return min;
}



/* FIND LOWEST CEILING IN THE SURROUNDING SECTORS*/

fixed_t
P_FindLowestCeilingSurrounding(const sector_t* sec)
{
    int			i;
    line_t*		check;
    sector_t*		other;
    fixed_t		height = MAXINT;

    BOOMSTATEMENT(if (!compatibility) height = 32000*FRACUNIT;)

    for (i=0 ;i < sec->linecount ; i++)
    {
	check = sec->lines[i];
	other = getNextSector(check,sec);

	if (!other)
	    continue;

	if (other->ceilingheight < height)
	    height = other->ceilingheight;
    }
    return height;
}



/* FIND HIGHEST CEILING IN THE SURROUNDING SECTORS*/

fixed_t	P_FindHighestCeilingSurrounding(const sector_t* sec)
{
    int		i;
    line_t*	check;
    sector_t*	other;
    fixed_t	height = 0;

    BOOMSTATEMENT(if (!compatibility) height = -32000*FRACUNIT;)

    for (i=0 ;i < sec->linecount ; i++)
    {
	check = sec->lines[i];
	other = getNextSector(check,sec);

	if (!other)
	    continue;

	if (other->ceilingheight > height)
	    height = other->ceilingheight;
    }
    return height;
}




/* RETURN NEXT SECTOR # THAT LINE TAG REFERS TO*/

int
P_FindSectorFromLineTag
( const line_t*	line,
  int		start )
{
#ifdef DIYBOOM
  start = start >= 0 ? sectors[start].nexttag :
    sectors[(unsigned) line->tag % (unsigned) numsectors].firsttag;
  while (start >= 0 && sectors[start].tag != line->tag)
    start = sectors[start].nexttag;
  return start;
#else
    int	i;

    for (i=start+1;i<numsectors;i++)
	if (sectors[i].tag == line->tag)
	    return i;

    return -1;
#endif
}





/* Find minimum light from an adjacent sector*/

int
P_FindMinSurroundingLight
( const sector_t*	sector,
  int			max )
{
    int		i;
    int		min;
    line_t*	line;
    sector_t*	check;

    min = max;
    for (i=0 ; i < sector->linecount ; i++)
    {
	line = sector->lines[i];
	check = getNextSector(line,sector);

	if (!check)
	    continue;

	if (check->lightlevel < min)
	    min = check->lightlevel;
    }
    return min;
}




/* EVENTS*/
/* Events are operations triggered by using, crossing,*/
/* or shooting special lines, or by timed thinkers.*/



/* P_CrossSpecialLine - TRIGGER*/
/* Called every time a thing origin is about*/
/*  to cross a line with a non 0 special.*/

void
P_CrossSpecialLine
( int		linenum,
  int		side,
  mobj_t*	thing )
{
    line_t*	line;
    int		ok;

    line = &lines[linenum];

    /*	Triggers that other things can activate*/
    if (!thing->player)
    {
	/* Things that should NOT trigger specials...*/
	switch(thing->type)
	{
	  case MT_ROCKET:
	  case MT_PLASMA:
	  case MT_BFG:
	  case MT_TROOPSHOT:
	  case MT_HEADSHOT:
	  case MT_BRUISERSHOT:
	    return;
	    break;

	  default: break;
	}
#ifdef DIYBOOM
    }
    /*jff 02/04/98 add check here for generalized lindef types */
    if (!demo_compatibility) /* generalized types not recognized if old demo */
    {
      /* pointer to line function is NULL by default, set non-null if */
      /* line special is walkover generalized linedef type */
      int (*linefunc)(const line_t *line)=NULL;

      /* check each range of generalized linedefs */
      if ((unsigned)line->special >= GenFloorBase)
      {
        if (!thing->player)
          if ((line->special & FloorChange) || !(line->special & FloorModel))
            return;     /* FloorModel is "Allow Monsters" if FloorChange is 0 */
        if (!line->tag) /*jff 2/27/98 all walk generalized types require tag */
          return;
        linefunc = EV_DoGenFloor;
      }
      else if ((unsigned)line->special >= GenCeilingBase)
      {
        if (!thing->player)
          if ((line->special & CeilingChange) || !(line->special & CeilingModel))
            return;     /* CeilingModel is "Allow Monsters" if CeilingChange is 0 */
        if (!line->tag) /*jff 2/27/98 all walk generalized types require tag */
          return;
        linefunc = EV_DoGenCeiling;
      }
      else if ((unsigned)line->special >= GenDoorBase)
      {
        if (!thing->player)
        {
          if (!(line->special & DoorMonster))
            return;                    /* monsters disallowed from this door */
          if (line->flags & ML_SECRET) /* they can't open secret doors either */
            return;
        }
        if (!line->tag) /*3/2/98 move outside the monster check */
          return;
        linefunc = EV_DoGenDoor;
      }
      else if ((unsigned)line->special >= GenLockedBase)
      {
        if (!thing->player)
          return;                     /* monsters disallowed from unlocking doors */
        if (((line->special&TriggerType)==WalkOnce) || ((line->special&TriggerType)==WalkMany))
        { /*jff 4/1/98 check for being a walk type before reporting door type */
          if (!P_CanUnlockGenDoor(line,thing->player))
            return;
        }
        else
          return;
        linefunc = EV_DoGenLockedDoor;
      }
      else if ((unsigned)line->special >= GenLiftBase)
      {
        if (!thing->player)
          if (!(line->special & LiftMonster))
            return; /* monsters disallowed */
        if (!line->tag) /*jff 2/27/98 all walk generalized types require tag */
          return;
        linefunc = EV_DoGenLift;
      }
      else if ((unsigned)line->special >= GenStairsBase)
      {
        if (!thing->player)
          if (!(line->special & StairMonster))
            return; /* monsters disallowed */
        if (!line->tag) /*jff 2/27/98 all walk generalized types require tag */
          return;
        /* cast is OK because line is not constant in this function */
        linefunc = (int (*)(const line_t*)) EV_DoGenStairs;
      }

      if (linefunc) /* if it was a valid generalized type */
      {
        switch((line->special & TriggerType) >> TriggerTypeShift)
        {
          case WalkOnce:
            if (linefunc(line))
              line->special = 0;    /* clear special if a walk once type */
            return;
          case WalkMany:
            linefunc(line);
            return;
          default:                  /* if not a walk type, do nothing here */
            return;
        }
      }
    }

    if (!thing->player)
    {
#endif
	ok = 0;
	switch(line->special)
	{
	  case 39:	/* TELEPORT TRIGGER*/
	  case 97:	/* TELEPORT RETRIGGER*/
	  case 125:	/* TELEPORT MONSTERONLY TRIGGER*/
	  case 126:	/* TELEPORT MONSTERONLY RETRIGGER*/
	  case 4:	/* RAISE DOOR*/
	  case 10:	/* PLAT DOWN-WAIT-UP-STAY TRIGGER*/
	  case 88:	/* PLAT DOWN-WAIT-UP-STAY RETRIGGER*/
	    ok = 1;
	    break;
#ifdef DIYBOOM
	  case 208:
	  case 207:
	  case 243:
	  case 244:
	  case 262:
	  case 263:
	  case 264:
	  case 265:
	  case 266:
	  case 267:
	  case 268:
	  case 269:
	    if (!demo_compatibility)
	      ok = 1;
	    break;
#endif
	}
	if (!ok)
	    return;
    }

#ifdef DIYBOOM
    if (!demo_compatibility && !P_CheckTag(line)) return;
# define BOOMLINEFUNC(func)	if (func || demo_compatibility)
#else
# define BOOMLINEFUNC(func)	func;
#endif

    /* Note: could use some const's here.*/
    switch (line->special)
    {
	/* TRIGGERS.*/
	/* All from here to RETRIGGERS.*/
      case 2:
	/* Open Door*/
        BOOMLINEFUNC(EV_DoDoor(line,openDoor))
	line->special = 0;
	break;

      case 3:
	/* Close Door*/
	BOOMLINEFUNC(EV_DoDoor(line,closeDoor))
	line->special = 0;
	break;

      case 4:
	/* Raise Door*/
	BOOMLINEFUNC(EV_DoDoor(line,normal))
	line->special = 0;
	break;

      case 5:
	/* Raise Floor*/
	BOOMLINEFUNC(EV_DoFloor(line,raiseFloor))
	line->special = 0;
	break;

      case 6:
	/* Fast Ceiling Crush & Raise*/
	BOOMLINEFUNC(EV_DoCeiling(line,fastCrushAndRaise))
	line->special = 0;
	break;

      case 8:
	/* Build Stairs*/
	BOOMLINEFUNC(EV_BuildStairs(line,build8))
	line->special = 0;
	break;

      case 10:
	/* PlatDownWaitUp*/
	BOOMLINEFUNC(EV_DoPlat(line,downWaitUpStay,0))
	line->special = 0;
	break;

      case 12:
	/* Light Turn On - brightest near*/
	BOOMLINEFUNC(EV_LightTurnOn(line,0))
	line->special = 0;
	break;

      case 13:
	/* Light Turn On 255*/
	BOOMLINEFUNC(EV_LightTurnOn(line,255))
	line->special = 0;
	break;

      case 16:
	/* Close Door 30*/
	BOOMLINEFUNC(EV_DoDoor(line,close30ThenOpen))
	line->special = 0;
	break;

      case 17:
	/* Start Light Strobing*/
	BOOMLINEFUNC(EV_StartLightStrobing(line))
	line->special = 0;
	break;

      case 19:
	/* Lower Floor*/
	BOOMLINEFUNC(EV_DoFloor(line,lowerFloor))
	line->special = 0;
	break;

      case 22:
	/* Raise floor to nearest height and change texture*/
	BOOMLINEFUNC(EV_DoPlat(line,raiseToNearestAndChange,0))
	line->special = 0;
	break;

      case 25:
	/* Ceiling Crush and Raise*/
	BOOMLINEFUNC(EV_DoCeiling(line,crushAndRaise))
	line->special = 0;
	break;

      case 30:
	/* Raise floor to shortest texture height*/
	/*  on either side of lines.*/
	BOOMLINEFUNC(EV_DoFloor(line,raiseToTexture))
	line->special = 0;
	break;

      case 35:
	/* Lights Very Dark*/
	BOOMLINEFUNC(EV_LightTurnOn(line,35))
	line->special = 0;
	break;

      case 36:
	/* Lower Floor (TURBO)*/
	BOOMLINEFUNC(EV_DoFloor(line,turboLower))
	line->special = 0;
	break;

      case 37:
	/* LowerAndChange*/
	BOOMLINEFUNC(EV_DoFloor(line,lowerAndChange))
	line->special = 0;
	break;

      case 38:
	/* Lower Floor To Lowest*/
	BOOMLINEFUNC(EV_DoFloor(line,lowerFloorToLowest))
	line->special = 0;
	break;

      case 39:
	/* TELEPORT!*/
        BOOMLINEFUNC(EV_Teleport(line,side,thing))
	line->special = 0;
	break;

      case 40:
	/* RaiseCeilingLowerFloor*/
        BOOMSTATEMENT(if (demo_compatibility) {)
	EV_DoCeiling(line,raiseToHighest);
	EV_DoFloor(line,lowerFloorToLowest);
	line->special = 0;
#ifdef DIYBOOM
        }
        else
          if (EV_DoCeiling(line,raiseToHighest))
            line->special = 0;
#endif
	break;

      case 44:
	/* Ceiling Crush*/
	BOOMLINEFUNC(EV_DoCeiling(line,lowerAndCrush))
	line->special = 0;
	break;

      case 52:
	/* EXIT!*/
	G_ExitLevel ();
	break;

      case 53:
	/* Perpetual Platform Raise*/
	BOOMLINEFUNC(EV_DoPlat(line,perpetualRaise,0))
	line->special = 0;
	break;

      case 54:
	/* Platform Stop*/
	BOOMLINEFUNC(EV_StopPlat(line))
	line->special = 0;
	break;

      case 56:
	/* Raise Floor Crush*/
	BOOMLINEFUNC(EV_DoFloor(line,raiseFloorCrush))
	line->special = 0;
	break;

      case 57:
	/* Ceiling Crush Stop*/
	BOOMLINEFUNC(EV_CeilingCrushStop(line))
	line->special = 0;
	break;

      case 58:
	/* Raise Floor 24*/
	BOOMLINEFUNC(EV_DoFloor(line,raiseFloor24))
	line->special = 0;
	break;

      case 59:
	/* Raise Floor 24 And Change*/
	BOOMLINEFUNC(EV_DoFloor(line,raiseFloor24AndChange))
	line->special = 0;
	break;

      case 104:
	/* Turn lights off in sector(tag)*/
	BOOMLINEFUNC(EV_TurnTagLightsOff(line))
	line->special = 0;
	break;

      case 108:
	/* Blazing Door Raise (faster than TURBO!)*/
	BOOMLINEFUNC(EV_DoDoor (line,blazeRaise))
	line->special = 0;
	break;

      case 109:
	/* Blazing Door Open (faster than TURBO!)*/
	BOOMLINEFUNC(EV_DoDoor (line,blazeOpen))
	line->special = 0;
	break;

      case 100:
	/* Build Stairs Turbo 16*/
	BOOMLINEFUNC(EV_BuildStairs(line,turbo16))
	line->special = 0;
	break;

      case 110:
	/* Blazing Door Close (faster than TURBO!)*/
	BOOMLINEFUNC(EV_DoDoor (line,blazeClose))
	line->special = 0;
	break;

      case 119:
	/* Raise floor to nearest surr. floor*/
	BOOMLINEFUNC(EV_DoFloor(line,raiseFloorToNearest))
	line->special = 0;
	break;

      case 121:
	/* Blazing PlatDownWaitUpStay*/
	BOOMLINEFUNC(EV_DoPlat(line,blazeDWUS,0))
	line->special = 0;
	break;

      case 124:
	/* Secret EXIT*/
	G_SecretExitLevel ();
	break;

      case 125:
	/* TELEPORT MonsterONLY*/
	if (!thing->player)
	{
	    BOOMLINEFUNC(EV_Teleport( line, side, thing ))
	    line->special = 0;
	}
	break;

      case 130:
	/* Raise Floor Turbo*/
	BOOMLINEFUNC(EV_DoFloor(line,raiseFloorTurbo))
	line->special = 0;
	break;

      case 141:
	/* Silent Ceiling Crush & Raise*/
	BOOMLINEFUNC(EV_DoCeiling(line,silentCrushAndRaise))
	line->special = 0;
	break;

	/* RETRIGGERS.  All from here till end.*/
      case 72:
	/* Ceiling Crush*/
	EV_DoCeiling( line, lowerAndCrush );
	break;

      case 73:
	/* Ceiling Crush and Raise*/
	EV_DoCeiling(line,crushAndRaise);
	break;

      case 74:
	/* Ceiling Crush Stop*/
	EV_CeilingCrushStop(line);
	break;

      case 75:
	/* Close Door*/
	EV_DoDoor(line,closeDoor);
	break;

      case 76:
	/* Close Door 30*/
	EV_DoDoor(line,close30ThenOpen);
	break;

      case 77:
	/* Fast Ceiling Crush & Raise*/
	EV_DoCeiling(line,fastCrushAndRaise);
	break;

      case 79:
	/* Lights Very Dark*/
	EV_LightTurnOn(line,35);
	break;

      case 80:
	/* Light Turn On - brightest near*/
	EV_LightTurnOn(line,0);
	break;

      case 81:
	/* Light Turn On 255*/
	EV_LightTurnOn(line,255);
	break;

      case 82:
	/* Lower Floor To Lowest*/
	EV_DoFloor( line, lowerFloorToLowest );
	break;

      case 83:
	/* Lower Floor*/
	EV_DoFloor(line,lowerFloor);
	break;

      case 84:
	/* LowerAndChange*/
	EV_DoFloor(line,lowerAndChange);
	break;

      case 86:
	/* Open Door*/
	EV_DoDoor(line,openDoor);
	break;

      case 87:
	/* Perpetual Platform Raise*/
	EV_DoPlat(line,perpetualRaise,0);
	break;

      case 88:
	/* PlatDownWaitUp*/
	EV_DoPlat(line,downWaitUpStay,0);
	break;

      case 89:
	/* Platform Stop*/
	EV_StopPlat(line);
	break;

      case 90:
	/* Raise Door*/
	EV_DoDoor(line,normal);
	break;

      case 91:
	/* Raise Floor*/
	EV_DoFloor(line,raiseFloor);
	break;

      case 92:
	/* Raise Floor 24*/
	EV_DoFloor(line,raiseFloor24);
	break;

      case 93:
	/* Raise Floor 24 And Change*/
	EV_DoFloor(line,raiseFloor24AndChange);
	break;

      case 94:
	/* Raise Floor Crush*/
	EV_DoFloor(line,raiseFloorCrush);
	break;

      case 95:
	/* Raise floor to nearest height*/
	/* and change texture.*/
	EV_DoPlat(line,raiseToNearestAndChange,0);
	break;

      case 96:
	/* Raise floor to shortest texture height*/
	/* on either side of lines.*/
	EV_DoFloor(line,raiseToTexture);
	break;

      case 97:
	/* TELEPORT!*/
	EV_Teleport( line, side, thing );
	break;

      case 98:
	/* Lower Floor (TURBO)*/
	EV_DoFloor(line,turboLower);
	break;

      case 105:
	/* Blazing Door Raise (faster than TURBO!)*/
	EV_DoDoor (line,blazeRaise);
	break;

      case 106:
	/* Blazing Door Open (faster than TURBO!)*/
	EV_DoDoor (line,blazeOpen);
	break;

      case 107:
	/* Blazing Door Close (faster than TURBO!)*/
	EV_DoDoor (line,blazeClose);
	break;

      case 120:
	/* Blazing PlatDownWaitUpStay.*/
	EV_DoPlat(line,blazeDWUS,0);
	break;

      case 126:
	/* TELEPORT MonsterONLY.*/
	if (!thing->player)
	    EV_Teleport( line, side, thing );
	break;

      case 128:
	/* Raise To Nearest Floor*/
	EV_DoFloor(line,raiseFloorToNearest);
	break;

      case 129:
	/* Raise Floor Turbo*/
	EV_DoFloor(line,raiseFloorTurbo);
	break;

      default:
#ifdef DIYBOOM
      if (!demo_compatibility)
        switch (line->special)
        {
          /* Extended walk once triggers */

          case 142:
            /* Raise Floor 512 */
            /* 142 W1  EV_DoFloor(raiseFloor512) */
            if (EV_DoFloor(line,raiseFloor512))
              line->special = 0;
            break;

          case 143:
            /* Raise Floor 24 and change */
            /* 143 W1  EV_DoPlat(raiseAndChange,24) */
            if (EV_DoPlat(line,raiseAndChange,24))
              line->special = 0;
            break;

          case 144:
            /* Raise Floor 32 and change */
            /* 144 W1  EV_DoPlat(raiseAndChange,32) */
            if (EV_DoPlat(line,raiseAndChange,32))
              line->special = 0;
            break;

          case 145:
            /* Lower Ceiling to Floor */
            /* 145 W1  EV_DoCeiling(lowerToFloor) */
            if (EV_DoCeiling( line, lowerToFloor ))
              line->special = 0;
            break;

          case 146:
            /* Lower Pillar, Raise Donut */
            /* 146 W1  EV_DoDonut() */
            if (EV_DoDonut(line))
              line->special = 0;
            break;

          case 199:
            /* Lower ceiling to lowest surrounding ceiling */
            /* 199 W1 EV_DoCeiling(lowerToLowest) */
            if (EV_DoCeiling(line,lowerToLowest))
              line->special = 0;
            break;

          case 200:
            /* Lower ceiling to highest surrounding floor */
            /* 200 W1 EV_DoCeiling(lowerToMaxFloor) */
            if (EV_DoCeiling(line,lowerToMaxFloor))
              line->special = 0;
            break;

          case 207:
            /* killough 2/16/98: W1 silent teleporter (normal kind) */
            if (EV_SilentTeleport(line, side, thing))
              line->special = 0;
            break;

            /*jff 3/16/98 renumber 215->153 */
          case 153: /*jff 3/15/98 create texture change no motion type */
            /* Texture/Type Change Only (Trig) */
            /* 153 W1 Change Texture/Type Only */
            if (EV_DoChange(line,trigChangeOnly))
              line->special = 0;
            break;

          case 239: /*jff 3/15/98 create texture change no motion type */
            /* Texture/Type Change Only (Numeric) */
            /* 239 W1 Change Texture/Type Only */
            if (EV_DoChange(line,numChangeOnly))
              line->special = 0;
            break;

          case 219:
            /* Lower floor to next lower neighbor */
            /* 219 W1 Lower Floor Next Lower Neighbor */
            if (EV_DoFloor(line,lowerFloorToNearest))
              line->special = 0;
            break;

          case 227:
            /* Raise elevator next floor */
            /* 227 W1 Raise Elevator next floor */
            if (EV_DoElevator(line,elevateUp))
              line->special = 0;
            break;

          case 231:
            /* Lower elevator next floor */
            /* 231 W1 Lower Elevator next floor */
            if (EV_DoElevator(line,elevateDown))
              line->special = 0;
            break;

          case 235:
            /* Elevator to current floor */
            /* 235 W1 Elevator to current floor */
            if (EV_DoElevator(line,elevateCurrent))
              line->special = 0;
            break;

          case 243: /*jff 3/6/98 make fit within DCK's 256 linedef types */
            /* killough 2/16/98: W1 silent teleporter (linedef-linedef kind) */
            if (EV_SilentLineTeleport(line, side, thing, false))
              line->special = 0;
            break;

          case 262: /*jff 4/14/98 add silent line-line reversed */
            if (EV_SilentLineTeleport(line, side, thing, true))
              line->special = 0;
            break;

          case 264: /*jff 4/14/98 add monster-only silent line-line reversed */
            if (!thing->player &&
                EV_SilentLineTeleport(line, side, thing, true))
              line->special = 0;
            break;

          case 266: /*jff 4/14/98 add monster-only silent line-line */
            if (!thing->player &&
                EV_SilentLineTeleport(line, side, thing, false))
              line->special = 0;
            break;

          case 268: /*jff 4/14/98 add monster-only silent */
            if (!thing->player && EV_SilentTeleport(line, side, thing))
              line->special = 0;
            break;

          /*jff 1/29/98 end of added W1 linedef types */

          /* Extended walk many retriggerable */

          /*jff 1/29/98 added new linedef types to fill all functions */
          /*out so that all have varieties SR, S1, WR, W1 */

          case 147:
            /* Raise Floor 512 */
            /* 147 WR  EV_DoFloor(raiseFloor512) */
            EV_DoFloor(line,raiseFloor512);
            break;

          case 148:
            /* Raise Floor 24 and Change */
            /* 148 WR  EV_DoPlat(raiseAndChange,24) */
            EV_DoPlat(line,raiseAndChange,24);
            break;

          case 149:
            /* Raise Floor 32 and Change */
            /* 149 WR  EV_DoPlat(raiseAndChange,32) */
            EV_DoPlat(line,raiseAndChange,32);
            break;

          case 150:
            /* Start slow silent crusher */
            /* 150 WR  EV_DoCeiling(silentCrushAndRaise) */
            EV_DoCeiling(line,silentCrushAndRaise);
            break;

          case 151:
            /* RaiseCeilingLowerFloor */
            /* 151 WR  EV_DoCeiling(raiseToHighest), */
            /*         EV_DoFloor(lowerFloortoLowest) */
            EV_DoCeiling( line, raiseToHighest );
            EV_DoFloor( line, lowerFloorToLowest );
            break;

          case 152:
            /* Lower Ceiling to Floor */
            /* 152 WR  EV_DoCeiling(lowerToFloor) */
            EV_DoCeiling( line, lowerToFloor );
            break;

            /*jff 3/16/98 renumber 153->256 */
          case 256:
            /* Build stairs, step 8 */
            /* 256 WR EV_BuildStairs(build8) */
            EV_BuildStairs(line,build8);
            break;

            /*jff 3/16/98 renumber 154->257 */
          case 257:
            /* Build stairs, step 16 */
            /* 257 WR EV_BuildStairs(turbo16) */
            EV_BuildStairs(line,turbo16);
            break;

          case 155:
            /* Lower Pillar, Raise Donut */
            /* 155 WR  EV_DoDonut() */
            EV_DoDonut(line);
            break;

          case 156:
            /* Start lights strobing */
            /* 156 WR Lights EV_StartLightStrobing() */
            EV_StartLightStrobing(line);
            break;

          case 157:
            /* Lights to dimmest near */
            /* 157 WR Lights EV_TurnTagLightsOff() */
            EV_TurnTagLightsOff(line);
            break;

          case 201:
            /* Lower ceiling to lowest surrounding ceiling */
            /* 201 WR EV_DoCeiling(lowerToLowest) */
            EV_DoCeiling(line,lowerToLowest);
            break;

          case 202:
            /* Lower ceiling to highest surrounding floor */
            /* 202 WR EV_DoCeiling(lowerToMaxFloor) */
            EV_DoCeiling(line,lowerToMaxFloor);
            break;

          case 208:
            /* killough 2/16/98: WR silent teleporter (normal kind) */
            EV_SilentTeleport(line, side, thing);
            break;

          case 212: /*jff 3/14/98 create instant toggle floor type */
            /* Toggle floor between C and F instantly */
            /* 212 WR Instant Toggle Floor */
            EV_DoPlat(line,toggleUpDn,0);
            break;

          /*jff 3/16/98 renumber 216->154 */
          case 154: /*jff 3/15/98 create texture change no motion type */
            /* Texture/Type Change Only (Trigger) */
            /* 154 WR Change Texture/Type Only */
            EV_DoChange(line,trigChangeOnly);
            break;

          case 240: /*jff 3/15/98 create texture change no motion type */
            /* Texture/Type Change Only (Numeric) */
            /* 240 WR Change Texture/Type Only */
            EV_DoChange(line,numChangeOnly);
            break;

          case 220:
            /* Lower floor to next lower neighbor */
            /* 220 WR Lower Floor Next Lower Neighbor */
            EV_DoFloor(line,lowerFloorToNearest);
            break;

          case 228:
            /* Raise elevator next floor */
            /* 228 WR Raise Elevator next floor */
            EV_DoElevator(line,elevateUp);
            break;

          case 232:
            /* Lower elevator next floor */
            /* 232 WR Lower Elevator next floor */
            EV_DoElevator(line,elevateDown);
            break;

          case 236:
            /* Elevator to current floor */
            /* 236 WR Elevator to current floor */
            EV_DoElevator(line,elevateCurrent);
            break;

          case 244: /*jff 3/6/98 make fit within DCK's 256 linedef types */
            /* killough 2/16/98: WR silent teleporter (linedef-linedef kind) */
            EV_SilentLineTeleport(line, side, thing, false);
            break;

          case 263: /*jff 4/14/98 add silent line-line reversed */
            EV_SilentLineTeleport(line, side, thing, true);
            break;

          case 265: /*jff 4/14/98 add monster-only silent line-line reversed */
            if (!thing->player)
              EV_SilentLineTeleport(line, side, thing, true);
            break;

          case 267: /*jff 4/14/98 add monster-only silent line-line */
            if (!thing->player)
              EV_SilentLineTeleport(line, side, thing, false);
            break;

          case 269: /*jff 4/14/98 add monster-only silent */
            if (!thing->player)
              EV_SilentTeleport(line, side, thing);
            break;

            /*jff 1/29/98 end of added WR linedef types */
        }
#endif
        break;
    }
}




/* P_ShootSpecialLine - IMPACT SPECIALS*/
/* Called when a thing shoots a special line.*/

void
P_ShootSpecialLine
( mobj_t*	thing,
  line_t*	line )
{
    int		ok;

#ifdef DIYBOOM
    /*jff 02/04/98 add check here for generalized linedef */
    if (!demo_compatibility)
    {
      /* pointer to line function is NULL by default, set non-null if */
      /* line special is gun triggered generalized linedef type */
      int (*linefunc)(const line_t *line)=NULL;

      /* check each range of generalized linedefs */
      if ((unsigned)line->special >= GenFloorBase)
      {
        if (!thing->player)
          if ((line->special & FloorChange) || !(line->special & FloorModel))
            return;   /* FloorModel is "Allow Monsters" if FloorChange is 0 */
        if (!line->tag) /*jff 2/27/98 all gun generalized types require tag */
          return;

        linefunc = EV_DoGenFloor;
      }
      else if ((unsigned)line->special >= GenCeilingBase)
      {
        if (!thing->player)
          if ((line->special & CeilingChange) || !(line->special & CeilingModel))
            return;   /* CeilingModel is "Allow Monsters" if CeilingChange is 0 */
        if (!line->tag) /*jff 2/27/98 all gun generalized types require tag */
          return;
        linefunc = EV_DoGenCeiling;
      }
      else if ((unsigned)line->special >= GenDoorBase)
      {
        if (!thing->player)
        {
          if (!(line->special & DoorMonster))
            return;   /* monsters disallowed from this door */
          if (line->flags & ML_SECRET) /* they can't open secret doors either */
            return;
        }
        if (!line->tag) /*jff 3/2/98 all gun generalized types require tag */
          return;
        linefunc = EV_DoGenDoor;
      }
      else if ((unsigned)line->special >= GenLockedBase)
      {
        if (!thing->player)
          return;   /* monsters disallowed from unlocking doors */
        if (((line->special&TriggerType)==GunOnce) || ((line->special&TriggerType)==GunMany))
        { /*jff 4/1/98 check for being a gun type before reporting door type */
          if (!P_CanUnlockGenDoor(line,thing->player))
            return;
        }
        else
          return;
        if (!line->tag) /*jff 2/27/98 all gun generalized types require tag */
          return;

        linefunc = EV_DoGenLockedDoor;
      }
      else if ((unsigned)line->special >= GenLiftBase)
      {
        if (!thing->player)
          if (!(line->special & LiftMonster))
            return; /* monsters disallowed */
        linefunc = EV_DoGenLift;
      }
      else if ((unsigned)line->special >= GenStairsBase)
      {
        if (!thing->player)
          if (!(line->special & StairMonster))
            return; /* monsters disallowed */
        if (!line->tag) /*jff 2/27/98 all gun generalized types require tag */
          return;
        /* cast is OK because line is not constant in this function */
        linefunc = (int (*)(const line_t *)) EV_DoGenStairs;
      }
      else if ((unsigned)line->special >= GenCrusherBase)
      {
        if (!thing->player)
          if (!(line->special & StairMonster))
            return; /* monsters disallowed */
        if (!line->tag) /*jff 2/27/98 all gun generalized types require tag */
          return;
        linefunc = EV_DoGenCrusher;
      }

      if (linefunc)
        switch((line->special & TriggerType) >> TriggerTypeShift)
        {
          case GunOnce:
            if (linefunc(line))
              P_ChangeSwitchTexture(line,0);
            return;
          case GunMany:
            if (linefunc(line))
              P_ChangeSwitchTexture(line,1);
            return;
          default:  /* if not a gun type, do nothing here */
            return;
        }
    }
#endif

    /*	Impacts that other things can activate.*/
    if (!thing->player)
    {
	ok = 0;
	switch(line->special)
	{
	  case 46:
	    /* OPEN DOOR IMPACT*/
	    ok = 1;
	    break;
	}
	if (!ok)
	    return;
    }

    BOOMSTATEMENT(if (!demo_compatibility && !P_CheckTag(line)) return;)

    switch(line->special)
    {
      case 24:
	/* RAISE FLOOR*/
	EV_DoFloor(line,raiseFloor);
	P_ChangeSwitchTexture(line,0);
	break;

      case 46:
	/* OPEN DOOR*/
	EV_DoDoor(line,openDoor);
	P_ChangeSwitchTexture(line,1);
	break;

      case 47:
	/* RAISE FLOOR NEAR AND CHANGE*/
	EV_DoPlat(line,raiseToNearestAndChange,0);
	P_ChangeSwitchTexture(line,0);
	break;
      default:
#ifdef DIYBOOM
      if (!demo_compatibility)
        switch (line->special)
        {
          case 197:
            /* Exit to next level */
            P_ChangeSwitchTexture(line,0);
            G_ExitLevel();
            break;

          case 198:
            /* Exit to secret level */
            P_ChangeSwitchTexture(line,0);
            G_SecretExitLevel();
            break;
            /*jff end addition of new gun linedefs */
        }
#endif
        break;
    }
}




/* P_PlayerInSpecialSector*/
/* Called every tic frame*/
/*  that the player origin is in a special sector*/

void P_PlayerInSpecialSector (player_t* player)
{
    sector_t*	sector;

    sector = player->mo->subsector->sector;

    /* Falling, not all the way down yet?*/
    if (player->mo->z != sector->floorheight)
	return;

    BOOMSTATEMENT(if (sector->special < 32) { )

    /* Has hitten ground.*/
    switch (sector->special)
    {
      case 5:
	/* HELLSLIME DAMAGE*/
	if (!player->powers[pw_ironfeet])
	    if (!(leveltime&0x1f))
		P_DamageMobj (player->mo, NULL, NULL, 10);
	break;

      case 7:
	/* NUKAGE DAMAGE*/
	if (!player->powers[pw_ironfeet])
	    if (!(leveltime&0x1f))
		P_DamageMobj (player->mo, NULL, NULL, 5);
	break;

      case 16:
	/* SUPER HELLSLIME DAMAGE*/
      case 4:
	/* STROBE HURT*/
	if (!player->powers[pw_ironfeet]
	    || (P_Random()<5) )
	{
	    if (!(leveltime&0x1f))
		P_DamageMobj (player->mo, NULL, NULL, 20);
	}
	break;

      case 9:
	/* SECRET SECTOR*/
	player->secretcount++;
	sector->special = 0;
	break;

      case 11:
	/* EXIT SUPER DAMAGE! (for E1M8 finale)*/
        BOOMSTATEMENT(if (compatibility))
	player->cheats &= ~CF_GODMODE;

	if (!(leveltime&0x1f))
	    P_DamageMobj (player->mo, NULL, NULL, 20);

	if (player->health <= 10)
	    G_ExitLevel();
	break;

      default:
	fprintf(logfile, "P_PlayerInSpecialSector: unknown special %i\n", sector->special);
	break;
    }
#ifdef DIYBOOM
    }
    else if (!demo_compatibility)
    {
      switch ((sector->special&DAMAGE_MASK)>>DAMAGE_SHIFT)
      {
        case 0: /* no damage */
          break;
        case 1: /* 2/5 damage per 31 ticks */
          if (!player->powers[pw_ironfeet])
            if (!(leveltime&0x1f))
              P_DamageMobj (player->mo, NULL, NULL, 5);
          break;
        case 2: /* 5/10 damage per 31 ticks */
          if (!player->powers[pw_ironfeet])
            if (!(leveltime&0x1f))
              P_DamageMobj (player->mo, NULL, NULL, 10);
          break;
        case 3: /* 10/20 damage per 31 ticks */
          if (!player->powers[pw_ironfeet]
              || (P_Random()<5))
          {
              /* take damage even with suit */
              if (!(leveltime&0x1f))
              P_DamageMobj (player->mo, NULL, NULL, 20);
          }
          break;
      }
      if (sector->special&SECRET_MASK)
      {
        player->secretcount++;
        sector->special &= ~SECRET_MASK;
        if (sector->special<32) /* if all extended bits clear, */
          sector->special=0;    /* sector is not special anymore */
      }
    }
#endif
}





/* P_UpdateSpecials*/
/* Animate planes, scroll walls, etc.*/

boolean		levelTimer;
int		levelTimeCount;
BOOMSTATEMENT(boolean levelFragLimit=false;)
BOOMSTATEMENT(int levelFragLimitCount;)

void P_UpdateSpecials (void)
{
    anim_t*	anim;
    int		pic;
    int		i;
    line_t*	line;

    /*	LEVEL TIMER*/
    if (levelTimer == true)
    {
	levelTimeCount--;
	if (!levelTimeCount)
	    G_ExitLevel();
    }

#ifdef DIYBOOM
    if (levelFragLimit == true)  /* we used -frags so compare count */
    {
      int k,m,fragcount,exitflag=false;
      for (k=0;k<MAXPLAYERS;k++)
      {
        if (!playeringame[k]) continue;
        fragcount = 0;
        for (m=0;m<MAXPLAYERS;m++)
        {
          if (!playeringame[m]) continue;
            fragcount += (m!=k)?  players[k].frags[m] : -players[k].frags[m];
        }
        if (fragcount >= levelFragLimitCount) exitflag = true;
        if (exitflag == true) break; /* skip out of the loop--we're done */
      }
      if (exitflag == true)
        G_ExitLevel();
    }
#endif

    /*	ANIMATE FLATS AND TEXTURES GLOBALLY*/
    for (anim = anims ; anim < lastanim ; anim++)
    {
	for (i=anim->basepic ; i<anim->basepic+anim->numpics ; i++)
	{
	    pic = anim->basepic + ( (leveltime/anim->speed + i)%anim->numpics );
	    if (anim->istexture)
	    {
		if (i < numtextures) texturetranslation[i] = pic;
	    }
	    else
	    {
		if (i < numflats) flattranslation[i] = pic;
	    }
	}
    }


    /*	ANIMATE LINE SPECIALS*/
    for (i = 0; i < numlinespecials; i++)
    {
	line = linespeciallist[i];
	switch(line->special)
	{
	  case 48:
	    /* EFFECT FIRSTCOL SCROLL +*/
	    sides[line->sidenum[0]].textureoffset += FRACUNIT;
	    break;
	}
    }


    /*	DO BUTTONS*/
    for (i = 0; i < MAXBUTTONS; i++)
	if (buttonlist[i].btimer)
	{
	    buttonlist[i].btimer--;
	    if (!buttonlist[i].btimer)
	    {
		switch(buttonlist[i].where)
		{
		  case top:
		    sides[buttonlist[i].line->sidenum[0]].toptexture =
			buttonlist[i].btexture;
		    break;

		  case middle:
		    sides[buttonlist[i].line->sidenum[0]].midtexture =
			buttonlist[i].btexture;
		    break;

		  case bottom:
		    sides[buttonlist[i].line->sidenum[0]].bottomtexture =
			buttonlist[i].btexture;
		    break;
		}
		S_StartSound((mobj_t *)&buttonlist[i].soundorg,sfx_swtchn);
		memset(&buttonlist[i],0,sizeof(button_t));
	    }
	}

}




/* Special Stuff that can not be categorized*/





/* SPECIAL SPAWNING*/



/* P_SpawnSpecials*/
/* After the map has been loaded, scan for specials*/
/*  that spawn thinkers*/


/* Parses command line parameters.*/
void P_SpawnSpecials (void)
{
    sector_t*	sector;
    int		i;
    int		episode;

    episode = 1;
    if (W_CheckNumForName("texture2") >= 0)
	episode = 2;


    /* See if -TIMER needs to be used.*/
    levelTimer = false;

    i = M_CheckParm("-avg");
    if (i && deathmatch)
    {
	levelTimer = true;
	levelTimeCount = 20 * 60 * 35;
    }

    i = M_CheckParm("-timer");
    if (i && deathmatch)
    {
	int	time;
	time = atoi(myargv[i+1]) * 60 * 35;
	levelTimer = true;
	levelTimeCount = time;
    }

#ifdef DIYBOOM
    levelFragLimit = false;
    i = M_CheckParm("-frags");
    if (i && deathmatch)
    {
        int frags;
        frags = atoi(myargv[i+1]);
        if (frags <= 0) frags = 10;
        levelFragLimit = true;
        levelFragLimitCount = frags;
    }
#endif

    /*	Init special SECTORs.*/
    sector = sectors;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
	if (!sector->special)
	    continue;

        BOOMSTATEMENT(if (sector->special & SECRET_MASK) totalsecret++;)

	switch (sector->special)
	{
	  case 1:
	    /* FLICKERING LIGHTS*/
	    P_SpawnLightFlash (sector);
	    break;

	  case 2:
	    /* STROBE FAST*/
	    P_SpawnStrobeFlash(sector,FASTDARK,0);
	    break;

	  case 3:
	    /* STROBE SLOW*/
	    P_SpawnStrobeFlash(sector,SLOWDARK,0);
	    break;

	  case 4:
	    /* STROBE FAST/DEATH SLIME*/
	    P_SpawnStrobeFlash(sector,FASTDARK,0);
#ifdef DIYBOOM
            if (!compatibility && !demo_compatibility)
              sector->special |= 3<<DAMAGE_SHIFT;
            else
#endif
	    sector->special = 4;
	    break;

	  case 8:
	    /* GLOWING LIGHT*/
	    P_SpawnGlowingLight(sector);
	    break;
	  case 9:
	    /* SECRET SECTOR*/
	    BOOMSTATEMENT(if (sector->special < 32))
	    totalsecret++;
	    break;

	  case 10:
	    /* DOOR CLOSE IN 30 SECONDS*/
	    P_SpawnDoorCloseIn30 (sector);
	    break;

	  case 12:
	    /* SYNC STROBE SLOW*/
	    P_SpawnStrobeFlash (sector, SLOWDARK, 1);
	    break;

	  case 13:
	    /* SYNC STROBE FAST*/
	    P_SpawnStrobeFlash (sector, FASTDARK, 1);
	    break;

	  case 14:
	    /* DOOR RAISE IN 5 MINUTES*/
	    P_SpawnDoorRaiseIn5Mins (sector, i);
	    break;

	  case 17:
	    P_SpawnFireFlicker(sector);
	    break;
	}
    }


    P_InitCeilings();
    P_InitPlatforms();

    /*	Init other misc stuff*/
    for (i = 0;i < MAXBUTTONS;i++)
	memset(&buttonlist[i],0,sizeof(button_t));

#ifdef DIYBOOM
    P_InitTagLists();

    if (!compatibility && !demo_compatibility)
    {
        P_SpawnScrollers();
        P_SpawnFriction();
        P_SpawnPushers();

        for (i=0; i<numlines; i++)
        {
          switch (lines[i].special)
          {
            int s, sec;

            /* killough 3/7/98: */
            /* support for drawn heights coming from different sector */
            case 242:
              sec = sides[*lines[i].sidenum].sector-sectors;
              for (s = -1; (s = P_FindSectorFromLineTag(lines+i,s)) >= 0;)
                sectors[s].heightsec = sec;
              break;

            /* killough 3/16/98: Add support for setting */
            /* floor lighting independently (e.g. lava) */
            case 213:
              sec = sides[*lines[i].sidenum].sector-sectors;
              for (s = -1; (s = P_FindSectorFromLineTag(lines+i,s)) >= 0;)
                sectors[s].floorlightsec = sec;
              break;

            /* killough 4/11/98: Add support for setting */
            /* ceiling lighting independently */
            case 261:
              sec = sides[*lines[i].sidenum].sector-sectors;
              for (s = -1; (s = P_FindSectorFromLineTag(lines+i,s)) >= 0;)
                sectors[s].ceilinglightsec = sec;
              break;
          }
        }
    }
#endif

    /*	Init line EFFECTs*/
    numlinespecials = 0;
    for (i = 0;i < numlines; i++)
    {
	switch(lines[i].special)
	{
	  case 48:
	    if (numlinespecials >= maxlinespecials)
	    {
	        line_t**	newSpecials;
	        int		wantSize;

	        wantSize = maxlinespecials + LINEANIM_GRANULARITY;
	        if (maximum_linespecials)
	        {
	            if (maxlinespecials >= maximum_linespecials) break;
	            if (wantSize > maximum_linespecials) wantSize = maximum_linespecials;
	        }
	        if ((newSpecials = (line_t**)Z_MallocNoAbort(wantSize * sizeof(line_t*), PU_STATIC, NULL)) != NULL)
	        {
	            memcpy(newSpecials, linespeciallist, maxlinespecials * sizeof(line_t*));
	            Z_Free(linespeciallist);
	            linespeciallist = newSpecials; maxlinespecials = wantSize;
	            fprintf(logfile, "P_SpawnSpecials: claimed %d linespecials, %d bytes\n", maxlinespecials, maxlinespecials * sizeof(line_t*));
	        }
	    }
	    if (numlinespecials < maxlinespecials)
	    {
	        /* EFFECT FIRSTCOL SCROLL+*/
	        linespeciallist[numlinespecials] = &lines[i];
	        numlinespecials++;
	    }
	    break;
	}
    }


    /* UNUSED: no horizonal sliders.*/
    /*	P_InitSlidingDoorFrames();*/
}




#ifdef DIYBOOM

/* here we go... */

int P_SectorActive(special_e t, const sector_t *sec)
{
  BOOMTRACEOUT("P_SectorActive")

  if (demo_compatibility)  /* return whether any thinker is active */
    return sec->floordata || sec->ceilingdata || sec->lightingdata;
  else
  {
    switch (t)             /* return whether thinker of same type is active */
    {
      case floor_special:
        return (int)sec->floordata;
      case ceiling_special:
        return (int)sec->ceilingdata;
      case lighting_special:
        return (int)sec->lightingdata;
    }
  }
  return 1; /* don't know which special, must be active, shouldn't be here */
}

int P_CheckTag(const line_t *line)
{
  BOOMTRACEOUT("P_CheckTag")

  if (compatibility)        /* killough: allow zero tags in compatibility mode */
    return 1;

  if (line->tag)            /* tag not zero, allowed */
    return 1;

  switch(line->special)
  {
    case 1:                 /* Manual door specials */
    case 26:
    case 27:
    case 28:
    case 31:
    case 32:
    case 33:
    case 34:
    case 117:
    case 118:

    case 139:               /* Lighting specials */
    case 170:
    case 79:
    case 35:
    case 138:
    case 171:
    case 81:
    case 13:
    case 192:
    case 169:
    case 80:
    case 12:
    case 194:
    case 173:
    case 157:
    case 104:
    case 193:
    case 172:
    case 156:
    case 17:

    case 195:               /* Thing teleporters */
    case 174:
    case 97:
    case 39:
    case 126:
    case 125:
    case 210:
    case 209:
    case 208:
    case 207:

    case 11:                /* Exits */
    case 52:
    case 197:
    case 51:
    case 124:
    case 198:

    case 48:                /* Scrolling walls */
    case 85:
      return 1;   /* zero tag allowed */

    default:
      break;
  }
  return 0;       /* zero tag not allowed */
}


sector_t *P_FindModelFloorSector(fixed_t floordestheight,int secnum)
{
  int i;
  sector_t *sec=NULL;
  int linecount;

  BOOMTRACEOUT("P_FindModelFloorSector")

  sec = &sectors[secnum];
  linecount = sec->linecount;
  for (i = 0; i < (demo_compatibility && sec->linecount<linecount?sec->linecount : linecount); i++)
  {
    if ( twoSided(secnum, i) )
    {
      if (getSide(secnum,i,0)->sector-sectors == secnum)
          sec = getSector(secnum,i,1);
      else
          sec = getSector(secnum,i,0);

      if (sec->floorheight == floordestheight)
        return sec;
    }
  }
  return NULL;
}


fixed_t P_FindNextLowestFloor(const sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  BOOMTRACEOUT("P_FindNextLowestFloor")

  for (i=0 ;i < sec->linecount ; i++)
    if ((other = getNextSector(sec->lines[i],sec)) &&
         other->floorheight < currentheight)
    {
      int height = other->floorheight;
      while (++i < sec->linecount)
        if ((other = getNextSector(sec->lines[i],sec)) &&
            other->floorheight > height &&
            other->floorheight < currentheight)
          height = other->floorheight;
      return height;
    }
  return currentheight;
}

fixed_t P_FindShortestTextureAround(int secnum)
{
  int minsize = MAXINT;
  side_t*     side;
  int i;
  sector_t *sec = &sectors[secnum];

  BOOMTRACEOUT("P_FindShortestTextureAround")

  if (!compatibility)
    minsize = 32000<<FRACBITS; /*jff 3/13/98 prevent overflow in height calcs */

  for (i = 0; i < sec->linecount; i++)
  {
    if (twoSided(secnum, i))
    {
      side = getSide(secnum,i,0);
      if (side->bottomtexture > 0)  /*jff 8/14/98 texture 0 is a placeholder */
        if (textureheight[side->bottomtexture] < minsize)
          minsize = textureheight[side->bottomtexture];
      side = getSide(secnum,i,1);
      if (side->bottomtexture > 0)  /*jff 8/14/98 texture 0 is a placeholder */
        if (textureheight[side->bottomtexture] < minsize)
          minsize = textureheight[side->bottomtexture];
    }
  }
  return minsize;
}

fixed_t P_FindShortestUpperAround(int secnum)
{
  int minsize = MAXINT;
  side_t*     side;
  int i;
  sector_t *sec = &sectors[secnum];

  BOOMTRACEOUT("P_FindShortestUpperAround")

  if (!compatibility)
    minsize = 32000<<FRACBITS; /*jff 3/13/98 prevent overflow */
                               /* in height calcs */
  for (i = 0; i < sec->linecount; i++)
  {
    if (twoSided(secnum, i))
    {
      side = getSide(secnum,i,0);
      if (side->toptexture > 0)     /*jff 8/14/98 texture 0 is a placeholder */
        if (textureheight[side->toptexture] < minsize)
          minsize = textureheight[side->toptexture];
      side = getSide(secnum,i,1);
      if (side->toptexture > 0)     /*jff 8/14/98 texture 0 is a placeholder */
        if (textureheight[side->toptexture] < minsize)
          minsize = textureheight[side->toptexture];
    }
  }
  return minsize;
}

sector_t *P_FindModelCeilingSector(fixed_t ceildestheight,int secnum)
{
  int i;
  sector_t *sec=NULL;
  int linecount;

  BOOMTRACEOUT("P_FindModelCeilingSector")

  sec = &sectors[secnum]; /*jff 3/2/98 woops! better do this */
  /*jff 5/23/98 don't disturb sec->linecount while searching */
  /* but allow early exit in old demos */
  linecount = sec->linecount;
  for (i = 0; i < (demo_compatibility && sec->linecount<linecount?
                   sec->linecount : linecount); i++)
  {
    if ( twoSided(secnum, i) )
    {
      if (getSide(secnum,i,0)->sector-sectors == secnum)
          sec = getSector(secnum,i,1);
      else
          sec = getSector(secnum,i,0);

      if (sec->ceilingheight == ceildestheight)
        return sec;
    }
  }
  return NULL;
}

fixed_t P_FindNextLowestCeiling(const sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  BOOMTRACEOUT("P_FindNextLowestCeiling")

  for (i=0 ;i < sec->linecount ; i++)
    if ((other = getNextSector(sec->lines[i],sec)) &&
        other->ceilingheight < currentheight)
    {
      int height = other->ceilingheight;
      while (++i < sec->linecount)
        if ((other = getNextSector(sec->lines[i],sec)) &&
            other->ceilingheight > height &&
            other->ceilingheight < currentheight)
          height = other->ceilingheight;
      return height;
    }
  return currentheight;
}

boolean P_CanUnlockGenDoor
( const line_t* line,
  player_t* player)
{
  /* does this line special distinguish between skulls and keys? */
  int skulliscard = (line->special & LockedNKeys)>>LockedNKeysShift;

  BOOMTRACEOUT("P_CanUnlockGenDoor")

  /* determine for each case of lock type if player's keys are adequate */
  switch((line->special & LockedKey)>>LockedKeyShift)
  {
    case AnyKey:
      if
      (
        !player->cards[it_redcard] &&
        !player->cards[it_redskull] &&
        !player->cards[it_bluecard] &&
        !player->cards[it_blueskull] &&
        !player->cards[it_yellowcard] &&
        !player->cards[it_yellowskull]
      )
      {
        player->message = pdoors_messages[MSG_PD_ANY];
        S_StartSound(player->mo,sfx_oof);             /* killough 3/20/98 */
        return false;
      }
      break;
    case RCard:
      if
      (
        !player->cards[it_redcard] &&
        (!skulliscard || !player->cards[it_redskull])
      )
      {
        player->message = pdoors_messages[skulliscard ? MSG_PD_REDK : MSG_PD_REDC];
        S_StartSound(player->mo,sfx_oof);             /* killough 3/20/98 */
        return false;
      }
      break;
    case BCard:
      if
      (
        !player->cards[it_bluecard] &&
        (!skulliscard || !player->cards[it_blueskull])
      )
      {
        player->message = pdoors_messages[skulliscard ? MSG_PD_BLUEK : MSG_PD_BLUEC];
        S_StartSound(player->mo,sfx_oof);             /* killough 3/20/98 */
        return false;
      }
      break;
    case YCard:
      if
      (
        !player->cards[it_yellowcard] &&
        (!skulliscard || !player->cards[it_yellowskull])
      )
      {
        player->message = pdoors_messages[skulliscard ? MSG_PD_YELLOWK : MSG_PD_YELLOWC];
        S_StartSound(player->mo,sfx_oof);             /* killough 3/20/98 */
        return false;
      }
      break;
    case RSkull:
      if
      (
        !player->cards[it_redskull] &&
        (!skulliscard || !player->cards[it_redcard])
      )
      {
        player->message = pdoors_messages[skulliscard ? MSG_PD_REDK : MSG_PD_REDS];
        S_StartSound(player->mo,sfx_oof);             /* killough 3/20/98 */
        return false;
      }
      break;
    case BSkull:
      if
      (
        !player->cards[it_blueskull] &&
        (!skulliscard || !player->cards[it_bluecard])
      )
      {
        player->message = pdoors_messages[skulliscard ? MSG_PD_BLUEK : MSG_PD_BLUES];
        S_StartSound(player->mo,sfx_oof);             /* killough 3/20/98 */
        return false;
      }
      break;
    case YSkull:
      if
      (
        !player->cards[it_yellowskull] &&
        (!skulliscard || !player->cards[it_yellowcard])
      )
      {
        player->message = pdoors_messages[skulliscard ? MSG_PD_YELLOWK : MSG_PD_YELLOWS];
        S_StartSound(player->mo,sfx_oof);             /* killough 3/20/98 */
        return false;
      }
      break;
    case AllKeys:
      if
      (
        !skulliscard &&
        (
          !player->cards[it_redcard] ||
          !player->cards[it_redskull] ||
          !player->cards[it_bluecard] ||
          !player->cards[it_blueskull] ||
          !player->cards[it_yellowcard] ||
          !player->cards[it_yellowskull]
        )
      )
      {
        player->message = pdoors_messages[MSG_PD_ALL6];
        S_StartSound(player->mo,sfx_oof);             /* killough 3/20/98 */
        return false;
      }
      if
      (
        skulliscard &&
        (
          (!player->cards[it_redcard] &&
            !player->cards[it_redskull]) ||
          (!player->cards[it_bluecard] &&
            !player->cards[it_blueskull]) ||
          (!player->cards[it_yellowcard] &&
            !player->cards[it_yellowskull])
        )
      )
      {
        player->message = pdoors_messages[MSG_PD_ALL3];
        S_StartSound(player->mo,sfx_oof);             /* killough 3/20/98 */
        return false;
      }
      break;
  }
  return true;
}

fixed_t P_FindNextHighestCeiling(const sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  BOOMTRACEOUT("P_FindNextHighestCeiling")

  for (i=0 ;i < sec->linecount ; i++)
    if ((other = getNextSector(sec->lines[i],sec)) &&
         other->ceilingheight > currentheight)
    {
      int height = other->ceilingheight;
      while (++i < sec->linecount)
        if ((other = getNextSector(sec->lines[i],sec)) &&
            other->ceilingheight < height &&
            other->ceilingheight > currentheight)
          height = other->ceilingheight;
      return height;
    }
  return currentheight;
}

int P_FindLineFromLineTag(const line_t *line, int start)
{
  BOOMTRACEOUT("P_FindLineFromLineTag")

  start = start >= 0 ? lines[start].nexttag :
    lines[(unsigned) line->tag % (unsigned) numlines].firsttag;
  while (start >= 0 && lines[start].tag != line->tag)
    start = lines[start].nexttag;
  return start;
}

static void P_InitTagLists(void)
{
  register int i;

  BOOMTRACEOUT("P_InitTagLists")

  for (i=numsectors; --i>=0; )        /* Initially make all slots empty. */
    sectors[i].firsttag = -1;
  for (i=numsectors; --i>=0; )        /* Proceed from last to first sector */
    {                                 /* so that lower sectors appear first */
      int j = (unsigned) sectors[i].tag % (unsigned) numsectors; /* Hash func */
      sectors[i].nexttag = sectors[j].firsttag;   /* Prepend sector to chain */
      sectors[j].firsttag = i;
    }

  /* killough 4/17/98: same thing, only for linedefs */

  for (i=numlines; --i>=0; )        /* Initially make all slots empty. */
    lines[i].firsttag = -1;
  for (i=numlines; --i>=0; )        /* Proceed from last to first linedef */
    {                               /* so that lower linedefs appear first */
      int j = (unsigned) lines[i].tag % (unsigned) numlines; /* Hash func */
      lines[i].nexttag = lines[j].firsttag;   /* Prepend linedef to chain */
      lines[j].firsttag = i;
    }
}

boolean P_IsSecret(const sector_t *sec)
{
  BOOMTRACEOUT("P_IsSecret")

  return (sec->special==9 || (sec->special&SECRET_MASK));
}

boolean P_WasSecret(const sector_t *sec)
{
  BOOMTRACEOUT("P_WasSecret")

  return (sec->oldspecial==9 || (sec->oldspecial&SECRET_MASK));
}


void T_Scroll(scroll_t *s)
{
    fixed_t dx = s->dx, dy = s->dy;

    BOOMTRACEOUT("T_Scroll")

    if (s->control != -1)
    {
      /* compute scroll amounts based on a sector's height changes */
      fixed_t height = sectors[s->control].floorheight +
        sectors[s->control].ceilingheight;
      fixed_t delta = height - s->last_height;
      s->last_height = height;
      dx = FixedMul(dx, delta);
      dy = FixedMul(dy, delta);
    }

    /* killough 3/14/98: Add acceleration */
    if (s->accel)
    {
      s->vdx = dx += s->vdx;
      s->vdy = dy += s->vdy;
    }

    if (!(dx | dy))                   /* no-op if both (x,y) offsets 0 */
      return;

    switch (s->type)
    {
      side_t *side;
      sector_t *sec;
      fixed_t height, waterheight;  /* killough 4/4/98: add waterheight */
      msecnode_t *node;
      mobj_t *thing;

      case sc_side:                   /* killough 3/7/98: Scroll wall texture */
        side = sides + s->affectee;
        side->textureoffset += dx;
        side->rowoffset += dy;
        break;

      case sc_floor:                  /* killough 3/7/98: Scroll floor texture */
        sec = sectors + s->affectee;
        sec->floor_xoffs += dx;
        sec->floor_yoffs += dy;
        break;

      case sc_ceiling:               /* killough 3/7/98: Scroll ceiling texture */
        sec = sectors + s->affectee;
        sec->ceiling_xoffs += dx;
        sec->ceiling_yoffs += dy;
        break;

      case sc_carry:
        sec = sectors + s->affectee;
        height = sec->floorheight;
        waterheight = sec->heightsec != -1 &&
        sectors[sec->heightsec].floorheight > height ?
        sectors[sec->heightsec].floorheight : MININT;

        for (node = sec->touching_thinglist; node; node = node->m_snext)
          if (!((thing = node->m_thing)->flags & MF_NOCLIP) &&
             (!(thing->flags & MF_NOGRAVITY || thing->z > height) ||
                thing->z < waterheight))
          {
            /* Move objects only if on floor or underwater, */
            /* non-floating, and clipped. */
            thing->momx += dx;
            thing->momy += dy;
          }
        break;

      case sc_carry_ceiling:       /* to be added later */
        break;
    }
}

static void Add_Scroller(int type, fixed_t dx, fixed_t dy,
                         int control, int affectee, int accel)
{
  scroll_t *s = Z_Malloc(sizeof *s, PU_LEVSPEC, 0);
  s->thinker.function.acp1 = (actionf_p1) T_Scroll;
  s->type = type;
  s->dx = dx;
  s->dy = dy;
  s->accel = accel;
  s->vdx = s->vdy = 0;

  BOOMTRACEOUT("Add_Scroller")

  if ((s->control = control) != -1)
  {
    s->last_height =
      sectors[control].floorheight + sectors[control].ceilingheight;
  }
  s->affectee = affectee;
  P_AddThinker(&s->thinker);
}

static void Add_WallScroller(fixed_t dx, fixed_t dy, const line_t *l, int control, int accel)
{
  fixed_t x = abs(l->dx), y = abs(l->dy), d;

  BOOMTRACEOUT("Add_WallScroller")

  if (y > x)
  {
    d = x; x = y; y = d;
  }
  d = FixedDiv(x, finesine[(tantoangle[FixedDiv(y,x) >> DBITS] + ANG90) >> ANGLETOFINESHIFT]);
  x = -FixedDiv(FixedMul(dy, l->dy) + FixedMul(dx, l->dx), d);
  y = -FixedDiv(FixedMul(dx, l->dy) - FixedMul(dy, l->dx), d);
  Add_Scroller(sc_side, x, y, control, *l->sidenum, accel);
}

/* Amount (dx,dy) vector linedef is shifted right to get scroll amount */
#define SCROLL_SHIFT 5

/* Factor to scale scrolling effect into mobj-carrying properties = 3/32. */
/* (This is so scrolling floors and objects on them can move at same speed.) */
#define CARRYFACTOR ((fixed_t)(FRACUNIT*.09375))

/* Initialize the scrollers */
static void P_SpawnScrollers(void)
{
    int i;
    line_t *l = lines;

    BOOMTRACEOUT("P_SpawnScrollers")

    for (i=0;i<numlines;i++,l++)
    {
      fixed_t dx = l->dx >> SCROLL_SHIFT;  /* direction and speed of scrolling */
      fixed_t dy = l->dy >> SCROLL_SHIFT;
      int control = -1, accel = 0;         /* no control sector or acceleration */
      int special = l->special;

      if (special >= 245 && special <= 249)         /* displacement scrollers */
      {
          special += 250-245;
          control = sides[*l->sidenum].sector - sectors;
      }
      else
      {
        if (special >= 214 && special <= 218)       /* accelerative scrollers */
          {
            accel = 1;
            special += 250-214;
            control = sides[*l->sidenum].sector - sectors;
          }
      }

      switch (special)
      {
        register int s;

        case 250:   /* scroll effect ceiling */
          for (s=-1; (s = P_FindSectorFromLineTag(l,s)) >= 0;)
            Add_Scroller(sc_ceiling, -dx, dy, control, s, accel);
          break;

        case 251:   /* scroll effect floor */
        case 253:   /* scroll and carry objects on floor */
          for (s=-1; (s = P_FindSectorFromLineTag(l,s)) >= 0;)
            Add_Scroller(sc_floor, -dx, dy, control, s, accel);
          if (special != 253)
            break;

        case 252: /* carry objects on floor */
          dx = FixedMul(dx,CARRYFACTOR);
          dy = FixedMul(dy,CARRYFACTOR);
          for (s=-1; (s = P_FindSectorFromLineTag(l,s)) >= 0;)
            Add_Scroller(sc_carry, dx, dy, control, s, accel);
          break;

          /* killough 3/1/98: scroll wall according to linedef */
          /* (same direction and speed as scrolling floors) */
        case 254:
          for (s=-1; (s = P_FindLineFromLineTag(l,s)) >= 0;)
            if (s != i)
              Add_WallScroller(dx, dy, lines+s, control, accel);
          break;

        case 255:    /* killough 3/2/98: scroll according to sidedef offsets */
          s = lines[i].sidenum[0];
          Add_Scroller(sc_side, -sides[s].textureoffset,
                       sides[s].rowoffset, -1, s, accel);
          break;

        case 48:                  /* scroll first side */
          Add_Scroller(sc_side,  FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
          break;

        case 85:                  /* jff 1/30/98 2-way scroll */
          Add_Scroller(sc_side, -FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
          break;
        }
    }
}


static void Add_Friction(int friction, int movefactor, int affectee)
{
    friction_t *f = Z_Malloc(sizeof *f, PU_LEVSPEC, 0);

    BOOMTRACEOUT("Add_Friction")

    f->thinker.function.acp1 = (actionf_p1) T_Friction;
    f->friction = friction;
    f->movefactor = movefactor;
    f->affectee = affectee;
    P_AddThinker(&f->thinker);
}

void T_Friction(friction_t *f)
{
    sector_t *sec;
    mobj_t   *thing;
    msecnode_t* node;

    BOOMTRACEOUT("T_Friction")

    if (compatibility || !variable_friction) return;

    sec = sectors + f->affectee;

    /* Be sure the special sector type is still turned on. If so, proceed. */
    /* Else, bail out; the sector type has been changed on us. */

    if (!(sec->special & FRICTION_MASK)) return;

    /* Assign the friction value to players on the floor, non-floating, */
    /* and clipped. Normally the object's friction value is kept at */
    /* ORIG_FRICTION and this thinker changes it for icy or muddy floors. */

    /* In Phase II, you can apply friction to Things other than players. */

    /* When the object is straddling sectors with the same */
    /* floorheight that have different frictions, use the lowest */
    /* friction value (muddy has precedence over icy). */

    node = sec->touching_thinglist; /* things touching this sector */
    while (node)
    {
        thing = node->m_thing;
        if ((thing->player) &&
           !(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)) &&
            (thing->z <= sec->floorheight))
        {
            /* normal friction? */
            if ((thing->friction == ORIG_FRICTION) || (f->friction < thing->friction))
            {
                thing->friction   = f->friction;
                thing->movefactor = f->movefactor;
            }
        }
        node = node->m_snext;
    }
}

static void P_SpawnFriction(void)
{
    int i;
    line_t *l = lines;
    register int s;
    int length;     /* line length controls magnitude */
    int friction;   /* friction value to be applied during movement */
    int movefactor; /* applied to each player move to simulate inertia */

    BOOMTRACEOUT("P_SpawnFriction")

    for (i = 0 ; i < numlines ; i++,l++)
    {
        if (l->special == 223)
        {
            length = P_AproxDistance(l->dx,l->dy)>>FRACBITS;
            friction = (0x1EB8*length)/0x80 + 0xD000;

            /* The following check might seem odd. At the time of movement, */
            /* the move distance is multiplied by 'friction/0x10000', so a */
            /* higher friction value actually means 'less friction'. */

            if (friction > ORIG_FRICTION)       /* ice */
                movefactor = ((0x10092 - friction)*(0x70))/0x158;
            else
                movefactor = ((friction - 0xDB34)*(0xA))/0x80;
            for (s = -1; (s = P_FindSectorFromLineTag(l,s)) >= 0 ; )
                Add_Friction(friction,movefactor,s);
        }
    }
}


#define PUSH_FACTOR 7

static void Add_Pusher(int type, int x_mag, int y_mag, mobj_t* source, int affectee)
{
    pusher_t *p = Z_Malloc(sizeof *p, PU_LEVSPEC, 0);

    BOOMTRACEOUT("Add_Pushers")

    p->thinker.function.acp1 = (actionf_p1) T_Pusher;
    p->source = source;
    p->type = type;
    p->x_mag = x_mag>>FRACBITS;
    p->y_mag = y_mag>>FRACBITS;
    p->magnitude = P_AproxDistance(p->x_mag,p->y_mag);
    if (source) /* point source exist? */
    {
        p->radius = (p->magnitude)<<(FRACBITS+1); /* where force goes to zero */
        p->x = p->source->x;
        p->y = p->source->y;
    }
    p->affectee = affectee;
    P_AddThinker(&p->thinker);
}

static pusher_t* tmpusher = NULL; /* pusher structure for blockmap searches */

boolean PIT_PushThing(mobj_t* thing)
{
    BOOMTRACEOUT("PIT_PushThing")

    if (thing->player && !(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
    {
        angle_t pushangle;
        int dist;
        int speed;
        int sx,sy;

        sx = tmpusher->x;
        sy = tmpusher->y;
        dist = P_AproxDistance(thing->x - sx,thing->y - sy);
        speed = (tmpusher->magnitude -
                 ((dist>>FRACBITS)>>1))<<(FRACBITS-PUSH_FACTOR-1);

        /* If speed <= 0, you're outside the effective radius. You also have */
        /* to be able to see the push/pull source point. */

        if ((speed > 0) && (P_CheckSight(thing,tmpusher->source)))
        {
            pushangle = R_PointToAngle2(thing->x,thing->y,sx,sy);
            if (tmpusher->source->type == MT_PUSH)
                pushangle += ANG180;    /* away */
            pushangle >>= ANGLETOFINESHIFT;
            thing->momx += FixedMul(speed,finecosine[pushangle]);
            thing->momy += FixedMul(speed,finesine[pushangle]);
        }
    }
    return true;
}

void T_Pusher(pusher_t *p)
{
    sector_t *sec;
    mobj_t   *thing;
    msecnode_t* node;
    int xspeed,yspeed;
    int xl,xh,yl,yh,bx,by;
    int radius;
    int ht = 0;

    BOOMTRACEOUT("T_Pusher")

    if (!allow_pushers)
        return;

    sec = sectors + p->affectee;

    /* Be sure the special sector type is still turned on. If so, proceed. */
    /* Else, bail out; the sector type has been changed on us. */

    if (!(sec->special & PUSH_MASK))
        return;

    /* For constant pushers (wind/current) there are 3 situations: */
    /* */
    /* 1) Affected Thing is above the floor. */
    /*    Apply the full force if wind, no force if current. */
    /* 2) Affected Thing is on the ground. */
    /*    Apply half force if wind, full force if current. */
    /* 3) Affected Thing is below the ground (underwater effect). */
    /*    Apply no force if wind, full force if current. */

    if (p->type == p_push)
    {

        /* Seek out all pushable things within the force radius of this */
        /* point pusher. Crosses sectors, so use blockmap. */

        tmpusher = p; /* MT_PUSH/MT_PULL point source */
        radius = p->radius; /* where force goes to zero */
        tmbbox[BOXTOP]    = p->y + radius;
        tmbbox[BOXBOTTOM] = p->y - radius;
        tmbbox[BOXRIGHT]  = p->x + radius;
        tmbbox[BOXLEFT]   = p->x - radius;

        xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
        xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
        yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
        yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
        for (bx=xl ; bx<=xh ; bx++)
            for (by=yl ; by<=yh ; by++)
                P_BlockThingsIterator(bx,by,PIT_PushThing);
        return;
    }

    /* constant pushers p_wind and p_current */

    if (sec->heightsec != -1) /* special water sector? */
        ht = sectors[sec->heightsec].floorheight;
    node = sec->touching_thinglist; /* things touching this sector */
    for ( ; node ; node = node->m_snext)
    {
        thing = node->m_thing;
        if (!thing->player || (thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
            continue;

        if (p->type == p_wind)
        {
            if (sec->heightsec == -1) /* NOT special water sector */
            {
                if (thing->z > thing->floorz) /* above ground */
                {
                    xspeed = p->x_mag; /* full force */
                    yspeed = p->y_mag;
                }
                else /* on ground */
                {
                    xspeed = (p->x_mag)>>1; /* half force */
                    yspeed = (p->y_mag)>>1;
                }
            }
            else /* special water sector */
            {
                if (thing->z > ht) /* above ground */
                {
                    xspeed = p->x_mag; /* full force */
                    yspeed = p->y_mag;
                }
                else if (thing->player->viewz < ht) /* underwater */
                {
                    xspeed = yspeed = 0; /* no force */
                }
                else /* wading in water */
                {
                    xspeed = (p->x_mag)>>1; /* half force */
                    yspeed = (p->y_mag)>>1;
                }
            }
        }
        else /* p_current */
        {
            if (sec->heightsec == -1) /* NOT special water sector */
            {
                if (thing->z > sec->floorheight) /* above ground */
                {
                    xspeed = yspeed = 0; /* no force */
                }
                else /* on ground */
                {
                    xspeed = p->x_mag; /* full force */
                    yspeed = p->y_mag;
                }
            }
            else /* special water sector */
            {
                if (thing->z > ht) /* above ground */
                {
                    xspeed = yspeed = 0; /* no force */
                }
                else /* underwater */
                {
                    xspeed = p->x_mag; /* full force */
                    yspeed = p->y_mag;
                }
            }
        }
        thing->momx += xspeed<<(FRACBITS-PUSH_FACTOR);
        thing->momy += yspeed<<(FRACBITS-PUSH_FACTOR);
    }
}

mobj_t* P_GetPushThing(int s)
{
    mobj_t* thing;
    sector_t* sec;

    BOOMTRACEOUT("P_GetPushThing")

    sec = sectors + s;
    thing = sec->thinglist;
    while (thing)
    {
        switch(thing->type)
        {
          case MT_PUSH:
          case MT_PULL:
            return thing;
          default:
            break;
        }
        thing = thing->snext;
    }
    return NULL;
}

static void P_SpawnPushers(void)
{
    int i;
    line_t *l = lines;
    register int s;
    mobj_t* thing;

    BOOMTRACEOUT("P_SpawnPushers")

    for (i = 0 ; i < numlines ; i++,l++)
    {
        switch(l->special)
        {
          case 224: /* wind */
            for (s = -1; (s = P_FindSectorFromLineTag(l,s)) >= 0 ; )
                Add_Pusher(p_wind,l->dx,l->dy,NULL,s);
            break;
          case 225: /* current */
            for (s = -1; (s = P_FindSectorFromLineTag(l,s)) >= 0 ; )
                Add_Pusher(p_current,l->dx,l->dy,NULL,s);
            break;
          case 226: /* push/pull */
            for (s = -1; (s = P_FindSectorFromLineTag(l,s)) >= 0 ; )
            {
                thing = P_GetPushThing(s);
                if (thing) /* No MT_P* means no effect */
                    Add_Pusher(p_push,l->dx,l->dy,thing,s);
            }
            break;
        }
    }
}

#endif
