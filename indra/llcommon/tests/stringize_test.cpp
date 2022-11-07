/**
 * @file   stringize_test.cpp
 * @author Nat Goodspeed
 * @date   2008-09-12
 * @brief  Test of stringize.h
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

/*==========================================================================*|
#if LL_WINDOWS
#pragma warning (disable : 4675) // "resolved by ADL" -- just as I want!
#endif
|*==========================================================================*/

// STL headers
#include <iomanip>

// Precompiled header
#include "linden_common.h"

// associated header
#include "../stringize.h"

// std headers
// external library headers
// other Linden headers
#include "../llsd.h"

#include "../test/lltut.h"

namespace tut
{
    struct stringize_data
    {
        stringize_data():
            c('c'),
            s(17),
            i(34),
            l(68),
            f(3.14159265358979f),
            d(3.14159265358979),
            // Including a space differentiates this from
            // boost::lexical_cast<std::string>, which doesn't handle embedded
            // spaces so well.
            abc("abc def")
        {
            llsd["i"]   = i;
            llsd["d"]   = d;
            llsd["abc"] = abc;
            def = L"def ghi";

        }

        char        c;
        short       s;
        int         i;
        long        l;
        float       f;
        double      d;
        std::string abc;
        std::wstring def;
        LLSD        llsd;
    };
    typedef test_group<stringize_data> stringize_group;
    typedef stringize_group::object stringize_object;
    tut::stringize_group strzgrp("stringize_h");

    template<> template<>
    void stringize_object::test<1>()
    {
        ensure_equals(stringize(c),    "c");
        ensure_equals(stringize(s),    "17");
        ensure_equals(stringize(i),    "34");
        ensure_equals(stringize(l),    "68");
        ensure_equals(stringize(f),    "3.14159");
        ensure_equals(stringize(d),    "3.14159");
        ensure_equals(stringize(abc),  "abc def");
        ensure_equals(stringize(def),  "def ghi"); //Will generate LL_WARNS() due to narrowing.
        ensure_equals(stringize(llsd), "{'abc':'abc def','d':r3.14159,'i':i34}");
    }

    template<> template<>
    void stringize_object::test<2>()
    {
        ensure_equals(STRINGIZE("c is " << c), "c is c");
        ensure_equals(STRINGIZE(std::setprecision(4) << d), "3.142");
    }

    template<> template<>
    void stringize_object::test<3>()
    {
        //Tests rely on validity of wstring_to_utf8str()
        ensure_equals(wstring_to_utf8str(wstringize(c)),    wstring_to_utf8str(L"c"));
        ensure_equals(wstring_to_utf8str(wstringize(s)),    wstring_to_utf8str(L"17"));
        ensure_equals(wstring_to_utf8str(wstringize(i)),    wstring_to_utf8str(L"34"));
        ensure_equals(wstring_to_utf8str(wstringize(l)),    wstring_to_utf8str(L"68"));
        ensure_equals(wstring_to_utf8str(wstringize(f)),    wstring_to_utf8str(L"3.14159"));
        ensure_equals(wstring_to_utf8str(wstringize(d)),    wstring_to_utf8str(L"3.14159"));
        ensure_equals(wstring_to_utf8str(wstringize(abc)),  wstring_to_utf8str(L"abc def"));
        ensure_equals(wstring_to_utf8str(wstringize(abc)),  wstring_to_utf8str(wstringize(abc.c_str())));
        ensure_equals(wstring_to_utf8str(wstringize(def)),  wstring_to_utf8str(L"def ghi"));
 //       ensure_equals(wstring_to_utf8str(wstringize(llsd)), wstring_to_utf8str(L"{'abc':'abc def','d':r3.14159,'i':i34}"));
    }
} // namespace tut
