/** 
 * @file llsrv.cpp
 * @brief Wrapper for DNS SRV record lookups
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

#include "llviewerprecompiledheaders.h"

#include "llsrv.h"
#include "llares.h"

struct Responder : public LLAres::UriRewriteResponder
{
    std::vector<std::string> mUris;
    void rewriteResult(const std::vector<std::string> &uris) {
        for (size_t i = 0; i < uris.size(); i++)
        {
            LL_INFOS() << "[" << i << "] " << uris[i] << LL_ENDL;
        }
        mUris = uris;
    }
};

std::vector<std::string> LLSRV::rewriteURI(const std::string& uri)
{
    LLPointer<Responder> resp = new Responder;

    gAres->rewriteURI(uri, resp);
    gAres->processAll();

    // It's been observed in deployment that c-ares can return control
    // to us without firing all of our callbacks, in which case the
    // returned vector will be empty, instead of a singleton as we
    // might wish.

    if (!resp->mUris.empty())
    {
        return resp->mUris;
    }

    std::vector<std::string> uris;
    uris.push_back(uri);
    return uris;
}
