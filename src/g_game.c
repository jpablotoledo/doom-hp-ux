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

/* DESCRIPTION:  none*/

/*-----------------------------------------------------------------------------*/


static const char
rcsid[] = "$Id: g_game.c,v 1.8 1997/02/03 22:45:09 b1 Exp $";

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "doomdef.h"
#include "doomstat.h"

#include "z_zone.h"
#include "f_finale.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "i_system.h"

#include "p_local.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "p_saven.h"
#include "p_tick.h"

#include "d_main.h"

#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

/* Needs access to LFB.*/
#include "v_video.h"

#include "w_wad.h"

#include "p_local.h"

#include "s_sound.h"

/* Data.*/
#include "dstrings.h"
#include "sounds.h"

/* SKY handling - still the wrong place.*/
#include "r_data.h"
#include "r_sky.h"



#include "g_game.h"


#ifdef DIYINLINE
#include "inl_pside.c"
#include "inl_psub.c"
#endif


#define SAVESTRINGSIZE	24



static void	G_DoLoadGame (void);

static void	G_ReadDemoTiccmd (ticcmd_t* cmd);
static void	G_WriteDemoTiccmd (ticcmd_t* cmd);
static void	G_DoReborn (int playernum);

static void	G_DoLoadLevel (void);
static void	G_DoNewGame (void);
static void	G_DoPlayDemo (void);
static void	G_DoCompleted (void);
static void	G_DoWorldDone (void);
static void	G_DoSaveGame (void);


gameaction_t    gameaction;
gamestate_t     gamestate;
skill_t         gameskill;
boolean		respawnmonsters;
int             gameepisode;
int             gamemap;

boolean         paused;
boolean         sendpause;             	/* send a pause event next tic */
static boolean  sendsave;             	/* send a save event next tic */
boolean         usergame;               /* ok to save / end game */

static boolean  timingdemo;             /* if true, exit with report on completion */
boolean		nodrawers;              /* for comparative timing purposes */
boolean		noblit;			/* for comparative timing purposes */
static int      starttime;          	/* for comparative timing purposes  	 */

boolean         viewactive;

boolean         deathmatch;           	/* only if started as net death */
boolean         netgame;                /* only true if packets are broadcast */
boolean         playeringame[MAXPLAYERS];
player_t        players[MAXPLAYERS];

int             consoleplayer;          /* player taking events and displaying */
int             displayplayer;          /* view being displayed */
int             gametic;
int		levelstarttic;          /* gametic at level start */
int             totalkills, totalitems, totalsecret;    /* for intermission */

static char     demoname[64];
boolean         demorecording;
boolean         demoplayback;
boolean		netdemo;
static byte*	demobuffer;
static byte*	demo_p;
static byte*	demoend;
boolean         singledemo;            	/* quit after playing a demo from cmdline */

boolean         precache = true;        /* if true, load all graphics at start */

wbstartstruct_t wminfo;               	/* parms for world map / intermission */

static short	consistancy[MAXPLAYERS][BACKUPTICS];

byte*		savebuffer;



/* controls (have defaults) */

int		oldsavegames;		/* old (unportable savegames) */

int             key_right;
int		key_left;

int		key_up;
int		key_down;
int             key_strafeleft;
int		key_straferight;
int             key_fire;
int		key_use;
int		key_strafe;
int		key_speed;
int		key_jump;
/* Those are cheats, so just use fixed keys... */
#define key_floatup	'\''
#define key_floatdown	'/'
/* This one isn't a cheat as such... */
#define key_reverse	']'

int             mousebfire;
int             mousebstrafe;
int             mousebforward;

int             joybfire;
int             joybstrafe;
int             joybuse;
int             joybspeed;



#define MAXPLMOVE		(forwardmove[1])

#define TURBOTHRESHOLD	0x32

fixed_t		forwardmove[2] = {0x19, 0x32};
fixed_t		sidemove[2] = {0x18, 0x28};
fixed_t		angleturn[3] = {640, 1280, 320};	/* + slow turn */

#define SLOWTURNTICS	6
#define QUICKREVERSE 32768 /* 180 degree reverse */

#define NUMKEYS		256

static boolean  gamekeydown[NUMKEYS];
static int      turnheld;				/* for accelerative turning */

static boolean	mousearray[4];
static boolean*	mousebuttons = &mousearray[1];		/* allow [-1]*/

/* mouse values are used once */
static int	mousex;
static int	mousey;

static int	dclicktime;
static int	dclickstate;
static int	dclicks;
static int	dclicktime2;
static int	dclickstate2;
static int	dclicks2;

/* joystick values are repeated */
static int	joyxmove;
static int	joyymove;
static boolean	joyarray[5];
static boolean*	joybuttons = &joyarray[1];		/* allow [-1] */

static int	savegameslot;
static char	savedescription[32];


#define	BODYQUESIZE	32

mobj_t*		bodyque[BODYQUESIZE];
int		bodyqueslot = 0;

void*		statcopy;				/* for statistics driver*/


/* Does DeHackEd need these too? */
#define GGAME_SKY1	0
#define GGAME_SKY2	1
#define GGAME_SKY3	2
#define GGAME_SKY4	3

char *ggame_lumps[GGAME_NUMBER] = {
  "SKY1",
  "SKY2",
  "SKY3",
  "SKY4"
};

/* Demo is from version... */
static int DemoVersionNumber;

int InitHealth = 100;
int InitAmmo = 50;




#if 0
static int G_CmdChecksum (ticcmd_t* cmd)
{
    int		i;
    int		sum = 0;

    for (i=0 ; i< sizeof(*cmd)/4 - 1 ; i++)
	sum += ((int *)cmd)[i];

    return sum;
}


/* G_InitPlayer */
/* Called at the start.*/
/* Called by the game initialization functions.*/

static void G_InitPlayer (int player)
{
    player_t*	p;

    /* set up the saved info         */
    p = &players[player];

    /* clear everything else to defaults */
    G_PlayerReborn (player);

}
#endif




/* G_BuildTiccmd*/
/* Builds a ticcmd from all of the available inputs*/
/* or reads it from the demo buffer. */
/* If recording a demo, write it out */

