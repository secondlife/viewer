/** 
 * @file lltut.cpp
 * @author Mark Lentczner
 * @date 5/16/06
 * @brief MacTester
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

#include "lltut.h"

#include "lldate.h"
#include "llformat.h"
#include "llsd.h"
#include "lluri.h"
#include "stringize.h"

namespace tut
{
	void ensure_equals(const char* msg, const LLDate& actual,
		const LLDate& expected)
	{
		ensure_equals(msg,
			actual.secondsSinceEpoch(), expected.secondsSinceEpoch());
	}

	void ensure_equals(const char* msg, const LLURI& actual,
		const LLURI& expected)
	{
		ensure_equals(msg,
			actual.asString(), expected.asString());
	}

	void ensure_equals(const char* msg,
		const std::vector<U8>& actual, const std::vector<U8>& expected)
	{
		std::string s(msg);
		
		ensure_equals(s + " size", actual.size(), expected.size());
		
		std::vector<U8>::const_iterator i, j;
		int k;
		for (i = actual.begin(), j = expected.begin(), k = 0;
			i != actual.end();
			++i, ++j, ++k)
		{
			ensure_equals(s + " field", *i, *j);
		}
	}

	void ensure_equals(const char* m, const LLSD& actual,
		const LLSD& expected)
    {
        ensure_equals(std::string(m), actual, expected);
    }

	void ensure_equals(const std::string& msg, const LLSD& actual,
		const LLSD& expected)
	{
		ensure_equals(msg + " type", actual.type(), expected.type());
		switch (actual.type())
		{
			case LLSD::TypeUndefined:
				return;
			
			case LLSD::TypeBoolean:
				ensure_equals(msg + " boolean", actual.asBoolean(), expected.asBoolean());
				return;
			
			case LLSD::TypeInteger:
				ensure_equals(msg + " integer", actual.asInteger(), expected.asInteger());
				return;
			
			case LLSD::TypeReal:
				ensure_equals(msg + " real", actual.asReal(), expected.asReal());
				return;
			
			case LLSD::TypeString:
				ensure_equals(msg + " string", actual.asString(), expected.asString());
				return;
			
			case LLSD::TypeUUID:
				ensure_equals(msg + " uuid", actual.asUUID(), expected.asUUID());
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
				ensure_equals(msg + " map size", actual.size(), expected.size());
				
				LLSD::map_const_iterator actual_iter = actual.beginMap();
				LLSD::map_const_iterator expected_iter = expected.beginMap();
				
				while(actual_iter != actual.endMap())
				{
					ensure_equals(msg + " map keys", 
						actual_iter->first, expected_iter->first);
					ensure_equals(msg + "[" + actual_iter->first + "]",
						actual_iter->second, expected_iter->second);
					++actual_iter;
					++expected_iter;
				}
				return;
			}
			case LLSD::TypeArray:
			{
				ensure_equals(msg + " array size", actual.size(), expected.size());
				
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
				throw failure(STRINGIZE("invalid type field " << actual.type()));
		}
	}

	void ensure_starts_with(const std::string& msg,
		const std::string& actual, const std::string& expectedStart)
	{
		if( actual.find(expectedStart, 0) != 0 )
		{
			std::stringstream ss;
			ss << msg << ": " << "expected to find " << expectedStart
				<< " at start of actual " << actual;
			throw failure(ss.str().c_str());
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
			ss << msg << ": " << "expected to find " << expectedEnd
				<< " at end of actual " << actual;
			throw failure(ss.str().c_str());
		}
	}

	void ensure_contains(const std::string& msg,
		const std::string& actual, const std::string& expectedSubString)
	{
		if( actual.find(expectedSubString, 0) == std::string::npos )
		{
			std::stringstream ss;
			ss << msg << ": " << "expected to find " << expectedSubString
				<< " in actual " << actual;
			throw failure(ss.str().c_str());
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
			throw failure(ss.str().c_str());
		}
	}
}

