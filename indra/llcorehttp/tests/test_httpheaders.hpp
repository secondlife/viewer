/** 
 * @file test_httpheaders.hpp
 * @brief unit tests for the LLCore::HttpHeaders class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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
#ifndef TEST_LLCORE_HTTP_HEADERS_H_
#define TEST_LLCORE_HTTP_HEADERS_H_

#include "httpheaders.h"

#include <iostream>

#include "test_allocator.h"


using namespace LLCoreInt;



namespace tut
{

struct HttpHeadersTestData
{
	// the test objects inherit from this so the member functions and variables
	// can be referenced directly inside of the test functions.
	size_t mMemTotal;
};

typedef test_group<HttpHeadersTestData> HttpHeadersTestGroupType;
typedef HttpHeadersTestGroupType::object HttpHeadersTestObjectType;
HttpHeadersTestGroupType HttpHeadersTestGroup("HttpHeaders Tests");

template <> template <>
void HttpHeadersTestObjectType::test<1>()
{
	set_test_name("HttpHeaders construction");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	HttpHeaders * headers = new HttpHeaders();
	ensure("One ref on construction of HttpHeaders", headers->getRefCount() == 1);
	ensure("Memory being used", mMemTotal < GetMemTotal());
	ensure("Nothing in headers", 0 == headers->mHeaders.size());

	// release the implicit reference, causing the object to be released
	headers->release();

	// make sure we didn't leak any memory
	ensure(mMemTotal == GetMemTotal());
}

template <> template <>
void HttpHeadersTestObjectType::test<2>()
{
	set_test_name("HttpHeaders construction");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	HttpHeaders * headers = new HttpHeaders();
	
	{
		// Append a few strings
		std::string str1("Pragma:");
		headers->mHeaders.push_back(str1);
		std::string str2("Accept: application/json");
		headers->mHeaders.push_back(str2);
	
		ensure("Headers retained", 2 == headers->mHeaders.size());
		ensure("First is first", headers->mHeaders[0] == str1);
		ensure("Second is second", headers->mHeaders[1] == str2);
	}
	
	// release the implicit reference, causing the object to be released
	headers->release();

	// make sure we didn't leak any memory
	ensure(mMemTotal == GetMemTotal());
}

}  // end namespace tut


#endif  // TEST_LLCORE_HTTP_HEADERS_H_
