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

/* DESCRIPTION: Door animation code (opening/closing)*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: p_doors.c,v 1.4 1997/02/03 16:47:53 b1 Exp $";


#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"


/* State.*/
#include "doomstat.h"
#include "r_state.h"

/* Data.*/
#include "dstrings.h"
#include "sounds.h"

/* For pdoors_messages */
#include "p_inter.h"


#if 0

/* Sliding door frame information*/

slidename_t	slideFrameNames[MAXSLIDEDOORS] =
{
    {"GDOORF1","GDOORF2","GDOORF3","GDOORF4",	/* front*/
     "GDOORB1","GDOORB2","GDOORB3","GDOORB4"},	/* back*/

    {"\0","\0","\0","\0"}
};
#endif



/*
 *  For easy DeHackEd support
 */

char *pdoors_messages[PDOORS_MESSAGES] = {
  PD_BLUEO,
  PD_REDO,
  PD_YELLOWO,
  PD_BLUEK,
  PD_REDK,
  PD_YELLOWK
#ifdef DIYBOOM
  ,
  PD_ANY,
  PD_REDC,
  PD_BLUEC,
  PD_YELLOWC,
  PD_REDS,
  PD_BLUES,
  PD_YELLOWS,
  PD_ALL3,
  PD_ALL6
#endif
};




/* VERTICAL DOORS*/



/* T_VerticalDoor*/

