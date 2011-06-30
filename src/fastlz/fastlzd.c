/*
 *  fastlzd.c - decompression code
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

#include "fastlz.h"
#include "fastlzintl.h"


int fastlz_decompress_init(fastlz_decompress_context_t *ctx)
{
  ctx->fastlz_alloc = malloc;
  ctx->fastlz_free = free;
  ctx->fastlz_refill = NULL;
  ctx->ioBuffer = NULL;
  ctx->ioBuffSize = 0x10000;
  return 0;
}

void fastlz_decompress_free(fastlz_decompress_context_t *ctx)
{
  if (ctx->ioBuffer != NULL)
  {
    ctx->fastlz_free(ctx->ioBuffer);
    ctx->ioBuffer = NULL;
  }
}

unsigned int fastlz_file_refill(fastlz_decompress_context_t *ctx, void *dest, unsigned int size)
{
  fastlz_decompfile_context_t *fctx = (fastlz_decompfile_context_t*)ctx;
  return fread(dest, 1, size, fctx->file);
}


#ifndef DECOMPRESS_ASM

/* Different code for standalone / armdeu code */
#ifdef COMPRESSION_STANDALONE

#define COMP_READ_NUMBERB(x,len,h,s) \
  x = 0; s = 0; \
  do { \
    h = bitA; \
    if (bitFill < len+1) { \
      bitA = fastlz_extern_long(*bits++); h |= (bitA << bitFill); bitFill += 31 - len; bitA >>= 32 - bitFill; \
    } else {bitA >>= len+1; bitFill -= len+1;} \
    x |= (h & ((1<<len)-1)) << s; s += len; \
  } while ((h & (1<<len)) != 0);

#define COMP_READ_BYTEB(x) \
  x = bitA; \
  if (bitFill < 8) { \
    bitA = fastlz_extern_long(*bits++); x |= (bitA << bitFill); bitFill += 24; bitA >>= 32 - bitFill; \
  } else {bitA >>= 8; bitFill -= 8;}

#else

#define COMP_READ_NUMBERB(x,len,h,s) \
  x = 0; s = 0; \
  do { \
    h = bitA; \
    if (bitFill < len+1) { \
      if (bits>=bitsTop) if (UncompressRefill00(ctx, &compSize,&bits,&bitsTop)==0) return -1; \
      bitA = fastlz_extern_long(*bits++); h |= (bitA << bitFill); bitFill += 31 - len; bitA >>= 32 - bitFill; \
    } else {bitA >>= len+1; bitFill -= len+1;} \
    x |= (h & ((1<<len)-1)) << s; s += len; \
  } while ((h & (1<<len)) != 0);


#define COMP_READ_BYTEB(x) \
  x = bitA; \
  if (bitFill < 8) { \
    if (bits>=bitsTop) if (UncompressRefill00(ctx, &compSize,&bits,&bitsTop)==0) return -1;\
    bitA = fastlz_extern_long(*bits++); x |= (bitA << bitFill); bitFill += 24; bitA >>= 32 - bitFill; \
  } else {bitA >>= 8; bitFill -= 8;}

#endif


static int UncompressRefill00(fastlz_decompress_context_t *ctx, unsigned int *compSize, unsigned int **bits, unsigned int **bitsTop)
{
  unsigned int h;

  if (*compSize <= 0) return 0;
  if (*compSize > ctx->ioBuffSize) h = ctx->ioBuffSize; else h = *compSize;
  if (ctx->fastlz_refill(ctx, ctx->ioBuffer, h) != h) h=0;
  *bits = (unsigned int*)(ctx->ioBuffer);
  *bitsTop = (unsigned int*)(((char*)ctx->ioBuffer) + h);
  *compSize -= h;
  return h;
}


int fastlz_decompress_block(fastlz_decompress_context_t *ctx, void *dest, unsigned int destSize)
{
  unsigned int compSize;
  int blockSize;

  if (ctx->ioBuffer == NULL)
  {
    if ((ctx->ioBuffer = ctx->fastlz_alloc(ctx->ioBuffSize)) == NULL)
      return -1;
  }
  /* The compressed size is stored in the first word, so read just that now */
  if (ctx->fastlz_refill(ctx, &blockSize, 4) != 4)
    return -1;
  blockSize = fastlz_extern_long(blockSize);

  if (blockSize < 0)
  {
    compSize = (unsigned int)(-blockSize);
    /*printf("Uncompressed block %d (%d)\n", compSize, blockSize);*/
    return (ctx->fastlz_refill(ctx, dest, compSize) == compSize) ? 0 : -1;
  }
  else
  {
    unsigned char *b, *upper, *ref;
    unsigned int *bits, bitA, bitFill;
    int optLit, optRef, optCount;
    int h, s;
    int literals, repeats;
    unsigned int *bitsTop;

    compSize = (unsigned int)blockSize;
    /* Now that we have the size, we can correctly fill the buffer */
    if (UncompressRefill00(ctx, &compSize, &bits, &bitsTop) == 0) return -1;
    bitA = fastlz_extern_long(*bits++);
    optLit = bitA & 31; optRef = (bitA >> 5) & 31; optCount = (bitA >> 10) & 31;
    bitA >>= 15; bitFill = 32-15;
#ifdef COMPRESS_DEBUG
    printf("%d, %d, %d, %d\n", optLit, optRef, optCount, outSize);
#endif
    b = dest; upper = b + destSize;
    while (b < upper)
    {
      if (bitFill == 0)
      {
        if (bits >= bitsTop)
          if (UncompressRefill00(ctx, &compSize, &bits, &bitsTop) == 0) return -1;
        bitA = fastlz_extern_long(*bits++); bitFill = 32;
      }
      h = (bitA & 1); bitA >>= 1; bitFill--;
      if (h == 0)
      {
        COMP_READ_NUMBERB(literals, optLit, h, s);
        literals++;
        /*printf("lit %d\n", literals);*/
        while (literals > 0) {COMP_READ_BYTEB(h); *b++ = h; literals--;}
      }
      else
      {
        COMP_READ_NUMBERB(literals, optRef, h, s);
        COMP_READ_NUMBERB(repeats, optCount, h, s);
        ref = b - (literals + 1); repeats += MINIMUM_REPEATS;
        /*printf("ref %d, count %d\n", literals+1, repeats);*/
        while (repeats > 0) {*b++ = *ref++; repeats--;}
      }
    }
  }
  return 0;
}

#endif	/* DECOMPRESS_ASM */
