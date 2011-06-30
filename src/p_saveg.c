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
/*	Archiving: SaveGame I/O.*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: p_tick.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";

#include "i_system.h"
#include "z_zone.h"
#include "p_local.h"
#include "p_saveg.h"
#include "am_map.h"

/* State.*/
#include "doomstat.h"
#include "r_state.h"

byte*		save_p;
byte*		save_high_p;
FILE*		save_file;



#ifdef DIYBOOM
#define endspecials_marker	tc_endspecials
#else
#define endspecials_marker	tc_flicker	/* first of the Boom types */
#endif

static int number_of_thinkers = 0;


void P_ThinkerToIndex(void)
{
  thinker_t *th;

  number_of_thinkers = 0;
  for (th = thinkercap.next; th != &thinkercap; th = th->next)
  {
    if (th->function.acp1 == (actionf_p1) P_MobjThinker)
      th->prev = (thinker_t*)(++number_of_thinkers);
  }
}

void P_IndexToThinker(void)
{
  thinker_t *th, *prev;

  /* Ancient bug -- in Boom too? First entry must be thinkercap, or really
     bag crashes ensue!!! */
  prev = &thinkercap;
  for (th = thinkercap.next; th != &thinkercap; prev = th, th = th->next)
  {
    th->prev = prev;
  }
}

#ifdef DIYBOOM

static void P_ArchiveThinkerBoom(mobj_t *o)
{
  if (o->target)
    o->target = (o->target->thinker.function.acp1 == (actionf_p1) P_MobjThinker) ?
      (mobj_t *) o->target->thinker.prev : NULL;

  if (o->tracer)
    o->tracer = (o->tracer->thinker.function.acp1 == (actionf_p1) P_MobjThinker) ?
      (mobj_t *) o->tracer->thinker.prev : NULL;

  if (o->lastenemy)
    o->lastenemy = (o->lastenemy->thinker.function.acp1 == (actionf_p1) P_MobjThinker) ?
      (mobj_t *) o->lastenemy->thinker.prev : NULL;

  if (o->above_thing)
    o->above_thing = (o->above_thing->thinker.function.acp1 == (actionf_p1) P_MobjThinker) ?
      (mobj_t *) o->above_thing->thinker.prev : NULL;

  if (o->below_thing)
    o->below_thing = (o->below_thing->thinker.function.acp1 == (actionf_p1) P_MobjThinker) ?
      (mobj_t *) o->below_thing->thinker.prev : NULL;
}

#endif





/* Pads save_p to a 4-byte boundary*/
/*  so that the load/save works on SGI&Gecko.*/
#define PADSAVEP()	save_p += (4 - ((int) save_p & 3)) & 3


#define ENSURE_SAVEBUFFER(need) \
	if ((save_p + (need)) > save_high_p) P_FlushSavebuffer();

/*
 *  We have to make sure the alignment is OK when reading the whole thing back in
 *  (one block!) ==> flush only whole words, copy remaining bytes to beginning.
 */

static void P_FlushSavebuffer(void)
{
    int bytes, padcount;

    bytes = ((int)(save_p - savebuffer)) & ~3;
    fwrite(savebuffer, 1, bytes, save_file);
    for (padcount=0; padcount<4; padcount++) savebuffer[padcount] = savebuffer[bytes+padcount];
    save_p = savebuffer + ((int)(save_p - savebuffer) - bytes);
}





/* P_ArchivePlayers*/

void P_ArchivePlayers (void)
{
    int		i;
    int		j;
    player_t*	dest;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!playeringame[i])
	    continue;

	ENSURE_SAVEBUFFER(3 + sizeof(player_t));
	PADSAVEP();

	dest = (player_t *)save_p;
	memcpy(save_p, &players[i], sizeof(player_t));
	save_p += sizeof(player_t);

	for (j=0 ; j<NUMPSPRITES ; j++)
	{
	    if (dest->psprites[j].state)
	    {
		dest->psprites[j].state
		    = (state_t *)(dest->psprites[j].state-states);
	    }
	}
    }
}




/* P_UnArchivePlayers*/

