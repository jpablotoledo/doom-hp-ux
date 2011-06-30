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

/* DESCRIPTION:  Ceiling aninmation (lowering, crushing, raising)*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: p_ceilng.c,v 1.4 1997/02/03 16:47:53 b1 Exp $";


#include "i_system.h"
#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

/* State.*/
#include "doomstat.h"
#include "r_state.h"

/* Data.*/
#include "sounds.h"


/* CEILINGS*/




int		maxceilings = 0;
int		freeceiling = 0;
ceiling_t	**activeceilings=NULL;



/* T_MoveCeiling*/


void T_MoveCeiling (ceiling_t* ceiling)
{
    result_e	res;

    switch(ceiling->direction)
    {
      case 0:
	/* IN STASIS*/
	break;
      case 1:
	/* UP*/
	res = T_MovePlane(ceiling->sector,
			  ceiling->speed,
			  ceiling->topheight,
			  false,1,ceiling->direction);

	if (!(leveltime&7))
	{
	    switch(ceiling->type)
	    {
	      case silentCrushAndRaise:
	      BOOMSTATEMENT(case genSilentCrusher:)
		break;
	      default:
		S_StartSound((mobj_t *)&ceiling->sector->soundorg,
			     sfx_stnmov);
		/* ?*/
		break;
	    }
	}

	if (res == pastdest)
	{
	    switch(ceiling->type)
	    {
	      case raiseToHighest:
		P_RemoveActiveCeiling(ceiling);
		break;
#ifdef DIYBOOM
              case genCeiling:
                if (!demo_compatibility)
                  P_RemoveActiveCeiling(ceiling);
                break;
              case genCeilingChgT:
              case genCeilingChg0:
                if (!demo_compatibility)
                {
                  ceiling->sector->special = ceiling->newspecial;
                  ceiling->sector->oldspecial = ceiling->oldspecial;
                }
              case genCeilingChg:
                if (!demo_compatibility)
                {
                  ceiling->sector->ceilingpic = ceiling->texture;
                  P_RemoveActiveCeiling(ceiling);
                }
                break;
              case genSilentCrusher:
              case genCrusher:
                if (!demo_compatibility)
                  ceiling->direction = -1;
                break;
#endif
	      case silentCrushAndRaise:
		S_StartSound((mobj_t *)&ceiling->sector->soundorg, sfx_pstop);
	      case fastCrushAndRaise:
	      case crushAndRaise:
		ceiling->direction = -1;
		break;
	      default:
		break;
	    }

	}
	break;

      case -1:
	/* DOWN*/
	res = T_MovePlane(ceiling->sector,
			  ceiling->speed,
			  ceiling->bottomheight,
			  ceiling->crush,1,ceiling->direction);

	if (!(leveltime&7))
	{
	    switch(ceiling->type)
	    {
	      case silentCrushAndRaise:
	      BOOMSTATEMENT(case genSilentCrusher:)
	        break;
	      default:
		S_StartSound((mobj_t *)&ceiling->sector->soundorg,
			     sfx_stnmov);
	    }
	}

	if (res == pastdest)
	{
	    switch(ceiling->type)
	    {
	      case silentCrushAndRaise:
		S_StartSound((mobj_t *)&ceiling->sector->soundorg,
			     sfx_pstop);
	      case crushAndRaise:
		ceiling->speed = CEILSPEED;
	      case fastCrushAndRaise:
		ceiling->direction = 1;
		break;

#ifdef DIYBOOM
              case genSilentCrusher:
              case genCrusher:
                if (!demo_compatibility)
                {
                  if (ceiling->oldspeed<CEILSPEED*3)
                    ceiling->speed = ceiling->oldspeed;
                  ceiling->direction = 1;
                }
                break;
              case genCeilingChgT:
              case genCeilingChg0:
                if (!demo_compatibility)
                {
                  ceiling->sector->special = ceiling->newspecial;
                  ceiling->sector->oldspecial = ceiling->oldspecial;
                }
              case genCeilingChg:
                if (!demo_compatibility)
                {
                  ceiling->sector->ceilingpic = ceiling->texture;
                  P_RemoveActiveCeiling(ceiling);
                }
                break;
              case lowerToLowest:
              case lowerToMaxFloor:
              case genCeiling:
                if (!demo_compatibility)
                {
                  P_RemoveActiveCeiling(ceiling);
                }
                break;
#endif
	      case lowerAndCrush:
	      case lowerToFloor:
		P_RemoveActiveCeiling(ceiling);
		break;
	      default:
		break;
	    }
	}
	else /* ( res != pastdest )*/
	{
	    if (res == crushed)
	    {
		switch(ceiling->type)
		{
		  case silentCrushAndRaise:
		  case crushAndRaise:
		  case lowerAndCrush:
		    ceiling->speed = CEILSPEED / 8;
		    break;
#ifdef DIYBOOM
                  case genCrusher:
                  case genSilentCrusher:
                    if (!demo_compatibility)
                    {
                      if (ceiling->oldspeed < CEILSPEED*3)
                        ceiling->speed = CEILSPEED / 8;
                    }
                    break;
#endif
		  default:
		    break;
		}
	    }
	}
	break;
    }
}