void T_VerticalDoor (vldoor_t* door)
{
    result_e	res;

    switch(door->direction)
    {
      case 0:
	/* WAITING*/
	if (!--door->topcountdown)
	{
	    vldoor_e dtype = door->type;

#ifdef DIYBOOM
            /* equivalence to old door types, treat new ones */
            if (!demo_compatibility)
            {
	      switch (door->type)
	      {
	        case genBlazeRaise:
	          dtype = blazeRaise; break;
	        case genRaise:
	          dtype = normal; break;
	        case genCdO:
	          dtype = close30ThenOpen; break;
                case genBlazeCdO:
                  door->direction = 1;  /* time to go back up */
                  S_StartSound((mobj_t *)&door->sector->soundorg,sfx_bdopn);
                  break;
	        default:
	          break;
	      }
            }
#endif
	    switch(dtype)
	    {
	      case blazeRaise:
		door->direction = -1; /* time to go back down*/
		S_StartSound((mobj_t *)&door->sector->soundorg, sfx_bdcls);
		break;

	      case normal:
		door->direction = -1; /* time to go back down*/
		S_StartSound((mobj_t *)&door->sector->soundorg, sfx_dorcls);
		break;

	      case close30ThenOpen:
		door->direction = 1;
		S_StartSound((mobj_t *)&door->sector->soundorg, sfx_doropn);
		break;

	      default:
		break;
	    }
	}
	break;

      case 2:
	/*  INITIAL WAIT*/
	if (!--door->topcountdown)
	{
	    switch(door->type)
	    {
	      case raiseIn5Mins:
		door->direction = 1;
		door->type = normal;
		S_StartSound((mobj_t *)&door->sector->soundorg, sfx_doropn);
		break;

	      default:
		break;
	    }
	}
	break;

      case -1:
	/* DOWN*/
	res = T_MovePlane(door->sector,
			  door->speed,
			  door->sector->floorheight,
			  false,1,door->direction);
	if (res == pastdest)
	{
	    vldoor_e dtype = door->type;

#ifdef DIYBOOM
            if (!demo_compatibility)
            {
              switch (door->type)
              {
                case genBlazeRaise:
                case genBlazeClose:
                  dtype = blazeRaise; break;
                case genRaise:
                case genClose:
                  dtype = normal; break;
                case genCdO:
                case genBlazeCdO:
                  door->direction = 0;
                  door->topcountdown = door->topwait;
                  break;
                default:
                  break;
              }
            }
#endif

	    switch(dtype)
	    {
	      case blazeRaise:
	      case blazeClose:
		door->sector->BOOMCEILGDATA = NULL;
#if 0
                if (demo_compatibility)
                {
                  door->sector->floordata = NULL; door->sector->lightingdata = NULL; /* TEST */
                }
#endif
		P_RemoveThinker (&door->thinker);  /* unlink and free*/
		BOOMSTATEMENT(if (compatibility))
		S_StartSound((mobj_t *)&door->sector->soundorg, sfx_bdcls);
		break;

	      case normal:
	      case closeDoor:
		door->sector->BOOMCEILGDATA = NULL;
#if 0
		if (demo_compatibility)
		{
		  door->sector->floordata = NULL; door->sector->lightingdata = NULL; /* TEST */
		}
#endif
		P_RemoveThinker (&door->thinker);  /* unlink and free*/
		break;

	      case close30ThenOpen:
		door->direction = 0;
		door->topcountdown = 35*30;
		break;

	      default:
		break;
	    }
#ifdef DIYBOOM
            if (!compatibility && door->line && door->line->tag)
            {
                if (door->line->special > GenLockedBase && (door->line->special&6)==6)
                    EV_TurnTagLightsOff(door->line);
                else
                {
                    switch (door->line->special)
                    {
                        case 1: case 31:
                        case 26:
                        case 27: case 28:
                        case 32: case 33:
                        case 34: case 117:
                        case 118:
                          EV_TurnTagLightsOff(door->line);
                        default:
                          break;
                    }
                }
            }
#endif
	}
	else if (res == crushed)
	{
	    switch(door->type)
	    {
	      case blazeClose:
	      case closeDoor:		/* DO NOT GO BACK UP!*/
		break;

#ifdef DIYBOOM
	      case genClose:
	      case genBlazeClose:
	        if (!demo_compatibility)
	          break;
#endif
	      default:
		door->direction = 1;
		S_StartSound((mobj_t *)&door->sector->soundorg, sfx_doropn);
		break;
	    }
	}
	break;

      case 1:
	/* UP*/
	res = T_MovePlane(door->sector,
			  door->speed,
			  door->topheight,
			  false,1,door->direction);

	if (res == pastdest)
	{
	    vldoor_e dtype = door->type;

#ifdef DIYBOOM
            if (!demo_compatibility)
            {
              switch (door->type)
              {
                case genRaise:
                case genBlazeRaise:
                  dtype = normal; break;
                case genBlazeOpen:
                case genOpen:
                case genCdO:
                case genBlazeCdO:
                  dtype = openDoor; break;
                default:
                  break;
              }
            }
#endif
	    switch(dtype)
	    {
	      case blazeRaise:
	      case normal:
		door->direction = 0; /* wait at top*/
		door->topcountdown = door->topwait;
		break;

	      case close30ThenOpen:
	      case blazeOpen:
	      case openDoor:
		door->sector->BOOMCEILGDATA = NULL;
#if 0
                if (demo_compatibility)
                {
                  door->sector->floordata = NULL; door->sector->lightingdata = NULL; /* TEST */
                }
#endif
		P_RemoveThinker (&door->thinker);  /* unlink and free*/
		break;

	      default:
		break;
	    }
#ifdef DIYBOOM
            if (!compatibility && door->line && door->line->tag)
            {
                if (door->line->special > GenLockedBase && (door->line->special&6)==6)
                    EV_LightTurnOn(door->line,0);
                else
                {
                    switch (door->line->special)
                    {
                        case 1: case 31:
                        case 26:
                        case 27: case 28:
                        case 32: case 33:
                        case 34: case 117:
                        case 118:
                          EV_LightTurnOn(door->line,0);
                        default:
                          break;
                    }
	        }
            }
#endif
	}
	break;
    }
}



/* EV_DoLockedDoor*/
/* Move a locked door up/down*/


