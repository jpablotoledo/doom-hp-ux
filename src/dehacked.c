/*
 *  DeHackEd version 3.0a
 *  Written by Greg Lewis, gregl@umich.edu
 *  If you release any versions of this code, please include
 *  the author in the credits.  Give credit where credit is due!
 *
 *
 *  Merged with Doom by Andreas Dehmel (dehmel@forwiss.tu-muenchen.de) with kind
 *  permission of the author. The following code is a rather wild mixture of lots
 *  of c(pp) and h files of the DeHackEd 3.0 source.
 */




#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __riscos__
/* Trap errors on some strange SharedCLib versions */
#ifndef ENOENT
#define ENOENT		5
#endif
#include "ROsupport.h"
#endif


/* Import all variables that are changed by DeHackEd: */
#include "r_defs.h"
#include "d_event.h"
#include "doomstat.h"
#include "d_player.h"
#include "d_main.h"
#include "d_items.h"
#include "p_inter.h"
#include "hu_stuff.h"
#include "f_finale.h"
#include "st_stuff.h"
#include "wi_stuff.h"
#include "am_map.h"
#include "g_game.h"
#include "p_spec.h"
#include "p_local.h"
#include "p_pspr.h"
#include "sounds.h"
#include "info.h"
#include "m_swap.h"
#include "m_cheat.h"
#include "i_system.h"



/* Is DeHackEd support requested by the user? */
#ifdef INBUILT_DEHACKED




/* DeHackEd.h: */

/* Number of versions supported */
#define NUMDATA    10

/* Editing modes */
typedef enum {THING, FRAME, WEAPON, SOUND, SPRITE, TEXT, CODEP, CHEAT, AMMO,
	      MISC} EData;

/* Number of fields for some types */
#define THING_FIELDS  23
#define FRAME_FIELDS   7
#define SOUND_FIELDS   9
#define WEAPON_FIELDS  6

#define NUMCODEP 448


/* My own defines */
typedef enum {NO, YES} EBool;

/*
 *  Labelled indices of Thing array info.  The fields appear in exactly
 *  the order that they appear in the .exe, so they can be loaded
 *  very easily.
 *  Frame # fields end in 'frame', and Sound # fields end in 'sound'.
 *  (Go figure!)
 */

#define IDNUM             0
#define NORMALFRAME       1
#define HP			        2
#define MOVEFRAME         3
#define ALERTSOUND        4
#define REACTIONTIME      5
#define ATTACKSOUND       6
#define INJUREFRAME       7
#define PAINCHANCE        8
#define PAINSOUND         9
#define CLOSEATTACKFRAME 10
#define FARATTACKFRAME   11
#define DEATHFRAME       12
#define EXPLDEATHFRAME   13
#define DEATHSOUND       14
#define SPEED				 15
#define WIDTH				 16
#define HEIGHT				 17
#define MASS				 18
#define MISSILEDAMAGE	 19
#define ACTSOUND			 20
#define BITS				 21
#define RESPAWNFRAME     22

/* Labelled defines for Frame arrays */

#define SPRITENUM 0
#define SPRITESUB 1
#define DURATION  2
#define ACTIONPTR 3
#define NEXTFRAME 4
#define FZERO1    5
#define FZERO2    6

/* Constants for fields of Sound info. */

#define TEXTP    0
#define ZERO_ONE 1
#define VALUE    2
#define SZERO1   3
#define NEGONE1  4
#define NEGONE2  5
#define SZERO2   6
#define SZERO3   7
#define SZERO4   8

/* Fields for Weapon info. */

#define AMMOTYPE   0
#define BOB1FRAME  1
#define BOB2FRAME  2
#define BOB3FRAME  3
#define SHOOTFRAME 4
#define FIREFRAME  5

/* Fields for Misc info. */

#define MISCFIELDS  18
#define INITHEALTH  0
#define INITAMMO    1
#define MAXHEALTH   2
#define MAXARMOR    3
#define GREENCLASS  4
#define BLUECLASS   5
#define MAXSOUL     6
#define SOULHEALTH  7
#define MEGAHEALTH  8
#define GODHEALTH   9
#define IDFAARMOR  10
#define IDFACLASS  11
#define IDKFAARMOR 12
#define IDKFACLASS 13
#define BFGAMMO    14
#define INFIGHTING 15
#define HOMINGMISS 16	/* New */
#define PLAYERJUMP 17	/* New */


/*
 *  This is all of the information needed for one particular type of
 *  data from the exe.
 */
typedef struct DataSetS {
  void *offset;	/* Formerly offsets for this data, now a pointer to memory! */
  long length;	/* length of the entire data block */
  long numobj;	/* Number of objects in this data block */
  long objsize;	/* Size of each object */
  char *name;	/* The name of the data */
} DataSet;



/* End of DeHackEd.h */


/* Function prototypes */
static void Convertpatch(FILE *patchp, char patchformat);
static int  GetNextLine(char *nextline, int *numlines, FILE *patchp);
static int  LoadDiff(FILE *patchp);
static int  LoadOld(FILE *patchp, EBool commandline);
static int  Loadpatch(const char *filename, EBool commandline);
static void Preparefilename(const char *filename, char *fullname);
static int  ProcessLine(char *nextline, char **line2);


static EBool Printwindow(const char *message, int type);


/* Special string compares redefined on non-DOS platforms */
/* extern int strcasecmp(const char *str1, const char *str2);*/

#define strcmpi(a,b)	strcasecmp(a,b)
#define stricmp(a,b)	strcasecmp(a,b)


/* From data.h: */

/* This header contains lots of data arrays that are used elsewhere */
/* in the program and have no real home. */


/* Thing conversion array from 1.2 to 1.666 */
static const unsigned char thingconvar[104]={
  0,  11,   1,   2,  12,  13,  14,  18,  15,  19,
  21,  30,  31,  32,  16,  33,  34,  35,  37,  38,
  39,  41,  42,  43,  44,  45,  46,  47,  48,  49,
  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
  60,  61,  63,  64,  65,  66,  67,  68,  69,  70,
  71,  72,  73,  74,  75,  76,  77,  81,  82,  83,
  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,
  94,  95,  96,  97,  98,  99, 100, 101, 102, 103,
  104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
  114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
  124, 125, 126, 137
};

/* Frame conversion array from 1.2 to 1.666 */
static const int frameconvar[512]={
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
  30,  31,  49,  50,  51,  52,  53,  54,  55,  56,
  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,
  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,
  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,
  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,
  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
  522, 523, 524, 525, 526, 107, 108, 109, 110, 111,
  /* 100 */
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
  122, 123, 124, 125, 126, 127, 128, 129, 149, 150,
  151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
  181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
  191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
  201, 202, 207, 208, 209, 210, 211, 212, 213, 214,
  215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
  225, 226, 227, 228, 229, 230, 231, 232, 233, 234,
  /* 200 */
  235, 442, 443, 444, 445, 446, 447, 448, 449, 450,
  451, 452, 453, 454, 455, 456, 457, 458, 459, 460,
  461, 462, 463, 464, 465, 466, 467, 468, 469, 475,
  476, 477, 478, 479, 480, 481, 482, 483, 484, 485,
  486, 487, 488, 489, 490, 491, 492, 493, 494, 495,
  502, 503, 504, 505, 506, 507, 508, 509, 510, 511,
  512, 513, 514, 515, 527, 528, 529, 530, 531, 532,
  533, 534, 535, 536, 537, 538, 539, 540, 541, 542,
  543, 544, 545, 546, 547, 548, 585, 586, 587, 588,
  589, 590, 591, 592, 593, 594, 595, 596, 597, 598,
  /* 300 */
  599, 600, 601, 602, 603, 604, 605, 606, 607, 608,
  609, 610, 611, 612, 613, 614, 615, 616, 617, 618,
  619, 620, 621, 622, 623, 624, 625, 626, 627, 628,
  629, 630, 631, 674, 675, 676, 677, 678, 679, 680,
  681, 682, 683, 684, 685, 686, 687, 688, 689, 690,
  691, 692, 693, 694, 695, 696, 697, 698, 699, 700,
  130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
  140, 141, 802, 803, 804, 805, 806, 807, 808, 809,
  810, 811, 812, 816, 817, 818, 819, 820, 821, 822,
  823, 824, 825, 826, 827, 828, 829, 830, 831, 832,
  /* 400 */
  833, 834, 835, 836, 837, 838, 839, 840, 841, 842,
  843, 844, 845, 846, 847, 848, 849, 850, 851, 852,
  853, 854, 855, 856, 861, 862, 863, 864, 865, 866,
  867, 868, 869, 870, 871, 872, 873, 874, 875, 876,
  877, 878, 879, 880, 881, 882, 883, 884, 886, 887,
  888, 889, 890, 891, 892, 893, 894, 895, 896, 897,
  898, 899, 900, 901, 902, 903, 904, 905, 906, 907,
  908, 909, 910, 911, 912, 913, 914, 915, 916, 917,
  918, 919, 920, 921, 922, 923, 924, 925, 926, 927,
  928, 929, 930, 931, 932, 933, 934, 935, 936, 937,
  /* 500 */
  938, 939, 940, 941, 942, 943, 944, 945, 946, 947,
  948, 949
};

