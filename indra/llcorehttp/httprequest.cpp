/**
 * @file httprequest.cpp
 * @brief Implementation of the HTTPRequest class
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

#include "httprequest.h"

#include "_httprequestqueue.h"
#include "_httpreplyqueue.h"
#include "_httpservice.h"
#include "_httpoperation.h"
#include "_httpoprequest.h"


namespace
{

bool has_inited(false);

}

namespace LLCore
{

// ====================================
// InternalHandler Implementation
// ====================================


class HttpRequest::InternalHandler : public HttpHandler
{
public:
	InternalHandler(HttpRequest & request)
		: mRequest(request)
		{}

protected:
	InternalHandler(const InternalHandler &);			// Not defined
	void operator=(const InternalHandler &);			// Not defined

public:
	void onCompleted(HttpHandle handle, HttpResponse * response)
		{
			HttpOperation * op(static_cast<HttpOperation *>(handle));
			HttpHandler * user_handler(op->getUserHandler());
			if (user_handler)
			{
				user_handler->onCompleted(handle, response);
			}
		}

protected:
	HttpRequest &		mRequest;
	
};  // end class HttpRequest::InternalHandler


// ====================================
// HttpRequest Implementation
// ====================================


unsigned int HttpRequest::sNextPolicyID(1);


HttpRequest::HttpRequest()
	: //HttpHandler(),
	  mReplyQueue(NULL),
	  mRequestQueue(NULL),
	  mSelfHandler(NULL)
{
	mRequestQueue = HttpRequestQueue::instanceOf();
	mRequestQueue->addRef();

	mReplyQueue = new HttpReplyQueue();

	mSelfHandler = new InternalHandler(*this);
}


HttpRequest::~HttpRequest()
{
	if (mRequestQueue)
	{
		mRequestQueue->release();
		mRequestQueue = NULL;
	}

	if (mReplyQueue)
	{
		mReplyQueue->release();
		mReplyQueue = NULL;
	}

	delete mSelfHandler;
	mSelfHandler = NULL;
}


// ====================================
// Policy Methods
// ====================================


HttpStatus HttpRequest::setPolicyGlobalOption(EGlobalPolicy opt, long value)
{
	HttpStatus status;

	return status;
}


unsigned int HttpRequest::createPolicyClass()
{
	unsigned int policy_id = 1;
	
	return policy_id;
}


HttpStatus HttpRequest::setPolicyClassOption(unsigned int policy_id,
											 EClassPolicy opt,
											 long value)
{
	HttpStatus status;

	return status;
}


// ====================================
// Request Methods
// ====================================


HttpStatus HttpRequest::getStatus() const
{
	return mLastReqStatus;
}


HttpHandle HttpRequest::requestGetByteRange(unsigned int policy_id,
											float priority,
											const std::string & url,
											size_t offset,
											size_t len,
											HttpOptions * options,
											HttpHeaders * headers,
											HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpRequest * op = new HttpOpRequest();
	if (! (status = op->setupGetByteRange(policy_id, priority, url, offset, len, options, headers)))
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	op->setHandlers(mReplyQueue, mSelfHandler, user_handler);
	mRequestQueue->addOp(op);			// transfers refcount
	
	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpHandle HttpRequest::requestNoOp(HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpNull * op = new HttpOpNull();
	op->setHandlers(mReplyQueue, mSelfHandler, user_handler);
	mRequestQueue->addOp(op);			// transfer refcount as well

	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpStatus HttpRequest::update(long millis)
{
	HttpStatus status;

	// *FIXME:  need timer stuff
	// long now(getNow());
	// long limit(now + millis);
	
	HttpOperation * op(NULL);
	while ((op = mReplyQueue->fetchOp()))
	{
		// Process operation
		op->visitNotifier(this);
		
		// We're done with the operation
		op->release();
	}
	
	return status;
}




// ====================================
// Request Management Methods
// ====================================


// ====================================
// Utility Methods
// ====================================

HttpStatus HttpRequest::createService()
{
	HttpStatus status;

	LLINT_ASSERT(! has_inited);
	HttpRequestQueue::init();
	HttpRequestQueue * rq = HttpRequestQueue::instanceOf();
	HttpService::init(rq);
	has_inited = true;
	
	return status;
}


HttpStatus HttpRequest::destroyService()
{
	HttpStatus status;

	LLINT_ASSERT(has_inited);
	HttpService::term();
	HttpRequestQueue::term();
	has_inited = false;
	
	return status;
}


HttpStatus HttpRequest::startThread()
{
	HttpStatus status;

	HttpService::instanceOf()->startThread();
	
	return status;
}


HttpHandle HttpRequest::requestStopThread(HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpStop * op = new HttpOpStop();
	op->setHandlers(mReplyQueue, mSelfHandler, user_handler);
	mRequestQueue->addOp(op);			// transfer refcount as well

	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);

	return handle;
}


}   // end namespace LLCore