void P_UnArchivePlayers (void)
{
    int		i;
    int		j;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!playeringame[i])
	    continue;

	PADSAVEP();

	memcpy (&players[i],save_p, sizeof(player_t));
	save_p += sizeof(player_t);

	/* will be set when unarc thinker*/
	players[i].mo = NULL;
	players[i].message = NULL;
	players[i].attacker = NULL;

	for (j=0 ; j<NUMPSPRITES ; j++)
	{
	    if (players[i]. psprites[j].state)
	    {
		players[i]. psprites[j].state
		    = &states[ (int)players[i].psprites[j].state ];
	    }
	}
    }
}



/* P_ArchiveWorld*/

void P_ArchiveWorld (void)
{
    int			i;
    int			j;
    sector_t*		sec;
    line_t*		li;
    side_t*		si;
    short*		put;

    /* do sectors*/
    for (i=0, sec = sectors ; i<numsectors ; i++,sec++)
    {
	ENSURE_SAVEBUFFER(8*sizeof(short));	/* Boom might need additional value! */
	put = (short*)save_p;
	*put++ = sec->floorheight >> FRACBITS;
	*put++ = sec->ceilingheight >> FRACBITS;
	*put++ = sec->floorpic;
	*put++ = sec->ceilingpic;
	*put++ = sec->lightlevel;
	*put++ = sec->special;		/* needed?*/
	*put++ = sec->tag;		/* needed?*/
	BOOMSTATEMENT(*put++ = (short)((long)((sec->soundtarget) ? sec->soundtarget->thinker.prev : 0));)
	save_p = (byte*)put;
    }

    /* do lines*/
    for (i=0, li = lines ; i<numlines ; i++,li++)
    {
	ENSURE_SAVEBUFFER((3 + 2*5)*sizeof(short));
	put = (short*)save_p;
	*put++ = li->flags;
	*put++ = li->special;
	*put++ = li->tag;
	for (j=0 ; j<2 ; j++)
	{
	    if (li->sidenum[j] == -1)
		continue;

	    si = &sides[li->sidenum[j]];

	    *put++ = si->textureoffset >> FRACBITS;
	    *put++ = si->rowoffset >> FRACBITS;
	    *put++ = si->toptexture;
	    *put++ = si->bottomtexture;
	    *put++ = si->midtexture;
	}
	save_p = (byte*)put;
    }
}




/* P_UnArchiveWorld*/

void P_UnArchiveWorld (void)
{
    int			i;
    int			j;
    sector_t*		sec;
    line_t*		li;
    side_t*		si;
    short*		get;

    get = (short *)save_p;

    /* do sectors*/
    for (i=0, sec = sectors ; i<numsectors ; i++,sec++)
    {
	sec->floorheight = *get++ << FRACBITS;
	sec->ceilingheight = *get++ << FRACBITS;
	sec->floorpic = *get++;
	sec->ceilingpic = *get++;
	sec->lightlevel = *get++;
	sec->special = *get++;		/* needed?*/
	sec->tag = *get++;		/* needed?*/
	sec->BOOMCEILGDATA = 0;
#ifdef DIYBOOM
	sec->floordata = 0;
	sec->lightingdata = 0;
	sec->soundtarget = (mobj_t*)((long)(*get++));
#else
	sec->soundtarget = 0;
#endif
    }

    /* do lines*/
    for (i=0, li = lines ; i<numlines ; i++,li++)
    {
	li->flags = *get++;
	li->special = *get++;
	li->tag = *get++;
	for (j=0 ; j<2 ; j++)
	{
	    if (li->sidenum[j] == -1)
		continue;
	    si = &sides[li->sidenum[j]];
	    si->textureoffset = *get++ << FRACBITS;
	    si->rowoffset = *get++ << FRACBITS;
	    si->toptexture = *get++;
	    si->bottomtexture = *get++;
	    si->midtexture = *get++;
	}
    }
    save_p = (byte *)get;
}






/* Thinkers*/


/* P_ArchiveThinkers*/