/* Sound conversion array from 1.2 to 1.666 */
static const unsigned char soundconvar[63] = {
  0,  1,  2,  3,  8,  9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 31, 32, 33, 34, 35, 36,
  37, 38, 39, 40, 41, 42, 43, 44, 45, 51,
  52, 55, 57, 59, 60, 61, 62, 63, 64, 65,
  66, 67, 68, 69, 75, 76, 77, 81, 82, 83,
  84, 85, 86
};

/* Sprite conversion array from 1.2 to 1.666 */
unsigned const char spriteconvar[105]={
  0,   1,   2,   3,   4,   5,   7,   8,   9,  10,
  11,  12,  13,  14,  15,  16,  17,  18,  19,  41,
  20,  21,  22,  23,  24,  25,  28,  29,  30,  39,
  40,  42,  44,  45,  49,  26,  55,  56,  57,  58,
  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,
  70,  71,  72,  73,  75,  76,  77,  78,  79,  80,
  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,
  91,  92,  94,  95,  96,  97,  98,  99, 100, 101,
  102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
  122, 123, 124, 125, 126
};



/* From dheinit.h */



static const char patchdir[1024]= "";			/* Patch file directory */


static const DataSet Data[NUMDATA] = {
  /* Global data for Things.  Refer to dehacked.h */
  /* complete struct explanation. */
  { NULL, 12696, 138, 92, "Thing" },

  /* Frame */
  { NULL, 27048, 966, 28, "Frame" },

  /* Weapon */
  { NULL, 216, 9, 24, "Weapon" },

  /* Sound */
  { NULL, 3852, 107, 36, "Sound" },

  /* Sprite */
  { NULL, 552, 138, 4, "Sprite" },

  /* Text */
  { NULL, 0, 0, 0, "Text" },

  /* Code pointers */
  { NULL, 3864, 966, 4, "Code Pointer" },

  /* Cheats */
  /* IDDT is always assumed to be 3624 bytes before the offset given. */
  /* If this ever changes... ick. */
  { NULL, 148, 17, 0, "Cheat" },

  /* Ammo */
  { NULL, 32, 8, 4, "Ammo" },

  /* Miscellaneous */
  /* The real offset material is stored in Miscoff. */
  { NULL, 64, MISCFIELDS, 4, "Misc" }
};


/* Order (field-wise) of things */
static const char thingorder[23] = {
  IDNUM, HP, SPEED, WIDTH, HEIGHT, MISSILEDAMAGE,
  REACTIONTIME, PAINCHANCE, MASS, BITS, ALERTSOUND,
  ATTACKSOUND, PAINSOUND, DEATHSOUND, ACTSOUND,
  NORMALFRAME, MOVEFRAME, INJUREFRAME, CLOSEATTACKFRAME,
  FARATTACKFRAME, DEATHFRAME, EXPLDEATHFRAME,
  RESPAWNFRAME
};



/* From print.h: */

/* Names for the different types of data... used in loading patches. */
/* Must be one word only. */
static const char *datanames[NUMDATA] = {
 "Thing",
 "Frame",
 "Weapon",
 "Sound",
 "Sprite",
 "Text",
 "Pointer",
 "Cheat",
 "Ammo",
 "Misc"
};

/* Name for all the fields of all the data types... whew... */
static const char *thingfields[23] = {
  "ID #",
  "Initial frame",
  "Hit points",
  "First moving frame",
  "Alert sound",
  "Reaction time",
  "Attack sound",
  "Injury frame",
  "Pain chance",
  "Pain sound",
  "Close attack frame",
  "Far attack frame",
  "Death frame",
  "Exploding frame",
  "Death sound",
  "Speed",
  "Width",
  "Height",
  "Mass",
  "Missile damage",
  "Action sound",
  "Bits",
  "Respawn frame"
};

static const char *soundfields[9] = {
  "Offset",
  "Zero/One",
  "Value",
  "Zero 1",
  "Neg. One 1",
  "Neg. One 2",
  "Zero 2",
  "Zero 3",
  "Zero 4"
};

static const char *framefields[7] = {
  "Sprite number",
  "Sprite subnumber",
  "Duration",
  "Action pointer",
  "Next frame",
  "Unknown 1",
  "Unknown 2"
};
static const char *weaponfields[6] = {
  "Ammo type",
  "Deselect frame",
  "Select frame",
  "Bobbing frame",
  "Shooting frame",
  "Firing frame"
};

static const char *miscfields[MISCFIELDS] = {
  "Initial Health",
  "Initial Bullets",
  "Max Health",
  "Max Armor",
  "Green Armor Class",
  "Blue Armor Class",
  "Max Soulsphere",
  "Soulsphere Health",
  "Megasphere Health",
  "God Mode Health",
  "IDFA Armor",
  "IDFA Armor Class",
  "IDKFA Armor",
  "IDKFA Armor Class",
  "BFG Cells/Shot",
  "Monsters Infight",
  "Homing Missiles",
  "Player Jump"
};

