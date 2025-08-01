/**
 * @file lldoctest.h
 * @author Converted from lltut.h for doctest migration
 * @date 2025-08-01
 * @brief helper doctest methods
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

#ifndef LL_LLDOCTEST_H
#define LL_LLDOCTEST_H

#include "is_approx_equal_fraction.h" // instead of llmath.h
#include <cstring>

// Include doctest framework
#include "doctest.h"

class LLDate;
class LLSD;
class LLURI;

// Doctest helper functions for LL-specific types
namespace doctest_ll
{
    void ensure_equals(const std::string& msg,
        const LLDate& actual, const LLDate& expected);

    void ensure_equals(const std::string& msg,
        const LLURI& actual, const LLURI& expected);

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

    inline void ensure_approximately_equals(const char* msg, F64 actual, F64 expected, U32 frac_bits)
    {
        if(!is_approx_equal_fraction(actual, expected, frac_bits))
        {
            std::stringstream ss;
            ss << (msg?msg:"") << (msg?": ":"") << "not equal actual: " << actual << " expected: " << expected;
            FAIL(ss.str().c_str());
        }
    }

    inline void ensure_approximately_equals(const char* msg, F32 actual, F32 expected, U32 frac_bits)
    {
        if(!is_approx_equal_fraction(actual, expected, frac_bits))
        {
            std::stringstream ss;
            ss << (msg?msg:"") << (msg?": ":"") << "not equal actual: " << actual << " expected: " << expected;
            FAIL(ss.str().c_str());
        }
    }

    inline void ensure_approximately_equals(F32 actual, F32 expected, U32 frac_bits)
    {
        ensure_approximately_equals(NULL, actual, expected, frac_bits);
    }

    inline void ensure_approximately_equals_range(const char *msg, F32 actual, F32 expected, F32 delta)
    {
        if (fabs(actual-expected)>delta)
        {
            std::stringstream ss;
            ss << (msg?msg:"") << (msg?": ":"") << "not equal actual: " << actual << " expected: " << expected << " tolerance: " << delta;
            FAIL(ss.str().c_str());
        }
    }

    inline void ensure_memory_matches(const char* msg,const void* actual, U32 actual_len, const void* expected,U32 expected_len)
    {
        if((expected_len != actual_len) ||
            (std::memcmp(actual, expected, actual_len) != 0))
        {
            std::stringstream ss;
            ss << (msg?msg:"") << (msg?": ":"") << "not equal";
            FAIL(ss.str().c_str());
        }
    }

    inline void ensure_memory_matches(const void* actual, U32 actual_len, const void* expected,U32 expected_len)
    {
        ensure_memory_matches("", actual, actual_len, expected, expected_len);
    }

    template <class T,class Q>
    void ensure_not_equals(const char* msg,const Q& actual,const T& expected)
    {
        if( expected == actual )
        {
            std::stringstream ss;
            ss << (msg?msg:"") << (msg?": ":"") << "both equal " << expected;
            FAIL(ss.str().c_str());
        }
    }

    template <class T,class Q>
    void ensure_not_equals(const Q& actual,const T& expected)
    {
        ensure_not_equals(NULL, actual, expected);
    }
}

// Compatibility macros to ease migration from tut to doctest
#define ensure_equals(msg, actual, expected) CHECK_MESSAGE(actual == expected, msg)
#define ensure(msg, condition) CHECK_MESSAGE(condition, msg)
#define fail(msg) FAIL(msg)

// For LL-specific types, use the helper functions
#define ensure_equals_ll(msg, actual, expected) doctest_ll::ensure_equals(msg, actual, expected)
#define ensure_approximately_equals_ll(msg, actual, expected, frac_bits) doctest_ll::ensure_approximately_equals(msg, actual, expected, frac_bits)
#define ensure_approximately_equals_range_ll(msg, actual, expected, delta) doctest_ll::ensure_approximately_equals_range(msg, actual, expected, delta)
#define ensure_memory_matches_ll(msg, actual, actual_len, expected, expected_len) doctest_ll::ensure_memory_matches(msg, actual, actual_len, expected, expected_len)
#define ensure_not_equals_ll(msg, actual, expected) doctest_ll::ensure_not_equals(msg, actual, expected)
#define ensure_starts_with_ll(msg, actual, expected) doctest_ll::ensure_starts_with(msg, actual, expected)
#define ensure_ends_with_ll(msg, actual, expected) doctest_ll::ensure_ends_with(msg, actual, expected)
#define ensure_contains_ll(msg, actual, expected) doctest_ll::ensure_contains(msg, actual, expected)
#define ensure_does_not_contain_ll(msg, actual, expected) doctest_ll::ensure_does_not_contain(msg, actual, expected)

#endif // LL_LLDOCTEST_H

