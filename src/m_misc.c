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
/*	Main loop menu stuff.*/
/*	Default Config File.*/
/*	PCX Screenshots.*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: m_misc.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>

#ifdef __riscos__
#include "ROsupport.h"
#endif

#include <ctype.h>


#include "doomdef.h"
#include "m_fixed.h"

#include "z_zone.h"
#include "g_game.h"

#include "m_swap.h"
#include "m_argv.h"
#include "m_menu.h"

#include "w_wad.h"

#include "i_system.h"
#include "i_video.h"
#include "i_sound.h"
#include "v_video.h"
#include "s_sound.h"

#include "hu_stuff.h"
#include "am_map.h"

/* State.*/
#include "doomstat.h"

/* Data.*/
#include "dstrings.h"

#include "m_misc.h"


/* M_DrawText*/
/* Returns the final X coordinate*/
/* HU_Init must have been called to init the font*/

int
M_DrawText
( int		x,
  int		y,
  boolean	direct,
  const char*	string )
{
    int 	c;
    int		w;

    while (*string)
    {
	c = toupper(*string) - HU_FONTSTART;
	string++;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    x += 4;
	    continue;
	}

	w = SHORT (hu_font[c]->width);
	if (x+w > SCREENWIDTH)
	    break;
	if (direct)
	    V_DrawPatchDirect(x, y, 0, hu_font[c]);
	else
	    V_DrawPatch(x, y, 0, hu_font[c]);
	x+=w;
    }

    return x;
}





/* M_WriteFile*/

boolean
M_WriteFile
( const char*	name,
  const void*	source,
  int		length )
{
    FILE        *handle;
    int         count;

    handle = fopen(name, "wb");

    if (handle == NULL)
        return -1;
    count = fwrite(source, 1, length, handle);
    fclose(handle);

    if (count < length)
	return false;

    return true;
}



/* M_ReadFile*/

int
M_ReadFile
( const char*	name,
  byte**	buffer )
{
    FILE *handle;
    int count, length;
    byte *buf;

    handle = fopen (name, "rb");
    if (handle == NULL)
        I_Error ("Couldn't read file %s", name);
    fseek (handle, 0, SEEK_END);
    length = (int) ftell (handle);
    fseek (handle, 0, SEEK_SET);
    buf = Z_Malloc (length, PU_STATIC, NULL);
    count = fread ((void*)buf, 1, length, handle);
    fclose (handle);

    if (count < length)
	I_Error ("Couldn't read file %s", name);

    *buffer = buf;
    return length;
}



/* DEFAULTS*/

static int	usemouse;
static int	usejoystick;


#ifdef LINUX
static char*	mousetype;
static char*	mousedev;
#endif



typedef struct
{
    const char*	name;
    int		isstring;
    union	{int *i; char **s;} location;
    union	{int  i; char  *s;} defaultvalue;
    int		scantranslate;		/* PC scan code hack*/
    int		untranslated;		/* lousy hack*/
} default_t;

