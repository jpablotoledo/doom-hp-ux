/*
 *  Copyright (C) 1998 by Andreas Dehmel.
 *
 *  Specific video- and system functionality for Doom It Yourself on RISC OS.
 *  Licensed under GPL.
 *
 */


#include <stdlib.h>
#include "ROsupport.h"
#include "GameSupp.h"

#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "i_sound.h"
#include "i_net.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"
#include "doomdef.h"
#include "m_menu.h"

#include "r_draw.h"
#include "r_context.h"
#include "r_state.h"
#include "r_main.h"
#include "z_zone.h"
#include "w_wad.h"







/* Define this if you want escape to exit Doom immediately */
/*#define ESCAPE_ABORTS_DOOM*/

/* Define this if you want left and right modifier keys treated identically */
#define LEFT_MODIFIER_KEYS

/*
 * 16/32bpp only: Define this if you want to use linear light levels.
 */
#define LINEAR_COLOURMAP_LIGHT	0x4000






/* signal handler type, valid for all libraries */
typedef void (*_sighandler_type_)(int);
#define SIGHANDLER_CAST		_sighandler_type_



#ifdef DIYBOOM
#define VIDEO_COLOURMAP		fullcolormap
#else
#define VIDEO_COLOURMAP		colormaps
#endif



static void I_SaveSprite(unsigned int num);

void ST_doRefresh(void);




/* Sprite area */
typedef struct {
  int  totalSize;
  int  numSprites;
  int  firstOff;
  int  firstFree;
  int  nextOff;
  char name[12];
  int  width;
  int  height;
  int  leftBit;
  int  rightBit;
  int  imgOff;
  int  maskOff;
  int  mode;
} sprite_area;





/*
 *  Global data:
 */

static int			this_frame = 0;
static int			clear_num_frames = 0;
static int			update_background; /*, update_status;*/
static game_supp_frame_buffer_t	*fbd;
static int		 	without_frame_buffers = 0;
static const byte*		last_palette = NULL;
boolean				PaletteChanged = false;
#if (LD_PIXEL_DEPTH == 3)
byte*			translated_colourmaps = NULL;
static byte*		pal_to_mode;
#ifdef DIYBOOM
static byte**		original_colourmaps = NULL;
#else
static byte*		original_colourmaps = NULL;
#endif
#else
/* We also need the 32nd colourmap (invulnerability) */
lighttable_t		translated_colourmaps[256*(NUMCOLORMAPS+1)];
static int		lightmultipliers[NUMCOLORMAPS];
/* Needed by the TranslateColourmaps* assembler functions */
static byte		TranslateColourmapsWork[256];
static int		last_gamma = -1;
#endif
static byte		gamma_palette[3*256];
static mouse_desc	lastMouse;
static int		HandlersActive = 0;

#ifdef DIYBOOM
lighttable_t		translated_basemap[256];
#endif

static char		coredumpfile[256];


/* exported to m_misc.c */
int	HardwareGamma;
int	BufferedKeyboard;
int	MinLightLevel;
int	InstHandlerFlags;





/*
 *  Only the right modifier keys or the left ones too?
 */

#ifdef LEFT_MODIFIER_KEYS
#define KEY_LSHIFT	KEY_RSHIFT
#define KEY_LCTRL	KEY_RCTRL
#else
#define KEY_LSHIFT	0
#define KEY_LCTRL	0
#endif



/* special keys on RISC OS (these are the symbols to use in doomrc) */
#define KEY_SCRLOCK     0xe0
#define KEY_INSERT      0xe1
#define KEY_HOME        0xe2
#define KEY_PAGEUP      0xe3
#define KEY_CAPSLOCK    0xe4
#define KEY_NUMLOCK     0xe5
#define KEY_PAGEDOWN    0xe6
#define KEY_COPY        0xe7


/* Bitfield: 1 for key pressed (internal key numbers) */
static unsigned char riscos_keys[128/8];

/*
 *  Translation tables: internal keynumbers -> Doom keynumbers
 *  Removed multiple mappings (e.g. 2 -> 49,71) which caused problems.
 *  The superfluous definitions are put in parantheses in the commentary.
 */
static unsigned char intkey_table_buff[128] = {
  KEY_ESCAPE, KEY_F1, KEY_F2, KEY_F3,	/* Esc, F1, F2, F3 */
  KEY_F4, KEY_F5, KEY_F6, KEY_F7,  	/* F4, F5, F6, F7 */
  KEY_F8, KEY_F9, KEY_F10, KEY_F11,	/* F8, F9, F10, F11 */
  KEY_F12, KEY_PRINT, 0, 0,		/* F12, Print, ScrlLk, Break */
  KEY_BACKQUOTE, '1', '2', '3',		/* `, 1, 2, 3 */
  '4', '5', '6', '7',			/* 4, 5, 6, 7 */
  '8', '9', '0', KEY_MINUS,		/* 8, 9, 0, - */
  '=', '£', KEY_BACKSPACE, 0,		/* =, £¹, <-|, Insert */
  0, 0, 0, '/',				/* Home, PgUp, NumLk, kp/ */
  '*', '#', KEY_TAB, 'q',		/* kp*, kp#, Tab, Q */
  'w', 'e', 'r', 't',  			/* W, E, R, T */
  'y', 'u', 'i', 'o',			/* Y, U, I, O */
  'p', '[', ']', '\\',			/* P, [, ], \ (# on Risc PC kbd) */
  KEY_BACKSPACE, KEY_PAUSE, 0, '7',	/* Del, Copy, PgDn, kp7 */
  '8', '9', '-', KEY_LCTRL,		/* kp8, kp9, kp-, lCtrl */
  'a', 's', 'd', 'f',			/* A, S, D, F */
  'g', 'h', 'j', 'k',			/* G, H, J, K */
  'l', ';', '\'', KEY_ENTER,		/* L, ;, ', Return */
  '4', '5', '6', '+', 			/* kp4, kp5, kp6, kp+ */
  KEY_LSHIFT, '\\', 'z', 'x',		/* lShift, (\ on RPC kbd), Z, X */
  'c', 'v', 'b', 'n',			/* C, V, B, N */
  'm', ',', '.', '/',			/* M, ',', ., / */
  KEY_RSHIFT, KEY_UPARROW, '1', '2',	/* rShift, Up, kp1, kp2 */
  '3', 0, KEY_LALT, ' ',		/* kp3, CapsLk, lAlt, Space */
  KEY_RALT, KEY_RCTRL, KEY_LEFTARROW, KEY_DOWNARROW,
					/* rAlt, rCtrl, Left, Down */
  KEY_RIGHTARROW, '0', '.', KEY_ENTER	/* Right, kp0, kp., Enter */
};

