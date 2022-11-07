/**
 * @file _httpopsetget.cpp
 * @brief Definitions for internal class HttpOpSetGet
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

#include "_httpopsetget.h"

#include "httpcommon.h"
#include "httprequest.h"

#include "_httpservice.h"
#include "_httppolicy.h"


namespace LLCore
{


// ==================================
// HttpOpSetget
// ==================================


HttpOpSetGet::HttpOpSetGet()
    : HttpOperation(),
      mReqOption(HttpRequest::PO_CONNECTION_LIMIT),
      mReqClass(HttpRequest::INVALID_POLICY_ID),
      mReqDoSet(false),
      mReqLongValue(0L),
      mReplyLongValue(0L)
{}


HttpOpSetGet::~HttpOpSetGet()
{}


HttpStatus HttpOpSetGet::setupGet(HttpRequest::EPolicyOption opt, HttpRequest::policy_t pclass)
{
    HttpStatus status;
    
    mReqOption = opt;
    mReqClass = pclass;
    return status;
}


HttpStatus HttpOpSetGet::setupSet(HttpRequest::EPolicyOption opt, HttpRequest::policy_t pclass, long value)
{
    HttpStatus status;

    if (! HttpService::sOptionDesc[opt].mIsLong)
    {
        return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
    }
    if (! HttpService::sOptionDesc[opt].mIsDynamic)
    {
        return HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_DYNAMIC);
    }
    
    mReqOption = opt;
    mReqClass = pclass;
    mReqDoSet = true;
    mReqLongValue = value;
    
    return status;
}


HttpStatus HttpOpSetGet::setupSet(HttpRequest::EPolicyOption opt, HttpRequest::policy_t pclass, const std::string & value)
{
    HttpStatus status;

    if (HttpService::sOptionDesc[opt].mIsLong)
    {
        return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
    }
    if (! HttpService::sOptionDesc[opt].mIsDynamic)
    {
        return HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_DYNAMIC);
    }

    mReqOption = opt;
    mReqClass = pclass;
    mReqDoSet = true;
    mReqStrValue = value;
    
    return status;
}


void HttpOpSetGet::stageFromRequest(HttpService * service)
{
    if (mReqDoSet)
    {
        if (HttpService::sOptionDesc[mReqOption].mIsLong)
        {
            mStatus = service->setPolicyOption(mReqOption, mReqClass,
                                               mReqLongValue, &mReplyLongValue);
        }
        else
        {
            mStatus = service->setPolicyOption(mReqOption, mReqClass,
                                               mReqStrValue, &mReplyStrValue);
        }
    }
    else
    {
        if (HttpService::sOptionDesc[mReqOption].mIsLong)
        {
            mStatus = service->getPolicyOption(mReqOption, mReqClass, &mReplyLongValue);
        }
        else
        {
            mStatus = service->getPolicyOption(mReqOption, mReqClass, &mReplyStrValue);
        }
    }
    
    addAsReply();
}


}   // end namespace LLCore

        