void G_BuildTiccmd (ticcmd_t* cmd)
{
    int		i;
    boolean	strafe;
    boolean	bstrafe;
    int		speed;
    int		tspeed;
    int		forward;
    int		side;

    ticcmd_t*	base;

    base = I_BaseTiccmd ();		/* empty, or external driver*/
    memcpy (cmd,base,sizeof(*cmd));

    cmd->consistancy =
	consistancy[consoleplayer][maketic%BACKUPTICS];


    strafe = gamekeydown[key_strafe] || mousebuttons[mousebstrafe]
	|| joybuttons[joybstrafe];
    speed = gamekeydown[key_speed] || joybuttons[joybspeed];

    forward = side = 0;

    /* use two stage accelerative turning*/
    /* on the keyboard and joystick*/
    if (joyxmove < 0
	|| joyxmove > 0
	|| gamekeydown[key_right]
	|| gamekeydown[key_left])
	turnheld += ticdup;
    else
	turnheld = 0;

    if (turnheld < SLOWTURNTICS)
	tspeed = 2;             /* slow turn */
    else
	tspeed = speed;

#ifdef DIYBOOM
    if (gamekeydown[key_reverse])
    {
	cmd->angleturn += QUICKREVERSE;
	gamekeydown[key_reverse] = false;
    }
#endif

    /* let movement keys cancel each other out*/
    if (strafe)
    {
	if (gamekeydown[key_right])
	{
	    side += sidemove[speed];
	}
	if (gamekeydown[key_left])
	{
	    side -= sidemove[speed];
	}
	if (joyxmove > 0)
	    side += sidemove[speed];
	if (joyxmove < 0)
	    side -= sidemove[speed];

    }
    else
    {
	if (gamekeydown[key_right])
	    cmd->angleturn -= angleturn[tspeed];
	if (gamekeydown[key_left])
	    cmd->angleturn += angleturn[tspeed];
	if (joyxmove > 0)
	    cmd->angleturn -= angleturn[tspeed];
	if (joyxmove < 0)
	    cmd->angleturn += angleturn[tspeed];
    }

    if (gamekeydown[key_up])
    {
	forward += forwardmove[speed];
    }
    if (gamekeydown[key_down])
    {
	forward -= forwardmove[speed];
    }
    if (joyymove < 0)
	forward += forwardmove[speed];
    if (joyymove > 0)
	forward -= forwardmove[speed];
    if (gamekeydown[key_straferight])
	side += sidemove[speed];
    if (gamekeydown[key_strafeleft])
	side -= sidemove[speed];

    /* buttons*/
    cmd->chatchar = HU_dequeueChatChar();

    if (gamekeydown[key_fire] || mousebuttons[mousebfire]
	|| joybuttons[joybfire])
	cmd->buttons |= BT_ATTACK;

    if (gamekeydown[key_use] || joybuttons[joybuse] )
    {
	cmd->buttons |= BT_USE;
	/* clear double clicks if hit use button */
	dclicks = 0;
    }

    if (!demoplayback)
    {
#ifdef FULL_NEW_FEATURES
#define STORE_VACCEL_IN		cmd->vaccel
	cmd->newflags = 0;
	if (jump_cheat) cmd->newflags |= NEW_FEATURE_JUMP;
	if (float_cheat) cmd->newflags |= NEW_FEATURE_FLOAT;
	if (homing_missiles) cmd->newflags |= NEW_FEATURE_HOMING;
#else
#define STORE_VACCEL_IN		vertical_acceleration
#endif
	/* Jump/Float? */
	STORE_VACCEL_IN = 0;
	/* Float overrides jump */
	if (float_cheat)
	{
	    if (gamekeydown[key_floatup])
	    {
		STORE_VACCEL_IN = (gamekeydown[key_speed]) ? 2 : 1;
	    }
	    if (gamekeydown[key_floatdown])
	    {
		STORE_VACCEL_IN = (gamekeydown[key_speed]) ? -2 : -1;
	    }
	}
	else if (jump_cheat)
	{
	    if (gamekeydown[key_jump])
	    {
		/* Set the acceleration here */
		STORE_VACCEL_IN = (gamekeydown[key_speed]) ? 16 : 12;
	    }
	}
    }

    /* chainsaw overrides */
#ifdef DIYBOOM
    i = gamekeydown['1'] ? wp_fist :
	gamekeydown['2'] ? wp_pistol :
	gamekeydown['3'] ? wp_shotgun :
	gamekeydown['4'] ? wp_chaingun :
	gamekeydown['5'] ? wp_missile :
	gamekeydown['6'] && gamemode != shareware ? wp_plasma :
	gamekeydown['7'] && gamemode != shareware ? wp_bfg :
	gamekeydown['8'] ? wp_chainsaw :
	gamekeydown['9'] && gamemode == commercial ? wp_supershotgun :
	wp_nochange;
    if (i != wp_nochange)
    {
	cmd->buttons |= BT_CHANGE;
	cmd->buttons |= i<<BT_WEAPONSHIFT;
    }
#else
    for (i=0 ; i<NUMWEAPONS-1 ; i++)
	if (gamekeydown['1'+i])
	{
	    cmd->buttons |= BT_CHANGE;
	    cmd->buttons |= i<<BT_WEAPONSHIFT;
	    break;
	}
#endif

    /* mouse*/
    if (mousebuttons[mousebforward])
	forward += forwardmove[speed];

    /* forward double click*/
    if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1 )
    {
	dclickstate = mousebuttons[mousebforward];
	if (dclickstate)
	    dclicks++;
	if (dclicks == 2)
	{
	    cmd->buttons |= BT_USE;
	    dclicks = 0;
	}
	else
	    dclicktime = 0;
    }
    else
    {
	dclicktime += ticdup;
	if (dclicktime > 20)
	{
	    dclicks = 0;
	    dclickstate = 0;
	}
    }

    /* strafe double click*/
    bstrafe =
	mousebuttons[mousebstrafe]
	|| joybuttons[joybstrafe];
    if (bstrafe != dclickstate2 && dclicktime2 > 1 )
    {
	dclickstate2 = bstrafe;
	if (dclickstate2)
	    dclicks2++;
	if (dclicks2 == 2)
	{
	    cmd->buttons |= BT_USE;
	    dclicks2 = 0;
	}
	else
	    dclicktime2 = 0;
    }
    else
    {
	dclicktime2 += ticdup;
	if (dclicktime2 > 20)
	{
	    dclicks2 = 0;
	    dclickstate2 = 0;
	}
    }

    forward += mousey;
    if (strafe)
	side += mousex*2;
    else
	cmd->angleturn -= mousex*0x8;

    mousex = mousey = 0;

    if (forward > MAXPLMOVE)
	forward = MAXPLMOVE;
    else if (forward < -MAXPLMOVE)
	forward = -MAXPLMOVE;
    if (side > MAXPLMOVE)
	side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
	side = -MAXPLMOVE;

    cmd->forwardmove += forward;
    cmd->sidemove += side;

    /* special buttons*/
    if (sendpause)
    {
	sendpause = false;
	cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

    if (sendsave)
    {
	sendsave = false;
	cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot<<BTS_SAVESHIFT);
    }
}



