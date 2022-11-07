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
    };

    typedef test_group<HttpOperationTestData> HttpOperationTestGroupType;
    typedef HttpOperationTestGroupType::object HttpOperationTestObjectType;
    HttpOperationTestGroupType HttpOperationTestGroup("HttpOperation Tests");

    template <> template <>
    void HttpOperationTestObjectType::test<1>()
    {
        set_test_name("HttpOpNull construction");

        // create a new ref counted object with an implicit reference
        HttpOperation::ptr_t op (new HttpOpNull());
        ensure(op.use_count() == 1);

        // release the implicit reference, causing the object to be released
        op.reset();
    }

    template <> template <>
    void HttpOperationTestObjectType::test<2>()
    {
        set_test_name("HttpOpNull construction with handlers");

        // Get some handlers
        LLCore::HttpHandler::ptr_t h1 (new TestHandler());
        
        // create a new ref counted object with an implicit reference
        HttpOperation::ptr_t op (new HttpOpNull());

        // Add the handlers
        op->setReplyPath(LLCore::HttpOperation::HttpReplyQueuePtr_t(), h1);

        // Check ref count
        ensure(op.unique() == 1);

        // release the reference, releasing the operation but
        // not the handlers.
        op.reset();

        // release the handlers
        h1.reset();
    }

}

#endif  // TEST_LLCORE_HTTP_OPERATION_H_