/* the location and defaultvalue members are not initialised in M_InitMiscData */
static default_t	defaults[] =
{
    {"mouse_sensitivity", 0},
    {"sfx_volume", 0},
    {"music_volume", 0},
    {"show_messages", 0},
    {"old_savegames", 0},

    /* game keys */
    {"key_right", 0},
    {"key_left", 0},
    {"key_up", 0},
    {"key_down", 0},
    {"key_strafeleft", 0},
    {"key_straferight", 0},

    {"key_fire", 0},
    {"key_use", 0},
    {"key_strafe", 0},
    {"key_speed", 0},
    {"key_jump", 0},

    /* map keys */
    {"mkey_pandown", 0},
    {"mkey_panup", 0},
    {"mkey_panright", 0},
    {"mkey_panleft", 0},
    {"mkey_zoomin", 0},
    {"mkey_zoomout", 0},
    {"mkey_activate", 0},
    {"mkey_altmap", 0},
    {"mkey_gobig", 0},
    {"mkey_follow", 0},
    {"mkey_grid", 0},
    {"mkey_mark", 0},
    {"mkey_clear", 0},
    
/* UNIX hack, to be removed. */
#ifdef NORMALUNIX
#ifdef SNDSERV
    {"sndserver", 1},
    {"mb_used", 0},
#endif

#endif

#ifdef LINUX
    {"mousedev", 1},
    {"mousetype", 1},
#endif

#ifdef __riscos__	/* Essentially we can use the same keys as LINUX... */
    {"hardware_gamma", 0},
    {"buffered_keys", 0},
    {"16bit_sound", 0},
    {"insthandflags", 0},
#endif

    {"use_mouse", 0},
    {"mouseb_fire", 0},
    {"mouseb_strafe", 0},
    {"mouseb_forward", 0},

    {"use_joystick", 0},
    {"joyb_fire", 0},
    {"joyb_strafe", 0},
    {"joyb_use", 0},
    {"joyb_speed", 0},

    {"screenblocks", 0},
    {"detaillevel", 0},
    {"minlightlevel", 0},

    {"snd_channels", 0},
    {"fadouttics", 0},
    {"freq_mult", 0},
    {"resamp_sound", 0},

    {"usegamma", 0},

    {"chatmacro0", 1},
    {"chatmacro1", 1},
    {"chatmacro2", 1},
    {"chatmacro3", 1},
    {"chatmacro4", 1},
    {"chatmacro5", 1},
    {"chatmacro6", 1},
    {"chatmacro7", 1},
    {"chatmacro8", 1},
    {"chatmacro9", 1},

    {"def_spechit", 0},
    {"max_spechit", 0},
    {"def_intercepts", 0},
    {"max_intercepts", 0},
    {"def_linespec", 0},
    {"max_linespec", 0},
    {"def_visplanes", 0},
    {"max_visplanes", 0},
    {"def_drawsegs", 0},
    {"max_drawsegs", 0},
    {"def_vissprites", 0},
    {"max_vissprites", 0},
    {"def_ceilings", 0},
    {"max_ceilings", 0},
    {"def_platforms", 0},
    {"max_platforms", 0}
};

static int	numdefaults;
static char*	defaultfile;



/* M_SaveDefaults*/

void M_SaveDefaults (void)
{
    int		i;
    FILE*	f;

    f = fopen (defaultfile, "w");
    if (!f)
	return; /* can't write the file, but don't complain*/

    for (i=0 ; i<numdefaults ; i++)
    {
        if (defaults[i].isstring == 0)
	{
	    fprintf (f,"%s\t\t%i\n", defaults[i].name, *(defaults[i].location.i));
	} else {
	    fprintf (f,"%s\t\t\"%s\"\n",defaults[i].name, *(defaults[i].location.s));
	}
    }

    fclose (f);
}



/* M_LoadDefaults*/

/*extern byte	scantokey[128];*/

void M_LoadDefaults (void)
{
    int		i;
    FILE*	f;
    char	buffer[256];
    char*	newstring = NULL;

    /* set everything to base values*/
    numdefaults = sizeof(defaults)/sizeof(defaults[0]);
    for (i=0 ; i<numdefaults ; i++)
    {
        if (defaults[i].isstring == 0)
	    *(defaults[i].location.i) = defaults[i].defaultvalue.i;
        else
	    *(defaults[i].location.s) = defaults[i].defaultvalue.s;
    }

    /* check for a custom default file*/
    i = M_CheckParm ("-config");
    if (i && i<myargc-1)
    {
	defaultfile = myargv[i+1];
	printf ("	default file: %s\n",defaultfile);
    }
    else
	defaultfile = basedefault;

    /* read the file in, overriding any set defaults*/
    f = fopen (defaultfile, "r");
    if (f)
    {
        buffer[255] = '\n';
	while (!feof(f))
	{
	    default_t *dptr;
	    char *def;
	    char *b;

	    if (fgets(buffer, 255, f) == NULL) break;

	    def = buffer;
	    while (isspace((unsigned int)(*def))) def++;
	    b = def;
	    while (!isspace((unsigned int)(*b))) b++;
	    *b++ = '\0';
	    for (i=0, dptr=defaults; i<numdefaults; i++, dptr++)
	    {
	        if (strcmp(dptr->name, def) == 0) break;
	    }
	    if (i >= numdefaults)
	    {
	        fprintf(logfile, "Warning, unknown config value %s\n", def);
	    }
	    else
	    {
	        while (isspace((unsigned int)(*b))) b++;
	        if (dptr->isstring == 0)
	        {
	            char *rest;
	            int val;
	            int base = 10;

	            if ((b[0] == '0') && (b[1] == 'x'))
	            {
	                b += 2; base = 16;
	            }
	            val = strtol(b, &rest, base);
	            if (rest == b)
	            {
	                fprintf(logfile, "Error parsing config value %s\n", def);
	            }
	            else
	            {
	                *(dptr->location.i) = val;
	            }
	        }
	        else
	        {
	            if (*b != '\"')
	            {
	                fprintf(logfile, "Error, config value %s is a string\n", def);
	            }
	            else
	            {
	                char *start = ++b;

	                while ((*b != '\0') && (*b != '\"')) b++;
	                *b = '\0';
	                if ((newstring = (char*)malloc((b-start)+1)) == NULL)
	                    I_Error("M_LoadDefaults: not enough memory");

	                strcpy(newstring, start);

	                *(dptr->location.s) = newstring;
	            }
	        }
	    }
	}

	fclose (f);
    }
}



