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
/*	System interface for sound.*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: i_unix.c,v 1.5 1997/02/03 22:45:10 b1 Exp $";


/*
 *  I know this file is a _royal_ mess. Someday I'll have to take it all apart,
 *  put stuff into different files and use function pointers for all the vast
 *  configuration options. But not anytime soon... <zarquon@t-online.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <math.h>

#ifdef __riscos__
#include "ROsupport.h"
#include "GameSupp.h"
#else
#include <sys/time.h>
#include <sys/types.h>

#if (!defined(LINUX) && !defined(__hpux))
#include <sys/filio.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#endif

#ifdef LINUX
/* Linux voxware output.*/
#include <linux/soundcard.h>
#endif

#ifdef __sun
/*#define DOOM_SOUND_ULAW*/
#include <multimedia/libaudio.h>
#endif

#ifdef __hpux
#include "simpleAudio.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
/* In-process OPL2/GENMIDI synth — no separate process, no pipe. See
 * hp_music.c for why: a second process, even a cheap one, introduced
 * enough scheduling/IPC latency on this single-core machine to still
 * audibly stutter whenever Doom's own render loop was busy. */
#include "hp_music.h"
#endif

/* Timer stuff. Experimental.*/
#include <time.h>
#include <signal.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"

/* UNIX hack, to be removed.*/
#ifdef SNDSERV
/* Separate sound server process.*/
FILE*	sndserver=0;
char*	sndserver_filename = "./sndserver ";
#elif SNDINTR

/* Update all 30 millisecs, approx. 30fps synchronized.*/
/* Linux resolution is allegedly 10 millisecs,*/
/*  scale is microseconds.*/
/*#define SOUND_INTERVAL     500*/

/* This is bullshit! Let's do it _right_ for fuck's sake. */
static int		sound_interval=0;		/* depends from SampleCount and SAMPLERATE */
static int		samples_written=0;
struct timeval		last_sync_time;
/*
 *  Number of buffers to write ahead. The larger the less likely you'll
 *  get buffer overflows, but the higher the delay between the action and
 *  the sound. 3 performed very well on a Sun Ultra 1.
 */
#define SOUND_WRITEAHEAD_BUFFERS		3

/* Get the interrupt. Set duration in millisecs.*/
static int  I_SoundSetTimer( int duration_of_tick );
static void I_SoundDelTimer( void );
#else
/* None?*/
#endif

/* New macro: if defined the sound volume is compressed, i.e. silent sound gets */
/* louder, loud stuff remains about the same. The value of the macro determines */
/* the steepness of the tangent in 0. The minimum value for this is 2 */
/*#define SOUND_COMPRESSOR	6*/

/* Fix the volume range: make it variable. For original code use 127, but that's */
/* wrong. To do it right use 15. */
#define MAXIMUM_VOLUME		15


/* Fadeout-time in tics */
int FadeoutTics;
/*
 *  Do synthesis at a higher rate than the usual 11025Hz (FrequencyMultiplier=1).
 *  This actually makes an audible difference, but you should only use it on a fast
 *  machine, especially if you're going for CD quality (FrequencyMultiplier=4).
 */
int FrequencyMultiplier;
int ResampleSound;

static int LdFreqMult;
static int SampleCount;
static int LdSampCount;
static int UseFrequency;


/* The number of internal mixing channels,*/
/*  the samples calculated for each mixing step,*/
/*  the size of the 16bit, 2 hardware channel (stereo)*/
/*  mixing buffer, and the samplerate of the raw data.*/


static int SampleCount;
static int MixBufferSize;

#ifndef __riscos__
#ifdef DOOM_SOUND_ULAW
static byte *mixbuffer;
#else
static short *mixbuffer;
#endif
#endif

/* Needed for calling the actual sound output.*/
#define NUM_CHANNELS		8
/* It is 2 for 16bit, and 2 for two channels.*/
#ifdef __riscos__
#define BUFMUL			2
#define SAMPLESIZE		1
#else
#ifdef DOOM_SOUND_ULAW
#define BUFMUL			2
#define SAMPLESIZE		1
#else
#define BUFMUL                  4
#define SAMPLESIZE		2   	/* 16bit*/
#endif
#endif

#define SAMPLERATE		11025	/* Hz*/

/* The actual lengths of all sound effects.*/
static int	lengths[NUMSFX];

/* The global mixing buffer.*/
/* Basically, samples from all active internal channels*/
/*  are modifed and added, and stored in the buffer*/
/*  that is submitted to the audio device.*/
#ifdef __riscos__
/* wot's zis? Easy: the mixbuffer isn't needed here, we get a chunk in */
/* DMA space courtesy of the channel handler. */
int		sound_ready = 0;
int		SixteenBitSound = 0;
/*static byte*	mixbuffer = NULL;*/
byte		LinToLogTable[0x2000];
static int	channelBase = 1;
#else
/* The actual output device.*/
static int	audio_fd;
#endif


/*
 *  Rearranged the sound stuff so synthesis is much more efficient by
 *  grouping all information needed for each channel.
 *  DO NOT REARRANGE THE MEMBERS ON RISC OS! Assembler code will crash
 *  big time otherwise.
 */

typedef struct {
  byte*		samples;
  int		stepremainder;
  int*		leftvol_lookup;
  int*		rightvol_lookup;
  int		step;
  byte*		end;
} channel_desc;

typedef struct {
  int		lumpnum;
  int		sampsize;
  int		offset;
} channel_control;

channel_desc	channels[NUM_CHANNELS];
channel_control	channel_ctrl[NUM_CHANNELS];




/* Time/gametic that the channel started playing,*/
/*  used to determine oldest, which automatically*/
/*  has lowest priority.*/
/* In case number of active sounds exceeds*/
/*  available channels.*/
static int	channelstart[NUM_CHANNELS];

/* The sound in channel handles,*/
/*  determined on registration,*/
/*  might be used to unregister/stop/modify,*/
/*  currently unused.*/
static int	channelhandles[NUM_CHANNELS];

/* SFX id of the playing sound effect.*/
/* Used to catch duplicates (like chainsaw).*/
static int	channelids[NUM_CHANNELS];

/* Pitch to stepping lookup, unused.*/
static int	steptable[256];

/* Volume lookups.*/
static int	vol_lookup[(MAXIMUM_VOLUME+1)*256];

/* Sound initialized? */
static int	SoundInitialized = 0;

/* use sound at all? */
static int	SoundDisabled = 0;





/* Number of tics in which to fade out sound */
#define FADE_TICS	TICRATE

