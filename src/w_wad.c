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
/*	Handles WAD file header, directory, lump I/O.*/

/*-----------------------------------------------------------------------------*/


static const char
rcsid[] = "$Id: w_wad.c,v 1.5 1997/02/03 16:47:57 b1 Exp $";


#ifdef NORMALUNIX
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#define O_BINARY		0
#endif

#ifdef __riscos__
#include "ROsupport.h"
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "doomstat.h"
#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"
#include "p_spec.h"

/* fastlz decompression */
#include "fastlz.h"





/* GLOBALS*/

/* 12 bytes following header are used to recognize compressed WADs */
static const char	compress_ident_zarquon[] = "!COMPRESS00!";


/* Location of each lump on disk.*/
lumpinfo_t*		lumpinfo;
int			numlumps;

/* contains lump numbers so that names are sorted */
int*			sortedlumps;
int			numsorted;

static int*		sameaslump=NULL;

void**			lumpcache;

static waddesc_t*	wad_descriptors;
static short		wad_number;


#define strcmpi	strcasecmp

static void strupr (char* s)
{
    while (*s) { *s = toupper(*s); s++; }
}


static int filelength (FILE *handle)
{
    size_t oldpos, endpos;

    oldpos = ftell (handle);
    fseek (handle, 0, SEEK_END);
    endpos = ftell (handle);
    fseek (handle, oldpos, SEEK_SET);
    return endpos;
}


void
W_ExtractFileBase
( const char*	path,
  char*		dest )
{
    const char*	src;
    int		length;

    src = path + strlen(path) - 1;

    /* back up until a \ or the start*/
    while (src != path
#ifdef __riscos__
	   && *(src-1) != ':'
	   && *(src-1) != '.')
#else
	   && *(src-1) != '\\'
	   && *(src-1) != '/')
#endif
    {
	src--;
    }

    /* copy up to eight characters*/
    memset (dest,0,8);
    length = 0;

    while (*src && *src != EXTSEPCHR)
    {
	if (++length == 9)
	    I_Error ("Filename base of %s >8 chars",path);

	*dest++ = toupper((int)*src++);
    }
}






/* Decompression interface to libfastlz */
static fastlz_decompfile_context_t decompCtx;

static void *decompress_z_zone_alloc(unsigned int size)
{
  return Z_MallocNoAbort(size, PU_STATIC, NULL);
}

static void decompress_z_zone_free(void *mem)
{
  Z_Free(mem);
}

static void W_InitWadDecompression(void)
{
  fastlz_decompress_context_t *ctx = &decompCtx.basectx;
  decompCtx.file = NULL;
  fastlz_decompress_init(ctx);
  ctx->fastlz_alloc = decompress_z_zone_alloc;
  ctx->fastlz_free  = decompress_z_zone_free;
  ctx->fastlz_refill = fastlz_file_refill;
  ctx->ioBuffSize = 0x10000;
}



/* LUMP BASED ROUTINES.*/



/* New functions to sort the lumpinfo and eliminate duplicates */

#define SWAP_LUMPS(x, y) \
    aux = sortedlumps[x]; sortedlumps[x] = sortedlumps[y]; sortedlumps[y] = aux;

/* Linearized quicksort */
static void W_QuicksortLumps(int from, int to)
{
    int		from_stack[32];
    int		to_stack[32];
    int		stackitems = 0;

    do
    {
	while (from < to)
	{
	    int i, j, name0, name1;
	    byte name2;
	    int aux;

	    j = (from + to) / 2;
	    SWAP_LUMPS(from, j);
	    j = from;
	    name0 = ((int*)lumpinfo[sortedlumps[from]].name)[0];
	    name1 = ((int*)lumpinfo[sortedlumps[from]].name)[1];
	    name2 = lumpinfo[sortedlumps[from]].namespace;
	    for (i=from+1; i<=to; i++)
	    {
		int test0 = ((int*)lumpinfo[sortedlumps[i]].name)[0];
		int test1 = ((int*)lumpinfo[sortedlumps[i]].name)[1];
		byte test2 = lumpinfo[sortedlumps[i]].namespace;
		if (test0 < name0 ||
		    (test0 == name0 &&
		     (test1 < name1 || (test1 == name1 && test2 < name2))))
		{
		    j++;
		    SWAP_LUMPS(i, j);
		}
	    }
	    SWAP_LUMPS(from, j);

	    if ((j - from) < (to - j))
	    {
		from_stack[stackitems] = j+1;
		to_stack[stackitems] = to;
		to = j-1;
	    }
	    else
	    {
		from_stack[stackitems] = from;
		to_stack[stackitems] = j-1;
		from = j+1;
	    }
	    stackitems++;
	    if (stackitems >= 32)
	    {
		I_Error("W_QuicksortLumps: fatal overflow!");
	    }
	}
	if (--stackitems >= 0)
	{
	    from = from_stack[stackitems];
	    to = to_stack[stackitems];
	}
    }
    while (stackitems >= 0);
}