void P_ArchiveThinkers (void)
{
    thinker_t*		th;
    mobj_t*		mobj;

#ifdef DIYBOOM
    /*ENSURE_SAVEBUFFER(4 + sizeof(brain));
    memcpy(save_p, &brain, sizeof(brain));
    save_p += sizeof(brain);*/
#endif

    /* save off the current thinkers*/
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
	if (th->function.acp1 == (actionf_p1)P_MobjThinker)
	{
	    ENSURE_SAVEBUFFER(4 + sizeof(mobj_t));
	    *save_p++ = tc_mobj;
	    PADSAVEP();
	    mobj = (mobj_t *)save_p;
	    memcpy (mobj, th, sizeof(*mobj));
	    save_p += sizeof(mobj_t);
	    mobj->state = (state_t *)(mobj->state - states);

#ifdef DIYBOOM
	    P_ArchiveThinkerBoom(mobj);
#endif

	    if (mobj->player)
		mobj->player = (player_t *)((mobj->player-players) + 1);
	    continue;
	}

	/* I_Error ("P_ArchiveThinkers: Unknown thinker function");*/
    }

    /* add a terminating marker*/
    ENSURE_SAVEBUFFER(1);
    *save_p++ = tc_end;
}




/* P_UnArchiveThinkers*/

void P_UnArchiveThinkers (void)
{
#ifdef DIYBOOM
    mobj_t**		mobj_p;
    size_t		numThinkers, size;
    byte*		sp;
    sector_t*		sec;
#endif
    byte		tclass;
    thinker_t*		th;

#ifdef DIYBOOM
    /*memcpy(&brain, save_p, sizeof(brain));
    save_p += sizeof(brain);*/
#endif

    /* remove all the current thinkers*/
    th = thinkercap.next;
    while (th != &thinkercap)
    {
	thinker_t *next = th->next;

	if (th->function.acp1 == (actionf_p1)P_MobjThinker)
	    P_RemoveMobj ((mobj_t *)th);
	else
	    Z_Free (th);

	th = next;
    }
    P_InitThinkers ();

#ifdef DIYBOOM
    sp = save_p;
    for (numThinkers=1; (tclass = *save_p++) == tc_mobj; numThinkers++)
    {
        PADSAVEP();
        save_p += sizeof(mobj_t);
    }
    if (tclass != tc_end)
        I_Error("P_UnArchiveThinkers: unknown tclass %d in savegame", tclass);

    if ((mobj_p = (mobj_t**)Z_MallocNoAbort(numThinkers * sizeof(mobj_t*), PU_STATIC, NULL)) != NULL)
        mobj_p[0] = NULL;	/* Must initialize first element !!! */

    save_p = sp;

    for (size=1; *save_p++ == tc_mobj; size++)
    {
        mobj_t*	mobj = (mobj_t*)Z_Malloc(sizeof(mobj_t), PU_LEVEL, NULL);

        if ((mobj_p != NULL) && (size < numThinkers)) mobj_p[size] = mobj;

        PADSAVEP();
        memcpy(mobj, save_p, sizeof(mobj_t));
        save_p += sizeof(mobj_t);
        mobj->state = states + (int)(mobj->state);

        if (mobj->player)
        {
            mobj->player = &players[(int)mobj->player-1];
            mobj->player->mo = mobj;
        }
        P_SetThingPosition(mobj);
        mobj->info = &mobjinfo[mobj->type];

        mobj->thinker.function.acp1 = (actionf_p1) P_MobjThinker;
        P_AddThinker(&mobj->thinker);
    }
    if (mobj_p != NULL)
    {
        int i;

        for (th = thinkercap.next; th != &thinkercap; th = th->next)
        {
            mobj_t*	mobj = (mobj_t*)th;

            mobj->target = mobj_p[(size_t)mobj->target];
            mobj->tracer = mobj_p[(size_t)mobj->tracer];
            mobj->lastenemy = mobj_p[(size_t)mobj->lastenemy];
            mobj->above_thing = mobj_p[(size_t)mobj->above_thing];
            mobj->below_thing = mobj_p[(size_t)mobj->below_thing];
        }
        for (i = 0, sec = sectors ; i < numsectors ; i++, sec++)
            sec->soundtarget = mobj_p[(size_t) sec->soundtarget];

        Z_Free(mobj_p);
    }
    else	/* just to make sure it doesn't bomb out... */
    {
        int i;

        for (th = thinkercap.next; th != &thinkercap; th = th->next)
        {
            mobj_t*	mobj = (mobj_t*)th;

            mobj->target = NULL;
            mobj->tracer = NULL;
            mobj->lastenemy = NULL;
            mobj->above_thing = NULL;
            mobj->below_thing = NULL;
        }
        for (i=0, sec = sectors; i < numsectors; i++, sec++)
            sec->soundtarget = NULL;
    }

    /*if (gamemode == commercial)
        P_SpawnBrainTargets();*/
#else

    /* read in saved thinkers*/
    while (1)
    {
        mobj_t*		mobj;

	tclass = *save_p++;
	switch (tclass)
	{
	  case tc_end:
	    return; 	/* end of list*/

	  case tc_mobj:
	    PADSAVEP();
	    mobj = Z_Malloc (sizeof(*mobj), PU_LEVEL, NULL);
	    memcpy (mobj, save_p, sizeof(*mobj));
	    save_p += sizeof(*mobj);
	    mobj->state = &states[(int)mobj->state];
	    mobj->target = NULL;
	    if (mobj->player)
	    {
		mobj->player = &players[(int)mobj->player-1];
		mobj->player->mo = mobj;
	    }
	    P_SetThingPosition (mobj);
	    mobj->info = &mobjinfo[mobj->type];
	    mobj->floorz = mobj->subsector->sector->floorheight;
	    mobj->ceilingz = mobj->subsector->sector->ceilingheight;
	    mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;
	    P_AddThinker (&mobj->thinker);
	    break;

	  default:
	    I_Error ("P_UnArchiveThinkers: unknown tclass %i in savegame", tclass);
	}
    }
#endif
}



