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

#include <cassert>
#include <cfloat>
#include <climits>
#include <limits>
#include <type_traits>

typedef signed char             S8;
typedef unsigned char           U8;
typedef signed short            S16;
typedef unsigned short          U16;
typedef signed int              S32;
typedef unsigned int            U32;

// to express an index that might go negative
// (ssize_t is provided by SOME compilers, don't collide)
typedef typename std::make_signed<std::size_t>::type llssize;

#if LL_WINDOWS
// https://docs.microsoft.com/en-us/cpp/build/reference/zc-wchar-t-wchar-t-is-native-type
// https://docs.microsoft.com/en-us/cpp/cpp/fundamental-types-cpp
// Windows wchar_t is 16-bit, whichever way /Zc:wchar_t is set. In effect,
// Windows wchar_t is always a typedef, either for unsigned short or __wchar_t.
// (__wchar_t, available either way, is Microsoft's native 2-byte wchar_t type.)
// The version of clang available with VS 2019 also defines wchar_t as __wchar_t
// which is also 16 bits.
// In any case, llwchar should be a UTF-32 type.
typedef U32                 llwchar;
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
typedef U32             TPACKETID;

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

/*****************************************************************************
*   Narrowing
*****************************************************************************/
/**
 * narrow() is used to cast a wider type to a narrower type with validation.
 *
 * In many cases we take the size() of a container and try to pass it to an
 * S32 or a U32 parameter. We used to be able to assume that the size of
 * anything we could fit into memory could be expressed as a 32-bit int. With
 * 64-bit viewers, though, size_t as returned by size() and length() and so
 * forth is 64 bits, and the compiler is unhappy about stuffing such values
 * into 32-bit types.
 *
 * It works to force the compiler to truncate, e.g. static_cast<S32>(len) or
 * S32(len) or (S32)len, but we can do better.
 *
 * For:
 * @code
 * std::vector<Object> container;
 * void somefunc(S32 size);
 * @endcode
 * call:
 * @code
 * somefunc(narrow(container.size()));
 * @endcode
 *
 * narrow() truncates but, in RelWithDebInfo builds, it validates (using
 * assert()) that the passed value can validly be expressed by the destination
 * type.
 */
// narrow_holder is a struct that accepts the passed value as its original
// type and provides templated conversion functions to other types.
template <typename FROM>
class narrow
{
private:
    FROM mValue;

public:
    constexpr narrow(FROM value): mValue(value) {}

    /*---------------------- Narrowing unsigned to signed ----------------------*/
    template <typename TO,
              typename std::enable_if<std::is_unsigned<FROM>::value &&
                                      std::is_signed<TO>::value,
                                      bool>::type = true>
    constexpr
    operator TO() const
    {
        // The reason we skip the
        // assert(value >= std::numeric_limits<TO>::lowest());
        // like the overload below is that to perform the above comparison,
        // the compiler promotes the signed lowest() to the unsigned FROM
        // type, making it hugely positive -- so a reasonable 'value' will
        // always fail the assert().
        assert(mValue <= std::numeric_limits<TO>::max());
        return static_cast<TO>(mValue);
    }

    /*----------------------- Narrowing all other cases ------------------------*/
    template <typename TO,
              typename std::enable_if<! (std::is_unsigned<FROM>::value &&
                                         std::is_signed<TO>::value),
                                      bool>::type = true>
    constexpr
    operator TO() const
    {
        // two different assert()s so we can tell which condition failed
        assert(mValue <= std::numeric_limits<TO>::max());
        // Funny, with floating point types min() is "positive epsilon" rather
        // than "largest negative" -- that's lowest().
        assert(mValue >= std::numeric_limits<TO>::lowest());
        // Do we really expect to use this with floating point types?
        // If so, does it matter if a very small value truncates to zero?
        //assert(fabs(mValue) >= std::numeric_limits<TO>::min());
        return static_cast<TO>(mValue);
    }
};

#endif