/* Fade out SFX */
void I_FadeSound(void)
{
    int		leftvol[NUM_CHANNELS], rightvol[NUM_CHANNELS];
    int		i, sfxon, time0, time1;

    if (SoundDisabled != 0)
        return;

    for (i=0; i<NUM_CHANNELS; i++)
    {
        if (channels[i].samples == NULL)
        {
            leftvol[i] = 0; rightvol[i] = 0;
        }
        else
        {
	    leftvol[i] = (channels[i].leftvol_lookup - vol_lookup) / 256;
	    rightvol[i] = (channels[i].rightvol_lookup - vol_lookup) / 256;
        }
    }

    time0 = I_GetTime();

    /* First wait for FadeoutTics tics for channels to stop playing */
    do
    {
	time1 = I_GetTime(); sfxon = 0;
	for (i=0; i<NUM_CHANNELS; i++)
	{
	    if (channels[i].samples != NULL) sfxon = 1;
	}
	if (sfxon == 0) break;
    }
    while (time1 - time0 < FadeoutTics);

    /* All channels mute anyway? */
    if (time1 - time0 >= FadeoutTics)
    {
	time0 = time1;
	/* Then fade out sound over FADE_TICS tics */
	do
	{
	    time1 = I_GetTime(); sfxon = 0;
	    for (i=0; i<NUM_CHANNELS; i++)
	    {
		if (channels[i].samples != NULL)
		{
		    sfxon = (leftvol[i]*(FADE_TICS + time0 - time1)) / FADE_TICS;
		    if (sfxon < 0) sfxon = 0;
		    channels[i].leftvol_lookup = vol_lookup + 256*sfxon;
		    sfxon = (rightvol[i]*(FADE_TICS + time0 - time1)) / FADE_TICS;
		    if (sfxon < 0) sfxon = 0;
		    channels[i].rightvol_lookup = vol_lookup + 256*sfxon;
		    sfxon = 1;
		}
	    }
	    if (sfxon == 0) break;
	}
	while (time1 - time0 < FADE_TICS);
    }

    for (i=0; i<NUM_CHANNELS; i++)
    {
      channel_ctrl[i].lumpnum = -1;
      channels[i].samples = NULL;
      channelhandles[i] = -1;
    }
}




/* Safe ioctl, convenience.*/
#ifndef __riscos__
static void
myioctl
( int	fd,
  int	command,
  void*	arg )
{
    int		rc;
    extern int	errno;

    rc = ioctl(fd, command, arg);
    if (rc < 0)
    {
	fprintf(logfile, "ioctl(dsp,%d,arg) failed\n", command);
	fprintf(logfile, "errno=%d\n", errno);
	exit(-1);
    }
}
#endif




/* This function loads the sound data from the WAD lump,*/
/*  for single sound.*/
#ifdef DONT_PRECACHE
static int
#else
static void*
#endif
getsfx
( char*         sfxname,
  int*          len )
{
    int                 sfxlump;
    char                name[20];
#ifndef DONT_PRECACHE
    unsigned char*      sfx;
    unsigned char*      paddedsfx;
    int                 size;
    int                 paddedsize;
    int                 i;
#endif

    /* Get the sound data from the WAD, allocate lump*/
    /*  in zone memory.*/
    sprintf(name, "ds%s", sfxname);

    /* Now, there is a severe problem with the*/
    /*  sound handling, in it is not (yet/anymore)*/
    /*  gamemode aware. That means, sounds from*/
    /*  DOOM II will be requested even with DOOM*/
    /*  shareware.*/
    /* The sound list is wired into sounds.c,*/
    /*  which sets the external variable.*/
    /* I do not do runtime patches to that*/
    /*  variable. Instead, we will use a*/
    /*  default sound for replacement.*/
    if ( W_CheckNumForName(name) == -1 )
      sfxlump = W_GetNumForName("dspistol");
    else
      sfxlump = W_GetNumForName(name);

#ifdef DONT_PRECACHE
    *len = W_LumpLength( sfxlump ) - 8;
    return sfxlump;
#else
    size = W_LumpLength( sfxlump );
    /* Debug.*/
    /* fprintf( stderr, "." );*/
    /*fprintf( stderr, " -loading  %s (lump %d, %d bytes)\n",*/
    /*	     sfxname, sfxlump, size );*/
    /*fflush( stderr );*/

    sfx = (unsigned char*)W_CacheLumpNum( sfxlump, PU_STATIC );

    /* Pads the sound effect out to the mixing buffer size.*/
    /* The original realloc would interfere with zone memory.*/
    paddedsize = ((size-8 + ((1 << LdSampCount)-1)) >> LdSampCount) << LdSampCount;

    /* Allocate from zone memory.*/
    paddedsfx = (unsigned char*)Z_Malloc( paddedsize+8, PU_STATIC, 0 );
    /* ddt: (unsigned char *) realloc(sfx, paddedsize+8);*/
    /* This should interfere with zone memory handling,*/
    /*  which does not kick in in the soundserver.*/

    /* Now copy and pad.*/
    memcpy(  paddedsfx, sfx, size );
    for (i=size ; i<paddedsize+8 ; i++)
        paddedsfx[i] = 128;

    /* Remove the cached lump.*/
    Z_Free( sfx );

    /* Preserve padded length.*/
    *len = paddedsize;

    /* Return allocated padded data.*/
    return (void *) (paddedsfx + 8);
#endif
}






/* This function adds a sound to the*/
/*  list of currently active sounds,*/
/*  which is maintained as a given number*/
/*  (eight, usually) of internal channels.*/
/* Returns a handle.*/

static int
addsfx
( int		sfxid,
  int		volume,
  int		step,
  int		seperation )
{
    static unsigned short	handlenums = 0;

    int		i;
    int		rc = -1;

    int		oldest = gametic;
    int		oldestnum = 0;
    int		slot;

    int		rightvol;
    int		leftvol;
    byte*       newsample;

    /* Chainsaw troubles.*/
    /* Play these sound effects only one at a time.*/
    if ( sfxid == sfx_sawup
	 || sfxid == sfx_sawidl
	 || sfxid == sfx_sawful
	 || sfxid == sfx_sawhit
	 || sfxid == sfx_stnmov
	 || sfxid == sfx_pistol	 )
    {
	/* Loop all channels, check.*/
	for (i=0 ; i<NUM_CHANNELS ; i++)
	{
	    /* Active, and using the same SFX?*/
	    if ( (channels[i].samples)
		 && (channelids[i] == sfxid) )
	    {
		/* Reset.*/
		channel_ctrl[i].lumpnum = -1;
		channels[i].samples = NULL;
		/* We are sure that iff,*/
		/*  there will only be one.*/
		break;
	    }
	}
    }

    /* Loop all channels to find oldest SFX.*/
    for (i=0; (i<NUM_CHANNELS) && (channels[i].samples); i++)
    {
	if (channelstart[i] < oldest)
	{
	    oldestnum = i;
	    oldest = channelstart[i];
	}
    }

    /* Tales from the cryptic.*/
    /* If we found a channel, fine.*/
    /* If not, we simply overwrite the first one, 0.*/
    /* Probably only happens at startup.*/
    if (i == NUM_CHANNELS)
	slot = oldestnum;
    else
	slot = i;

    /* Stop old sound */
    channel_ctrl[slot].lumpnum = -1;
    channels[slot].samples = NULL;

    /* Okay, in the less recent channel,*/
    /*  we will handle the new SFX.*/
    /* Set pointer to raw data.*/
#ifdef DONT_PRECACHE
    newsample = (byte *) W_CacheLumpNum(S_sfx[sfxid].lumpnum, PU_CACHE) + 8;
    if (newsample == NULL) return rc;
#else
    newsample = (byte *) S_sfx[sfxid].data;
#endif
    /* Set pointer to end of raw data.*/
    channels[slot].end = newsample + lengths[sfxid];
    channel_ctrl[slot].sampsize = lengths[sfxid];
    channel_ctrl[slot].offset = 0;

    /* Reset current handle number, limited to 0..100.*/
    if (!handlenums)
	handlenums = 100;

    /* Assign current handle number.*/
    /* Preserved so sounds could be stopped (unused).*/
    channelhandles[slot] = rc = handlenums++;

    /* Set stepping???*/
    /* Kinda getting the impression this is never used.*/
    channels[slot].step = step >> LdFreqMult;
    /* ???*/
    channels[slot].stepremainder = 0;
    /* Should be gametic, I presume.*/
    channelstart[slot] = gametic;

    /* Separation, that is, orientation/stereo.*/
    /*  range is: 1 - 256*/
    seperation += 1;

    /* Per left/right channel.*/
    /*  x^2 seperation,*/
    /*  adjust volume properly.*/
    leftvol =
	volume - ((volume*seperation*seperation) >> 16); /*/(256*256);*/
    seperation = seperation - 257;
    rightvol =
	volume - ((volume*seperation*seperation) >> 16);

    /* Sanity check, clamp volume.*/
    if (rightvol < 0 || rightvol > MAXIMUM_VOLUME)
	I_Error("rightvol out of bounds");

    if (leftvol < 0 || leftvol > MAXIMUM_VOLUME)
	I_Error("leftvol out of bounds");

    /* Get the proper lookup table piece*/
    /*  for this volume level???*/
    channels[slot].leftvol_lookup = &vol_lookup[leftvol*256];
    channels[slot].rightvol_lookup = &vol_lookup[rightvol*256];

    /* Preserve sound SFX id,*/
    /*  e.g. for avoiding duplicates of chainsaw.*/
    channelids[slot] = sfxid;

    /* Set the actual sample only after everything else is OK */
    channel_ctrl[slot].lumpnum = S_sfx[sfxid].lumpnum;
    channels[slot].samples = newsample;

    /* You tell me.*/
    return rc;
}






