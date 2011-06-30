/*
 *  fastlzintl.h - shared internal code
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

#ifndef _FAST_LZ_COMPRESSION_INTL_H_
#define _FAST_LZ_COMPRESSION_INTL_H_


#define MINIMUM_REPEATS		3
#define MAXIMUM_BITS		20


extern unsigned long fastlz_extern_long(unsigned int x);

#endif