static char namelist[138][18] = {
  "Player",
  "Trooper",
  "Sargeant",
  "Archvile",
  "Archvile Attack",
  "Revenant",
  "Revenant Fireball",
  "Fireball Trail",
  "Mancubus",
  "Mancubus Fireball",
  "Chaingun Sargeant",

  "Imp",
  "Demon",
  "Spectre",
  "Cacodemon",
  "Baron of Hell",
  "Baron Fireball",
  "Hell Knight",
  "Lost Soul",
  "Spiderdemon",
  "Arachnotron",

  "Cyberdemon",
  "Pain Elemental",
  "SS Nazi",
  "Commander Keen",
  "Big Brain",
  "Demon Spawner",
  "Demon Spawn Spot",
  "Demon Spawn Cube",
  "Demon Spawn Fire",
  "Barrel",

  "Imp Fireball",
  "Caco Fireball",
  "Rocket (in air)",
  "Plasma Bullet",
  "BFG Shot",
  "Arach. Fireball",
  "Bullet Puff",
  "Blood Splat",
  "Teleport Flash",
  "Item Respawn Fog",

  "Teleport Exit",
  "BFG Hit",
  "Green Armor",
  "Blue Armor",
  "Health Potion",
  "Armor Helmet",
  "Blue Keycard",
  "Red Keycard",
  "Yellow Keycard",
  "Yellow Skull Key",

  "Red Skull Key",
  "Blue Skull Key",
  "Stim Pack",
  "Medical Kit",
  "Soul Sphere",
  "Invulnerability",
  "Berserk Sphere",
  "Blur Sphere",
  "Radiation Suit",
  "Computer Map",

  "Lite Amp. Visor",
  "Mega Sphere",
  "Ammo Clip",
  "Box of Ammo",
  "Rocket",
  "Box of Rockets",
  "Energy Cell",
  "Energy Pack",
  "Shells",
  "Box of Shells",

  "Backpack",
  "BFG 9000",
  "Chaingun",
  "Chainsaw",
  "Rocket Launcher",
  "Plasma Gun",
  "Shotgun",
  "Super Shotgun",
  "Tall Lamp",
  "Tall Lamp 2",

  "Short Lamp",
  "Tall Gr. Pillar",
  "Short Gr. Pillar",
  "Tall Red Pillar",
  "Short Red Pillar",
  "Pillar w/Skull",
  "Pillar w/Heart",
  "Eye in Symbol",
  "Flaming Skulls",
  "Grey Tree",

  "Tall Blue Torch",
  "Tall Green Torch",
  "Tall Red Torch",
  "Small Blue Torch",
  "Small Gr. Torch",
  "Small Red Torch",
  "Brown Stub",
  "Technical Column",
  "Candle",
  "Candelabra",

  "Swaying Body",
  "Hanging Arms Out",
  "One-legged Body",
  "Hanging Torso",
  "Hanging Leg",
  "Hanging Arms Out2",
  "Hanging Torso 2",
  "One-legged Body 2",
  "Hanging Leg 2",
  "Swaying Body 2",

  "Dead Cacodemon",
  "Dead Marine",
  "Dead Trooper",
  "Dead Demon",
  "Dead Lost Soul",
  "Dead Imp",
  "Dead Sargeant",
  "Guts and Bones",
  "Guts and Bones 2",
  "Skewered Heads",

  "Pool of Blood",
  "Pole with Skull",
  "Pile of Skulls",
  "Impaled Body",
  "Twitching Body",
  "Large Tree",
  "Flaming Barrel",
  "Hanging Body 1",
  "Hanging Body 2",
  "Hanging Body 3",

  "Hanging Body 4",
  "Hanging Body 5",
  "Hanging Body 6",
  "Pool Of Blood 1",
  "Pool Of Blood 2",
  "Brains",
  "Clipboard"
};



/* Some new variables */
#define TEXT_STRING_NUMBER	(10 + 22 + CCAST_NUMBER + FLUMP_NUMBER + PINTER_MESSAGES + PDOORS_MESSAGES + NUMSPRITES + CHAT_MACRO_NUMBER + MAPNAME_NUMBER + MAPNAME2_NUMBER + MAPNAMEP_NUMBER + MAPNAMET_NUMBER + STMSG_NUMBER + 2*ANIMATION_NUMBER + 2*SWITCHES_NUMBER + GGAME_NUMBER + (NUMSFX-1) + WILUMP_NUMBER)

typedef struct {
  char *name;
  int offset;
  int number;
} sort_strtab_t;

typedef struct {
  long sprname;
  long soundname;
  long soundlink;
} base_offset_t;

static char **textareas;
static state_t **codeptrs;
static actionf_t *actionfunctions;	/* copies of the state's actionpointers */
static sort_strtab_t *spritesorted;
static sort_strtab_t *soundsorted;
static int DeHackPatchVersion;


/*
 *  These offsets were determined using the DOS 1.9 executable that came with Doom2
 *  and the Simpsons patch file for 1.66 with Doom2. In the binary all the addresses
 *  (names, links) were relative to 0x88014. Looking at the rest of DeHackEd it appears
 *  that the offsets are assumed constant for all versions of the binary (see the
 *  original dehacked.c). Anyway, try emulating this using the following base
 *  offsets for various versions; you get the offset in the corresponding table by
 *  subtracting these base offsets from the addresses stored in the DEH file. To make
 *  sure we don't crash anything check whether these offsets are within the corresponding
 *  table. The offsets should be OK for the first 4 entries, but I have no idea what
 *  they look like in Doom 1.9U.
 */

static base_offset_t base_offset_vers[5] = {
  /* Doom1 1.66 */
  {0x24b58, 0x24790, 0x147c0},
  /* Doom2 1.66 */
  {0x24b58, 0x24790, 0x147c0},
  /* Doom2 1.7 */
  {0x24b58, 0x24790, 0x147c0},
  /* Doom2 1.9 */
  {0x24f20, 0x24b58, 0x148b0},
  /* DoomU 1.9 -- this is mere guessing! */
  {0x24f20, 0x24b58, 0x148b0}
};





/*
 *  Why do we need all this? Because the compiler is in no way forced to put the
 *  sprite / sound names sequentially, in ascending order into memory, like the
 *  PC one does. As a matter of fact GCC on RISC OS seems to always reverse the
 *  order, so we have to use another system or the pointers will be completely
 *  wrong. Therefore initialise the two sort_strtab_t arrays with the actual addresses
 *  of the strings and the offsets that are valid on DOS and look up those offsets.
 *  Let's just hope the names don't overlap or anything...
 *  Will only be used if DeHackEmulationLevel > 0.
 */

static char *AddressFromOffset(sort_strtab_t *table, int number, int offset)
{
  int pos, step, depth;

  /* Quick check whether a match _possible_ at all */
  if ((offset < 0) || (offset > table[number-1].offset)) return NULL;

  pos = (number+1) >> 1; step = (pos+1) >> 1; depth = pos << 1;

  while (depth != 0)
  {
    if ((table[pos].offset <= offset) && (offset < table[pos+1].offset))
      return (table[pos].name + offset - table[pos].offset);

    if (table[pos].offset < offset)
    {
      pos += step; if (pos >= number) pos = number-1;
    }
    else
    {
      pos -= step; if (pos < 0) pos = 0;
    }
    step = (step+1) >> 1;
    depth >>= 1;
  }
  return NULL;
}



/* Some re-writes of internally used functions: */
#define INFO	0
#define ERROR	1


static void AbortProg(const char *error)
{
  fprintf(logfile, "DeHackEd: %s\n", error);
  exit(-1);
}

static EBool Printwindow(const char *message, int type)
{
  fprintf(logfile, "DeHackEd: ");
  if (type == ERROR) fprintf(logfile, "Error: ");
  fprintf(logfile, "%s\n", message);
  return ((EBool)0);
}

