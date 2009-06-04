/** 
 * @file 
 * @brief 
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
 * 
 * The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
 * this source code is governed by the Linden Lab Source Code Disclosure
 * Agreement ("Agreement") previously entered between you and Linden
 * Lab. By accessing, using, copying, modifying or distributing this
 * software, you acknowledge that you have been informed of your
 * obligations under the Agreement and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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