int
EV_DoLockedDoor
( const line_t*	line,
  vldoor_e	type,
  mobj_t*	thing )
{
    player_t*	p;

    p = thing->player;

    if (!p)
	return 0;

    switch(line->special)
    {
      case 99:	/* Blue Lock*/
      case 133:
	if ( !p )
	    return 0;
	if (!p->cards[it_bluecard] && !p->cards[it_blueskull])
	{
	    p->message = pdoors_messages[MSG_PD_BLUEO];
	    S_StartSound(NULL,sfx_oof);
	    return 0;
	}
	break;

      case 134: /* Red Lock*/
      case 135:
	if ( !p )
	    return 0;
	if (!p->cards[it_redcard] && !p->cards[it_redskull])
	{
	    p->message = pdoors_messages[MSG_PD_REDO];
	    S_StartSound(NULL,sfx_oof);
	    return 0;
	}
	break;

      case 136:	/* Yellow Lock*/
      case 137:
	if ( !p )
	    return 0;
	if (!p->cards[it_yellowcard] &&
	    !p->cards[it_yellowskull])
	{
	    p->message = pdoors_messages[MSG_PD_YELLOWO];
	    S_StartSound(NULL,sfx_oof);
	    return 0;
	}
	break;
    }

    return EV_DoDoor(line,type);
}


int
EV_DoDoor
( const line_t*	line,
  vldoor_e	type )
{
    int		secnum,rtn;
    sector_t*	sec;
    vldoor_t*	door;

    secnum = -1;
    rtn = 0;

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
	door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
	P_AddThinker (&door->thinker);
	sec->BOOMCEILGDATA = door;

	door->thinker.function.acp1 = (actionf_p1) T_VerticalDoor;
	door->sector = sec;
	door->type = type;
	door->topwait = VDOORWAIT;
	door->speed = VDOORSPEED;
	BOOMSTATEMENT(door->line = line;)

	switch(type)
	{
	  case blazeClose:
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    door->direction = -1;
	    door->speed = VDOORSPEED * 4;
	    S_StartSound((mobj_t *)&door->sector->soundorg, sfx_bdcls);
	    break;

	  case closeDoor:
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    door->direction = -1;
	    S_StartSound((mobj_t *)&door->sector->soundorg, sfx_dorcls);
	    break;

	  case close30ThenOpen:
	    door->topheight = sec->ceilingheight;
	    door->direction = -1;
	    S_StartSound((mobj_t *)&door->sector->soundorg, sfx_dorcls);
	    break;

	  case blazeRaise:
	  case blazeOpen:
	    door->direction = 1;
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    door->speed = VDOORSPEED * 4;
	    if (door->topheight != sec->ceilingheight)
		S_StartSound((mobj_t *)&door->sector->soundorg, sfx_bdopn);
	    break;

	  case normal:
	  case openDoor:
	    door->direction = 1;
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    if (door->topheight != sec->ceilingheight)
		S_StartSound((mobj_t *)&door->sector->soundorg, sfx_doropn);
	    break;

	  default:
	    break;
	}

    }
    return rtn;
}



/* EV_VerticalDoor : open a door manually, no tag value*/

