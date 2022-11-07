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

