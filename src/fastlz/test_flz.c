/*
 *  test_flz.c - compression test code
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


#include <stdio.h>
#include <stdlib.h>

#include "fastlz.h"


int main(int argc, char *argv[])
{
  fastlz_compress_context_t cctx;
  fastlz_decompfile_context_t dctx;
  unsigned char *inbuffer, *outbuffer, *decbuffer;
  unsigned int dataSize, compSize;
  char filename[256] = "test_flz";
  const char compname[] = "compdata";
  int backreference = 16384;
  FILE *fp;
  int i;

  fastlz_compress_init(&cctx);
  fastlz_decompress_init(&dctx.basectx);

  i=1;
  while (i<argc)
  {
    if (argv[i][0] == '-')
    {
      if ((argv[i][1] >= '1') && (argv[i][1] <= '9'))
      {
        backreference = 1 << (argv[i][1] - '0' + 7);
      }
      else
        fprintf(stderr, "Bad switch -%c\n", argv[i][1]);
    }
    else
      strcpy(filename, argv[i]);

    i++;
  }

  fp = fopen(filename, "rb");
  if (fp == NULL)
    return -1;
  fseek(fp, 0, SEEK_END);
  dataSize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if ((inbuffer = (unsigned char*)malloc(dataSize)) == NULL)
    return -1;
  fread(inbuffer, 1, dataSize, fp);
  fclose(fp);

  cctx.backreference = backreference;
  cctx.flags = COMPRESS_FLAG_EMIT;

  if ((outbuffer = fastlz_compress_block(&cctx, inbuffer, dataSize, &compSize)) == NULL)
    return -1;
  printf("Compressed size %d\n", compSize);

  fp = fopen(compname, "wb");
  if (fp != NULL)
  {
    fwrite(outbuffer, 1, compSize, fp);
    fclose(fp);
  }

  if ((decbuffer = (unsigned char*)malloc(dataSize)) == NULL)
    return -1;

  dctx.basectx.fastlz_refill = fastlz_file_refill;
  dctx.basectx.ioBuffSize = 32768;
  /*dctx.basectx.ioBuffer = dctx.basectx.fastlz_alloc(dctx.basectx.ioBuffSize);*/
  dctx.file = fopen(compname, "rb");

  fastlz_decompress_block(&dctx.basectx, decbuffer, dataSize);
  fclose(dctx.file);

  printf("Compare decompressed: %d\n", memcmp(decbuffer, inbuffer, dataSize));

  free(inbuffer);
  cctx.fastlz_free(outbuffer);
  free(decbuffer);

  fastlz_compress_free(&cctx);
  fastlz_decompress_free(&dctx.basectx);

  return 0;
}
