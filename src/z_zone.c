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
/*	Zone Memory Allocation. Neat.*/

/*-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: z_zone.c,v 1.4 1997/02/03 16:47:58 b1 Exp $";

#include "z_zone.h"
#include "i_system.h"
#include "doomdef.h"
#include "doomstat.h"



/* ZONE MEMORY ALLOCATION*/

/* There is never any space between memblocks,*/
/*  and there will never be two contiguous free memblocks.*/
/* The rover can be left pointing at a non-empty block.*/

/* It is of no value to free a cachable block,*/
/*  because it will get overwritten automatically if needed.*/


#define ZONEID	0x1d4a11


/*
 *  NATIVE_MEMORY_ALLOCATION is for debugging purposes only. It doesn't
 *  behave nicely at all when it's running out of memory, the sole reason
 *  for using the native memory subsystem (malloc/free) is to be able
 *  to use Purify on Unix systems to hunt down memory errors in Z_Zone
 *  blocks. Make sure you haven't defined NATIVE_MEMORY_ALLOCATION unless
 *  you're debugging (and on Unix).
 */

#ifndef NATIVE_MEMORY_ALLOCATION

/* standard Z_Zone, Doom's own memory manager */

typedef struct
{
    /* total bytes malloced, including header*/
    int		size;

    /* start / end cap for linked list*/
    memblock_t	blocklist;

    memblock_t*	rover;

} memzone_t;



static memzone_t*	mainzone;




#if 0
/* Z_ClearZone*/

static void Z_ClearZone (memzone_t* zone)
{
    memblock_t*		block;

    /* set the entire zone to one free block*/
    zone->blocklist.next =
	zone->blocklist.prev =
	block = (memblock_t *)( (byte *)zone + sizeof(memzone_t) );

    zone->blocklist.user = (void *)zone;
    zone->blocklist.tag = PU_STATIC;
    zone->rover = block;

    block->prev = block->next = &zone->blocklist;

    /* NULL indicates a free block.*/
    block->user = NULL;

    block->size = zone->size - sizeof(memzone_t);
}
#endif




/* Z_Init*/

void Z_Init (void)
{
    memblock_t*	block;
    int		size;

    mainzone = (memzone_t *)I_ZoneBase (&size);
    mainzone->size = size;

    if (mainzone == NULL)
    {
        I_Error("Unable to claim memory!");
    }

    /* set the entire zone to one free block*/
    mainzone->blocklist.next =
	mainzone->blocklist.prev =
	block = (memblock_t *)( (byte *)mainzone + sizeof(memzone_t) );

    mainzone->blocklist.user = (void *)mainzone;
    mainzone->blocklist.tag = PU_STATIC;
    mainzone->rover = block;

    block->prev = block->next = &mainzone->blocklist;

    /* NULL indicates a free block.*/
    block->user = NULL;

    block->size = mainzone->size - sizeof(memzone_t);
}



/* Z_Free*/

void Z_Free (void* ptr)
{
    memblock_t*		block;
    memblock_t*		other;

    block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
	I_Error ("Z_Free: freed a pointer without ZONEID");

    if (block->user > (void **)0x100)
    {
	/* smaller values are not pointers*/
	/* Note: OS-dependend?*/

	/* clear the user's mark*/
	*block->user = 0;
    }

    /* mark as free*/
    block->user = NULL;
    block->tag = 0;
    block->id = 0;

    other = block->prev;

    if (!other->user)
    {
	/* merge with previous free block*/
	other->size += block->size;
	other->next = block->next;
	other->next->prev = other;

	if (block == mainzone->rover)
	    mainzone->rover = other;

	block = other;
    }

    other = block->next;
    if (!other->user)
    {
	/* merge the next free block onto the end*/
	block->size += other->size;
	block->next = other->next;
	block->next->prev = block;

	if (other == mainzone->rover)
	    mainzone->rover = block;
    }
}




