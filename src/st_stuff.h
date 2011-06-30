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
/*	Status bar code.*/
/*	Does the face/direction indicator animatin.*/
/*	Does palette indicators as well (red pain/berserk, bright pickup)*/

/*-----------------------------------------------------------------------------*/

#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "doomtype.h"
#include "d_event.h"

/* Size of statusbar.*/
/* Now sensitive for scaling.*/
#define ST_HEIGHT	32*SCREEN_MUL
#define ST_WIDTH	320	/*SCREENWIDTH*/
#ifdef STATIC_RESOLUTION
#define ST_Y		(SCREENHEIGHT - ST_HEIGHT)
#else
extern int ST_Y;
void ST_initForResolution(void);
#endif



/* STATUS BAR*/


/* Called by main loop.*/
boolean ST_Responder (const event_t* ev);

/* Called by main loop.*/
void ST_Ticker (void);

/* Called by main loop.*/
void ST_Drawer (boolean fullscreen, boolean refresh);

/* Called when the console player is spawned on each level.*/
void ST_Start (void);

/* Called by startup code.*/
void ST_Init (void);



/* States for status bar code.*/
typedef enum
{
    AutomapState,
    FirstPersonState

} st_stateenum_t;


/* States for the chat code.*/
typedef enum
{
    StartChatState,
    WaitDestState,
    GetChatState

} st_chatstateenum_t;


/* For c.m_cheat */
/* Dimensions given in characters.*/
#define ST_MSGWIDTH			52

/* For DeHackEd */
#define STMSG_NUMBER	22
extern char *ststuff_messages[];

extern int IDFAarmor;
extern int IDFAarmorclass;
extern int IDKFAarmor;
extern int IDKFAarmorclass;
extern int GodHealth;
extern int EnableTransparency;

extern unsigned char cheat_mus_seq[];
extern unsigned char cheat_choppers_seq[];
extern unsigned char cheat_god_seq[];
extern unsigned char cheat_ammo_seq[];
extern unsigned char cheat_ammonokey_seq[];
extern unsigned char cheat_noclip_seq[];
extern unsigned char cheat_commercial_noclip_seq[];
extern unsigned char cheat_powerup_seq[7][10];
extern unsigned char cheat_clev_seq[];
extern unsigned char cheat_mypos_seq[];

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
