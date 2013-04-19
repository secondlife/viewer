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

