/** 
 * @file lltut.cpp
 * @author Mark Lentczner
 * @date 5/16/06
 * @brief MacTester
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "lltut.h"

#include "llformat.h"
#include "llsd.h"

namespace tut
{
	template<>
	void ensure_equals(const char* msg, const LLDate& actual,
		const LLDate& expected)
	{
		ensure_equals(msg,
			actual.secondsSinceEpoch(), expected.secondsSinceEpoch());
	}

	template<>
	void ensure_equals(const char* msg, const LLURI& actual,
		const LLURI& expected)
	{
		ensure_equals(msg,
			actual.asString(), expected.asString());
	}

	template<>
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

	template<>
	void ensure_equals(const char* m, const LLSD& actual,
		const LLSD& expected)
	{
		const std::string& msg = m;
		
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