/* SFX API*/
/* Note: this was called by S_Init.*/
/* However, whatever they did in the*/
/* old DPMS based DOS version, this*/
/* were simply dummies in the Linux*/
/* version.*/
/* See soundserver initdata().*/

void I_SetChannels(void)
{
  /* Init internal lookups (raw data, mixing buffer, channels).*/
  /* This function sets up internal lookups used during*/
  /*  the mixing process. */
  int		i;
  int		j;
#ifdef SOUND_COMPRESSOR
  int		vol, vol_inf, vol_inf_norm;
#endif

  int*	steptablemid = steptable + 128;

  /* Okay, reset internal mixing channels to zero.*/
  for (i=0; i<NUM_CHANNELS; i++)
  {
    channel_ctrl[i].lumpnum = -1;
    channels[i].samples = NULL;
  }

  /* This table provides step widths for pitch parameters.*/
  /* I fail to see that this is currently used.*/
  for (i=-128 ; i<128 ; i++)
    steptablemid[i] = (int)(pow(2.0, (i/64.0))*65536.0);


  /* Generates volume lookup tables*/
  /*  which also turn the unsigned samples*/
  /*  into signed samples.*/
#ifdef SOUND_COMPRESSOR
  vol_inf = 0x7fff*SOUND_COMPRESSOR / (SOUND_COMPRESSOR - 1);
  vol_inf_norm = vol_inf / SOUND_COMPRESSOR;
  for (i=0 ; i<MAXIMUM_VOLUME+1 ; i++)
  {
    vol_lookup[i*256] = -i*256; vol_lookup[i*256 + 128] = 0;
    for (j=1 ; j<128 ; j++)
    {
      /*  Don't play around with this algorithm. The values just about fit
       *  into a signed integer (vol_inf <= 2*0x7fff (SOUND_COMPRESSOR >= 2),
       *  vol <= 0x7f00 ==> product <= 0x7eff0200, i.e. a fucking close shave).
       */
      vol = (i*j*256)/MAXIMUM_VOLUME;
      vol = vol*vol_inf / (vol + vol_inf_norm);
      vol_lookup[i*256 + 128 + j] = vol;
      vol_lookup[i*256 + 128 - j] = -vol;
    }
  }
#else
  for (i=0 ; i<MAXIMUM_VOLUME+1 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i*256+j] = (i*(j-128)*256)/MAXIMUM_VOLUME;
#endif
}


#if 0
static void I_SetSfxVolume(int volume)
{
  /* Identical to DOS.*/
  /* Basically, this should propagate*/
  /*  the menu/config file setting*/
  /*  to the state variable used in*/
  /*  the mixing.*/
  snd_SfxVolume = volume;
}
#endif

/* Retrieve the raw data lump index*/
/*  for a given SFX name.*/

int I_GetSfxLumpNum(const sfxinfo_t* sfx)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}


/* Starting a sound means adding it*/
/*  to the current list of active sounds*/
/*  in the internal channels.*/
/* As the SFX info struct contains*/
/*  e.g. a pointer to the raw data,*/
/*  it is ignored.*/
/* As our sound handling does not handle*/
/*  priority, it is ignored.*/
/* Pitching (that is, increased speed of playback)*/
/*  is set, but currently not used by mixing.*/

int
I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority )
{

  /* UNUSED*/
  priority = 0;

#ifdef SNDSERV
    if (sndserver)
    {
	fprintf(sndserver, "p%2.2x%2.2x%2.2x%2.2x\n", id, pitch, vol, sep);
	fflush(sndserver);
    }
    /* warning: control reaches end of non-void function.*/
    return id;
#else
#ifdef __hpux
    {
      static int sfx_trigger_count = 0;
      sfx_trigger_count++;
      if (sfx_trigger_count <= 8)
        fprintf(logfile, "SFX trigger #%d id=%d vol=%d fd=%d\n",
                sfx_trigger_count, id, vol, audio_fd);
    }
#endif
    /* Returns a handle (not used).*/
    id = addsfx( id, vol, steptable[pitch], sep );

    return id;
#endif
}



/* Really does something now */
void I_StopSound (int handle)
{
    int		i;

    for (i=0; i<NUM_CHANNELS; i++)
    {
	if (channelhandles[i] == handle)
	{
	    /* Although you should turn of the channel this really doesn't sound good! */
	    /* channels[i].samples = NULL; */
	    channelhandles[i] = -1;
	    return;
	}
    }
}


/* Ditto */
int I_SoundIsPlaying(int handle)
{
    int		i;

    for (i=0; i<NUM_CHANNELS; i++)
    {
	if (channelhandles[i] == handle)
	{
	    if (channels[i].samples != NULL) return 1;
	    channelhandles[i] = -1;
	    return 0;
	}
    }
    return 0;
}




/*
 * RISC OS method:
 * Synthesis code is called from the channel handler code (DigitalRenderer
 * support module). That approach requires synthesis to be done in an
 * assembler function because it requires special handling (because it's
 * executed in IRQ mode with IRQs _en_abled!). Here's the function prototype.
 * NEVER attempt to call this function directly from C, it'll crash big time!
 * I_UpdateSound only sets a flag word to 1 to indicate the sound system is
 * ready (god knows what would happen if the callback function tried to
 * access uninitialised data).
 */
#ifdef __riscos__
void CallBackFillCode(byte *buffer, byte *lin2log, int size);


/*
 * This function builds a LinToLog table based on maximum volume. The one
 * passed by the channel handler is scaled by the volume specified under
 * RISC OS. We have our own volume control in the game, though, so we use
 * this table rather than the channel handler's.
 */
static void BuildLinToLogTable(void)
{
  int i, vol, step, width, w, x;

  LinToLogTable[0] = 0; LinToLogTable[0x1000] = 0xff;
  step = (1<<16); vol = step; width = 16; w = width;
  for (i=1; i<0x1000; i++)
  {
    x = (vol>>16);
    if (x >= 0x80) x = 0x7f;
    LinToLogTable[i] = (x << 1);
    LinToLogTable[0x2000 - i] = (x << 1) | 1;
    vol += step;
    if (--w == 0)
    {
      width <<= 1; step >>= 1; w = width;
    }
  }
}
#endif


