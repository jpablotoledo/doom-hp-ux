/*
 *  fastlzc.c - compression code
 *
 *  Written by:
 *   Andreas Dehmel <zarquon@t-online.de>
 *
 *  This file is part of libfastlz, a small and fast LZ77-based compression
 *  library originally developed for WAD compression in Doom. It is released
 *  under the GNU Public License (GPL) in the hope that it proves useful.
 *  Please note there is NO WARRANTY. For more information read the file
 *  License included in this release.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "fastlz.h"
#include "fastlzintl.h"


/* sorry it's a bit of a mess... */
typedef struct hash_entry_s {
  unsigned int off;
  unsigned short prev;
  unsigned short next;
} hash_entry_t;



/* Global variables and defined */

/* Make sure that the hashcode contains unmodified bits of each byte */
#define HASH_ENCODING(a,b,c) (((a) | ((b)<<8)) ^ ((c)&0x0f) ^ (((c)&0xf0)<<8))
#define PREV_MODE_HEAD	0x80000000



static void CompUpdateHashTable(fastlz_compress_context_t *ctx, const unsigned char *in, int offset)
{
  hash_entry_t *thisEntry;
  unsigned short entryNo;
  int hashCode;

  hashCode = HASH_ENCODING(in[offset], in[offset+1], in[offset+2]);
  /*
   *  Trap run-length data to maintain reasonable performance: only keep
   *  the first and the last triple in the hash table.
   */
  if (offset >= 2)
  {
    if ((in[offset] == in[offset+2]) && (in[offset] == in[offset+1])
     && (in[offset] == in[offset-1]) && (in[offset] == in[offset-2]))
    {
      /* This should always be true, but just to be on the safe side */
      if ((ctx->hashList[ctx->hashCodes[hashCode]].off & ~PREV_MODE_HEAD) == offset-1)
      {
        ctx->hashList[ctx->hashCodes[hashCode]].off++; return;
      }
    }
  }

  entryNo = (unsigned short)(ctx->hashEntryNo);
  thisEntry = ctx->hashList + entryNo;

  /*
   *  hashFillLevel describes how much of the hash table has been filled.
   *  That's a lot more efficient for small files ( < maxBackReference).
   */
  if (ctx->hashEntryNo < ctx->hashFillLevel)
  {
    /* First unlink thisEntry from any chains it might have been involved in */
    /* Was this the head of a hash list? */
    if ((thisEntry->off & PREV_MODE_HEAD) != 0)
    {
      /* Does it have a successor? */
      if (thisEntry->next != entryNo)
      {
        /* Yes ==> link hashCodes with it and vice versa */
        ctx->hashCodes[thisEntry->prev] = thisEntry->next;
        ctx->hashList[thisEntry->next].prev = thisEntry->prev;
        ctx->hashList[thisEntry->next].off |= PREV_MODE_HEAD;
      }
      else
      {
        /* No ==> no hashCodes defined */
        ctx->hashValid[(thisEntry->prev)>>3] &= ~(1<<((thisEntry->prev) & 7));
      }
    }
    else
    {
      /* Was anything defined for this entry? */
      if (thisEntry->prev != entryNo)
      {
        /* Does it have a successor? */
        if (thisEntry->next != entryNo)
        {
          /* Yes ==> link predecessor and successor */
          ctx->hashList[thisEntry->prev].next = thisEntry->next;
          ctx->hashList[thisEntry->next].prev = thisEntry->prev;
        }
        else
        {
          /* No ==> mark prev.next as invalid */
          ctx->hashList[thisEntry->prev].next = thisEntry->prev;
        }
      }
    }
  }
  else
  {
    (ctx->hashFillLevel)++;
  }

  /* Now link it at the head the new hashcode list */
  thisEntry->off = offset | PREV_MODE_HEAD;
  thisEntry->prev = hashCode;
  /* Does a hash list exist for this hash code? */
  if ((ctx->hashValid[hashCode>>3] & (1<<(hashCode & 7))) != 0)
  {
    /* Yes ==> add new entry */
    thisEntry->next = ctx->hashCodes[hashCode];
    ctx->hashList[thisEntry->next].prev = entryNo;
    ctx->hashList[thisEntry->next].off &= ~PREV_MODE_HEAD;
  }
  else
  {
    /* No ==> create new entry */
    thisEntry->next = entryNo;
    ctx->hashValid[hashCode>>3] |= (1<<(hashCode & 7));
  }
  ctx->hashCodes[hashCode] = entryNo;

  /* Update next entry pointer. Wrap around if it overflows */
  (ctx->hashEntryNo)++;
  if (ctx->hashEntryNo >= ctx->maxBackReference) ctx->hashEntryNo = 0;
}






