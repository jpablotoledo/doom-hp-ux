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
/*	System specific interface stuff.*/

/*-----------------------------------------------------------------------------*/


#ifndef __D_TICCMD__
#define __D_TICCMD__

#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

/* The data sampled per tick (single player)*/
/* and transmitted to other peers (multiplayer).*/
/* Mainly movements/button commands per game tick,*/
/* plus a checksum for internal state consistency.*/
typedef struct
{
    char	forwardmove;	/* *2048 for move*/
    char	sidemove;	/* *2048 for move*/
    short	angleturn;	/* <<16 for angle delta*/
    short	consistancy;	/* checks for net game*/
    byte	chatchar;
    byte	buttons;
#ifdef FULL_NEW_FEATURES
#define NEW_FEATURE_JUMP	1
#define NEW_FEATURE_FLOAT	2
#define NEW_FEATURE_HOMING	4
    byte	newflags;	/* What new features are on? */
    char	vaccel;		/* vertical acceleration */
    char	misc1, misc2;	/* Might be handy later, apart from that it word-aligns */
#endif
} ticcmd_t;



#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