/* G_DoLoadLevel */

static void G_DoLoadLevel (void)
{
    int             i;

    /* Set the sky map.*/
    /* First thing, we have a dummy sky texture name,*/
    /*  a flat. The data is in the WAD only because*/
    /*  we look for an actual index, instead of simply*/
    /*  setting one.*/
    skyflatnum = R_FlatNumForName ( SKYFLATNAME );

    /* DOOM determines the sky texture to be used*/
    /* depending on the current episode, and the game version.*/
    if ( (gamemode == commercial)
	 || ( gamemission == pack_tnt )
	 || ( gamemission == pack_plut ) )
    {
	skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY3]);
	if (gamemap < 12)
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY1]);
	else
	    if (gamemap < 21)
		skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY2]);
    }
    else
    {
	switch (gameepisode)
	{
	  case 1:
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY1]);
	    break;
	  case 2:
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY2]);
	    break;
	  case 3:
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY3]);
	    break;
	  case 4:	/* Special Edition sky*/
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY4]);
	    break;
	}
    }

    levelstarttic = gametic;        /* for time calculation*/

    if (wipegamestate == GS_LEVEL)
	wipegamestate = -1;             /* force a wipe */

    gamestate = GS_LEVEL;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (playeringame[i] && players[i].playerstate == PST_DEAD)
	    players[i].playerstate = PST_REBORN;
	memset (players[i].frags,0,sizeof(players[i].frags));
    }

    BOOMSTATEMENT(sector_list = NULL;)
    BOOMSTATEMENT(headsecnode = NULL;)

    P_SetupLevel (gameepisode, gamemap, 0, gameskill);
    displayplayer = consoleplayer;		/* view the guy you are playing    */
    starttime = I_GetTime ();
    gameaction = ga_nothing;
    Z_CheckHeap ();

    /* clear cmd building stuff*/
    memset (gamekeydown, 0, sizeof(gamekeydown));
    joyxmove = joyymove = 0;
    mousex = mousey = 0;
    sendpause = sendsave = paused = false;
    memset (mousebuttons, 0, sizeof(mousebuttons));
    memset (joybuttons, 0, sizeof(joybuttons));
}



/* G_Responder  */
/* Get info needed to make ticcmd_ts for the players.*/

boolean G_Responder (const event_t* ev)
{
    /* allow spy mode changes even during the demo*/
    if (gamestate == GS_LEVEL && ev->type == ev_keydown
	&& ev->data1 == KEY_F12 && (singledemo || !deathmatch) )
    {
	/* spy mode */
	do
	{
	    displayplayer++;
	    if (displayplayer == MAXPLAYERS)
		displayplayer = 0;
	} while (!playeringame[displayplayer] && displayplayer != consoleplayer);
	return true;
    }

    /* any other key pops up menu if in demos*/
    if (gameaction == ga_nothing && !singledemo &&
	(demoplayback || gamestate == GS_DEMOSCREEN)
	)
    {
	if (ev->type == ev_keydown ||
	    (ev->type == ev_mouse && ev->data1) ||
	    (ev->type == ev_joystick && ev->data1) )
	{
	    M_StartControlPanel ();
	    return true;
	}
	return false;
    }

    if (gamestate == GS_LEVEL)
    {
#if 0
	if (devparm && ev->type == ev_keydown && ev->data1 == ';')
	{
	    G_DeathMatchSpawnPlayer (0);
	    return true;
	}
#endif
	if (HU_Responder (ev))
	    return true;	/* chat ate the event */
	if (ST_Responder (ev))
	    return true;	/* status window ate it */
	if (AM_Responder (ev))
	    return true;	/* automap ate it */
    }

    if (gamestate == GS_FINALE)
    {
	if (F_Responder (ev))
	    return true;	/* finale ate the event */
    }

    switch (ev->type)
    {
      case ev_keydown:
	if (ev->data1 == KEY_PAUSE)
	{
	    sendpause = true;
	    return true;
	}
	if (ev->data1 <NUMKEYS)
	    gamekeydown[ev->data1] = true;
	return true;    /* eat key down events */

      case ev_keyup:
	if (ev->data1 <NUMKEYS)
	    gamekeydown[ev->data1] = false;
	return false;   /* always let key up events filter down */

      case ev_mouse:
	mousebuttons[0] = ev->data1 & 1;
	mousebuttons[1] = ev->data1 & 2;
	mousebuttons[2] = ev->data1 & 4;
	mousex = ev->data2*(mouseSensitivity+5)/10;
	mousey = ev->data3*(mouseSensitivity+5)/10;
	return true;    /* eat events */

      case ev_joystick:
	joybuttons[0] = ev->data1 & 1;
	joybuttons[1] = ev->data1 & 2;
	joybuttons[2] = ev->data1 & 4;
	joybuttons[3] = ev->data1 & 8;
	joyxmove = ev->data2;
	joyymove = ev->data3;
	return true;    /* eat events */

      default:
	break;
    }

    return false;
}




/* G_Ticker*/
/* Make ticcmd_ts for the players.*/