static unsigned char intkey_table_raw[128] = {
  0, 0, 0, KEY_LSHIFT,			/* 000: shift, ctrl, alt, shiftL */
  KEY_LCTRL, KEY_LALT, KEY_RSHIFT, KEY_RCTRL,	/* 004: ctrll, altl, shiftr, ctrlr */
  KEY_RALT, 0, 0, 0,			/* 008: altr, select, menu, adjust */
  0, 0, 0, 0,				/* 012: void */
  'q', '3', '4', '5',			/* 016: q, 3, 4, 5 */
  KEY_F4, '8', KEY_F7, KEY_MINUS,	/* 020: F4, 8, F7, - */
  '6', KEY_LEFTARROW, '6', '7',		/* 024: 6, crsrl, num6, num7 */
  KEY_F11, KEY_F12, KEY_F10, KEY_SCRLOCK,/* 028: F11, F12, F10, ScrollLock */
  KEY_PRINT, 'w', 'e', 't',		/* 032: Print, w, e, t */
  '7', 'i', '9', '0',			/* 036: 7, i, 9, 0 */
  0, KEY_DOWNARROW, '8', '9',		/* 040: (-), crsrd, num8, num9 */
  KEY_PAUSE, KEY_BACKQUOTE, '£', KEY_BACKSPACE,	/* 044: Break, `, £, Del */
  '1', '2', 'd', 'r',			/* 048: 1, 2, d, r */
  0, 'u', 'o', 'p',			/* 052: (6), u, o, p */
  '[', KEY_UPARROW, '+', KEY_MINUS,	/* 056: [, crsru, num+, num- */
  KEY_ENTER, KEY_INSERT, KEY_HOME, KEY_PAGEUP, /* 060: numEnter, Insert, Home, PgUp */
  KEY_CAPSLOCK, 'a', 'x', 'f',		/* 064: CapsLck, a, x, f */
  'y', 'j', 'k', 0,			/* 068: y, j, k, (2) */
  ';', KEY_ENTER, '/', 0,		/* 072: ;, Return, num/, void */
  '.', KEY_NUMLOCK, KEY_PAGEDOWN, '\'',	/* 076: num., numLck, PgDown, ' */
  0, 's', 'c', 'g',			/* 080: void, s, c, g */
  'h', 'n', 'l', 0,			/* 084: h, n, l, (;) */
  ']', KEY_BACKSPACE, '#', '*',		/* 088: ], Delete, num#, num* */
  0, KEY_EQUALS, '\\', 0,		/* 092: void, =, Extra, void */
  KEY_TAB, 'z', ' ', 'v',		/* 096: Tab, z, Space, v */
  'b', 'm', ',', '.',			/* 100: b, m, ',', . */
  '/', KEY_COPY, '0', '1',		/* 104: /, Copy, num0, num1 */
  '3', 0, 0, 0,				/* 108: num3, void */
  KEY_ESCAPE, KEY_F1, KEY_F2, KEY_F3,	/* 112: Esc, F1, F2, F3 */
  KEY_F5, KEY_F6, KEY_F8, KEY_F9,	/* 116: F5, F6, F8, F9 */
  '\\', KEY_RIGHTARROW, '4', '5',	/* 120: \, crsrr, num4, num5 */
  '2', 0, 0, 0				/* 124: num2, void */
};

static unsigned char *intkey_table = intkey_table_raw;




/* SharedCLib and UnixLib have completely different signal numbers! */
static const char *abort_codes[] = {
#ifdef __UNIXLIB_TYPES_H
  "SIGNONE",
  "SIGHUP",
  "SIGINT",
  "SIGQUIT",
  "SIGILL",
  "SIGTRAP",
  "SIGIOT",
  "SIGEMT",
  "SIGFPE",
  "SIGKILL",
  "SIGBUS",	/* 10 */
  "SIGSEGV",
  "SIGSYS",
  "SIGPIPE",
  "SIGALARM",
  "SIGTERM",
  "SIGURG",
  "SIGSTOP",
  "SIGTSP",
  "SIGCONT",
  "SIGCHLD",	/* 20 */
  "SIGTTIN",
  "SIGTTOU",
  "SIGIO",
  "SIGXCPU",
  "SIGXFSZ",
  "SIGVTALRM",
  "SIGPROF",
  "SIGWINCH",
  "SIGINFO",
  "SIGUSR1",	/* 30 */
  "SIGUSR2",
  "SIGLOST",
  "SIGERR"
#else
  "SIGNONE",
  "SIGABRT",
  "SIGFPE",
  "SIGILL",
  "SIGINT",
  "SIGSEGV",
  "SIGTERM",
  "SIGSTAK",
  "SIGUSR1",
  "SIGUSR2",
  "SIGOSERROR"
#endif
};




