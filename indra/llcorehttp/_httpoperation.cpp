/**
 * @file _httpoperation.cpp
 * @brief Definitions for internal classes based on HttpOperation
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2014, Linden Research, Inc.
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


namespace
{

static const char * const LOG_CORE("CoreHttp");

} // end anonymous namespace


namespace LLCore
{


// ==================================
// HttpOperation
// ==================================
/*static*/ 
HttpOperation::handleMap_t  HttpOperation::mHandleMap;
LLCoreInt::HttpMutex        HttpOperation::mOpMutex;

HttpOperation::HttpOperation():
    boost::enable_shared_from_this<HttpOperation>(),
    mReplyQueue(),
    mUserHandler(),
    mReqPolicy(HttpRequest::DEFAULT_POLICY_ID),
    mReqPriority(0U),
    mTracing(HTTP_TRACE_OFF),
    mMyHandle(LLCORE_HTTP_HANDLE_INVALID)
{
    mMetricCreated = totalTime();
}


HttpOperation::~HttpOperation()
{
    destroyHandle();
    mReplyQueue.reset();
    mUserHandler.reset();
}


void HttpOperation::setReplyPath(HttpReplyQueue::ptr_t reply_queue,
                                 HttpHandler::ptr_t user_handler)
{
    mReplyQueue.swap(reply_queue);
    mUserHandler.swap(user_handler);
}



void HttpOperation::stageFromRequest(HttpService *)
{
    // Default implementation should never be called.  This
    // indicates an operation making a transition that isn't
    // defined.
    LL_ERRS(LOG_CORE) << "Default stageFromRequest method may not be called."
                      << LL_ENDL;
}


void HttpOperation::stageFromReady(HttpService *)
{
    // Default implementation should never be called.  This
    // indicates an operation making a transition that isn't
    // defined.
    LL_ERRS(LOG_CORE) << "Default stageFromReady method may not be called."
                      << LL_ENDL;
}


void HttpOperation::stageFromActive(HttpService *)
{
    // Default implementation should never be called.  This
    // indicates an operation making a transition that isn't
    // defined.
    LL_ERRS(LOG_CORE) << "Default stageFromActive method may not be called."
                      << LL_ENDL;
}


void HttpOperation::visitNotifier(HttpRequest *)
{
    if (mUserHandler)
    {
        HttpResponse * response = new HttpResponse();

        response->setStatus(mStatus);
        mUserHandler->onCompleted(getHandle(), response);

        response->release();
    }
}


HttpStatus HttpOperation::cancel()
{
    HttpStatus status;

    return status;
}

// Handle methods
HttpHandle HttpOperation::getHandle()
{
    if (mMyHandle == LLCORE_HTTP_HANDLE_INVALID)
        return createHandle();

    return mMyHandle;
}

HttpHandle HttpOperation::createHandle()
{
    HttpHandle handle = static_cast<HttpHandle>(this);

    {
        LLCoreInt::HttpScopedLock lock(mOpMutex);

        mHandleMap[handle] = shared_from_this();
        mMyHandle = handle;
    }

    return mMyHandle;
}

void HttpOperation::destroyHandle()
{
    if (mMyHandle == LLCORE_HTTP_HANDLE_INVALID)
        return;
    {
        LLCoreInt::HttpScopedLock lock(mOpMutex);

        handleMap_t::iterator it = mHandleMap.find(mMyHandle);
        if (it != mHandleMap.end())
            mHandleMap.erase(it);
    }
}

/*static*/
HttpOperation::ptr_t HttpOperation::findByHandle(HttpHandle handle)
{
    wptr_t weak;

    if (!handle)
        return ptr_t();

    {
        LLCoreInt::HttpScopedLock lock(mOpMutex);

        handleMap_t::iterator it = mHandleMap.find(handle);
        if (it == mHandleMap.end())
        {
            LL_WARNS("LLCore::HTTP") << "Could not find operation for handle " << handle << LL_ENDL;
            return ptr_t();
        }

        weak = (*it).second;
    }

    if (!weak.expired())
        return weak.lock();
    
    return ptr_t();
}


void HttpOperation::addAsReply()
{
    if (mTracing > HTTP_TRACE_OFF)
    {
        LL_INFOS(LOG_CORE) << "TRACE, ToReplyQueue, Handle:  "
                           << getHandle()
                           << LL_ENDL;
    }
    
    if (mReplyQueue)
    {
        HttpOperation::ptr_t op = shared_from_this();
        mReplyQueue->addOp(op);
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
        ms_sleep(1);            // backoff interlock plumbing a bit
        HttpOperation::ptr_t opptr = shared_from_this();
        service->getRequestQueue().addOp(opptr);
    }
}


}   // end namespace LLCore