/*
 *  Note on resampling: it's best to resample after rather than before
 *  the volume lookup, because we're interpolating 16bit values rather
 *  than 8bit values then; that is considerably more expensive, though:
 *  If we do it before, we need 1 multiplication plus two volume-lookups,
 *  but if we did it afterwards we require 2 multiplications and 4 volume
 *  lookups. Decide on which variant you want depending on the setting
 *  of the resamp_sound config value: 0 = none, 1 = normal, 2 = best;
 */

#define READ_SOUND_SAMPLE \
  /* Get the raw data from the channel. */ \
  sample = *channels[ chan ].samples; \
  /* Add left and right part for this channel (sound)*/ \
  /* to the current data. Adjust volume accordingly.*/ \
  dl += channels[ chan ].leftvol_lookup[ sample ]; \
  dr += channels[ chan ].rightvol_lookup [sample ];

#define RESAMPLE_SOUND_SAMPLE_NORM \
  sample = (channels[chan].samples)[0]; \
  if (channels[chan].samples < channels[chan].end-1) \
  { \
    sample += (((signed int)((channels[chan].samples)[1] - sample) * channels[chan].stepremainder)) >> 16; \
  } \
  dl += channels[chan].leftvol_lookup[sample]; \
  dr += channels[chan].rightvol_lookup[sample];

#define RESAMPLE_SOUND_SAMPLE_BEST \
  if (channels[chan].samples < channels[chan].end-1) \
  { \
    sample = channels[chan].leftvol_lookup[(channels[chan].samples)[0]]; \
    dl += sample + (((signed int)((channels[chan].leftvol_lookup[(channels[chan].samples)[1]] - sample) * channels[chan].stepremainder)) >> 16); \
    sample = channels[chan].rightvol_lookup[(channels[chan].samples)[0]]; \
    dr += sample + (((signed int)((channels[chan].rightvol_lookup[(channels[chan].samples)[1]] - sample) * channels[chan].stepremainder)) >> 16); \
  } \
  else \
  { \
    READ_SOUND_SAMPLE; \
  }

#define UPDATE_SOUND_SAMPLE(samplefunc) \
  /* Reset left/right value. */ \
  dl = 0; \
  dr = 0; \
  /* Love thy L2 chache - made this a loop.*/ \
  /* Now more channels could be set at compile time*/ \
  /*  as well. Thus loop those  channels.*/ \
  for ( chan = 0; chan < NUM_CHANNELS; chan++ ) \
  { \
    /* Check channel, if active.*/ \
    if (channels[ chan ].samples) \
    { \
      samplefunc; \
      /* Increment index ???*/ \
      channels[ chan ].stepremainder += channels[ chan ].step; \
      /* MSB is next sample???*/ \
      channels[ chan ].samples += channels[ chan ].stepremainder >> 16; \
      /* Limit to LSB???*/ \
      channels[ chan ].stepremainder &= 65536-1; \
      /* Check whether we are done.*/ \
      if (channels[ chan ].samples >= channels[ chan ].end) \
        channels[ chan ].samples = 0; \
    } \
  } \
  /* Clamp to range. Left hardware channel.*/ \
  if (dl > 0x7fff) \
    dl = 0x7fff; \
  else if (dl < -0x8000) \
    dl = -0x8000; \
  /* Same for right hardware channel.*/ \
  if (dr > 0x7fff) \
    dr = 0x7fff; \
  else if (dr < -0x8000) \
    dr = -0x8000;


#ifdef DOOM_SOUND_ULAW
  /* This is badly documentated in libaudio.h: */
  /* linear to ulaw tablesize is 14 bits, with 0 in the middle (!) */
#ifdef __sun
/* Compiler must extend sign */
#define CONVERT_SAMPLE(s) _linear2ulaw[(((signed int)(s))>>3)]
#else
#error  "Don't know ulaw tables for this machine!"
#endif
#else
#define CONVERT_SAMPLE(s) (s)
#endif

#define STORE_SOUND_AND_STEP \
  *leftout = CONVERT_SAMPLE(dl); \
  *rightout = CONVERT_SAMPLE(dr); \
  /* Increment current pointers in mixbuffer.*/ \
  leftout += step; \
  rightout += step;


/* This function loops all active (internal) sound*/
/*  channels, retrieves a given number of samples*/
/*  from the raw sound data, modifies it according*/
/*  to the current (internal) channel parameters,*/
/*  mixes the per channel samples into the global*/
/*  mixbuffer, clamping it to the allowed range,*/
/*  and sets up everything for transferring the*/
/*  contents of the mixbuffer to the (two)*/
/*  hardware channels (left and right, that is).*/

/* This function currently supports only 16bit.*/

#ifndef __riscos__
void I_UpdateSound( void )
{
  /* Mix current sound data.*/
  /* Data, from raw sound, for right and left.*/
  register unsigned int	sample;
  register int		dl;
  register int		dr;

  /* Pointers in global mixbuffer, left, right, end.*/
#ifdef DOOM_SOUND_ULAW
  byte*			leftout;
  byte*			rightout;
  byte*			leftend;
#else
  signed short*		leftout;
  signed short*		rightout;
  signed short*		leftend;
#endif
  /* Step in mixbuffer, left and right, thus two.*/
  int				step;

  /* Mixing channel index.*/
  int				chan;

  step = 2;

  if (snd_SfxVolume == 0)
  {
    memset(mixbuffer, CONVERT_SAMPLE(0), MixBufferSize);
    return;
  }

  /* Left and right channel*/
  /*  are in global mixbuffer, alternating.*/
  leftout = mixbuffer;
  rightout = mixbuffer+1;

  /* Determine end, for left channel only*/
  /*  (right channel is implicit).*/
  leftend = mixbuffer + (step << LdSampCount);

  /* Mix sounds into the mixing buffer.*/
  /* Loop over step*SampleCount,*/
  /*  that is 512 values for two channels.*/
  switch (ResampleSound)
  {
    case 1:
      while (leftout != leftend)
      {
        UPDATE_SOUND_SAMPLE(RESAMPLE_SOUND_SAMPLE_NORM);
        STORE_SOUND_AND_STEP;
      }
      break;
    case 2:
      while (leftout != leftend)
      {
        UPDATE_SOUND_SAMPLE(RESAMPLE_SOUND_SAMPLE_BEST);
	STORE_SOUND_AND_STEP;
      }
      break;
    default:
      while (leftout != leftend)
      {
        UPDATE_SOUND_SAMPLE(READ_SOUND_SAMPLE);
        STORE_SOUND_AND_STEP;
      }
      break;
  }
}
#endif



/* This would be used to write out the mixbuffer*/
/*  during each game loop update.*/
/* Updates sound buffer and audio device at runtime. */
/* It is called during Timer interrupt with SNDINTR.*/
/* Mixing now done synchronous, and*/
/*  only output be done asynchronous?*/