static int InAbortHandler = 0;

#ifndef __UNIXLIB_TYPES_H
static int AbortEnvironmentCorrupted;
#endif

  /* for Shared C Library: do postmortem stack backtrace */
static void do_stack_backtrace(void)
{
#ifndef __UNIXLIB_TYPES_H
  _kernel_unwindblock ub;
  int status;
  char *language;
  int slotsize;
  const int* regs;
  unsigned int pcmask = (RunningIn32bitMode()) ? 0xffffffff : 0x03fffffc;

  /*
   *  OK, folks, this is an absolutely _horrible_ hack! It should work with
   *  the currently used APCS format, but boy is it ugly. The root of the
   *  problem is that at least in my version of the SCL _kernel_procname()
   *  simply doesn't work. Fuck!
   */
  ReadProcessorRegisters(&ub);
  slotsize = WimpReadSlotSize();
  language = _kernel_language(ub.pc & pcmask);

  if (AbortEnvironmentCorrupted != 0)
  {
    fprintf(logfile, "Abort from non-C environment, had fix... fingers crossed.\n");
    fflush(logfile);
  }

  if ((regs = ReadExceptionRegisters()) != NULL)
  {
    unsigned int *faultadr;

    fprintf (logfile,
	     "  r0 = %08X  r1 = %08X  r2 = %08X  r3 = %08X\n"
	     "  r4 = %08X  r5 = %08X  r6 = %08X  r7 = %08X\n"
	     "  r8 = %08X  r9 = %08X  sl = %08X  fp = %08X\n"
	     "  ip = %08X  sp = %08X  lr = %08X  pc = %08X\n",
	     regs[0], regs[1], regs[2], regs[3],
	     regs[4], regs[5], regs[6], regs[7],
	     regs[8], regs[9], regs[10], regs[11],
	     regs[12], regs[13], regs[14], regs[15]);

    faultadr = (unsigned int*)(regs[15] & pcmask);
    /* abort in plotters? ==> word following faulting address contains PLOT */
    if (faultadr[1] == 0x544f4c50)
    {
      if ((faultadr[2] >= 0x8000) && (faultadr[2] < 0x8000 + slotsize))
      {
        fprintf(logfile, "---> Abort in plotter %s\n", (char*)(faultadr[2]));
      }
    }
  }
  do
  {
    char *proc=NULL;

    /*proc = _kernel_procname(ub.pc & pcmask);*/
    /* is the frame pointer within the slot and larger than the stack pointer? */
    if ((ub.fp >= 0x8000) & (ub.fp < 0x8000 + slotsize) && (ub.fp > ub.sp))
    {
      int func;

      /*
       *  If the pc is stored on the stack it's in the top position. Check
       *  if the word stored there is within the slot.
       */
      func = *((int*)(ub.fp)) & pcmask;
      if ((func >= 0x8000) && (func < 0x8000 + slotsize))
      {
        int tries=4;
        int *fptr;

        fptr = (int*)(func-16);	/* minimum offset to function start */
        while (tries > 0)
        {
          if ((*fptr & 0xffffff00) == 0xff000000) break;
          fptr--; tries--;
        }
        /* found marker, use offset for function name */
        if (tries > 0) proc = ((char*)fptr) - (*fptr & 0xff);
      }
    }
    fprintf(logfile,
	    "In procedure %s:\n"
	    "  r4=%08x r5=%08x r6=%08x r7=%08x r8=%08x r9=%08x\n"
	    "  fp=%08x sp=%08x pc=%08x sl=%08x\n",
	    (proc == NULL) ? "?" : proc,
	    ub.r4, ub.r5, ub.r6, ub.r7, ub.r8, ub.r9,
	    ub.fp, ub.sp, ub.pc, ub.sl);
    status = _kernel_unwind(&ub, &language);
  }
  while (status > 0);
  fflush(logfile);
#endif
}


/*
 *  Called on escape or segmentation violation. Vital to avoid serious crashes.
 *  Deinstalls all sorts of handlers (video, sound) before exiting the program.
 */
static void handle_abort(int code)
{
  if (InAbortHandler != 0) exit(0);

  InAbortHandler = 1;

  I_ShutdownGraphics();
  I_ShutdownSound();
  I_ShutdownNetwork();
  fprintf(logfile, "Emergency Exit (%s)\n", abort_codes[code]);
  fflush(logfile);

  do_stack_backtrace();

  /* core dump */
  if (coredumpfile[0] != '\0')
  {
    FILE *fp;

    if ((fp = fopen(coredumpfile, "wb")) != NULL)
    {
      int slotsize;

      slotsize = WimpReadSlotSize();
      fwrite((void*)0x8000, 1, slotsize, fp);
      fclose(fp);
    }
  }
  I_ReleaseMemory();
  exit(0);
}


static void handle_exit(void)
{
  if (HandlersActive != 0)
  {
    I_ShutdownGraphics();
    I_ShutdownSound();
    I_ShutdownNetwork();
    fprintf(logfile, "Extraordinary Exit\n"); fflush(logfile);

    do_stack_backtrace();
  }

  if ((InstHandlerFlags & IHFlag_Abort) != 0)
    GameSupp_RemoveAbortGuard();

  fclose(logfile);
}





/*
 *  If you don't want escape to abort Doom: ignore escape presses
 */
#ifndef ESCAPE_ABORTS_DOOM
static void ignore_escape(int code)
{
  signal (SIGINT, (SIGHANDLER_CAST) ignore_escape);
}
#endif




