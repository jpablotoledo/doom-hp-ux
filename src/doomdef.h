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
/*  Internally used data structures for virtually everything,*/
/*   key definitions, lots of other stuff.*/

/*-----------------------------------------------------------------------------*/

#ifndef __DOOMDEF__
#define __DOOMDEF__

#include <stdio.h>
#include <string.h>



/*
 *  Define this to be able to use jumping and floating over the network at the
 *  cost of utter incompatibility with old versions (incl. savegames & demos).
 *  This will increase the version number to 111.
 */
/*#define FULL_NEW_FEATURES*/



/* Global parameters/defines.*/

/* DOOM version*/
#define DIY_VERSION_DOOM	110
#define DIY_VERSION_DOOMF	111
#define DIY_VERSION_BOOM	112
#define DIY_VERSION_BOOMF	113

#ifdef FULL_NEW_FEATURES
# ifdef DIYBOOM
enum { VERSION =  DIY_VERSION_BOOMF };
# else
enum { VERSION =  DIY_VERSION_DOOMF };
# endif
#else
# ifdef DIYBOOM
enum { VERSION =  DIY_VERSION_BOOM };
# else
enum { VERSION =  DIY_VERSION_DOOM };
# endif
#endif



/* Game mode handling - identify IWAD version*/
/*  to handle IWAD dependend animations etc.*/
typedef enum
{
  shareware,	/* DOOM 1 shareware, E1, M9*/
  registered,	/* DOOM 1 registered, E3, M27*/
  commercial,	/* DOOM 2 retail, E1 M34*/
  /* DOOM 2 german edition not handled*/
  retail,	/* DOOM 1 retail, E4, M36*/
  indetermined	/* Well, no IWAD found.*/

} GameMode_t;


/* Mission packs - might be useful for TC stuff?*/
typedef enum
{
  doom,		/* DOOM 1*/
  doom2,	/* DOOM 2*/
  pack_tnt,	/* TNT mission pack*/
  pack_plut,	/* Plutonia pack*/
  none

} GameMission_t;


/* Identify language to use, software localization.*/
typedef enum
{
  english,
  french,
  german,
  unknown

} Language_t;


/* If rangecheck is undefined,*/
/* most parameter validation debugging code will not be compiled*/
#define RANGECHECK

/* New makro: if defined a violation of the range results in an abort of the */
/* program. Otherwise the object is simply ignored. */
/*#define RANGECHECK_ABORTS*/


/* Do or do not use external soundserver.*/
/* The sndserver binary to be run separately*/
/*  has been introduced by Dave Taylor.*/
/* The integrated sound support is experimental,*/
/*  and unfinished. Default is synchronous.*/
/* Experimental asynchronous timer based is*/
/*  handled by SNDINTR. */
/* #define SNDSERV  1*/
/*#define SNDINTR  1*/
#if (defined(__sun) || defined(LINUX))
#define SNDINTR		1
#endif


/* This one switches between MIT SHM (no proper mouse)*/
/* and XFree86 DGA (mickey sampling). The original*/
/* linuxdoom used SHM, which is default.*/
/*#define X11_DGA		1*/



/* For resize of screen, at start of game.*/
/* It will not work dynamically, see visplanes.*/

#define	BASE_WIDTH		320

/* It is educational but futile to change this*/
/*  scaling e.g. to 2. Drawing of status bar,*/
/*  menues etc. is tied to the scale implied*/
/*  by the graphics.*/
#define	SCREEN_MUL		1
/*#define	INV_ASPECT_RATIO	0.625  //0.75, ideally*/

/* Defines suck. C sucks.*/
/* C++ might sucks for OOP, but it sure is a better C.*/
/* So there.*/


/* Moved the MAXSCREENWIDHT/HEIGHT macros from r_draw/c to here to use them */
/* consistently throughout the program in case of dynamic resolution. */
#ifdef STATIC_RESOLUTION
#define SCREENWIDTH  320
/*SCREEN_MUL*BASE_WIDTH //320*/
#define SCREENHEIGHT 200
/*(int)(SCREEN_MUL*BASE_WIDTH*INV_ASPECT_RATIO) //200*/
/*#define SCREENWIDTH	448
#define SCREENHEIGHT	336*/
#define INV_ASPECT_RATIO	((float)SCREENHEIGHT / SCREENWIDTH)
#define MAXSCREENWIDTH	SCREENWIDTH
#define MAXSCREENHEIGHT	SCREENHEIGHT
#else
extern int SCREENWIDTH;
extern int SCREENHEIGHT;
extern float INV_ASPECT_RATIO;
#ifndef MAXSCREENWIDTH
#define MAXSCREENWIDTH	640
#endif
#ifndef MAXSCREENHEIGHT
#define MAXSCREENHEIGHT	480
#endif
#endif

