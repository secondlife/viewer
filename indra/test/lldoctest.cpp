/**
 * @file lldoctest.cpp
 * @author Converted from lltut.cpp for doctest migration
 * @date 2025-08-01
 * @brief Doctest helper implementations
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "linden_common.h"

#include "lldoctest.h"

#include "lldate.h"
#include "llformat.h"
#include "llsd.h"
#include "lluri.h"
#include "stringize.h"

namespace doctest_ll
{
    void ensure_equals(const std::string& msg, const LLDate& actual,
        const LLDate& expected)
    {
        CHECK_MESSAGE(actual.secondsSinceEpoch() == expected.secondsSinceEpoch(), msg);
    }

    void ensure_equals(const std::string& msg, const LLURI& actual,
        const LLURI& expected)
    {
        CHECK_MESSAGE(actual.asString() == expected.asString(), msg);
    }

    void ensure_equals(const std::string& msg,
        const std::vector<U8>& actual, const std::vector<U8>& expected)
    {
        CHECK_MESSAGE(actual.size() == expected.size(), msg + " size");

        std::vector<U8>::const_iterator i, j;
        int k;
        for (i = actual.begin(), j = expected.begin(), k = 0;
            i != actual.end();
            ++i, ++j, ++k)
        {
            CHECK_MESSAGE(*i == *j, msg + " field");
        }
    }

    void ensure_equals(const std::string& msg, const LLSD& actual,
        const LLSD& expected)
    {
        CHECK_MESSAGE(actual.type() == expected.type(), msg + " type");
        switch (actual.type())
        {
            case LLSD::TypeUndefined:
                return;

            case LLSD::TypeBoolean:
                CHECK_MESSAGE(actual.asBoolean() == expected.asBoolean(), msg + " boolean");
                return;

            case LLSD::TypeInteger:
                CHECK_MESSAGE(actual.asInteger() == expected.asInteger(), msg + " integer");
                return;

            case LLSD::TypeReal:
                CHECK_MESSAGE(actual.asReal() == expected.asReal(), msg + " real");
                return;

            case LLSD::TypeString:
                CHECK_MESSAGE(actual.asString() == expected.asString(), msg + " string");
                return;

            case LLSD::TypeUUID:
                CHECK_MESSAGE(actual.asUUID() == expected.asUUID(), msg + " uuid");
                return;

            case LLSD::TypeDate:
                ensure_equals(msg + " date", actual.asDate(), expected.asDate());
                return;

            case LLSD::TypeURI:
                ensure_equals(msg + " uri", actual.asURI(), expected.asURI());
                return;

            case LLSD::TypeBinary:
                ensure_equals(msg + " binary", actual.asBinary(), expected.asBinary());
                return;

            case LLSD::TypeMap:
            {
                CHECK_MESSAGE(actual.size() == expected.size(), msg + " map size");

                LLSD::map_const_iterator actual_iter = actual.beginMap();
                LLSD::map_const_iterator expected_iter = expected.beginMap();

                while(actual_iter != actual.endMap())
                {
                    CHECK_MESSAGE(actual_iter->first == expected_iter->first, msg + " map keys");
                    ensure_equals(msg + "[" + actual_iter->first + "]",
                        actual_iter->second, expected_iter->second);
                    ++actual_iter;
                    ++expected_iter;
                }
                return;
            }
            case LLSD::TypeArray:
            {
                CHECK_MESSAGE(actual.size() == expected.size(), msg + " array size");

                for(int i = 0; i < actual.size(); ++i)
                {
                    ensure_equals(msg + llformat("[%d]", i),
                        actual[i], expected[i]);
                }
                return;
            }
            default:
                // should never get here, but compiler produces warning if we
                // don't cover this case, and at Linden warnings are fatal.
                FAIL(STRINGIZE("invalid type field " << actual.type()));
        }
    }

    void ensure_starts_with(const std::string& msg,
        const std::string& actual, const std::string& expectedStart)
    {
        if( actual.find(expectedStart, 0) != 0 )
        {
            std::stringstream ss;
            ss << msg << ": " << "expected to find '" << expectedStart
               << "' at start of actual '" << actual << "'";
            FAIL(ss.str().c_str());
        }
    }

    void ensure_ends_with(const std::string& msg,
        const std::string& actual, const std::string& expectedEnd)
    {
        if( actual.size() < expectedEnd.size()
            || actual.rfind(expectedEnd)
                != (actual.size() - expectedEnd.size()) )
        {
            std::stringstream ss;
            ss << msg << ": " << "expected to find '" << expectedEnd
               << "' at end of actual '" << actual << "'";
            FAIL(ss.str().c_str());
        }
    }

    void ensure_contains(const std::string& msg,
        const std::string& actual, const std::string& expectedSubString)
    {
        if( actual.find(expectedSubString, 0) == std::string::npos )
        {
            std::stringstream ss;
            ss << msg << ": " << "expected to find '" << expectedSubString
               << "' in actual '" << actual << "'";
            FAIL(ss.str().c_str());
        }
    }

    void ensure_does_not_contain(const std::string& msg,
        const std::string& actual, const std::string& expectedSubString)
    {
        if( actual.find(expectedSubString, 0) != std::string::npos )
        {
            std::stringstream ss;
            ss << msg << ": " << "expected not to find " << expectedSubString
                << " in actual " << actual;
            FAIL(ss.str().c_str());
        }
    }
}

