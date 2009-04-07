/** 
 * @file lltut.h
 * @author Phoenix
 * @date 2005-09-26
 * @brief helper tut methods
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
			(memcmp(actual, expected, actual_len) != 0))
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