int
EV_VerticalDoor
( line_t*	line,
  mobj_t*	thing )
{
    player_t*	player;
    int		secnum;
    sector_t*	sec;
    vldoor_t*	door;
    int		side;

    side = 0;	/* only front sides can be used*/

    /*	Check for locks*/
    player = thing->player;

    switch(line->special)
    {
      case 26: /* Blue Lock*/
      case 32:
	if ( !player )
	    return 0;

	if (!player->cards[it_bluecard] && !player->cards[it_blueskull])
	{
	    player->message = pdoors_messages[MSG_PD_BLUEK];
	    S_StartSound(NULL,sfx_oof);
	    return 0;
	}
	break;

      case 27: /* Yellow Lock*/
      case 34:
	if ( !player )
	    return 0;

	if (!player->cards[it_yellowcard] &&
	    !player->cards[it_yellowskull])
	{
	    player->message = pdoors_messages[MSG_PD_YELLOWK];
	    S_StartSound(NULL,sfx_oof);
	    return 0;
	}
	break;

      case 28: /* Red Lock*/
      case 33:
	if ( !player )
	    return 0;

	if (!player->cards[it_redcard] && !player->cards[it_redskull])
	{
	    player->message = pdoors_messages[MSG_PD_REDK];
	    S_StartSound(NULL,sfx_oof);
	    return 0;
	}
	break;
      default:
	break;
    }
#ifdef DIYBOOM
    if (!demo_compatibility && (line->sidenum[1]==-1))
    {
        S_StartSound(player->mo,sfx_oof);
        return 0;
    }
#endif

    /* if the sector has an active thinker, use it*/
    sec = sides[ line->sidenum[side^1]] .sector;
    secnum = sec-sectors;

    if (sec->BOOMCEILGDATA)
    {
	door = sec->BOOMCEILGDATA;
	switch(line->special)
	{
	  case	1: /* ONLY FOR "RAISE" DOORS, NOT "OPEN"s*/
	  case	26:
	  case	27:
	  case	28:
	  case	117:
	    if (door->direction == -1)
		door->direction = 1;	/* go back up*/
	    else
	    {
		if (!thing->player)
		    return 0;		/* JDC: bad guys never close doors*/

		door->direction = -1;	/* start going down immediately*/
	    }
	    return 1;
	}
    }

    /* for proper sound*/
    switch(line->special)
    {
      case 117:	/* BLAZING DOOR RAISE*/
      case 118:	/* BLAZING DOOR OPEN*/
	S_StartSound((mobj_t *)&sec->soundorg,sfx_bdopn);
	break;

      case 1:	/* NORMAL DOOR SOUND*/
      case 31:
	S_StartSound((mobj_t *)&sec->soundorg,sfx_doropn);
	break;

      default:	/* LOCKED DOOR SOUND*/
	S_StartSound((mobj_t *)&sec->soundorg,sfx_doropn);
	break;
    }


    /* new door thinker*/
    door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
    P_AddThinker (&door->thinker);
    sec->BOOMCEILGDATA = door;
    door->thinker.function.acp1 = (actionf_p1) T_VerticalDoor;
    door->sector = sec;
    door->direction = 1;
    door->speed = VDOORSPEED;
    door->topwait = VDOORWAIT;
    BOOMSTATEMENT(door->line = line;)

    switch(line->special)
    {
      case 1:
      case 26:
      case 27:
      case 28:
	door->type = normal;
	break;

      case 31:
      case 32:
      case 33:
      case 34:
	door->type = openDoor;
	line->special = 0;
	break;

      case 117:	/* blazing door raise*/
	door->type = blazeRaise;
	door->speed = VDOORSPEED*4;
	break;
      case 118:	/* blazing door open*/
	door->type = blazeOpen;
	line->special = 0;
	door->speed = VDOORSPEED*4;
	break;
    }

    /* find the top and bottom of the movement range*/
    door->topheight = P_FindLowestCeilingSurrounding(sec);
    door->topheight -= 4*FRACUNIT;
    return 1;
}



/* Spawn a door that closes after 30 seconds*/

void P_SpawnDoorCloseIn30 (sector_t* sec)
{
    vldoor_t*	door;

    door = Z_Malloc ( sizeof(*door), PU_LEVSPEC, 0);

    P_AddThinker (&door->thinker);

    sec->BOOMCEILGDATA = door;
    sec->special = 0;

    door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
    door->sector = sec;
    door->direction = 0;
    door->type = normal;
    door->speed = VDOORSPEED;
    door->topcountdown = 30 * 35;
    BOOMSTATEMENT(door->line = NULL;)
}


/* Spawn a door that opens after 5 minutes*/

void
P_SpawnDoorRaiseIn5Mins
( sector_t*	sec,
  int		secnum )
{
    vldoor_t*	door;

    door = Z_Malloc ( sizeof(*door), PU_LEVSPEC, 0);

    P_AddThinker (&door->thinker);

    sec->BOOMCEILGDATA = door;
    sec->special = 0;

    door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
    door->sector = sec;
    door->direction = 2;
    door->type = raiseIn5Mins;
    door->speed = VDOORSPEED;
    door->topheight = P_FindLowestCeilingSurrounding(sec);
    door->topheight -= 4*FRACUNIT;
    door->topwait = VDOORWAIT;
    door->topcountdown = 5 * 60 * 35;
    BOOMSTATEMENT(door->line = NULL;)
}



