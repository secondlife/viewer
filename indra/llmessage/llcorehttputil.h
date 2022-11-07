/** 
 * @file llcorehttputil.h
 * @date 2014-08-25
 * @brief Adapter and utility classes expanding the llcorehttp interfaces.
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#ifndef LL_LLCOREHTTPUTIL_H
#define LL_LLCOREHTTPUTIL_H

#include <string>

#include "httpcommon.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httpheaders.h"
#include "httpoptions.h"
#include "httphandler.h"
#include "llhttpconstants.h" // *TODO: move to llcorehttp
#include "bufferarray.h"
#include "bufferstream.h"
#include "llsd.h"
#include "llevents.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llassettype.h"
#include "lluuid.h"

///
/// The base llcorehttp library implements many HTTP idioms
/// used in the viewer but not all.  That library intentionally
/// avoids the use of LLSD and its conventions which aren't
/// universally applicable.  This module, using namespace
/// LLCoreHttpUtil, provides the additional helper functions
/// that support idiomatic LLSD transport via the newer
/// llcorehttp library.
///
namespace LLCoreHttpUtil
{
/// Allow access to to the property settings methods.
typedef boost::function<bool(const std::string &)> BoolSettingQuery_t;
typedef boost::function<void(const std::string &, bool, const std::string &)> BoolSettingUpdate_t;

void setPropertyMethods(BoolSettingQuery_t queryfn, BoolSettingUpdate_t updatefn);


extern const F32 HTTP_REQUEST_EXPIRY_SECS;

/// Attempt to convert a response object's contents to LLSD.
/// It is expected that the response body will be of non-zero
/// length on input but basic checks will be performed and
/// and error (false status) returned if there is no data.
/// If there is data but it cannot be successfully parsed,
/// an error is also returned.  If successfully parsed,
/// the output LLSD object, out_llsd, is written with the
/// result and true is returned.
///
/// @arg    response    Response object as returned in
///                     in an HttpHandler onCompleted() callback.
/// @arg    log         If true, LLSD parser will emit errors
///                     as LL_INFOS-level messages as it parses.
///                     Otherwise, it *should* be a quiet parse.
/// @arg    out_llsd    Output LLSD object written only upon
///                     successful parse of the response object.
///
/// @return             Returns true (and writes to out_llsd) if
///                     parse was successful.  False otherwise.
///
bool responseToLLSD(LLCore::HttpResponse * response,
                    bool log,
                    LLSD & out_llsd);

/// Create a std::string representation of a response object
/// suitable for logging.  Mainly intended for logging of
/// failures and debug information.  This won't be fast,
/// just adequate.
std::string responseToString(LLCore::HttpResponse * response);


/// Issue a standard HttpRequest::requestPost() call but using
/// and LLSD object as the request body.  Conventions are the
/// same as with that method.  Caller is expected to provide
/// an HttpHeaders object with a correct 'Content-Type:' header.
/// One will not be provided by this call.  You might look after
/// the 'Accept:' header as well.
///
/// @return             If request is successfully issued, the
///                     HttpHandle representing the request.
///                     On error, LLCORE_HTTP_HANDLE_INVALID
///                     is returned and caller can fetch detailed
///                     status with the getStatus() method on the
///                     request object.  In case of error, no
///                     request is queued and caller may need to
///                     perform additional cleanup such as freeing
///                     a now-useless HttpHandler object.
///
LLCore::HttpHandle requestPostWithLLSD(LLCore::HttpRequest * request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    const LLCore::HttpOptions::ptr_t &options,
    const LLCore::HttpHeaders::ptr_t &headers,
    const LLCore::HttpHandler::ptr_t &handler);

inline LLCore::HttpHandle requestPostWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    const LLCore::HttpOptions::ptr_t & options,
    const LLCore::HttpHeaders::ptr_t & headers,
    const LLCore::HttpHandler::ptr_t & handler)
{
    return requestPostWithLLSD(request.get(), policy_id, priority,
        url, body, options, headers, handler);
}

inline LLCore::HttpHandle requestPostWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    const LLCore::HttpHandler::ptr_t &handler)
{
    LLCore::HttpOptions::ptr_t options;
    LLCore::HttpHeaders::ptr_t headers;

    return requestPostWithLLSD(request.get(), policy_id, priority,
        url, body, options, headers, handler);
}


/// Issue a standard HttpRequest::requestPut() call but using
/// and LLSD object as the request body.  Conventions are the
/// same as with that method.  Caller is expected to provide
/// an HttpHeaders object with a correct 'Content-Type:' header.
/// One will not be provided by this call.
///
/// @return             If request is successfully issued, the
///                     HttpHandle representing the request.
///                     On error, LLCORE_HTTP_HANDLE_INVALID
///                     is returned and caller can fetch detailed
///                     status with the getStatus() method on the
///                     request object.  In case of error, no
///                     request is queued and caller may need to
///                     perform additional cleanup such as freeing
///                     a now-useless HttpHandler object.
///
LLCore::HttpHandle requestPutWithLLSD(LLCore::HttpRequest * request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    const LLCore::HttpOptions::ptr_t &options,
    const LLCore::HttpHeaders::ptr_t &headers,
    const LLCore::HttpHandler::ptr_t &handler);

inline LLCore::HttpHandle requestPutWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    const LLCore::HttpOptions::ptr_t & options,
    const LLCore::HttpHeaders::ptr_t & headers,
    LLCore::HttpHandler::ptr_t handler)
{
    return requestPutWithLLSD(request.get(), policy_id, priority,
        url, body, options, headers, handler);
}

inline LLCore::HttpHandle requestPutWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    LLCore::HttpHandler::ptr_t handler)
{
    LLCore::HttpOptions::ptr_t options;
    LLCore::HttpHeaders::ptr_t headers;

    return requestPutWithLLSD(request.get(), policy_id, priority,
        url, body, options, headers, handler);
}

/// Issue a standard HttpRequest::requestPatch() call but using
/// and LLSD object as the request body.  Conventions are the
/// same as with that method.  Caller is expected to provide
/// an HttpHeaders object with a correct 'Content-Type:' header.
/// One will not be provided by this call.
///
/// @return             If request is successfully issued, the
///                     HttpHandle representing the request.
///                     On error, LLCORE_HTTP_HANDLE_INVALID
///                     is returned and caller can fetch detailed
///                     status with the getStatus() method on the
///                     request object.  In case of error, no
///                     request is queued and caller may need to
///                     perform additional cleanup such as freeing
///                     a now-useless HttpHandler object.
///
LLCore::HttpHandle requestPatchWithLLSD(LLCore::HttpRequest * request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    const LLCore::HttpOptions::ptr_t &options,
    const LLCore::HttpHeaders::ptr_t &headers,
    const LLCore::HttpHandler::ptr_t &handler);

inline LLCore::HttpHandle requestPatchWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    const LLCore::HttpOptions::ptr_t & options,
    const LLCore::HttpHeaders::ptr_t & headers,
    const LLCore::HttpHandler::ptr_t & handler)
{
    return requestPatchWithLLSD(request.get(), policy_id, priority,
        url, body, options, headers, handler);
}

inline LLCore::HttpHandle requestPatchWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    const LLCore::HttpHandler::ptr_t &handler)
{
    LLCore::HttpOptions::ptr_t options;
    LLCore::HttpHeaders::ptr_t headers;

    return requestPatchWithLLSD(request.get(), policy_id, priority,
        url, body, options, headers, handler);
}

//=========================================================================
/// The HttpCoroHandler is a specialization of the LLCore::HttpHandler for 
/// interacting with coroutines. When the request is completed the response 
/// will be posted onto the supplied Event Pump.
/// 
/// The LLSD posted back to the coroutine will have the following additions:
/// llsd["http_result"] -+- ["message"] - An error message returned from the HTTP status
///                      +- ["status"]  - The status code associated with the HTTP call
///                      +- ["success"] - Success of failure of the HTTP call and LLSD parsing.
///                      +- ["type"]    - The LLCore::HttpStatus type associted with the HTTP call
///                      +- ["url"]     - The URL used to make the call.
///                      +- ["headers"] - A map of name name value pairs with the HTTP headers.
///                      
class HttpCoroHandler : public LLCore::HttpHandler
{
public:

    typedef boost::shared_ptr<HttpCoroHandler>  ptr_t;
    typedef boost::weak_ptr<HttpCoroHandler>    wptr_t;

    HttpCoroHandler(LLEventStream &reply);

    static void writeStatusCodes(LLCore::HttpStatus status, const std::string &url, LLSD &result);

    virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

    inline LLEventStream &getReplyPump()
    {
        return mReplyPump;
    }

protected:
    /// this method may modify the status value
    virtual LLSD handleSuccess(LLCore::HttpResponse * response, LLCore::HttpStatus &status) = 0;
    virtual LLSD parseBody(LLCore::HttpResponse *response, bool &success) = 0;

private:
    void buildStatusEntry(LLCore::HttpResponse *response, LLCore::HttpStatus status, LLSD &result);

    LLEventStream &mReplyPump;
};

//=========================================================================
/// An adapter to handle some of the boilerplate code surrounding HTTP and coroutine 
/// interaction.
/// 
/// Construct an HttpCoroutineAdapter giving it a name and policy Id. After 
/// any application specific setup call the post, put or get method.  The request 
/// will be automatically pumped and the method will return with an LLSD describing
/// the result of the operation.  See HttpCoroHandler for a description of the 
/// decoration done to the returned LLSD.
/// 
/// Posting through the adapter will automatically add the following headers to 
/// the request if they have not been previously specified in a supplied 
/// HttpHeaders object:
///     "Accept=application/llsd+xml"
///     "X-SecondLife-UDP-Listen-Port=###"
///     
class HttpCoroutineAdapter
{
public:
    static const std::string HTTP_RESULTS;
    static const std::string HTTP_RESULTS_SUCCESS;
    static const std::string HTTP_RESULTS_TYPE;
    static const std::string HTTP_RESULTS_STATUS;
    static const std::string HTTP_RESULTS_MESSAGE;
    static const std::string HTTP_RESULTS_URL;
    static const std::string HTTP_RESULTS_HEADERS;
    static const std::string HTTP_RESULTS_CONTENT;
    static const std::string HTTP_RESULTS_RAW;

    typedef boost::shared_ptr<HttpCoroutineAdapter> ptr_t;
    typedef boost::weak_ptr<HttpCoroutineAdapter>   wptr_t;

    HttpCoroutineAdapter(const std::string &name, LLCore::HttpRequest::policy_t policyId,
        LLCore::HttpRequest::priority_t priority = 0L);
    ~HttpCoroutineAdapter();

    /// Execute a Post transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available. 
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    LLSD postAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD postAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, LLCore::BufferArray::ptr_t rawbody,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));

    LLSD postAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return postAndSuspend(request, url, body,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }

    LLSD postAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::BufferArray::ptr_t &rawbody,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return postAndSuspend(request, url, rawbody,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }

    LLSD postRawAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, LLCore::BufferArray::ptr_t rawbody,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));

    LLSD postRawAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::BufferArray::ptr_t &rawbody,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return postRawAndSuspend(request, url, rawbody,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }

    LLSD postFileAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, std::string fileName,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));

    LLSD postFileAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, std::string fileName,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return postFileAndSuspend(request, url, fileName,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }


    LLSD postFileAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, LLUUID assetId, LLAssetType::EType assetType,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));

    LLSD postFileAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, LLUUID assetId, LLAssetType::EType assetType,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return postFileAndSuspend(request, url, assetId, assetType,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }

    LLSD postJsonAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD postJsonAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return postJsonAndSuspend(request, url, body,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }



    /// Execute a Put transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available.
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    LLSD putAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));

    LLSD putAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, const LLSD & body,
        LLCore::HttpHeaders::ptr_t headers)
    {
        return putAndSuspend(request, url, body,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }

    LLSD putJsonAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD putJsonAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return putJsonAndSuspend(request, url, body,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }

    /// Execute a Get transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available.
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    /// 
    LLSD getAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD getAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::HttpHeaders::ptr_t &headers)
    {
        return getAndSuspend(request, url,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
            headers);
    }

    LLSD getRawAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD getRawAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::HttpHeaders::ptr_t &headers)
    {
        return getRawAndSuspend(request, url,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
            headers);
    }

    /// These methods have the same behavior as @getAndSuspend() however they are 
    /// expecting the server to return the results formatted in a JSON string. 
    /// On a successful GET call the JSON results will be converted into LLSD 
    /// before being returned to the caller.
    LLSD getJsonAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD getJsonAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::HttpHeaders::ptr_t &headers)
    {
        return getJsonAndSuspend(request, url,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
            headers);
    }


    /// Execute a DELETE transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available.
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    LLSD deleteAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD deleteAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, LLCore::HttpHeaders::ptr_t headers)
    {
        return deleteAndSuspend(request, url,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
            headers);
    }

    /// These methods have the same behavior as @deleteAndSuspend() however they are 
    /// expecting the server to return any results formatted in a JSON string. 
    /// On a successful DELETE call the JSON results will be converted into LLSD 
    /// before being returned to the caller.
    LLSD deleteJsonAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD deleteJsonAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, LLCore::HttpHeaders::ptr_t headers)
    {
        return deleteJsonAndSuspend(request, url,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
            headers);
    }


    /// Execute a PATCH transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available. 
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    LLSD patchAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD patchAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return patchAndSuspend(request, url, body,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }

    /// Execute a COPY transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available. 
    /// 
    /// @Note: The destination is passed through the HTTP pipe as a header
    /// The header used is defined as: HTTP_OUT_HEADER_DESTINATION("Destination");
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    LLSD copyAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, const std::string dest,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD copyAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const std::string & dest,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return copyAndSuspend(request, url, dest,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }

    /// Execute a MOVE transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available. 
    /// 
    /// @Note: The destination is passed through the HTTP pipe in the headers.
    /// The header used is defined as: HTTP_OUT_HEADER_DESTINATION("Destination");
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    LLSD moveAndSuspend(LLCore::HttpRequest::ptr_t request,
        const std::string & url, const std::string dest,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders()));
    LLSD moveAndSuspend(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const std::string & dest,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return moveAndSuspend(request, url, dest,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()), headers);
    }

    ///
    void cancelSuspendedOperation();

    static LLCore::HttpStatus getStatusFromLLSD(const LLSD &httpResults);

    /// The convenience routines below can be provided with callback functors 
    /// which will be invoked in the case of success or failure.  These callbacks
    /// should match this form.
    /// @sa callbackHttpGet
    /// @sa callbackHttpPost
    typedef boost::function<void(const LLSD &)> completionCallback_t;

    static void callbackHttpGet(const std::string &url, LLCore::HttpRequest::policy_t policyId, completionCallback_t success = NULL, completionCallback_t failure = NULL);
    static void callbackHttpGet(const std::string &url, completionCallback_t success = NULL, completionCallback_t failure = NULL)
    {
        callbackHttpGet(url, LLCore::HttpRequest::DEFAULT_POLICY_ID, success, failure);
    }
    static void callbackHttpPost(const std::string &url, LLCore::HttpRequest::policy_t policyId, const LLSD &postData, completionCallback_t success = NULL, completionCallback_t failure = NULL);
    static void callbackHttpPost(const std::string &url, const LLSD &postData, completionCallback_t success = NULL, completionCallback_t failure = NULL)
    {
        callbackHttpPost(url, LLCore::HttpRequest::DEFAULT_POLICY_ID, postData, success, failure);
    }

    /// Generic Get and post routines for HTTP via coroutines.
    /// These static methods do all required setup for the GET or POST operation.
    /// When the operation completes successfully they will put the success message in the log at INFO level, 
    /// If the operation fails the failure message is written to the log at WARN level.
    /// 
    static void messageHttpGet(const std::string &url, const std::string &success = std::string(), const std::string &failure = std::string());
    static void messageHttpPost(const std::string &url, const LLSD &postData, const std::string &success, const std::string &failure);


private:
    static LLSD buildImmediateErrorResult(const LLCore::HttpRequest::ptr_t &request, const std::string &url);

    void saveState(LLCore::HttpHandle yieldingHandle, LLCore::HttpRequest::ptr_t &request,
            HttpCoroHandler::ptr_t &handler);
    void cleanState();

    LLSD postAndSuspend_(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    LLSD postAndSuspend_(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::BufferArray::ptr_t &rawbody,
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    LLSD putAndSuspend_(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    LLSD putAndSuspend_(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLCore::BufferArray::ptr_t & rawbody,
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    LLSD getAndSuspend_(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::HttpOptions::ptr_t &options, 
        LLCore::HttpHeaders::ptr_t &headers, HttpCoroHandler::ptr_t &handler);

    LLSD deleteAndSuspend_(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::HttpOptions::ptr_t &options,
        LLCore::HttpHeaders::ptr_t &headers, HttpCoroHandler::ptr_t &handler);

    LLSD patchAndSuspend_(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    LLSD copyAndSuspend_(LLCore::HttpRequest::ptr_t &request,
        const std::string & url, 
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    LLSD moveAndSuspend_(LLCore::HttpRequest::ptr_t &request,
        const std::string & url,
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    static void trivialGetCoro(std::string url, LLCore::HttpRequest::policy_t policyId, completionCallback_t success, completionCallback_t failure);
    static void trivialPostCoro(std::string url, LLCore::HttpRequest::policy_t policyId, LLSD postData, completionCallback_t success, completionCallback_t failure);

    void checkDefaultHeaders(LLCore::HttpHeaders::ptr_t &headers);

    std::string                     mAdapterName;
    LLCore::HttpRequest::priority_t mPriority;
    LLCore::HttpRequest::policy_t   mPolicyId;

    LLCore::HttpHandle              mYieldingHandle;
    LLCore::HttpRequest::wptr_t     mWeakRequest;
    HttpCoroHandler::wptr_t         mWeakHandler;
};


} // end namespace LLCoreHttpUtil

#endif // LL_LLCOREHTTPUTIL_H
