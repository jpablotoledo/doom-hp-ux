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
/*	WAD I/O functions.*/

/*-----------------------------------------------------------------------------*/


#ifndef __W_WAD__
#define __W_WAD__


#ifdef __GNUG__
#pragma interface
#endif

#include "doomtype.h"



/* TYPES*/

typedef struct
{
    /* Should be "IWAD" or "PWAD".*/
    char		identification[4];
    int			numlumps;
    int			infotableofs;

} wadinfo_t;


typedef struct
{
    int			filepos;
    int			size;
    char		name[8];

} filelump_t;


/* New for handling compressed WADs */
typedef enum
{
    compress_none=0,
    compress_zarquon
} compress_type_t;


typedef struct
{
    FILE*		handle;
    compress_type_t	compressType;
} waddesc_t;




/* WADFILE I/O related stuff.*/

typedef enum
{
    ns_global=0,
    ns_sprites,
    ns_flats,
    ns_colormaps
} namespace_t;

typedef struct
{
    char	name[8];
    int		position;
    int		size;
    short	wadnr;
    /* if we use namespace_t, GCC2.7.2 uses 4 bytes on RISC OS */
    byte	namespace;
} lumpinfo_t;



extern	void**		lumpcache;
extern	lumpinfo_t*	lumpinfo;
extern	int		numlumps;

extern	int*		sortedlumps;
extern	int		numsorted;


void	W_ExtractFileBase(const char *path, char *dest);

void    W_InitMultipleFiles (const char** filenames);
void    W_Reload (void);

#define W_CheckNumForName(name) (W_CheckNumForName)(name, ns_global)
int     (W_CheckNumForName)(const char* name, namespace_t);
int     W_CheckNumForName2(const char* name, namespace_t);
  /* the '2' variants try the given namespace then fall back on ns_global */

#define W_GetNumForName(name) (W_GetNumForName)(name, ns_global)
int	(W_GetNumForName)(const char* name, namespace_t);
int	W_GetNumForName2(const char* name, namespace_t);

int	W_LumpLength (int lump);
void    W_ReadLump (int lump, void *dest);

void*	W_CacheLumpNum (int lump, int tag);

#define W_CacheLumpName(name,tag) W_CacheLumpNum(W_GetNumForName(name),(tag))
#define W_CacheLumpName2(name,space,tag) W_CacheLumpNum(W_GetNumForName2((name),(space)),(tag))


#ifdef DIYBOOM

typedef struct prelumpinfo_t {
    char name[8];
    int size;
    const void *data;
} prelumpinfo_t;

extern const prelumpinfo_t	predefined_lumps[];
extern const int		num_predefined_lumps;

#endif

#endif
/*-----------------------------------------------------------------------------*/

/* $Log:$*/

/*-----------------------------------------------------------------------------*/