static void RO_ScanKeyboard(void)
{
  event_t event;

  if (BufferedKeyboard)
  {
    int key;

    while ((key=GameSupp_GetKeyPress())!=-1) {
      event.data1 = intkey_table[key>>1];
      event.type = (key & 1) ? ev_keydown : ev_keyup;
      D_PostEvent(&event);
      /*printf("Key <%3d:%3d>\r", event.data1, event.type);*/
    }
  }
  else
  {
    unsigned int from, i;
    unsigned char newkeys[128/8], map;

    /* Clear new keys */
    memset(newkeys, 0, 128/8);

    from = 0;
    while (from < 128)
    {
      from = ScanKeyboard(from);
      if (from < 128)
      {
        newkeys[from>>3] |= (1<<(from&7));
        from++;
      }
    }
    /* Check for changes in key positions */
    for (from=0; from<128/8; from++)
    {
      if ((map = (riscos_keys[from] ^ newkeys[from])) != 0)
      {
        for (i=0; i<8; i++)
        {
          if ((map & (1<<i)) != 0)
          {
            if (intkey_table[(from<<3) + i] != 0)
            {
              event.data1 = intkey_table[(from<<3) + i];
              if ((newkeys[from] & (1<<i)) == 0)
              {
                event.type = ev_keyup;
              }
              else
              {
                event.type = ev_keydown;
              }
              D_PostEvent(&event); /*printf("Key <%d:%d>\n", event.data1, event.type);*/
            }
          }
        }
      }
    }

    /* Finally copy the new keyboard state */
    memcpy(riscos_keys, newkeys, 128/8);
  }
}



/*
 *  Called when some things that are usually constant in all frames (background,
 *  status bar background) change. All frame buffers must be updated when they're
 *  built in that case.
 */
void I_InvalidateArea(unsigned int flags)
{
  if ((flags & INVAL_BACKGRND) != 0) update_background = fbd->buffers - 1;
}



/* The following two functions are used by f_wipe */
pixel_t *I_GetDisplayedScreen(void)
{
  if (without_frame_buffers != 0)
  {
    pixel_t *scr;

    scr = (fbd->buffers < 2) ? ((pixel_t*)(screens[2])) : ((pixel_t*)(fbd->screens[1]));
    memcpy(scr, screens[0], SCREENWIDTH*SCREENHEIGHT*sizeof(pixel_t));
    return scr;
  }
  else
  {
    /* Mark all non-visible frames as empty */
    fbd->plot = 0;
    return (pixel_t*)(fbd->screens[fbd->show]);
  }
}


/* Get a 3rd frame buffer if possible */
pixel_t *I_GetSpareScreen(void)
{
  int number;

  if (without_frame_buffers != 0)
  {
    if (fbd->buffers < 3) return screens[3];
    return (pixel_t*)(fbd->screens[2]);
  }

  if (fbd->buffers < 3) return screens[3];

  /* Find the buffer that's neither displayed nor currently plotted into */
  number = fbd->show;
  do
  {
    number++;
    if (number >= fbd->buffers) number -= fbd->buffers;
    if (number != this_frame) break;
  }
  while (number != fbd->show);

  return (pixel_t*)(fbd->screens[number]);
}



void I_StartFrame(void)
{
  if (without_frame_buffers == 0)
  {
    while ((this_frame = GameSupp_GetNextFrame()) < 0);
    screens[0] = (pixel_t*)(fbd->screens[this_frame]);
    if (clear_num_frames > 0)
    {
      memset(screens[0], 0, fbd->width * fbd->height * sizeof(pixel_t));
      clear_num_frames--;
    }
  }

  /* Reset cursor position to disable scrolling if necessary */
  if (logfile == stderr) HomeCursor();

  /*if (update_status > 0)
  {
    ST_doRefresh(); update_status--;
  }*/
  if (update_background > 0)
  {
    R_DrawViewBorder(); update_background--;
  }
}



void I_StartTic(void)
{
  event_t event;
  mouse_desc newMouse;
  int buts, dx, dy;

  RO_ScanKeyboard();

  ReadMouseState(&newMouse);

  /* Has anything changed? */
  buts = lastMouse.buttons ^ newMouse.buttons;
  /* Take into account that mouse coordinates are in OS units! eigx, eigy >= 1! */
  dx = 2*(newMouse.x - (SCREENWIDTH << (fbd->eigx - 1)));
  dy = 2*(newMouse.y - (SCREENHEIGHT << (fbd->eigy - 1)));
  if ((buts != 0) || (dx != 0) || (dy != 0))
  {
    event.type = ev_mouse;
    event.data1 =
      ((newMouse.buttons & 1) << 2) | (newMouse.buttons & 2) | ((newMouse.buttons & 4) >> 2);
    event.data2 = dx; event.data3 = dy;
    D_PostEvent(&event);
    /*lastMouse.x = newMouse.x; lastMouse.y = newMouse.y; lastMouse.time = newMouse.time;*/
    lastMouse.buttons = newMouse.buttons;
    PutMouseAt(SCREENWIDTH << (fbd->eigx - 1), SCREENHEIGHT << (fbd->eigy - 1));
  }
}




/* Translate the specified palette according to the currently active gamma values */
static void I_TranslatePalette(const byte *palette)
{
  unsigned int *p;
  const byte *gamma;
  int i;

  p = (unsigned int *)gamma_palette;
  gamma = gammatable[usegamma];

  for (i=0; i<192; i++)
  {
    *p++ = gamma[(unsigned char)(palette[0])] | (gamma[(unsigned char)(palette[1])] << 8) |
    (gamma[(unsigned char)(palette[2])] << 16) | (gamma[(unsigned char)(palette[3])] << 24);
    palette += 4;
  }
}




