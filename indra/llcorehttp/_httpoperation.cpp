/**
 * @file _httpoperation.cpp
 * @brief Definitions for internal classes based on HttpOperation
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

#include "_httpoperation.h"

#include "httphandler.h"
#include "httpresponse.h"
#include "httprequest.h"

#include "_httprequestqueue.h"
#include "_httpreplyqueue.h"
#include "_httpservice.h"
#include "_httpinternal.h"

#include "lltimer.h"


namespace LLCore
{


// ==================================
// HttpOperation
// ==================================


HttpOperation::HttpOperation()
	: LLCoreInt::RefCounted(true),
	  mReplyQueue(NULL),
	  mUserHandler(NULL),
	  mReqPolicy(HttpRequest::DEFAULT_POLICY_ID),
	  mReqPriority(0U),
	  mTracing(0)
{
	mMetricCreated = totalTime();
}


HttpOperation::~HttpOperation()
{
	setReplyPath(NULL, NULL);
}


void HttpOperation::setReplyPath(HttpReplyQueue * reply_queue,
								 HttpHandler * user_handler)
{
	if (reply_queue != mReplyQueue)
	{
		if (mReplyQueue)
		{
			mReplyQueue->release();
		}

		if (reply_queue)
		{
			reply_queue->addRef();
		}

		mReplyQueue = reply_queue;
	}

	// Not refcounted
	mUserHandler = user_handler;
}



void HttpOperation::stageFromRequest(HttpService *)
{
	// Default implementation should never be called.  This
	// indicates an operation making a transition that isn't
	// defined.
	LL_ERRS("HttpCore") << "Default stageFromRequest method may not be called."
						<< LL_ENDL;
}


void HttpOperation::stageFromReady(HttpService *)
{
	// Default implementation should never be called.  This
	// indicates an operation making a transition that isn't
	// defined.
	LL_ERRS("HttpCore") << "Default stageFromReady method may not be called."
						<< LL_ENDL;
}


void HttpOperation::stageFromActive(HttpService *)
{
	// Default implementation should never be called.  This
	// indicates an operation making a transition that isn't
	// defined.
	LL_ERRS("HttpCore") << "Default stageFromActive method may not be called."
						<< LL_ENDL;
}


void HttpOperation::visitNotifier(HttpRequest *)
{
	if (mUserHandler)
	{
		HttpResponse * response = new HttpResponse();

		response->setStatus(mStatus);
		mUserHandler->onCompleted(static_cast<HttpHandle>(this), response);

		response->release();
	}
}


HttpStatus HttpOperation::cancel()
{
	HttpStatus status;

	return status;
}


void HttpOperation::addAsReply()
{
	if (mTracing > HTTP_TRACE_OFF)
	{
		LL_INFOS("CoreHttp") << "TRACE, ToReplyQueue, Handle:  "
							 << static_cast<HttpHandle>(this)
							 << LL_ENDL;
	}
	
	if (mReplyQueue)
	{
		addRef();
		mReplyQueue->addOp(this);
	}
}


// ==================================
// HttpOpStop
// ==================================


HttpOpStop::HttpOpStop()
	: HttpOperation()
{}


HttpOpStop::~HttpOpStop()
{}


void HttpOpStop::stageFromRequest(HttpService * service)
{
	// Do operations
	service->stopRequested();
	
	// Prepare response if needed
	addAsReply();
}


// ==================================
// HttpOpNull
// ==================================


HttpOpNull::HttpOpNull()
	: HttpOperation()
{}


HttpOpNull::~HttpOpNull()
{}


void HttpOpNull::stageFromRequest(HttpService * service)
{
	// Perform op
	// Nothing to perform.  This doesn't fall into the libcurl
	// ready/active queues, it just bounces over to the reply
	// queue directly.
	
	// Prepare response if needed
	addAsReply();
}


// ==================================
// HttpOpSpin
// ==================================


HttpOpSpin::HttpOpSpin(int mode)
	: HttpOperation(),
	  mMode(mode)
{}


HttpOpSpin::~HttpOpSpin()
{}


void HttpOpSpin::stageFromRequest(HttpService * service)
{
	if (0 == mMode)
	{
		// Spin forever
		while (true)
		{
			ms_sleep(100);
		}
	}
	else
	{
		ms_sleep(1);			// backoff interlock plumbing a bit
		this->addRef();
		if (! service->getRequestQueue().addOp(this))
		{
			this->release();
		}
	}
}


}   // end namespace LLCore
