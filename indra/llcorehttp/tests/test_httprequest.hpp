/** 
 * @file test_httprequest.hpp
 * @brief unit tests for the LLCore::HttpRequest class
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
#ifndef TEST_LLCORE_HTTP_REQUEST_H_
#define TEST_LLCORE_HTTP_REQUEST_H_

#include "httprequest.h"
#include "httphandler.h"
#include "httpresponse.h"
#include "_httpservice.h"
#include "_httprequestqueue.h"

#include <curl/curl.h>

#include "test_allocator.h"
#include "llcorehttp_test.h"


using namespace LLCoreInt;


namespace
{

#if defined(WIN32)

void usleep(unsigned long usec);

#endif

}

namespace tut
{

struct HttpRequestTestData
{
	// the test objects inherit from this so the member functions and variables
	// can be referenced directly inside of the test functions.
	size_t			mMemTotal;
	int				mHandlerCalls;
	HttpStatus		mStatus;
};

class TestHandler2 : public LLCore::HttpHandler
{
public:
	TestHandler2(HttpRequestTestData * state,
				 const std::string & name)
		: mState(state),
		  mName(name),
		  mExpectHandle(LLCORE_HTTP_HANDLE_INVALID)
		{}
	
	virtual void onCompleted(HttpHandle handle, HttpResponse * response)
		{
			if (LLCORE_HTTP_HANDLE_INVALID != mExpectHandle)
			{
				ensure("Expected handle received in handler", mExpectHandle == handle);
			}
			ensure("Handler got a response", NULL != response);
			if (response && mState)
			{
				const HttpStatus actual_status(response->getStatus());
				
				ensure("Expected HttpStatus received in response", actual_status == mState->mStatus);
			}
			if (mState)
			{
				mState->mHandlerCalls++;
			}
			// std::cout << "TestHandler2::onCompleted() invoked" << std::endl;
		}

	HttpRequestTestData * mState;
	std::string mName;
	HttpHandle mExpectHandle;
};

typedef test_group<HttpRequestTestData> HttpRequestTestGroupType;
typedef HttpRequestTestGroupType::object HttpRequestTestObjectType;
HttpRequestTestGroupType HttpRequestTestGroup("HttpRequest Tests");

template <> template <>
void HttpRequestTestObjectType::test<1>()
{
	ScopedCurlInit ready;
	
	set_test_name("HttpRequest construction");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// Get singletons created
	HttpRequest::createService();
		
	// create a new ref counted object with an implicit reference
	HttpRequest * req = new HttpRequest();
	ensure(mMemTotal < GetMemTotal());
		
	// release the request object
	delete req;
	req = NULL;

	HttpRequest::destroyService();

	// make sure we didn't leak any memory
	ensure(mMemTotal == GetMemTotal());
}

template <> template <>
void HttpRequestTestObjectType::test<2>()
{
	ScopedCurlInit ready;

	set_test_name("HttpRequest and Null Op queued");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// Get singletons created
	HttpRequest::createService();
		
	// create a new ref counted object with an implicit reference
	HttpRequest * req = new HttpRequest();
	ensure(mMemTotal < GetMemTotal());

	// Issue a NoOp
	HttpHandle handle = req->requestNoOp(NULL);
	ensure(handle != LLCORE_HTTP_HANDLE_INVALID);
		
	// release the request object
	delete req;
	req = NULL;

	// We're still holding onto the operation which is
	// sitting, unserviced, on the request queue so...
	ensure(mMemTotal < GetMemTotal());

	// Request queue should have two references:  global singleton & service object
	ensure("Two references to request queue", 2 == HttpRequestQueue::instanceOf()->getRefCount());

	// Okay, tear it down
	HttpRequest::destroyService();
	// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
	ensure(mMemTotal == GetMemTotal());
}


template <> template <>
void HttpRequestTestObjectType::test<3>()
{
	ScopedCurlInit ready;

	set_test_name("HttpRequest NoOp + Stop execution");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
		
	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;
		
	// Get singletons created
	HttpRequest::createService();
		
	// Start threading early so that thread memory is invariant
	// over the test.
	HttpRequest::startThread();

	// create a new ref counted object with an implicit reference
	HttpRequest * req = new HttpRequest();
	ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

	// Issue a NoOp
	HttpHandle handle = req->requestNoOp(&handler);
	ensure("Valid handle returned for first request", handle != LLCORE_HTTP_HANDLE_INVALID);

	// Run the notification pump.
	int count(0);
	int limit(20);
	while (count++ < limit && mHandlerCalls < 1)
	{
		req->update(1000);
		usleep(100000);
	}
	ensure("Request executed in reasonable time", count < limit);
	ensure("One handler invocation for request", mHandlerCalls == 1);

	// Okay, request a shutdown of the servicing thread
	handle = req->requestStopThread(&handler);
	ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
	// Run the notification pump again
	count = 0;
	limit = 100;
	while (count++ < limit && mHandlerCalls < 2)
	{
		req->update(1000);
		usleep(100000);
	}
	ensure("Second request executed in reasonable time", count < limit);
	ensure("Second handler invocation", mHandlerCalls == 2);

	// See that we actually shutdown the thread
	count = 0;
	limit = 10;
	while (count++ < limit && ! HttpService::isStopped())
	{
		usleep(100000);
	}
	ensure("Thread actually stopped running", HttpService::isStopped());
	
	// release the request object
	delete req;
	req = NULL;

	// Shut down service
	HttpRequest::destroyService();
	
	// We have left over state so can't really say something
	// definitive about memory usage at the end of this.
	ensure("Two handler calls on the way out", 2 == mHandlerCalls);
	// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
	ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
}

template <> template <>
void HttpRequestTestObjectType::test<4>()
{
	ScopedCurlInit ready;

	set_test_name("2 HttpRequest instances, one thread");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	TestHandler2 handler1(this, "handler1");
	TestHandler2 handler2(this, "handler2");
		
	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;
		
	// Get singletons created
	HttpRequest::createService();
		
	// Start threading early so that thread memory is invariant
	// over the test.
	HttpRequest::startThread();

	// create a new ref counted object with an implicit reference
	HttpRequest * req1 = new HttpRequest();
	HttpRequest * req2 = new HttpRequest();
	ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

	// Issue some NoOps
	HttpHandle handle = req1->requestNoOp(&handler1);
	ensure("Valid handle returned for first request", handle != LLCORE_HTTP_HANDLE_INVALID);
	handler1.mExpectHandle = handle;

	handle = req2->requestNoOp(&handler2);
	ensure("Valid handle returned for first request", handle != LLCORE_HTTP_HANDLE_INVALID);
	handler2.mExpectHandle = handle;

	// Run the notification pump.
	int count(0);
	int limit(20);
	while (count++ < limit && mHandlerCalls < 2)
	{
		req1->update(1000);
		req2->update(1000);
		usleep(100000);
	}
	ensure("Request executed in reasonable time", count < limit);
	ensure("One handler invocation for request", mHandlerCalls == 2);

	// Okay, request a shutdown of the servicing thread
	handle = req2->requestStopThread(&handler2);
	ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	handler2.mExpectHandle = handle;
	
	// Run the notification pump again
	count = 0;
	limit = 100;
	while (count++ < limit && mHandlerCalls < 3)
	{
		req1->update(1000);
		req2->update(1000);
		usleep(100000);
	}
	ensure("Second request executed in reasonable time", count < limit);
	ensure("Second handler invocation", mHandlerCalls == 3);

	// See that we actually shutdown the thread
	count = 0;
	limit = 10;
	while (count++ < limit && ! HttpService::isStopped())
	{
		usleep(100000);
	}
	ensure("Thread actually stopped running", HttpService::isStopped());
	
	// release the request object
	delete req1;
	req1 = NULL;
	delete req2;
	req2 = NULL;

	// Shut down service
	HttpRequest::destroyService();
	
	// We have left over state so can't really say something
	// definitive about memory usage at the end of this.
	ensure("Two handler calls on the way out", 3 == mHandlerCalls);
	// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
	ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
}

template <> template <>
void HttpRequestTestObjectType::test<5>()
{
	ScopedCurlInit ready;

	set_test_name("HttpRequest GET + Stop execution");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
		
	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	// Get singletons created
	HttpRequest::createService();
		
	// Start threading early so that thread memory is invariant
	// over the test.
	HttpRequest::startThread();

	// create a new ref counted object with an implicit reference
	HttpRequest * req = new HttpRequest();
	ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

	// Issue a GET that can't connect
	mStatus = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_CONNECT);
	HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
												 0.0f,
												 "http://127.0.0.1:2/nothing/here",
												 0,
												 0,
												 NULL,
												 NULL,
												 &handler);
	ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

	// Run the notification pump.
	int count(0);
	int limit(20);
	while (count++ < limit && mHandlerCalls < 1)
	{
		req->update(1000);
		usleep(100000);
	}
	ensure("Request executed in reasonable time", count < limit);
	ensure("One handler invocation for request", mHandlerCalls == 1);

	// Okay, request a shutdown of the servicing thread
	mStatus = HttpStatus();
	handle = req->requestStopThread(&handler);
	ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
	// Run the notification pump again
	count = 0;
	limit = 100;
	while (count++ < limit && mHandlerCalls < 2)
	{
		req->update(1000);
		usleep(100000);
	}
	ensure("Second request executed in reasonable time", count < limit);
	ensure("Second handler invocation", mHandlerCalls == 2);

	// See that we actually shutdown the thread
	count = 0;
	limit = 10;
	while (count++ < limit && ! HttpService::isStopped())
	{
		usleep(100000);
	}
	ensure("Thread actually stopped running", HttpService::isStopped());
	
	// release the request object
	delete req;
	req = NULL;

	// Shut down service
	HttpRequest::destroyService();
	
	// We have left over state so can't really say something
	// definitive about memory usage at the end of this.
	ensure("Two handler calls on the way out", 2 == mHandlerCalls);
	// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
	ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
}

}  // end namespace tut

namespace
{

#if defined(WIN32)

void usleep(unsigned long usec)
{
	Sleep((DWORD) (usec / 1000UL));
}

#endif

}

#endif  // TEST_LLCORE_HTTP_REQUEST_H_
