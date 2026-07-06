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
/*	DOOM graphics stuff for X11, UNIX.*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <X11/extensions/XShm.h>
/* Had to dig up XShm.c for this one.*/
/* It is in the libXext, but not in the XFree86 headers.*/
#ifdef LINUX
int XShmGetEventBase( Display* dpy ); /* problems with g++?*/
#endif

#ifdef VIDMODE_FULLSCREEN
/* Full screen mode extensions; if these includes don't work for you, undefine VIDMODE_FULLSCREEN */
#include <X11/extensions/xf86vmode.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#endif

#ifdef HAVE_LIBPNG
#include <png.h>
#endif

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"
#include "m_menu.h"

#include "doomdef.h"

#include "r_main.h"
#include "r_draw.h"
#include "r_context.h"


#define POINTER_WARP_COUNTDOWN	1

static void Expand4(const unsigned int*, double *);

static Display*	X_display=0;
static Window	X_mainWindow;
static int use_pixel_depth;
static const byte*	last_palette = NULL;
#if (LD_PIXEL_DEPTH == 3)
static Colormap	X_cmap;
#else
boolean		PaletteChanged=false;
lighttable_t	translated_colourmaps[256*(NUMCOLORMAPS+1)];
static unsigned long	light_multipliers[NUMCOLORMAPS];
#endif
static Visual*	X_visual;
static GC	X_gc;
static XEvent	X_event;
static int	X_screen;
static XVisualInfo	X_visualinfo;
static XImage*	image;
static int	X_width;
static int	X_height;
static int	X_drawOrigX=0;
static int	X_drawOrigY=0;
/* MIT SHared Memory extension.*/
static boolean	doShm;

static XShmSegmentInfo	X_shminfo;
static int		X_shmeventtype;

/* Fake mouse handling.*/
/* This cannot work properly w/o DGA.*/
/* Needs an invisible mouse cursor at least.*/
static boolean	grabMouse;
static int	doPointerWarp = POINTER_WARP_COUNTDOWN;

/* Blocky mode,*/
/* replace each 320x200 pixel with multiply*multiply pixels.*/
/* According to Dave Taylor, it still is a bonehead thing*/
/* to use ....*/
static int	multiply=1;


#ifdef DIYBOOM
lighttable_t	translated_basemap[256];
#endif

int MinLightLevel;



#ifdef VIDMODE_FULLSCREEN
static XF86VidModeModeInfo **videoModes = NULL;
static int numVidModes = 0;
static int fullScreenMode = -1;

static void I_DisableFullScreenMode(void)
{
  /*printf("Disable full screen: %p %p\n", X_display, oldMode);*/
  if ((X_display != NULL) && (videoModes != NULL))
  {
    printf("Switching back to %dx%d...\n", videoModes[0]->hdisplay, videoModes[0]->vdisplay);
    XF86VidModeLockModeSwitch(X_display, X_screen, 0);
    XF86VidModeSwitchToMode(X_display, X_screen, videoModes[0]);
    XF86VidModeSetViewPort(X_display, X_screen, 0, 0);
    /* Basically I should free this structure, but since the program's terminating anyway... */
    videoModes = NULL;
    /* Need to call XSync(), otherwise the mode switch requests will be lost */
    XSync(X_display, False);
  }
}


static int grab_device (int grabresult, const char *device)
{
  switch (grabresult)
  {
    case GrabSuccess:
    case GrabNotViewable:
      return 0;
    case AlreadyGrabbed:
      if (1) printf ("I_EnableFullScreenMode: the %s is already grabbed!\n", device); else
    case GrabFrozen:
      if (1) printf ("I_EnableFullScreenMode: the %s is frozen!\n", device); else
    case GrabInvalidTime:
      if (1) printf ("I_EnableFullScreenMode: beaten to a %s grab...\n", device);
      sleep (1);
      return 1;
    default:
      I_Error("I_EnableFullScreenMode: unexpected error while grabbing the %s\n, device");
      return 1;
  }
}


static void I_EnableFullScreenMode(void)
{
  int i, bestidx;
  double error;

  if (!XF86VidModeGetAllModeLines(X_display, X_screen, &numVidModes, &videoModes))
  {
    fprintf(stderr, "Unable to read mode info\n");
    return;
  }

  error = 1e10;
  bestidx = -1;
  for (i=0; i<numVidModes; i++)
  {
    if ((videoModes[i]->hdisplay >= X_width) && (videoModes[i]->vdisplay >= X_height))
    {
      double dx, dy, err;
      dx = (double)(videoModes[i]->hdisplay - X_width);
      dy = (double)(videoModes[i]->vdisplay - X_height);
      err = sqrt(dx*dx + dy*dy);
      if (error > err)
      {
	error = err;
	bestidx = i;
      }
    }
  }
  if (bestidx >= 0)
  {
    int eventMask;
    
    XWindowAttributes wAttr;
    XF86VidModeModeInfo *useMode = videoModes[bestidx];

    fullScreenMode = bestidx;
    printf("Best vidmode %d: %dx%d; current vidmode: %dx%d\n", bestidx, useMode->hdisplay,
	   useMode->vdisplay, videoModes[0]->hdisplay, videoModes[0]->vdisplay);

    /* Get the window drawable's coordinates */
    XGetWindowAttributes(X_display, X_mainWindow, &wAttr);
    /* resize to full screen width and move to origin */
    XSetWindowBorderWidth(X_display, X_mainWindow, 0);
    XResizeWindow(X_display, X_mainWindow, useMode->hdisplay, useMode->vdisplay);
    X_drawOrigX = (useMode->hdisplay - X_width) / 2;
    X_drawOrigY = (useMode->vdisplay - X_height) / 2;
    XMoveWindow(X_display, X_mainWindow, 0, 0);
    XFillRectangle(X_display, X_mainWindow, X_gc, 0, 0, useMode->hdisplay, useMode->vdisplay);
    XF86VidModeSwitchToMode(X_display, X_screen, useMode);
    XF86VidModeSetViewPort(X_display, X_screen, wAttr.x, wAttr.y);
    
    eventMask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    while (grab_device
	    (XGrabPointer (X_display, X_mainWindow, True, eventMask, GrabModeAsync,
			   GrabModeAsync, X_mainWindow, None, CurrentTime),
	     "pointer"))
      ;
    while (grab_device
	    (XGrabKeyboard (X_display, X_mainWindow, False, GrabModeAsync,
			    GrabModeAsync, CurrentTime),
	     "keyboard"))
      ;

    atexit(I_DisableFullScreenMode);
  }
}
#endif