void G_Ticker (void)
{
    int		i;
    int		buf;
    ticcmd_t*	cmd;

    /* do player reborns if needed*/
    for (i=0 ; i<MAXPLAYERS ; i++)
	if (playeringame[i] && players[i].playerstate == PST_REBORN)
	    G_DoReborn (i);

    /* do things to change the game state*/
    while (gameaction != ga_nothing)
    {
	switch (gameaction)
	{
	  case ga_loadlevel:
	    G_DoLoadLevel ();
	    break;
	  case ga_newgame:
	    G_DoNewGame ();
	    break;
	  case ga_loadgame:
	    G_DoLoadGame ();
	    break;
	  case ga_savegame:
	    G_DoSaveGame ();
	    break;
	  case ga_playdemo:
	    G_DoPlayDemo ();
	    break;
	  case ga_completed:
	    G_DoCompleted ();
	    break;
	  case ga_victory:
	    F_StartFinale ();
	    break;
	  case ga_worlddone:
	    G_DoWorldDone ();
	    break;
	  case ga_screenshot:
	    M_ScreenShot ();
	    gameaction = ga_nothing;
	    break;
	  case ga_nothing:
	    break;
	}
    }

    /* get commands, check consistancy,*/
    /* and build new consistancy check*/
    buf = (gametic/ticdup)%BACKUPTICS;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (playeringame[i])
	{
	    cmd = &players[i].cmd;

	    memcpy (cmd, &netcmds[i][buf], sizeof(ticcmd_t));

	    if (demoplayback)
	    {
		G_ReadDemoTiccmd (cmd);
#ifdef FULL_NEW_FEATURES
		/* Normally the console player isn't updated via the ticcmd */
		if (i == consoleplayer)
		{
		    jump_cheat = ((cmd->newflags & NEW_FEATURE_JUMP) != 0);
		    if ((cmd->newflags & NEW_FEATURE_FLOAT) == 0)
		    {
			players[i].mo->flags &= ~(MF_NOGRAVITY + MF_FLOAT);
		    }
		    else
		    {
			players[i].mo->flags |= (MF_NOGRAVITY + MF_FLOAT);
		    }
		    if ((((cmd->newflags & NEW_FEATURE_HOMING) == 0) && homing_missiles)
		     || (((cmd->newflags & NEW_FEATURE_HOMING) != 0) && !homing_missiles))
		    {
			P_MakeHomingMissiles(-1);
		    }
		}
#endif
	    }
	    if (demorecording)
		G_WriteDemoTiccmd (cmd);

	    /* check for turbo cheats*/
	    /* Note: cast to signed char done to fix things on RISCOS */
	    if ((signed char)(cmd->forwardmove) > TURBOTHRESHOLD
		&& !(gametic&31) && ((gametic>>5)&3) == i )
	    {
		static char turbomessage[80];

		sprintf (turbomessage, "%s is turbo!",player_names[i]);
		players[consoleplayer].message = turbomessage;
	    }

	    if (netgame && !netdemo && !(gametic%ticdup) )
	    {
		if (gametic > BACKUPTICS
		    && consistancy[i][buf] != cmd->consistancy)
		{
		    I_Error ("consistency failure (%i should be %i)",
			     cmd->consistancy, consistancy[i][buf]);
		}
		if (players[i].mo)
		    consistancy[i][buf] = players[i].mo->x;
		else
		    consistancy[i][buf] = rndindex;
	    }
	}
    }

    /* check for special buttons*/
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (playeringame[i])
	{
	    if (players[i].cmd.buttons & BT_SPECIAL)
	    {
		switch (players[i].cmd.buttons & BT_SPECIALMASK)
		{
		  case BTS_PAUSE:
		    paused ^= 1;
		    if (paused)
			S_PauseSound ();
		    else
			S_ResumeSound ();
		    break;

		  case BTS_SAVEGAME:
		    if (!savedescription[0])
			strcpy (savedescription, "NET GAME");
		    savegameslot =
			(players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT;
		    gameaction = ga_savegame;
		    break;
		}
	    }
	}
    }

    /* do main actions*/
    switch (gamestate)
    {
      case GS_LEVEL:
	P_Ticker ();
	ST_Ticker ();
	AM_Ticker ();
	HU_Ticker ();
	break;

      case GS_INTERMISSION:
	WI_Ticker ();
	break;

      case GS_FINALE:
	F_Ticker ();
	break;

      case GS_DEMOSCREEN:
	D_PageTicker ();
	break;
    }
}



/* PLAYER STRUCTURE FUNCTIONS*/
/* also see P_SpawnPlayer in P_Things*/



/* G_PlayerFinishLevel*/
/* Can when a player completes a level.*/

static void G_PlayerFinishLevel (int player)
{
    player_t*	p;

    p = &players[player];

    memset (p->powers, 0, sizeof (p->powers));
    memset (p->cards, 0, sizeof (p->cards));
    p->mo->flags &= ~MF_SHADOW;		/* cancel invisibility */
    p->extralight = 0;			/* cancel gun flashes */
    p->fixedcolormap = 0;		/* cancel ir gogles */
    p->damagecount = 0;			/* no palette changes */
    p->bonuscount = 0;
}



/* G_PlayerReborn*/
/* Called after a player dies */
/* almost everything is cleared and initialized */

void G_PlayerReborn (int player)
{
    player_t*	p;
    int		i;
    int		frags[MAXPLAYERS];
    int		killcount;
    int		itemcount;
    int		secretcount;

    memcpy (frags,players[player].frags,sizeof(frags));
    killcount = players[player].killcount;
    itemcount = players[player].itemcount;
    secretcount = players[player].secretcount;

    p = &players[player];
    memset (p, 0, sizeof(*p));

    memcpy (players[player].frags, frags, sizeof(players[player].frags));
    players[player].killcount = killcount;
    players[player].itemcount = itemcount;
    players[player].secretcount = secretcount;

    p->usedown = p->attackdown = true;	/* don't do anything immediately */
    p->playerstate = PST_LIVE;
    p->health = InitHealth;
    p->readyweapon = p->pendingweapon = wp_pistol;
    p->weaponowned[wp_fist] = true;
    p->weaponowned[wp_pistol] = true;
    p->ammo[am_clip] = InitAmmo;

    for (i=0 ; i<NUMAMMO ; i++)
	p->maxammo[i] = maxammo[i];

}


/* G_CheckSpot  */
/* Returns false if the player cannot be respawned*/
/* at the given mapthing_t spot  */
/* because something is occupying it */

static boolean
G_CheckSpot
( int		playernum,
  mapthing_t*	mthing )
{
    fixed_t		x;
    fixed_t		y;
    subsector_t*	ss;
    unsigned		an;
    mobj_t*		mo;
    int			i;

    if (!players[playernum].mo)
    {
	/* first spawn of level, before corpses*/
	for (i=0 ; i<playernum ; i++)
	    if (players[i].mo->x == mthing->x << FRACBITS
		&& players[i].mo->y == mthing->y << FRACBITS)
		return false;
	return true;
    }

    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;

    if (!P_CheckPosition (players[playernum].mo, x, y) )
	return false;

    /* flush an old corpse if needed */
    if (bodyqueslot >= BODYQUESIZE)
	P_RemoveMobj (bodyque[bodyqueslot%BODYQUESIZE]);
    bodyque[bodyqueslot%BODYQUESIZE] = players[playernum].mo;
    bodyqueslot++;

    /* spawn a teleport fog */
    ss = R_PointInSubsector (x,y);
    an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT;

    mo = P_SpawnMobj (x+20*finecosine[an], y+20*finesine[an]
		      , ss->sector->floorheight
		      , MT_TFOG);

    if (players[consoleplayer].viewz != 1)
	S_StartSound (mo, sfx_telept);	/* don't start sound on first frame */

    return true;
}



