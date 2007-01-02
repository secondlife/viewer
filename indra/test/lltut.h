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

	void ensure_contains(const std::string& msg,
		const std::string& actual, const std::string& expectedSubString);
}


#endif // LL_LLTUT_H