/* UNUSED*/
/* Separate into p_slidoor.c?*/

#if 0		/* ABANDONED TO THE MISTS OF TIME!!!*/

/* EV_SlidingDoor : slide a door horizontally*/
/* (animate midtexture, then set noblocking line)*/



slideframe_t slideFrames[MAXSLIDEDOORS];

void P_InitSlidingDoorFrames(void)
{
    int		i;
    int		f1;
    int		f2;
    int		f3;
    int		f4;

    /* DOOM II ONLY...*/
    if ( gamemode != commercial)
	return;

    for (i = 0;i < MAXSLIDEDOORS; i++)
    {
	if (!slideFrameNames[i].frontFrame1[0])
	    break;

	f1 = R_TextureNumForName(slideFrameNames[i].frontFrame1);
	f2 = R_TextureNumForName(slideFrameNames[i].frontFrame2);
	f3 = R_TextureNumForName(slideFrameNames[i].frontFrame3);
	f4 = R_TextureNumForName(slideFrameNames[i].frontFrame4);

	slideFrames[i].frontFrames[0] = f1;
	slideFrames[i].frontFrames[1] = f2;
	slideFrames[i].frontFrames[2] = f3;
	slideFrames[i].frontFrames[3] = f4;

	f1 = R_TextureNumForName(slideFrameNames[i].backFrame1);
	f2 = R_TextureNumForName(slideFrameNames[i].backFrame2);
	f3 = R_TextureNumForName(slideFrameNames[i].backFrame3);
	f4 = R_TextureNumForName(slideFrameNames[i].backFrame4);

	slideFrames[i].backFrames[0] = f1;
	slideFrames[i].backFrames[1] = f2;
	slideFrames[i].backFrames[2] = f3;
	slideFrames[i].backFrames[3] = f4;
    }
}



/* Return index into "slideFrames" array*/
/* for which door type to use*/

int P_FindSlidingDoorType(line_t*	line)
{
    int		i;
    int		val;

    for (i = 0;i < MAXSLIDEDOORS;i++)
    {
	val = sides[line->sidenum[0]].midtexture;
	if (val == slideFrames[i].frontFrames[0])
	    return i;
    }

    return -1;
}

void T_SlidingDoor (slidedoor_t*	door)
{
    switch(door->status)
    {
      case sd_opening:
	if (!door->timer--)
	{
	    if (++door->frame == SNUMFRAMES)
	    {
		/* IF DOOR IS DONE OPENING...*/
		sides[door->line->sidenum[0]].midtexture = 0;
		sides[door->line->sidenum[1]].midtexture = 0;
		door->line->flags &= ML_BLOCKING^0xff;

		if (door->type == sdt_openOnly)
		{
		    door->frontsector->specialdata = NULL;
		    P_RemoveThinker (&door->thinker);
		    break;
		}

		door->timer = SDOORWAIT;
		door->status = sd_waiting;
	    }
	    else
	    {
		/* IF DOOR NEEDS TO ANIMATE TO NEXT FRAME...*/
		door->timer = SWAITTICS;

		sides[door->line->sidenum[0]].midtexture =
		    slideFrames[door->whichDoorIndex].
		    frontFrames[door->frame];
		sides[door->line->sidenum[1]].midtexture =
		    slideFrames[door->whichDoorIndex].
		    backFrames[door->frame];
	    }
	}
	break;

      case sd_waiting:
	/* IF DOOR IS DONE WAITING...*/
	if (!door->timer--)
	{
	    /* CAN DOOR CLOSE?*/
	    if (door->frontsector->thinglist != NULL ||
		door->backsector->thinglist != NULL)
	    {
		door->timer = SDOORWAIT;
		break;
	    }

	    /*door->frame = SNUMFRAMES-1;*/
	    door->status = sd_closing;
	    door->timer = SWAITTICS;
	}
	break;

      case sd_closing:
	if (!door->timer--)
	{
	    if (--door->frame < 0)
	    {
		/* IF DOOR IS DONE CLOSING...*/
		door->line->flags |= ML_BLOCKING;
		door->frontsector->specialdata = NULL;
		P_RemoveThinker (&door->thinker);
		break;
	    }
	    else
	    {
		/* IF DOOR NEEDS TO ANIMATE TO NEXT FRAME...*/
		door->timer = SWAITTICS;

		sides[door->line->sidenum[0]].midtexture =
		    slideFrames[door->whichDoorIndex].
		    frontFrames[door->frame];
		sides[door->line->sidenum[1]].midtexture =
		    slideFrames[door->whichDoorIndex].
		    backFrames[door->frame];
	    }
	}
	break;
    }
}