/* G_DeathMatchSpawnPlayer */
/* Spawns a player at one of the random death match spots */
/* called at level load and each death */

void G_DeathMatchSpawnPlayer (int playernum)
{
    int             i,j;
    int				selections;

    selections = deathmatch_p - deathmatchstarts;
    if (selections < 4)
	I_Error ("Only %i deathmatch spots, 4 required", selections);

    for (j=0 ; j<20 ; j++)
    {
	i = P_Random() % selections;
	if (G_CheckSpot (playernum, &deathmatchstarts[i]) )
	{
	    deathmatchstarts[i].type = playernum+1;
	    P_SpawnPlayer (&deathmatchstarts[i]);
	    return;
	}
    }

    /* no good spot, so the player will probably get stuck */
    P_SpawnPlayer (&playerstarts[playernum]);
}


/* G_DoReborn */

static void G_DoReborn (int playernum)
{
    int                             i;

    if (!netgame)
    {
	/* reload the level from scratch*/
	gameaction = ga_loadlevel;
    }
    else
    {
	/* respawn at the start*/

	/* first dissasociate the corpse */
	players[playernum].mo->player = NULL;

	/* spawn at random spot if in death match */
	if (deathmatch)
	{
	    G_DeathMatchSpawnPlayer (playernum);
	    return;
	}

	if (G_CheckSpot (playernum, &playerstarts[playernum]) )
	{
	    P_SpawnPlayer (&playerstarts[playernum]);
	    return;
	}

	/* try to spawn at one of the other players spots */
	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (G_CheckSpot (playernum, &playerstarts[i]) )
	    {
		playerstarts[i].type = playernum+1;	/* fake as other player */
		P_SpawnPlayer (&playerstarts[i]);
		playerstarts[i].type = i+1;		/* restore */
		return;
	    }
	    /* he's going to be inside something.  Too bad.*/
	}
	P_SpawnPlayer (&playerstarts[playernum]);
    }
}


void G_ScreenShot (void)
{
    gameaction = ga_screenshot;
}



/* DOOM Par Times*/
static int pars[4][10] =
{
    {0},
    {0,30,75,120,90,165,180,180,30,165},
    {0,90,90,90,120,90,360,240,30,170},
    {0,90,45,90,150,90,90,165,30,135}
};

/* DOOM II Par Times*/
static int cpars[32] =
{
    30,90,120,120,90,150,120,120,270,90,	/*  1-10*/
    210,150,150,150,210,150,420,150,210,150,	/* 11-20*/
    240,150,180,150,150,300,330,420,300,180,	/* 21-30*/
    120,30					/* 31-32*/
};



/* G_DoCompleted */

static boolean		secretexit;

void G_ExitLevel (void)
{
    secretexit = false;
    gameaction = ga_completed;
}

/* Here's for the german edition.*/
void G_SecretExitLevel (void)
{
    /* IF NO WOLF3D LEVELS, NO SECRET EXIT!*/
    if ( (gamemode == commercial)
      && (W_CheckNumForName("map31")<0))
	secretexit = false;
    else
	secretexit = true;
    gameaction = ga_completed;
}

static void G_DoCompleted (void)
{
    int             i;

    gameaction = ga_nothing;

    for (i=0 ; i<MAXPLAYERS ; i++)
	if (playeringame[i])
	    G_PlayerFinishLevel (i);        /* take away cards and stuff */

    if (automapactive)
	AM_Stop ();

    if ( gamemode != commercial)
	switch(gamemap)
	{
	  case 8:
	    gameaction = ga_victory;
	    return;
	  case 9:
	    for (i=0 ; i<MAXPLAYERS ; i++)
		players[i].didsecret = true;
	    break;
	}

/*#if 0  Hmmm - why?*/
    if ( (gamemap == 8)
	 && (gamemode != commercial) )
    {
	/* victory */
	gameaction = ga_victory;
	return;
    }

    if ( (gamemap == 9)
	 && (gamemode != commercial) )
    {
	/* exit secret level */
	for (i=0 ; i<MAXPLAYERS ; i++)
	    players[i].didsecret = true;
    }
/*#endif*/


    wminfo.didsecret = players[consoleplayer].didsecret;
    wminfo.epsd = gameepisode -1;
    wminfo.last = gamemap -1;

    /* wminfo.next is 0 biased, unlike gamemap*/
    if ( gamemode == commercial)
    {
	if (secretexit)
	    switch(gamemap)
	    {
	      case 15: wminfo.next = 30; break;
	      case 31: wminfo.next = 31; break;
	    }
	else
	    switch(gamemap)
	    {
	      case 31:
	      case 32: wminfo.next = 15; break;
	      default: wminfo.next = gamemap;
	    }
    }
    else
    {
	if (secretexit)
	    wminfo.next = 8; 	/* go to secret level */
	else if (gamemap == 9)
	{
	    /* returning from secret level */
	    switch (gameepisode)
	    {
	      case 1:
		wminfo.next = 3;
		break;
	      case 2:
		wminfo.next = 5;
		break;
	      case 3:
		wminfo.next = 6;
		break;
	      case 4:
		wminfo.next = 2;
		break;
	    }
	}
	else
	    wminfo.next = gamemap;          /* go to next level */
    }

    wminfo.maxkills = totalkills;
    wminfo.maxitems = totalitems;
    wminfo.maxsecret = totalsecret;
    wminfo.maxfrags = 0;
    if ( gamemode == commercial )
	wminfo.partime = 35*cpars[gamemap-1];
    else
	wminfo.partime = 35*pars[gameepisode][gamemap];
    wminfo.pnum = consoleplayer;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	wminfo.plyr[i].in = playeringame[i];
	wminfo.plyr[i].skills = players[i].killcount;
	wminfo.plyr[i].sitems = players[i].itemcount;
	wminfo.plyr[i].ssecret = players[i].secretcount;
	wminfo.plyr[i].stime = leveltime;
	memcpy (wminfo.plyr[i].frags, players[i].frags
		, sizeof(wminfo.plyr[i].frags));
    }

    gamestate = GS_INTERMISSION;
    viewactive = false;
    automapactive = mixmap = false;

    if (statcopy)
	memcpy (statcopy, &wminfo, sizeof(wminfo));

    WI_Start (&wminfo);
}



/* G_WorldDone */

