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
/*  AutoMap module.*/

/*-----------------------------------------------------------------------------*/

#ifndef __AMMAP_H__
#define __AMMAP_H__

/* Used by ST StatusBar stuff.*/
#define AM_MSGHEADER (('a'<<24)+('m'<<16))
#define AM_MSGENTERED (AM_MSGHEADER | ('e'<<8))
#define AM_MSGEXITED (AM_MSGHEADER | ('x'<<8))


/* Called by main loop.*/
boolean AM_Responder (const event_t* ev);

/* Called by main loop.*/
void AM_Ticker (void);

/* Called by main loop,*/
/* called instead of view drawer if automap active.*/
void AM_Drawer (void);

/* Called to force the automap to quit*/
/* if the level is completed while it is up.*/
void AM_Stop (void);

void AM_Start (void);


extern boolean	automapactive, mixmap;


#ifndef STATIC_RESOLUTION
void AM_initForResolution(void);
#endif

/* check whether the key is known to automap */
boolean AM_IsMapKey(int key);

/* For DeHackEd */
extern unsigned char cheat_amap_seq[];

/* Map keys, for m_misc */
extern int mapkey_pandown;
extern int mapkey_panup;
extern int mapkey_panright;
extern int mapkey_panleft;
extern int mapkey_zoomin;
extern int mapkey_zoomout;
extern int mapkey_activate;
extern int mapkey_altmap;
extern int mapkey_gobig;
extern int mapkey_follow;
extern int mapkey_grid;
extern int mapkey_mark;
extern int mapkey_clearmark;

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