/*  Translates the key currently in X_event*/


static int xlatekey(void)
{

    int rc;

    switch(rc = XKeycodeToKeysym(X_display, X_event.xkey.keycode, 0))
    {
      case XK_Left:	rc = KEY_LEFTARROW;	break;
      case XK_Right:	rc = KEY_RIGHTARROW;	break;
      case XK_Down:	rc = KEY_DOWNARROW;	break;
      case XK_Up:	rc = KEY_UPARROW;	break;
      case XK_Escape:	rc = KEY_ESCAPE;	break;
      case XK_Return:	rc = KEY_ENTER;		break;
      case XK_Tab:	rc = KEY_TAB;		break;
      case XK_F1:	rc = KEY_F1;		break;
      case XK_F2:	rc = KEY_F2;		break;
      case XK_F3:	rc = KEY_F3;		break;
      case XK_F4:	rc = KEY_F4;		break;
      case XK_F5:	rc = KEY_F5;		break;
      case XK_F6:	rc = KEY_F6;		break;
      case XK_F7:	rc = KEY_F7;		break;
      case XK_F8:	rc = KEY_F8;		break;
      case XK_F9:	rc = KEY_F9;		break;
      case XK_F10:	rc = KEY_F10;		break;
      case XK_F11:	rc = KEY_F11;		break;
      case XK_F12:	rc = KEY_F12;		break;

      case XK_BackSpace:
      case XK_Delete:	rc = KEY_BACKSPACE;	break;

      case XK_Pause:	rc = KEY_PAUSE;		break;

      case XK_KP_Equal:
      case XK_equal:	rc = KEY_EQUALS;	break;

      case XK_KP_Subtract:
      case XK_minus:	rc = KEY_MINUS;		break;

      case XK_Shift_L:
      case XK_Shift_R:
	rc = KEY_RSHIFT;
	break;

      case XK_Control_L:
      case XK_Control_R:
	rc = KEY_RCTRL;
	break;

      case XK_Alt_L:
      case XK_Meta_L:
      case XK_Alt_R:
      case XK_Meta_R:
	rc = KEY_RALT;
	break;

      case XK_0:
      case XK_KP_0:	rc = '0';		break;
      case XK_KP_1:	rc = '1';		break;
      case XK_KP_2:	rc = '2';		break;
      case XK_KP_3:	rc = '3';		break;
      case XK_KP_4:	rc = '4';		break;
      case XK_KP_5:	rc = '5';		break;
      case XK_KP_6:	rc = '6';		break;
      case XK_KP_7:	rc = '7';		break;
      case XK_KP_8:	rc = '8';		break;

      case XK_Print:	rc = KEY_PRINT;		break;
      
      default:
	if (rc >= XK_space && rc <= XK_asciitilde)
	    rc = rc - XK_space + ' ';
	if (rc >= 'A' && rc <= 'Z')
	    rc = rc - 'A' + 'a';
	break;
    }

    return rc;

}

void I_ShutdownGraphics(void)
{
  if (X_display != NULL)
  {
    /* Detach from X server*/
    if (!XShmDetach(X_display, &X_shminfo))
      I_Error("XShmDetach() failed in I_ShutdownGraphics()");

    /* Release shared memory.*/
    shmdt(X_shminfo.shmaddr);
    shmctl(X_shminfo.shmid, IPC_RMID, 0);

    /* Paranoia.*/
    image->data = NULL;

#ifdef VIDMODE_FULLSCREEN
    I_DisableFullScreenMode();
#endif
  }
}




/* I_StartFrame*/

void I_StartFrame (void)
{
    /* er?*/
}

static int	lastmousex = 0;
static int	lastmousey = 0;
static boolean	mousemoved = false;
static boolean	shmFinished;
static unsigned int lastButtons = 0;

