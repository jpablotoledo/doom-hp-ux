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
/*	Cheat sequence checking.*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: m_cheat.c,v 1.1 1997/02/03 21:24:34 b1 Exp $";

#include "doomstat.h"
#include "dstrings.h"
#include "g_game.h"
#include "m_cheat.h"
#include "p_inter.h"
#include "p_local.h"
#include "st_stuff.h"
#include "s_sound.h"
#include "sounds.h"
#include <ctype.h>

#ifdef FULL_NEW_FEATURES
# define NOT_NET always
#else
# define NOT_NET not_net
#endif

static char buf[ST_MSGWIDTH];


/* CHEAT SEQUENCE PACKAGE*/

static void cheat_mus(char *);
static void cheat_choppers(void);
static void cheat_god(void);
static void cheat_fa(void);
static void cheat_k(void);
static void cheat_kfa(void);
static void cheat_noclip(void);
static void cheat_pw(int);
static void cheat_behold(void);
static void cheat_clev(char *);
static void cheat_mypos(void);
#ifdef DIYBOOM
static void cheat_comp(void);
static void cheat_friction(void);
static void cheat_pushers(void);
#endif
#if (defined(DIYTRANSPARENCY) && (LD_PIXEL_DEPTH > 3))
static void cheat_tnttran(void);
#endif
static void cheat_massacre(void);
static void cheat_ddt(void);
/*static void cheat_hom();*/
static void cheat_fast(void);
static void cheat_tntkey(void);
static void cheat_tntkeyx(void);
static void cheat_tntkeyxx(int);
static void cheat_tntweap(void);
static void cheat_tntweapx(char *);
static void cheat_tntammo(void);
static void cheat_tntammox(char *);
/*static void cheat_smart();*/
/*static void cheat_pitch();*/

static void cheat_homing(void);
static void cheat_jump(void);
static void cheat_float(void);
static void cheat_fight(void);


struct cheat_s cheat[] = {
  {"idmus",	 "Change music",      always,
   cheat_mus,	   -2},

  {"idchoppers", "Chainsaw",	      not_net | not_demo,
   cheat_choppers },

  {"iddqd",	 "God mode",	      not_net | not_demo,
   cheat_god	  },

#if 0
  {"idk",	 NULL,		      not_net | not_demo | not_deh,
   cheat_k },  /* The most controversial cheat code in Doom history!!! */
#endif

  {"idkfa",	 "Ammo & Keys",	      not_net | not_demo,
   cheat_kfa },

  {"idfa",	 "Ammo",	      not_net | not_demo,
   cheat_fa  },

  {"idspispopd", "No Clipping 1",     not_net | not_demo,
   cheat_noclip },

  {"idclip",	 "No Clipping 2",     not_net | not_demo,
   cheat_noclip },

  {"idbeholdv",  "Invincibility",     not_net | not_demo,
   cheat_pw,  pw_invulnerability },

  {"idbeholds",  "Berserk",	      not_net | not_demo,
   cheat_pw,  pw_strength	 },

  {"idbeholdi",  "Invisibility",      not_net | not_demo,
   cheat_pw,  pw_invisibility	 },

  {"idbeholdr",  "Radiation Suit",    not_net | not_demo,
   cheat_pw,  pw_ironfeet	 },

  {"idbeholda",  "Auto-map",	      not_net | not_demo,
   cheat_pw,  pw_allmap		 },

  {"idbeholdl",  "Lite-Amp Goggles",  not_net | not_demo,
   cheat_pw,  pw_infrared	 },

  {"idbehold",	 "BEHOLD menu",	      not_net | not_demo,
   cheat_behold	  },

  {"idclev",	 "Level Warp",	      not_net | not_demo | not_menu,
   cheat_clev,	  -2},

  {"idmypos",	 "Player Position",   not_net | not_demo,
   cheat_mypos	  },
#ifdef DIYBOOM
  {"tntcomp",	 NULL,		      not_net | not_demo,
   cheat_comp	  },
#endif
  {"tntem",	 NULL,		      not_net | not_demo,
   cheat_massacre },

  {"iddt",	 "Map cheat",	      not_dm  | not_demo,
   cheat_ddt	  },

/*  {"tnthom",	   NULL,		not_net | not_demo,
   cheat_hom	  },*/

  {"tntkey",	 NULL,		      not_net | not_demo,
   cheat_tntkey	  },

  {"tntkeyr",	 NULL,		      not_net | not_demo,
   cheat_tntkeyx  },