/* Z_Malloc*/
/* You can pass a NULL user if the tag is < PU_PURGELEVEL.*/

#define MINFRAGMENT		64


void*
Z_MallocNoAbort
( int		size,
  int		tag,
  void*		user )
{
    int		extra;
    memblock_t*	start;
    memblock_t* rover;
    memblock_t* newblock;
    memblock_t*	base;

    size = (size + 3) & ~3;

    /* scan through the block list,*/
    /* looking for the first free block*/
    /* of sufficient size,*/
    /* throwing out any purgable blocks along the way.*/

    /* account for size of block header*/
    size += sizeof(memblock_t);

    /* if there is a free block behind the rover,*/
    /*  back up over them*/
    base = mainzone->rover;

    if (!base->prev->user)
	base = base->prev;

    rover = base;
    start = base->prev;

    do
    {
	if (rover == start)
	{
	    /* scanned all the way around the list*/
	    return NULL;
	}

	if (rover->user)
	{
	    if (rover->tag < PU_PURGELEVEL)
	    {
		/* hit a block that can't be purged,*/
		/*  so move base past it*/
		base = rover = rover->next;
	    }
	    else
	    {
		/* free the rover block (adding the size to base)*/

		/* the rover can be the base block*/
		base = base->prev;
		Z_Free ((byte *)rover+sizeof(memblock_t));
		base = base->next;
		rover = base->next;
	    }
	}
	else
	    rover = rover->next;
    } while (base->user || base->size < size);


    /* found a block big enough*/
    extra = base->size - size;

    if (extra >  MINFRAGMENT)
    {
	/* there will be a free fragment after the allocated block*/
	newblock = (memblock_t *) ((byte *)base + size );
	newblock->size = extra;

	/* NULL indicates free block.*/
	newblock->user = NULL;
	newblock->tag = 0;
	newblock->prev = base;
	newblock->next = base->next;
	newblock->next->prev = newblock;

	base->next = newblock;
	base->size = size;
    }

    if (user)
    {
	/* mark as an in use block*/
	base->user = user;
	*(void **)user = (void *) ((byte *)base + sizeof(memblock_t));
    }
    else
    {
	if (tag >= PU_PURGELEVEL)
	    I_Error ("Z_Malloc: an owner is required for purgable blocks");

	/* mark as in use, but unowned	*/
	base->user = (void *)2;
    }
    base->tag = tag;

    /* next allocation will start looking here*/
    mainzone->rover = base->next;

    base->id = ZONEID;

    return (void *) ((byte *)base + sizeof(memblock_t));
}




/* Z_FreeTags*/

void
Z_FreeTags
( int		lowtag,
  int		hightag )
{
    memblock_t*	block;
    memblock_t*	next;

    for (block = mainzone->blocklist.next ;
	 block != &mainzone->blocklist ;
	 block = next)
    {
	/* get link before freeing*/
	next = block->next;

	/* free block?*/
	if (!block->user)
	    continue;

	if (block->tag >= lowtag && block->tag <= hightag)
	    Z_Free ( (byte *)block+sizeof(memblock_t));
    }
}




/* Z_DumpHeap*/
/* Note: TFileDumpHeap( stdout ) ?*/

void
Z_DumpHeap
( int		lowtag,
  int		hightag )
{
    memblock_t*	block;

    fprintf (logfile, "zone size: %i  location: %p\n",
	    mainzone->size,mainzone);

    fprintf (logfile, "tag range: %i to %i\n",
	    lowtag, hightag);

    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
	if (block->tag >= lowtag && block->tag <= hightag)
	    fprintf (logfile, "block:%p    size:%7i    user:%p    tag:%3i\n",
		    block, block->size, block->user, block->tag);

	if (block->next == &mainzone->blocklist)
	{
	    /* all blocks have been hit*/
	    break;
	}

	if ( (byte *)block + block->size != (byte *)block->next)
	    fprintf (logfile, "ERROR: block size does not touch the next block\n");

	if ( block->next->prev != block)
	    fprintf (logfile, "ERROR: next block doesn't have proper back link\n");

	if (!block->user && !block->next->user)
	    fprintf (logfile, "ERROR: two consecutive free blocks\n");
    }
}