void I_GetEvent(void)
{

    event_t event;

    /* put event-grabbing stuff in here*/
    XNextEvent(X_display, &X_event);
    switch (X_event.type)
    {
      case KeyPress:
	event.type = ev_keydown;
	event.data1 = xlatekey();
	D_PostEvent(&event);
	/* fprintf(stderr, "k");*/
	break;
      case KeyRelease:
	event.type = ev_keyup;
	event.data1 = xlatekey();
	D_PostEvent(&event);
	/* fprintf(stderr, "ku");*/
	break;
      case ButtonPress:
	event.type = ev_mouse;
	lastButtons =
	    (X_event.xbutton.state & Button1Mask ? 1 : 0)
	    | (X_event.xbutton.state & Button2Mask ? 2 : 0)
	    | (X_event.xbutton.state & Button3Mask ? 4 : 0)
	    | (X_event.xbutton.button == Button1 ? 1 : 0)
	    | (X_event.xbutton.button == Button2 ? 2 : 0)
	    | (X_event.xbutton.button == Button3 ? 4 : 0);
	event.data1 = lastButtons;
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	/* fprintf(stderr, "b");*/
	break;
      case ButtonRelease:
	event.type = ev_mouse;
	event.data1 =
	    (X_event.xbutton.state & Button1Mask ? 1 : 0)
	    | (X_event.xbutton.state & Button2Mask ? 2 : 0)
	    | (X_event.xbutton.state & Button3Mask ? 4 : 0);
	/* suggest parentheses around arithmetic in operand of |*/
	lastButtons =
	    event.data1
	    ^ (X_event.xbutton.button == Button1 ? 1 : 0)
	    ^ (X_event.xbutton.button == Button2 ? 2 : 0)
	    ^ (X_event.xbutton.button == Button3 ? 4 : 0);
	event.data1 = lastButtons;
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	/* fprintf(stderr, "bu");*/
	break;
      case MotionNotify:
	event.type = ev_mouse;
	event.data1 = lastButtons;
	    /*(X_event.xmotion.state & Button1Mask ? 1 : 0)
	    | (X_event.xmotion.state & Button2Mask ? 2 : 0)
	    | (X_event.xmotion.state & Button3Mask ? 4 : 0);*/
	event.data2 = (X_event.xmotion.x - lastmousex) << 2;
	event.data3 = (lastmousey - X_event.xmotion.y) << 2;

	if (event.data2 || event.data3)
	{
	    lastmousex = X_event.xmotion.x;
	    lastmousey = X_event.xmotion.y;
	    if (X_event.xmotion.x != X_width/2 &&
		X_event.xmotion.y != X_height/2)
	    {
		D_PostEvent(&event);
		/* fprintf(stderr, "m");*/
		mousemoved = false;
	    } else
	    {
		mousemoved = true;
	    }
	}
	break;

      case Expose:
      case ConfigureNotify:
	break;

      default:
	if (doShm && X_event.type == X_shmeventtype) shmFinished = true;
	break;
    }

}

static Cursor
createnullcursor
( Display*	display,
  Window	root )
{
    Pixmap cursormask;
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
				 &dummycolour,&dummycolour, 0,0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}


/* I_StartTic*/

void I_StartTic (void)
{

    if (!X_display)
	return;

    while (XPending(X_display))
	I_GetEvent();

    /* Warp the pointer back to the middle of the window*/
    /*  or it will wander off - that is, the game will*/
    /*  lose input focus within X11.*/
    if (grabMouse)
    {
	if (!--doPointerWarp)
	{
	    XWarpPointer( X_display,
			  None,
			  X_mainWindow,
			  0, 0,
			  0, 0,
			  X_width/2, X_height/2);

	    doPointerWarp = POINTER_WARP_COUNTDOWN;
	}
    }

    mousemoved = false;

}



/* I_UpdateNoBlit*/

void I_UpdateNoBlit (void)
{
    /* what is this?*/
}


/* I_FinishUpdate*/

