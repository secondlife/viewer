/** 
 * @file 
 * @brief 
 *
 * $LicenseInfo:firstyear=2008&license=internal$
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

/* Macro Definitions */
#ifndef LL_LLMOCKHTTPCLIENT_H
#define LL_LLMOCKHTTPCLIENT_H

#include "linden_common.h"
#include "llhttpclientinterface.h"

#include <gmock/gmock.h>

class LLMockHTTPClient : public LLHTTPClientInterface
{
public:
  MOCK_METHOD2(get, void(const std::string& url, LLCurl::ResponderPtr responder));
  MOCK_METHOD3(get, void(const std::string& url, LLCurl::ResponderPtr responder, const LLSD& headers));
  MOCK_METHOD3(put, void(const std::string& url, const LLSD& body, LLCurl::ResponderPtr responder));
};

// A helper to match responder types
template<typename T>
struct ResponderType
{
	bool operator()(LLCurl::ResponderPtr ptr) const
	{
		T* p = dynamic_cast<T*>(ptr.get());
		return p != NULL;
	}
};

inline bool operator==(const LLSD& l, const LLSD& r)
{
	std::ostringstream ls, rs;
	ls << l;
	rs << r;
	return ls.str() == rs.str();

}


#endif //LL_LLMOCKHTTPCLIENT_H

