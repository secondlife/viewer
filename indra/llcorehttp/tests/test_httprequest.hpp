/** 
 * @file test_httprequest.hpp
 * @brief unit tests for the LLCore::HttpRequest class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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
#include "bufferarray.h"
#include "httphandler.h"
#include "httpheaders.h"
#include "httpresponse.h"
#include "httpoptions.h"
#include "_httpservice.h"
#include "_httprequestqueue.h"

#include <curl/curl.h>
#include <boost/regex.hpp>
#include <sstream>

#include "test_allocator.h"
#include "llcorehttp_test.h"


using namespace LLCoreInt;

// spin/sleep waiting times for client/server exchange tests
//
// These are now fairly generous to try to get around timeout
// ('reasonable time') failures during execution on a heavily-
// loaded system where the unit test is in competition with
// other programs.
static const int LOOP_SLEEP_INTERVAL(10000);
static const int LOOP_COUNT_SHORT(500);			// 5-second dwell time
static const int LOOP_COUNT_LONG(3000);			// 30-second dwell time

namespace
{

#if defined(WIN32)

void usleep(unsigned long usec);

#endif

}

namespace tut
{

typedef std::vector<std::pair<boost::regex, boost::regex> > regex_container_t;

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
				std::ostringstream test;
				test << "Expected HttpStatus received in response.  Wanted:  "
					 << mState->mStatus.toHex() << " Received:  " << actual_status.toHex();
				ensure(test.str().c_str(), actual_status == mState->mStatus);
			}
			if (mState)
			{
				mState->mHandlerCalls++;
			}
			if (! mHeadersRequired.empty() || ! mHeadersDisallowed.empty())
			{
				ensure("Response required with header check", response != NULL);
				HttpHeaders::ptr_t header(response->getHeaders());	// Will not hold onto this
				ensure("Some quantity of headers returned", header != NULL);

				if (! mHeadersRequired.empty())
				{
					for (int i(0); i < mHeadersRequired.size(); ++i)
					{
						bool found = false;
						for (HttpHeaders::const_iterator iter(header->begin());
							 header->end() != iter;
							 ++iter)
						{
							// std::cerr << "Header: " << (*iter).first
							//		  << ": " << (*iter).second << std::endl;
							
							if (boost::regex_match((*iter).first,
												   mHeadersRequired[i].first) &&
								boost::regex_match((*iter).second,
												   mHeadersRequired[i].second))
							{
								found = true;
								break;
							}
						}
						std::ostringstream str;
						str << "Required header # " << i << " found in response";
						ensure(str.str(), found);
					}
				}

				if (! mHeadersDisallowed.empty())
				{
					for (int i(0); i < mHeadersDisallowed.size(); ++i)
					{
						for (HttpHeaders::const_iterator iter(header->begin());
							 header->end() != iter;
							 ++iter)
						{
							if (boost::regex_match((*iter).first,
												   mHeadersDisallowed[i].first) &&
								boost::regex_match((*iter).second,
												   mHeadersDisallowed[i].second))
							{
								std::ostringstream str;
								str << "Disallowed header # " << i << " not found in response";
								ensure(str.str(), false);
							}
						}
					}
				}
			}
			
			if (! mCheckContentType.empty())
			{
				ensure("Response required with content type check", response != NULL);
				std::string con_type(response->getContentType());
				ensure("Content-Type as expected (" + mCheckContentType + ")",
					   mCheckContentType == con_type);
			}

			// std::cout << "TestHandler2::onCompleted() invoked" << std::endl;
		}

	HttpRequestTestData * mState;
	std::string mName;
	HttpHandle mExpectHandle;
	std::string mCheckContentType;
	regex_container_t mHeadersRequired;
	regex_container_t mHeadersDisallowed;
};

typedef test_group<HttpRequestTestData> HttpRequestTestGroupType;
typedef HttpRequestTestGroupType::object HttpRequestTestObjectType;
HttpRequestTestGroupType HttpRequestTestGroup("HttpRequest Tests");

template <> template <>
void HttpRequestTestObjectType::test<1>()
{
	ScopedCurlInit ready;
	
	set_test_name("HttpRequest construction");

	HttpRequest * req = NULL;

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	try
	{
		// Get singletons created
		HttpRequest::createService();
		
		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory being used", mMemTotal < GetMemTotal());
		
		// release the request object
		delete req;
		req = NULL;

		HttpRequest::destroyService();

		// make sure we didn't leak any memory
		ensure("Memory returned", mMemTotal == GetMemTotal());
	}
	catch (...)
	{
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}

template <> template <>
void HttpRequestTestObjectType::test<2>()
{
	ScopedCurlInit ready;

	set_test_name("HttpRequest and Null Op queued");

	HttpRequest * req = NULL;

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	try
	{
		// Get singletons created
		HttpRequest::createService();
		
		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory being used", mMemTotal < GetMemTotal());

		// Issue a NoOp
		HttpHandle handle = req->requestNoOp(LLCore::HttpHandler::ptr_t());
		ensure("Request issued", handle != LLCORE_HTTP_HANDLE_INVALID);
		
		// release the request object
		delete req;
		req = NULL;

		// We're still holding onto the operation which is
		// sitting, unserviced, on the request queue so...
		ensure("Memory being used 2", mMemTotal < GetMemTotal());

		// Request queue should have two references:  global singleton & service object
		ensure("Two references to request queue", 2 == HttpRequestQueue::instanceOf()->getRefCount());

		// Okay, tear it down
		HttpRequest::destroyService();
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory returned", mMemTotal == GetMemTotal());
	}
	catch (...)
	{
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}

namespace
{
    void NoOpDeletor(LLCore::HttpHandler *) { }
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
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	
	try
	{
		// Get singletons created
        HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue a NoOp
		HttpHandle handle = req->requestNoOp(handlerp);
		ensure("Valid handle returned for first request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_SHORT);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 2 == mHandlerCalls);
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
	}
	catch (...)
	{
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
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

    LLCore::HttpHandler::ptr_t handler1p(&handler1, NoOpDeletor);
    LLCore::HttpHandler::ptr_t handler2p(&handler2, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req1 = NULL;
	HttpRequest * req2 = NULL;
	
	try
	{

		// Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req1 = new HttpRequest();
		req2 = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue some NoOps
		HttpHandle handle = req1->requestNoOp(handler1p);
		ensure("Valid handle returned for first request", handle != LLCORE_HTTP_HANDLE_INVALID);
		handler1.mExpectHandle = handle;

		handle = req2->requestNoOp(handler2p);
		ensure("Valid handle returned for first request", handle != LLCORE_HTTP_HANDLE_INVALID);
		handler2.mExpectHandle = handle;

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 2)
		{
			req1->update(1000000);
			req2->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 2);

		// Okay, request a shutdown of the servicing thread
		handle = req2->requestStopThread(handler2p);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
		handler2.mExpectHandle = handle;
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 3)
		{
			req1->update(1000000);
			req2->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 3);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release the request object
		delete req1;
		req1 = NULL;
		delete req2;
		req2 = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 3 == mHandlerCalls);
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
	}
	catch (...)
	{
		stop_thread(req1);
		delete req1;
		delete req2;
		HttpRequest::destroyService();
		throw;
	}
}

template <> template <>
void HttpRequestTestObjectType::test<5>()
{
	ScopedCurlInit ready;
	
	set_test_name("HttpRequest Spin (soft) + NoOp + hard termination");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	
	try
	{
		// Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue a Spin
		HttpHandle handle = req->requestSpin(1);
		ensure("Valid handle returned for spin request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Issue a NoOp
		handle = req->requestNoOp(handlerp);
		ensure("Valid handle returned for no-op request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_SHORT);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("NoOp notification received", mHandlerCalls == 1);

		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();

		// Check memory usage
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
		// This memory test should work but could give problems as it
		// relies on the worker thread picking up a friendly request
		// to shutdown.  Doing so, it drops references to things and
		// we should go back to where we started.  If it gives you
		// problems, look into the code before commenting things out.
	}
	catch (...)
	{
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


template <> template <>
void HttpRequestTestObjectType::test<6>()
{
	ScopedCurlInit ready;
	
	set_test_name("HttpRequest Spin + NoOp + hard termination");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
		
	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	
	try
	{
        LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

		// Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue a Spin
		HttpHandle handle = req->requestSpin(0);		// Hard spin
		ensure("Valid handle returned for spin request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Issue a NoOp
		handle = req->requestNoOp(handlerp);
		ensure("Valid handle returned for no-op request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_SHORT);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("No notifications received", mHandlerCalls == 0);

		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();

		// Check memory usage
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		// ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
		// This memory test won't work because we're killing the thread
		// hard with the hard spinner.  There's no opportunity to join
		// nicely so many things leak or get destroyed unilaterally.
	}
	catch (...)
	{
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


template <> template <>
void HttpRequestTestObjectType::test<7>()
{
	ScopedCurlInit ready;

	set_test_name("HttpRequest GET to dead port + Stop execution");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");

    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t opts;
	
	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

        opts = HttpOptions::ptr_t(new HttpOptions());
		opts->setRetries(1);			// Don't try for too long - default retries take about 18S
		
		// Issue a GET that can't connect
		mStatus = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_CONNECT);
		HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
													 0U,
													 "http://127.0.0.1:2/nothing/here",
													 0,
													 0,
													 opts,
													 HttpHeaders::ptr_t(),
													 handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());

		// release options
        opts.reset();
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 2 == mHandlerCalls);

#if 0 // defined(WIN32)
		// Can't do this on any platform anymore, the LL logging system holds
		// on to memory and produces what looks like memory leaks...

		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		stop_thread(req);
        opts.reset();
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


template <> template <>
void HttpRequestTestObjectType::test<8>()
{
	ScopedCurlInit ready;

	std::string url_base(get_base_url());
	// std::cerr << "Base:  "  << url_base << std::endl;
	
	set_test_name("HttpRequest GET to real service");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;

	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue a GET that *can* connect
		mStatus = HttpStatus(200);
		HttpHandle handle = req->requestGet(HttpRequest::DEFAULT_POLICY_ID,
											0U,
											url_base,
											HttpOptions::ptr_t(),
                                            HttpHeaders::ptr_t(),
											handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 2 == mHandlerCalls);

#if defined(WIN32)
		// Can only do this memory test on Windows.  On other platforms,
		// the LL logging system holds on to memory and produces what looks
		// like memory leaks...
	
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


template <> template <>
void HttpRequestTestObjectType::test<9>()
{
	ScopedCurlInit ready;

	std::string url_base(get_base_url());
	// std::cerr << "Base:  "  << url_base << std::endl;
	
	set_test_name("HttpRequest GET with Range: header to real service");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;

	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue a GET that *can* connect
		mStatus = HttpStatus(200);
		HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
													 0U,
													 url_base,
													 0,
													 0,
													 HttpOptions::ptr_t(),
                                                     HttpHeaders::ptr_t(),
													 handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 2 == mHandlerCalls);

#if defined(WIN32)
		// Can only do this memory test on Windows.  On other platforms,
		// the LL logging system holds on to memory and produces what looks
		// like memory leaks...
	
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


template <> template <>
void HttpRequestTestObjectType::test<10>()
{
	ScopedCurlInit ready;

	std::string url_base(get_base_url());
	// std::cerr << "Base:  "  << url_base << std::endl;
	
	set_test_name("HttpRequest PUT to real service");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	BufferArray * body = new BufferArray;
	
	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue a GET that *can* connect
		static const char * body_text("Now is the time for all good men...");
		body->append(body_text, strlen(body_text));
		mStatus = HttpStatus(200);
		HttpHandle handle = req->requestPut(HttpRequest::DEFAULT_POLICY_ID,
											0U,
											url_base,
											body,
                                            HttpOptions::ptr_t(),
                                            HttpHeaders::ptr_t(),
                                            handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());

		// Lose the request body
		body->release();
		body = NULL;
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 2 == mHandlerCalls);

#if 0 // defined(WIN32)
		// Can't do this on any platform anymore, the LL logging system holds
		// on to memory and produces what looks like memory leaks...
	
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		if (body)
		{
			body->release();
		}
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}

template <> template <>
void HttpRequestTestObjectType::test<11>()
{
	ScopedCurlInit ready;

	std::string url_base(get_base_url());
	// std::cerr << "Base:  "  << url_base << std::endl;
	
	set_test_name("HttpRequest POST to real service");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	BufferArray * body = new BufferArray;
	
	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue a GET that *can* connect
		static const char * body_text("Now is the time for all good men...");
		body->append(body_text, strlen(body_text));
		mStatus = HttpStatus(200);
		HttpHandle handle = req->requestPost(HttpRequest::DEFAULT_POLICY_ID,
											 0U,
											 url_base,
											 body,
                                             HttpOptions::ptr_t(),
                                             HttpHeaders::ptr_t(),
                                             handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());

		// Lose the request body
		body->release();
		body = NULL;
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 2 == mHandlerCalls);

#if defined(WIN32)
		// Can only do this memory test on Windows.  On other platforms,
		// the LL logging system holds on to memory and produces what looks
		// like memory leaks...
	
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		if (body)
		{
			body->release();
		}
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}

template <> template <>
void HttpRequestTestObjectType::test<12>()
{
	ScopedCurlInit ready;

	std::string url_base(get_base_url());
	// std::cerr << "Base:  "  << url_base << std::endl;
	
	set_test_name("HttpRequest GET with some tracing");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;

	try
	{
        // Get singletons created
		HttpRequest::createService();

		// Enable tracing
		HttpRequest::setStaticPolicyOption(HttpRequest::PO_TRACE, HttpRequest::DEFAULT_POLICY_ID, 2, NULL);

		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue a GET that *can* connect
		mStatus = HttpStatus(200);
		HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
													 0U,
													 url_base,
													 0,
													 0,
                                                     HttpOptions::ptr_t(),
                                                     HttpHeaders::ptr_t(),
                                                     handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 2 == mHandlerCalls);

#if 0	// defined(WIN32)
		// Can't do this on any platform anymore, the LL logging system holds
		// on to memory and produces what looks like memory leaks...
	
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


template <> template <>
void HttpRequestTestObjectType::test<13>()
{
	ScopedCurlInit ready;

	// Warmup boost::regex to pre-alloc memory for memory size tests
	boost::regex warmup("askldjflasdj;f", boost::regex::icase);
	boost::regex_match("akl;sjflajfk;ajsk", warmup);
	
	std::string url_base(get_base_url());
	// std::cerr << "Base:  "  << url_base << std::endl;
	
	set_test_name("HttpRequest GET with returned headers");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
	handler.mHeadersRequired.reserve(20);				// Avoid memory leak test failure
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t opts;

	try
	{
        // Get singletons created
		HttpRequest::createService();

		// Enable tracing
		HttpRequest::setStaticPolicyOption(HttpRequest::PO_TRACE, HttpRequest::DEFAULT_POLICY_ID, 2, NULL);

		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

        opts = HttpOptions::ptr_t(new HttpOptions());
		opts->setWantHeaders(true);
		
		// Issue a GET that succeeds
		mStatus = HttpStatus(200);
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(boost::regex("X-LL-Special", boost::regex::icase),
										  boost::regex(".*", boost::regex::icase)));
		HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
													 0U,
													 url_base,
													 0,	
												 0,
													 opts,
                                                     HttpHeaders::ptr_t(),
                                                     handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// release options
        opts.reset();

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handler.mHeadersRequired.clear();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 2 == mHandlerCalls);

#if defined(WIN32)
		// Can only do this memory test on Windows.  On other platforms,
		// the LL logging system holds on to memory and produces what looks
		// like memory leaks...
	
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		stop_thread(req);
        opts.reset();
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


template <> template <>
void HttpRequestTestObjectType::test<14>()
{
	ScopedCurlInit ready;

	set_test_name("HttpRequest GET timeout");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
	LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);
	std::string url_base(get_base_url() + "/sleep/");   // path to a 30-second sleep

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t opts;

	try
	{
		// Get singletons created
		HttpRequest::createService();

		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		opts = HttpOptions::ptr_t(new HttpOptions);
		opts->setRetries(0);            // Don't retry
		opts->setTimeout(2);

		// Issue a GET that sleeps
		mStatus = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_OPERATION_TIMEDOUT);
		HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
													 0U,
													 url_base,
													 0,
													 0,
													 opts,
													 HttpHeaders::ptr_t(),
													 handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());

		// release options
		opts.reset();

		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();

		ensure("Two handler calls on the way out", 2 == mHandlerCalls);

#if 0 // defined(WIN32)
		// Can't do this on any platform anymore, the LL logging system holds
		// on to memory and produces what looks like memory leaks...

		// printf("Old mem:	 %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		stop_thread(req);
		opts.reset();
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}

// Test retrieval of Content-Type/Content-Encoding headers
template <> template <>
void HttpRequestTestObjectType::test<15>()
{
	ScopedCurlInit ready;

	std::string url_base(get_base_url());
	// std::cerr << "Base:  "  << url_base << std::endl;
	
	set_test_name("HttpRequest GET with Content-Type");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// Load and clear the string setting to preload std::string object
	// for memory return tests.
	handler.mCheckContentType = "application/llsd+xml";
	handler.mCheckContentType.clear();
		
	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;

	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// Issue a GET that *can* connect
		mStatus = HttpStatus(200);
		handler.mCheckContentType = "application/llsd+xml";
		HttpHandle handle = req->requestGet(HttpRequest::DEFAULT_POLICY_ID,
											0U,
											url_base,
                                            HttpOptions::ptr_t(),
                                            HttpHeaders::ptr_t(),
                                            handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handler.mCheckContentType.clear();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	
		ensure("Two handler calls on the way out", 2 == mHandlerCalls);

#if defined(WIN32)
		// Can only do this memory test on Windows.  On other platforms,
		// the LL logging system holds on to memory and produces what looks
		// like memory leaks...
	
		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


// Test header generation on GET requests
template <> template <>
void HttpRequestTestObjectType::test<16>()
{
	ScopedCurlInit ready;

	// Warmup boost::regex to pre-alloc memory for memory size tests
	boost::regex warmup("askldjflasdj;f", boost::regex::icase);
	boost::regex_match("akl;sjflajfk;ajsk", warmup);

	std::string url_base(get_base_url());
	
	set_test_name("Header generation for HttpRequest GET");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t options;
	HttpHeaders::ptr_t headers;

	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();

		// options set
        options = HttpOptions::ptr_t(new HttpOptions());
		options->setWantHeaders(true);
		
		// Issue a GET that *can* connect
		mStatus = HttpStatus(200);
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-connection", boost::regex::icase),
				boost::regex("keep-alive", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept", boost::regex::icase),
				boost::regex("\\*/\\*", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept-encoding", boost::regex::icase),
				boost::regex("((gzip|deflate),\\s*)+(gzip|deflate)", boost::regex::icase))); // close enough
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-keep-alive", boost::regex::icase),
				boost::regex("\\d+", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-host", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-cache-control", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-pragma", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-range", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-transfer-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-referer", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		HttpHandle handle = req->requestGet(HttpRequest::DEFAULT_POLICY_ID,
											0U,
											url_base + "reflect/",
											options,
											HttpHeaders::ptr_t(),
											handlerp);
		ensure("Valid handle returned for get request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Do a texture-style fetch
		headers = HttpHeaders::ptr_t(new HttpHeaders);
		headers->append("Accept", "image/x-j2c");
		
		mStatus = HttpStatus(200);
		handler.mHeadersRequired.clear();
		handler.mHeadersDisallowed.clear();
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-connection", boost::regex::icase),
				boost::regex("keep-alive", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept", boost::regex::icase),
				boost::regex("image/x-j2c", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept-encoding", boost::regex::icase),
				boost::regex("((gzip|deflate),\\s*)+(gzip|deflate)", boost::regex::icase))); // close enough
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-keep-alive", boost::regex::icase),
				boost::regex("\\d+", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-host", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("\\W*X-Reflect-range", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));

		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-cache-control", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-pragma", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-transfer-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-referer", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
										  0U,
										  url_base + "reflect/",
										  0,
										  47,
										  options,
										  headers,
										  handlerp);
		ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 2);


		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handler.mHeadersRequired.clear();
		handler.mHeadersDisallowed.clear();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 3)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 3);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release options & headers
        options.reset();
        headers.reset();
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	}
	catch (...)
	{
		stop_thread(req);
        options.reset();
        headers.reset();

		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


// Test header generation on POST requests
template <> template <>
void HttpRequestTestObjectType::test<17>()
{
	ScopedCurlInit ready;

	// Warmup boost::regex to pre-alloc memory for memory size tests
	boost::regex warmup("askldjflasdj;f", boost::regex::icase);
	boost::regex_match("akl;sjflajfk;ajsk", warmup);

	std::string url_base(get_base_url());
	
	set_test_name("Header generation for HttpRequest POST");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t options;
	HttpHeaders::ptr_t headers;
	BufferArray * ba = NULL;
	
	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();

		// options set
        options = HttpOptions::ptr_t(new HttpOptions());
		options->setWantHeaders(true);

		// And a buffer array
		const char * msg("It was the best of times, it was the worst of times.");
		ba = new BufferArray;
		ba->append(msg, strlen(msg));
			
		// Issue a default POST
		mStatus = HttpStatus(200);
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-connection", boost::regex::icase),
				boost::regex("keep-alive", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept", boost::regex::icase),
				boost::regex("\\*/\\*", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept-encoding", boost::regex::icase),
				boost::regex("((gzip|deflate),\\s*)+(gzip|deflate)", boost::regex::icase))); // close enough
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-keep-alive", boost::regex::icase),
				boost::regex("\\d+", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-host", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-length", boost::regex::icase),
				boost::regex("\\d+", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex("application/x-www-form-urlencoded", boost::regex::icase)));

		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-cache-control", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-pragma", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-range", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-referer", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-expect", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-transfer_encoding", boost::regex::icase),
				boost::regex(".*chunked.*", boost::regex::icase)));
		HttpHandle handle = req->requestPost(HttpRequest::DEFAULT_POLICY_ID,
											 0U,
											 url_base + "reflect/",
											 ba,
											 options,
											 HttpHeaders::ptr_t(),
											 handlerp);
		ensure("Valid handle returned for get request", handle != LLCORE_HTTP_HANDLE_INVALID);
		ba->release();
		ba = NULL;
			
		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);


		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handler.mHeadersRequired.clear();
		handler.mHeadersDisallowed.clear();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release options & headers
        options.reset();
        headers.reset();
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	}
	catch (...)
	{
		stop_thread(req);
		if (ba)
		{
			ba->release();
			ba = NULL;
		}
        options.reset();
        headers.reset();

        delete req;
		HttpRequest::destroyService();
		throw;
	}
}