/* P_ArchiveSpecials*/

/* Things to handle:*/

/* T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list*/
/* T_VerticalDoor, (vldoor_t: sector_t * swizzle),*/
/* T_MoveFloor, (floormove_t: sector_t * swizzle),*/
/* T_LightFlash, (lightflash_t: sector_t * swizzle),*/
/* T_StrobeFlash, (strobe_t: sector_t *),*/
/* T_Glow, (glow_t: sector_t *),*/
/* T_PlatRaise, (plat_t: sector_t *), - active list*/

void P_ArchiveSpecials (void)
{
    thinker_t*		th;
    ceiling_t*		ceiling;
    plat_t*		plat;
    int			i;

    /* save off the current thinkers*/
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
	if (th->function.acv == (actionf_v)NULL)
	{
#ifdef DIYBOOM
            for (i=0; i<maxplatforms; i++)
                if (activeplats[i] == (plat_t*)th) break;

            if (i < maxplatforms)
            {
	        ENSURE_SAVEBUFFER(4 + sizeof(plat_t));
	        *save_p++ = tc_plat;
	        PADSAVEP();
	        plat = (plat_t *)save_p;
	        memcpy (plat, th, sizeof(*plat));
	        save_p += sizeof(*plat);
	        plat->sector = (sector_t *)(plat->sector - sectors);
            }
#endif
	    for (i=0; i<maxceilings; i++)
		if (activeceilings[i] == (ceiling_t *)th) break;

	    if (i < maxceilings)
	    {
		ENSURE_SAVEBUFFER(4 + sizeof(ceiling_t));
		*save_p++ = tc_ceiling;
		PADSAVEP();
		ceiling = (ceiling_t *)save_p;
		memcpy (ceiling, th, sizeof(*ceiling));
		save_p += sizeof(*ceiling);
		ceiling->sector = (sector_t *)(ceiling->sector - sectors);
	    }
	    continue;
	}

	if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
	{
	    ENSURE_SAVEBUFFER(4 + sizeof(ceiling_t));
	    *save_p++ = tc_ceiling;
	    PADSAVEP();
	    ceiling = (ceiling_t *)save_p;
	    memcpy (ceiling, th, sizeof(*ceiling));
	    save_p += sizeof(*ceiling);
	    ceiling->sector = (sector_t *)(ceiling->sector - sectors);
	    continue;
	}

	if (th->function.acp1 == (actionf_p1)T_VerticalDoor)
	{
            vldoor_t*		door;
	    ENSURE_SAVEBUFFER(4 + sizeof(vldoor_t));
	    *save_p++ = tc_door;
	    PADSAVEP();
	    door = (vldoor_t *)save_p;
	    memcpy (door, th, sizeof(*door));
	    save_p += sizeof(*door);
	    door->sector = (sector_t *)(door->sector - sectors);
	    continue;
	}

	if (th->function.acp1 == (actionf_p1)T_MoveFloor)
	{
            floormove_t*	floor;
	    ENSURE_SAVEBUFFER(4 + sizeof(floormove_t));
	    *save_p++ = tc_floor;
	    PADSAVEP();
	    floor = (floormove_t *)save_p;
	    memcpy (floor, th, sizeof(*floor));
	    save_p += sizeof(*floor);
	    floor->sector = (sector_t *)(floor->sector - sectors);
	    continue;
	}

	if (th->function.acp1 == (actionf_p1)T_PlatRaise)
	{
	    ENSURE_SAVEBUFFER(4 + sizeof(plat_t));
	    *save_p++ = tc_plat;
	    PADSAVEP();
	    plat = (plat_t *)save_p;
	    memcpy (plat, th, sizeof(*plat));
	    save_p += sizeof(*plat);
	    plat->sector = (sector_t *)(plat->sector - sectors);
	    continue;
	}

	if (th->function.acp1 == (actionf_p1)T_LightFlash)
	{
            lightflash_t*	flash;
	    ENSURE_SAVEBUFFER(4 + sizeof(lightflash_t));
	    *save_p++ = tc_flash;
	    PADSAVEP();
	    flash = (lightflash_t *)save_p;
	    memcpy (flash, th, sizeof(*flash));
	    save_p += sizeof(*flash);
	    flash->sector = (sector_t *)(flash->sector - sectors);
	    continue;
	}

	if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
	{
            strobe_t*		strobe;
	    ENSURE_SAVEBUFFER(4 + sizeof(strobe_t));
	    *save_p++ = tc_strobe;
	    PADSAVEP();
	    strobe = (strobe_t *)save_p;
	    memcpy (strobe, th, sizeof(*strobe));
	    save_p += sizeof(*strobe);
	    strobe->sector = (sector_t *)(strobe->sector - sectors);
	    continue;
	}

	if (th->function.acp1 == (actionf_p1)T_Glow)
	{
            glow_t*		glow;
	    ENSURE_SAVEBUFFER(4 + sizeof(glow_t));
	    *save_p++ = tc_glow;
	    PADSAVEP();
	    glow = (glow_t *)save_p;
	    memcpy (glow, th, sizeof(*glow));
	    save_p += sizeof(*glow);
	    glow->sector = (sector_t *)(glow->sector - sectors);
	    continue;
	}

#ifdef DIYBOOM
        if (th->function.acp1 == (actionf_p1) T_FireFlicker)
        {
            fireflicker_t*	flick;
            ENSURE_SAVEBUFFER(4 + sizeof(fireflicker_t));
            *save_p++ = tc_flicker;
            PADSAVEP();
            flick = (fireflicker_t*)save_p;
            memcpy(flick, th, sizeof(*flick));
            save_p += sizeof(*flick);
            flick->sector = (sector_t*)(flick->sector - sectors);
            continue;
        }

        if (th->function.acp1 == (actionf_p1) T_MoveElevator)
        {
            elevator_t*	elevator;
            ENSURE_SAVEBUFFER(4 + sizeof(elevator_t));
            *save_p++ = tc_elevator;
            PADSAVEP();
            elevator = (elevator_t*)save_p;
            memcpy(elevator, th, sizeof(*elevator));
            save_p += sizeof(*elevator);
            elevator->sector = (sector_t*)(elevator->sector - sectors);
        }

        if (th->function.acp1 == (actionf_p1) T_Scroll)
        {
            ENSURE_SAVEBUFFER(4 + sizeof(scroll_t));
            *save_p++ = tc_scroll;
            PADSAVEP();
            memcpy(save_p, th, sizeof(scroll_t));
            save_p += sizeof(scroll_t);
            continue;
        }

        if (th->function.acp1 == (actionf_p1) T_Friction)
        {
            ENSURE_SAVEBUFFER(4 + sizeof(friction_t));
            *save_p++ = tc_friction;
            PADSAVEP();
            memcpy(save_p, th, sizeof(friction_t));
            save_p += sizeof(friction_t);
            continue;
        }

        if (th->function.acp1 == (actionf_p1) T_Pusher)
        {
            ENSURE_SAVEBUFFER(4 + sizeof(pusher_t));
            *save_p++ = tc_pusher;
            PADSAVEP();
            memcpy(save_p, th, sizeof(pusher_t));
            save_p += sizeof(pusher_t);
            continue;
        }
#endif

    }

    /* add a terminating marker*/
    ENSURE_SAVEBUFFER(1);
    *save_p++ = endspecials_marker;
}