void
I_SubmitSound(void)
{
#ifdef __riscos__
  /* On RISC OS this is automatic: the sound driver module calls back the
     fill code when it needs to. */
#else
#ifndef DOOM_NO_SFX
  if (SoundDisabled == 0)
  {
#ifdef __hpux
    /* Throttle writes: I_SubmitSound is called 35x/sec but UseFrequency is
       11025Hz, so without throttling we write 1.6x faster than playback and
       the Aserver buffer fills up causing growing lag. Track queued samples
       against real-time and skip writes when we're more than 2 buffers ahead. */
    {
      static struct timeval prev = {0, 0};
      static int hp_queued = 0;
      struct timeval now;
      long usec_diff;
      int consumed;

      gettimeofday(&now, NULL);
      if (prev.tv_sec == 0) { prev = now; hp_queued = 0; }

      usec_diff = (now.tv_sec - prev.tv_sec) * 1000000L +
                  (now.tv_usec - prev.tv_usec);
      consumed = (int)((long)usec_diff * UseFrequency / 1000000L);
      prev = now;

      hp_queued -= consumed;
      if (hp_queued < 0) hp_queued = 0;

      if (hp_queued < 2 * SampleCount)
      {
        /* Generate music directly (in-process synth, no pipe — see
         * hp_music.c) and mix it into the SFX buffer before output. */
        {
          short music_buf[1024];   /* max SampleCount stereo frames = 512*2 */
          int ns = (int)(MixBufferSize / sizeof(short));
          signed short *mp = music_buf;
          signed short *sp = mixbuffer;
          signed short *end = mixbuffer + ns;

          HPMusic_Generate(music_buf, ns / 2);
          while (sp < end)
          {
            int a = (int)*sp;
            int b = (int)*mp;
            int v;
            /* Virtual-analog mix: compress same-sign pairs toward 0.
             * Both-pos: subtract a*b/MAX. Both-neg: add (works on magnitudes).
             * Different signs: simple sum (guaranteed within range). */
            if (a >= 0 && b >= 0)
              v = a + b - (int)((long)a * b / 32767);
            else if (a < 0 && b < 0) {
              int na = -a, nb = -b;
              v = -(na + nb - (int)((long)na * nb / 32767));
            } else
              v = a + b;
            *sp = (signed short)v;
            sp++; mp++;
          }
        }

        {
          int wret = write(audio_fd, mixbuffer, MixBufferSize);
          if (wret > 0)
            hp_queued += SampleCount;
        }
      }
    }
#else
    /* Write it to DSP device.*/
    write(audio_fd, mixbuffer, MixBufferSize);
#endif
  }
#endif
#endif
}

#ifdef __hpux
/* Real-time-independent audio feeding: on real hardware, Doom's own
 * render loop is already CPU-bound (~85% of this machine's single core
 * with no music at all), so I_UpdateSound()/I_SubmitSound() called only
 * once per game-loop iteration (as d_main.c does below) get invoked
 * irregularly under load — often much less than the ~35/sec the audio
 * pipeline assumes. When that happens Aserver's own buffer runs dry
 * between calls and the output audibly cuts.
 *
 * This mirrors why the original SFX throttle (docs/agregar-sonido.md)
 * was needed — writing too FAST overran Aserver — except this is the
 * mirror-image problem: writing too INFREQUENTLY underruns it. The
 * codebase already had a generic answer for exactly this (the SNDINTR
 * timer-interrupt scaffolding further down), but it was never wired up
 * for HP-UX, and its handler predates the music work so it would
 * silently bypass all of it. This installs an equivalent real
 * SIGALRM/setitimer interrupt that calls our already-validated
 * I_UpdateSound()+I_SubmitSound() on a fixed schedule, independent of
 * how long the current render frame is taking. d_main.c blocks SIGALRM
 * around its own (still-present) synchronous calls to avoid the two
 * ever truly overlapping.
 *
 * This alone wasn't enough while music ran in a separate process
 * (musserver-hpux, talking to Doom over a pipe): even a cheap child
 * process introduced enough inter-process scheduling/IPC latency to
 * keep stuttering under load, confirmed by a raw capture of Aserver's
 * input showing the synth itself was never the bottleneck. hp_music.c
 * replaces that with an in-process synth called directly from
 * I_SubmitSound() below — no process boundary left to schedule around. */
/* Shared, syscall-free reentrancy guard between the SIGALRM handler and
 * the main loop's own direct call (d_main.c) — both go through this
 * instead of calling I_UpdateSound()/I_SubmitSound() themselves, so
 * whichever gets there first wins and the other just skips that one
 * cycle. sig_atomic_t is safe to read/write from a signal handler
 * without a lock. This replaces an earlier sigprocmask()-based version:
 * that blocked/unblocked SIGALRM around every single main-loop call,
 * which is fine at 35 calls/sec but this D_DoomLoop() while(1) has no
 * frame cap and can spin thousands of times/sec on light frames —
 * measured live via vmstat, the two extra syscalls/iteration pushed the
 * machine to ~50-60k syscalls/sec and ~0% idle CPU, which was the
 * actual cause of a regression that looked like an audio/render stall. */
static volatile sig_atomic_t hp_audio_busy = 0;

void I_HPAudioTick(void)
{
  if (hp_audio_busy) return;
  hp_audio_busy = 1;
  I_UpdateSound();
  I_SubmitSound();
  hp_audio_busy = 0;
}

static void hp_audio_timer_handler(int signum)
{
  signum = 0;
  I_HPAudioTick();
}

void I_HPStartAudioTimer(void)
{
  struct sigaction act;
  struct itimerval value;
  int interval_us;

  if (SoundDisabled) return;

  memset(&act, 0, sizeof(act));
  act.sa_handler = hp_audio_timer_handler;
  act.sa_flags = SA_RESTART;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, SIGALRM);
  sigaction(SIGALRM, &act, NULL);

  /* Same interval formula as the Sun/HP branch of the generic SNDINTR
   * timer further down: slightly under 100% of buffer playtime so we
   * feed Aserver a bit ahead of when it needs the next chunk. */
  interval_us = (int)((950000L * SampleCount) / UseFrequency);
  value.it_interval.tv_sec  = 0;
  value.it_interval.tv_usec = interval_us;
  value.it_value.tv_sec  = 0;
  value.it_value.tv_usec = interval_us;
  setitimer(ITIMER_REAL, &value, NULL);
  fprintf(logfile, "I_HPStartAudioTimer: %d microsecs\n", interval_us);
}

void I_HPStopAudioTimer(void)
{
  struct itimerval value;
  memset(&value, 0, sizeof(value));
  setitimer(ITIMER_REAL, &value, NULL);
}
#endif



void
I_UpdateSoundParams
( int	handle,
  int	vol,
  int	sep,
  int	pitch)
{
  /* I fail too see that this is used.*/
  /* Would be using the handle to identify*/
  /*  on which channel the sound might be active,*/
  /*  and resetting the channel parameters.*/

  /* UNUSED.*/
  handle = vol = sep = pitch = 0;
}




void I_ShutdownSound(void)
{
  if (SoundInitialized == 0) return;

#ifdef SNDSERV
  if (sndserver)
  {
    /* Send a "quit" command.*/
    fprintf(sndserver, "q\n");
    fflush(sndserver);
  }
#else
  /* FIXME (below).*/
  fprintf( logfile, "I_ShutdownSound: NOT finishing pending sounds\n");
  fflush( logfile );

#ifndef DOOM_NO_SOUND
  /* Wait till all pending sounds are finished.*/
  I_FadeSound();

#ifdef SNDINTR
  I_SoundDelTimer();
#endif

#ifdef __hpux
  I_HPStopAudioTimer();
#endif

#ifdef __riscos__
  RemoveDoomSound();
#elif defined(__hpux)
  closeAStream(audio_fd);
  closeAudio();
#else
  /* Cleaning up -releasing the DSP device.*/
  close ( audio_fd );
#endif
#endif
#endif	/* DOOM_NO_SOUND */

  SoundInitialized = 0;

  /* Done.*/
  return;
}