/* SCREEN SHOTS*/



typedef struct
{
    char		manufacturer;
    char		version;
    char		encoding;
    char		bits_per_pixel;

    unsigned short	xmin;
    unsigned short	ymin;
    unsigned short	xmax;
    unsigned short	ymax;

    unsigned short	hres;
    unsigned short	vres;

    unsigned char	palette[48];

    char		reserved;
    char		color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;

    char		filler[58];
    unsigned char	data;		/* unbounded*/
} pcx_t;



/* WritePCXfile*/

void
WritePCXfile
( const char*	filename,
  const byte*	data,
  int		width,
  int		height,
  const byte*	palette )
{
    int		i;
    int		length;
    pcx_t*	pcx;
    byte*	pack;

    pcx = Z_Malloc (width*height*2+1000, PU_STATIC, NULL);

    pcx->manufacturer = 0x0a;		/* PCX id*/
    pcx->version = 5;			/* 256 color*/
    pcx->encoding = 1;			/* uncompressed*/
    pcx->bits_per_pixel = 8;		/* 256 color*/
    pcx->xmin = 0;
    pcx->ymin = 0;
    pcx->xmax = SHORT(width-1);
    pcx->ymax = SHORT(height-1);
    pcx->hres = SHORT(width);
    pcx->vres = SHORT(height);
    memset (pcx->palette,0,sizeof(pcx->palette));
    pcx->color_planes = 1;		/* chunky image*/
    pcx->bytes_per_line = SHORT(width);
    pcx->palette_type = SHORT(2);	/* not a grey scale*/
    memset (pcx->filler,0,sizeof(pcx->filler));


    /* pack the image*/
    pack = &pcx->data;

    for (i=0 ; i<width*height ; i++)
    {
	if ( (*data & 0xc0) != 0xc0)
	    *pack++ = *data++;
	else
	{
	    *pack++ = 0xc1;
	    *pack++ = *data++;
	}
    }

    /* write the palette*/
    *pack++ = 0x0c;	/* palette ID byte*/
    for (i=0 ; i<768 ; i++)
	*pack++ = *palette++;

    /* write output file*/
    length = pack - (byte *)pcx;
    M_WriteFile (filename, pcx, length);

    Z_Free (pcx);
}



/* M_ScreenShot*/

void M_ScreenShot (void)
{
    int		i;
    pixel_t*	linear;
    char	lbmname[12];
    int		numoff;

    /* munge planar buffer to linear*/
    linear = screens[2];
    I_ReadScreen (linear);

    /* find a file name to save it to*/
#ifdef __riscos__
    strcpy(lbmname,"Doom:DOOM00/pcx");
    numoff = 9;
#else
    strcpy(lbmname,"DOOM00.pcx");
    numoff = 4;
#endif

    for (i=0 ; i<=99 ; i++)
    {
	lbmname[numoff]  = i/10 + '0';
	lbmname[numoff+1] = i%10 + '0';
	if (access(lbmname,0) == -1)
	    break;	/* file doesn't exist*/
    }
    if (i==100)
	I_Error ("M_ScreenShot: Couldn't create a PCX");

    /* save the pcx file*/
    WritePCXfile (lbmname, (byte*)linear,
		  SCREENWIDTH, SCREENHEIGHT,
		  W_CacheLumpName ("PLAYPAL",PU_CACHE));

    players[consoleplayer].message = "screen shot";
}



