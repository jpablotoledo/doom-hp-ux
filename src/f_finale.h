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


#ifndef __F_FINALE__
#define __F_FINALE__


#include "doomtype.h"
#include "d_event.h"

/* FINALE*/


/* Called by main loop.*/
boolean F_Responder (const event_t* ev);

/* Called by main loop.*/
void F_Ticker (void);

/* Called by main loop.*/
void F_Drawer (void);


void F_StartFinale (void);


#ifndef STATIC_RESOLUTION
void F_initForResolution(void);
#endif


/* Moved outside here for DeHackEd */
typedef struct
{
    char		*name;
    mobjtype_t		type;
} castinfo_t;


/* For DeHackEd: */
extern char *e1text;
extern char *e2text;
extern char *e3text;
extern char *e4text;

extern char *c1text;
extern char *c2text;
extern char *c3text;
extern char *c4text;
extern char *c5text;
extern char *c6text;

extern char *p1text;
extern char *p2text;
extern char *p3text;
extern char *p4text;
extern char *p5text;
extern char *p6text;

extern char *t1text;
extern char *t2text;
extern char *t3text;
extern char *t4text;
extern char *t5text;
extern char *t6text;

#define CCAST_NUMBER	17
#define FLUMP_NUMBER	19
extern castinfo_t castorder[];
extern char *finale_lumps[];

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