/* sort the lumps by (wadnr, offset) to find shared lumps */
static void W_QuicksortLumpDir(int from, int to)
{
    int		from_stack[32];
    int		to_stack[32];
    int		stackitems = 0;

    do
    {
        while (from < to)
        {
            int i, j, aux, position;
            short wadnr;

            j = (from + to) / 2;
            SWAP_LUMPS(from, j);
            j = from;
            wadnr = lumpinfo[sortedlumps[from]].wadnr;
            position = lumpinfo[sortedlumps[from]].position;
            for (i=from+1; i<=to; i++)
            {
                short newnr = lumpinfo[sortedlumps[i]].wadnr;

                if ((newnr < wadnr) || ((newnr == wadnr) && (lumpinfo[sortedlumps[i]].position < position)))
                {
                    j++;
                    SWAP_LUMPS(i, j);
                }
            }
            SWAP_LUMPS(from, j);

            if ((j - from) < (to - j))
            {
                from_stack[stackitems] = j+1;
                to_stack[stackitems] = to;
                to = j-1;
            }
            else
            {
                from_stack[stackitems] = from;
                to_stack[stackitems] = j-1;
                from = j+1;
            }
            stackitems++;
            if (stackitems >= 32)
            {
                I_Error("W_QuicksortLumpDir: fatal overflow");
            }
        }
        if (--stackitems >= 0)
        {
            from = from_stack[stackitems];
            to = to_stack[stackitems];
        }
    }
    while (stackitems >= 0);
}


static void W_ProcessSharedLumps(void)
{
    /* requires sortedlumps array to be in memory (but not initialized) */
    if ((sameaslump = (int*)malloc(numlumps * sizeof(int))) == NULL)
        sameaslump = (int*)Z_MallocNoAbort(numlumps * sizeof(int), PU_STATIC, NULL);
    if (sameaslump == NULL)
    {
        fprintf(logfile, "W_ProcessSharedLumps: couldn't claim memory for shared lumps\n");
        return;
    }
    else
    {
        int i, numshared, sizeshared;

        for (i=0; i<numlumps; i++) sortedlumps[i] = i;

        W_QuicksortLumpDir(0, numlumps-1);

        i = 0; numshared = 0; sizeshared = 0;
        while (i<numlumps)
        {
            lumpinfo_t*	ref;
            int		j;

            ref = lumpinfo + sortedlumps[i];
            sameaslump[sortedlumps[i]] = -1;
            j = i+1;

            /*
             *  Size is important ;-)
             *  Consider MAP## and THINGS which can have the same offset, but of course
             *  MAP## is size 0. Load THINGS and get MAP## instead... ouch!
             *  Don't do anything if the size is 0.
             */
            if (ref->size != 0)
            {
                for (; j<numlumps; j++)
                {
                    lumpinfo_t*	chk = lumpinfo + sortedlumps[j];

                    if ((chk->wadnr != ref->wadnr) || (chk->position != ref->position) || (chk->size != ref->size)) break;
                    sameaslump[sortedlumps[j]] = sortedlumps[i];
                    numshared++; sizeshared += ref->size;
                }
            }
            i = j;
        }
        fprintf(logfile, "W_ProcessSharedLumps: %d shared lumps found, %d bytes\n", numshared, sizeshared);

#if 0
        for (i=0; i<numlumps; i++)
        {
            char lname[9];
            lumpinfo_t *l;

            l = lumpinfo + i;
            strncpy(lname, l->name, 8); lname[8] = 0;
            fprintf(logfile, "%4d: %8s %2d %7d", i, lname, l->wadnr, l->position);
            if (sameaslump[i] != -1)
            {
                l = lumpinfo + sameaslump[i];
                strncpy(lname, l->name, 8);
                fprintf(logfile, " == %4d: %8s %2d %7d", sameaslump[i], lname, l->wadnr, l->position);
            }
            fprintf(logfile, "\n");
        }
#endif
    }
}


