/** 
 * @file 
 * @brief 
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

/* Macro Definitions */
#ifndef LL_LLTESTHTTPCLIENTADAPTER_H
#define LL_LLTESTHTTPCLIENTADAPTER_H


#include "linden_common.h"
#include "llhttpclientinterface.h"

class LLTestHTTPClientAdapter : public LLHTTPClientInterface
{
public:
	LLTestHTTPClientAdapter();
	virtual ~LLTestHTTPClientAdapter();
	virtual void get(const std::string& url, LLCurl::ResponderPtr responder);
	virtual void get(const std::string& url, LLCurl::ResponderPtr responder, const LLSD& headers);

	virtual void put(const std::string& url, const LLSD& body, LLCurl::ResponderPtr responder);
	U32 putCalls() const;

	std::vector<LLSD> mPutBody;
	std::vector<LLSD> mGetHeaders;
	std::vector<std::string> mPutUrl;
	std::vector<std::string> mGetUrl;
	std::vector<LLCurl::ResponderPtr> mPutResponder;
	std::vector<LLCurl::ResponderPtr> mGetResponder;
};



#endif //LL_LLSIMULATORPRESENCESENDER_H

