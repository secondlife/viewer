/**
 * @file httprequest.cpp
 * @brief Implementation of the HTTPRequest class
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
#include "httpstats.h"

namespace
{

bool has_inited(false);

}

namespace LLCore
{

// ====================================
// HttpRequest Implementation
// ====================================


HttpRequest::HttpRequest()
    : mReplyQueue(),
      mRequestQueue(NULL)
{
    mRequestQueue = HttpRequestQueue::instanceOf();
    mRequestQueue->addRef();

    mReplyQueue.reset( new HttpReplyQueue() );

    HTTPStats::instance().recordHTTPRequest();
}


HttpRequest::~HttpRequest()
{
    if (mRequestQueue)
    {
        mRequestQueue->release();
        mRequestQueue = NULL;
    }

    mReplyQueue.reset();
}


// ====================================
// Policy Methods
// ====================================


HttpRequest::policy_t HttpRequest::createPolicyClass()
{
    if (HttpService::RUNNING == HttpService::instanceOf()->getState())
    {
        return 0;
    }
    return HttpService::instanceOf()->createPolicyClass();
}


HttpStatus HttpRequest::setStaticPolicyOption(EPolicyOption opt, policy_t pclass,
                                              long value, long * ret_value)
{
    if (HttpService::RUNNING == HttpService::instanceOf()->getState())
    {
        return HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_DYNAMIC);
    }
    return HttpService::instanceOf()->setPolicyOption(opt, pclass, value, ret_value);
}


HttpStatus HttpRequest::setStaticPolicyOption(EPolicyOption opt, policy_t pclass,
                                              const std::string & value, std::string * ret_value)
{
    if (HttpService::RUNNING == HttpService::instanceOf()->getState())
    {
        return HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_DYNAMIC);
    }
    return HttpService::instanceOf()->setPolicyOption(opt, pclass, value, ret_value);
}

HttpStatus HttpRequest::setStaticPolicyOption(EPolicyOption opt, policy_t pclass, policyCallback_t value, policyCallback_t * ret_value)
{
    if (HttpService::RUNNING == HttpService::instanceOf()->getState())
    {
        return HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_DYNAMIC);
    }

    return HttpService::instanceOf()->setPolicyOption(opt, pclass, value, ret_value);
}

HttpHandle HttpRequest::setPolicyOption(EPolicyOption opt, policy_t pclass,
                                        long value, HttpHandler::ptr_t handler)
{
    HttpStatus status;

    HttpOpSetGet::ptr_t op(new HttpOpSetGet());
    if (! (status = op->setupSet(opt, pclass, value)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    
    mLastReqStatus = status;
    return op->getHandle();
}


HttpHandle HttpRequest::setPolicyOption(EPolicyOption opt, policy_t pclass,
                                        const std::string & value, HttpHandler::ptr_t handler)
{
    HttpStatus status;

    HttpOpSetGet::ptr_t op (new HttpOpSetGet());
    if (! (status = op->setupSet(opt, pclass, value)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    
    mLastReqStatus = status;
    return op->getHandle();
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
                                   const HttpOptions::ptr_t & options,
                                   const HttpHeaders::ptr_t & headers,
                                   HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOpRequest::ptr_t op(new HttpOpRequest());
    if (! (status = op->setupGet(policy_id, priority, url, options, headers)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, user_handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    
    mLastReqStatus = status;
    return op->getHandle();
}


HttpHandle HttpRequest::requestGetByteRange(policy_t policy_id,
                                            priority_t priority,
                                            const std::string & url,
                                            size_t offset,
                                            size_t len,
                                            const HttpOptions::ptr_t & options,
                                            const HttpHeaders::ptr_t & headers,
                                            HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOpRequest::ptr_t op(new HttpOpRequest());
    if (! (status = op->setupGetByteRange(policy_id, priority, url, offset, len, options, headers)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, user_handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    
    mLastReqStatus = status;
    return op->getHandle();
}


HttpHandle HttpRequest::requestPost(policy_t policy_id,
                                    priority_t priority,
                                    const std::string & url,
                                    BufferArray * body,
                                    const HttpOptions::ptr_t & options,
                                    const HttpHeaders::ptr_t & headers,
                                    HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOpRequest::ptr_t op(new HttpOpRequest());
    if (! (status = op->setupPost(policy_id, priority, url, body, options, headers)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, user_handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    
    mLastReqStatus = status;
    return op->getHandle();
}


HttpHandle HttpRequest::requestPut(policy_t policy_id,
                                   priority_t priority,
                                   const std::string & url,
                                   BufferArray * body,
                                   const HttpOptions::ptr_t & options,
                                   const HttpHeaders::ptr_t & headers,
                                   HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOpRequest::ptr_t op (new HttpOpRequest());
    if (! (status = op->setupPut(policy_id, priority, url, body, options, headers)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, user_handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    
    mLastReqStatus = status;
    return op->getHandle();
}

HttpHandle HttpRequest::requestDelete(policy_t policy_id,
    priority_t priority,
    const std::string & url,
    const HttpOptions::ptr_t & options,
    const HttpHeaders::ptr_t & headers,
    HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOpRequest::ptr_t op(new HttpOpRequest());
    if (!(status = op->setupDelete(policy_id, priority, url, options, headers)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, user_handler);
    if (!(status = mRequestQueue->addOp(op)))           // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }

    mLastReqStatus = status;
    return op->getHandle();
}

HttpHandle HttpRequest::requestPatch(policy_t policy_id,
    priority_t priority,
    const std::string & url,
    BufferArray * body,
    const HttpOptions::ptr_t & options,
    const HttpHeaders::ptr_t & headers,
    HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOpRequest::ptr_t op (new HttpOpRequest());
    if (!(status = op->setupPatch(policy_id, priority, url, body, options, headers)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, user_handler);
    if (!(status = mRequestQueue->addOp(op)))           // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }

    mLastReqStatus = status;
    return op->getHandle();
}

HttpHandle HttpRequest::requestCopy(policy_t policy_id,
    priority_t priority,
    const std::string & url,
    const HttpOptions::ptr_t & options,
    const HttpHeaders::ptr_t & headers,
    HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOpRequest::ptr_t op(new HttpOpRequest());
    if (!(status = op->setupCopy(policy_id, priority, url, options, headers)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, user_handler);
    if (!(status = mRequestQueue->addOp(op)))           // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }

    mLastReqStatus = status;
    return op->getHandle();

}

HttpHandle HttpRequest::requestMove(policy_t policy_id,
    priority_t priority,
    const std::string & url,
    const HttpOptions::ptr_t & options,
    const HttpHeaders::ptr_t & headers,
    HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOpRequest::ptr_t op (new HttpOpRequest());
    if (!(status = op->setupMove(policy_id, priority, url, options, headers)))
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }
    op->setReplyPath(mReplyQueue, user_handler);
    if (!(status = mRequestQueue->addOp(op)))           // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }

    mLastReqStatus = status;
    return op->getHandle();
}


HttpHandle HttpRequest::requestNoOp(HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOperation::ptr_t op (new HttpOpNull());
    op->setReplyPath(mReplyQueue, user_handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }

    mLastReqStatus = status;
    return op->getHandle();
}


HttpStatus HttpRequest::update(long usecs)
{
    HttpOperation::ptr_t op;
    
    if (usecs)
    {
        const HttpTime limit(totalTime() + HttpTime(usecs));
        while (limit >= totalTime() && (op = mReplyQueue->fetchOp()))
        {
            // Process operation
            op->visitNotifier(this);
        
            // We're done with the operation
            op.reset();
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
                op.reset();
                op.swap(*iter);
            
                // Process operation
                op->visitNotifier(this);
        
                // We're done with the operation
            }
        }
    }
    
    return HttpStatus();
}




// ====================================
// Request Management Methods
// ====================================

HttpHandle HttpRequest::requestCancel(HttpHandle request, HttpHandler::ptr_t user_handler)
{
    HttpStatus status;

    HttpOperation::ptr_t op(new HttpOpCancel(request));
    op->setReplyPath(mReplyQueue, user_handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }

    mLastReqStatus = status;
    return op->getHandle();
}


HttpHandle HttpRequest::requestSetPriority(HttpHandle request, priority_t priority,
                                           HttpHandler::ptr_t handler)
{
    HttpStatus status;

    HttpOperation::ptr_t op (new HttpOpSetPriority(request, priority));
    op->setReplyPath(mReplyQueue, handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return LLCORE_HTTP_HANDLE_INVALID;
    }

    mLastReqStatus = status;
    return op->getHandle();
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


HttpHandle HttpRequest::requestStopThread(HttpHandler::ptr_t user_handler)
{
    HttpStatus status;
    HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

    HttpOperation::ptr_t op(new HttpOpStop());
    op->setReplyPath(mReplyQueue, user_handler);
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return handle;
    }

    mLastReqStatus = status;
    handle = op->getHandle();

    return handle;
}


HttpHandle HttpRequest::requestSpin(int mode)
{
    HttpStatus status;
    HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

    HttpOperation::ptr_t op(new HttpOpSpin(mode));
    op->setReplyPath(mReplyQueue, HttpHandler::ptr_t());
    if (! (status = mRequestQueue->addOp(op)))          // transfers refcount
    {
        mLastReqStatus = status;
        return handle;
    }

    mLastReqStatus = status;
    handle = op->getHandle();

    return handle;
}


}   // end namespace LLCore

