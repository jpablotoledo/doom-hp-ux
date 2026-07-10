/*
 * opl2_stdint_hpux.h
 * <stdint.h> replacement for HP-UX B.11.00 (no stdint.h shipped, but
 * uint8_t/uint16_t/uint32_t/uint64_t/uint_fast32_t are all already
 * provided by <sys/_inttypes.h>, pulled in transitively by <inttypes.h>).
 * Requires compiling with -Ae (not -Aa) so "long long" is available for
 * the uint64_t/int64_t typedefs inside that system header.
 */
#ifndef _OPL2_STDINT_HPUX_H_
#define _OPL2_STDINT_HPUX_H_

#include <inttypes.h>

#endif
