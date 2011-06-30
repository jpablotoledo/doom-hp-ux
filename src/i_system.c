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

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#ifdef __riscos__
#include "ROsupport.h"
#include "GameSupp.h"
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "doomdef.h"
#include "dstrings.h"
#include "m_misc.h"
#include "m_argv.h"
#include "i_video.h"
#include "i_sound.h"
#include "i_net.h"

#include "d_net.h"
#include "g_game.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"





int     kb_used = 32*1024;


void
I_Tactile
( int	on,
  int	off,
  int	total )
{
  /* UNUSED.*/
  on = off = total = 0;
}

static ticcmd_t	emptycmd;
ticcmd_t*	I_BaseTiccmd(void)
{
    return &emptycmd;
}


int  I_GetHeapSize (void)
{
    return kb_used*1024;
}

byte* I_ZoneBase (int*	size)
{
    byte *block=NULL;
#ifdef __riscos__
    char *reserve, *resend;
    int reserve_bytes;

    /* Read number of bytes to keep free from environment variable, if it's defined. */
    if ((reserve = getenv("DOOMRESERVE")) != NULL)
    {
      reserve_bytes = strtol(reserve, &resend, 0);
      /* k for kilobyte added? */
      if ((*resend == 'k') || (*resend == 'K')) reserve_bytes <<= 10;
      /* some debugging code relies on Z_Zone in wimpslot, therefore allow
         disabling dynamic areas for debugging purposes */
      if (!M_CheckParm("-nodynarea"))
      {
        if (GameSupp_ClaimDynamicArea(1024*kb_used, reserve_bytes, 0x00000080, "Doom Z-Zone") == NULL)
        {
          const game_supp_dynamic_area_t *da = GameSupp_GetDynamicArea();

          atexit(I_ReleaseMemory);
          printf("I_ZoneBase: Starting with %dk memory (DA)\n", kb_used);
          *size = da->size;
          return da->memory;
        }
      }
      /* Reserve this many bytes for later mallocs */
      if ((reserve = (char*)malloc(reserve_bytes*sizeof(char))) == NULL)
      {
        I_Error("I_ZoneBase: Far too little free memory");
      }
      else
      {
        printf("I_ZoneBase: Reserving %d bytes...\n", reserve_bytes);
      }
    }
#endif
    /* Try to get as much memory as possible */
    if ((block = (byte*)malloc(1024*kb_used)) == NULL)
    {
      int trysize, step;

      /* binary search memory allocation */
      trysize = kb_used / 2; step = trysize / 2;
      while (step >= 64)
      {
        if ((block = (byte*)malloc(trysize * 1024)) == NULL)
        {
          trysize -= step;
        }
        else
        {
          free(block); kb_used = trysize; trysize += step;
        }
        step >>= 1;
      }
      /* Just to be on the safe side */
      while ((block = (byte*)malloc(1024 * kb_used)) == NULL)
      {
        kb_used -= 64;
      }
    }
    *size = 1024 * kb_used;
    printf("I_ZoneBase: Starting with %dk memory.\n", kb_used);
#ifdef __riscos__
    if (reserve != NULL) {free(reserve);}
#endif
    return block;
}




static unsigned int basetime = 0;


/* I_GetTime*/
/* returns time in 1/70th second tics*/

int  I_GetTime (void)
{
#ifdef __riscos__
    unsigned int newtime;

    newtime = ReadMonotonicTime();
    if (!basetime)
        basetime = newtime;
    /* Time in centiseconds */
    newtime = ((newtime - basetime) * TICRATE) / 100;
    return (int)newtime;
#else
    struct timeval	tp;
    struct timezone	tzp;
    int			newtics;

    gettimeofday(&tp, &tzp);
    if (!basetime)
	basetime = tp.tv_sec;
    newtics = (tp.tv_sec-basetime)*TICRATE + tp.tv_usec*TICRATE/1000000;
    return newtics;
#endif
}



void I_TimeBase(void)
{
  /*basetime = ReadMonotonicTime();*/
}



/* I_Init*/

void I_Init (void)
{
    I_InitSound();
    I_InitMusic();
    /*  I_InitGraphics();*/
}


void I_ReleaseMemory(void)
{
#ifdef __riscos__
    GameSupp_ReleaseDynamicArea();
#endif
}


/* I_Quit*/

void I_Quit (void)
{
    D_QuitNetGame ();
    I_ShutdownSound();
    I_ShutdownMusic();
    M_SaveDefaults ();
    I_ShutdownGraphics();
    I_ReleaseMemory();
    exit(0);
}


/* On RISC OS these functions are in i_video where they belong */
#ifndef __riscos__
void I_WaitVBL(int count)
{
#ifdef SGI
    sginap(1);
#else
#if (defined(SUN) || defined(__hpux) || defined(LINUX))
    sleep(0);
#else
    usleep (count * (1000000/70) );
#endif
#endif
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}
#endif

byte*	I_AllocLow(int length)
{
    byte*	mem;

    if ((mem = (byte *)malloc (length)) != NULL)
	memset (mem,0,length);

    return mem;
}



/* I_Error*/

/* RISC OS GCC + SharedCLib hack */
#ifdef RISCOS_FIX_VARARGS
#define __builtin_next_arg(p)	(((char*)(&p)) + __va_rounded_size(p))
#endif

void I_Error (const char *error, ...)
{
    va_list	argptr;

    fprintf(logfile, "Error: ");
#ifdef __riscos__
#ifdef RISCOS_FIX_VARARGS
    va_start(argptr, error);
    vfprintf(logfile, error, (va_list)(&argptr));
    va_end(argptr);
#else
    va_start(argptr, error);
    vfprintf(logfile, error, argptr);
    va_end(argptr);
#endif
#else
    /* Message first.*/
    va_start (argptr,error);
    vfprintf (logfile,error,argptr);
    va_end (argptr);
#endif
    fprintf (logfile, "\n");

    fflush( logfile );

    /* Shutdown. Here might be other errors.*/
    if (demorecording)
	G_CheckDemoStatus();

    D_QuitNetGame ();
    I_ShutdownGraphics();
    I_ShutdownSound();
    I_ShutdownNetwork();

    exit(-1);
}



/* I_GetCDSaveDir */

#ifdef __riscos__
const char *GamePathVar = "DoomGames$Path";
const char *DefaultGamePath = "Doom:Games.";
#else
const char *GamePathVar = "DOOMGAMEPATH";
const char *DefaultGamePath = "./";
static char *cdsavedir = NULL;
#define NOHOMESAVEDIR	"/tmp"
#endif

const char *I_GetCDSaveDir(void)
{
#ifdef __riscos__
  return "ChoicesWrite:"DOOMDATA;
#else
  const char *homedir;

  if (cdsavedir != NULL)
    return cdsavedir;

  if ((homedir = getenv("HOME")) == NULL)
  {
    if ((cdsavedir = (char*)malloc(strlen(NOHOMESAVEDIR DOOMDATA) + 2)) != NULL)
      sprintf(cdsavedir, NOHOMESAVEDIR DIRSEPSTR DOOMDATA);
  }
  else
  {
    if ((cdsavedir = (char*)malloc(strlen(homedir) + strlen(DOOMDATA) + 2)) != NULL)
      sprintf(cdsavedir, "%s" DIRSEPSTR DOOMDATA, homedir);
  }
  return cdsavedir;
#endif
}