/* EV_DoCeiling*/
/* Move a ceiling up/down and all around!*/

int
EV_DoCeiling
( const line_t*	line,
  ceiling_e	type )
{
    int		secnum;
    int		rtn;
    sector_t*	sec;
    ceiling_t*	ceiling;

    secnum = -1;
    rtn = 0;

    /*	Reactivate in-stasis ceilings...for certain types.*/
    switch(type)
    {
      case fastCrushAndRaise:
      case silentCrushAndRaise:
      case crushAndRaise:
	P_ActivateInStasisCeiling(line);
      default:
	break;
    }

    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
	sec = &sectors[secnum];
#ifdef DIYBOOM
        if (P_SectorActive(ceiling_special, sec))
            continue;
#else
	if (sec->specialdata)
	    continue;
#endif

	/* new door thinker*/
	rtn = 1;
	ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVSPEC, 0);
	P_AddThinker (&ceiling->thinker);
	sec->BOOMCEILGDATA = ceiling;
	ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;
	ceiling->sector = sec;
	ceiling->crush = false;

	switch(type)
	{
	  case fastCrushAndRaise:
	    ceiling->crush = true;
	    ceiling->topheight = sec->ceilingheight;
	    ceiling->bottomheight = sec->floorheight + (8*FRACUNIT);
	    ceiling->direction = -1;
	    ceiling->speed = CEILSPEED * 2;
	    break;

	  case silentCrushAndRaise:
	  case crushAndRaise:
	    ceiling->crush = true;
	    ceiling->topheight = sec->ceilingheight;
	  case lowerAndCrush:
	  case lowerToFloor:
	    ceiling->bottomheight = sec->floorheight;
	    if (type != lowerToFloor)
		ceiling->bottomheight += 8*FRACUNIT;
	    ceiling->direction = -1;
	    ceiling->speed = CEILSPEED;
	    break;

	  case raiseToHighest:
	    ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
	    ceiling->direction = 1;
	    ceiling->speed = CEILSPEED;
	    break;
#ifdef DIYBOOM
          case lowerToLowest:
            if (!demo_compatibility)
            {
              ceiling->bottomheight = P_FindLowestCeilingSurrounding(sec);
              ceiling->direction = -1;
              ceiling->speed = CEILSPEED;
            }
            break;
          case lowerToMaxFloor:
            if (!demo_compatibility)
            {
              ceiling->bottomheight = P_FindHighestFloorSurrounding(sec);
              ceiling->direction = -1;
              ceiling->speed = CEILSPEED;
            }
            break;
#endif
          default:
            break;
	}

	ceiling->tag = sec->tag;
	ceiling->type = type;
	P_AddActiveCeiling(ceiling);
    }
    return rtn;
}



/* Add an active ceiling*/