#if 0
/* Debug only */
static void W_PrintLumps(void)
{
    FILE *fp;
    int i, j;
    char *b;

    fp = fopen("lumps", "w");

    fprintf(fp, "lumps: %d, unique: %d\n", numlumps, numsorted);

    for (i=0; i<numsorted; i++)
    {
	b = lumpinfo[sortedlumps[i]].name;
	for (j=0; (j<8) && (b[j] != '\0'); j++) fprintf(fp, "%c", b[j]);
	while (j<12) {fprintf(fp, " "); j++;}
	fprintf(fp, "%d\n", lumpinfo[sortedlumps[i]].position);
    }

    fclose(fp);
}
#endif



typedef union ci_union {
  char	c[9];
  int	i[2];
} ci_union;


static int W_CheckNextNumForName (const char* name, int num) /* -1 on first call */
{
    ci_union name8;
    int v1, v2, lump=num;

    name8.i[0]=0; name8.i[1]=0; name8.c[8]=0;

    strncpy (name8.c,name,8);
    strupr (name8.c);

    v1 = name8.i[0];
    v2 = name8.i[1];

    /*fprintf(logfile,"Next lump %s - init %i, found ",name,lump);*/

    /* This is deliberately written as a linear search... */
    while (++lump<numlumps)
    {
	if ((v1 == ((int*)lumpinfo[lump].name)[0])
	    && (v2 == ((int*)lumpinfo[lump].name)[1]))
	{
	    /*fprintf(logfile,"%i\n",lump);*/
	    return lump;
	}
    }
    /*fprintf(logfile,"(no)\n");*/
    return -1;
}


/* Merge sprite / patch / flat areas */
static void W_MergePatchArea(const char *start, const char *end, const char *pstart, const char *pend, const char *dummy, namespace_t space)
{
    int		i, j, k, l;
    lumpinfo_t*	lump;

    /* gather all of the area lumps */
    i = W_CheckNextNumForName(start, -1);
    j = W_CheckNextNumForName(end, i);

    if ((i>=0) && (j>=0))
    {
	k = W_CheckNextNumForName(start, j);
	if (k<0) k = W_CheckNextNumForName(pstart, j);

	while (k>=0)
	{
	    strncpy(lumpinfo[k].name, dummy, 8);
	    l=W_CheckNextNumForName(end,k);
	    if (l<0) l=W_CheckNextNumForName(pend,k);
	    if (l<0) break;
	    strncpy(lumpinfo[l].name, dummy, 8);
	    lump = Z_Malloc((l-k-1)*sizeof(lumpinfo_t), PU_STATIC, NULL);
	    /* Save new lumps to temporary Z_Zone block */
	    memcpy(lump, lumpinfo+k+1, (l-k-1)*sizeof(lumpinfo_t));
	    /* Move X_END, ... XX_START to X_END + (XX_END - XX_START - 1) */
	    memmove(lumpinfo + j + (l-k-1), lumpinfo + j, (k-j+1)*sizeof(lumpinfo_t));
	    /* Copy new range lumps before X_END lump */
	    memcpy(lumpinfo + j, lump, (l-k-1)*sizeof(lumpinfo_t));
	    Z_Free(lump);
	    j += (l-k-1);
	    k=W_CheckNextNumForName(start, l);
	    if (k<=0) k=W_CheckNextNumForName(pstart, l);
	}
    }

    /* put them in the specified namespace */
    i = W_CheckNextNumForName(start, -1);
    j = W_CheckNextNumForName(end, i);
    while (++i < j)
	lumpinfo[i].namespace = (byte)space;
}


