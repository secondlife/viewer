/**
 * @file lltut.h
 * @author Phoenix
 * @date 2005-09-26
 * @brief Helper assertion functions for tests, now backed by doctest.
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
#include <cstring>
#include <sstream>
#include <string>

// ---------------------------------------------------------------------------
// doctest
// ---------------------------------------------------------------------------
// turn off clang warnings for the doctest header itself
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "doctest/doctest.h"

#if __clang__
#pragma clang diagnostic pop
#endif

// ---------------------------------------------------------------------------
// Forward declarations for LL types used in ensure_equals overloads
// ---------------------------------------------------------------------------
class LLDate;
class LLSD;
class LLURI;

// ---------------------------------------------------------------------------
// tut::failure – kept so that old helper code compiled together with test
// files can still throw / catch it.  doctest's exception translator below
// converts it into a test failure.
// ---------------------------------------------------------------------------
namespace tut
{
    // Source directory set by the test runner (--sourcedir flag).
    // Declared here; defined in test.cpp.
    extern std::string sSourceDir;

    /**
     * Exception class used by the legacy tut-style assertion helpers.
     * When thrown inside a doctest test case it is caught by the registered
     * exception translator and reported as a test failure.
     */
    class failure : public std::exception
    {
    public:
        explicit failure(const std::string& msg) : mMsg(msg) {}
        const char* what() const noexcept override { return mMsg.c_str(); }
    private:
        std::string mMsg;
    };

    /**
     * Exception class used to skip a tut test (maps to doctest skip).
     */
    class skip_exception : public failure
    {
    public:
        explicit skip_exception(const std::string& msg = std::string()) : failure(msg) {}
    };

} // namespace tut

// ---------------------------------------------------------------------------
// Register tut::failure as a known exception type with doctest so that tests
// which still throw it get a useful failure message instead of "unknown exception".
// ---------------------------------------------------------------------------
// (registration happens in test.cpp via REGISTER_EXCEPTION_TRANSLATOR)

// ---------------------------------------------------------------------------
// Assertion helpers – these functions throw tut::failure on error so that
// existing test code requires zero changes.  doctest catches the exception
// via its translator and marks the test as failed.
// ---------------------------------------------------------------------------

// ---- basic boolean ensure ----
inline void ensure(const std::string& msg, bool condition)
{
    if (!condition)
        throw tut::failure(msg);
}

inline void ensure(const char* msg, bool condition)
{
    ensure(msg ? std::string(msg) : std::string(), condition);
}

inline void ensure(bool condition)
{
    ensure(std::string(), condition);
}

// ---- ensure_equals (generic template) ----
namespace tut
{
    template<class T, class Q>
    void ensure_equals(const std::string& msg, const Q& actual, const T& expected)
    {
        if (!(actual == expected))
        {
            std::ostringstream ss;
            ss << msg << ": not equal";
            throw failure(ss.str());
        }
    }

    template<class T, class Q>
    void ensure_equals(const Q& actual, const T& expected)
    {
        ensure_equals(std::string(), actual, expected);
    }

    // ---- ensure_not_equals ----
    template<class T, class Q>
    void ensure_not_equals(const char* msg, const Q& actual, const T& expected)
    {
        if (actual == expected)
        {
            std::ostringstream ss;
            ss << (msg ? msg : "") << (msg ? ": " : "") << "both equal " << expected;
            throw failure(ss.str());
        }
    }

    template<class T, class Q>
    void ensure_not_equals(const Q& actual, const T& expected)
    {
        ensure_not_equals(nullptr, actual, expected);
    }

    // ---- LLDate, LLURI, LLSD::Binary, LLSD overloads (implemented in lltut.cpp) ----
    void ensure_equals(const std::string& msg,
        const LLDate& actual, const LLDate& expected);

    void ensure_equals(const std::string& msg,
        const LLURI& actual, const LLURI& expected);

    // std::vector<U8> is the current definition of LLSD::Binary.
    void ensure_equals(const std::string& msg,
        const std::vector<U8>& actual, const std::vector<U8>& expected);

    void ensure_equals(const std::string& msg,
        const LLSD& actual, const LLSD& expected);

    // ---- string content helpers ----
    void ensure_starts_with(const std::string& msg,
        const std::string& actual, const std::string& expectedStart);

    void ensure_ends_with(const std::string& msg,
        const std::string& actual, const std::string& expectedEnd);

    void ensure_contains(const std::string& msg,
        const std::string& actual, const std::string& expectedSubString);

    void ensure_does_not_contain(const std::string& msg,
        const std::string& actual, const std::string& expectedSubString);

    // ---- floating-point helpers ----
    inline void ensure_approximately_equals(const char* msg, F64 actual, F64 expected, U32 frac_bits)
    {
        if (!is_approx_equal_fraction(actual, expected, frac_bits))
        {
            std::ostringstream ss;
            ss << (msg ? msg : "") << (msg ? ": " : "")
               << "not equal actual: " << actual << " expected: " << expected;
            throw failure(ss.str());
        }
    }

    inline void ensure_approximately_equals(const char* msg, F32 actual, F32 expected, U32 frac_bits)
    {
        if (!is_approx_equal_fraction(actual, expected, frac_bits))
        {
            std::ostringstream ss;
            ss << (msg ? msg : "") << (msg ? ": " : "")
               << "not equal actual: " << actual << " expected: " << expected;
            throw failure(ss.str());
        }
    }

    inline void ensure_approximately_equals(F32 actual, F32 expected, U32 frac_bits)
    {
        ensure_approximately_equals(nullptr, actual, expected, frac_bits);
    }

    inline void ensure_approximately_equals_range(const char* msg, F32 actual, F32 expected, F32 delta)
    {
        if (std::fabs(actual - expected) > delta)
        {
            std::ostringstream ss;
            ss << (msg ? msg : "") << (msg ? ": " : "")
               << "not equal actual: " << actual << " expected: " << expected
               << " tolerance: " << delta;
            throw failure(ss.str());
        }
    }

    inline void ensure_memory_matches(const char* msg,
        const void* actual, U32 actual_len,
        const void* expected, U32 expected_len)
    {
        if ((expected_len != actual_len) ||
            (std::memcmp(actual, expected, actual_len) != 0))
        {
            std::ostringstream ss;
            ss << (msg ? msg : "") << (msg ? ": " : "") << "not equal";
            throw failure(ss.str());
        }
    }

    inline void ensure_memory_matches(
        const void* actual, U32 actual_len,
        const void* expected, U32 expected_len)
    {
        ensure_memory_matches("", actual, actual_len, expected, expected_len);
    }

} // namespace tut

// ---------------------------------------------------------------------------
// Pull the most-commonly-used names into the global namespace so that test
// files that use them unqualified (without "tut::") continue to compile.
// ---------------------------------------------------------------------------
using tut::ensure_equals;
using tut::ensure_not_equals;
using tut::ensure_approximately_equals;
using tut::ensure_approximately_equals_range;
using tut::ensure_memory_matches;
using tut::ensure_starts_with;
using tut::ensure_ends_with;
using tut::ensure_contains;
using tut::ensure_does_not_contain;

#endif // LL_LLTUT_H
