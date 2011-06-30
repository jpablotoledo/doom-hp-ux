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
/*	Main program, simply calls D_DoomMain high level loop.*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: i_main.c,v 1.4 1997/02/03 22:45:10 b1 Exp $";



#include "doomdef.h"

#include "m_argv.h"
#include "d_main.h"

#if (defined(__riscos__) && defined(__UNIXLIB_TYPES_H))
#include <unistd.h>
#ifndef __UNAME_NO_PROCESS
/* Are we using UnixLib 3.8 rather than 3.7? */
#include <unixlib/local.h>
#endif
#endif

int
main
( int		argc,
  char**	argv )
{
    myargc = argc;
    myargv = argv;

    /* In RISC OS + UNIXLIB we switch OFF filename conversions! */
#if (defined(__riscos__) && defined(__UNIXLIB_TYPES_H))
#ifdef __RISCOSIFY_NO_PROCESS
    __riscosify_control = __RISCOSIFY_NO_PROCESS;
#else
    __uname_control = __UNAME_NO_PROCESS;
#endif
#endif
    D_DoomMain ();

    return 0;
}