  {"tntkeyy",	 NULL,		      not_net | not_demo,
   cheat_tntkeyx  },

  {"tntkeyb",	 NULL,		      not_net | not_demo,
   cheat_tntkeyx  },

  {"tntkeyrc",	 NULL,		      not_net | not_demo,
   cheat_tntkeyxx, it_redcard	 },

  {"tntkeyyc",	 NULL,		      not_net | not_demo,
   cheat_tntkeyxx, it_yellowcard },

  {"tntkeybc",	 NULL,		      not_net | not_demo,
   cheat_tntkeyxx, it_bluecard	 },

  {"tntkeyrs",	 NULL,		      not_net | not_demo,
   cheat_tntkeyxx, it_redskull	 },

  {"tntkeyys",	 NULL,		      not_net | not_demo,
   cheat_tntkeyxx, it_yellowskull},

  {"tntkeybs",	 NULL,		      not_net | not_demo,
   cheat_tntkeyxx, it_blueskull  },

  {"tntka",	 NULL,		      not_net | not_demo,
   cheat_k    },

  {"tntweap",	 NULL,		      not_net | not_demo,
   cheat_tntweap  },

  {"tntweap",	 NULL,		      not_net | not_demo,
   cheat_tntweapx, -1},

  {"tntammo",	 NULL,		      not_net | not_demo,
   cheat_tntammo  },

  {"tntammo",	 NULL,		      not_net | not_demo,
   cheat_tntammox, -1},

#if (defined(DIYTRANSPARENCY) && (LD_PIXEL_DEPTH > 3))
  {"tnttran",	 NULL,		      always,
   cheat_tnttran  },
#endif

/*  {"tntsmart",   NULL,		not_net | not_demo,
   cheat_smart}, */

/*  {"tntpitch",   NULL,		always,
   cheat_pitch}, */

#if (defined(DIYTRANSPARENCY) && (LD_PIXEL_DEPTH > 3))
  {"tntran",	 NULL,		      always,
   cheat_tnttran    },	 /* same as tnttran */
#endif

  {"tntamo",	 NULL,		      not_net | not_demo,
   cheat_tntammo    },	 /* same as tntammo */

  {"tntamo",	 NULL,		      not_net | not_demo,
   cheat_tntammox, -1},  /* same as tntammo */

  {"tntfast",	   NULL,		not_net | not_demo,
   cheat_fast	    },

#ifdef DIYBOOM
  {"tntice",	 NULL,		      not_net | not_demo,
   cheat_friction   },

  {"tntpush",	 NULL,		      not_net | not_demo,
   cheat_pushers    },
#endif

  {"adhoming",	 NULL,		      NOT_NET | not_demo,
   cheat_homing	    },

  {"adjump",	 NULL,		      NOT_NET | not_demo,
   cheat_jump	    },

  {"admercury",  NULL,		      NOT_NET | not_demo,
   cheat_float	    },

  {"adfight",	 NULL,		      always,
   cheat_fight	    },

#if (defined(DIYTRANSPARENCY) && (LD_PIXEL_DEPTH > 3))
  {"dstrans",	 NULL,		      always,
   cheat_tnttran    },	 /* same as tnttran */
#endif

  {NULL} /* end-of-list marker */
};


/* Added fors DeHackEd */
#define STMSG_MUS	0
#define STMSG_NOMUS	1
#define STMSG_DQDON	2
#define STMSG_DQDOFF	3
#define STMSG_KFAADDED	4
#define STMSG_FAADDED	5
#define STMSG_NCON	6
#define STMSG_NCOFF	7
#define STMSG_BEHOLD	8
#define STMSG_BEHOLDX	9
#define STMSG_CHOPPERS	10
#define STMSG_CLEV	11
#define STMSG_HOMEON	12
#define STMSG_HOMEOFF	13
#define STMSG_JUMPON	14
#define STMSG_JUMPOFF	15
#define STMSG_FLOATON	16
#define STMSG_FLOATOFF	17
#define STMSG_FIGHTON	18
#define STMSG_FIGHTOFF	19
#define STMSG_TRANSON	20
#define STMSG_TRANSOFF	21