/* Z_FileDumpHeap*/

void Z_FileDumpHeap (FILE* f)
{
    memblock_t*	block;

    fprintf (f,"zone size: %i  location: %p\n",mainzone->size,mainzone);

    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
	fprintf (f,"block:%p    size:%7i    user:%p    tag:%3i\n",
		 block, block->size, block->user, block->tag);

	if (block->next == &mainzone->blocklist)
	{
	    /* all blocks have been hit*/
	    break;
	}

	if ( (byte *)block + block->size != (byte *)block->next)
	    fprintf (f,"ERROR: block size does not touch the next block\n");

	if ( block->next->prev != block)
	    fprintf (f,"ERROR: next block doesn't have proper back link\n");

	if (!block->user && !block->next->user)
	    fprintf (f,"ERROR: two consecutive free blocks\n");
    }
}




/* Z_CheckHeap*/

void Z_CheckHeap (void)
{
    memblock_t*	block;

    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
	if (block->next == &mainzone->blocklist)
	{
	    /* all blocks have been hit*/
	    break;
	}

	if ( (byte *)block + block->size != (byte *)block->next)
	    I_Error ("Z_CheckHeap: block size does not touch the next block\n");

	if ( block->next->prev != block)
	    I_Error ("Z_CheckHeap: next block doesn't have proper back link\n");

	if (!block->user && !block->next->user)
	    I_Error ("Z_CheckHeap: two consecutive free blocks\n");
    }
}





/* Z_ChangeTag*/

void
Z_ChangeTag2
( const void*	ptr,
  int		tag )
{
    memblock_t*	block;

    block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
	I_Error ("Z_ChangeTag: freed a pointer without ZONEID");

    if (tag >= PU_PURGELEVEL && (unsigned)block->user < 0x100)
	I_Error ("Z_ChangeTag: an owner is required for purgable blocks");

    block->tag = tag;
}




/* Z_FreeMemory*/

int Z_FreeMemory (void)
{
    memblock_t*		block;
    int			free;

    free = 0;

    for (block = mainzone->blocklist.next ;
	 block != &mainzone->blocklist;
	 block = block->next)
    {
	if (!block->user || block->tag >= PU_PURGELEVEL)
	    free += block->size;
    }
    return free;
}

#else

/* Z_Zone emulated using standard memory subsytems. For debugging on Unix only! */

#include <stdlib.h>
#include "doomstat.h"

static memblock_t *mem_first = NULL;
static memblock_t *mem_last = NULL;
static memblock_t *mem_seek = NULL;
static int total_memory = 0;
static int biggest_block = 0;
#ifdef __riscos__
static int maximum_memory = 4*1024*1024;
#else
static int maximum_memory = 0;
#endif


void Z_Init(void)
{
    fprintf(logfile, "Z_Zone: using native memory management\n");
}


#define MINFRAGMENT	64

