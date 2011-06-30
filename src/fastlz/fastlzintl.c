/*
 *  fastlzintl.c - compression library shared code
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

unsigned long fastlz_extern_long(unsigned long x)
{
  unsigned long res;
  unsigned char *rptr = (unsigned char*)&res;

  rptr[0] = x & 0xff;
  rptr[1] = (x >> 8) & 0xff;
  rptr[2] = (x >> 16) & 0xff;
  rptr[3] = (x >> 24) & 0xff;

  return res;
}