char *ststuff_messages[STMSG_NUMBER] = {
  STSTR_MUS,
  STSTR_NOMUS,
  STSTR_DQDON,
  STSTR_DQDOFF,
  STSTR_KFAADDED,
  STSTR_FAADDED,
  STSTR_NCON,
  STSTR_NCOFF,
  STSTR_BEHOLD,
  STSTR_BEHOLDX,
  STSTR_CHOPPERS,
  STSTR_CLEV,
  STSTR_HOMEON,
  STSTR_HOMEOFF,
  STSTR_JUMPON,
  STSTR_JUMPOFF,
  STSTR_FLOATON,
  STSTR_FLOATOFF,
  STSTR_FIGHTON,
  STSTR_FIGHTOFF,
  STSTR_TRANSON,
  STSTR_TRANSOFF
};

int IDFAarmor = 200;
int IDFAarmorclass = 2;
int IDKFAarmor = 200;
int IDKFAarmorclass = 2;
int GodHealth = 100;
/* ignored unless compiled with DIYTRANSPARENCY */
int EnableTransparency = 1;


static void cheat_mus(char *buf)
{
  int musnum;

  /* note: this cheat allowed in netgame/demorecord */

  /* avoid musnum being negative and crashing */
  if (!isdigit((unsigned)buf[0]) || !isdigit((unsigned)buf[1]))
    return;

  players[consoleplayer].message = ststuff_messages[STMSG_MUS];

  if (gamemode == commercial)
    {
      musnum = mus_runnin + (buf[0]-'0')*10 + buf[1]-'0' - 1;
      /* prevent IDMUS00 in DOOMII and IDMUS36 or greater */
      if (musnum < mus_runnin ||  ((buf[0]-'0')*10 + buf[1]-'0') > 35)
	players[consoleplayer].message = ststuff_messages[STMSG_NOMUS];
      else
	S_ChangeMusic(musnum, 1);
    }
  else
    {
      musnum = mus_e1m1 + (buf[0]-'1')*9 + (buf[1]-'1');

      /* prevent IDMUS0x IDMUSx0 in DOOMI and greater than introa */
      if (buf[0] < '1' || buf[1] < '1' || ((buf[0]-'1')*9 + buf[1]-'1') > 31)
	players[consoleplayer].message = ststuff_messages[STMSG_NOMUS];
      else
	S_ChangeMusic(musnum, 1);
    }
}

/* 'choppers' invulnerability & chainsaw */
static void cheat_choppers(void)
{
  players[consoleplayer].weaponowned[wp_chainsaw] = true;
  players[consoleplayer].powers[pw_invulnerability] = true;
  players[consoleplayer].message = ststuff_messages[STMSG_CHOPPERS];
}

static void cheat_god(void)
{ /* 'dqd' cheat for toggleable god mode */
  players[consoleplayer].cheats ^= CF_GODMODE;
  if (players[consoleplayer].cheats & CF_GODMODE)
    {
      players[consoleplayer].health = 100 /*god_health*/;
      players[consoleplayer].message = ststuff_messages[STMSG_DQDON];
    }
  else
    players[consoleplayer].message = ststuff_messages[STMSG_DQDOFF];
}

static void cheat_fa(void)
{
  int i;

  if (!players[consoleplayer].backpack)
    {
      for (i=0 ; i<NUMAMMO ; i++)
	players[consoleplayer].maxammo[i] *= 2;
      players[consoleplayer].backpack = true;
    }

  players[consoleplayer].armorpoints = IDFAarmor;
  players[consoleplayer].armortype = IDFAarmorclass;

  /* You can't own weapons that aren't in the game */
  for (i=0;i<NUMWEAPONS;i++)
    if (!(((i == wp_plasma || i == wp_bfg) && gamemode == shareware) ||
	  (i == wp_supershotgun && gamemode != commercial)))
      players[consoleplayer].weaponowned[i] = true;

  for (i=0;i<NUMAMMO;i++)
    if (i!=am_cell || gamemode!=shareware)
      players[consoleplayer].ammo[i] = players[consoleplayer].maxammo[i];

  players[consoleplayer].message = ststuff_messages[STMSG_FAADDED];
}

static void cheat_k(void)
{
  int i;
  for (i=0;i<NUMCARDS;i++)
    if (!players[consoleplayer].cards[i])/* only print message if at least one key added */
      {		  	/* however, caller may overwrite message anyway */
	players[consoleplayer].cards[i] = true;
	players[consoleplayer].message = "Keys Added";
      }
}

static void cheat_kfa(void)
{
  cheat_k();
  cheat_fa();
  players[consoleplayer].message = ststuff_messages[STMSG_KFAADDED];
}

