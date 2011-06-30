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
/*	Endianess handling, swapping 16bit and 32bit.*/

/*-----------------------------------------------------------------------------*/


#ifndef __M_SWAP__
#define __M_SWAP__


#ifdef __GNUG__
#pragma interface
#endif


/* Endianess handling.*/
/* WAD files are stored little endian.*/
#ifdef __BIG_ENDIAN__
unsigned short	SwapSHORT(unsigned short);
unsigned long	SwapLONG(unsigned long);
#define SHORT(x)	((short)SwapSHORT((unsigned short) (x)))
#define LONG(x)         ((long)SwapLONG((unsigned long) (x)))
#else
#define SHORT(x)	(x)
#define LONG(x)         (x)
#endif


/* These macros is the same for all endian types! */
#define READ_UA_SHORT(p)	((short)((p)[0] | ((p)[1] << 8)))
#define READ_UA_LONG(p)	((long)((p)[0] | ((p)[1] << 8) | ((p)[2] << 16) | ((p)[3] << 24)))


#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