void
EV_SlidingDoor
( const line_t*	line,
  mobj_t*	thing )
{
    sector_t*		sec;
    slidedoor_t*	door;

    /* DOOM II ONLY...*/
    if (gamemode != commercial)
	return;

    /* Make sure door isn't already being animated*/
    sec = line->frontsector;
    door = NULL;
    if (sec->specialdata)
    {
	if (!thing->player)
	    return;

	door = sec->specialdata;
	if (door->type == sdt_openAndClose)
	{
	    if (door->status == sd_waiting)
		door->status = sd_closing;
	}
	else
	    return;
    }

    /* Init sliding door vars*/
    if (!door)
    {
	door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
	P_AddThinker (&door->thinker);
	sec->specialdata = door;

	door->type = sdt_openAndClose;
	door->status = sd_opening;
	door->whichDoorIndex = P_FindSlidingDoorType(line);

	if (door->whichDoorIndex < 0)
	    I_Error("EV_SlidingDoor: Can't use texture for sliding door!");

	door->frontsector = sec;
	door->backsector = line->backsector;
	door->thinker.function = T_SlidingDoor;
	door->timer = SWAITTICS;
	door->frame = 0;
	door->line = line;
    }
}
#endif


int EV_DoDonut(const line_t*	line)
{
    sector_t*		s1;
    sector_t*		s2;
    sector_t*		s3;
    int			secnum;
    int			rtn;
    int			i;
    floormove_t*	floor;

    secnum = -1;
    rtn = 0;
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
	s1 = &sectors[secnum];

#ifdef DIYBOOM
        if (P_SectorActive(floor_special, s1))
            continue;
#else
	/* ALREADY MOVING?  IF SO, KEEP GOING...*/
	if (s1->specialdata)
	    continue;
#endif
	rtn = 1;
	s2 = getNextSector(s1->lines[0],s1);

	BOOMSTATEMENT(if (!compatibility && P_SectorActive(floor_special, s2)) continue;)

	for (i = 0;i < s2->linecount;i++)
	{
	    BOOMSTATEMENT(if (compatibility) {)
	    if ((!s2->lines[i]->flags & ML_TWOSIDED) ||
		(s2->lines[i]->backsector == s1))
		continue;
	    BOOMSTATEMENT(} else if (!s2->lines[i]->backsector || s2->lines[i]->backsector == s1) continue;)
            BOOMSTATEMENT(rtn=1;)

	    s3 = s2->lines[i]->backsector;

	    /*	Spawn rising slime*/
	    floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
	    P_AddThinker (&floor->thinker);
	    s2->BOOMFLOORDATA = floor;
	    floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
	    floor->type = donutRaise;
	    floor->crush = false;
	    floor->direction = 1;
	    floor->sector = s2;
	    floor->speed = FLOORSPEED / 2;
	    floor->texture = s3->floorpic;
	    floor->newspecial = 0;
	    floor->floordestheight = s3->floorheight;

	    /*	Spawn lowering donut-hole*/
	    floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
	    P_AddThinker (&floor->thinker);
	    s1->BOOMFLOORDATA = floor;
	    floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
	    floor->type = lowerFloor;
	    floor->crush = false;
	    floor->direction = -1;
	    floor->sector = s1;
	    floor->speed = FLOORSPEED / 2;
	    floor->floordestheight = s3->floorheight;
	    break;
	}
    }
    return rtn;
}