static void cheat_noclip(void)
{
  /* Simplified, accepting both "noclip" and "idspispopd". */
  /* no clipping mode cheat */
  players[consoleplayer].cheats ^= CF_NOCLIP;
  players[consoleplayer].message =
    ststuff_messages[players[consoleplayer].cheats & CF_NOCLIP
		     ? STMSG_NCON : STMSG_NCOFF];
}

/* 'behold?' power-up cheats (modified for infinite duration in Boom) */
static void cheat_pw(int pw)
{
  if (players[consoleplayer].powers[pw])
    players[consoleplayer].powers[pw] = pw!=pw_strength && pw!=pw_allmap;  /* killough */
  else
    {
#ifdef DIYBOOM
      if (compatibility)
      {
#endif
	if (!players[consoleplayer].powers[pw])
	  P_GivePower(&players[consoleplayer], pw);
	else if (pw != pw_strength)
	  players[consoleplayer].powers[pw] = 1;
	else
	  players[consoleplayer].powers[pw] = 0;
#ifdef DIYBOOM
      }
      else
      {
	P_GivePower(&players[consoleplayer], pw);
	if (pw != pw_strength)
	  players[consoleplayer].powers[pw] = -1; /* infinite duration */
      }
#endif
    }
  players[consoleplayer].message = ststuff_messages[STMSG_BEHOLDX];
}

/* 'behold' power-up menu */
static void cheat_behold(void)
{
  players[consoleplayer].message = ststuff_messages[STMSG_BEHOLD];
}

/* 'clev' change-level cheat */
static void cheat_clev(char *buf)
{
  int epsd, map;

  if (gamemode == commercial)
    {
      epsd = 1;
      map = (buf[0] - '0')*10 + buf[1] - '0';
    }
  else
    {
      epsd = buf[0] - '0';
      map = buf[1] - '0';
    }

  /* Catch invalid maps. */
  if (epsd < 1 || map < 1 ||   /* Ohmygod - this is not going to work. */
      (gamemode == retail     && (epsd > 4 || map > 9  )) ||
      (gamemode == registered && (epsd > 3 || map > 9  )) ||
      (gamemode == shareware  && (epsd > 1 || map > 9  )) ||
      (gamemode == commercial && (epsd > 1 || map > 32 )) )
    return; /* 8/14/98 allowed */

  /* So be it. */
  players[consoleplayer].message = ststuff_messages[STMSG_CLEV];
  G_DeferedInitNew(gameskill, epsd, map);
}

/* 'mypos' for player position */
static void cheat_mypos(void)
{
  sprintf(buf, "Position (%d,%d,%d)\tAngle %-.0f",
	  players[consoleplayer].mo->x >> FRACBITS,
	  players[consoleplayer].mo->y >> FRACBITS,
	  players[consoleplayer].mo->z >> FRACBITS,
	  players[consoleplayer].mo->angle * (90.0/ANG90));
  players[consoleplayer].message = buf;
}

#ifdef DIYBOOM
/* compatibility cheat */

static void cheat_comp(void)
{
  players[consoleplayer].message =
    (compatibility = !compatibility)
    ? "Compatibility Mode On" : "Compatibility Mode Off";
}

/* variable friction cheat */
static void cheat_friction(void)
{
  players[consoleplayer].message =
    (variable_friction = !variable_friction)
    ? "Variable Friction enabled" : "Variable Friction disabled";
}

/* Pusher cheat */
static void cheat_pushers(void)
{
  players[consoleplayer].message = (allow_pushers = !allow_pushers)
				   ? "Pushers enabled" : "Pushers disabled";
}
#endif

#if (defined(DIYTRANSPARENCY) && (LD_PIXEL_DEPTH > 3))
/* translucency cheat */
static void cheat_tnttran(void)
{
  players[consoleplayer].message =
    ststuff_messages[(EnableTransparency = !EnableTransparency)
		     ? STMSG_TRANSON : STMSG_TRANSOFF];
/*
  if (general_translucency && !main_tranmap)
    R_InitTranMap(0);
*/
}
#endif