static void W_ProcessLumps(void)
{
    int i, j, k, l, store;

    W_MergePatchArea("S_START", "S_END", "SS_START", "SS_END", "SS_DUMMY", ns_sprites);
    W_MergePatchArea("P_START", "P_END", "PP_START", "PP_END", "PP_DUMMY", ns_global);
    W_MergePatchArea("F_START", "F_END", "FF_START", "FF_END", "FF_DUMMY", ns_flats);
    BOOMSTATEMENT(W_MergePatchArea("C_START", "C_END", "CC_START", "CC_END", "CC_DUMMY", ns_colormaps);)

    /*for (i=0; i<numlumps; i++)
    {
	char	sprname[9];

	strncpy(sprname, lumpinfo[i].name, 8); sprname[8] = 0;
	fprintf(logfile, "%s\n", sprname);
    }*/

    /* Now start sorting. */
    if ((sortedlumps = malloc(numlumps*sizeof(int))) == NULL)
    {
	I_Error("Couldn't claim memory for sortedlumps!\n");
    }

    W_ProcessSharedLumps();

    for (i=0; i<numlumps; i++) sortedlumps[i] = i;

    W_QuicksortLumps(0, numlumps-1);

    /* Eliminate duplicates, keeping the last one */
    i=0; store = 0;
    while (i < numlumps-1)
    {
	int name0 = ((int*)lumpinfo[sortedlumps[i]].name)[0];
	int name1 = ((int*)lumpinfo[sortedlumps[i]].name)[1];
	byte name2 = lumpinfo[sortedlumps[i]].namespace;
	for (j=i+1; j<numlumps; j++)
	    if ((((int*)lumpinfo[sortedlumps[j]].name)[0] != name0) || (((int*)lumpinfo[sortedlumps[j]].name)[1] != name1) || (lumpinfo[sortedlumps[j]].namespace != name2)) break;
	if (i < j-1)
	{
	    k = sortedlumps[i];
	    /* Determine the most recent lump */
	    for (l=i+1; l<j; l++)
	    {
		if (sortedlumps[l] > k) k = sortedlumps[l];
	    }
	    sortedlumps[store++] = k;
	    i = j;
	}
	else
	{
	    sortedlumps[store++] = sortedlumps[i++];
	}
    }
    /* Were the last two not identical? */
    if (i == numlumps-1)
    {
	sortedlumps[store++] = sortedlumps[i];
    }
    numsorted = store;

    /*W_PrintLumps();*/
}



/*
 *  Finds the maps in a WAD's directory and prints them sorted
 */
static int W_OutputMaps(const filelump_t *fileinfo, int numlumps)
{
    /* reserve one additional field at the end which will always be 0 */
    char doomuMaps[37];
    char doom2Maps[33];
    int  i, j, numMaps;

    /* find the maps in this WAD */
    memset(doomuMaps, 0, sizeof(doomuMaps));
    memset(doom2Maps, 0, sizeof(doom2Maps));

    for (j=0, numMaps=0; j<numlumps; j++)
    {
        const char *name = fileinfo[j].name;
        if ((name[0] == 'E') && (name[1] >= '1') && (name[1] <= '9') &&
            (name[2] == 'M') && (name[3] >= '1') && (name[3] <= '9'))
        {
            int mapnum = 9*(name[1]-'1') + (name[3]-'1');
            doomuMaps[mapnum] = 1;
            numMaps++;
        }
        else if (strncmp(name, "MAP", 3) == 0)
        {
            if ((name[3]>='0') && (name[3]<='9') && (name[4]>='0') && (name[4]<='9'))
            {
                int mapnum = 10*(name[3]-'0') + (name[4]-'0');
                if ((mapnum >= 1) && (mapnum <= 32))
                {
                    doom2Maps[mapnum-1] = 1;
                    numMaps++;
                }
            }
        }
    }

    if (numMaps > 0)
    {
        printf(" --> ");
        i=0;
        while (i<36)
        {
            while ((i < 36) && (doomuMaps[i] == 0)) i++;
            if (i < 36)
            {
                j = i;
                printf(" E%dM%d", 1 + i/9, 1 + i%9);
                /* the last field in doomuMaps is always 0 */
                while (doomuMaps[i] != 0) i++;
                if (i-1 > j)
                    printf("-E%dM%d", 1 + (i-1)/9, 1 + (i-1)%9);
            }
        }
        i=0;
        while (i<32)
        {
            while ((i < 32) && (doom2Maps[i] == 0)) i++;
            if (i < 32)
            {
                j = i;
                printf(" MAP%02d", i+1);
                /* the last field in doom2Maps is always 0 */
                while (doom2Maps[i] != 0) i++;
                if (i-1 > j)
                    printf("-MAP%02d", i);
            }
        }
    }
    return numMaps;
}