void
I_InitSound(void)
{
  int i;

#ifdef SNDSERV
  char buffer[256];

  if (getenv("DOOMWADDIR"))
    sprintf(buffer, "%s/%s",
	    getenv("DOOMWADDIR"),
	    sndserver_filename);
  else
    sprintf(buffer, "%s", sndserver_filename);

  /* start sound process*/
  if ( !access(buffer, X_OK) )
  {
    strcat(buffer, " -quiet");
    sndserver = popen(buffer, "w");
  }
  else
    fprintf(logfile, "Could not start sound server [%s]\n", buffer);
#endif

  for (i=0; i<NUM_CHANNELS; i++) channelhandles[i] = -1;

  i = M_CheckParm("-fadeout");
  if (i & (i < myargc-1))
  {
    FadeoutTics = TICRATE * atoi(myargv[i+1]);
  }

  /* Secure and configure sound device first.*/
  fprintf( logfile, "I_InitSound: ");

  switch (FrequencyMultiplier)
  {
    case 2:
      LdFreqMult = 1; break;
    case 4:
      LdFreqMult = 2; break;
    default:
      LdFreqMult = 0; break;
  }
  FrequencyMultiplier = (1 << LdFreqMult);
  SampleCount = 512 * FrequencyMultiplier;
  LdSampCount = LdFreqMult + 9;
  MixBufferSize = SampleCount * BUFMUL;
  UseFrequency = SAMPLERATE * FrequencyMultiplier;

#ifndef __riscos__
#ifdef DOOM_SOUND_ULAW
  if ((mixbuffer = (byte*)malloc(MixBufferSize * sizeof(byte))) == NULL)
    mixbuffer = (byte*)Z_MallocNoAbort(MixBufferSize * sizeof(byte), PU_STATIC, NULL);
#else
  if ((mixbuffer = (short*)malloc(MixBufferSize * sizeof(short))) == NULL)
    mixbuffer = (short*)Z_MallocNoAbort(MixBufferSize * sizeof(short), PU_STATIC, NULL);
#endif
  if (mixbuffer == NULL)
  {
    I_Error("Couldn't claim memory for mixbuffer");
  }
#endif

#ifndef DOOM_NO_SFX
  if (M_CheckParm("-nosound"))
  {
    fprintf(logfile, "[disabled]");
    SoundDisabled = 1;
  }
  else
  {
#ifdef __riscos__
    BuildLinToLogTable();

    SampleCount = 512 * FrequencyMultiplier;

    i = M_CheckParm("-channelbase");
    if (i & (i < myargc-1))
    {
      channelBase = atoi(myargv[i+1]);
    }

    if (SixteenBitSound != 0)
    {
      if (InstallDoomSound16(UseFrequency) != 0)
      {
        fprintf(logfile, "Couldn't init 16bit Doom sound.\n");
      }
      else
      {
        fprintf(logfile, "[16bit linear] ");
      }
    }
    else
    {
      if (InstallDoomSound(channelBase, SampleCount, 1000000 / UseFrequency) != 0)
      {
        fprintf(logfile, "Couldn't init Doom voices.\n");
      }
      else
      {
        fprintf(logfile, "[8bit ulaw] ");
      }
    }
#else
    SampleCount = 512 * FrequencyMultiplier;
#ifdef __sun
#ifndef O_SYNC
#define O_SYNC	0
    fprintf(logfile, "(no synchronized option) ");
#endif
    audio_fd = open("/dev/audio", O_WRONLY | O_SYNC);
    if (audio_fd < 0)
    {
      fprintf(logfile, "Could not open /dev/audio\n");
      return;
    }
    /* Write audio file info */
    {
      Audio_hdr head;

      head.sample_rate = UseFrequency;
      head.samples_per_unit = 1;
#ifdef DOOM_SOUND_ULAW
      fprintf(logfile, "using 8bit ulaw format; ");
      head.sample_rate = 2 * UseFrequency;	/* Odd? Yes, but necessary */
      head.bytes_per_unit = 1;
      head.encoding = AUDIO_ENCODING_ULAW;
#else
      fprintf(logfile, "using 16bit linear format; ");
      head.bytes_per_unit = 2;
      head.encoding = AUDIO_ENCODING_LINEAR;
#endif
      head.channels = 2;
      head.data_size = AUDIO_UNKNOWN_SIZE;
      head.endian = AUDIO_ENDIAN_BIG;
      audio_set_play_config(audio_fd, &head);
    }
#elif defined(__hpux)
    if (openAudio() != 0)
    {
      fprintf(logfile, "Could not connect to HP audio server\n");
      return;
    }
    audio_fd = openAStream(PLAY_STREAM, UseFrequency, USE_STEREO,
                           USE_LIN16, USE_DEFAULT_SPEAKER, START_IMMEDIATELY);
    if (audio_fd < 0)
    {
      fprintf(logfile, "Could not open HP-UX audio stream\n");
      closeAudio();
      return;
    }
    fcntl(audio_fd, F_SETFL, fcntl(audio_fd, F_GETFL) | O_NONBLOCK);
    fprintf(logfile, "using HP Alib 16bit linear stereo; ");
#else
    audio_fd = open("/dev/dsp", O_WRONLY);
    if (audio_fd<0)
    {
      fprintf(logfile, "Could not open /dev/dsp\n");
      return;
    }
    myioctl(audio_fd, SNDCTL_DSP_GETBLKSIZE, &i);
    /*printf("Block size %d, MixBufferSize %d\n", i, MixBufferSize);*/
    i = 11 | ((((2+SOUND_WRITEAHEAD_BUFFERS)*FrequencyMultiplier*MixBufferSize)/i)<<16);
    myioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &i);
    myioctl(audio_fd, SNDCTL_DSP_RESET, 0);

    i=UseFrequency;

    myioctl(audio_fd, SNDCTL_DSP_SPEED, &i);

    i=1;
    myioctl(audio_fd, SNDCTL_DSP_STEREO, &i);

    myioctl(audio_fd, SNDCTL_DSP_GETFMTS, &i);

    if (i&=AFMT_S16_LE)
      myioctl(audio_fd, SNDCTL_DSP_SETFMT, &i);
    else
      fprintf(logfile, "Could not play signed 16 data\n");
#endif	/* __sun */
#endif	/* __riscos__ */
  }
#endif	/* DOOM_NO_SFX */

  fprintf(logfile, " configured audio device, %dHz\n", UseFrequency );


  /* Initialize external data (all sounds) at start, keep static.*/
  fprintf( logfile, "I_InitSound: ");

  for (i=1 ; i<NUMSFX ; i++)
  {
    /* Alias? Example is the chaingun sound linked to pistol.*/
    if (!S_sfx[i].link)
    {
#ifdef DONT_PRECACHE
      /* just get the lump numbers. */
      S_sfx[i].lumpnum = getsfx( S_sfx[i].name, &lengths[i] );
#else
      /* Load data from WAD file.*/
      S_sfx[i].data = getsfx( S_sfx[i].name, &lengths[i] );
#endif
    }
    else
    {
#ifdef DONT_PRECACHE
      /* The offset calculation was wrong -- the divide by sizeof(sfxinfo_t) is */
      /* already done implicitly by subtracting two pointers of that type! */
      S_sfx[i].lumpnum = S_sfx[(S_sfx[i].link - S_sfx)].lumpnum;
#else
      /* Previously loaded already?*/
      S_sfx[i].data = S_sfx[i].link->data;
#endif
      lengths[i] = lengths[(S_sfx[i].link - S_sfx)];
    }
  }

#ifdef DONT_PRECACHE
  fprintf( logfile, " initialised all SFX lump numbers\n");