/*
 *  This function is called for old style 8bpp every time the palette changes.
 *  For more see I_SetPalette.
 */
void I_TranslateColourmaps(const byte *palette)
{
#if (LD_PIXEL_DEPTH == 3)
  if (fbd->colours != 255)
  {
    unsigned char maptable[256];
    unsigned int *cmapi;
    int i, r, g, b;
    byte *gpal;
    byte *origmap;

    fprintf(logfile, "I_TranslateColourmaps: Have to remap colourtables\n");
    I_TranslatePalette(palette);

    /* Build a lookup table for mapping Doom colours to mode colours */
    gpal = gamma_palette;
    for (i=0; i<256; i++)
    {
      r = (*gpal++ + 15) >> 4; g = (*gpal++ + 15) >> 4; b = (*gpal++ + 15) >> 4;
      if (r > 15) r = 15; if (g > 15) g = 15; if (b > 15) b = 15;
      maptable[i] = pal_to_mode[r | (g<<4) | (b<<8)];
    }
#ifdef DIYBOOM
    for (i=0; i<numcolormaps; i++)
    {
      if (colormaps[i] == fullcolormap) break;
    }
    if (i >= numcolormaps) i = 0;
    origmap = original_colourmaps[i];
#else
    origmap = original_colourmaps;
#endif
    /* We also have to translate the inverted colourmap (no. 32)! */
    cmapi = (unsigned int*)VIDEO_COLOURMAP;
    for (i=0; i<256*(NUMCOLORMAPS+1); i+=4)
    {
      *cmapi++ = maptable[(unsigned char)(origmap[i])] |
                (maptable[(unsigned char)(origmap[i+1])] << 8) |
                (maptable[(unsigned char)(origmap[i+2])] << 16) |
                (maptable[(unsigned char)(origmap[i+3])] << 24);
    }

    /*
     *  Set this variable to signify to V_DrawPatch that pixels have to be translated.
     *  This is currently only implemented in Varm_DrawPatch (assembler), so don't look
     *  for any specific code in v_video.
     */
    translated_colourmaps = (byte*)VIDEO_COLOURMAP;
  }
#endif
}




