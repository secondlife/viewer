/**
 * @file   lleventapi.cpp
 * @author Nat Goodspeed
 * @date   2009-11-10
 * @brief  Implementation for lleventapi.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

// Precompiled header
#include "linden_common.h"
// associated header
#include "lleventapi.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llerror.h"

LLEventAPI::LLEventAPI(const std::string& name, const std::string& desc, const std::string& field):
    lbase(name, field),
    ibase(name),
    mDesc(desc)
{
}

LLEventAPI::~LLEventAPI()
{
}

LLEventAPI::Response::Response(const LLSD& seed, const LLSD& request, const LLSD::String& replyKey):
    mResp(seed),
    mReq(request),
    mKey(replyKey)
{}

LLEventAPI::Response::~Response()
{
    // When you instantiate a stack Response object, if the original
    // request requested a reply, send it when we leave this block, no
    // matter how.
    sendReply(mResp, mReq, mKey);
}

void LLEventAPI::Response::warn(const std::string& warning)
{
    LL_WARNS("LLEventAPI::Response") << warning << LL_ENDL;
    mResp["warnings"].append(warning);
}

void LLEventAPI::Response::error(const std::string& error)
{
    // Use LL_WARNS rather than LL_ERROR: we don't want the viewer to shut
    // down altogether.
    LL_WARNS("LLEventAPI::Response") << error << LL_ENDL;

    mResp["error"] = error;
}