void ApplyDehackPatch(const char *filename)
{
  int i, j, offset;
  char patchfile[1024];
  char *TextAreas[TEXT_STRING_NUMBER];
  state_t *CodePtrs[NUMCODEP];
  actionf_t ActionFunctions[NUMSTATES];
  sort_strtab_t SpriteSorted[NUMSPRITES+1];
  sort_strtab_t SoundSorted[NUMSFX];

  /* Make sure these only need memory when DeHackEd is active */
  textareas = TextAreas; codeptrs = CodePtrs; actionfunctions = ActionFunctions;
  spritesorted = SpriteSorted; soundsorted = SoundSorted;

  /* Init textareas */
  i = 0;
  TextAreas[i++] = retail_message;
  TextAreas[i++] = shareware_message;
  TextAreas[i++] = registered_message;
  TextAreas[i++] = commercial_message;
  TextAreas[i++] = pack_plut_message;
  TextAreas[i++] = pack_tnt_message;
  TextAreas[i++] = public_message;
  TextAreas[i++] = modified_banner;
  TextAreas[i++] = shareware_banner;
  TextAreas[i++] = commercial_banner;

  TextAreas[i++] = e1text;
  TextAreas[i++] = e2text;
  TextAreas[i++] = e3text;
  TextAreas[i++] = e4text;
  TextAreas[i++] = c1text;
  TextAreas[i++] = c2text;
  TextAreas[i++] = c3text;
  TextAreas[i++] = c4text;
  TextAreas[i++] = c5text;
  TextAreas[i++] = c6text;
  TextAreas[i++] = p1text;
  TextAreas[i++] = p2text;
  TextAreas[i++] = p3text;
  TextAreas[i++] = p4text;
  TextAreas[i++] = p5text;
  TextAreas[i++] = p6text;
  TextAreas[i++] = t1text;
  TextAreas[i++] = t2text;
  TextAreas[i++] = t3text;
  TextAreas[i++] = t4text;
  TextAreas[i++] = t5text;
  TextAreas[i++] = t6text;

  for (j=0; j<CCAST_NUMBER; j++)
  {
    TextAreas[i++] = castorder[j].name;
  }
  for (j=0; j<FLUMP_NUMBER; j++)
  {
    TextAreas[i++] = finale_lumps[j];
  }
  for (j=0; j<PINTER_MESSAGES; j++)
  {
    TextAreas[i++] = pinter_messages[j];
  }
  for (j=0; j<PDOORS_MESSAGES; j++)
  {
    TextAreas[i++] = pdoors_messages[j];
  }
  /* Build table for sorting in parallel. offset is the offset assumed by DeHackEd */
  offset = 0;
  for (j=0; j<NUMSPRITES; j++)
  {
    TextAreas[i++] = sprnames[j];
    SpriteSorted[j].name = sprnames[j]; SpriteSorted[j].offset = offset;
    SpriteSorted[j].number = j;
    /* All names start at word-aligned addresses */
    offset += ((strlen(sprnames[j]) + 4) & ~3);
  }
  SpriteSorted[j].offset = offset;
  for (j=0; j<CHAT_MACRO_NUMBER; j++)
  {
    TextAreas[i++] = chat_macros[j];
  }
  for (j=0; j<MAPNAME_NUMBER; j++)
  {
    TextAreas[i++] = mapnames[j];
  }
  for (j=0; j<MAPNAME2_NUMBER; j++)
  {
    TextAreas[i++] = mapnames2[j];
  }
  for (j=0; j<MAPNAMEP_NUMBER; j++)
  {
    TextAreas[i++] = mapnamesp[j];
  }
  for (j=0; j<MAPNAMET_NUMBER; j++)
  {
    TextAreas[i++] = mapnamest[j];
  }
  for (j=0; j<STMSG_NUMBER; j++)
  {
    TextAreas[i++] = ststuff_messages[j];
  }
#ifndef DIYBOOM
  for (j=0; j<ANIMATION_NUMBER; j++)
  {
    TextAreas[i++] = animdefs[j].endname;
    TextAreas[i++] = animdefs[j].startname;
  }
  for (j=0; j<SWITCHES_NUMBER; j++)
  {
    TextAreas[i++] = alphSwitchList[j].name1;
    TextAreas[i++] = alphSwitchList[j].name2;
  }
#endif
  for (j=0; j<GGAME_NUMBER; j++)
  {
    TextAreas[i++] = ggame_lumps[j];
  }
  /* Build table for sorting in parallel. See Sprites */
  offset = 0;
  /* Ignore the first one (PC binaries don't contain "none") */
  for (j=0; j<NUMSFX-1; j++)
  {
    TextAreas[i++] = S_sfx[j+1].name;
    SoundSorted[j].name = S_sfx[j].name; SoundSorted[j].offset = offset;
    SoundSorted[j].number = j;
    offset += ((strlen(S_sfx[j].name) + 4) & ~3);
  }
  SoundSorted[j].offset = offset;
  for (j=0; j<WILUMP_NUMBER; j++)
  {
    TextAreas[i++] = wistuff_lumps[j];
  }
  /*printf("Text: should: %d, is: %d\n", TEXT_STRING_NUMBER, i);*/

  /*for (i=0; i<TEXT_STRING_NUMBER; i++)
  {
    printf("%d: %s\n", i, TextAreas[i]);
  }*/

  i=0;
  for (j=0; j<NUMSTATES; j++)
  {
    if (states[j].action.acv != NULL)
    {
      if (i < NUMCODEP) CodePtrs[i] = states + j;
      i++;
    }
    ActionFunctions[j].acv = states[j].action.acv;
  }
  if (i > NUMCODEP)
  {
    fprintf(logfile, "Too many code pointers found (%d). What the fuck???\n", i);
  }

#if 0
  QuickSortStringTable(SpriteSorted, 0, NUMSPRITES-1);
  QuickSortStringTable(SoundSorted, 0, NUMSFX-2);

  fprintf(logfile, "Sprites:\n");
  OutputSortedStringTable(SpriteSorted, NUMSPRITES);
  fprintf(logfile, "Sounds:\n");
  OutputSortedStringTable(SoundSorted, NUMSFX-1);
#endif

  /*for (i=0; i<NUMSPRITES; i++)
  {
    fprintf(logfile, "off: %d, adr %p\n",
    8*i, AddressFromOffset(SpriteSorted, NUMSPRITES, 8*i));
  }*/

  /*
   * Can't say I know why but without copying this into a safe buffer things
   * can go phenomenally wrong...
   */
  strcpy(patchfile, filename);
  Loadpatch(patchfile, YES);

  /*printf("DeHackFile loaded\n");*/
  /*exit(0);*/
}



/* From file.c */

/* Converts a patch file from 1.2 to 1.666 format. */

#define COPY_STATE(x)	stat->##x = newstat->##x

static void Convertpatch(FILE *patchp, char patchformat)
{
  int i, j;
  long buffer[22];
  char forder[7] = {NORMALFRAME, MOVEFRAME, INJUREFRAME, CLOSEATTACKFRAME,
		    FARATTACKFRAME, DEATHFRAME, EXPLDEATHFRAME};
  char sorder[5] = {ALERTSOUND, ATTACKSOUND, PAINSOUND, DEATHSOUND, ACTSOUND};

  /* Cycle through every Thing in Doom 1.2, converting as we go. */
  for (i=0; i<103; i++)
  {
    fread(buffer, 88, 1, patchp);
#ifdef __BIG_ENDIAN__
    for (j=0; j<22; j++) buffer[j] = LONG(buffer[j]);
#endif
    for (j=0; j<5; j++)
      buffer[(unsigned int)sorder[j]] = soundconvar[buffer[(unsigned int)sorder[j]]];
    for (j=0; j<7; j++)
      buffer[(unsigned int)forder[j]] = frameconvar[buffer[(unsigned int)forder[j]]];
    memcpy(mobjinfo + thingconvar[i], buffer, 88);
  }

  /* Ammo is the same (read both arrays (int[4]) separately for security) */
  fread(maxammo, 4, 4, patchp);
  fread(clipammo, 4, 4, patchp);
#ifdef __BIG_ENDIAN__
  for (i=0; i<4; i++)
  {
    maxammo[i] = LONG(maxammo[i]);
    clipammo[i] = LONG(clipammo[i]);
  }
#endif
  for (i=0; i<8; i++)
  {
    fread(buffer,  24, 1, patchp);
#ifdef __BIG_ENDIAN__
    for (j=0; j<6; j++) buffer[j] = LONG(buffer[j]);
#endif
    for (j=0; j<5; j++)
      buffer[j+1] = frameconvar[buffer[j+1]];
    memcpy(weaponinfo + i, buffer, 24);
  }

  if (patchformat == 2)
  {
    state_t *stat, *newstat;

    newstat = (state_t*)buffer;

    for (i=0; i<512; i++)
    {
      fread(buffer, 28, 1, patchp);
#ifdef __BIG_ENDIAN__
      for (j=0; j<7; j++) buffer[j] = LONG(buffer[j]);
#endif
      buffer[SPRITENUM] = spriteconvar[buffer[SPRITENUM]];
      buffer[NEXTFRAME] = frameconvar[buffer[NEXTFRAME]];
      stat = states + frameconvar[i];
      COPY_STATE(sprite);
      COPY_STATE(frame);
      COPY_STATE(tics);
      COPY_STATE(nextstate);
      COPY_STATE(misc1);
      COPY_STATE(misc2);
    }
  }
}


/* Gets the next line of the patch file that's being loaded */

