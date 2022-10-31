/**
 * @file llcurl_stub.cpp
 * @brief stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_CURL_STUB_CPP
#define LL_CURL_STUB_CPP


#include "linden_common.h"
#include "llcurl.h"
#include "llhttpconstants.cpp"

LLCurl::Responder::Responder()
{
}

void LLCurl::Responder::httpCompleted()
{
    if (isGoodStatus())
    {
        httpSuccess();
    }
    else
    {
        httpFailure();
    }
}

void LLCurl::Responder::completedRaw(LLChannelDescriptors const&,
                                     boost::shared_ptr<LLBufferArray> const&)
{
}

void LLCurl::Responder::httpFailure()
{
}

LLCurl::Responder::~Responder ()
{
}

void LLCurl::Responder::httpSuccess()
{
}

std::string LLCurl::Responder::dumpResponse() const
{
    return "dumpResponse()";
}

void LLCurl::Responder::successResult(const LLSD& content)
{
    setResult(HTTP_OK, "", content);
    httpSuccess();
}

void LLCurl::Responder::failureResult(S32 status, const std::string& reason, const LLSD& content)
{
    setResult(status, reason, content);
    httpFailure();
}


void LLCurl::Responder::completeResult(S32 status, const std::string& reason, const LLSD& content)
{
    setResult(status, reason, content);
    httpCompleted();
}

void LLCurl::Responder::setResult(S32 status, const std::string& reason, const LLSD& content /* = LLSD() */)
{
    mStatus = status;
    mReason = reason;
    mContent = content;
}

#endif