/* for strict ANSI conformity */
void M_InitMiscData(void)
{
    default_t*	def;

    def = defaults;
    /* This has to be exactly the same structure as the defaults - array */
    def->location.i = &mouseSensitivity; def->defaultvalue.i = 5; def++;
    def->location.i = &snd_SfxVolume; def->defaultvalue.i = 8; def++;
    def->location.i = &snd_MusicVolume; def->defaultvalue.i = 8; def++;
    def->location.i = &showMessages; def->defaultvalue.i = 1; def++;
    def->location.i = &oldsavegames; def->defaultvalue.i = 0; def++;
    def->location.i = &key_right; def->defaultvalue.i = KEY_RIGHTARROW; def++;
    def->location.i = &key_left; def->defaultvalue.i = KEY_LEFTARROW; def++;
    def->location.i = &key_up; def->defaultvalue.i = KEY_UPARROW; def++;
    def->location.i = &key_down; def->defaultvalue.i = KEY_DOWNARROW; def++;
    def->location.i = &key_strafeleft; def->defaultvalue.i = ','; def++;
    def->location.i = &key_straferight; def->defaultvalue.i = '.'; def++;
    def->location.i = &key_fire; def->defaultvalue.i = KEY_RCTRL; def++;
    def->location.i = &key_use; def->defaultvalue.i = ' '; def++;
    def->location.i = &key_strafe; def->defaultvalue.i = KEY_RALT; def++;
    def->location.i = &key_speed; def->defaultvalue.i = KEY_RSHIFT; def++;
    def->location.i = &key_jump; def->defaultvalue.i = '0'; def++;

    def->location.i = &mapkey_pandown; def->defaultvalue.i = KEY_DOWNARROW; def++;
    def->location.i = &mapkey_panup; def->defaultvalue.i = KEY_UPARROW; def++;
    def->location.i = &mapkey_panright; def->defaultvalue.i = KEY_RIGHTARROW; def++;
    def->location.i = &mapkey_panleft; def->defaultvalue.i = KEY_LEFTARROW; def++;
    def->location.i = &mapkey_zoomin; def->defaultvalue.i = '='; def++;
    def->location.i = &mapkey_zoomout; def->defaultvalue.i = '-'; def++;
    def->location.i = &mapkey_activate; def->defaultvalue.i = KEY_TAB; def++;
    def->location.i = &mapkey_altmap; def->defaultvalue.i = '`'; def++;
    def->location.i = &mapkey_gobig; def->defaultvalue.i = '0'; def++;
    def->location.i = &mapkey_follow; def->defaultvalue.i = 'f'; def++;
    def->location.i = &mapkey_grid; def->defaultvalue.i = 'g'; def++;
    def->location.i = &mapkey_mark; def->defaultvalue.i = 'm'; def++;
    def->location.i = &mapkey_clearmark; def->defaultvalue.i = 'c'; def++;
    
#ifdef NORMALUNIX
#ifdef SNDSERV
    def->location.s = &sndserver_filename; def->defaultvalue.s = "sndserver"; def++;
    def->location.i = &mb_used; def->defaultvalue.i = 2;
#endif
#endif
#ifdef LINUX
    def->location.s = &mousedev; def->defaultvalue.s = "/dev/ttyS0"; def++;
    def->location.s = &mousetype; def->defaultvalue.s = "microsoft"; def++;
#endif
#ifdef __riscos__
    def->location.i = &HardwareGamma; def->defaultvalue.i = 0; def++;
    def->location.i = &BufferedKeyboard; def->defaultvalue.i = 0; def++;
    def->location.i = &SixteenBitSound; def->defaultvalue.i = 0; def++;
    def->location.i = &InstHandlerFlags; def->defaultvalue.i = IHFlag_Exit | IHFlag_Abort | IHFlag_StFix; def++;
#endif
    def->location.i = &usemouse; def->defaultvalue.i = 1; def++;
    def->location.i = &mousebfire; def->defaultvalue.i = 0; def++;
    def->location.i = &mousebstrafe; def->defaultvalue.i = 1; def++;
    def->location.i = &mousebforward; def->defaultvalue.i = 2; def++;
    def->location.i = &usejoystick; def->defaultvalue.i = 0; def++;
    def->location.i = &joybfire; def->defaultvalue.i = 0; def++;
    def->location.i = &joybstrafe; def->defaultvalue.i = 1; def++;
    def->location.i = &joybuse; def->defaultvalue.i = 3; def++;
    def->location.i = &joybspeed; def->defaultvalue.i = 2; def++;
    def->location.i = &screenblocks; def->defaultvalue.i = 9; def++;
    def->location.i = &detailLevel; def->defaultvalue.i = 0; def++;
    def->location.i = &MinLightLevel; def->defaultvalue.i = 32; def++;
    def->location.i = &numChannels; def->defaultvalue.i = 8; def++; /* used to be 3 */
    def->location.i = &FadeoutTics; def->defaultvalue.i = 2*TICRATE; def++;
    def->location.i = &FrequencyMultiplier; def->defaultvalue.i = 1; def++;
    def->location.i = &ResampleSound; def->defaultvalue.i = 0; def++;
    def->location.i = &usegamma; def->defaultvalue.i = 0; def++;
    def->location.s = &chat_macros[0]; def->defaultvalue.s = HUSTR_CHATMACRO0; def++;
    def->location.s = &chat_macros[1]; def->defaultvalue.s = HUSTR_CHATMACRO1; def++;
    def->location.s = &chat_macros[2]; def->defaultvalue.s = HUSTR_CHATMACRO2; def++;
    def->location.s = &chat_macros[3]; def->defaultvalue.s = HUSTR_CHATMACRO3; def++;
    def->location.s = &chat_macros[4]; def->defaultvalue.s = HUSTR_CHATMACRO4; def++;
    def->location.s = &chat_macros[5]; def->defaultvalue.s = HUSTR_CHATMACRO5; def++;
    def->location.s = &chat_macros[6]; def->defaultvalue.s = HUSTR_CHATMACRO6; def++;
    def->location.s = &chat_macros[7]; def->defaultvalue.s = HUSTR_CHATMACRO7; def++;
    def->location.s = &chat_macros[8]; def->defaultvalue.s = HUSTR_CHATMACRO8; def++;
    def->location.s = &chat_macros[9]; def->defaultvalue.s = HUSTR_CHATMACRO9; def++;

    /* defaults for dynamic structures */
    def->location.i = &default_spechit; def->defaultvalue.i = 32; def++;
    def->location.i = &maximum_spechit; def->defaultvalue.i = 0; def++;
    def->location.i = &default_intercepts; def->defaultvalue.i = 32; def++;
    def->location.i = &maximum_intercepts; def->defaultvalue.i = 0; def++;
    def->location.i = &default_linespecials; def->defaultvalue.i = 64; def++;
    def->location.i = &maximum_linespecials; def->defaultvalue.i = 0; def++;
    def->location.i = &default_visplanes; def->defaultvalue.i = 64; def++;
    def->location.i = &maximum_visplanes; def->defaultvalue.i = 192; def++;
    def->location.i = &default_drawsegs; def->defaultvalue.i = 256; def++;
    def->location.i = &maximum_drawsegs; def->defaultvalue.i = 1024; def++;
    def->location.i = &default_vissprites; def->defaultvalue.i = 64; def++;
    def->location.i = &maximum_vissprites; def->defaultvalue.i = 256; def++;
    def->location.i = &default_ceilings; def->defaultvalue.i = 32; def++;
    def->location.i = &maximum_ceilings; def->defaultvalue.i = 0; def++;
    def->location.i = &default_platforms; def->defaultvalue.i = 32; def++;
    def->location.i = &maximum_platforms; def->defaultvalue.i = 0; def++;
}