void P_AddActiveCeiling(ceiling_t* c)
{
    for (; freeceiling < maxceilings; freeceiling++)
    {
        if (activeceilings[freeceiling] == NULL) break;
    }
    if (freeceiling >= maxceilings)
    {
        ceiling_t **newCeilings;
        int wantCeil;

        wantCeil = maxceilings + MAXCEILINGS;
        if (maximum_ceilings)
        {
            if (maxceilings >= maximum_ceilings) wantCeil = 0;
            if (wantCeil > maximum_ceilings) wantCeil = maximum_ceilings;
        }
        if ((wantCeil != 0) && ((newCeilings = Z_MallocNoAbort(wantCeil*sizeof(ceiling_t*), PU_STATIC, NULL)) != NULL))
        {
            memcpy(newCeilings, activeceilings, maxceilings*sizeof(ceiling_t*));
            memset(newCeilings + maxceilings, 0, (wantCeil-maxceilings)*sizeof(ceiling_t*));
            Z_Free(activeceilings); activeceilings = newCeilings;
            maxceilings = wantCeil;
        }
        else
        {
            BOOMSTATEMENT(c->activeidx = -1;)
            return;
        }
    }
    activeceilings[freeceiling] = c;
    BOOMSTATEMENT(c->activeidx = freeceiling;)
    freeceiling++;
}




/* Remove a ceiling's thinker*/

void P_RemoveActiveCeiling(ceiling_t* c)
{
#ifdef DIYBOOM
    /* normally I expect this to be the case always! */
    if ((c->activeidx >= 0) && (activeceilings[c->activeidx] == c))
    {
        c->sector->ceilingdata = NULL;
        /*if (demo_compatibility) TEST
        {
          c->sector->floordata = NULL; c->sector->lightingdata = NULL;
        }*/
        P_RemoveThinker(&c->thinker);
        activeceilings[c->activeidx] = NULL;
        if (freeceiling > c->activeidx) freeceiling = c->activeidx;
        c->activeidx = -1;
    }
#else
    int i;

    for (i=0; i<maxceilings; i++)
    {
        if (activeceilings[i] == c)
        {
            c->sector->specialdata = NULL;
            P_RemoveThinker(&c->thinker);
            activeceilings[i] = NULL;
            if (freeceiling > i) freeceiling = i;
            return;
        }
    }
#endif
}


void P_InitCeilings(void)
{
    if (activeceilings == NULL)
    {
        maxceilings = default_ceilings;
        if ((activeceilings = Z_MallocNoAbort(maxceilings*sizeof(ceiling_t*), PU_STATIC, NULL)) == NULL)
            I_Error("Couldn't init ceilings");
        freeceiling = 0;
        memset(activeceilings, 0, maxceilings * sizeof(ceiling_t*));
    }
}


/* Restart a ceiling that's in-stasis*/

int P_ActivateInStasisCeiling(const line_t* line)
{
    int		i;
    int		rtn=0;

    for (i = 0; i < maxceilings; i++)
    {
	if (activeceilings[i]
	    && (activeceilings[i]->tag == line->tag)
	    && (activeceilings[i]->direction == 0))
	{
	    activeceilings[i]->direction = activeceilings[i]->olddirection;
	    activeceilings[i]->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;
	    rtn=1;
	}
    }
    return rtn;
}




/* EV_CeilingCrushStop*/
/* Stop a ceiling from crushing!*/

int	EV_CeilingCrushStop(const line_t	*line)
{
    int		i;
    int		rtn;

    rtn = 0;
    for (i = 0; i < maxceilings; i++)
    {
	if (activeceilings[i]
	    && (activeceilings[i]->tag == line->tag)
	    && (activeceilings[i]->direction != 0))
	{
	    activeceilings[i]->olddirection = activeceilings[i]->direction;
	    activeceilings[i]->thinker.function.acv = (actionf_v)NULL;
	    activeceilings[i]->direction = 0;		/* in-stasis*/
	    rtn = 1;
	}
    }

    return rtn;
}
