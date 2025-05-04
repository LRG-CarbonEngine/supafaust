/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* math_ops.h:
**  Copyright (C) 2007-2021 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
** Some ideas from:
**  blargg
**  http://graphics.stanford.edu/~seander/bithacks.html
*/

#ifndef __MDFN_MATH_OPS_H
#define __MDFN_MATH_OPS_H

#if defined(_MSC_VER)
 #include <intrin.h>
#endif

namespace Mednafen
{

static INLINE unsigned MDFN_lzcount32_0UD(uint32 v)
{
 #if defined(__GNUC__) || defined(__clang__) || defined(__ICC) || defined(__INTEL_COMPILER)
 return __builtin_clz(v);
 #elif defined(_MSC_VER)
 unsigned long idx;

 _BitScanReverse(&idx, v);

 return 31 ^ idx;
 #else
 unsigned ret = 0;
 unsigned tmp;

 tmp = !(v & 0xFFFF0000) << 4; v <<= tmp; ret += tmp;
 tmp = !(v & 0xFF000000) << 3; v <<= tmp; ret += tmp;
 tmp = !(v & 0xF0000000) << 2; v <<= tmp; ret += tmp;
 tmp = !(v & 0xC0000000) << 1; v <<= tmp; ret += tmp;
 tmp = !(v & 0x80000000) << 0;            ret += tmp;

 return(ret);
 #endif
}

static INLINE unsigned MDFN_lzcount64_0UD(uint64 v)
{
 #if defined(__GNUC__) || defined(__clang__) || defined(__ICC) || defined(__INTEL_COMPILER)
 return __builtin_clzll(v);
 #elif defined(_MSC_VER)
  #if defined(_WIN64)
   unsigned long idx;
   _BitScanReverse64(&idx, v);
   return 63 ^ idx;
  #else
   unsigned long idx0;
   unsigned long idx1;

   _BitScanReverse(&idx1, v >> 0);
   idx1 -= 32;
   if(!_BitScanReverse(&idx0, v >> 32))
    idx0 = idx1;

   idx0 += 32;

   return 63 ^ idx0;
  #endif
 #else
 unsigned ret = 0;
 unsigned tmp;

 tmp = !(v & 0xFFFFFFFF00000000ULL) << 5; v <<= tmp; ret += tmp;
 tmp = !(v & 0xFFFF000000000000ULL) << 4; v <<= tmp; ret += tmp;
 tmp = !(v & 0xFF00000000000000ULL) << 3; v <<= tmp; ret += tmp;
 tmp = !(v & 0xF000000000000000ULL) << 2; v <<= tmp; ret += tmp;
 tmp = !(v & 0xC000000000000000ULL) << 1; v <<= tmp; ret += tmp;
 tmp = !(v & 0x8000000000000000ULL) << 0;            ret += tmp;

 return(ret);
 #endif
}

//
// Result is defined for all possible inputs(including 0).
//
static INLINE unsigned MDFN_log2(uint32 v) { return 31 ^ MDFN_lzcount32_0UD(v | 1); }
static INLINE unsigned MDFN_log2(uint64 v) { return 63 ^ MDFN_lzcount64_0UD(v | 1); }

static INLINE unsigned MDFN_log2(int32 v) { return MDFN_log2((uint32)v); }
static INLINE unsigned MDFN_log2(int64 v) { return MDFN_log2((uint64)v); }

//
//
static INLINE int64 MDFN_abs64(int64 v) { return (uint64)(v ^ (v >> 63)) + ((uint64)v >> 63); }

// Rounds up to the nearest power of 2(treats input as unsigned to a degree, but be aware of integer promotion rules).
// Returns 0 on overflow.
static INLINE uint64 round_up_pow2(uint32 v) { uint64 tmp = (uint64)1 << MDFN_log2(v); return tmp << (tmp < v); }
static INLINE uint64 round_up_pow2(uint64 v) { uint64 tmp = (uint64)1 << MDFN_log2(v); return tmp << (tmp < v); }

static INLINE uint64 round_up_pow2(int32 v) { return round_up_pow2((uint32)v); }
static INLINE uint64 round_up_pow2(int64 v) { return round_up_pow2((uint64)v); }

// Some compilers' optimizers and some platforms might fubar the generated code from these macros,
// so some tests are run in...tests.cpp
#define sign_x_to_s16(n, v) ((int16)((uint32)(v) << (16 - (n))) >> (16 - (n)))
#define sign_8_to_s16(v)  ((int16)(int8)(v))
#define sign_9_to_s16(v)  sign_x_to_s16(9, (v))
#define sign_10_to_s16(v) sign_x_to_s16(10, (v))
#define sign_11_to_s16(v) sign_x_to_s16(11, (v))
#define sign_12_to_s16(v) sign_x_to_s16(12, (v))
#define sign_13_to_s16(v) sign_x_to_s16(13, (v))
#define sign_14_to_s16(v) sign_x_to_s16(14, (v))
#define sign_15_to_s16(v) sign_x_to_s16(15, (v))

// This obviously won't convert higher-than-32 bit numbers to signed 32-bit ;)
// Also, this shouldn't be used for 8-bit and 16-bit signed numbers, since you can
// convert those faster with typecasts...
#define sign_x_to_s32(_bits, _value) (((int32)((uint32)(_value) << (32 - _bits))) >> (32 - _bits))

}
#endif