/* P_UnArchiveSpecials*/

void P_UnArchiveSpecials (void)
{
    byte		tclass;
    ceiling_t*		ceiling;
    vldoor_t*		door;
    floormove_t*	floor;
    plat_t*		plat;
    lightflash_t*	flash;
    strobe_t*		strobe;
    glow_t*		glow;
#ifdef DIYBOOM
    fireflicker_t*	flick;
    elevator_t*		elevator;
    scroll_t*		scroll;
    friction_t*		friction;
    pusher_t*		pusher;
#endif

    /* read in saved thinkers*/
    while (1)
    {
	tclass = *save_p++;
	switch (tclass)
	{
	  case endspecials_marker:
	    return;	/* end of list*/

	  case tc_ceiling:
	    PADSAVEP();
	    ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVEL, NULL);
	    memcpy (ceiling, save_p, sizeof(*ceiling));
	    save_p += sizeof(*ceiling);
	    ceiling->sector = &sectors[(int)ceiling->sector];
	    ceiling->sector->BOOMCEILGDATA = ceiling;

	    if (ceiling->thinker.function.acp1)
		ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;

	    P_AddThinker (&ceiling->thinker);
	    P_AddActiveCeiling(ceiling);
	    break;

	  case tc_door:
	    PADSAVEP();
	    door = Z_Malloc (sizeof(*door), PU_LEVEL, NULL);
	    memcpy (door, save_p, sizeof(*door));
	    save_p += sizeof(*door);
	    door->sector = &sectors[(int)door->sector];
	    door->sector->BOOMCEILGDATA = door;
	    door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
	    P_AddThinker (&door->thinker);
	    break;

	  case tc_floor:
	    PADSAVEP();
	    floor = Z_Malloc (sizeof(*floor), PU_LEVEL, NULL);
	    memcpy (floor, save_p, sizeof(*floor));
	    save_p += sizeof(*floor);
	    floor->sector = &sectors[(int)floor->sector];
	    floor->sector->BOOMFLOORDATA = floor;
	    floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
	    P_AddThinker (&floor->thinker);
	    break;

	  case tc_plat:
	    PADSAVEP();
	    plat = Z_Malloc (sizeof(*plat), PU_LEVEL, NULL);
	    memcpy (plat, save_p, sizeof(*plat));
	    save_p += sizeof(*plat);
	    plat->sector = &sectors[(int)plat->sector];
	    plat->sector->BOOMFLOORDATA = plat;

	    if (plat->thinker.function.acp1)
		plat->thinker.function.acp1 = (actionf_p1)T_PlatRaise;

	    P_AddThinker (&plat->thinker);
	    P_AddActivePlat(plat);
	    break;

	  case tc_flash:
	    PADSAVEP();
	    flash = Z_Malloc (sizeof(*flash), PU_LEVEL, NULL);
	    memcpy (flash, save_p, sizeof(*flash));
	    save_p += sizeof(*flash);
	    flash->sector = &sectors[(int)flash->sector];
	    flash->thinker.function.acp1 = (actionf_p1)T_LightFlash;
	    P_AddThinker (&flash->thinker);
	    break;

	  case tc_strobe:
	    PADSAVEP();
	    strobe = Z_Malloc (sizeof(*strobe), PU_LEVEL, NULL);
	    memcpy (strobe, save_p, sizeof(*strobe));
	    save_p += sizeof(*strobe);
	    strobe->sector = &sectors[(int)strobe->sector];
	    strobe->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
	    P_AddThinker (&strobe->thinker);
	    break;

	  case tc_glow:
	    PADSAVEP();
	    glow = Z_Malloc (sizeof(*glow), PU_LEVEL, NULL);
	    memcpy (glow, save_p, sizeof(*glow));
	    save_p += sizeof(*glow);
	    glow->sector = &sectors[(int)glow->sector];
	    glow->thinker.function.acp1 = (actionf_p1)T_Glow;
	    P_AddThinker (&glow->thinker);
	    break;
#ifdef DIYBOOM
          case tc_flicker:
            PADSAVEP();
            flick = Z_Malloc (sizeof(*flick), PU_LEVEL, NULL);
            memcpy (flick, save_p, sizeof(*flick));
            save_p += sizeof(*flick);
            flick->sector = &sectors[(int)flick->sector];
            flick->thinker.function.acp1 = (actionf_p1) T_FireFlicker;
            P_AddThinker (&flick->thinker);
            break;
          case tc_elevator:
            PADSAVEP();
            elevator = Z_Malloc (sizeof(*elevator), PU_LEVEL, NULL);
            memcpy (elevator, save_p, sizeof(*elevator));
            save_p += sizeof(*elevator);
            elevator->sector = &sectors[(int)elevator->sector];
            elevator->sector->floordata = elevator;
            elevator->sector->ceilingdata = elevator;
            elevator->thinker.function.acp1 = (actionf_p1) T_MoveElevator;
            P_AddThinker (&elevator->thinker);
            break;
          case tc_scroll:
            PADSAVEP();
            scroll = Z_Malloc (sizeof(*scroll), PU_LEVEL, NULL);
            memcpy (scroll, save_p, sizeof(*scroll));
            save_p += sizeof(*scroll);
            scroll->thinker.function.acp1 = (actionf_p1) T_Scroll;
            P_AddThinker(&scroll->thinker);
            break;
          case tc_friction:
            PADSAVEP();
            friction = Z_Malloc(sizeof(*friction), PU_LEVEL, NULL);
            memcpy (friction, save_p, sizeof(*friction));
            save_p += sizeof(*friction);
            friction->thinker.function.acp1 = (actionf_p1) T_Friction;
            P_AddThinker(&friction->thinker);
            break;
          case tc_pusher:
            PADSAVEP();
            pusher = Z_Malloc(sizeof(*pusher), PU_LEVEL, NULL);
            memcpy (pusher, save_p, sizeof(*pusher));
            save_p += sizeof(*pusher);
            pusher->thinker.function.acp1 = (actionf_p1) T_Pusher;
            pusher->source = P_GetPushThing(pusher->affectee);
            P_AddThinker(&pusher->thinker);
            break;
#endif
	  default:
	    I_Error ("P_UnarchiveSpecials: Unknown tclass %i in savegame", tclass);
	}

    }

}



