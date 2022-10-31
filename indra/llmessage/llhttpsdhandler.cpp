/**
* @file llhttpsdhandler.h
* @brief Public-facing declarations for the HttpHandler class
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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
#include "llhttpconstants.h"

#include "llhttpsdhandler.h"
#include "httpresponse.h"
#include "httpheaders.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "bufferstream.h"
#include "llcorehttputil.h"

//========================================================================
LLHttpSDHandler::LLHttpSDHandler()
{
}

void LLHttpSDHandler::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
{
    LLCore::HttpStatus status = response->getStatus();

    if (!status)
    {
        this->onFailure(response, status);
    }
    else
    {
        LLSD resplsd;
        const bool emit_parse_errors = false;

        bool parsed = !((response->getBodySize() == 0) ||
            !LLCoreHttpUtil::responseToLLSD(response, emit_parse_errors, resplsd));

        if (!parsed) 
        {
            // Only emit a warning if we failed to parse when 'content-type' == 'application/llsd+xml'
            LLCore::HttpHeaders::ptr_t headers(response->getHeaders());
            const std::string *contentType = (headers) ? headers->find(HTTP_IN_HEADER_CONTENT_TYPE) : NULL;

            if (contentType && (HTTP_CONTENT_LLSD_XML == *contentType))
            {
                std::string thebody = LLCoreHttpUtil::responseToString(response);

                LL_WARNS() << "Failed to deserialize . " << response->getRequestURL() << " [status:" << response->getStatus().toString() << "] "
                    << " body: " << thebody << LL_ENDL;
            }
        }

        this->onSuccess(response, resplsd);
    }

}