// Test header generation on PUT requests
template <> template <>
void HttpRequestTestObjectType::test<18>()
{
	ScopedCurlInit ready;

	// Warmup boost::regex to pre-alloc memory for memory size tests
	boost::regex warmup("askldjflasdj;f", boost::regex::icase);
	boost::regex_match("akl;sjflajfk;ajsk", warmup);

	std::string url_base(get_base_url());
	
	set_test_name("Header generation for HttpRequest PUT");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t options;
	HttpHeaders::ptr_t headers;
	BufferArray * ba = NULL;
	
	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();

		// options set
		options = HttpOptions::ptr_t(new HttpOptions());
		options->setWantHeaders(true);

		// And a buffer array
		const char * msg("It was the best of times, it was the worst of times.");
		ba = new BufferArray;
		ba->append(msg, strlen(msg));
			
		// Issue a default PUT
		mStatus = HttpStatus(200);
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-connection", boost::regex::icase),
				boost::regex("keep-alive", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept", boost::regex::icase),
				boost::regex("\\*/\\*", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept-encoding", boost::regex::icase),
				boost::regex("((gzip|deflate),\\s*)+(gzip|deflate)", boost::regex::icase))); // close enough
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-keep-alive", boost::regex::icase),
				boost::regex("\\d+", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-host", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-length", boost::regex::icase),
				boost::regex("\\d+", boost::regex::icase)));

		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-cache-control", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-pragma", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-range", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-referer", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-expect", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-transfer-encoding", boost::regex::icase),
				boost::regex(".*chunked.*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));

		HttpHandle handle = req->requestPut(HttpRequest::DEFAULT_POLICY_ID,
											0U,
											url_base + "reflect/",
											ba,
											options,
											HttpHeaders::ptr_t(),
											handlerp);
		ensure("Valid handle returned for get request", handle != LLCORE_HTTP_HANDLE_INVALID);
		ba->release();
		ba = NULL;
			
		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);


		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handler.mHeadersRequired.clear();
		handler.mHeadersDisallowed.clear();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release options & headers
        options.reset();
        headers.reset();
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	}
	catch (...)
	{
		stop_thread(req);
		if (ba)
		{
			ba->release();
			ba = NULL;
		}
        options.reset();
        headers.reset();

        delete req;
		HttpRequest::destroyService();
		throw;
	}
}


