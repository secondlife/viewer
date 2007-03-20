/** 
 * @file lltut.h
 * @author Phoenix
 * @date 2005-09-26
 * @brief helper tut methods
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/** 
 *
 * THOROUGH_DESCRIPTION
 *
 */

#ifndef LL_LLTUT_H
#define LL_LLTUT_H

#include <tut/tut.h>

#include "lldate.h"
#include "lluri.h"

class LLSD;

namespace tut
{
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

	template<>
	void ensure_equals(const char* msg,
		const LLDate& actual, const LLDate& expected);

	template<>
	void ensure_equals(const char* msg,
		const LLURI& actual, const LLURI& expected);
		
	template<>
	void ensure_equals(const char* msg,
		const std::vector<U8>& actual, const std::vector<U8>& expected);

	template<>
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