static void cheat_massacre(void)
{
  /* kill all monsters */
  int killcount=0;
  thinker_t *currentthinker=&thinkercap;
  extern void A_PainDie(mobj_t *);

  while ((currentthinker=currentthinker->next)!=&thinkercap)
    if (currentthinker->function.acp1 == (actionf_p1) P_MobjThinker &&
	(((mobj_t *) currentthinker)->flags & MF_COUNTKILL ||
	 ((mobj_t *) currentthinker)->type == MT_SKULL))
      {
	if (((mobj_t *) currentthinker)->health > 0)
	  {
	    killcount++;
	    P_DamageMobj((mobj_t *)currentthinker, NULL, NULL, 10000);
	  }
	if (((mobj_t *) currentthinker)->type == MT_PAIN)
	  {
	    A_PainDie((mobj_t *) currentthinker);
	    P_SetMobjState ((mobj_t *) currentthinker, S_PAIN_DIE6);
	  }
      }
  sprintf(buf, "%d Monster%s Killed", killcount, killcount==1 ? "" : "s");
  players[consoleplayer].message = buf;
}

/* moved here from c.am_map */
static void cheat_ddt(void)
{
  extern int ddt_cheating;
  extern boolean automapactive, mixmap;
  if (automapactive || mixmap)
    ddt_cheating = (ddt_cheating+1) % 4;
}

#if 0
/* HOM autodetection */
static void cheat_hom(void)
{
  extern int autodetect_hom;
  players[consoleplayer].message = (autodetect_hom = !autodetect_hom) ? "HOM Detection On" :
    "HOM Detection Off";
}
#endif

static void cheat_fast(void)
{
  players[consoleplayer].message = (fastparm = !fastparm)
		  ? "Fast Monsters On" : "Fast Monsters Off";
  G_SetFastParms(fastparm);
}

static void cheat_tntkey(void)
{
  players[consoleplayer].message = "Red, Yellow, Blue";
}

static void cheat_tntkeyx(void)
{
  players[consoleplayer].message = "Card, Skull";
}

static void cheat_tntkeyxx(int key)
{
  players[consoleplayer].message = (players[consoleplayer].cards[key] = !players[consoleplayer].cards[key])
		  ? "Key Added" : "Key Removed";
}

/* generalised weapon cheats */

static void cheat_tntweap(void)
{
  players[consoleplayer].message = gamemode==commercial
		  ? "Weapon number 1-9" : "Weapon number 1-8";
}

static void cheat_tntweapx(char *buf)
{
  int w = *buf - '1';

  if ((w==wp_supershotgun && gamemode!=commercial) ||
      ((w==wp_bfg || w==wp_plasma) && gamemode==shareware))
    return;

  if (w==wp_fist) /* make '1' apply beserker strength toggle */
    cheat_pw(pw_strength);
  else
  {
    if (w >= 0 && w < NUMWEAPONS)
    {
      if ((players[consoleplayer].weaponowned[w] = !players[consoleplayer].weaponowned[w]))
	players[consoleplayer].message = "Weapon Added";
      else
      {
	  players[consoleplayer].message = "Weapon Removed";
	  if (w==players[consoleplayer].readyweapon) /* maybe switch if weapon removed */
	    players[consoleplayer].pendingweapon = wp_fist;
      }
    }
  }
}

/* generalised ammo cheats */
static void cheat_tntammo(void)
{
  players[consoleplayer].message = "Ammo 1-4, Backpack";
}

static void cheat_tntammox(char *buf)
{
  int a = *buf - '1';
  if (*buf == 'b')
    if ((players[consoleplayer].backpack = !players[consoleplayer].backpack))
      for (players[consoleplayer].message = "Backpack Added",   a=0 ; a<NUMAMMO ; a++)
	players[consoleplayer].maxammo[a] <<= 1;
    else
      for (players[consoleplayer].message = "Backpack Removed", a=0 ; a<NUMAMMO ; a++)
	{
	  if (players[consoleplayer].ammo[a] > (players[consoleplayer].maxammo[a] >>= 1))
	    players[consoleplayer].ammo[a] = players[consoleplayer].maxammo[a];
	}
  else
    if (a>=0 && a<NUMAMMO)
      { /* switch plasma and rockets for now -- KLUDGE  */
	a = a==am_cell ? am_misl : a==am_misl ? am_cell : a;  /* HACK */
	players[consoleplayer].message = (players[consoleplayer].ammo[a] = !players[consoleplayer].ammo[a]) ?
	  players[consoleplayer].ammo[a] = players[consoleplayer].maxammo[a], "Ammo Added" : "Ammo Removed";
      }
}

#if 0
static void cheat_smart(void)
{
  extern int monsters_remember;
  players[consoleplayer].message = (monsters_remember = !monsters_remember) ?
    "Smart Monsters Enabled" : "Smart Monsters Disabled";
}