// Test header generation on GET requests with overrides
template <> template <>
void HttpRequestTestObjectType::test<19>()
{
	ScopedCurlInit ready;

	// Warmup boost::regex to pre-alloc memory for memory size tests
	boost::regex warmup("askldjflasdj;f", boost::regex::icase);
	boost::regex_match("akl;sjflajfk;ajsk", warmup);

	std::string url_base(get_base_url());
	
	set_test_name("Header generation for HttpRequest GET with header overrides");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t options;
	HttpHeaders::ptr_t headers;

	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();

		// options set
        options = HttpOptions::ptr_t(new HttpOptions());
		options->setWantHeaders(true);

		// headers
		headers = HttpHeaders::ptr_t(new HttpHeaders);
		headers->append("Keep-Alive", "120");
		headers->append("Accept-encoding", "deflate");
		headers->append("Accept", "text/plain");

		// Issue a GET with modified headers
		mStatus = HttpStatus(200);
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-connection", boost::regex::icase),
				boost::regex("keep-alive", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept", boost::regex::icase),
				boost::regex("text/plain", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept-encoding", boost::regex::icase),
				boost::regex("deflate", boost::regex::icase))); // close enough
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-keep-alive", boost::regex::icase),
				boost::regex("120", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-host", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));

		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept-encoding", boost::regex::icase),
				boost::regex("((gzip|deflate),\\s*)+(gzip|deflate)", boost::regex::icase))); // close enough
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-keep-alive", boost::regex::icase),
				boost::regex("300", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept", boost::regex::icase),
				boost::regex("\\*/\\*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-cache-control", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-pragma", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-range", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-transfer-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-referer", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		HttpHandle handle = req->requestGet(HttpRequest::DEFAULT_POLICY_ID,
											0U,
											url_base + "reflect/",
											options,
											headers,
											handlerp);
		ensure("Valid handle returned for get request", handle != LLCORE_HTTP_HANDLE_INVALID);

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handler.mHeadersRequired.clear();
		handler.mHeadersDisallowed.clear();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release options & headers
        options.reset();
        headers.reset();
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	}
	catch (...)
	{
		stop_thread(req);
        options.reset();
        headers.reset();

		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


// Test header generation on POST requests with overrides
template <> template <>
void HttpRequestTestObjectType::test<20>()
{
	ScopedCurlInit ready;

	// Warmup boost::regex to pre-alloc memory for memory size tests
	boost::regex warmup("askldjflasdj;f", boost::regex::icase);
	boost::regex_match("akl;sjflajfk;ajsk", warmup);

	std::string url_base(get_base_url());
	
	set_test_name("Header generation for HttpRequest POST with header overrides");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t options;
	HttpHeaders::ptr_t headers;
	BufferArray * ba = NULL;
	
	try
	{

        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();

		// options set
        options = HttpOptions::ptr_t(new HttpOptions());
		options->setWantHeaders(true);

		// headers
		headers = HttpHeaders::ptr_t(new HttpHeaders());
		headers->append("keep-Alive", "120");
		headers->append("Accept", "text/html");
		headers->append("content-type", "application/llsd+xml");
		headers->append("cache-control", "no-store");
		
		// And a buffer array
		const char * msg("<xml><llsd><string>It was the best of times, it was the worst of times.</string></llsd></xml>");
		ba = new BufferArray;
		ba->append(msg, strlen(msg));
			
		// Issue a default POST
		mStatus = HttpStatus(200);
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-connection", boost::regex::icase),
				boost::regex("keep-alive", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept", boost::regex::icase),
				boost::regex("text/html", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept-encoding", boost::regex::icase),
				boost::regex("((gzip|deflate),\\s*)+(gzip|deflate)", boost::regex::icase))); // close enough
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-keep-alive", boost::regex::icase),
				boost::regex("120", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-host", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-length", boost::regex::icase),
				boost::regex("\\d+", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex("application/llsd\\+xml", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-cache-control", boost::regex::icase),
				boost::regex("no-store", boost::regex::icase)));

		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex("application/x-www-form-urlencoded", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept", boost::regex::icase),
				boost::regex("\\*/\\*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-keep-alive", boost::regex::icase),
				boost::regex("300", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-pragma", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-range", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-referer", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-expect", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-transfer-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));

		HttpHandle handle = req->requestPost(HttpRequest::DEFAULT_POLICY_ID,
											 0U,
											 url_base + "reflect/",
											 ba,
											 options,
											 headers,
											 handlerp);
		ensure("Valid handle returned for get request", handle != LLCORE_HTTP_HANDLE_INVALID);
		ba->release();
		ba = NULL;
			
		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);


		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handler.mHeadersRequired.clear();
		handler.mHeadersDisallowed.clear();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release options & headers
        options.reset();
        headers.reset();
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	}
	catch (...)
	{
		stop_thread(req);
		if (ba)
		{
			ba->release();
			ba = NULL;
		}
        options.reset();
        headers.reset();
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}


// Test header generation on PUT requests with overrides
template <> template <>
void HttpRequestTestObjectType::test<21>()
{
	ScopedCurlInit ready;

	// Warmup boost::regex to pre-alloc memory for memory size tests
	boost::regex warmup("askldjflasdj;f", boost::regex::icase);
	boost::regex_match("akl;sjflajfk;ajsk", warmup);

	std::string url_base(get_base_url());
	
	set_test_name("Header generation for HttpRequest PUT with header overrides");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t options;
	HttpHeaders::ptr_t headers;
	BufferArray * ba = NULL;
	
	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();

		// options set
        options = HttpOptions::ptr_t(new HttpOptions());
		options->setWantHeaders(true);

		// headers
		headers = HttpHeaders::ptr_t(new HttpHeaders);
		headers->append("content-type", "text/plain");
		headers->append("content-type", "text/html");
		headers->append("content-type", "application/llsd+xml");
		
		// And a buffer array
		const char * msg("<xml><llsd><string>It was the best of times, it was the worst of times.</string></llsd></xml>");
		ba = new BufferArray;
		ba->append(msg, strlen(msg));
			
		// Issue a default PUT
		mStatus = HttpStatus(200);
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-connection", boost::regex::icase),
				boost::regex("keep-alive", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept", boost::regex::icase),
				boost::regex("\\*/\\*", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-accept-encoding", boost::regex::icase),
				boost::regex("((gzip|deflate),\\s*)+(gzip|deflate)", boost::regex::icase))); // close enough
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-keep-alive", boost::regex::icase),
				boost::regex("\\d+", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-host", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-length", boost::regex::icase),
				boost::regex("\\d+", boost::regex::icase)));
		handler.mHeadersRequired.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex("application/llsd\\+xml", boost::regex::icase)));

		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-cache-control", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-pragma", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-range", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-referer", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-expect", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-transfer-encoding", boost::regex::icase),
				boost::regex(".*", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex("text/plain", boost::regex::icase)));
		handler.mHeadersDisallowed.push_back(
			regex_container_t::value_type(
				boost::regex("X-Reflect-content-type", boost::regex::icase),
				boost::regex("text/html", boost::regex::icase)));
		HttpHandle handle = req->requestPut(HttpRequest::DEFAULT_POLICY_ID,
											0U,
											url_base + "reflect/",
											ba,
											options,
											headers,
											handlerp);
		ensure("Valid handle returned for get request", handle != LLCORE_HTTP_HANDLE_INVALID);
		ba->release();
		ba = NULL;
			
		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == 1);


		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		handler.mHeadersRequired.clear();
		handler.mHeadersDisallowed.clear();
		handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 2)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 2);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());
	
		// release options & headers
        options.reset();
        headers.reset();
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();
	}
	catch (...)
	{
		stop_thread(req);
		if (ba)
		{
			ba->release();
			ba = NULL;
		}
        options.reset();
        headers.reset();
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}

