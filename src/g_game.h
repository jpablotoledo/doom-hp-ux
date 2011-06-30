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
/*   Duh.*/

/*-----------------------------------------------------------------------------*/


#ifndef __G_GAME__
#define __G_GAME__

#include "doomdef.h"
#include "d_event.h"
#include "d_ticcmd.h"




/* GAME*/

void G_DeathMatchSpawnPlayer (int playernum);

void G_InitNew (skill_t skill, int episode, int map);
void G_PlayerReborn (int player);
void G_BuildTiccmd(ticcmd_t* cmd);

/* Can be called by the startup code or M_Responder.*/
/* A normal game starts at map 1,*/
/* but a warp test can start elsewhere*/
void G_DeferedInitNew (skill_t skill, int episode, int map);

void G_DeferedPlayDemo (const char* demo);

/* Can be called by the startup code or M_Responder,*/
/* calls P_SetupLevel or W_EnterWorld.*/
void G_LoadGame (const char* name);

/*void G_DoLoadGame (void);*/

/* Called by M_Responder.*/
void G_SaveGame (int slot, const char* description);

/* Only called by startup code.*/
void G_RecordDemo (const char* name);

void G_BeginRecording (void);

void G_PlayDemo (const char* name);
void G_TimeDemo (const char* name);
boolean G_CheckDemoStatus (void);

void G_ExitLevel (void);
void G_SecretExitLevel (void);

void G_SetFastParms (int);

void G_WorldDone (void);

void G_Ticker (void);
boolean G_Responder (const event_t*	ev);

void G_ScreenShot (void);


extern boolean	viewactive;
extern boolean	sendpause;
extern boolean	demorecording;
extern int	oldsavegames;
extern int	key_right;
extern int	key_left;
extern int	key_up;
extern int	key_down;
extern int	key_straferight;
extern int	key_strafeleft;
extern int	key_fire;
extern int	key_use;
extern int	key_strafe;
extern int	key_speed;
extern int	key_jump;
extern int	mousebfire;
extern int	mousebstrafe;
extern int	mousebforward;
extern int	joybfire;
extern int	joybstrafe;
extern int	joybuse;
extern int	joybspeed;
extern void*	statcopy;

extern fixed_t	forwardmove[2];
extern fixed_t	sidemove[2];
extern fixed_t	angleturn[3];


/* DeHackEd */
#define GGAME_NUMBER	4
extern char *ggame_lumps[];

extern int InitHealth;
extern int InitAmmo;

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