static void cheat_pitch(void)
{
  extern int pitched_sounds;
  players[consoleplayer].message=(pitched_sounds = !pitched_sounds) ? "Pitch Effects Enabled" :
    "Pitch Effects Disabled";
}
#endif

static void cheat_homing(void)
{
  players[consoleplayer].message = ststuff_messages[P_MakeHomingMissiles(-1)
				   ? STMSG_HOMEON : STMSG_HOMEOFF];
}

static void cheat_jump(void)
{
  players[consoleplayer].message = ststuff_messages[(jump_cheat = !jump_cheat)
				   ? STMSG_JUMPON : STMSG_JUMPOFF];
}

static void cheat_float(void)
{
  if ((players[consoleplayer].mo->flags & MF_NOGRAVITY) == 0)
  {
    players[consoleplayer].mo->flags |= (MF_NOGRAVITY + MF_FLOAT);
    players[consoleplayer].message = ststuff_messages[STMSG_FLOATON];
  }
  else
  {
    players[consoleplayer].mo->flags &= ~(MF_NOGRAVITY + MF_FLOAT);
    players[consoleplayer].message = ststuff_messages[STMSG_FLOATOFF];
  }
}

static void cheat_fight(void)
{
  players[consoleplayer].message = ststuff_messages[(monster_infighting = !monster_infighting)
				   ? STMSG_FIGHTON : STMSG_FIGHTOFF];
}

#define CHEAT_ARGS_MAX 8  /* Maximum number of args at end of cheats */

#define ADDKEY(sr, key) \
  { \
    sr.high = (sr.high << 5) | (sr.low >> 27); \
    sr.low = (sr.low << 5) | (key); \
  }

int M_FindCheats(int key)
{
  static cheat_code_s sr;
  static char argbuf[CHEAT_ARGS_MAX+1], *arg;
  static int init, argsleft, cht;
  int i, ret, matchedbefore, mask;
  extern int has_dehack;

  if (!init)				/* initialize aux entries of table */
    {
      init = 1;
      for (i=0;cheat[i].cheat;i++)
	{
	  cheat_code_s c = sr, m = sr; /* sr == {0,0} */
	  const /*unsigned*/ char *p;
	  for (p=cheat[i].cheat; *p; p++)
	    {
	      unsigned key = tolower(*p)-'a';  /* convert to 0-31 */
	      if (key >= 32)		/* ignore most non-alpha cheat letters */
		continue;
	      ADDKEY (c, key);		/* shift key into code */
	      ADDKEY (m, 31);		/* shift 1's into mask */
	    }
	  cheat[i].code = c;		/* code for this cheat key */
	  cheat[i].mask = m;		/* mask for this cheat key */
	}
    }

  /* If we are expecting arguments to a cheat */
  /* (e.g. idclev), put them in the arg buffer */

  if (argsleft)
    {
      *arg++ = tolower(key);		/* store key in arg buffer */
      if (!--argsleft)			/* if last key in arg list, */
	cheat[cht].func(argbuf);	/* process the arg buffer */
      return 1;				/* affirmative response */
    }

  key = tolower(key) - 'a';
  if (key < 0 || key >= 32)		/* ignore most non-alpha cheat letters */
    {
      sr.high = sr.low = 0; /* clear shift register */
      return 0;
    }

  ADDKEY (sr, key);			/* shift this key into shift register */

  mask = (deathmatch ? not_dm : 0) |
	 ((netgame && !deathmatch) ? not_coop : 0) |
	 ((demorecording || demoplayback) ? not_demo : 0) |
	 (menuactive ? not_menu : 0) |
	 (has_dehack ? not_deh : 0);

  for (matchedbefore = ret = i = 0; cheat[i].cheat; i++)
    if ((sr.low & cheat[i].mask.low) == cheat[i].code.low &&
	(sr.high & cheat[i].mask.high) == cheat[i].code.high &&
	!(cheat[i].when & mask))	/* if match found & cheat allowed */
    {
      if (cheat[i].arg < 0)		/* if additional args are required */
	{
	  cht = i;			/* remember this cheat code */
	  arg = argbuf;			/* point to start of arg buffer */
	  argsleft = -cheat[i].arg;	/* number of args expected */
	  ret = 1;			/* responder has eaten key */
	}
      else
	if (!matchedbefore)		/* allow only one cheat at a time  */
	  {
	    matchedbefore = ret = 1;	/* responder has eaten key */
	    cheat[i].func(cheat[i].arg);/* call cheat handler */
	  }
    }
  return ret;
}