static int GetNextLine(char *nextline, int *numlines, FILE *patchp)
{
  char buffer[256], *line;
  int i;

  /* Continue looping until we find a non-whitespace and non-comment */
  /* line. */
  while (fgets(buffer, 255, patchp))
  {
    (*numlines)++;
    for (i = strlen(buffer); i && isspace((unsigned int)(buffer[i-1])); --i)
      ;

    buffer[i] = 0;

    for (line=buffer; *line && isspace((unsigned int)(*line)); ++line)
      ;

    if (!*line || (*line == '#'))
      continue;
    strcpy(nextline, line);

    return 1;
  }

  return 0;
}

/* Loads the new text file format.  Similar to CreateDiffSave... */

static int LoadDiff(FILE *patchp)
{
  int tempversion, patchformat;
  int  error = 0;
  char *nextline, *line2, *curText;
  int result;
  int curType = 0;
  int curNumber;
  int numlines = 1;
  EBool matched, valid, AbortLoop = NO;
  int i;
  long readValue, help;
  int *intPtr;
  int length1, length2;
  char *errormsg[3] = {
    "Line %d: No value after equal sign.",
    "Line %d: No value before equal sign.",
    "Line %d: Invalid single word line."
  };

  /* See Savepatch for file formats */

  /* Get memory for a line, and for text loading. */
#ifdef __cplusplus
  if ((nextline = new char[120]) == NULL)
    AbortProg("in LoadDiff");
#else
  if ((nextline = (char*)malloc(120*sizeof(char))) == NULL)
    AbortProg("in LoadDiff");
#endif

  /* Read in the text section from the exe file.  This prevents erroneous */
  /* Text reads when the same string is replaced several times in a messy */
  /* way. */

  /* Find an end-of-line somewere after the version number, which */
  /* should have been read in the main LoadPatch routine. */
  while (fgetc(patchp) != '\n') ;

  /* Process the whole file one line at a time.  GetNextLine returns 0 */
  /* at EOF. */
  while (GetNextLine(nextline, &numlines, patchp) && !AbortLoop)
  {
    /* Set matched to NO to be sure we catch any errors. Also assume it's */
    /* valid unless flagged otherwise. */
    matched = NO;
    valid = YES;

    /* Parse the line the for spaces or equal signs. */
    result = ProcessLine(nextline, &line2);

    /* Result determines what happened during parsing... whether we */
    /* have an error (negative) or an equals sign (1) or a space after */
    /* a word (2). */
    switch (result)
    {
      case 1:
	/* Found an equals sign.  Check for all possible lvalues */
	/* and take the appropriate action. */
	if (strcmpi(nextline, "doom version") == 0)
	{
	  matched = YES;
	  if (sscanf(line2, "%d", &tempversion) == 0)
	    tempversion = 0;
	  switch (tempversion)
	  {
	    case 12: DeHackPatchVersion = -1; break;
	    case 16: DeHackPatchVersion = 0; break;
	    case 20: DeHackPatchVersion = 1; break;
	    case 17: DeHackPatchVersion = 2; break;
	    case 19: DeHackPatchVersion = 3; break;
	    case 21: DeHackPatchVersion = 4; break;
	    default:
	      sprintf(nextline, "Line %d: Invalid Doom version number, assuming 1.9.", numlines);
	      AbortLoop = Printwindow(nextline, ERROR);
	      error = 2;
	      tempversion = 19;
	      break;
	  }
	}
	else if (strcmpi(nextline, "patch format") == 0)
	{
	  matched = YES;
	  if ((sscanf(line2, "%d", &patchformat) == 0) ||
	      (patchformat < 5))
	  {
	    sprintf(nextline, "Line %d: Invalid patch format number.", numlines);
	    AbortLoop = Printwindow(nextline, ERROR);
	    error = 1;
	  }

	  /* Check that the format is usable by this version. */
	  if (patchformat > 6)
	  {
	    strcpy(nextline, "This patch was created with a newer version of DeHackEd.");
	    Printwindow(nextline, ERROR);
	    strcpy(nextline, "You will need to upgrade to use this patch file.");
	    Printwindow(nextline, ERROR);
	    AbortLoop = YES;
	    error = 1;
	  }
	}
	/* For any particular mode that we're in there could be multiple */
	/* lvalues, stored in the "...fields" arrays.  Check them all. */
	else if (curType == THING)
	{
	  for (i=0; i<THING_FIELDS; i++)
	    if (strcmpi(nextline, thingfields[i]) == 0)
	    {
	      matched = YES;
	      if (sscanf(line2, "%ld", &readValue) == 0)
	      {
		valid = NO; break;
	      }

	      /* Not entirely clean, but better than 23 cases */
	      intPtr = (int*)(&mobjinfo[curNumber - 1]);
	      intPtr[i] = (int)readValue;

	      break;
	    }
	}
	else if (curType == FRAME)
	{
	  for (i=0; i<FRAME_FIELDS; i++)
	    if (strcmpi(nextline, framefields[i]) == 0)
	    {
	      matched = YES;
	      if (sscanf(line2, "%ld", &readValue) == 0)
	      {
		valid = NO; break;
	      }

	      switch (i)
	      {
		case 0: states[curNumber].sprite = (spritenum_t)readValue; break;
		case 1: states[curNumber].frame = readValue; break;
		case 2: states[curNumber].tics = readValue; break;
		case 3: /* This should be done via the code pointers */ break;
		case 4: states[curNumber].nextstate = (statenum_t)readValue; break;
		case 5: states[curNumber].misc1 = readValue; break;
		case 6: states[curNumber].misc2 = readValue; break;
		default: break;
	      }

	      break;
	    }
	}
	else if (curType == SOUND)
	{
	  for (i=0; i<SOUND_FIELDS; i++)
	    if (strcmpi(nextline, soundfields[i]) == 0)
	    {
	      matched = YES;
	      if (sscanf(line2, "%ld", &readValue) == 0)
	      {
		valid = NO; break;
	      }

	      /* The offsets in DeHackEd point to the pistol as the first entry. */
	      /* Since here the first one is "none" we have to add 1 tu curNumb. */
	      switch (i)
	      {
		case 0: if (DeHackEmulationLevel <= 0) break;
		  readValue -= base_offset_vers[DeHackPatchVersion].soundname;
		  if ((curText = AddressFromOffset(soundsorted, NUMSFX-1, (int)readValue)) == NULL)
		  {
		    fprintf(logfile, "Bad sound name offset at %d.\n", numlines);
		  }
		  else
		  {
		    S_sfx[curNumber+1].name = curText;
		  }
		  break;
		case 1: S_sfx[curNumber+1].singularity = (int)readValue; break;
		case 2: S_sfx[curNumber+1].priority = (int)readValue; break;
		case 3: if (readValue != 0)
		{
		  readValue -= base_offset_vers[DeHackPatchVersion].soundlink;
		  help = readValue / sizeof(sfxinfo_t);
		  if ((help < 0) || (help >= NUMSFX) || (help*sizeof(sfxinfo_t) != readValue))
		  {
		    fprintf(logfile, "Bad sound link at %d.\n", numlines);
		  }
		  else
		  {
		    S_sfx[curNumber+1].link = S_sfx + help;
		  }
		}
		break;
		case 4: S_sfx[curNumber+1].pitch = (int)readValue; break;
		case 5: S_sfx[curNumber+1].volume = (int)readValue; break;
		case 6: /* Writing the data before startup doesn't do anything */ break;
		case 7: /* This field is overwritten in S_Init */ break;
		case 8: /* Writing the lump number before startup doesn't do anything */ break;
		default: break;
	      }

	      break;
	    }
	}
	else if (curType == SPRITE)
	{
	  if (strcmpi(nextline, "offset") == 0)
	  {
	    matched = YES;
	    if (DeHackEmulationLevel > 0)
	    {
	      if (sscanf(line2, "%ld", &readValue) == 0)
	      {
		valid = NO; break;
	      }
	      readValue -= base_offset_vers[DeHackPatchVersion].sprname;
	      if ((curText = AddressFromOffset(spritesorted, NUMSPRITES, (int)readValue)) == NULL)
	      {
		fprintf(logfile, "Bad sprite name offset at %d.\n", numlines);
	      }
	      else
	      {
		sprnames[curNumber] = curText;
	      }
	    }
	  }
	}
	else if (curType == AMMO)
	{
	  if (strcmpi(nextline, "Max ammo") == 0)
	  {
	    matched = YES;
	    if (sscanf(line2, "%ld", &readValue) == 0)
	    {
	      valid = NO; break;
	    }
	    maxammo[curNumber] = (int)readValue;
	  }
	  else if (strcmpi(nextline, "Per ammo") == 0)
	  {
	    matched = YES;
	    if (sscanf(line2, "%ld", &readValue) == 0)
	    {
	      valid = NO; break;
	    }
	    clipammo[curNumber] = (int)readValue;
	  }

	}
	else if (curType == WEAPON)
	{
	  for (i=0; i<WEAPON_FIELDS; i++)
	    if (strcmpi(nextline, weaponfields[i]) == 0)
	    {
	      matched = YES;
	      if (sscanf(line2, "%ld", &readValue) == 0)
	      {
		valid = NO; break;
	      }

	      switch(i)
	      {
		case 0: weaponinfo[curNumber].ammo = (ammotype_t)readValue; break;
		case 1: weaponinfo[curNumber].upstate = (int)readValue; break;
		case 2: weaponinfo[curNumber].downstate = (int)readValue; break;
		case 3: weaponinfo[curNumber].readystate = (int)readValue; break;
		case 4: weaponinfo[curNumber].atkstate = (int)readValue; break;
		case 5: weaponinfo[curNumber].flashstate = (int)readValue; break;
		default: break;
	      }
	      break;
	    }
	}
	else if (curType == CHEAT)
	{
	  for (i=0; cheat[i].cheat; i++)
	    if (cheat[i].deh_cheat && !strcmpi (nextline, cheat[i].deh_cheat))
	    {
	      int j = 0;
	      char *p, *q;
	      matched = YES;
	      while (line2[j] != '\0' && line2[j] != (char)255)
	        j++;
	      line2[j] = '\0';
	      p = line2;
	      while (*p == ' ') ++p;
	      if ((q = malloc(strlen (p) + 1)) == NULL)
		AbortProg("in LoadDiff");
	      strcpy (q, p);
	      cheat[i].cheat = (const char *)q;
	      break;
	    }
	}
	else if (curType == CODEP)
	{
	  if (strcmpi(nextline, "Codep Frame") == 0)
	  {
	    matched = YES;
	    if (sscanf(line2, "%d", &length1) == 0)
	      valid = NO;
	    else
	    {
	      codeptrs[curNumber]->action.acv = actionfunctions[length1].acv;
	    }
	  }
	}
	else if (curType == MISC)
	{
	  for (i=0; i<Data[MISC].numobj; i++)
	  {
	    if (strcmpi(nextline, miscfields[i]) == 0)
	    {
	      matched = YES;
	      if (sscanf(line2, "%ld", &readValue) == 0)
	      {
		valid = NO; break;
	      }

	      switch (i)
	      {
		case 0: InitHealth = (int)readValue; break;
		case 1: InitAmmo = (int)readValue; break;
		case 2: MaxHealth = (int)readValue; break;
		case 3: MaxArmor = (int)readValue; break;
		case 4: GreenArmorClass = (int)readValue; break;
		case 5: BlueArmorClass = (int)readValue; break;
		case 6: MaxSoulsphere = (int)readValue; break;
		case 7: SoulsphereHealth = (int)readValue; break;
		case 8: MegasphereHealth = (int)readValue; break;
		case 9: GodHealth = (int)readValue; break;
		case 10: IDFAarmor = (int)readValue; break;
		case 11: IDFAarmorclass = (int)readValue; break;
		case 12: IDKFAarmor = (int)readValue; break;
		case 13: IDKFAarmorclass = (int)readValue; break;
		case 14: BFGcells = (int)readValue; break;
		case 15: monster_infighting = (readValue == 0xDD) ? true : false; break;
		case 16: if (readValue != 0) P_MakeHomingMissiles(1); break;
		case 17: if (readValue != 0) jump_cheat = true; break;
		default: break;
	      }

	      break;
	    }
	  }
	}
	break;
      case 2:
        /*
	 *  Found two words (or more) on a line.  Check for all
	 *  appropriate meanings and take the appropriate action.
	 *  These should be the section headers "Thing 1 (Player)".
         *
	 *  Compare the first word with all known object types
	 *  ("Thing", "Frame", etc...)
	 */
	matched = NO;
	for (i=0; i < NUMDATA; i++)
	  if (strcmpi(nextline, datanames[i]) == 0)
	  {
	    matched = YES;
	    curType = i;
	    if (sscanf(line2, "%d", &curNumber) == 0)
	    {
	      sprintf(nextline, "Line %d: Unreadable %s number.", numlines, datanames[i]);
	      AbortLoop = Printwindow(nextline, ERROR);
	      error = 2;
	    }
	    else if ((curNumber < 0) ||
		     ((curType != TEXT) && (curNumber > Data[curType].numobj)))
	    {
	      sprintf(nextline, "Line %d: %s number out of range.", numlines, datanames[i]);
	      AbortLoop = Printwindow(nextline, ERROR);
	      error = 2;
	    }
	    else if (curType == THING)
	    {
	      length1 = length2 = -1;
	      /* Find first ( */
	      for (i=0; i < strlen(line2); i++)
		if (line2[i] == '(')
		{
		  length1 = i+1;
		  break;
		}

	      /* Find last ) */
	      for (i=strlen(line2); i>= 0; i--)
		if (line2[i] == ')')
		{
		  length2 = i;
		  break;
		}

	      /* Provided we found a ( and a ), copy every in between */
	      /* to the namelist. */
	      if (length2 != -1 || length1 != -1)
	      {
		length2 -= length1;

		if (length2 > 17)
		  length2 = 17;
		strncpy(namelist[curNumber-1], line2+length1, length2);
		namelist[curNumber-1][length2] = 0;
	      }
	      break;
	    }
	  }

	/* This is a special case... gotta read the text directly */
	/* from the next line if we found a "Text" identifier. */
	if (curType == TEXT)
	{
	  if (sscanf(line2, "%d%d", &length1, &length2) < 2)
	  {
	    sprintf(nextline, "Line %d: Unreadable length value, aborting read.", numlines);
	    error = 1;
	    goto ErrorJump;
	  }
	  else
	  {
#ifdef __cplusplus
	    line2 = new char[length1+1];
#else
	    line2 = (char*)malloc((length1+1)*sizeof(char));
#endif

	    /* Just so ya know, I really hate doing this.  Anyways, */
	    /* read in whole string 1 char at a time, checking for */
	    /* 0D 0A combos, which are really new-lines. */
	    i = 0;
	    while (i<length1)
	    {
	      line2[i] = fgetc(patchp);
	      /* Ignore carriage returns */
	      if (line2[i] != 0x0D)
	      {
		if (line2[i] == '\n') numlines++;
		i++;
	      }
	    }
	    line2[length1] = 0;

	    /* OK, skip through the entire text section, trying to */
	    /* match up the string. */
	    valid = NO;
	    /* Changed completely: scan "textareas" */
	    for (i=0; i<TEXT_STRING_NUMBER; i++)
	    {
	      if (strcmp(textareas[i], line2) == 0)
	      {
		curNumber = i;
		valid = YES;
		break;
	      }
	    }

	    if (valid == YES)
	    {
	      /* Just so ya know, I really hate doing this again. */
	      /* Anyways, read in whole string 1 char at a time, */
	      /* checking for 0D 0A combos, which are really new- */
	      /* lines. */
	      curText = textareas[curNumber];
	      i = 0;
	      while ((i<length2) && (curText[i] != '\0'))
	      {
		curText[i] = fgetc(patchp);
		if (curText[i] != 0x0D)
		{
		  if (curText[i] == '\n') numlines++;
		  i++;
		}
	      }
	      curText[i] = 0;
	      while (i <length2)
	      {
	        char nextchar = fgetc(patchp);
	        if (nextchar != 0x0D)
	        {
	          if (nextchar == '\n') numlines++;
	          i++;
	        }
	      }
	    }
	    else
	    {
	      /* Keep numlines counter on track by scanning through */
	      /* worthless text section. */
	      i = 0;
	      while (i<length2)
	      {
		line2[0] = fgetc(patchp);
		if (line2[0] != 0x0D)
		{
		  if (line2[0] == '\n') numlines++;
		  i++;
		}
	      }
	      sprintf(nextline, "Line %d: Unmatchable text string.", numlines);
	      AbortLoop = Printwindow(nextline, ERROR);
	      error = 2;
	      valid = YES;
	    }
#ifdef __cplusplus
	    delete line2;
#else
	    free(line2);
#endif
	  }

	  /* Reset the current type */
	  curType = 0;
	}
	break;
      case -1:
      case -2:
      case -3:
	/* Error. Print error, and quit loading patch file entirely if */
	/* the user pressed Escape at the Printwindow prompt. */
	sprintf(nextline, errormsg[-result-1], numlines);
	AbortLoop = Printwindow(nextline, ERROR);
	error = 2;
	break;
    }

    /* Whoops, it didn't match anything at all... */
    if ((matched == NO) && (result >= 0))
    {
      sprintf(nextline, "Line %d: Unknown line.", numlines);
      AbortLoop = Printwindow(nextline, ERROR);
      error = 2;
    }

    /* Whoops, invalid number somewhere... */
    if (valid == NO)
    {
      sprintf(nextline, "Line %d: Unreadable value in a %s field.", numlines, datanames[curType]);
      AbortLoop = Printwindow(nextline, ERROR);
      error = 2;
    }
  }

  /* Jump straight here if we have an unrecoverable error.  Yeah, yeah, I */
  /* know, but gotos are the easiest way. */
 ErrorJump:

  /* Print different messages according to results. */
  if (AbortLoop)
    Printwindow("Patch file load aborted by user.", ERROR);
  else if (error == 0)
  {
#ifdef __cplusplus
    delete[] nextline;
#else
    free(nextline);
#endif
    return 0;
  }
  else if (error == 1)
    Printwindow(nextline, ERROR);
  else
    Printwindow("Patch file read, one or more errors detected.", ERROR);
#ifdef __cplusplus
  delete[] nextline;
#else
  free(nextline);
#endif
  return -1;
}