#else
  fprintf( logfile, " pre-cached all sound data\n");
#endif

#ifndef __riscos__
  /* Now initialize mixbuffer with zero.*/
  for ( i = 0; i< MixBufferSize; i++ )
    mixbuffer[i] = 0;
#endif

  if (SoundDisabled == 0)
  {
    /* Only start the sound tic _after_ the device was configured! */
#ifdef SNDINTR
    /*
   *  Set the interval between buffer fill calls to less than 100% the playtime
   *  to avoid buffer overflows. If the number of samples written out exceeds
   *  their playtime by a certain amount synchronize by not writing a buffer.
   */
#ifdef __sun
    sound_interval = ((950000*SampleCount) / UseFrequency);
#else
    /* need a little shorter interval on e.g. Linux */
    sound_interval = ((850000*SampleCount) / UseFrequency);
#endif
    fprintf( logfile, "I_SoundSetTimer: %d microsecs\n", sound_interval );
    I_SoundSetTimer( sound_interval );
#endif

#ifdef __hpux
    if (!M_CheckParm("-notimer"))
      I_HPStartAudioTimer();
#endif

    /* Finished initialization.*/
    fprintf(logfile, "I_InitSound: sound module ready\n");
  }

  SoundInitialized = 1;
}





/* MUSIC API.*/

#ifdef __hpux

static int nomusic = 0;
static int looping = 0;

void I_InitMusic(void)
{
  int genmidi_lump;

  if (M_CheckParm("-nomusic"))
  {
    nomusic = 1;
    return;
  }

  genmidi_lump = W_CheckNumForName("GENMIDI");
  if (genmidi_lump < 0)
  {
    fprintf(logfile, "I_InitMusic: missing GENMIDI lump!\n");
    nomusic = 1;
    return;
  }

  {
    int genmidi_len = W_LumpLength(genmidi_lump);
    const byte *genmidi_data = W_CacheLumpNum(genmidi_lump, PU_CACHE);
    HPMusic_Init(genmidi_data, genmidi_len);
  }

  if (!HPMusic_GenMidiOk())
  {
    fprintf(logfile, "I_InitMusic: GENMIDI init failed\n");
    nomusic = 1;
    return;
  }

  fprintf(logfile, "I_InitMusic: in-process OPL2 synth ready\n");
}

void I_ShutdownMusic(void)
{
  HPMusic_Stop();
}

void I_SetMusicVolume(int volume)
{
  snd_MusicVolume = volume;
  /* Doom's volume range is 0-15 (S_MAX_VOLUME); passed straight through. */
  HPMusic_SetVolume(volume);
}

void I_PlaySong(int handle, int loop)
{
  handle = 0;
  if (nomusic) return;
  looping = loop;
  HPMusic_Play(loop);
}

void I_PauseSong(int handle)
{
  handle = 0;
  if (nomusic) return;
  HPMusic_Pause();
}

void I_ResumeSong(int handle)
{
  handle = 0;
  if (nomusic) return;
  HPMusic_Resume();
}

void I_StopSong(int handle)
{
  handle = 0;
  looping = 0;
  if (nomusic) return;
  HPMusic_Stop();
}

void I_UnRegisterSong(int handle)
{
  handle = 0;
}

int I_RegisterSong(void *data, int length)
{
  /* Parse MUS header using byte access (avoids packed struct issues on
   * HP C in -Aa mode). MUS header layout (little-endian):
   *   [0-3] sig "MUS\x1A"  [4-5] scorelen  [6-7] scorestart
   *   [8-9] channels  [10-11] sec_channels  [12-13] instrumentcount
   */
  unsigned char *hdr = (unsigned char *)data;
  unsigned short scorelen, scorestart;
  int mus_len;

  if (nomusic) return 0;
  if (length < 16) return 0;

  scorelen   = (unsigned short)(hdr[4] | (hdr[5] << 8));
  scorestart = (unsigned short)(hdr[6] | (hdr[7] << 8));
  mus_len = (int)scorestart + (int)scorelen;

  if (mus_len < length)
  {
    /* Excess trailing data in the lump (Gady Kozma fix): correct the
     * header's scorelen so the parser reads all of it. */
    scorelen += (unsigned short)(length - mus_len);
    hdr[4] = (unsigned char)(scorelen & 0xFF);
    hdr[5] = (unsigned char)((scorelen >> 8) & 0xFF);
    mus_len = length;
  }
  else if (mus_len > length)
  {
    fprintf(logfile, "I_RegisterSong: incomplete MUS lump (%d < %d)\n", length, mus_len);
    return 0;
  }

  HPMusic_LoadSong(hdr, mus_len, looping);
  return 1;
}

int I_QrySongPlaying(int handle)
{
  handle = 0;
  return HPMusic_IsPlaying();
}

#else /* !__hpux : original MUSSERV-protocol external process */

static FILE *musserver = NULL;
static int nomusic = 0;

#ifdef MUSSERV
static void I_AbortMusic(int signal)
{
  nomusic = true;
}
#endif

void I_InitMusic (void)
{
#ifdef MUSSERV
  int genmidi_lump;

  if (M_CheckParm("-nomusic"))
  {
    nomusic = 1;
    return;
  }

  printf ("I_InitMusic: Starting music server.\n");

  signal (SIGCHLD, I_AbortMusic);
  signal (SIGPIPE, SIG_IGN);

  musserver = popen (MUSSERV, "w");
  if (!musserver)
  {
    printf ("I_InitMusic: failed to run linux music server\n");
    nomusic = true;
    return;
  }

  genmidi_lump = W_CheckNumForName ("GENMIDI");
  if (genmidi_lump < 0)
    printf ("I_InitMusic: missing GENMIDI lump!\n");
  else
  {
    int genmidi_len = W_LumpLength (genmidi_lump);
    const byte *genmidi_data = W_CacheLumpNum (genmidi_lump, PU_CACHE);
    fwrite (genmidi_data, genmidi_len, 1, musserver);
    fflush (musserver);
    if (ferror (musserver))
    {
      printf ("I_InitMusic: init FAILED\n");
      pclose (musserver);
      musserver = NULL;
      nomusic = true;
    }
  }
#endif
}

static void fflush_musserver (void)
{
  fflush (musserver);
  if (ferror (musserver))
  {
    fclose (musserver);
    musserver = NULL;
    nomusic = true;
  }
}

void I_ShutdownMusic(void)
{
#ifdef MUSSERV
  if (musserver)
  {
    fputc('Q', musserver);
    fflush_musserver ();
    wait (NULL);
  }
#endif
}

static int	looping=0;
static int	musicdies=-1;
/*static int mushandle = 0;*/

void I_SetMusicVolume(int volume)
{
  /* Internal state variable.*/
  snd_MusicVolume = volume;
  /* Now set volume on output device.*/
#ifdef MUSSERV
  if (musserver)
  {
    fprintf (musserver, "V%i R", volume);
    fflush_musserver ();
  }
#endif
}

void I_PlaySong(int handle, int loop)
{
#ifdef MUSSERV
  if (musserver)
  {
    fputc ('R', musserver);
    fflush_musserver ();
    looping = loop;
  }
#else
  musicdies = gametic + TICRATE*30;
#endif
}

void I_PauseSong (int handle)
{
#ifdef MUSSERV
  if (musserver)
  {
    fputc ('P', musserver);
    fflush_musserver ();
  }
#endif
}

void I_ResumeSong (int handle)
{
#ifdef MUSSERV
  if (musserver)
  {
    fputc ('R', musserver);
    fflush_musserver ();
  }
#endif
}