/* W_AddFile*/
/* All files are optional, but at least one file must be*/
/*  found (PWAD, if all required lumps are present).*/
/* Files with a .wad extension are wadlink files*/
/*  with multiple lumps.*/
/* Other files are single lumps with the base filename*/
/*  for the lump name.*/

/* If filename starts with a tilde, the file is handled*/
/*  specially to allow map reloads.*/
/* But: the reload feature is a fragile hack...*/

static int		reloadlump;
static const char*	reloadname;


static void W_AddFile (const char *filename)
{
    wadinfo_t		header;
    lumpinfo_t*		lump_p;
    unsigned int	i;
    FILE*               handle;
    int			length;
    int			startlump;
    filelump_t*		fileinfo;
    filelump_t*		filebase = NULL;
    filelump_t		singleinfo;
    waddesc_t*		wadDesc;

    /* open the file and add to directory*/

    /* handle reload indicator.*/
    if (filename[0] == '~')
    {
	filename++;
	reloadname = filename;
	reloadlump = numlumps;
    }
    if ( (handle = fopen (filename, "rb")) == NULL)
    {
	printf (" couldn't open %s\n",filename);
	return;
    }

    printf (" adding %s\n",filename);
    startlump = numlumps;

    wadDesc = wad_descriptors + wad_number;
    wadDesc->compressType = compress_none;

#ifdef __riscos__
    if ((i = strcmpi (filename+strlen(filename)-3 , "wad" )) != 0)
    {
	unsigned int block[4];
	if (ReadFileInfo(filename, block) != 0)
	{
	    if ((block[0] & 0xffffff00) == 0xfff16c00) i = 0;
	}
    }
    if (i != 0)
#else
    if (strcmpi (filename+strlen(filename)-3 , "wad" ) )
#endif
    {
	/* single lump file*/
	fileinfo = &singleinfo;
	singleinfo.filepos = 0;
	singleinfo.size = LONG(filelength(handle));
	W_ExtractFileBase (filename, singleinfo.name);
	numlumps++;
    }
    else
    {
	char compString[12]="";
        int  needlf;

	/* WAD file*/
        fread ((void*)&header, 1, sizeof(header), handle);
        if (strncmp(header.identification,"IWAD",4))
	{
	    /* Homebrew levels?*/
	    if (strncmp(header.identification,"PWAD",4))
	    {
		I_Error ("Wad file %s doesn't have IWAD "
			 "or PWAD id\n", filename);
	    }

	    /* ???modifiedgame = true;		*/
	}
	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);
	length = header.numlumps*sizeof(filelump_t);

	/*
	 *  Claim memory for new WAD in Z_Zone (currently empty anyway) to avoid
	 *  memory fragmentation and make failure of the realloc less likely.
	 */
	if ((filebase = Z_MallocNoAbort(length, PU_STATIC, NULL)) == NULL)
	    I_Error("W_AddFile: Couldn't claim memory for fileinfo");
	fileinfo = filebase;

	fread(compString, 1, 12, handle);
        /* fileinfo will be modified later on, so memorize it! */
        filebase = fileinfo;
	fseek (handle, header.infotableofs, SEEK_SET);
	fread ((void*)fileinfo, 1, length, handle);
	numlumps += header.numlumps;

        needlf = 0;
	if (strncmp(compString, compress_ident_zarquon, 12) == 0)
	{
	    wadDesc->compressType = compress_zarquon;
	    printf(" (Compressed WAD)");
	    needlf = 1;
	}

	needlf += W_OutputMaps(fileinfo, numlumps - startlump);

	if (needlf != 0)
	    printf("\n");
    }

    /* Fill in lumpinfo*/
    lumpinfo = realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));

    if (!lumpinfo)
	I_Error ("Couldn't realloc lumpinfo");

    lump_p = &lumpinfo[startlump];

    wadDesc->handle = reloadname ? NULL : handle;

    for (i=startlump ; i<numlumps; i++, lump_p++, fileinfo++)
    {
	lump_p->wadnr = wad_number;
	lump_p->position = LONG(fileinfo->filepos);
	lump_p->size = LONG(fileinfo->size);
	lump_p->namespace = (byte)ns_global;
	memset(lump_p->name, 0, 8);
	strncpy (lump_p->name, fileinfo->name, 8);
    }

    if (filebase != NULL) {Z_Free(filebase);}

    if (reloadname)
	fclose (handle);
}