/* Loads a patch file, the old format. */
/* (Strangely enough these binary files seem to be in little endian order). */
static int LoadOld(FILE *patchp, EBool commandline)
{
  char buffer[80];
  char tempversion, patchformat;
  EBool error = YES;
  int i;
  state_t *bfst;
  mobjinfo_t *mobj;
#ifdef __BIG_ENDIAN__
  int j;
  long *b;
#endif

  fread(&tempversion, sizeof(char), 1, patchp);
  fread(&patchformat, sizeof(char), 1, patchp);

  if (patchformat == 3)
    strcpy(buffer, "Doom 1.6 beta patches are no longer valid.  Sorry!");
  else if (patchformat != 4)
    strcpy(buffer, "Bad patch version number!!");
  else if (tempversion > 20 || tempversion < 16)
    strcpy(buffer, "Bad Doom release number found!");
  else
    error = NO;

  if (error == YES)
  {
    Printwindow(buffer, ERROR);
    return -1;
  }

  switch (tempversion)
  {
    case 16: DeHackPatchVersion = 0; break;
    case 20: DeHackPatchVersion = 1; break;
    case 17: DeHackPatchVersion = 2; break;
    case 19: DeHackPatchVersion = 3; break;
    default: break;
  }

  /* OK, if it passes all the tests, load the sucker in. */
  for (i=0, mobj=mobjinfo; i<Data[THING].numobj-1; i++, mobj++)
  {
    fread(mobj, (size_t)Data[THING].objsize, 1, patchp);
#ifdef __BIG_ENDIAN__
    b = (long*)mobj;
    for (j=0; j<THING_FIELDS; j++) b[j] = LONG(b[j]);
#endif
  }

  /* Read in separately */
  fread(maxammo, 4, 4, patchp);
  fread(clipammo, 4, 4, patchp);
#ifdef __BIG_ENDIAN__
  for (j=0; j<4; j++)
  {
    maxammo[j] = LONG(maxammo[j]);
    clipammo[j] = LONG(clipammo[j]);
  }
#endif

  for (i=0; i<Data[WEAPON].numobj; i++)
  {
    fread((void*)(&weaponinfo[i]),  (size_t)Data[WEAPON].objsize,  1, patchp);
#ifdef __BIG_ENDIAN__
    b = (long*)(weaponinfo + i);
    for (j=0; j<WEAPON_FIELDS; j++) b[j] = LONG(b[j]);
#endif
  }

  /* Don't overwrite code pointers! */
  bfst = (state_t*)buffer;
  for (i=0; i<Data[FRAME].numobj; i++)
  {
    fread(buffer, (size_t)Data[FRAME].objsize, 1, patchp);
    states[i].sprite = LONG(bfst->sprite);
    states[i].frame = LONG(bfst->frame);
    states[i].tics = LONG(bfst->tics);
    states[i].nextstate = LONG(bfst->nextstate);
    states[i].misc1 = LONG(bfst->misc1);
    states[i].misc2 = LONG(bfst->misc2);
  }

  /* These sections ARE different between Doom 2 and Doom 1.666... */
  /* only load them in straight if they are the correct version. */
  if ((tempversion == 20 /*&& version == DOOM2_16*/) ||
      (tempversion == 16 /*&& version == DOOM1_16*/))
  {
    sfxinfo_t *bfsfx;
    long help, offs;
    char *curText;

    bfsfx = (sfxinfo_t*)buffer;
    /*
     *  Don't overwrite sensitive data (ptrs). Also start from 1 because the
     *  offsets used in the original DeHackEd pointed to "pistol", not "none".
     *  And make sure we don't forget the last one ==> <=Data[SOUND].numobj
     */
    for (i=1; i<=Data[SOUND].numobj; i++)
    {
      fread(buffer, Data[SOUND].objsize, 1, patchp);
      if (DeHackEmulationLevel > 0)
      {
	offs = LONG((long)(bfsfx->name)) - base_offset_vers[DeHackPatchVersion].soundname;
	if ((curText = AddressFromOffset(soundsorted, NUMSFX-1, (int)offs)) == NULL)
	{
	  fprintf(logfile, "Bad sound name offset (%ld)\n", offs + base_offset_vers[DeHackPatchVersion].soundname);
	}
	else
	{
	  S_sfx[i].name = curText;
	}
      }
      S_sfx[i].singularity = LONG(bfsfx->singularity);
      S_sfx[i].priority = LONG(bfsfx->priority);
      offs = LONG((long)(bfsfx->link));
      if (offs != 0)
      {
	offs -= base_offset_vers[DeHackPatchVersion].soundlink;
	help = offs / Data[SOUND].objsize;
	if ((offs < 0) || (help >= NUMSFX) || (help*sizeof(sfxinfo_t) != offs))
	{
	  fprintf(logfile, "Bad sound link (%ld)\n", offs + base_offset_vers[DeHackPatchVersion].soundlink);
	}
	else
	{
	  S_sfx[i].link = S_sfx + help;
	}
      }
      S_sfx[i].pitch = LONG(bfsfx->pitch);
      S_sfx[i].volume = LONG(bfsfx->volume);
      S_sfx[i].usefulness = LONG(bfsfx->usefulness);
    }
    if (DeHackEmulationLevel <= 0)
    {
      fseek(patchp, Data[SPRITE].objsize * Data[SPRITE].numobj, SEEK_CUR);
    }
    else
    {
      for (i=0; i<Data[SPRITE].numobj; i++)
      {
	fread(&offs, (size_t)Data[SPRITE].objsize, 1, patchp);
	offs = LONG(offs);
	offs -= base_offset_vers[DeHackPatchVersion].sprname;
	if ((curText = AddressFromOffset(spritesorted, NUMSPRITES, (int)offs)) == NULL)
	{
	  fprintf(logfile, "Bad sprite name offset (%ld)\n", offs + base_offset_vers[DeHackPatchVersion].sprname);
	}
	else
	{
	  sprnames[i] = curText;
	}
      }
    }
  }
  else
    Printwindow("Older patch format detected. Loading compatible data.", INFO);

  return 0;
}