void I_InitGraphics(void)
{
  _kernel_oserror *err;
  int i, status;
#if (LD_PIXEL_DEPTH > 3)
# ifndef LINEAR_COLOURMAP_LIGHT
  int j;
  byte *playpal, *pal;
# endif
#endif

  /*
   *  Init with information for the frame buffer manager.
   *  It returns successful if
   *
   *  1) The depth of the current mode is 8.
   *  1) The width of the current mode is == the width requested.
   *  2) The height of the current mode is >= the height requested.
   *  3) There is enough memory for at least 2 frame buffers (it tries to
   *     extend the screen size but unfortunately SharedCLib won't let it.)
   *
   *  If successful it returns the actual height and number of buffers in fbd.
   */

  fbd = NULL;

  if (M_CheckParm("-noframes"))
  {
    without_frame_buffers = 1;
  }

  status = -1;
  if (without_frame_buffers == 0)
  {
    err = GameSupp_ClaimFrameBuffer(3, SCREENWIDTH, SCREENHEIGHT, (1<<LD_PIXEL_DEPTH), &fbd);
    if (err != NULL)
    {
      fprintf(logfile, "ClaimFrameBuffer: %s\n", err->errmess);
    }
    else
      status = 0;
  }
  /* if installing the frame buffer failed we try to use use copying */
  if (status != 0)
  {
    err = GameSupp_FrameBufferInfo(3, SCREENWIDTH, SCREENHEIGHT, (1<<LD_PIXEL_DEPTH), &fbd);
    if (err != NULL)
    {
      fprintf(logfile, "FrameBufferInfo: %s\n", err->errmess);
    }
    else
    {
      game_supp_frame_buffer_t *newfb;
      unsigned int fbsize;

      /* make a copy of the frame buffer descriptor for safety's sake */
      fbsize = sizeof(game_supp_frame_buffer_t);
      if (fbd->buffers > 1)
        fbsize += (fbd->buffers-1)*sizeof(void*);
      newfb = (game_supp_frame_buffer_t*)malloc(fbsize);
      memcpy(newfb, fbd, fbsize);
      fbd = newfb;
      fbd->show = 0;
      fbd->plot = 0;
      i = SCREENWIDTH * SCREENHEIGHT * sizeof(pixel_t);
      screens[0] = Z_Malloc(i, PU_STATIC, NULL);
      memset(screens[0], 0, i);
      PrepareWithoutFrames(fbd);
      fprintf(logfile, "No frame buffers, copy screens around\n");
      without_frame_buffers = 1;
      status = 0;
    }
  }

  if (status != 0)
  {
    I_Error("Couldn't install frame buffer!");
  }

  HandlersActive = 1;
  atexit(handle_exit);

  if ((InstHandlerFlags & IHFlag_Abort) != 0)
  {
    int flags = 0;
#ifndef __UNIXLIB_TYPES_H
    if ((InstHandlerFlags & IHFlag_StFix) != 0)
      flags = GameSupp_AbortRepair;
#endif
    err = GameSupp_InstallAbortGuard(flags, NULL, NULL);
    if (err != NULL)
      fprintf(logfile, "Abort handler: %s\n", err->errmess);
  }

  /*fprintf(logfile, "%d colours, eigx %d, eigy %d\n", fbd->colours, fbd->eigx, fbd->eigy);*/
  /* Install handlers for problem cases */
  signal (SIGABRT, (SIGHANDLER_CAST) handle_abort);
  signal (SIGILL, (SIGHANDLER_CAST) handle_abort);
  signal (SIGSEGV, (SIGHANDLER_CAST) handle_abort);
  signal (SIGFPE, (SIGHANDLER_CAST) handle_abort);
#ifdef __UNIXLIB_TYPES_H
  signal (SIGBUS, (SIGHANDLER_CAST) handle_abort);
  signal (SIGSTOP, (SIGHANDLER_CAST) handle_abort);
  signal (SIGKILL, (SIGHANDLER_CAST) handle_abort);
#else
  signal (SIGSTAK, (SIGHANDLER_CAST) handle_abort);
  signal (SIGOSERROR, (SIGHANDLER_CAST) handle_abort);
#endif

#ifdef ESCAPE_ABORTS_DOOM
  signal (SIGINT, (SIGHANDLER_CAST) handle_abort);
#else
  signal (SIGINT, (SIGHANDLER_CAST) ignore_escape);
  /* Make sure that escape is turned off! */
  ChangeEscapeEffect(1, 0xfe);
#endif

  if (without_frame_buffers == 0)
  {
    if ((this_frame = GameSupp_GetNextFrame()) < 0)
    {
      I_Error("No frame available ?!?");
    }
    /* do one FB step to avoid the first screen being drawn into the visible screen */
    I_FinishUpdate();
    while ((this_frame = GameSupp_GetNextFrame() < 0)) ;

    screens[0] = (pixel_t*)(fbd->screens[this_frame]);
  }

  /* Clear all screen buffers (screen dimensions could differ, so use actual dimensions) */
  memset(fbd->screens[0], 0, fbd->buffers * fbd->width * fbd->height * sizeof(pixel_t));

  update_background = 0; /*update_status = 0;*/

#if (LD_PIXEL_DEPTH == 3)
  translated_colourmaps = NULL;	/* Indicates fully-programmable 256 colour palette */
  original_colourmaps = NULL;

  if (fbd->colours == 255)
    d_ctx.fuzz_colourmap = VIDEO_COLOURMAP + 0x600;
  else
  {
    unsigned int *modePalette, val;
    int r, g, b;
    byte *fcmap;

    /*
     *  Init the palette-to-mode-colour table. Since old machines could only
     *  handle 4096 colours anyway this can be done very efficiently using a 4kB
     *  lookup table. For each 8bit sample copy the top nibble into the bottom one.
     */
    if ((pal_to_mode = (byte*)malloc(4096*sizeof(byte))) == NULL)
    {
      pal_to_mode = Z_Malloc(4096, PU_STATIC, NULL);
    }

    for (i=0; i<4096; i++)
    {
      r = (i & 0x00f); g = ((i & 0x0f0) >> 4); b = ((i & 0xf00) >> 8);
      pal_to_mode[i] = ReturnClosestColour(r | (r<<4), g | (g<<4), b | (b<<4));
    }

    /*
     *  Read the current palette and build the fuzz translation table by mapping
     *  each colour to a colour 26/32rds its brightness which is what the original Doom
     *  engine does (using colourmap #6)
     */
    if ((fcmap = (byte*)malloc(256*sizeof(byte))) == NULL)
    {
      fcmap = Z_Malloc(256, PU_STATIC, NULL);
    }
    ReadPalette((byte*)(fbd->screens[0]), 1024);
    modePalette = (unsigned int*)fbd->screens[0];
    for (i=0; i<256; i++)
    {
      val = *modePalette++;
      r = (val & 0xff00) >> 8; g = (val & 0xff0000) >> 16; b = (val & 0xff000000) >> 24;
      fcmap[i] = pal_to_mode[((r*26) >> 9) + (((g*26) >> 5) & 0x0f0) + (((b*26) >> 1) & 0xf00)];
    }
    d_ctx.fuzz_colourmap = fcmap;
    memset(fbd->screens[0], 0, 1024);

    /* Preserve the original colourmaps */
#ifdef DIYBOOM
    d_ctx.static_colourmap = colormaps[0];

    if ((original_colourmaps = (byte**)malloc(numcolormaps * sizeof(byte*))) == NULL)
    {
      original_colourmaps = (byte**)Z_Malloc(numcolormaps * sizeof(byte*), PU_STATIC, NULL);
    }
    for (i=0; i<numcolormaps; i++)
    {
      byte *mem;

      if ((mem = (byte*)malloc(256*(NUMCOLORMAPS+1))) == NULL)
      {
        mem = (byte*)Z_Malloc(256*(NUMCOLORMAPS+1), PU_STATIC, NULL);
      }
      original_colourmaps[i] = mem;
      memcpy(mem, colormaps[i], 256*(NUMCOLORMAPS+1));
    }
#else
    d_ctx.static_colourmap = colormaps;

    if ((original_colourmaps = (byte*)malloc(256*(NUMCOLORMAPS+1))) == NULL)
    {
      original_colourmaps = Z_Malloc(256*(NUMCOLORMAPS+1), PU_STATIC, NULL);
    }
    memcpy(original_colourmaps, VIDEO_COLOURMAP, 256*(NUMCOLORMAPS+1));
#endif
  }
#endif

  /* Init the keyboard bitfield */
  memset(riscos_keys, 0, 128/8);

  /* Init mouse state */
  ReadMouseState(&lastMouse);
  PutMouseAt(SCREENWIDTH << (fbd->eigx - 1), SCREENHEIGHT << (fbd->eigy - 1));

#if (LD_PIXEL_DEPTH > 3)
  /* Mark inverse colourmap (invulnerability) as undefined */
  translated_colourmaps[NUMCOLORMAPS*256] = -1;

# ifdef DIYBOOM
  d_ctx.static_colourmap = translated_basemap;
# else
  d_ctx.static_colourmap = translated_colourmaps;
# endif

#if (defined(LINEAR_COLOURMAP_LIGHT) || (NUMCOLORMAPS != 32))
  i = M_CheckParm("-minlight");
  if (i && (i < myargc-1))
    MinLightLevel = atoi(myargv[i+1]);	/* minlightlevel between 0 and 256, default 32 */

  for (i=0; i<NUMCOLORMAPS; i++)
  {
    lightmultipliers[i] = 0x10000 - (i*(0x10000 - (MinLightLevel << 8))) / (NUMCOLORMAPS - 1);
  }
#else
  /*
   *  Init the multipliers (fixpoint numbers) for each lightlevel.
   *  Fiddle around with these values at your own risk. What I do here is determine
   *  the intensity levels of the real Doom colourmaps and calculate the multipliers
   *  from these. Unfortunately the first ones are lighter than the later ones, so
   *  linear approximation wouldn't work too well.
   */
  playpal = W_CacheLumpName("PLAYPAL", PU_CACHE);

  for (i=0; i<NUMCOLORMAPS; i++)
  {
    lightmultipliers[i] = 0;
    for (j=0; j<256; j++)
    {
      pal = playpal + 3*VIDEO_COLOURMAP[i*256 + j];
      lightmultipliers[i] += pal[0] + pal[1] + pal[2];
    }
  }
  for (i=1; i<NUMCOLORMAPS; i++)
  {
    lightmultipliers[i] = (int)((65536.0 * (double)lightmultipliers[i]) / lightmultipliers[0]);
  }
  lightmultipliers[0] = 65536;
#endif
#endif

  if (BufferedKeyboard)
  {
    _kernel_swi_regs r;
    intkey_table = intkey_table_buff;
    r.r[0]=1;
    _kernel_swi(0x2003e,&r,&r);	/* XOS_InstallKeyHandler */
    if (r.r[0]==2) intkey_table[51]='#'; /* # left of Return */
    err = GameSupp_ClaimKeyPress();
    if (err != NULL)
      I_Error("Keyboard handler: %s", err->errmess);
  }

  /* Init draw context */
  d_ctx.trans_colmaps = translated_colourmaps;
  d_ctx.num_colourmaps = NUMCOLORMAPS;
#if (LD_PIXEL_DEPTH > 3)
  d_ctx.lightmultipliers = lightmultipliers;
  d_ctx.trans_cmap_work = TranslateColourmapsWork;
#endif
  d_ctx.frame_buffer_start = (without_frame_buffers != 0) ? (byte*)(screens[0]) : (byte*)(fbd->screens[0]);
  d_ctx.frame_buffer_end = (byte*)(d_ctx.frame_buffer_start + fbd->buffers * fbd->pitch * fbd->height);
  d_ctx.end_of_slot = (byte*)(0x8000 + WimpReadSlotSize());
  fprintf(logfile, "I_InitGraphics: %d frame buffer(s) from %p to %p, wimpslot to %p\n", fbd->buffers, d_ctx.frame_buffer_start, d_ctx.frame_buffer_end, d_ctx.end_of_slot);

  coredumpfile[0] = '\0';
  i = M_CheckParm("-core");
  if (i && (i < myargc-1))
  {
    strcpy(coredumpfile, myargv[i+1]);
  }
  fflush(logfile);

  if (HardwareGamma && (LD_PIXEL_DEPTH == 3))
    HardwareGamma = 0;
}