void G_WorldDone (void)
{
    gameaction = ga_worlddone;

    if (secretexit)
	players[consoleplayer].didsecret = true;

    if ( gamemode == commercial )
    {
	switch (gamemap)
	{
	  case 15:
	  case 31:
	    if (!secretexit)
		break;
	  case 6:
	  case 11:
	  case 20:
	  case 30:
	    F_StartFinale ();
	    break;
	}
    }
}

static void G_DoWorldDone (void)
{
    gamestate = GS_LEVEL;
    gamemap = wminfo.next+1;
    G_DoLoadLevel ();
    gameaction = ga_nothing;
    viewactive = true;
}




/* G_InitFromSavegame*/
/* Can be called by the startup code or the menu task. */

static const char save_new_ident[] = "version 11X";

static char	savename[256];

void G_LoadGame (const char* name)
{
    strcpy (savename, name);
    gameaction = ga_loadgame;
}

#define VERSIONSIZE		16


static void G_LoadGameOld(void)
{
    P_UnArchivePlayers ();
    P_UnArchiveWorld ();
    P_UnArchiveThinkers ();
    P_UnArchiveSpecials ();
    BOOMSTATEMENT(P_UnArchiveMap();)
}

static void G_LoadGameNew(void)
{
    P_UnArchivePlayersNew();
    P_UnArchiveWorldNew();
    P_UnArchiveThinkersNew();
    P_UnArchiveSpecialsNew();
    P_UnArchiveMapNew();
}

static void G_DoLoadGame (void)
{
    int		length;
    int		i;
    int		a,b,c;
    char	vcheck[VERSIONSIZE];
    boolean	oldstyle;

    gameaction = ga_nothing;

    length = M_ReadFile (savename, &savebuffer);
    save_p = savebuffer + SAVESTRINGSIZE;

    /* skip the description field */
    memset (vcheck,0,sizeof(vcheck));
    sprintf (vcheck,"version %i",VERSION);
    oldstyle = true;
    if (strcmp ((char*)save_p, vcheck) != 0)
    {
      if (strcmp((char*)save_p, save_new_ident) == 0)
        oldstyle = false;
      else
	return;				/* bad version */
    }
    save_p += VERSIONSIZE;

    gameskill = *save_p++;
    gameepisode = *save_p++;
    gamemap = *save_p++;
    for (i=0 ; i<MAXPLAYERS ; i++)
	playeringame[i] = *save_p++;

    /* load a base level */
    G_InitNew (gameskill, gameepisode, gamemap);

    /* get the times */
    a = *save_p++;
    b = *save_p++;
    c = *save_p++;
    leveltime = (a<<16) + (b<<8) + c;

    /* dearchive all the modifications*/
    if (oldstyle)
      G_LoadGameOld();
    else
      G_LoadGameNew();

    if (*save_p != 0x1d)
	I_Error ("Bad savegame");

    /* done */
    Z_Free (savebuffer);

    if (setsizeneeded)
	R_ExecuteSetViewSize ();

    /* draw the pattern into the back screen*/
    R_FillBackScreen ();
}



static void G_SaveGameOld(void)
{
    int i;

    P_ArchivePlayers ();
    BOOMSTATEMENT(P_ThinkerToIndex();)
    P_ArchiveWorld ();
    P_ArchiveThinkers ();
    BOOMSTATEMENT(P_IndexToThinker();)
    P_ArchiveSpecials ();
    BOOMSTATEMENT(P_ArchiveMap();)

    if ((i = (int)(save_p - savebuffer)) > 0)
	fwrite(savebuffer, 1, i, save_file);
}

static void G_SaveGameNew(void)
{
    P_ArchivePlayersNew();
    P_ThinkerToIndex();
    P_ArchiveWorldNew();
    P_ArchiveThinkersNew();
    P_IndexToThinker();
    P_ArchiveSpecialsNew();
    P_ArchiveMapNew();

    P_FlushSavebufferNew();
}

/* G_SaveGame*/
/* Called by the menu task.*/
/* Description is a 24 byte text string */

void
G_SaveGame
( int	slot,
  const char*	description )
{
    savegameslot = slot;
    strcpy (savedescription, description);
    sendsave = true;
}

static void G_DoSaveGame (void)
{
    char	name[100];
    char	name2[VERSIONSIZE];
    char*	description;
    int		i;
    const char *gamepath;

    gamepath = getenv(GamePathVar);

    /* Does a directory for saved games exist? */
    if (gamepath == NULL)
    {
	if (M_CheckParm("-cdrom"))
	    sprintf(name, "%s"DIRSEPSTR SAVEGAMENAME"%d"EXTSEPSTR SAVEGAMEEXT, I_GetCDSaveDir(), savegameslot);
	else
	    sprintf(name, "%s"SAVEGAMENAME"%d"EXTSEPSTR SAVEGAMEEXT, DefaultGamePath, savegameslot);
    }
    else
    {
	sprintf(name, "%s"SAVEGAMENAME"%d"EXTSEPSTR SAVEGAMEEXT, gamepath, savegameslot);
    }

    description = savedescription;
    save_p = savebuffer = ((byte*)(screens[1])) + 0x4000;
    save_high_p = (byte*)(screens[1] + SCREENWIDTH*SCREENHEIGHT);

    /* Let's do a decent job here... */
    if ((save_file = fopen(name, "wb")) == NULL)
	I_Error("Couldn't save game.");

    memcpy (save_p, description, SAVESTRINGSIZE);
    save_p += SAVESTRINGSIZE;
    memset (name2,0,sizeof(name2));
    if (oldsavegames)
      sprintf (name2,"version %i",VERSION);
    else
      strcpy (name2, save_new_ident);
    memcpy (save_p, name2, VERSIONSIZE);
    save_p += VERSIONSIZE;

    *save_p++ = gameskill;
    *save_p++ = gameepisode;
    *save_p++ = gamemap;
    for (i=0 ; i<MAXPLAYERS ; i++)
	*save_p++ = playeringame[i];
    *save_p++ = leveltime>>16;
    *save_p++ = leveltime>>8;
    *save_p++ = leveltime;

    if (oldsavegames)
      G_SaveGameOld();
    else
      G_SaveGameNew();

    fputc(0x1d, save_file);	/* consistancy marker */
    fclose(save_file);

    gameaction = ga_nothing;
    savedescription[0] = 0;

    players[consoleplayer].message = GGSAVED;

    /* draw the pattern into the back screen*/
    memset(screens[1], 0, SCREENWIDTH*SCREENHEIGHT*sizeof(pixel_t));
    R_FillBackScreen ();
}



