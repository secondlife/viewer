/**
 * @file _httpopsetget.h
 * @brief Internal declarations for the HttpOpSetGet subclass
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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

#ifndef _LLCORE_HTTP_OPSETGET_H_
#define _LLCORE_HTTP_OPSETGET_H_


#include "linden_common.h"      // Modifies curl/curl.h interfaces

#include "httpcommon.h"

#include <curl/curl.h>

#include "_httpoperation.h"
#include "_refcounted.h"


namespace LLCore
{


/// HttpOpSetGet requests dynamic changes to policy and
/// configuration settings.
///
/// *NOTE:  Expect this to change.  Don't really like it yet.
///
/// *TODO:  Can't return values to caller yet.  Need to do
/// something better with HttpResponse and visitNotifier().
///
class HttpOpSetGet : public HttpOperation
{
public:
    typedef std::shared_ptr<HttpOpSetGet> ptr_t;

    HttpOpSetGet();

    virtual ~HttpOpSetGet();                            // Use release()

private:
    HttpOpSetGet(const HttpOpSetGet &);                 // Not defined
    void operator=(const HttpOpSetGet &);               // Not defined

public:
    /// Threading:  called by application thread
    HttpStatus setupGet(HttpRequest::EPolicyOption opt, HttpRequest::policy_t pclass);
    HttpStatus setupSet(HttpRequest::EPolicyOption opt, HttpRequest::policy_t pclass, long value);
    HttpStatus setupSet(HttpRequest::EPolicyOption opt, HttpRequest::policy_t pclass, const std::string & value);

    virtual void stageFromRequest(HttpService *);

public:
    // Request data
    HttpRequest::EPolicyOption  mReqOption;
    HttpRequest::policy_t       mReqClass;
    bool                        mReqDoSet;
    long                        mReqLongValue;
    std::string                 mReqStrValue;

    // Reply Data
    long                        mReplyLongValue;
    std::string                 mReplyStrValue;
};  // end class HttpOpSetGet


}   // end namespace LLCore

#endif  // _LLCORE_HTTP_OPSETGET_H_