void I_ShutdownGraphics(void)
{
  if (without_frame_buffers == 0)
    GameSupp_ReleaseFrameBuffer();

  HandlersActive = 0;
  /* Clear keyboard buffer */
  FlushABuffer(0);
  /* Clear mouse buffer */
  FlushABuffer(9);

  if (BufferedKeyboard)
    GameSupp_ReleaseKeyPress();

#ifndef ESCAPE_ABORTS_DOOM
  ChangeEscapeEffect(0, 0xfe);
#endif
  /*InsertInBuffer(0, 32);*/	/* Doesn't work, sadly */
}




void I_SetPalette(const byte *palette)
{
  const byte *srcPalette;
#if (LD_PIXEL_DEPTH > 3)
  const unsigned char *paletteName = gamma_palette;
#endif

  if (palette == NULL)
  {
    srcPalette = last_palette;
  }
  else
  {
    srcPalette = palette; last_palette = palette;
  }
  if (srcPalette == NULL) return;

#if (LD_PIXEL_DEPTH == 3)

  /*
   *  Got a new approach to colour matching which should be fast enough to do
   *  palette effects even on old machines with a fixed 8bpp palette.
   */
  if (fbd->colours == 255)
  {
    if (palette != NULL)
    {
      I_TranslatePalette(srcPalette);
      InstallPalette(gamma_palette);
    }
  }
  else
  {
    I_TranslateColourmaps(srcPalette);
    PaletteChanged = true;
  }
#else

  /*
   *  For true colour modes we have to translate all colourmaps. Ouch...
   */

  PaletteChanged = true;

  if (palette != NULL)
  {
    if (HardwareGamma)
    {
        paletteName = srcPalette;

        if (usegamma != last_gamma)
        {
            InstallGamma((char*)gammatable[usegamma]);
            last_gamma = usegamma;
        }
    }
    else
    {
        paletteName = gamma_palette;
        I_TranslatePalette(srcPalette);
    }

#ifdef DIYBOOM
    I_TranslateBaseMap(paletteName, translated_basemap, colormaps[0]);
#endif
  }

#if (LD_PIXEL_DEPTH == 4)
  TranslateColourmaps16(&d_ctx, paletteName, VIDEO_COLOURMAP);
#else
  TranslateColourmaps32(&d_ctx, paletteName, VIDEO_COLOURMAP);
#endif

#endif
}