/* Pass 1 read/write */
#define COMP_WRITE_NUMBER(x,b) \
    while(1) { \
      if ((x) < 128) {*b++ = (x); break;} else *b++ = 128 + ((x) & 127); \
      (x) >>= 7; \
    }

#define COMP_READ_NUMBER(x,d,s) \
  (x) = (*d) & 127; s = 7; \
  while (((*d) & 128) != 0) {d++; (x) |= ((*d) & 127) << s; s += 7;} \
  d++;

#define COMP_WRITE_LITERALS \
  if (literals != 0) { \
    /*int litlength = literals;*/ \
    b = CompWriteNumberAndCount((literals-1) << 1, literals-1, b, litLength); \
    literals = 0; \
    while (anchor < i) {*b++ = *(in + anchor++);} \
  }

/* Pass 2 read/write */
#define COMP_WRITE_NUMBERB(x,len,h) \
  do { \
    h = (x) & ((1<<len)-1); if (h != (x)) h |= (1<<len); \
    bitA |= h << bitFill; bitFill += (len+1); \
    if (bitFill >= 32) { \
      *bits++ = fastlz_extern_long(bitA); bitA = h >> (len + 33 - bitFill); bitFill -= 32; \
    } \
    (x) >>= len; \
  } while ((h & (1<<len)) != 0);

#define COMP_WRITE_BYTEB(x) \
  bitA |= x << bitFill; bitFill += 8; \
  if (bitFill >= 32) {*bits++ = fastlz_extern_long(bitA); bitA = x >> (40 - bitFill); bitFill -= 32;}


/* Misc */
#define COMP_ADD_CODE_LENGTH(len, num, h, accu) \
  h=num; \
  while (1) { \
    accu[len] += len+1; if (h < (1<<len)) break; else h >>= len; \
  }


/*
 *  Write a number and update the lengths array
 *  x is the number to write, y is the number to count (different for lit, ref).
 */
static unsigned char *CompWriteNumberAndCount(int x, int y, unsigned char *b, unsigned int *lengths)
{
  int j, h;

  for (j=1; j<MAXIMUM_BITS; j++)
  {
    COMP_ADD_CODE_LENGTH(j, y, h, lengths);
  }
  COMP_WRITE_NUMBER(x, b);

  return b;
}


int fastlz_compress_init(fastlz_compress_context_t *ctx)
{
  ctx->fastlz_alloc = malloc;
  ctx->fastlz_free = free;
  ctx->hashList = NULL;
  ctx->hashListSize = 0;
  ctx->backreference = MAXIMUM_BACKREFERENCE;
  ctx->maxBackReference = ctx->backreference;
  ctx->hashFillLevel = 0;
  ctx->flags = 0;
  return 0;
}

void fastlz_compress_free(fastlz_compress_context_t *ctx)
{
  if (ctx->hashList != NULL)
  {
    ctx->fastlz_free(ctx->hashList);
    ctx->hashList = NULL;
  }
  ctx->hashListSize = 0;
  ctx->hashFillLevel = 0;
}


