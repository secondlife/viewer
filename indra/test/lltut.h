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

#include <tut/tut.hpp>
#include <cstring>

class LLDate;
class LLSD;
class LLURI;

namespace tut
{
	inline void ensure_approximately_equals(const char* msg, F64 actual, F64 expected, U32 frac_bits)
	{
		if(!is_approx_equal_fraction(actual, expected, frac_bits))
		{
			std::stringstream ss;
			ss << (msg?msg:"") << (msg?": ":"") << "not equal actual: " << actual << " expected: " << expected;
			throw tut::failure(ss.str().c_str());
		}
	}

	inline void ensure_approximately_equals(const char* msg, F32 actual, F32 expected, U32 frac_bits)
	{
		if(!is_approx_equal_fraction(actual, expected, frac_bits))
		{
			std::stringstream ss;
			ss << (msg?msg:"") << (msg?": ":"") << "not equal actual: " << actual << " expected: " << expected;
			throw tut::failure(ss.str().c_str());
		}
	}

	inline void ensure_approximately_equals(F32 actual, F32 expected, U32 frac_bits)
	{
		ensure_approximately_equals(NULL, actual, expected, frac_bits);
	}

	inline void ensure_memory_matches(const char* msg,const void* actual, U32 actual_len, const void* expected,U32 expected_len)
	{
		if((expected_len != actual_len) || 
			(std::memcmp(actual, expected, actual_len) != 0))
		{
			std::stringstream ss;
			ss << (msg?msg:"") << (msg?": ":"") << "not equal";
			throw tut::failure(ss.str().c_str());
		}
	}

	inline void ensure_memory_matches(const void* actual, U32 actual_len, const void* expected,U32 expected_len)
	{
		ensure_memory_matches(NULL, actual, actual_len, expected, expected_len);
	}

	template <class T,class Q>
	void ensure_not_equals(const char* msg,const Q& actual,const T& expected)
	{
		if( expected == actual )
		{
			std::stringstream ss;
			ss << (msg?msg:"") << (msg?": ":"") << "both equal " << expected;
			throw tut::failure(ss.str().c_str());
		}
	}

	template <class T,class Q>
	void ensure_not_equals(const Q& actual,const T& expected)
	{
		ensure_not_equals(NULL, actual, expected);
	}
	
	
	template <class T,class Q>
	void ensure_equals(const std::string& msg,
		const Q& actual,const T& expected)
		{ ensure_equals(msg.c_str(), actual, expected); }

	void ensure_equals(const char* msg,
		const LLDate& actual, const LLDate& expected);

	void ensure_equals(const char* msg,
		const LLURI& actual, const LLURI& expected);
		
	void ensure_equals(const char* msg,
		const std::vector<U8>& actual, const std::vector<U8>& expected);

	void ensure_equals(const char* msg,
		const LLSD& actual, const LLSD& expected);

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


#endif // LL_LLTUT_H
