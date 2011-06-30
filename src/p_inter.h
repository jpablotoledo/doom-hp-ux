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


/*-----------------------------------------------------------------------------*/


#ifndef __P_INTER__
#define __P_INTER__


#ifdef __GNUG__
#pragma interface
#endif


boolean	P_GivePower(player_t*, int);


/* For DeHackEd support */
#define PINTER_MESSAGES		37

#define MSG_PD_BLUEO		0
#define MSG_PD_REDO		1
#define MSG_PD_YELLOWO		2
#define MSG_PD_BLUEK		3
#define MSG_PD_REDK		4
#define MSG_PD_YELLOWK		5
#ifdef DIYBOOM
#define PDOORS_MESSAGES		15
#define MSG_PD_ANY		6
#define MSG_PD_REDC		7
#define MSG_PD_BLUEC		8
#define MSG_PD_YELLOWC		9
#define MSG_PD_REDS		10
#define MSG_PD_BLUES		11
#define MSG_PD_YELLOWS		12
#define MSG_PD_ALL3		13
#define MSG_PD_ALL6		14
#else
#define PDOORS_MESSAGES		6
#endif

extern char *pinter_messages[PINTER_MESSAGES];
extern char *pdoors_messages[PDOORS_MESSAGES];

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