void I_FinishUpdate(void)
{
  if (without_frame_buffers != 0)
    memcpy(fbd->screens[0], screens[0], SCREENWIDTH*SCREENHEIGHT*sizeof(pixel_t));
  else
    GameSupp_MarkFrameNumber(this_frame);

  if (screenshotPending)
  {
    I_SaveSprite((without_frame_buffers == 0) ? this_frame : 0);
    screenshotPending = false;
  }
}




void I_WaitVBL(int count)
{
  WaitForVSync();
}




void I_ReadScreen(pixel_t *scr)
{
  /* Doom would probably break if I used the actual screen height ( >= SCREENHEIGHT) */
  memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT*sizeof(pixel_t));
}


void I_ReadDisplayedScreen(pixel_t *scr)
{
  if (without_frame_buffers != 0)
    memcpy(scr, screens[0], SCREENWIDTH*SCREENHEIGHT*sizeof(pixel_t));
  else
    memcpy(scr, fbd->screens[fbd->show], SCREENWIDTH*SCREENHEIGHT*sizeof(pixel_t));
}



/* clear all frame buffers */
void I_ClearFrameBuffers(void)
{
  if (without_frame_buffers == 0)
  {
    /* delayed frame clear */
    clear_num_frames = fbd->buffers;
  }
}


void I_DisplayFrameBuffer(void *adr)
{
  if (without_frame_buffers == 0)
  {
    int i;

    for (i=0; i<fbd->buffers; i++)
    {
      if (adr == fbd->screens[i]) break;
    }
    if (i < fbd->buffers)
      GameSupp_DisplayFrameNumber(i);
  }
}



void I_ScreenShot(void)
{
  I_SaveSprite((without_frame_buffers == 0) ? fbd->show : 0);
}


/*
 *  Save the currently displayed screen as a sprite.
 */

static void I_SaveSprite(unsigned int num)
{
  sprite_area sprite;
  FILE *fp;
  char spriteName[] = "Doom:Snapshots.sprite00";
  int noff, i, headSize;
#if (LD_PIXEL_DEPTH == 3)
  int *sprPal;
#endif
  /*int *xxx = (int*)0x80000000; I_InvalidateArea(*xxx);*/

#if (LD_PIXEL_DEPTH == 3)
  headSize = sizeof(sprite_area) + 8*256;
#else
  headSize = sizeof(sprite_area);
#endif

  sprite.numSprites = 1;
  sprite.firstOff = 16;
  sprite.firstFree = SCREENWIDTH*SCREENHEIGHT*sizeof(pixel_t) + headSize;
  sprite.nextOff = sprite.firstFree - 16;
  strcpy(sprite.name, "doom_snap");
  /* Screenmodes are always a multiple of words wide, so these fields are really simple. */
  sprite.width = (SCREENWIDTH >> (5 - LD_PIXEL_DEPTH)) - 1;
  sprite.height = SCREENHEIGHT - 1;
  sprite.leftBit = 0; sprite.rightBit = 31;
#if (LD_PIXEL_DEPTH == 3)
  sprite.imgOff = 44 + 8*256;
  sprite.mode = 28;
#elif (LD_PIXEL_DEPTH == 4)
  sprite.imgOff = 44;
  sprite.mode = 0x281680b5;
#elif (LD_PIXEL_DEPTH == 5)
  sprite.imgOff = 44;
  sprite.mode = 0x301680b5;
#endif
  sprite.maskOff = sprite.imgOff;

  noff = strlen(spriteName) - 2;
  for (i=0; i<100; i++)
  {
    spriteName[noff] = i/10 + '0';
    spriteName[noff+1] = i%10 + '0';
    if (access(spriteName, R_OK)) break;
  }
  if (i >= 100) return;

  if ((fp = fopen(spriteName, "wb")) == NULL) return;
  fwrite(((char*)(&sprite.numSprites)), 1, sizeof(sprite_area) - 4, fp);
#if (LD_PIXEL_DEPTH == 3)
  if ((sprPal = (int*)Z_Malloc(0x800, PU_STATIC, NULL)) != NULL)
  {
    if (fbd->colours == 255)
    {
      for (i=0; i<256; i++)
      {
        noff = 0x10 | (gamma_palette[3*i] << 8) | (gamma_palette[3*i+1] << 16) | (gamma_palette[3*i+2] << 24);
        sprPal[2*i] = noff; sprPal[2*i+1] = noff;
      }
    }
    else
    {
      int *srcPal;

      srcPal = sprPal + 256;
      ReadPalette((byte*)srcPal, 1024);
      for (i=0; i<256; i++)
      {
        sprPal[2*i] = *srcPal; sprPal[2*i+1] = *srcPal++;
      }
    }
    fwrite(sprPal, 1, 0x800, fp);
    Z_Free(sprPal);
  }
#endif
  fwrite(fbd->screens[num], sizeof(pixel_t), SCREENWIDTH*SCREENHEIGHT, fp);
  fclose(fp);
  SetFiletype(spriteName, 0xff9);

  players[consoleplayer].message = "Saved sprite.";
}




/*
 *  The remaining functions don't do anything.
 */
void I_UpdateNoBlit(void)
{
  ;
}




void I_BeginRead(void)
{
  ;
}




void I_EndRead(void)
{
  ;
}