// BUG-2295 Tests - Content-Range header received but no body
template <> template <>
void HttpRequestTestObjectType::test<22>()
{
	ScopedCurlInit ready;

	std::string url_base(get_base_url());
	// std::cerr << "Base:  "  << url_base << std::endl;
	
	set_test_name("BUG-2295");

	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpOptions::ptr_t options;
	HttpRequest * req = NULL;

	try
	{
        // options set
        options = HttpOptions::ptr_t(new HttpOptions());
		options->setRetries(1);			// Partial_File is retryable and can timeout in here

		// Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

		// ======================================
		// Issue bug2295 GETs that will get a 206
		// ======================================
		mStatus = HttpStatus(206);
		static const int test_count(3);
		for (int i(0); i < test_count; ++i)
		{
			char buffer[128];
			sprintf(buffer, "/bug2295/%d/", i);
			HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
														 0U,
														 url_base + buffer,
														 0,
														 25,
														 options,
                                                         HttpHeaders::ptr_t(),
                                                         handlerp);
			ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);
		}
		
		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < test_count)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time - ms1", count < limit);
		ensure("One handler invocation for each request - ms1", mHandlerCalls == test_count);

		// ======================================
		// Issue bug2295 GETs that will get a libcurl 18 (PARTIAL_FILE)
		// ======================================
		mHandlerCalls = 0;
		mStatus = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_PARTIAL_FILE);
		static const int test2_count(1);
		for (int i(0); i < test2_count; ++i)
		{
			char buffer[128];
			sprintf(buffer, "/bug2295/00000012/%d/", i);
			HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
														 0U,
														 url_base + buffer,
														 0,
														 25,
														 options,
														 HttpHeaders::ptr_t(),
														 handlerp);
			ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);
		}
		
		// Run the notification pump.
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < test2_count)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time - ms2", count < limit);
		ensure("One handler invocation for each request - ms2", mHandlerCalls == test2_count);

		// ======================================
		// Issue bug2295 GETs that will get an llcorehttp HE_INV_CONTENT_RANGE_HDR status
		// ======================================
		mHandlerCalls = 0;
		mStatus = HttpStatus(HttpStatus::LLCORE, HE_INV_CONTENT_RANGE_HDR);
		static const int test3_count(1);
		for (int i(0); i < test3_count; ++i)
		{
			char buffer[128];
			sprintf(buffer, "/bug2295/inv_cont_range/%d/", i);
			HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
														 0U,
														 url_base + buffer,
														 0,
														 25,
														 options,
                                                         HttpHeaders::ptr_t(),
                                                         handlerp);
			ensure("Valid handle returned for ranged request", handle != LLCORE_HTTP_HANDLE_INVALID);
		}
		
		// Run the notification pump.
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < test3_count)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time - ms3", count < limit);
		ensure("One handler invocation for each request - ms3", mHandlerCalls == test3_count);

		// ======================================
		// Okay, request a shutdown of the servicing thread
		// ======================================
		mStatus = HttpStatus();
		mHandlerCalls = 0;
		HttpHandle handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Shutdown request executed in reasonable time", count < limit);
		ensure("Shutdown handler invocation", mHandlerCalls == 1);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());

		// release options
        options.reset();
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();