void *Z_MallocNoAbort(int size, int tag, void *user)
{
    int actual_size;
    memblock_t *ptr = NULL;
    void *memory;

    actual_size = (size + sizeof(memblock_t) + 3) & ~3;

    if ((maximum_memory == 0) || (total_memory < maximum_memory) || (actual_size > biggest_block))
        ptr = malloc(actual_size);

    if (ptr != NULL)
    {
        ptr->size = actual_size;
        ptr->id = ZONEID;
        ptr->next = NULL;
        ptr->prev = mem_last;
        if (mem_first == NULL)
        {
          mem_first = ptr; mem_seek = ptr;
        }
        else
        {
          mem_last->next = ptr;
        }
        mem_last = ptr;
        total_memory += actual_size;
        if ((actual_size > biggest_block) && (tag >= PU_PURGELEVEL)) biggest_block = actual_size;
        /*fprintf(logfile, "a %p : %d (%d)\n", ptr, actual_size, total_memory);*/
    }
    else
    {
        memblock_t *best_match, *first_match;

        best_match = NULL; first_match = NULL;
        if (mem_first != NULL)
        {
            int best_size;

            if (mem_seek == NULL) mem_seek = mem_first;
            ptr = mem_seek; best_size = 0x7fffffff;
            while (ptr->next != mem_seek)
            {
                if ((ptr->tag >= PU_PURGELEVEL) && (ptr->size >= actual_size))
                {
                    if (first_match == NULL) first_match = ptr;
                    if (ptr->size < best_size)
                    {
                        best_match = ptr; best_size = ptr->size;
                    }
                    if (ptr->size <= actual_size + MINFRAGMENT) break;
                }
                ptr = ptr->next; if (ptr == NULL) ptr = mem_first;
            }
            if (ptr->next == mem_seek)	/* Didn't find a good enough match? */
            {
                best_match = first_match;
            }
            if (best_match == NULL)
            {
                return NULL;
            }
        }
        ptr = best_match;
        if (ptr->user != NULL)
        {
            *(ptr->user) = NULL;
        }
        mem_seek = best_match->next;
        if (mem_seek == NULL) mem_seek = mem_first;
        /*fprintf(logfile, "a %p : %d / %d\n", ptr, actual_size, ptr->size); fflush(logfile);*/
    }
    memory = (void*)((byte*)ptr + sizeof(memblock_t));
    ptr->user = (void**)user;
    ptr->tag = tag;
    if (user != NULL) *(void**)user = memory;

    return memory;
}


void Z_Free(void *memory)
{
    memblock_t *ptr;

    ptr = (memblock_t*)((byte*)memory - sizeof(memblock_t));
    if (ptr->user != NULL) *(ptr->user) = NULL;

    if ((ptr->next == NULL) && (ptr->prev == NULL))
    {
        mem_first = NULL; mem_last = NULL; mem_seek = NULL;
    }
    else
    {
        if (ptr->prev == NULL) mem_first = ptr->next; else ptr->prev->next = ptr->next;
        if (ptr->next == NULL) mem_last = ptr->prev; else ptr->next->prev = ptr->prev;
    }
    if (mem_seek == ptr) mem_seek = mem_first;
    total_memory -= ptr->size;
    /*fprintf(logfile, "f %p (%d)\n", ptr, total_memory); fflush(logfile);*/

    free(ptr);

    if (maximum_memory != 0)
    {
        int biggest;

        ptr = mem_first; biggest = 0;
        while (ptr != NULL)
        {
            if (ptr->size > biggest) biggest = ptr->size;
            ptr = ptr->next;
        }
        biggest_block = biggest;
    }
}


void Z_FreeTags(int lowtag, int hightag)
{
    memblock_t *ptr, *next;

    ptr = mem_first;
    while (ptr != NULL)
    {
        next = ptr->next;
        if ((ptr->user != NULL) && (ptr->tag >= lowtag) && (ptr->tag <= hightag))
            Z_Free((byte*)ptr + sizeof(memblock_t));

        ptr = next;
    }
}


void Z_ChangeTag2(const void *memory, int tag)
{
    memblock_t *ptr;

    ptr = (memblock_t*)((byte*)memory - sizeof(memblock_t));
    ptr->tag = tag;
}


void Z_CheckHeap(void)
{
}


int Z_FreeMemory(void)
{
    return 0;
}

#endif



void*
Z_Malloc
( int		size,
  int		tag,
  void*		user )
{
    void*	result;

    if ((result = Z_MallocNoAbort(size, tag, user)) == NULL)
    {
	I_Error ("Z_Malloc: failed on allocation of %i bytes", size);
    }
    return result;
}