void I_StopSong(int handle)
{
#ifdef MUSSERV
  if (musserver)
  {
    fputc ('S', musserver);
    fflush_musserver ();
  }
#endif
  looping = 0;
  musicdies = 0;
}

void I_UnRegisterSong(int handle)
{
}

int I_RegisterSong(void* data, int length)
{
#ifdef MUSSERV
  /* Adapted from EDGE 1.27
   * (i.e. uses the length as specified by the data, not the lump) */
  if (musserver)
  {
    /* Parse MUS header using byte access (avoids packed struct issues on HP C).
     * MUS header layout (little-endian):
     *   [0-3]  sig         "MUS\x1A"
     *   [4-5]  scorelen    score section length
     *   [6-7]  scorestart  offset of score data from lump start
     *   [8-9]  channels
     *   [10-11] sec_channels
     *   [12-13] instrumentcount
     *   [14-15] pad
     */
    unsigned char *hdr = (unsigned char *)data;
    unsigned short scorelen, scorestart, instrumentcount;
    int mus_len;

    if (length < 16) return 0;
    scorelen      = (unsigned short)(hdr[4]  | (hdr[5]  << 8));
    scorestart    = (unsigned short)(hdr[6]  | (hdr[7]  << 8));
    instrumentcount = (unsigned short)(hdr[12] | (hdr[13] << 8));

    mus_len = (int)scorestart + (int)scorelen;

    if (mus_len < length)
    {
      printf ("I_RegisterSong: excess data in MUS lump "
	      "(%d > %d), correcting header\n", length, mus_len);
      /* Correct scorelen in header (Gady Kozma fix) */
      scorelen += (unsigned short)(length - mus_len);
      hdr[4] = (unsigned char)(scorelen & 0xFF);
      hdr[5] = (unsigned char)((scorelen >> 8) & 0xFF);
      mus_len = length;
    }
    else if (mus_len > length)
    {
      printf ("I_RegisterSong: incomplete MUS lump (%d < %d)\n",
	      length, mus_len);
      return 0;
    }

    clearerr (musserver);

    fprintf (musserver, "N%d\n", 1 /* FIXME: looping_current */ );
    fwrite (data, mus_len, 1, musserver);
    fflush (musserver);

    if (ferror (musserver))
    {
      printf ("I_RegisterSong: error transferring data.\n");
      fclose (musserver);
      musserver = NULL;
      return 0;
    }
  }
#endif
  return 1;
}

/* Is the song playing?*/
int I_QrySongPlaying(int handle)
{
  /* UNUSED.*/
  handle = 0;
  return looping || musicdies > gametic;
}
#endif /* __hpux */




#ifdef SNDINTR
/* Experimental stuff.*/
/* A Linux timer interrupt, for asynchronous*/
/*  sound output.*/
/* I ripped this out of the Timer class in*/
/*  our Difference Engine, including a few*/
/*  SUN remains...*/

#ifdef sun
    typedef     sigset_t        tSigSet;
#else
    typedef     int             tSigSet;
#endif


/* We might use SIGVTALRM and ITIMER_VIRTUAL, if the process*/
/*  time independend timer happens to get lost due to heavy load.*/
/* SIGALRM and ITIMER_REAL doesn't really work well.*/
/* There are issues with profiling as well.*/
#ifndef __riscos__
static int /*__itimer_which*/  itimer = ITIMER_REAL;
static int sig = SIGALRM;
#endif

/* Interrupt handler.*/
static void I_HandleSoundTimer( int ignore )
{
  static int inHandler = 0;

  /* Crude reentrancy guard (not atomic, will not work in extreme situations) */
  if (inHandler++ == 0)
  {
  /* See I_SubmitSound().*/
  /* Write it to DSP device.*/
#ifndef __riscos__
    int 			blocks = 0;
#if defined(__sun) || defined(__hpux)
    /* Synchronize via the timer; somewhat crude, but works OK on Solaris/HP-UX*/
    struct timeval	tp;
    struct timezone	tzp;
    int			need_samples;
    float			usecs;

    gettimeofday(&tp, &tzp);
    usecs = (float)(tp.tv_usec - last_sync_time.tv_usec);
    need_samples = (tp.tv_sec - last_sync_time.tv_sec) * UseFrequency +
      (int)((usecs * UseFrequency) / 1000000);
    /*printf("written %d, need: %d\n", samples_written, need_samples);*/
    /* Can we skip the next buffer? */
    if ((samples_written - need_samples) >= SOUND_WRITEAHEAD_BUFFERS*SampleCount)
    {
      samples_written = (samples_written - need_samples);
      last_sync_time.tv_sec = tp.tv_sec; last_sync_time.tv_usec = tp.tv_usec;
      /*printf("skip\n");*/
    }
    else
    {
      blocks = 1;
      /* In case we have an underflow at some time make sure sound can recover */
      if ((samples_written - need_samples) < 0)
      {
	/*printf("Buffer underflow!\n"); fflush(stdout);*/
	samples_written = need_samples;
	blocks++;
      }
      /*printf("blocks %d\n", blocks);*/
    }
#else
    /* Synchronize via the available buffer space; Linux may freeze us if it overflows, so careful! */
    audio_buf_info ainfo;
    int restSpace;

    myioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &ainfo);
    /*printf("Frags %d/%d: %d\n", ainfo.fragments, ainfo.fragstotal, ainfo.fragsize);*/
    restSpace = ainfo.fragments * ainfo.fragsize;
    while (restSpace >= MixBufferSize)
    {
      blocks++;
      restSpace -= MixBufferSize;
    }
#endif

    /* synthesize */
    while (blocks > 0)
    {
      I_UpdateSound();
      write(audio_fd, mixbuffer, MixBufferSize);
      samples_written += SampleCount;
      blocks--;
    }
#endif

    inHandler = 0;
    /* UNUSED, but required.*/
    ignore = 0;
  }
}

/* Get the interrupt. Set duration in millisecs.*/
static int I_SoundSetTimer( int duration_of_tick )
{
#ifdef __riscos__
  return 0;
#else
  /* Needed for gametick clockwork.*/
  struct itimerval    value;
  struct itimerval    ovalue;
  struct sigaction    act;
  struct sigaction    oact;
  struct timezone     tzp;
  int res;

  /* This sets to SA_ONESHOT and SA_NOMASK, thus we can not use it.*/
  /*     signal( _sig, handle_SIG_TICK );*/

  /* Now we have to change this attribute for repeated calls.*/
  act.sa_handler = I_HandleSoundTimer;
#ifndef sun
  /*ac	t.sa_mask = _sig;*/
#endif
  act.sa_flags = SA_RESTART;

  sigaction( sig, &act, &oact );

  value.it_interval.tv_sec    = 0;
  value.it_interval.tv_usec   = duration_of_tick;
  value.it_value.tv_sec       = 0;
  value.it_value.tv_usec      = duration_of_tick;

  samples_written = 0;
  gettimeofday(&last_sync_time, &tzp);

  /* Error is -1.*/
  res = setitimer( itimer, &value, &ovalue );

  /* Debug.*/
  if ( res == -1 )
    fprintf( logfile, "I_SoundSetTimer: interrupt n.a.\n");

  return res;
#endif
}


/* Remove the interrupt. Set duration to zero.*/
static void I_SoundDelTimer(void)
{
  /* Debug.*/
  if ( I_SoundSetTimer( 0 ) == -1)
    fprintf( logfile, "I_SoundDelTimer: failed to remove interrupt. Doh!\n");
}
#endif
