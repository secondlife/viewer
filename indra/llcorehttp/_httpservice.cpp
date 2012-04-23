/**
 * @file _httpservice.cpp
 * @brief Internal definitions of the Http service thread
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

#include "_httpservice.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "_httpoperation.h"
#include "_httprequestqueue.h"
#include "_httppolicy.h"
#include "_httplibcurl.h"
#include "_thread.h"


namespace LLCore
{

HttpService * HttpService::sInstance(NULL);
volatile HttpService::EState HttpService::sState(NOT_INITIALIZED);

HttpService::HttpService()
	: mRequestQueue(NULL),
	  mExitRequested(false),
	  mThread(NULL),
	  mPolicy(NULL),
	  mTransport(NULL)
{
}


HttpService::~HttpService()
{
	if (mRequestQueue)
	{
		mRequestQueue->release();
		mRequestQueue = NULL;
	}

	if (mPolicy)
	{
		// *TODO:  need a finalization here
		;
	}
	
	if (mTransport)
	{
		// *TODO:  need a finalization here
		delete mTransport;
		mTransport = NULL;
	}
	
	if (mPolicy)
	{
		delete mPolicy;
		mPolicy = NULL;
	}

	if (mThread)
	{
		mThread->release();
		mThread = NULL;
	}
}
	

void HttpService::init(HttpRequestQueue * queue)
{
	LLINT_ASSERT(! sInstance);
	LLINT_ASSERT(NOT_INITIALIZED == sState);
	sInstance = new HttpService();

	queue->addRef();
	sInstance->mRequestQueue = queue;
	sInstance->mPolicy = new HttpPolicy(sInstance);
	sInstance->mTransport = new HttpLibcurl(sInstance);
	sState = INITIALIZED;
}


void HttpService::term()
{
	LLINT_ASSERT(RUNNING != sState);
	if (sInstance)
	{
		delete sInstance;
		sInstance = NULL;
	}
	sState = NOT_INITIALIZED;
}


bool HttpService::isStopped()
{
	// What is really wanted here is something like:
	//
	//     HttpService * service = instanceOf();
	//     return STOPPED == sState && (! service || ! service->mThread || ! service->mThread->joinable());
	//
	// But boost::thread is not giving me a consistent story on joinability
	// of a thread after it returns.  Debug and non-debug builds are showing
	// different behavior on Linux/Etch so we do a weaker test that may
	// not be globally correct (i.e. thread *is* stopping, may not have
	// stopped but will very soon):
	
	return STOPPED == sState;
}


void HttpService::startThread()
{
	LLINT_ASSERT(! mThread || STOPPED == sState);
	LLINT_ASSERT(INITIALIZED == sState || STOPPED == sState);

	if (mThread)
	{
		mThread->release();
	}
	mThread = new LLCoreInt::HttpThread(boost::bind(&HttpService::threadRun, this, _1));
	mThread->addRef();		// Need an explicit reference, implicit one is used internally
	sState = RUNNING;
}


void HttpService::stopRequested()
{
	mExitRequested = true;
}


void HttpService::shutdown()
{
	// *FIXME:  Run down everything....
}


void HttpService::threadRun(LLCoreInt::HttpThread * thread)
{
	boost::this_thread::disable_interruption di;

	while (! mExitRequested)
	{
		processRequestQueue();

		// Process ready queue issuing new requests as needed
		mPolicy->processReadyQueue();
		
		// Give libcurl some cycles
		mTransport->processTransport();
		
		// Determine whether to spin, sleep briefly or sleep for next request
		// *FIXME:  For now, do this
#if defined(WIN32)
		Sleep(50);
#else
		usleep(5000);
#endif
	}
	shutdown();
	sState = STOPPED;
}


void HttpService::processRequestQueue()
{
	HttpRequestQueue::OpContainer ops;

	mRequestQueue->fetchAll(false, ops);
	while (! ops.empty())
	{
		HttpOperation * op(ops.front());
		ops.erase(ops.begin());

		// Process operation
		if (! mExitRequested)
		{
			op->stageFromRequest(this);
		}
				
		// Done with operation
		op->release();
	}
}


}  // end namespace LLCore
