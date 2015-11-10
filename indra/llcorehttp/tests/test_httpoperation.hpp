/** 
 * @file test_httpoperation.hpp
 * @brief unit tests for the LLCore::HttpOperation-derived classes
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
#ifndef TEST_LLCORE_HTTP_OPERATION_H_
#define TEST_LLCORE_HTTP_OPERATION_H_

#include "_httpoperation.h"
#include "httphandler.h"

#include <iostream>

#include "test_allocator.h"


using namespace LLCoreInt;


namespace
{

class TestHandler : public LLCore::HttpHandler
{
public:
	virtual void onCompleted(HttpHandle, HttpResponse *)
		{
			std::cout << "TestHandler::onCompleted() invoked" << std::endl;
		}
	
};


}  // end namespace anonymous


namespace tut
{
	struct HttpOperationTestData
	{
		// the test objects inherit from this so the member functions and variables
		// can be referenced directly inside of the test functions.
		size_t mMemTotal;
	};

	typedef test_group<HttpOperationTestData> HttpOperationTestGroupType;
	typedef HttpOperationTestGroupType::object HttpOperationTestObjectType;
	HttpOperationTestGroupType HttpOperationTestGroup("HttpOperation Tests");

	template <> template <>
	void HttpOperationTestObjectType::test<1>()
	{
		set_test_name("HttpOpNull construction");

		// record the total amount of dynamically allocated memory
		mMemTotal = GetMemTotal();

		// create a new ref counted object with an implicit reference
		HttpOpNull * op = new HttpOpNull();
		ensure(op->getRefCount() == 1);
		ensure(mMemTotal < GetMemTotal());
		
		// release the implicit reference, causing the object to be released
		op->release();

		// make sure we didn't leak any memory
		ensure(mMemTotal == GetMemTotal());
	}

	template <> template <>
	void HttpOperationTestObjectType::test<2>()
	{
		set_test_name("HttpOpNull construction with handlers");

		// record the total amount of dynamically allocated memory
		mMemTotal = GetMemTotal();

		// Get some handlers
		TestHandler * h1 = new TestHandler();
		
		// create a new ref counted object with an implicit reference
		HttpOpNull * op = new HttpOpNull();

		// Add the handlers
		op->setReplyPath(NULL, h1);

		// Check ref count
		ensure(op->getRefCount() == 1);

		// release the reference, releasing the operation but
		// not the handlers.
		op->release();
		op = NULL;
		ensure(mMemTotal != GetMemTotal());
		
		// release the handlers
		delete h1;
		h1 = NULL;

		ensure(mMemTotal == GetMemTotal());
	}

}

#endif  // TEST_LLCORE_HTTP_OPERATION_H_
