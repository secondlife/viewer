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
#include "_httppolicy.h"
#include "_httpoperation.h"
#include "_httpoprequest.h"
#include "_httpopsetpriority.h"
#include "_httpopcancel.h"
#include "_httpopsetget.h"

#include "lltimer.h"


namespace
{

bool has_inited(false);

}

namespace LLCore
{

// ====================================
// HttpRequest Implementation
// ====================================


HttpRequest::policy_t HttpRequest::sNextPolicyID(1);


HttpRequest::HttpRequest()
	: //HttpHandler(),
	  mReplyQueue(NULL),
	  mRequestQueue(NULL)
{
	mRequestQueue = HttpRequestQueue::instanceOf();
	mRequestQueue->addRef();

	mReplyQueue = new HttpReplyQueue();
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
}


// ====================================
// Policy Methods
// ====================================


HttpStatus HttpRequest::setPolicyGlobalOption(EGlobalPolicy opt, long value)
{
	if (HttpService::RUNNING == HttpService::instanceOf()->getState())
	{
		return HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_DYNAMIC);
	}
	return HttpService::instanceOf()->getGlobalOptions().set(opt, value);
}


HttpStatus HttpRequest::setPolicyGlobalOption(EGlobalPolicy opt, const std::string & value)
{
	if (HttpService::RUNNING == HttpService::instanceOf()->getState())
	{
		return HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_DYNAMIC);
	}
	return HttpService::instanceOf()->getGlobalOptions().set(opt, value);
}


HttpRequest::policy_t HttpRequest::createPolicyClass()
{
	if (HttpService::RUNNING == HttpService::instanceOf()->getState())
	{
		return 0;
	}
	return HttpService::instanceOf()->createPolicyClass();
}


HttpStatus HttpRequest::setPolicyClassOption(policy_t policy_id,
											 EClassPolicy opt,
											 long value)
{
	if (HttpService::RUNNING == HttpService::instanceOf()->getState())
	{
		return HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_DYNAMIC);
	}
	return HttpService::instanceOf()->getClassOptions(policy_id).set(opt, value);
}


// ====================================
// Request Methods
// ====================================


HttpStatus HttpRequest::getStatus() const
{
	return mLastReqStatus;
}


HttpHandle HttpRequest::requestGet(policy_t policy_id,
								   priority_t priority,
								   const std::string & url,
								   HttpOptions * options,
								   HttpHeaders * headers,
								   HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpRequest * op = new HttpOpRequest();
	if (! (status = op->setupGet(policy_id, priority, url, options, headers)))
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	op->setReplyPath(mReplyQueue, user_handler);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	
	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpHandle HttpRequest::requestGetByteRange(policy_t policy_id,
											priority_t priority,
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
	op->setReplyPath(mReplyQueue, user_handler);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	
	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpHandle HttpRequest::requestPost(policy_t policy_id,
									priority_t priority,
									const std::string & url,
									BufferArray * body,
									HttpOptions * options,
									HttpHeaders * headers,
									HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpRequest * op = new HttpOpRequest();
	if (! (status = op->setupPost(policy_id, priority, url, body, options, headers)))
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	op->setReplyPath(mReplyQueue, user_handler);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	
	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpHandle HttpRequest::requestPut(policy_t policy_id,
								   priority_t priority,
								   const std::string & url,
								   BufferArray * body,
								   HttpOptions * options,
								   HttpHeaders * headers,
								   HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpRequest * op = new HttpOpRequest();
	if (! (status = op->setupPut(policy_id, priority, url, body, options, headers)))
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	op->setReplyPath(mReplyQueue, user_handler);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	
	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpHandle HttpRequest::requestNoOp(HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpNull * op = new HttpOpNull();
	op->setReplyPath(mReplyQueue, user_handler);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}

	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpStatus HttpRequest::update(long usecs)
{
	HttpOperation * op(NULL);
	
	if (usecs)
	{
		const HttpTime limit(totalTime() + HttpTime(usecs));
		while (limit >= totalTime() && (op = mReplyQueue->fetchOp()))
		{
			// Process operation
			op->visitNotifier(this);
		
			// We're done with the operation
			op->release();
		}
	}
	else
	{
		// Same as above, just no time limit
		HttpReplyQueue::OpContainer replies;
		mReplyQueue->fetchAll(replies);
		if (! replies.empty())
		{
			for (HttpReplyQueue::OpContainer::iterator iter(replies.begin());
				 replies.end() != iter;
				 ++iter)
			{
				// Swap op pointer for NULL;
				op = *iter; *iter = NULL;	
			
				// Process operation
				op->visitNotifier(this);
		
				// We're done with the operation
				op->release();
			}
		}
	}
	
	return HttpStatus();
}




// ====================================
// Request Management Methods
// ====================================

HttpHandle HttpRequest::requestCancel(HttpHandle request, HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle ret_handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpCancel * op = new HttpOpCancel(request);
	op->setReplyPath(mReplyQueue, user_handler);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return ret_handle;
	}

	mLastReqStatus = status;
	ret_handle = static_cast<HttpHandle>(op);
	
	return ret_handle;
}


HttpHandle HttpRequest::requestSetPriority(HttpHandle request, priority_t priority,
										   HttpHandler * handler)
{
	HttpStatus status;
	HttpHandle ret_handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpSetPriority * op = new HttpOpSetPriority(request, priority);
	op->setReplyPath(mReplyQueue, handler);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return ret_handle;
	}

	mLastReqStatus = status;
	ret_handle = static_cast<HttpHandle>(op);
	
	return ret_handle;
}


// ====================================
// Utility Methods
// ====================================

HttpStatus HttpRequest::createService()
{
	HttpStatus status;

	if (! has_inited)
	{
		HttpRequestQueue::init();
		HttpRequestQueue * rq = HttpRequestQueue::instanceOf();
		HttpService::init(rq);
		has_inited = true;
	}
	
	return status;
}


HttpStatus HttpRequest::destroyService()
{
	HttpStatus status;

	if (has_inited)
	{
		HttpService::term();
		HttpRequestQueue::term();
		has_inited = false;
	}
	
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
	op->setReplyPath(mReplyQueue, user_handler);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}

	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);

	return handle;
}


HttpHandle HttpRequest::requestSpin(int mode)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpSpin * op = new HttpOpSpin(mode);
	op->setReplyPath(mReplyQueue, NULL);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}

	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);

	return handle;
}

// ====================================
// Dynamic Policy Methods
// ====================================

HttpHandle HttpRequest::requestSetHttpProxy(const std::string & proxy, HttpHandler * handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpSetGet * op = new HttpOpSetGet();
	op->setupSet(GP_HTTP_PROXY, proxy);
	op->setReplyPath(mReplyQueue, handler);
	if (! (status = mRequestQueue->addOp(op)))			// transfers refcount
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}

	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);

	return handle;
}


}   // end namespace LLCore

