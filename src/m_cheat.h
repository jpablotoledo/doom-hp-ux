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
/*	Cheat code checking.*/

/*-----------------------------------------------------------------------------*/


#ifndef __M_CHEAT__
#define __M_CHEAT__

/* CHEAT SEQUENCE PACKAGE*/

typedef struct {
  unsigned long low, high;
} cheat_code_s;

extern struct cheat_s {
  const /*unsigned*/ char *cheat;
  const char *const deh_cheat;
  enum {
    always   = 0,
    not_dm   = 1,
    not_coop = 2,
    not_demo = 4,
    not_menu = 8,
    not_deh = 16,
    not_net = not_dm | not_coop
  } const when;
  void (*const func)();
  const int arg;
  cheat_code_s code, mask;
} cheat[];

int M_FindCheats (int key);

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
