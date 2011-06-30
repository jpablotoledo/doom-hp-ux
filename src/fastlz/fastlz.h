/*
 *  fastlz.h - compression/decompression code
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

#ifndef _FAST_LZ_COMPRESSION_H_
#define _FAST_LZ_COMPRESSION_H_

#include <stdio.h>


/* Maximum backwards reference (do not increase!) */
#define MAXIMUM_BACKREFERENCE	0x10000

/* Compress flags */
#define COMPRESS_FLAG_EMIT		1


struct hash_entry_s;

typedef struct fastlz_compress_context_s {
  void *(*fastlz_alloc)(unsigned int);
  void (*fastlz_free)(void *);
  struct hash_entry_s *hashList;
  int hashListSize;
  int backreference;
  int maxBackReference;
  int hashFillLevel;
  int flags;
  unsigned int hashEntryNo;
  unsigned short hashCodes[MAXIMUM_BACKREFERENCE];
  unsigned char hashValid[MAXIMUM_BACKREFERENCE/8];
} fastlz_compress_context_t;


typedef struct fastlz_decompress_context_s {
  void *(*fastlz_alloc)(unsigned int);
  void (*fastlz_free)(void *);
  unsigned int (*fastlz_refill)(struct fastlz_decompress_context_s *, void *, unsigned int);
  void *ioBuffer;
  int ioBuffSize;
} fastlz_decompress_context_t;

typedef struct fastlz_decompfile_context_s {
  fastlz_decompress_context_t basectx;
  FILE *file;
} fastlz_decompfile_context_t;

extern unsigned int fastlz_file_refill(fastlz_decompress_context_t *, void *, unsigned int);


extern int  fastlz_compress_init(fastlz_compress_context_t *ctx);
extern void fastlz_compress_free(fastlz_compress_context_t *ctx);
extern void *fastlz_compress_block(fastlz_compress_context_t *ctx, const void *src, unsigned int srcSize, unsigned int *outSize);

extern int  fastlz_decompress_init(fastlz_decompress_context_t *ctx);
extern void fastlz_decompress_free(fastlz_decompress_context_t *ctx);
extern int  fastlz_decompress_block(fastlz_decompress_context_t *ctx, void *dest, unsigned int destSize);

#endif