#if 0 // defined(WIN32)
		// Can't do this on any platform anymore, the LL logging system holds
		// on to memory and produces what looks like memory leaks...

		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		stop_thread(req);
		delete req;
		HttpRequest::destroyService();
		throw;
	}
}

template <> template <>
void HttpRequestTestObjectType::test<23>()
{
	ScopedCurlInit ready;

	set_test_name("HttpRequest GET 503s with 'Retry-After'");

	// This tests mainly that the code doesn't fall over if
	// various well- and mis-formed Retry-After headers are
	// sent along with the response.  Direct inspection of
	// the parsing result isn't supported.
	
	// Handler can be stack-allocated *if* there are no dangling
	// references to it after completion of this method.
	// Create before memory record as the string copy will bump numbers.
	TestHandler2 handler(this, "handler");
    LLCore::HttpHandler::ptr_t handlerp(&handler, NoOpDeletor);
    std::string url_base(get_base_url() + "/503/");	// path to 503 generators
		
	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();
	mHandlerCalls = 0;

	HttpRequest * req = NULL;
	HttpOptions::ptr_t opts;
	
	try
	{
        // Get singletons created
		HttpRequest::createService();
		
		// Start threading early so that thread memory is invariant
		// over the test.
		HttpRequest::startThread();

		// create a new ref counted object with an implicit reference
		req = new HttpRequest();
		ensure("Memory allocated on construction", mMemTotal < GetMemTotal());

        opts = HttpOptions::ptr_t(new HttpOptions());
		opts->setRetries(1);			// Retry once only
		opts->setUseRetryAfter(true);	// Try to parse the retry-after header
		
		// Issue a GET that 503s with valid retry-after
		mStatus = HttpStatus(503);
		int url_limit(6);
		for (int i(0); i < url_limit; ++i)
		{
			std::ostringstream url;
			url << url_base << i << "/";
			HttpHandle handle = req->requestGetByteRange(HttpRequest::DEFAULT_POLICY_ID,
														 0U,
														 url.str(),
														 0,
														 0,
														 opts,
                                                         HttpHeaders::ptr_t(),
                                                         handlerp);

			std::ostringstream testtag;
			testtag << "Valid handle returned for 503 request #" << i;
			ensure(testtag.str(), handle != LLCORE_HTTP_HANDLE_INVALID);
		}
		

		// Run the notification pump.
		int count(0);
		int limit(LOOP_COUNT_LONG);
		while (count++ < limit && mHandlerCalls < url_limit)
		{
			req->update(0);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Request executed in reasonable time", count < limit);
		ensure("One handler invocation for request", mHandlerCalls == url_limit);

		// Okay, request a shutdown of the servicing thread
		mStatus = HttpStatus();
		mHandlerCalls = 0;
		HttpHandle handle = req->requestStopThread(handlerp);
		ensure("Valid handle returned for second request", handle != LLCORE_HTTP_HANDLE_INVALID);
	
		// Run the notification pump again
		count = 0;
		limit = LOOP_COUNT_LONG;
		while (count++ < limit && mHandlerCalls < 1)
		{
			req->update(1000000);
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Second request executed in reasonable time", count < limit);
		ensure("Second handler invocation", mHandlerCalls == 1);

		// See that we actually shutdown the thread
		count = 0;
		limit = LOOP_COUNT_SHORT;
		while (count++ < limit && ! HttpService::isStopped())
		{
			usleep(LOOP_SLEEP_INTERVAL);
		}
		ensure("Thread actually stopped running", HttpService::isStopped());

		// release options
        opts.reset();
		
		// release the request object
		delete req;
		req = NULL;

		// Shut down service
		HttpRequest::destroyService();

#if 0 // defined(WIN32)
		// Can't do this on any platform anymore, the LL logging system holds
		// on to memory and produces what looks like memory leaks...

		// printf("Old mem:  %d, New mem:  %d\n", mMemTotal, GetMemTotal());
		ensure("Memory usage back to that at entry", mMemTotal == GetMemTotal());
#endif
	}
	catch (...)
	{
		stop_thread(req);
        opts.reset();
		delete req;
		HttpRequest::destroyService();
		throw;
	}
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