/* G_InitNew*/
/* Can be called by the startup code or the menu task,*/
/* consoleplayer, displayplayer, playeringame[] should be set. */

skill_t	d_skill;
int     d_episode;
int     d_map;

void
G_DeferedInitNew
( skill_t	skill,
  int		episode,
  int		map)
{
    d_skill = skill;
    d_episode = episode;
    d_map = map;
    gameaction = ga_newgame;
}


static void G_DoNewGame (void)
{
    demoplayback = false;
    netdemo = false;
    netgame = false;
    deathmatch = false;
    playeringame[1] = playeringame[2] = playeringame[3] = 0;
    respawnparm = false;
    fastparm = false;
    nomonsters = false;
    consoleplayer = 0;
    BOOMSTATEMENT(compatibility=0; demo_compatibility = 0;)
    G_InitNew (d_skill, d_episode, d_map);
    gameaction = ga_nothing;
}


void G_SetFastParms(int fast_pending)
{
    static int fast = 0;
    int i;
    if (fast == fast_pending)
      return;
    if ((fast = fast_pending) != 0)
    {
	for (i=S_SARG_RUN1; i<=S_SARG_PAIN2; i++)
	    if (states[i].tics != 1 BOOMSTATEMENT(|| demo_compatibility))
		states[i].tics >>= 1;
	mobjinfo[MT_BRUISERSHOT].speed = 20*FRACUNIT;
	mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT;
	mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT;
    }
    else
    {
	for (i=S_SARG_RUN1; i<=S_SARG_PAIN2; i++)
	    states[i].tics <<= 1;
	mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT;
	mobjinfo[MT_HEADSHOT].speed = 10*FRACUNIT;
	mobjinfo[MT_TROOPSHOT].speed = 10*FRACUNIT;
    }
}

/* The sky texture to be used instead of the F_SKY1 dummy.*/

void
G_InitNew
( skill_t	skill,
  int		episode,
  int		map )
{
    int             i;

    if (paused)
    {
	paused = false;
	S_ResumeSound ();
    }


    if (skill > sk_nightmare)
	skill = sk_nightmare;


    /* This was quite messy with SPECIAL and commented parts.*/
    /* Supposedly hacks to make the latest edition work.*/
    /* It might not work properly.*/
    if (episode < 1)
      episode = 1;

    if ( gamemode == retail )
    {
      if (episode > 4)
	episode = 4;
    }
    else if ( gamemode == shareware )
    {
      if (episode > 1)
	   episode = 1;	/* only start episode 1 on shareware*/
    }
    else
    {
      if (episode > 3)
	episode = 3;
    }



    if (map < 1)
	map = 1;

    if ( (map > 9)
	 && ( gamemode != commercial) )
      map = 9;

    G_SetFastParms(fastparm || skill == sk_nightmare);

    M_ClearRandom ();

    if (skill == sk_nightmare || respawnparm )
	respawnmonsters = true;
    else
	respawnmonsters = false;

    /* force players to be initialized upon first level load         */
    for (i=0 ; i<MAXPLAYERS ; i++)
	players[i].playerstate = PST_REBORN;

    usergame = true;                /* will be set false if a demo */
    paused = false;
    demoplayback = false;
    automapactive = mixmap = false;
    viewactive = true;
    gameepisode = episode;
    gamemap = map;
    gameskill = skill;

    viewactive = true;

    /* set the sky map for the episode*/
    if ( gamemode == commercial)
    {
	skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY3]);
	if (gamemap < 12)
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY1]);
	else
	    if (gamemap < 21)
		skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY2]);
    }
    else
	switch (episode)
	{
	  case 1:
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY1]);
	    break;
	  case 2:
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY2]);
	    break;
	  case 3:
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY3]);
	    break;
	  case 4:	/* Special Edition sky*/
	    skytexture = R_TextureNumForName (ggame_lumps[GGAME_SKY4]);
	    break;
	}

    G_DoLoadLevel ();
}



/* DEMO RECORDING */

#define DEMOMARKER		0x80


static void G_ReadDemoTiccmd (ticcmd_t* cmd)
{
    /* modified: allow 'q' to abort demo playback */
    if ((*demo_p == DEMOMARKER) || (gamekeydown['q']))
    {
	/* end of demo data stream */
	G_CheckDemoStatus ();
	return;
    }
    cmd->forwardmove = ((signed char)*demo_p++);
    cmd->sidemove = ((signed char)*demo_p++);
    cmd->angleturn = ((signed char)*demo_p++)<<8;
    cmd->buttons = (unsigned char)*demo_p++;
#ifdef FULL_NEW_FEATURES
    if (DemoVersionNumber == VERSION)
    {
        cmd->newflags = ((byte)*demo_p++);
        cmd->vaccel = ((signed char)*demo_p++);
        cmd->misc1 = 0; cmd->misc2 = 0;
    }
#endif
}


static void G_WriteDemoTiccmd (ticcmd_t* cmd)
{
    if (gamekeydown['q'])           /* press q to end demo recording */
	G_CheckDemoStatus ();
    *demo_p++ = cmd->forwardmove;
    *demo_p++ = cmd->sidemove;
    *demo_p++ = (cmd->angleturn+128)>>8;
    *demo_p++ = cmd->buttons;
#ifdef FULL_NEW_FEATURES
    if (DemoVersionNumber == VERSION)
    {
        *demo_p++ = cmd->newflags;
        *demo_p++ = cmd->vaccel;
        demo_p -= 2;
    }
#endif
    demo_p -= 4;

    if (demo_p > demoend - 16)
    {
	/* no more space */
	G_CheckDemoStatus ();
	return;
    }

    G_ReadDemoTiccmd (cmd);         /* make SURE it is exactly the same */
}




/* G_RecordDemo */

void G_RecordDemo (const char* name)
{
    int             i;
    int				maxsize;

    usergame = false;
#ifdef __riscos__
    sprintf(demoname, "Doom:Demos.%s/lmp", name);
#else
    strcpy (demoname, name);
    strcat (demoname, ".lmp");
#endif
    fprintf(logfile, "Record demo to file %s\n", demoname);
    maxsize = 0x20000;
    i = M_CheckParm ("-maxdemo");
    if (i && i<myargc-1)
	maxsize = atoi(myargv[i+1])*1024;
    demobuffer = Z_Malloc (maxsize,PU_STATIC,NULL);
    demoend = demobuffer + maxsize;

    demorecording = true;
}


