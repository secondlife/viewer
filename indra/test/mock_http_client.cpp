/** 
 * @file mock_http_client.cpp
 * @brief Framework for testing HTTP requests
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "linden_common.h"
#include "llhttpnode.h"

namespace tut
{
    class SuccessNode : public LLHTTPNode
    {
    public:
        void get(ResponsePtr r, const LLSD& context) const
        { 
            LLSD result;
            result["state"] = "complete";
            result["test"] = "test";
            r->result(result);
        }
        void post(ResponsePtr r, const LLSD& context, const LLSD& input) const
        {
            LLSD result;
            result["state"] = "complete";
            result["test"] = "test";
            r->result(result);
        }
    };

    class ErrorNode : public LLHTTPNode
    {
    public:
        void get(ResponsePtr r, const LLSD& context) const
            { r->status(599, "Intentional error"); }
        void post(ResponsePtr r, const LLSD& context, const LLSD& input) const
            { r->status(input["status"], input["reason"]); }
    };

    class TimeOutNode : public LLHTTPNode
    {
    public:
        void get(ResponsePtr r, const LLSD& context) const
        {
            /* do nothing, the request will eventually time out */ 
        }
    };

    LLSD storage;
    
    class LLSDStorageNode : public LLHTTPNode
    {
    public:
        LLSD get() const{ return storage; }
        LLSD put(const LLSD& value) const{ storage = value; return LLSD(); }
    };

    LLHTTPRegistration<LLSDStorageNode> gStorageNode("/test/storage");
    LLHTTPRegistration<SuccessNode>     gSuccessNode("/test/success");
    LLHTTPRegistration<ErrorNode>       gErrorNode("/test/error");
    LLHTTPRegistration<TimeOutNode>     gTimeOutNode("/test/timeout");
}