void *fastlz_compress_block(fastlz_compress_context_t *ctx, const void *src, unsigned int srcSize, unsigned int *outSize)
{
  int hashCode;
  int followdepth, maxfollowdepth;
  int i, j, k;
  unsigned char *b, *optr;
  const unsigned char *ref;
  int literals, repeats, anchor;
  int maxrepeats, maxrepos;
  int lastOutput;
  unsigned int litLength[MAXIMUM_BITS], refLength[MAXIMUM_BITS], countLength[MAXIMUM_BITS];
  int optLit, optRef, optCount;
  unsigned int *bits, bitA, bitFill;
  const unsigned char *in;
  unsigned char *out;

  in = (const unsigned char*)src;
  if (srcSize == 0)
  {
    *outSize = 0; return NULL;
  }
  memset(ctx->hashValid, 0, MAXIMUM_BACKREFERENCE/8);
  if (ctx->backreference > MAXIMUM_BACKREFERENCE) ctx->backreference = MAXIMUM_BACKREFERENCE;
#ifdef COMPRESS_DEBUG
  printf("backreference = %d\n", ctx->backreference);
#endif
  if (ctx->hashListSize < ctx->backreference)
  {
    if (ctx->hashList != NULL) ctx->fastlz_free(ctx->hashList);
    if ((ctx->hashList = (hash_entry_t*)(ctx->fastlz_alloc(ctx->backreference * sizeof(hash_entry_t)))) == NULL)
    {
      fprintf(stderr, "Out of memory! Aborting!\n");
      return NULL;
    }
    ctx->hashListSize = ctx->backreference;
  }
  ctx->maxBackReference = ctx->backreference;
#if 0
  for (i=0; i<ctx->hashListSize; i++)
  {
    ctx->hashList[i].off = 0;	/* Make sure PREV_MODE_HEAD is off */
    ctx->hashList[i].prev = (unsigned short)i;
    ctx->hashList[i].next = (unsigned short)i;
  }
#endif
  ctx->hashEntryNo = 0;
  ctx->hashFillLevel = 0;
  anchor = 0; maxrepos = 0; ref = NULL;

  /* Now we're ready to rumble ... */
  /* Claim enough memory for really bad cases */
  *outSize = (9*srcSize)/8;
  /* Very small blocks are tricky... */
  if (*outSize - srcSize < 1024) *outSize = srcSize + 1024;
  if ((out = (unsigned char*)(ctx->fastlz_alloc(*outSize))) == NULL)
  {
    fprintf(stderr, "Out of memory! Aborting!\n");
    return NULL;
  }

  /* Init the arrays for counting optimum code lengths */
  for (i=1; i<MAXIMUM_BITS; i++)
  {
    litLength[i] = 0; refLength[i] = 0; countLength[i] = 0;
  }

  /*
   *  Pass 1: deflate into literal count / repeat sequences
   *  Counters are coded by a sequence of bytes; top bit set means the next byte
   *  holds the next 7 bits.
   *  Literals: 2*(literals-1), <data>
   *  Repeats: 2*(back-reference-1), (repeat-count - MINIMUM_REPEATS)
   */
  lastOutput = 0;
  maxfollowdepth = ctx->backreference >> 6;
  if (maxfollowdepth < 16) maxfollowdepth = 16;
  i = 0; b = (unsigned char*)out; literals = 0;
  while (i < srcSize)
  {
    if ((i - lastOutput >= 4096) && ((ctx->flags & COMPRESS_FLAG_EMIT) != 0))
    {
      printf("\r%3d%% / %3d%%", (100*i)/srcSize, (100*(b - out)) / i);
      lastOutput = i;
      /*printf("\nmaxfollow: %d, average: %d\n", maxfollowdepth, (follownum==0) ? 0 : followsum/follownum); maxfollowdepth=0; follownum=0; followsum=0;*/
    }
    if (literals == 0)
    {
      anchor = i;
    }
    if (srcSize - i >= MINIMUM_REPEATS)
    {
      maxrepeats = 0;
      hashCode = HASH_ENCODING(in[i], in[i+1], in[i+2]);
      /* Does a hash list exist? */
      if ((ctx->hashValid[hashCode>>3] & (1<<(hashCode & 7))) != 0)
      {
        followdepth=0;
	j = (int)(ctx->hashCodes[hashCode]);
	do
	{
          int repeatpos;

          followdepth++;
          if (followdepth > maxfollowdepth) break;
	  ref = in + (ctx->hashList[j].off & ~PREV_MODE_HEAD); k = i;
	  while ((k < srcSize) && (in[k] == *ref))
	  {
	    k++; ref++;
	  }
	  repeats = (k - i);
	  repeatpos = (ctx->hashList[j].off & ~PREV_MODE_HEAD);
	  if (repeats > maxrepeats)
	  {
	    /* Increment the minimum number of repeats for long offsets */
	    if ((i - repeatpos < 4096) || (repeats >= MINIMUM_REPEATS+1))
	    {
	      maxrepeats = repeats; maxrepos = repeatpos;
	    }
	  }
	  if ((int)(ctx->hashList[j].next) != j)
	    j = (int)(ctx->hashList[j].next);
	  else
	    break;
	}
	while (k != srcSize);
      }

      if (maxrepeats >= MINIMUM_REPEATS)
      {
        /*printf("lit: %d, repeat: %d %d\n", literals, i-maxrepos, maxrepeats);*/
        /* flush out literals */
        if (b >= out + (*outSize))
        {
          fprintf(stderr, "Fatal error, buffer overflow (1)!\n");
          *outSize = 0;
          return NULL;
        }
        COMP_WRITE_LITERALS;
        j = (i - maxrepos - 1);
        b = CompWriteNumberAndCount((j << 1) + 1, j, b, refLength);
        j = (maxrepeats - MINIMUM_REPEATS);
        b = CompWriteNumberAndCount(j, j, b, countLength);

	/* Update hash table with all repeated codes */
	k = i + maxrepeats; if (k > (srcSize - 3)) k = srcSize-3;
	for (j=i; j<k; j++)
	{
	  CompUpdateHashTable(ctx, in, j);
	}

	i += maxrepeats;
      }
      else
      {
        CompUpdateHashTable(ctx, in, i);
        literals++;
        i++;
      }
    }
    else
    {
      literals += srcSize - i;
      i = srcSize;
      break;
    }
  }
  COMP_WRITE_LITERALS;

  optr = out + (*outSize);
  *outSize = (b - out);
  optr -= *outSize;
  /* Copy to end of buffer to free as much as possible at the beginning */
  memmove(optr, out, *outSize);

  optLit = 1; optRef = 1; optCount = 1;
  literals = litLength[optLit]; anchor = refLength[optRef]; repeats = countLength[optCount];
  for (i=2; i<MAXIMUM_BITS; i++)
  {
    if (litLength[i] < literals) {literals = litLength[i]; optLit = i;}
    if (refLength[i] < anchor) {anchor = refLength[i]; optRef = i;}
    if (countLength[i] < repeats) {countLength[i] = repeats; optCount = i;}
  }

#ifdef COMPRESS_DEBUG
  k = (litLength[7] + refLength[7] + countLength[7]) - (litLength[optLit] + refLength[optRef] + countLength[optCount]);
  printf("Optimum code length lit %d, ref %d, count %d (saves %d bits = %d bytes)\n", optLit, optRef, optCount, k, (k+7)>>3);
#endif

  /* Now deflate the bytestream into a bitstream */
  bits = (unsigned int*)(out+4); b = optr; optr = b + (*outSize);
  bitA = optLit | (optRef << 5) | (optCount << 10); bitFill = 15;
  while (b < optr)
  {
    if ((unsigned char*)bits >= b)
    {
      fprintf(stderr, "Fatal error: buffer overflow (2)!\n");
      *outSize = 0;
      return NULL;
    }
    /*printf("%p -- %p\n", bits, b);*/
    COMP_READ_NUMBER(literals, b, k);
    /* Command bit: 0 ==> literal, 1 ==> ref/count */
    if ((literals & 1) != 0)
    {
      bitA |= (1<<bitFill);
    }
    bitFill++; if (bitFill == 32) {*bits++ = fastlz_extern_long(bitA); bitA = 0; bitFill = 0;}
    if ((literals & 1) == 0)
    {
      literals >>= 1; i = literals;
      COMP_WRITE_NUMBERB(i, optLit, j);
      while (literals >= 0)
      {
        /* The AND is actually necessary on some platforms! */
        i = *b++;
        COMP_WRITE_BYTEB(i);
        literals--;
      }
    }
    else
    {
      literals >>= 1;
      COMP_READ_NUMBER(repeats, b, k);
      COMP_WRITE_NUMBERB(literals, optRef, i);
      COMP_WRITE_NUMBERB(repeats, optCount, i);
    }
  }
  if (bitFill != 0) *bits = fastlz_extern_long(bitA);
  *outSize = (((unsigned char*)bits) - out) + ((bitFill + 7) >> 3);
  /*printf("\n%d / %d : %d\n", ((unsigned char*)bits) - out, bitFill, *outSize);*/

  if ((ctx->flags & COMPRESS_FLAG_EMIT) != 0)
    printf("\r%d%%          \n", (100*(*outSize)) / srcSize);

  bits = (unsigned int *)out;
  if (*outSize >= srcSize+4)
  {
    memcpy(out + 4, in, srcSize); *bits = fastlz_extern_long(-srcSize);
    *outSize = (srcSize + 4);
  }
  else
  {
    *bits = fastlz_extern_long((*outSize)-4);
  }
  return out;
}