/* W_Reload*/
/* Flushes any of the reloadable lumps in memory*/
/*  and reloads the directory.*/

void W_Reload (void)
{
    wadinfo_t		header;
    int			lumpcount;
    lumpinfo_t*		lump_p;
    unsigned		i;
    FILE*		handle;
    int			length;
    filelump_t*		fileinfo;
    filelump_t*		filebase;
    int			claimedfrom;

    if (!reloadname)
	return;

    if ( (handle = fopen (reloadname, "rb")) == NULL)
	I_Error ("W_Reload: couldn't open %s",reloadname);

    fread ((void*)&header, 1, sizeof(header), handle);
    lumpcount = LONG(header.numlumps);
    header.infotableofs = LONG(header.infotableofs);
    length = lumpcount*sizeof(filelump_t);

    claimedfrom = 0;
    if ((filebase = malloc(length)) == NULL)
    {
	claimedfrom++;
	if ((filebase = Z_MallocNoAbort(length, PU_STATIC, NULL)) == NULL)
            I_Error("W_Reload: Couldn't claim memory for fileinfo.");
    }
    fileinfo = filebase;

    fseek (handle, header.infotableofs, SEEK_SET);
    fread ((void*)fileinfo, 1, length, handle);

    /* Fill in lumpinfo*/
    lump_p = &lumpinfo[reloadlump];

    for (i=reloadlump ;
	 i<reloadlump+lumpcount ;
	 i++,lump_p++, fileinfo++)
    {
	if (lumpcache[i])
	    Z_Free (lumpcache[i]);

	lump_p->position = LONG(fileinfo->filepos);
	lump_p->size = LONG(fileinfo->size);
    }

    if (claimedfrom == 0)
	free(filebase);
    else
	Z_Free(filebase);

    fclose (handle);
}




/* W_InitMultipleFiles*/
/* Pass a null terminated list of files to use.*/
/* All files are optional, but at least one file*/
/*  must be found.*/
/* Files with a .wad extension are idlink files*/
/*  with multiple lumps.*/
/* Other files are single lumps with the base filename*/
/*  for the lump name.*/
/* Lump names can appear multiple times.*/
/* The name searcher looks backwards, so a later file*/
/*  does override all earlier ones.*/

void W_InitMultipleFiles (const char** filenames)
{
    int		size;
    BOOMSTATEMENT(int i;)
    BOOMSTATEMENT(const char *pbase;)

#ifdef DIYBOOM
    /*
     *  predefined lumps are marked by wadnr == -1. The position entry is the
     *  offset relative to the first predefined lump. They must be loaded into
     *  Z_Zone memory just like other lumps because the Boom lumps may be
     *  overloaded by a PWAD. See W_ReadLump for more.
     */
    numlumps = num_predefined_lumps;
    if ((lumpinfo = malloc(numlumps*sizeof(lumpinfo_t))) != NULL)
    {
        pbase = (const char*)(predefined_lumps[0].data);
        for (i=0; i<num_predefined_lumps; i++)
        {
            memset(lumpinfo[i].name, 0, 8);
            strncpy(lumpinfo[i].name, predefined_lumps[i].name, 8);
            lumpinfo[i].wadnr = -1;
            lumpinfo[i].position = (const char*)(predefined_lumps[i].data) - pbase;
            lumpinfo[i].size = predefined_lumps[i].size;
            lumpinfo[i].namespace = (byte)ns_global;
        }
    }
    else
#else
    /* open all the files, load headers, and count lumps*/
    numlumps = 0;

    /* will be realloced as lumps are added*/
    if ((lumpinfo = malloc(1)) == NULL)
#endif
        I_Error("W_InitMultipleFiles: Couldn't claim memory for lumpinfo");

    W_InitWadDecompression();

    for (size=0; filenames[size] != NULL; size++) ;

    if ((wad_descriptors = (waddesc_t*)malloc(size*sizeof(waddesc_t))) == NULL)
        wad_descriptors = (waddesc_t*)Z_MallocNoAbort(size*sizeof(waddesc_t), PU_STATIC, NULL);
    if (wad_descriptors == NULL)
        I_Error("W_InitMultipleFiles: Couldn't claim memory for WAD descriptors");

    for (wad_number=0 ; *filenames ; filenames++, wad_number++)
	W_AddFile (*filenames);

    if (!numlumps)
	I_Error ("W_InitMultipleFiles: no files found");

    /* set up caching*/
    size = numlumps * sizeof(*lumpcache);
    lumpcache = malloc (size);

    if (!lumpcache)
	I_Error ("Couldn't allocate lumpcache");

    memset (lumpcache,0, size);

    /* Quicksort lumps and eliminate duplicates */
    W_ProcessLumps();
}





