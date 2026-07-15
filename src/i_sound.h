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
/*	System interface, sound.*/

/*-----------------------------------------------------------------------------*/

#ifndef __I_SOUND__
#define __I_SOUND__


/*
 *  Define this if you don't want to precache sound data. For instance on RISC OS
 *  pre-caching isn't necessary and can cut down your Z_Zone requirements dramatically.
 */
#define DONT_PRECACHE


#include "doomdef.h"

/* UNIX hack, to be removed.*/
#ifdef SNDSERV
#include <stdio.h>
extern FILE* sndserver;
extern char* sndserver_filename;
extern int   mb_used;
#endif

#include "doomstat.h"
#include "sounds.h"


#ifdef __riscos__
extern int	sound_ready;
extern int	SixteenBitSound;
#endif

/* exported to m_misc.c */
extern int FadeoutTics;
extern int FrequencyMultiplier;
extern int ResampleSound;


/* Init at program start...*/
void I_InitSound();

/* ... update sound buffer and audio device at runtime...*/
void I_UpdateSound(void);
void I_SubmitSound(void);

/* ... shut down and relase at program termination.*/
void I_ShutdownSound(void);

/* Fade out SFX */
void I_FadeSound(void);



/*  SFX I/O*/


/* Initialize channels?*/
void I_SetChannels();

/* Get raw data lump index for sound descriptor.*/
int I_GetSfxLumpNum (const sfxinfo_t* sfxinfo );


/* Starts a sound in a particular sound channel.*/
int
I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority );


/* Stops a sound channel.*/
void I_StopSound(int handle);

/* Called by S_*() functions*/
/*  to see if a channel is still playing.*/
/* Returns 0 if no longer playing, 1 if playing.*/
int I_SoundIsPlaying(int handle);

/* Updates the volume, separation,*/
/*  and pitch of a sound channel.*/
void
I_UpdateSoundParams
( int		handle,
  int		vol,
  int		sep,
  int		pitch );



/*  MUSIC I/O*/

void I_InitMusic(void);
void I_ShutdownMusic(void);
/* Volume.*/
void I_SetMusicVolume(int volume);
/* PAUSE game handling.*/
void I_PauseSong(int handle);
void I_ResumeSong(int handle);
/* Registers a song handle to song data.*/
int I_RegisterSong(void *data, int length);
/* Called by anything that wishes to start music.*/
/*  plays a song, and when the song is done,*/
/*  starts playing it again in an endless loop.*/
/* Horrible thing to do, considering.*/
void
I_PlaySong
( int		handle,
  int		looping );
/* Stops a song over 3 seconds.*/
void I_StopSong(int handle);
/* See above (register), then think backwards*/
void I_UnRegisterSong(int handle);

#ifdef __hpux
/* Shared entry point for both the SIGALRM audio timer and the main
 * loop's own synchronous call - see i_sound.c for why this replaced
 * calling I_UpdateSound()/I_SubmitSound() directly from d_main.c. */
void I_HPAudioTick(void);
#endif

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