/* Loads a patch file */

static int Loadpatch(const char *filename, EBool commandline)
{
  FILE *patchp;
  char tempversion;
  char fullname[1024] = "";
  char errmsg[1024];
  EBool error = NO;
  char idstring[30];

  /* Tack on the extension if there isn't one. */
  Preparefilename(filename, fullname);

  /* Try to open the file, return an error if we can't for some reason. */
  if ((patchp = fopen(fullname, "rb")) == NULL)
  {
    if (errno == ENOENT)
    {
      sprintf(errmsg, "File %s does not exist!", fullname);
      Printwindow(errmsg, ERROR);
      return -1;
    }
    else
    {
      sprintf(errmsg, "Error reading %s.", fullname);
      Printwindow(errmsg, ERROR);
      return -1;
    }
  }

  /* See Savepatch for file formats */

  fseek(patchp, 0, SEEK_SET);

  /* Read in the first character, and compare what it is to see if */
  /* we are dealing with one of the really old patch formats. */
  fread(&tempversion, sizeof(char), 1, patchp);

  /* Tempversion of 12 is Doom 1.2 */
  if (tempversion == 12)
  {
    /* The only patch formats for 1.2 are 1 and 2. */
    fread(&tempversion, sizeof(char), 1, patchp);
    if (tempversion != 1 && tempversion != 2)
    {
      Printwindow("Unknown patch file format!", ERROR);
      error = YES;
      goto ErrorJump;
    }

    Convertpatch(patchp, tempversion);
  }
  else if (tempversion == 'P')
  {
    /* OK, so it's one of the newer versions. */
    fread(idstring, 24, 1, patchp);
    idstring[24] = 0;
    if (stricmp(idstring, "atch File for DeHackEd v") != 0)
    {
      Printwindow("This is not a DeHackEd patch file!", ERROR);
      error = YES;
      goto ErrorJump;
    }

    /* Read in the version number, and convert it to an int to check */
    /* if we have a bad version. */
    fread(idstring, 3, 1, patchp);
    idstring[3] = 0;
    idstring[4] = (idstring[0]-'0')*10+(idstring[2]-'0');
    if (idstring[4] < 20)
    {
      Printwindow("This patch file has an incorrect version number!", ERROR);
      error = YES;
    }
    else if (idstring[4] < 23)
    {
      if (LoadOld(patchp, commandline) == -1)
	error = YES;
    }
    else
    {
      if (LoadDiff(patchp) == -1)
	error = YES;
    }
  }
  else
  {
    /* The catch-all */
    Printwindow("This is not a DeHackEd patch file!", ERROR);
    error = YES;
  }

 ErrorJump:
  fclose(patchp);

  if (error == YES)
    return -1;

  sprintf(errmsg, "Patch file %s read.", fullname);
  Printwindow(errmsg, INFO);
  return 0;
}