void I_FinishUpdate (void)
{

    static int	lasttic;
    int		tics;
    int		i;
    /* UNUSED static unsigned char *bigscreen=0;*/

    if (screenshotPending)
    {
      I_ScreenShot();
      screenshotPending = false;
    }

    /* draws little dots on the bottom of the screen*/
    if (devparm)
    {

	i = I_GetTime();
	tics = i - lasttic;
	lasttic = i;
	if (tics > 20) tics = 20;

	for (i=0 ; i<tics*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
	for ( ; i<20*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;

    }

    /* scales the screen size before blitting it*/
    if (multiply == 2)
    {
	unsigned int *olineptrs[2];
	const unsigned int *ilineptr;
	int x, y, i;
	unsigned int twoopixels;
	unsigned int twomoreopixels;
	unsigned int fouripixels;

	ilineptr = (const unsigned int *) (screens[0]);
	for (i=0 ; i<2 ; i++)
	    olineptrs[i] = (unsigned int *) &image->data[i*X_width*sizeof(pixel_t)];

	y = SCREENHEIGHT;
	while (y--)
	{
	    x = SCREENWIDTH;
#if (LD_PIXEL_DEPTH == 3)
	    do
	    {
		fouripixels = *ilineptr++;
		twoopixels =	(fouripixels & 0xff000000)
		    |	((fouripixels>>8) & 0xffff00)
		    |	((fouripixels>>16) & 0xff);
		twomoreopixels =	((fouripixels<<16) & 0xff000000)
		    |	((fouripixels<<8) & 0xffff00)
		    |	(fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
#else
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
#endif
	    } while (x-=4);
	    olineptrs[0] += X_width/4;
	    olineptrs[1] += X_width/4;
#elif (LD_PIXEL_DEPTH == 4)
	    do
	    {
		fouripixels = *ilineptr++;
		twoopixels = (fouripixels & 0xffff0000); twoopixels |= (twoopixels >> 16);
		twomoreopixels = (fouripixels & 0xffff); twomoreopixels |= (twomoreopixels << 16);
#ifdef __BIG_ENDIAN__
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
#else
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
#endif
	    } while (x-=2);
	    olineptrs[0] += X_width/2;
	    olineptrs[1] += X_width/2;
#else
	    do
	    {
		fouripixels = *ilineptr++;
		*olineptrs[0]++ = fouripixels;
		*olineptrs[0]++ = fouripixels;
		*olineptrs[1]++ = fouripixels;
		*olineptrs[1]++ = fouripixels;
	    }
	    while (x--);
	    olineptrs[0] += X_width;
	    olineptrs[1] += X_width;
#endif
	}

    }
    else if (multiply == 3)
    {
	unsigned int *olineptrs[3];
	const unsigned int *ilineptr;
	int x, y, i;
	unsigned int fouropixels[3];
	unsigned int fouripixels;

	ilineptr = (const unsigned int *) (screens[0]);
	for (i=0 ; i<3 ; i++)
	    olineptrs[i] = (unsigned int *) &image->data[i*X_width*sizeof(pixel_t)];

	y = SCREENHEIGHT;
	while (y--)
	{
	    x = SCREENWIDTH;
#if (LD_PIXEL_DEPTH == 3)
	    do
	    {
		fouripixels = *ilineptr++;
		fouropixels[0] = (fouripixels & 0xff000000)
		    |	((fouripixels>>8) & 0xff0000)
		    |	((fouripixels>>16) & 0xffff);
		fouropixels[1] = ((fouripixels<<8) & 0xff000000)
		    |	(fouripixels & 0xffff00)
		    |	((fouripixels>>8) & 0xff);
		fouropixels[2] = ((fouripixels<<16) & 0xffff0000)
		    |	((fouripixels<<8) & 0xff00)
		    |	(fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
		*olineptrs[0]++ = fouropixels[0];
		*olineptrs[1]++ = fouropixels[0];
		*olineptrs[2]++ = fouropixels[0];
		*olineptrs[0]++ = fouropixels[1];
		*olineptrs[1]++ = fouropixels[1];
		*olineptrs[2]++ = fouropixels[1];
		*olineptrs[0]++ = fouropixels[2];
		*olineptrs[1]++ = fouropixels[2];
		*olineptrs[2]++ = fouropixels[2];
#else
		*olineptrs[0]++ = fouropixels[2];
		*olineptrs[1]++ = fouropixels[2];
		*olineptrs[2]++ = fouropixels[2];
		*olineptrs[0]++ = fouropixels[1];
		*olineptrs[1]++ = fouropixels[1];
		*olineptrs[2]++ = fouropixels[1];
		*olineptrs[0]++ = fouropixels[0];
		*olineptrs[1]++ = fouropixels[0];
		*olineptrs[2]++ = fouropixels[0];
#endif
	    } while (x-=4);
	    olineptrs[0] += 2*X_width/4;
	    olineptrs[1] += 2*X_width/4;
	    olineptrs[2] += 2*X_width/4;
#elif (LD_PIXEL_DEPTH == 4)
	    do
	    {
		fouripixels = *ilineptr++;
#ifdef __BIG_ENDIAN__
		fouropixels[0] = (fouripixels & 0xffff0000);
		fouropixels[0] |= (fouropixels[0] >> 16);
		fouropixels[1] = fouripixels;
		fouropixels[2] = (fouripixels & 0xffff);
		fouropixels[2] |= (fouropixels[2] << 16);
#else
		fouropixels[0] = (fouripixels & 0xffff);
		fouropixels[1] = fouropixels[0];
		fouropixels[0] |= (fouropixels[0] << 16);
		fouropixels[2] = (fouripixels & 0xffff0000);
		fouropixels[1] |= fouropixels[2];
		fouropixels[2] |= (fouropixels[2] >> 16);
#endif
		*olineptrs[0]++ = fouropixels[0];
		*olineptrs[0]++ = fouropixels[1];
		*olineptrs[0]++ = fouropixels[2];
		*olineptrs[1]++ = fouropixels[0];
		*olineptrs[1]++ = fouropixels[1];
		*olineptrs[1]++ = fouropixels[2];
		*olineptrs[2]++ = fouropixels[0];
		*olineptrs[2]++ = fouropixels[1];
		*olineptrs[2]++ = fouropixels[2];
	    } while (x-=2);
	    olineptrs[0] += X_width;
	    olineptrs[1] += X_width;
	    olineptrs[2] += X_width;
#else
	    do
	    {
		fouripixels = *ilineptr++;
		*olineptrs[0]++ = fouripixels;
		*olineptrs[0]++ = fouripixels;
		*olineptrs[0]++ = fouripixels;
		*olineptrs[1]++ = fouripixels;
		*olineptrs[1]++ = fouripixels;
		*olineptrs[1]++ = fouripixels;
		*olineptrs[2]++ = fouripixels;
		*olineptrs[2]++ = fouripixels;
		*olineptrs[2]++ = fouripixels;
	    } while (x--);
	    olineptrs[0] += 2*X_width;
	    olineptrs[1] += 2*X_width;
	    olineptrs[2] += 2*X_width;
#endif
	}

    }
    else if (multiply == 4)
    {
	/* Broken. Gotta fix this some day.*/
  	Expand4 ((const unsigned int*)(screens[0]), (double *) (image->data));
    }

    if (doShm)
    {

	if (!XShmPutImage(	X_display,
				X_mainWindow,
				X_gc,
				image,
				0, 0,
				X_drawOrigX, X_drawOrigY,
				X_width, X_height,
				True ))
	    I_Error("XShmPutImage() failed\n");

	/* wait for it to finish and processes all input events*/
	shmFinished = false;
	do
	{
	    I_GetEvent();
	} while (!shmFinished);

    }
    else
    {

	/* draw the image*/
	XPutImage(	X_display,
			X_mainWindow,
			X_gc,
			image,
			0, 0,
			X_drawOrigX, X_drawOrigY,
			X_width, X_height );

	/* sync up with server*/
	XSync(X_display, False);

    }

}



/* I_ReadScreen*/

void I_ReadScreen (pixel_t* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT*sizeof(pixel_t));
}



/* Palette stuff.*/

static XColor	colors[256];

static void UploadNewPalette(Colormap cmap, const byte *palette)
{

    register int	i;
    register int	c;
    static boolean	firstcall = true;

#ifdef __cplusplus
    if (X_visualinfo.c_class == PseudoColor && X_visualinfo.depth == 8)
#else
    if (X_visualinfo.class == PseudoColor && X_visualinfo.depth == 8)
#endif
	{
	    /* initialize the colormap*/
	    if (firstcall)
	    {
		firstcall = false;
		for (i=0 ; i<256 ; i++)
		{
		    colors[i].pixel = i;
		    colors[i].flags = DoRed|DoGreen|DoBlue;
		}
	    }

	    /* set the X colormap entries*/
	    for (i=0 ; i<256 ; i++)
	    {
		c = gammatable[usegamma][*palette++];
		colors[i].red = (c<<8) + c;
		c = gammatable[usegamma][*palette++];
		colors[i].green = (c<<8) + c;
		c = gammatable[usegamma][*palette++];
		colors[i].blue = (c<<8) + c;
	    }

	    /* store the colors to the current colormap*/
	    XStoreColors(X_display, cmap, colors, 256);

	}
}




#if (LD_PIXEL_DEPTH > 3)

void I_TranslatePalette(const byte *palette)
{
    unsigned char	gamma_palette[768];
    unsigned char*	pal;
    unsigned char*	gt;
    unsigned char*	invul;
    int			i, j;
    unsigned long*	gp;
    lighttable_t*	cmap;
    unsigned long	intensity;
    int			red, green, blue;
    const unsigned char* useColourmap;

#ifdef DIYBOOM
    useColourmap = fullcolormap;
#else
    useColourmap = colormaps;
#endif
    gt = (unsigned char*)(gammatable[usegamma]); gp = (unsigned long*)gamma_palette;
    for (i=0; i<768/4; i++)
    {
	*gp++ =
#ifdef __BIG_ENDIAN__
	(gt[(unsigned char)(palette[0])] << 24) | (gt[(unsigned char)(palette[1])] << 16) |
	(gt[(unsigned char)(palette[2])] << 8)  | (gt[(unsigned char)(palette[3])]);
#else
	(gt[(unsigned char)(palette[0])]) | (gt[(unsigned char)(palette[1])] << 8) |
	(gt[(unsigned char)(palette[2])] << 16) | (gt[(unsigned char)(palette[3])] << 24);
#endif
	palette += 4;
    }

    cmap = translated_colourmaps;
    for (j=0; j<NUMCOLORMAPS; j++)
    {
	intensity = light_multipliers[j];
        for (i=0; i<256; i++)
	{
	    pal = gamma_palette + 3*useColourmap[i];
	    red   = (intensity * pal[0]) >> 16;
	    green = (intensity * pal[1]) >> 16;
	    blue  = (intensity * pal[2]) >> 16;
	    *cmap++ = BUILD_RGB_PIXEL(red, green, blue);
	}
    }

    invul = (unsigned char*)(useColourmap + 32*256);

    for (i=0; i<256; i++)
    {
	pal = gamma_palette + 3*((unsigned char)(invul[i]));
	*cmap++ = BUILD_RGB_PIXEL(pal[0], pal[1], pal[2]);
    }
    PaletteChanged = true;

#ifdef DIYBOOM
    cmap = translated_basemap;
    for (i=0; i<256; i++)
    {
        pal = gamma_palette + 3*colormaps[0][i];
        red   = pal[0];
        green = pal[1];
        blue  = pal[2];
        *cmap++ = BUILD_RGB_PIXEL(red, green, blue);
    }
#endif
}
#endif



/* I_SetPalette*/

void I_SetPalette (const byte* palette)
{
    const byte *srcPalette;

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
    UploadNewPalette(X_cmap, srcPalette);
#else
    I_TranslatePalette(srcPalette);
#endif
}



/* This function is probably redundant,*/
/*  if XShmDetach works properly.*/
/* ddt never detached the XShm memory,*/
/*  thus there might have been stale*/
/*  handles accumulating.*/

static void grabsharedmemory(int size)
{

  int			key = ('d'<<24) | ('o'<<16) | ('o'<<8) | 'm';
  struct shmid_ds	shminfo;
  int			minsize = 320*200;
  int			id;
  int			rc;
  /* UNUSED int done=0;*/
  int			pollution=5;

  /* try to use what was here before*/
  do
  {
    id = shmget((key_t) key, minsize, 0777); /* just get the id*/
    if (id != -1)
    {
      rc=shmctl(id, IPC_STAT, &shminfo); /* get stats on it*/
      if (!rc)
      {
	if (shminfo.shm_nattch)
	{
	  fprintf(stderr, "User %d appears to be running "
		  "DOOM.  Is that wise?\n", shminfo.shm_cpid);
	  key++;
	}
	else
	{
	  if (getuid() == shminfo.shm_perm.cuid)
	  {
	    rc = shmctl(id, IPC_RMID, 0);
	    if (!rc)
	      fprintf(stderr,
		      "Was able to kill my old shared memory\n");
	    else
	      I_Error("Was NOT able to kill my old shared memory");

	    id = shmget((key_t)key, size, IPC_CREAT|0777);
	    if (id==-1)
	      I_Error("Could not get shared memory");

	    rc=shmctl(id, IPC_STAT, &shminfo);

	    break;

	  }
	  if (size >= shminfo.shm_segsz)
	  {
	    fprintf(stderr,
		    "will use %d's stale shared memory\n",
		    shminfo.shm_cpid);
	    break;
	  }
	  else
	  {
	    fprintf(stderr,
		    "warning: can't use stale "
		    "shared memory belonging to id %d, "
		    "key=0x%x\n",
		    shminfo.shm_cpid, key);
	    key++;
	  }
	}
      }
      else
      {
	I_Error("could not get stats on key=%d", key);
      }
    }
    else
    {
      id = shmget((key_t)key, size, IPC_CREAT|0777);
      if (id==-1)
      {
	extern int errno;
	fprintf(stderr, "errno=%d\n", errno);
	I_Error("Could not get any shared memory");
      }
      break;
    }
  } while (--pollution);

  if (!pollution)
  {
    I_Error("Sorry, system too polluted with stale "
	    "shared memory segments.\n");
    }

  X_shminfo.shmid = id;

  /* attach to the shared memory segment*/
  image->data = X_shminfo.shmaddr = shmat(id, 0, 0);

  fprintf(stderr, "shared memory id=%d, addr=0x%x\n", id,
	  (int) (image->data));
}

void I_InitGraphics(void)
{

    char*		displayname;
    char*		d;
    int			n;
    int			pnum;
    int			x=0;
    int			y=0;

    /* warning: char format, different type arg*/
    char		xsign=' ';
    char		ysign=' ';

    int			oktodraw;
    unsigned long	attribmask;
    XSetWindowAttributes attribs;
    XGCValues		xgcvalues;
    int			valuemask;
    static int		firsttime=1;

    if (!firsttime)
	return;
    firsttime = 0;

    signal(SIGINT, (void (*)(int)) I_Quit);

    if (M_CheckParm("-2"))
	multiply = 2;

    if (M_CheckParm("-3"))
	multiply = 3;

    if (M_CheckParm("-4"))
	multiply = 4;

    X_width = SCREENWIDTH * multiply;
    X_height = SCREENHEIGHT * multiply;

    /* check for command-line display name*/
    if ( (pnum=M_CheckParm("-disp")) ) /* suggest parentheses around assignment*/
	displayname = myargv[pnum+1];
    else
	displayname = 0;

    /* check if the user wants to grab the mouse (quite unnice)*/
    grabMouse = !!M_CheckParm("-grabmouse");

    /* check for command-line geometry*/
    if ( (pnum=M_CheckParm("-geom")) ) /* suggest parentheses around assignment*/
    {
	/* warning: char format, different type arg 3,5*/
	n = sscanf(myargv[pnum+1], "%c%d%c%d", &xsign, &x, &ysign, &y);

	if (n==2)
	    x = y = 0;
	else if (n==6)
	{
	    if (xsign == '-')
		x = -x;
	    if (ysign == '-')
		y = -y;
	}
	else
	    I_Error("bad -geom parameter");
    }

    /* open the display*/
    X_display = XOpenDisplay(displayname);
    if (!X_display)
    {
	if (displayname)
	    I_Error("Could not open display [%s]", displayname);
	else
	    I_Error("Could not open display (DISPLAY=[%s])", getenv("DISPLAY"));
    }

    /* use the default visual */
    X_screen = DefaultScreen(X_display);

#if (LD_PIXEL_DEPTH == 3)
    use_pixel_depth = 8;
    n = XMatchVisualInfo(X_display, X_screen, use_pixel_depth, PseudoColor, &X_visualinfo);
#elif (LD_PIXEL_DEPTH == 4)
    use_pixel_depth = 15;
    n = XMatchVisualInfo(X_display, X_screen, use_pixel_depth, TrueColor, &X_visualinfo);
    if (!n)
    {
      use_pixel_depth = 16;
      n = XMatchVisualInfo(X_display, X_screen, use_pixel_depth, TrueColor, &X_visualinfo);
    }
#else
    use_pixel_depth = 24;
    n = XMatchVisualInfo(X_display, X_screen, use_pixel_depth, TrueColor, &X_visualinfo);
    if (!n)
    {
      use_pixel_depth = 32;
      n = XMatchVisualInfo(X_display, X_screen, use_pixel_depth, TrueColor, &X_visualinfo);
    }
#endif
    if (!n)
	I_Error("Can't claim visual for pixel depth %d. Try another binary.", use_pixel_depth);

    X_visual = X_visualinfo.visual;

    /* check for the MITSHM extension*/
    doShm = XShmQueryExtension(X_display);

    /* even if it's available, make sure it's a local connection*/
    if (doShm)
    {
	if (!displayname) displayname = (char *) getenv("DISPLAY");
	if (displayname)
	{
	    d = displayname;
	    while (*d && (*d != ':')) d++;
	    if (*d) *d = 0;
	    if (!(!strcasecmp(displayname, "unix") || !*displayname)) doShm = false;
	}
    }

    fprintf(stderr, "Using MITSHM extension\n");

#if (LD_PIXEL_DEPTH == 3)
    /* create the colormap*/
    X_cmap = XCreateColormap(X_display, RootWindow(X_display,
						   X_screen), X_visual, AllocAll);
    attribs.colormap = X_cmap;
# ifdef DIYBOOM
    d_ctx.static_colourmap = fullcolormap;
# else
    d_ctx.static_colourmap = colormaps;
# endif
#else
    attribs.colormap = 0;

    /* Init the lighttable intensities */
    for (n=0; n<NUMCOLORMAPS; n++)
    {
	light_multipliers[n] = 0x10000 - (n * (0x10000 - (MinLightLevel << 8))) / (NUMCOLORMAPS - 1);
    }
# ifdef DIYBOOM
    d_ctx.static_colourmap = translated_basemap;
# else
    d_ctx.static_colourmap = translated_colourmaps;
# endif
#endif

    /* setup attributes for main window*/
    attribmask = CWEventMask | CWColormap | CWBorderPixel | CWBackPixel;
    attribs.event_mask =
	KeyPressMask
	| KeyReleaseMask
	| ExposureMask;

    if (grabMouse)
      attribs.event_mask |= PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

    attribs.border_pixel = 0;
    attribs.background_pixel = 0;
    /* create the main window*/
    X_mainWindow = XCreateWindow(	X_display,
					RootWindow(X_display, X_screen),
					x, y,
					X_width, X_height,
					0, /* borderwidth*/
					use_pixel_depth, /* depth*/
					InputOutput,
					X_visual,
					attribmask,
					&attribs );

    XDefineCursor(X_display, X_mainWindow,
		  createnullcursor( X_display, X_mainWindow ) );

    /* create the GC*/
    valuemask = GCGraphicsExposures;
    xgcvalues.graphics_exposures = False;
    X_gc = XCreateGC(	X_display,
  			X_mainWindow,
  			valuemask,
  			&xgcvalues );

    /* map the window*/
    XMapWindow(X_display, X_mainWindow);

    /* wait until it is OK to draw*/
    oktodraw = 0;
    while (!oktodraw)
    {
	XNextEvent(X_display, &X_event);
	if (X_event.type == Expose
	    && !X_event.xexpose.count)
	{
	    oktodraw = 1;
	}
    }

    /* grabs the pointer so it is restricted to this window*/
    if (grabMouse)
	XGrabPointer(X_display, X_mainWindow, True,
		     ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
		     GrabModeAsync, GrabModeAsync,
		     X_mainWindow, None, CurrentTime);

    if (doShm)
    {

	X_shmeventtype = XShmGetEventBase(X_display) + ShmCompletion;

	/* create the image*/
	image = XShmCreateImage(	X_display,
					X_visual,
				        use_pixel_depth,
					ZPixmap,
					0,
					&X_shminfo,
					X_width,
					X_height );

	grabsharedmemory(image->bytes_per_line * image->height);


	/* UNUSED*/
	/* create the shared memory segment*/
	/* X_shminfo.shmid = shmget (IPC_PRIVATE,*/
	/* image->bytes_per_line * image->height, IPC_CREAT | 0777);*/
	/* if (X_shminfo.shmid < 0)*/
	/* {*/
	/* perror("");*/
	/* I_Error("shmget() failed in InitGraphics()");*/
	/* }*/
	/* fprintf(stderr, "shared memory id=%d\n", X_shminfo.shmid);*/
	/* attach to the shared memory segment*/
	/* image->data = X_shminfo.shmaddr = shmat(X_shminfo.shmid, 0, 0);*/


	if (!image->data)
	{
	    perror("");
	    I_Error("shmat() failed in InitGraphics()");
	}

	/* get the X server to attach to it*/
	if (!XShmAttach(X_display, &X_shminfo))
	    I_Error("XShmAttach() failed in InitGraphics()");

    }
    else
    {
	image = XCreateImage(	X_display,
    				X_visual,
    				use_pixel_depth,
    				ZPixmap,
    				0,
    				(char*)malloc(X_width * X_height * sizeof(pixel_t)),
    				X_width, X_height,
    				8,
    				X_width * sizeof(pixel_t) );

    }

    if (multiply == 1)
	screens[0] = (pixel_t *) (image->data);
    else
	screens[0] = (pixel_t *) malloc (SCREENWIDTH * SCREENHEIGHT * sizeof(pixel_t));

#ifdef VIDMODE_FULLSCREEN
    if (M_CheckParm("-fullscreen"))
    {
      I_EnableFullScreenMode();
    }
#endif

}


void I_ClearFrameBuffers(void)
{
}


/* Write a screenshot in PNG format */
void I_ScreenShot(void)
{
#ifdef HAVE_LIBPNG
  char filename[64];
  int i;

  for (i=0; i<100; i++)
  {
    sprintf(filename, "screenshot%02d.png", i);
    if (access(filename, R_OK)) break;
  }
  if (i < 100)
  {
    FILE *fp = fopen(filename, "wb");
    if (fp != NULL)
    {
      png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
      if (png_ptr != NULL)
      {
        png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
	  png_destroy_write_struct(&png_ptr, 0);
	  fclose(fp);
	  return;
	}
	else
	{
	  const pixel_t *pixels = (const pixel_t*)screens[0];
	  png_byte **rowPtrs = NULL;
	  png_byte *row = NULL;
	  png_color_8 sigBit;
	  png_text infoText[1];
	  png_time imgTime;
#if (LD_PIXEL_DEPTH == 3)
	  int colourType = PNG_COLOR_TYPE_PALETTE;
	  png_color palette[256];
#else
	  int colourType = PNG_COLOR_TYPE_RGB;
#endif

	  if (setjmp(png_jmpbuf(png_ptr)))
	  {
	    png_destroy_write_struct(&png_ptr, &info_ptr);
	    if (rowPtrs != NULL)
	    {
	      free(rowPtrs);
	      rowPtrs = NULL;
	    }
	    if (row != NULL)
	    {
	      free(row);
	      row = NULL;
	    }
	    fclose(fp);
	    return;
	  }

	  png_init_io(png_ptr, fp);
	  png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
	  png_set_IHDR(png_ptr, info_ptr, SCREENWIDTH, SCREENHEIGHT, 8, colourType,
		       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

#if (LD_PIXEL_DEPTH == 3)
	  for (i=0; i<256; i++)
	  {
	    palette[i].red =   (colors[i].red >> 8);
	    palette[i].green = (colors[i].green >> 8);
	    palette[i].blue =  (colors[i].blue >> 8);
	  }
	  png_set_PLTE(png_ptr, info_ptr, palette, 256);
#endif
	  sigBit.red = 8; sigBit.green = 8; sigBit.blue = 8; sigBit.gray = 8;
	  png_set_sBIT(png_ptr, info_ptr, &sigBit);

	  infoText[0].key = "Description";
	  infoText[0].text = "DIY Screenshot";
	  infoText[0].text_length = strlen(infoText[0].text);
	  infoText[0].compression = PNG_TEXT_COMPRESSION_NONE;
	  png_set_text(png_ptr, info_ptr, infoText, 1);

	  png_convert_from_time_t(&imgTime, time(NULL));
	  png_set_tIME(png_ptr, info_ptr, &imgTime);

	  png_write_info(png_ptr, info_ptr);

#if (LD_PIXEL_DEPTH == 3)
	  rowPtrs = (png_byte**)malloc(SCREENHEIGHT * sizeof(png_byte*));
	  for (i=0; i<SCREENHEIGHT; i++)
	    rowPtrs[i] = ((png_byte*)pixels) + i*SCREENWIDTH;
	  png_write_image(png_ptr, rowPtrs);
	  free(rowPtrs);
	  rowPtrs = NULL;
#else
	  row = (png_byte*)malloc(SCREENWIDTH * 3);
	  for (i=0; i<SCREENHEIGHT; i++)
	  {
	    int j;
	    /*printf("row %d/%d\n", i, SCREENHEIGHT); fflush(stdout);*/
	    for (j=0; j<SCREENWIDTH; j++, pixels++)
	    {
	      READ_RGB_PIXEL(*pixels, row[3*j], row[3*j+1], row[3*j+2]);
	    }
	    png_write_row(png_ptr, row);
	  }
	  free(row);
	  row = NULL;
#endif
	  png_write_end(png_ptr, info_ptr);
	  png_destroy_write_struct(&png_ptr, &info_ptr);
        }
      }
      fclose(fp);
    }
  }
#endif
}

#if (LD_PIXEL_DEPTH == 3)
static unsigned	int exptable[256];

static void InitExpand (void)
{
    int		i;

    for (i=0 ; i<256 ; i++)
	exptable[i] = i | (i<<8) | (i<<16) | (i<<24);
}

static double	exptable2[256*256];

static void InitExpand2 (void)
{
    int		i;
    int		j;
    /* UNUSED unsigned	iexp, jexp;*/
    double*	exp;
    union
    {
	double 		d;
	unsigned	u[2];
    } pixel;

    printf ("building exptable2...\n");
    exp = exptable2;
    for (i=0 ; i<256 ; i++)
    {
	pixel.u[0] = i | (i<<8) | (i<<16) | (i<<24);
	for (j=0 ; j<256 ; j++)
	{
	    pixel.u[1] = j | (j<<8) | (j<<16) | (j<<24);
	    *exp++ = pixel.d;
	}
    }
    printf ("done.\n");
}

static int	inited;

static void Expand4
( const unsigned int*	lineptr,
  double*	xline )
{
    unsigned	x;
    unsigned 	y;
    unsigned	fourpixels;
    double*	exp;
    double*	row0;

    exp = exptable2;
    if (!inited)
    {
	inited = 1;
	InitExpand2 ();
    }

    y = SCREENHEIGHT-1;
    do
    {
	row0 = xline;
	x = SCREENWIDTH;

	do
	{
	    fourpixels = lineptr[0];
	    xline[0] = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[1] = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );

	    fourpixels = lineptr[1];
	    xline[2] = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[3] = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );

	    fourpixels = lineptr[2];
	    xline[4] = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[5] = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );

	    fourpixels = lineptr[3];
	    xline[6] = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[7] = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );

	    lineptr+=4;
	    xline+=8;
	} while (x-=16);

	/* xline is now at row1 start; row0 is hot in cache - replicate with memcpy */
	memcpy(xline,       row0, X_width);
	memcpy(xline + 160, row0, X_width);
	memcpy(xline + 320, row0, X_width);
	xline += 3*SCREENWIDTH/2;
    } while (y--);
}

#else

static void InitExpand(void) {;}

static void InitExpand2(void) {;}

static void Expand4
( const unsigned int* lineptr,
  double*	xline )
{
    printf("-4 not supported!\n");
}
#endif
