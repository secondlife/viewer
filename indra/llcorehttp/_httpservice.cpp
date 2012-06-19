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
#include "_httpinternal.h"

#include "lltimer.h"
#include "llthread.h"


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
	
	delete mPolicy;
	mPolicy = NULL;

	if (mThread)
	{
		mThread->release();
		mThread = NULL;
	}
}
	

void HttpService::init(HttpRequestQueue * queue)
{
	llassert_always(! sInstance);
	llassert_always(NOT_INITIALIZED == sState);
	sInstance = new HttpService();

	queue->addRef();
	sInstance->mRequestQueue = queue;
	sInstance->mPolicy = new HttpPolicy(sInstance);
	sInstance->mTransport = new HttpLibcurl(sInstance);
	sState = INITIALIZED;
}


void HttpService::term()
{
	llassert_always(RUNNING != sState);
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
	llassert_always(! mThread || STOPPED == sState);
	llassert_always(INITIALIZED == sState || STOPPED == sState);

	if (mThread)
	{
		mThread->release();
	}

	// Push current policy definitions
	mPolicy->setPolicies(mPolicyGlobal);
	
	mThread = new LLCoreInt::HttpThread(boost::bind(&HttpService::threadRun, this, _1));
	mThread->addRef();		// Need an explicit reference, implicit one is used internally
	sState = RUNNING;
}


void HttpService::stopRequested()
{
	mExitRequested = true;
}

bool HttpService::changePriority(HttpHandle handle, HttpRequest::priority_t priority)
{
	bool found(false);

	// Skip the request queue as we currently don't leave earlier
	// requests sitting there.  Start with the ready queue...
	found = mPolicy->changePriority(handle, priority);

	// If not there, we could try the transport/active queue but priority
	// doesn't really have much effect there so we don't waste cycles.
	
	return found;
}


void HttpService::shutdown()
{
	// *FIXME:  Run down everything....
}


// Working thread loop-forever method.  Gives time to
// each of the request queue, policy layer and transport
// layer pieces and then either sleeps for a small time
// or waits for a request to come in.  Repeats until
// requested to stop.
void HttpService::threadRun(LLCoreInt::HttpThread * thread)
{
	boost::this_thread::disable_interruption di;

	LLThread::registerThreadID();
	
	ELoopSpeed loop(REQUEST_SLEEP);
	while (! mExitRequested)
	{
		loop = processRequestQueue(loop);

		// Process ready queue issuing new requests as needed
		ELoopSpeed new_loop = mPolicy->processReadyQueue();
		loop = (std::min)(loop, new_loop);
		
		// Give libcurl some cycles
		new_loop = mTransport->processTransport();
		loop = (std::min)(loop, new_loop);
		
		// Determine whether to spin, sleep briefly or sleep for next request
		if (REQUEST_SLEEP != loop)
		{
			ms_sleep(LOOP_SLEEP_NORMAL_MS);
		}
	}

	shutdown();
	sState = STOPPED;
}


HttpService::ELoopSpeed HttpService::processRequestQueue(ELoopSpeed loop)
{
	HttpRequestQueue::OpContainer ops;
	const bool wait_for_req(REQUEST_SLEEP == loop);
	
	mRequestQueue->fetchAll(wait_for_req, ops);
	while (! ops.empty())
	{
		HttpOperation * op(ops.front());
		ops.erase(ops.begin());

		// Process operation
		if (! mExitRequested)
		{
			// Setup for subsequent tracing
			long tracing(TRACE_OFF);
			mPolicy->getGlobalOptions().get(HttpRequest::GP_TRACE, &tracing);
			op->mTracing = (std::max)(op->mTracing, int(tracing));

			if (op->mTracing > TRACE_OFF)
			{
				LL_INFOS("CoreHttp") << "TRACE, FromRequestQueue, Handle:  "
									 << static_cast<HttpHandle>(op)
									 << LL_ENDL;
			}

			// Stage
			op->stageFromRequest(this);
		}
				
		// Done with operation
		op->release();
	}

	// Queue emptied, allow polling loop to sleep
	return REQUEST_SLEEP;
}


}  // end namespace LLCore
