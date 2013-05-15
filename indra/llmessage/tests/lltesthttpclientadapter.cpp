/** 
 * @file 
 * @brief 
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
#include "lltesthttpclientadapter.h"

LLTestHTTPClientAdapter::LLTestHTTPClientAdapter()
{
}

LLTestHTTPClientAdapter::~LLTestHTTPClientAdapter()
{
}

void LLTestHTTPClientAdapter::get(const std::string& url, LLCurl::ResponderPtr responder)
{
	mGetUrl.push_back(url);
	mGetResponder.push_back(responder);
}

void LLTestHTTPClientAdapter::put(const std::string& url, const LLSD& body, LLCurl::ResponderPtr responder)
{
	mPutUrl.push_back(url);
	mPutBody.push_back(body);
	mPutResponder.push_back(responder);
}

U32 LLTestHTTPClientAdapter::putCalls() const 
{ 
	return mPutUrl.size(); 
}

void LLTestHTTPClientAdapter::get(const std::string& url, LLCurl::ResponderPtr responder, const LLSD& headers)
{
	mGetUrl.push_back(url);
	mGetHeaders.push_back(headers);
	mGetResponder.push_back(responder);
}