#if 0
/* W_InitFile*/
/* Just initialize from a single file.*/

static void W_InitFile (const char* filename)
{
    const char*	names[2];

    names[0] = filename;
    names[1] = NULL;
    W_InitMultipleFiles (names);
}
#endif




/* W_NumLumps*/

int W_NumLumps (void)
{
    return numlumps;
}




/* W_CheckNumForName*/
/* Returns -1 if name not found.*/


int (W_CheckNumForName) (const char* name, namespace_t space)
{
    union {
	char	s[9];
	int	x[2];

    } name8;

    int		v1;
    int		v2;
    /* New for binary search */
    int		pos, step, count;

    /* In principle a good idea, however bytes after the string terminator are UNDEFINED!!!
       So if we want to compare two integers we have to initialise them first. */
    name8.x[0] = 0; name8.x[1] = 0;

    /* make the name into two integers for easy compares*/
    strncpy (name8.s,name,8);

    /* in case the name was a fill 8 chars*/
    name8.s[8] = 0;

    /* case insensitive*/
    strupr (name8.s);

    v1 = name8.x[0];
    v2 = name8.x[1];

    /* Re-write, does binary rather than linear search */
    pos = (numsorted+1) >> 1; step = (pos+1) >> 1; count = (pos << 1);
    while (count != 0)
    {
	int	    w1 = ((int*)lumpinfo[sortedlumps[pos]].name)[0];
	int	    w2 = ((int*)lumpinfo[sortedlumps[pos]].name)[1];
	byte	    w3 = lumpinfo[sortedlumps[pos]].namespace;

	if ((v1 == w1) && (v2 == w2) && (space == w3))
	    return sortedlumps[pos];

	if (v1 < w1 || (v1 == w1 && (v2 < w2 || (v2 == w2 && space < w3))))
	{
	    pos -= step; if (pos < 0) pos = 0;
	}
	else
	{
	    pos += step; if (pos >= numsorted) pos = numsorted-1;
	}
	step = (step+1) >> 1; count >>= 1;
    }
    /* TFB. Not found.*/
    return -1;
}



/* W_CheckNumForName2*/
/* Returns -1 if name not found. (Also tries ns_global.) */

int W_CheckNumForName2 (const char* name, namespace_t space)
{
    int i = (W_CheckNumForName)(name, space);
    return i == -1 ? W_CheckNumForName(name) : i ;
}



/* W_GetNumForName*/
/* Calls W_CheckNumForName, but bombs out if not found.*/

int (W_GetNumForName) (const char* name, namespace_t space)
{
    int	i;

    i = (W_CheckNumForName) (name, space);

    if (i == -1)
    {
	/*I_Error ("W_GetNumForName: %s not found!", name);*/
	fprintf(logfile, "W_GetNumForName: %s not found\n", name);
	/* It's a long shot, but it's better than aborting */
	i = 0;
    }

    return i;
}



/* W_GetNumForName2*/
/* Calls W_CheckNumForName, but bombs out if not found.
 * (Also tries ns_global.) */

int W_GetNumForName2 (const char* name, namespace_t space)
{
    int i = (W_CheckNumForName)(name, space);
    return i == -1 ? W_GetNumForName(name) : i;
}



/* W_LumpLength*/
/* Returns the buffer size needed to load the given lump.*/

int W_LumpLength (int lump)
{
    if (lump >= numlumps)
    {
	/*I_Error ("W_LumpLength: %i >= numlumps",lump);*/
	fprintf(logfile, "W_LumpLength: %i >= numlumps\n", lump);
	lump = 0;
    }

    return lumpinfo[lump].size;
}