/* Log2 of the number of bits per pixel, i.e. 3 for 8bpp, 4 for 16bpp, 5 for 32bpp. */
#ifndef LD_PIXEL_DEPTH
#define LD_PIXEL_DEPTH	3
#endif





/* The maximum number of players, multiplayer/networking.*/
#define MAXPLAYERS		4

/* State updates, number of tics / second.*/
#define TICRATE		35

/* The current state of the game: whether we are*/
/* playing, gazing at the intermission screen,*/
/* the game final animation, or a demo. */
typedef enum
{
    GS_LEVEL,
    GS_INTERMISSION,
    GS_FINALE,
    GS_DEMOSCREEN
} gamestate_t;


/* Difficulty/skill settings/filters.*/


/* Skill flags.*/
#define	MTF_EASY		1
#define	MTF_NORMAL		2
#define	MTF_HARD		4

/* Deaf monsters/do not react to sound.*/
#define	MTF_AMBUSH		8

typedef enum
{
#ifdef DIYBOOM
    sk_none=-1,
#endif
    sk_baby=0,
    sk_easy,
    sk_medium,
    sk_hard,
    sk_nightmare
} skill_t;





/* Key cards.*/

typedef enum
{
    it_bluecard,
    it_yellowcard,
    it_redcard,
    it_blueskull,
    it_yellowskull,
    it_redskull,

    NUMCARDS

} card_t;



/* The defined weapons,*/
/*  including a marker indicating*/
/*  user has not changed weapon.*/
typedef enum
{
    wp_fist,
    wp_pistol,
    wp_shotgun,
    wp_chaingun,
    wp_missile,
    wp_plasma,
    wp_bfg,
    wp_chainsaw,
    wp_supershotgun,

    NUMWEAPONS,

    /* No pending weapon change.*/
    wp_nochange

} weapontype_t;


/* Ammunition types defined.*/
typedef enum
{
    am_clip,	/* Pistol / chaingun ammo.*/
    am_shell,	/* Shotgun / double barreled shotgun.*/
    am_cell,	/* Plasma rifle, BFG.*/
    am_misl,	/* Missile launcher.*/
    NUMAMMO,
    am_noammo	/* Unlimited for chainsaw / fist.	*/

} ammotype_t;


/* Power up artifacts.*/
typedef enum
{
    pw_invulnerability,
    pw_strength,
    pw_invisibility,
    pw_ironfeet,
    pw_allmap,
    pw_infrared,
    NUMPOWERS

} powertype_t;




/* Power up durations,*/
/*  how many seconds till expiration,*/
/*  assuming TICRATE is 35 ticks/second.*/

typedef enum
{
    INVULNTICS	= (30*TICRATE),
    INVISTICS	= (60*TICRATE),
    INFRATICS	= (120*TICRATE),
    IRONTICS	= (60*TICRATE)

} powerduration_t;





/* DOOM keyboard definition.*/
/* This is the stuff configured by Setup.Exe.*/
/* Most key data are simple ascii (uppercased).*/

#define KEY_RIGHTARROW	0xae
#define KEY_LEFTARROW	0xac
#define KEY_UPARROW	0xad
#define KEY_DOWNARROW	0xaf
#define KEY_ESCAPE	27
#define KEY_ENTER	13
#define KEY_TAB		9
#define KEY_BACKQUOTE   0x60
#define KEY_F1		(0x80+0x3b)
#define KEY_F2		(0x80+0x3c)
#define KEY_F3		(0x80+0x3d)
#define KEY_F4		(0x80+0x3e)
#define KEY_F5		(0x80+0x3f)
#define KEY_F6		(0x80+0x40)
#define KEY_F7		(0x80+0x41)
#define KEY_F8		(0x80+0x42)
#define KEY_F9		(0x80+0x43)
#define KEY_F10		(0x80+0x44)
#define KEY_F11		(0x80+0x57)
#define KEY_F12		(0x80+0x58)

#define KEY_PRINT	(0x80+0x60)

#define KEY_BACKSPACE	127
#define KEY_PAUSE	0xff

#define KEY_EQUALS	0x3d
#define KEY_MINUS	0x2d
#define KEY_PLUS	0x2b

#define KEY_RSHIFT	(0x80+0x36)
#define KEY_RCTRL	(0x80+0x1d)
#define KEY_RALT	(0x80+0x38)

#define KEY_LALT	KEY_RALT



#ifdef DIYBOOM

#define DEN_PLAYER5 4001
#define DEN_PLAYER6 4002
#define DEN_PLAYER7 4003
#define DEN_PLAYER8 4004

#define MORE_FRICTION_MOMENTUM 15000       /* mud factor based on momentum */
#define ORIG_FRICTION          0xE800      /* original value */
#define ORIG_FRICTION_FACTOR   2048        /* original value */

#endif

#endif          /* __DOOMDEF__*/
