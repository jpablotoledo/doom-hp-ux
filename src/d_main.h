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
/*	System specific interface stuff.*/

/*-----------------------------------------------------------------------------*/


#ifndef __D_MAIN__
#define __D_MAIN__

#include "d_event.h"

#ifdef __GNUG__
#pragma interface
#endif



#define MAXWADFILES             20
extern const char*		wadfiles[MAXWADFILES];
extern boolean			advancedemo;
extern gamestate_t		wipegamestate;
extern char*			pagename;

void D_AddFile (const char *file);




/* D_DoomMain()*/
/* Not a globally visible function, just included for source reference,*/
/* calls all startup code, parses command line options.*/
/* If not overrided by user input, calls N_AdvanceDemo.*/

void D_DoomMain (void);

/* Called by IO functions when input is detected.*/
void D_PostEvent (const event_t* ev);




/* BASE LEVEL*/

void D_PageTicker (void);
void D_PageDrawer (void);
void D_AdvanceDemo (void);
void D_StartTitle (void);

void D_ProcessEvents (void);
void D_DoAdvanceDemo (void);



/* For DeHackEd */

/*
 *  Comment this macro if you don't want inbuilt DeHackEd support (saves about
 *  20kB in the executable).
 */
#define INBUILT_DEHACKED

extern char retail_message[];
extern char shareware_message[];
extern char registered_message[];
extern char commercial_message[];
extern char pack_plut_message[];
extern char pack_tnt_message[];
extern char public_message[];
extern char modified_banner[];
extern char shareware_banner[];
extern char commercial_banner[];

extern int DeHackEmulationLevel;

extern void ApplyDehackPatch(const char *filename);

#endif