void G_BeginRecording (void)
{
    int             i;

    demo_p = demobuffer;

    DemoVersionNumber = VERSION;
#ifdef FULL_NEW_FEATURES
    /* record a normal demo or one with jump/float extensions? */
    if (M_CheckParm("-normaldemo"))
#ifdef DIYBOOM
        DemoVersionNumber = DIY_VERSION_BOOM;
#else
        DemoVersionNumber = DIY_VERSION_DOOM;
#endif
#endif
    *demo_p++ = DemoVersionNumber;
    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    *demo_p++ = deathmatch;
    *demo_p++ = respawnparm;
    *demo_p++ = fastparm;
    *demo_p++ = nomonsters;
    *demo_p++ = consoleplayer;

    for (i=0 ; i<MAXPLAYERS ; i++)
	*demo_p++ = playeringame[i];
}



/* G_PlayDemo */


static const char*	defdemoname = NULL;
static int		lastdemolump = 0;
static int 		demosplayed = 0;

void G_DeferedPlayDemo (const char* name)
{
    defdemoname = name;
    gameaction = ga_playdemo;
    if (!singledemo) lastdemolump = 0;
}

/* primitive pattern matching code */
static int G_MatchDemoName(const char *pat, const char *strl)
{
    const unsigned char *b, *d;

    if (pat == NULL)
        return -1;

    b = (const unsigned char*)pat; d = (const unsigned char*)strl;

    while (*b != '\0')
    {
        if (tolower(*b) == tolower(*d))
        {
            b++; d++;
        }
        else if (*b == '#')
        {
            if ((*d < '0') || (*d > '9')) return -1;
            b++; d++;
        }
        else if (*b == '?')
        {
            if (*d == '\0') return -1;
            b++; d++;
        }
        else if (*b == '*')
        {
            b++;
            while (G_MatchDemoName((const char*)b, (const char*)d) != 0)
            {
                if (*d++ == '\0') return -1;
            }
            return 0;
        }
        else return -1;
    }

    if (*d == '\0') return 0;

    return -1;
}

static void G_DoPlayDemo (void)
{
    skill_t		skill;
    int			i, episode, map;
    char		lumpname[9];
    int			findlump;

    lumpname[8] = '\0';
    do
    {
        findlump = 0;

        for ( ; lastdemolump < numlumps; lastdemolump++)
        {
            strncpy(lumpname, lumpinfo[lastdemolump].name, 8);
            if (G_MatchDemoName(defdemoname, lumpname) == 0)
            {
                /* is this the _last_ lump of that name? */
                if (W_GetNumForName(lumpname) == lastdemolump) break;
                /* if not just ignore it, we'll get there eventually. */
            }
        }
        if (lastdemolump >= numlumps)
        {
            if (demosplayed == 0)
            {
                /* only print this message if no demo matching this pattern was found,
                   not if several were played already and there are no more */
                fprintf(logfile, "No demo matching %s found\n", defdemoname); fflush(logfile);
            }
            I_Quit();
        }
        fprintf(logfile, "Playing demo %s\n", lumpname); fflush(logfile);
        demobuffer = demo_p = W_CacheLumpNum(lastdemolump++, PU_STATIC);
        demosplayed++;

        DemoVersionNumber = (int)(*demo_p++);
#if 0
        if ( DemoVersionNumber != VERSION)
        {
            fprintf( logfile, "Demo is from a different game version (%d)!\n", DemoVersionNumber); fflush(logfile);
            /*gameaction = ga_nothing;*/
            findlump = 1;
        }
#else
        if ( DemoVersionNumber > VERSION)
        {
            fprintf( logfile, "Demo is from an unknown, newer version (%d)!\n", DemoVersionNumber); fflush(logfile);
            /*gameaction = ga_nothing;*/
            findlump = 1;
        }
#endif
    }
    while (findlump != 0);

#ifdef DIYBOOM
    if ((DemoVersionNumber != DIY_VERSION_BOOM) && (DemoVersionNumber != DIY_VERSION_BOOMF))
    {
        compatibility = 1; demo_compatibility = 1;
        variable_friction = 0; allow_pushers = 0;
    }
    else
    {
        compatibility = 0; demo_compatibility = 0;
    }
    fprintf(logfile, "Boom demo compatibility: %d\n", demo_compatibility);
#endif
#ifdef FULL_NEW_FEATURES
    if (DemoVersionNumber == VERSION)
    {
        fprintf(logfile, "Playing back full features demo\n");
    }
#endif
    skill = *demo_p++;
    episode = *demo_p++;
    map = *demo_p++;
    deathmatch = *demo_p++;
    respawnparm = *demo_p++;
    fastparm = *demo_p++;
    nomonsters = *demo_p++;
    consoleplayer = *demo_p++;

    for (i=0 ; i<MAXPLAYERS ; i++)
	playeringame[i] = *demo_p++;
    if (playeringame[1])
    {
	netgame = true;
	netdemo = true;
    }

    /* don't spend a lot of time in loadlevel */
    precache = false;
    G_InitNew (skill, episode, map);
    precache = true;

    usergame = false;
    demoplayback = true;
}


/* G_TimeDemo */

void G_TimeDemo (const char* name)
{
    nodrawers = M_CheckParm ("-nodraw");
    noblit = M_CheckParm ("-noblit");
    timingdemo = true;
    singletics = true;

    defdemoname = name;
    gameaction = ga_playdemo;
}


/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

boolean G_CheckDemoStatus (void)
{
    int             endtime;

    if (timingdemo)
    {
	endtime = I_GetTime ();
	fprintf(logfile, "timed %i gametics in %i realtics\n", gametic-levelstarttic, endtime-starttime);
    }

    if (demoplayback)
    {
	/*if (singledemo)
	    I_Quit ();*/

	Z_ChangeTag (demobuffer, PU_CACHE);
	demoplayback = false;
	netdemo = false;
	netgame = false;
	deathmatch = false;
	playeringame[1] = playeringame[2] = playeringame[3] = 0;
	respawnparm = false;
	fastparm = false;
	nomonsters = false;
	consoleplayer = 0;
        BOOMSTATEMENT(compatibility = 0; demo_compatibility = 0;)

	if (singledemo)
	    G_DeferedPlayDemo(defdemoname);
	else
	    D_AdvanceDemo();

	/*D_AdvanceDemo ();*/
	return true;
    }

    if (demorecording)
    {
	*demo_p++ = DEMOMARKER;
	M_WriteFile (demoname, demobuffer, demo_p - demobuffer);
	Z_Free (demobuffer);
	demorecording = false;
	I_Error ("Demo %s recorded",demoname);
    }

    return false;
}