#ifdef DIYBOOM
void P_ArchiveMap(void)
{
    ENSURE_SAVEBUFFER(/*sizeof(followplayer) + sizeof(markpointnum) + markpointnum * sizeof(*markpoints) + sizeof(automap_grid)*/ + sizeof(automapactive) + sizeof(viewactive));
    memcpy(save_p, &automapactive, sizeof(automapactive));
    save_p += sizeof(automapactive);
    memcpy(save_p, &viewactive, sizeof(viewactive));
    save_p += sizeof(viewactive);
    /*memcpy(save_p, &automap_grid, sizeof(automap_grid));
    save_p += sizeof(automap_grid);
    memcpy(save_p, &followplayer, sizeof(followplayer));
    save_p += sizeof(followplayer);
    memcpy(save_p, &markpointnum, sizeof(markpointnum));
    save_p += sizeof(markpointnum);
    if (markpointnum != 0)
    {
        memcpy(save_p, markpoints, markpointnum * sizeof(*markpoints));
        save_p += markpointnum * sizeof(*markpoints);
    }*/
}


void P_UnArchiveMap(void)
{
    memcpy(&automapactive, save_p, sizeof(automapactive));
    save_p += sizeof(automapactive);
    mixmap = false;
    memcpy(&viewactive, save_p, sizeof(viewactive));
    save_p += sizeof(viewactive);
    /*memcpy(&automap_grid, save_p, sizeof(automap_grid));
    save_p += sizeof(automap_grid);
    memcpy(&followplayer, save_p, sizeof(followplayer));
    save_p += sizeof(followplayer);*/

    if (automapactive)
        AM_Start();

    /*memcpy(&markpointnum, save_p, sizeof(markpointnum));
    save_p += sizeof(markpointnum);
    if (markpointnum != 0)
    {
        save_p += markpointnum * sizeof(*markpoints);
    }*/
}
#endif