/* W_ReadLump*/
/* Loads the lump into the given buffer,*/
/*  which must be >= W_LumpLength().*/

void
W_ReadLump
( int		lump,
  void*		dest )
{
    int		c;
    lumpinfo_t*	l;
    FILE*	handle;
    waddesc_t*	wadDesc;

    if (lump >= numlumps)
    {
	/*I_Error ("W_ReadLump: %i >= numlumps",lump);*/
	fprintf(logfile, "W_ReadLump: %i >= numlumps\n", lump);
	lump = 0;
    }

    l = lumpinfo+lump;

#ifdef DIYBOOM
    if (l->wadnr == -1) /* predefined lump? */
    {
        memcpy(dest, (const char*)(predefined_lumps[0].data) + l->position, l->size);
        return;
    }
#endif

    /* ??? I_BeginRead ();*/
    wadDesc = wad_descriptors + l->wadnr;
    if (wadDesc->handle == NULL)
    {
	/* reloadable file, so use open / read / close*/
        if ( (handle = fopen (reloadname, "rb")) == NULL)
	    I_Error ("W_ReadLump: couldn't open %s",reloadname);
    }
    else
	handle = wadDesc->handle;

    fseek (handle, l->position, SEEK_SET);

    c = 0;
    if (wadDesc->compressType == compress_none)
    {
	c = fread ((void*)dest, 1, l->size, handle);
    }
    else if (wadDesc->compressType == compress_zarquon)
    {
	if (l->size != 0)
	{
	    decompCtx.file = wadDesc->handle;
	    if (fastlz_decompress_block(&decompCtx.basectx, dest, l->size) == 0)
	        c = l->size;
	}
    }

    if (c < l->size)
	I_Error ("W_ReadLump: only read %i of %i on lump %i",
		 c,l->size,lump);

    if (wadDesc->handle == NULL)
        fclose (handle);

    /* ??? I_EndRead ();*/
}




/* W_CacheLumpNum*/

void*
W_CacheLumpNum
( int		lump,
  int		tag )
{
    byte*	ptr;

    if ((unsigned)lump >= numlumps)
    {
	/*I_Error ("W_CacheLumpNum: %i >= numlumps",lump);*/
	fprintf(logfile, "W_CacheLumpNum: %i >= numlumps\n", lump);
	lump = 0;
    }

    if ((sameaslump != NULL) && (sameaslump[lump] != -1)) lump = sameaslump[lump];

    if (!lumpcache[lump])
    {
	/* read the lump in*/
	/*printf ("cache miss on lump %i\n",lump);*/
	ptr = Z_Malloc (W_LumpLength (lump), tag, &lumpcache[lump]);
	W_ReadLump (lump, lumpcache[lump]);
    }
    else
    {
	/*printf ("cache hit on lump %i\n",lump);*/
	Z_ChangeTag (lumpcache[lump],tag);
    }

    return lumpcache[lump];
}



#if 0
/* W_Profile*/

static int	info[2500][10];
static int	profilecount;

void W_Profile (void)
{
    int		i;
    memblock_t*	block;
    void*	ptr;
    char	ch;
    FILE*	f;
    int		j;
    char	name[9];


    for (i=0 ; i<numlumps ; i++)
    {
	ptr = lumpcache[i];
	if (!ptr)
	{
	    ch = ' ';
	    continue;
	}
	else
	{
	    block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
	    if (block->tag < PU_PURGELEVEL)
		ch = 'S';
	    else
		ch = 'P';
	}
	info[i][profilecount] = ch;
    }
    profilecount++;

#ifdef __riscos__
    f = fopen ("Doom:waddump","w");
#else
    f = fopen ("waddump.txt","w");
#endif
    name[8] = 0;

    for (i=0 ; i<numlumps ; i++)
    {
	memcpy (name,lumpinfo[i].name,8);

	for (j=0 ; j<8 ; j++)
	    if (!name[j])
		break;

	for ( ; j<8 ; j++)
	    name[j] = ' ';

	fprintf (f,"%s ",name);

	for (j=0 ; j<profilecount ; j++)
	    fprintf (f,"    %c",info[i][j]);

	fprintf (f,"\n");
    }
    fclose (f);
}
#endif
