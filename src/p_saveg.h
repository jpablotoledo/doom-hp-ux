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
/*	Savegame I/O, archiving, persistence.*/

/*-----------------------------------------------------------------------------*/


#ifndef __P_SAVEG__
#define __P_SAVEG__


#ifdef __GNUG__
#pragma interface
#endif


/* enum types, formerly in c-file */
typedef enum
{
    tc_end,
    tc_mobj

} thinkerclass_t;


typedef enum
{
    tc_ceiling,
    tc_door,
    tc_floor,
    tc_plat,
    tc_flash,
    tc_strobe,
    tc_glow,
    /* additional Boom types, make visible on vanilla Doom */
    tc_flicker,
    tc_elevator,
    tc_scroll,
    tc_friction,
    tc_pusher,
    tc_endspecials

} specials_e;



/* Persistent storage/archiving.*/
/* These are the load / save game routines.*/
void P_ArchivePlayers (void);
void P_UnArchivePlayers (void);
void P_ArchiveWorld (void);
void P_UnArchiveWorld (void);
void P_ArchiveThinkers (void);
void P_UnArchiveThinkers (void);
void P_ArchiveSpecials (void);
void P_UnArchiveSpecials (void);

void P_ThinkerToIndex(void);
void P_IndexToThinker(void);
#ifdef DIYBOOM
void P_ArchiveMap(void);
void P_UnArchiveMap(void);
#endif

extern byte*		save_p;
extern byte*		save_high_p;
extern FILE*		save_file;

/* Actually defined in g_game */
extern byte*		savebuffer;

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