/* First, prepends the patchdir if a directory is not specified in the */
/* filename itself.  Next, finds the filename in a path\filenames combination */
/* and turns it into the familiar <8>.<3> combination if it's invalid. */
/* Also appends ".deh" if there is no extension. */

static void Preparefilename(const char *filename, char *fullname)
{
  int i = 0;
  char *realname, *dot;

#ifdef NORMALUNIX
  strcpy(fullname, filename);
#else
  /* Fix the patch filename, put on the patchdir if we need to. */
#ifdef __riscos__
  if ((strchr(filename, '$') == NULL) && (strchr(filename, ':') == NULL))
  {
    realname = getenv("DeHackEd$Dir");
    if (realname == NULL)
      strcpy(fullname,"Doom:DeHackEd.");
    else
      sprintf(fullname, "%s.", realname);
  }
#else
  if (filename[0] != '\\' && filename[1] != ':')
  {
    strcpy(fullname, patchdir);
    if (fullname[0] != 0)
      strcat(fullname, "\\");
  }
#endif
  strcat(fullname, filename);

  realname = fullname;

  /* Realname is a pointer to the actual filename, without the path.  So */
  /* step through the full path/filename combo, and keep moving the */
  /* current location of realname whenever we come to a directory separator. */
  while (fullname[i] != 0)
  {
#ifdef __riscos__
    if (fullname[i] == '.' || fullname[i] == ':')
#else
    if (fullname[i] == '\\' || fullname[i] == ':')
#endif
	realname = fullname+i+1;
    i++;
  }

  /* Try to find the extension of the filename */
  i = 0;
  while (realname[i] != 0 && realname[i] != EXTSEPCHR)
      i++;

  dot = realname + i;

  if (dot[0] == 0)
  {
    /* OK, no extension at all, so add our own at the correct place. */
    if (strlen(realname) > 8)
      realname[8] = 0;
    strcat(realname, DIRSEPSTR "deh");
  }
  else
  {
    if (dot - realname > 8)
    {
      strncpy(realname + 8, dot, 4);
      realname[12] = 0;
    }
    else
      dot[4] = 0;
  }
#endif	/* NORMALUNIX */
}


/*
 * This procedure processes a line from a patch file.  If an equals sign
 * exists in the input line, the following is done:
 *
 *     *next line
 *         \--->  first part  =  second part
 *              zero byte --^    ^--- *line2
 *
 * If there is no equals sign, the following is done:
 *
 *     *next line
 *         \--->  word1 word2 and other words
 *         zero byte --^^--- *line2
 *
 * Return values:
 *   1  Successful - found an equals sign
 *   2  Successful - found a word
 *  -1  No info after equals sign
 *  -2  No value before equals sign
 *  -3  No info after first word in line
 */
static int ProcessLine(char *nextline, char **line2)
{
  int i = 0, j = 0;

  /* Search line for an = */
  while (nextline[i] != 0 && nextline[i] != '=')
    i++;

  /* If we found one... */
  if (nextline[i] == '=')
  {
    /* Search for the first non-space after the =. */
    j = i--;
    while (isspace((unsigned int)nextline[++j]))
      ;

    /* It was all whitespace, error... should be equal to something */
    if (nextline[j] == 0)
      return -1;

    /* Set line2 to the first non-space after an = */
    *line2 = nextline+j;

    /* Kill any whitespace before the =... */
    while (i >= 0 && isspace((unsigned int)nextline[i]))
      i--;

    /* It was all whitespace, error... should be something before = */
    if (i == -1)
      return -2;

    /* OK, put in an end-of-string character to kill the space(s) */
    nextline[i+1] = 0;

    /* Successful */
    return 1;
  }
  /* Otherwise, the line should have two separate words on it */
  else
  {
    /* Search for first space on the line */
    while (nextline[j] != 0 && !isspace((unsigned int)nextline[j]))
      j++;

    /* Now search for the second word, (or, implicity, end of string) */
    i = j-1;
    while (isspace((unsigned int)nextline[++i]))
      ;

    /* No non-spaces after the first word (or only one word) */
    if (nextline[j] == 0)
      return -3;

    /* Set this to the first letter of the second word */
    *line2 = nextline+i;

    /* Terminate the first word's string */
    nextline[j] = 0;

    /* Successful */
    return 2;
  }
}


#endif	/* INBUILT DEHACKED */
