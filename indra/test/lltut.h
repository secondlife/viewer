/**
 * @file lltut.h
 * @author Phoenix
 * @date 2005-09-26
 * @brief helper tut methods
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLTUT_H
#define LL_LLTUT_H

#include "is_approx_equal_fraction.h" // instead of llmath.h
#include "stringize.h"
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

class LLDate;
class LLSD;
class LLURI;

namespace tut
{
    void ensure_equals(const std::string& msg,
        const LLDate& actual, const LLDate& expected);

    void ensure_equals(const std::string& msg,
        const LLURI& actual, const LLURI& expected);

    // std::vector<U8> is the current definition of LLSD::Binary. Because
    // we're only forward-declaring LLSD in this header file, we can't
    // directly reference that nested type. If the build complains that
    // there's no definition for this declaration, it could be that
    // LLSD::Binary has changed, and that this declaration must be adjusted to
    // match.
    void ensure_equals(const std::string& msg,
        const std::vector<U8>& actual, const std::vector<U8>& expected);

    void ensure_equals(const std::string& msg,
        const LLSD& actual, const LLSD& expected);

    void ensure_starts_with(const std::string& msg,
        const std::string& actual, const std::string& expectedStart);

    void ensure_ends_with(const std::string& msg,
        const std::string& actual, const std::string& expectedEnd);

    void ensure_contains(const std::string& msg,
        const std::string& actual, const std::string& expectedSubString);

    void ensure_does_not_contain(const std::string& msg,
        const std::string& actual, const std::string& expectedSubString);
}

// This is an odd place to #include an important contributor -- but the usual
// rules are reversed here. Instead of the overloads above referencing tut.hpp
// features, we need calls in tut.hpp template functions to dispatch to our
// overloads declared above.

// turn off warnings about unused functions from clang for tut package
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#include <tut/tut.hpp>
#if __clang__
#pragma clang diagnostic pop
#endif

// The functions BELOW this point actually consume tut.hpp functionality.
namespace tut
{
    template <typename F>           // replace with C++20 floating-point concept
    inline void ensure_approximately_equals(std::string_view msg, F actual, F expected, U32 frac_bits)
    {
        if(!is_approx_equal_fraction(actual, expected, frac_bits))
        {
            throw tut::failure(stringize(msg, (msg.empty()?"":": "), "not equal actual: ",
                                         actual, " expected: ", expected));
        }
    }

    template <typename F>           // replace with C++20 floating-point concept
    inline void ensure_approximately_equals(F actual, F expected, U32 frac_bits)
    {
        ensure_approximately_equals("", actual, expected, frac_bits);
    }

    template <typename F>           // replace with C++20 floating-point concept
    inline void ensure_approximately_equals_range(std::string_view msg, F actual, F expected, F delta)
    {
        if (fabs(actual-expected)>delta)
        {
            throw tut::failure(stringize(msg, (msg.empty()?"":": "), "not equal actual: ",
                                         actual, " expected: ", expected, " tolerance: ", delta));
        }
    }

    inline void ensure_memory_matches(std::string_view msg,const void* actual, U32 actual_len, const void* expected,U32 expected_len)
    {
        if((expected_len != actual_len) ||
            (std::memcmp(actual, expected, actual_len) != 0))
        {
            throw tut::failure(stringize(msg, (msg.empty()?"":": "), "not equal"));
        }
    }

    inline void ensure_memory_matches(const void* actual, U32 actual_len, const void* expected,U32 expected_len)
    {
        ensure_memory_matches("", actual, actual_len, expected, expected_len);
    }

    template <class T,class Q>
    void ensure_not_equals(std::string_view msg,const Q& actual,const T& expected)
    {
        if( expected == actual )
        {
            throw tut::failure(stringize(msg, (msg.empty()?"":": "), "both equal ", expected));
        }
    }

    template <class T,class Q>
    void ensure_not_equals(const Q& actual,const T& expected)
    {
        ensure_not_equals("", actual, expected);
    }
}

#endif // LL_LLTUT_H
