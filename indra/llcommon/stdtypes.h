/** 
 * @file stdtypes.h
 * @brief Basic type declarations for cross platform compatibility.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#ifndef LL_STDTYPES_H
#define LL_STDTYPES_H

#include <cfloat>
#include <climits>

typedef signed char         S8;
typedef unsigned char           U8;
typedef signed short            S16;
typedef unsigned short          U16;
typedef signed int          S32;
typedef unsigned int            U32;

#if LL_WINDOWS
// https://docs.microsoft.com/en-us/cpp/build/reference/zc-wchar-t-wchar-t-is-native-type
// https://docs.microsoft.com/en-us/cpp/cpp/fundamental-types-cpp
// Windows wchar_t is 16-bit, whichever way /Zc:wchar_t is set. In effect,
// Windows wchar_t is always a typedef, either for unsigned short or __wchar_t.
// (__wchar_t, available either way, is Microsoft's native 2-byte wchar_t type.)
// The version of clang available with VS 2019 also defines wchar_t as __wchar_t
// which is also 16 bits.
// In any case, llwchar should be a UTF-32 type.
typedef U32             llwchar;
#else
typedef wchar_t             llwchar;
// What we'd actually want is a simple module-scope 'if constexpr' to test
// std::is_same<wchar_t, llwchar>::value and use that to define, or not
// define, string conversion specializations. Since we don't have that, we'll
// have to rely on #if instead. Sorry, Dr. Stroustrup.
#define LLWCHAR_IS_WCHAR_T 1
#endif

#if LL_WINDOWS
typedef signed __int64          S64;
// probably should be 'hyper' or similiar
#define S64L(a)                 (a)
typedef unsigned __int64        U64;
#define U64L(a)                 (a)
#else
typedef long long int           S64;
typedef long long unsigned int      U64;
#if LL_DARWIN || LL_LINUX
#define S64L(a)             (a##LL)
#define U64L(a)             (a##ULL)
#endif
#endif

typedef float               F32;
typedef double              F64;

typedef S32             BOOL;
typedef U8              KEY;
typedef U32             MASK;
typedef U32                     TPACKETID;

// Use #define instead of consts to avoid conversion headaches
#define S8_MAX      (SCHAR_MAX)
#define U8_MAX      (UCHAR_MAX)
#define S16_MAX     (SHRT_MAX)
#define U16_MAX     (USHRT_MAX)
#define S32_MAX     (INT_MAX)
#define U32_MAX     (UINT_MAX)
#define F32_MAX     (FLT_MAX)
#define F64_MAX     (DBL_MAX)

#define S8_MIN      (SCHAR_MIN)
#define U8_MIN      (0)
#define S16_MIN     (SHRT_MIN)
#define U16_MIN     (0)
#define S32_MIN     (INT_MIN)
#define U32_MIN     (0)
#define F32_MIN     (FLT_MIN)
#define F64_MIN     (DBL_MIN)


#ifndef TRUE
#define TRUE            (1)
#endif

#ifndef FALSE
#define FALSE           (0)
#endif

#ifndef NULL
#define NULL            (0)
#endif

typedef U8 LLPCode;

#define LL_ARRAY_SIZE( _kArray ) ( sizeof( (_kArray) ) / sizeof( _kArray[0] ) )

#if LL_LINUX && __GNUC__ <= 2
typedef int intptr_t;
#endif

#endif
