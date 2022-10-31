/** 
 * @file test_httprequestqueue.hpp
 * @brief unit tests for the LLCore::HttpRequestQueue class
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
#ifndef TEST_LLCORE_HTTP_REQUESTQUEUE_H_
#define TEST_LLCORE_HTTP_REQUESTQUEUE_H_

#include "_httprequestqueue.h"

#include <iostream>

#include "_httpoperation.h"


using namespace LLCoreInt;



namespace tut
{

struct HttpRequestqueueTestData
{
    // the test objects inherit from this so the member functions and variables
    // can be referenced directly inside of the test functions.
};

typedef test_group<HttpRequestqueueTestData> HttpRequestqueueTestGroupType;
typedef HttpRequestqueueTestGroupType::object HttpRequestqueueTestObjectType;
HttpRequestqueueTestGroupType HttpRequestqueueTestGroup("HttpRequestqueue Tests");

template <> template <>
void HttpRequestqueueTestObjectType::test<1>()
{
    set_test_name("HttpRequestQueue construction");

    // create a new ref counted object with an implicit reference
    HttpRequestQueue::init();
    
    ensure("One ref on construction of HttpRequestQueue", HttpRequestQueue::instanceOf()->getRefCount() == 1);

    // release the implicit reference, causing the object to be released
    HttpRequestQueue::term();
}

template <> template <>
void HttpRequestqueueTestObjectType::test<2>()
{
    set_test_name("HttpRequestQueue refcount works");

    // create a new ref counted object with an implicit reference
    HttpRequestQueue::init();

    HttpRequestQueue * rq = HttpRequestQueue::instanceOf();
    rq->addRef();
    
    // release the singleton, hold on to the object
    HttpRequestQueue::term();
    
    ensure("One ref after term() called", rq->getRefCount() == 1);

    // Drop ref
    rq->release();
}

template <> template <>
void HttpRequestqueueTestObjectType::test<3>()
{
    set_test_name("HttpRequestQueue addOp/fetchOp work");

    // create a new ref counted object with an implicit reference
    HttpRequestQueue::init();

    HttpRequestQueue * rq = HttpRequestQueue::instanceOf();

    HttpOperation::ptr_t op(new HttpOpNull());

    rq->addOp(op);      // transfer my refcount

    op = rq->fetchOp(true);     // Potentially hangs the test on failure
    ensure("One goes in, one comes out", static_cast<bool>(op));
    op.reset();

    op = rq->fetchOp(false);
    ensure("Better not be two of them", !op);
    
    // release the singleton, hold on to the object
    HttpRequestQueue::term();
}

template <> template <>
void HttpRequestqueueTestObjectType::test<4>()
{
    set_test_name("HttpRequestQueue addOp/fetchAll work");

    // create a new ref counted object with an implicit reference
    HttpRequestQueue::init();

    HttpRequestQueue * rq = HttpRequestQueue::instanceOf();

    HttpOperation::ptr_t op (new HttpOpNull());
    rq->addOp(op);      // transfer my refcount

    op.reset(new HttpOpNull());
    rq->addOp(op);      // transfer my refcount

    op.reset(new HttpOpNull());
    rq->addOp(op);      // transfer my refcount
    
    {
        HttpRequestQueue::OpContainer ops;
        rq->fetchAll(true, ops);        // Potentially hangs the test on failure
        ensure("Three go in, three come out", 3 == ops.size());

        op = rq->fetchOp(false);
        ensure("Better not be any more of them", !op);
        op.reset();

        // release the singleton, hold on to the object
        HttpRequestQueue::term();

        // Release them
        ops.clear();
//      while (! ops.empty())
//      {
//          HttpOperation * op = ops.front();
//          ops.erase(ops.begin());
//          op->release();
//      }
    }
}

}  // end namespace tut


#endif  // TEST_LLCORE_HTTP_REQUESTQUEUE_H_
