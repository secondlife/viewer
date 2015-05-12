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
#include "bufferarray.h"
#include "bufferstream.h"
#include "llsd.h"
#include "llevents.h"
#include "llcoros.h"
#include "lleventcoro.h"

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

/// Attempt to convert a response object's contents to LLSD.
/// It is expected that the response body will be of non-zero
/// length on input but basic checks will be performed and
/// and error (false status) returned if there is no data.
/// If there is data but it cannot be successfully parsed,
/// an error is also returned.  If successfully parsed,
/// the output LLSD object, out_llsd, is written with the
/// result and true is returned.
///
/// @arg	response	Response object as returned in
///						in an HttpHandler onCompleted() callback.
/// @arg	log			If true, LLSD parser will emit errors
///						as LL_INFOS-level messages as it parses.
///						Otherwise, it *should* be a quiet parse.
/// @arg	out_llsd	Output LLSD object written only upon
///						successful parse of the response object.
///
/// @return				Returns true (and writes to out_llsd) if
///						parse was successful.  False otherwise.
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
/// @return				If request is successfully issued, the
///						HttpHandle representing the request.
///						On error, LLCORE_HTTP_HANDLE_INVALID
///						is returned and caller can fetch detailed
///						status with the getStatus() method on the
///						request object.  In case of error, no
///						request is queued and caller may need to
///						perform additional cleanup such as freeing
///						a now-useless HttpHandler object.
///
LLCore::HttpHandle requestPostWithLLSD(LLCore::HttpRequest * request,
									   LLCore::HttpRequest::policy_t policy_id,
									   LLCore::HttpRequest::priority_t priority,
									   const std::string & url,
									   const LLSD & body,
									   LLCore::HttpOptions * options,
									   LLCore::HttpHeaders * headers,
									   LLCore::HttpHandler * handler);

inline LLCore::HttpHandle requestPostWithLLSD(LLCore::HttpRequest::ptr_t & request,
	LLCore::HttpRequest::policy_t policy_id,
	LLCore::HttpRequest::priority_t priority,
	const std::string & url,
	const LLSD & body,
	LLCore::HttpOptions::ptr_t & options,
	LLCore::HttpHeaders::ptr_t & headers,
	LLCore::HttpHandler * handler)
{
    return requestPostWithLLSD(request.get(), policy_id, priority,
        url, body, options.get(), headers.get(), handler);
}

inline LLCore::HttpHandle requestPostWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    LLCore::HttpHandler * handler)
{
    return requestPostWithLLSD(request.get(), policy_id, priority,
        url, body, NULL, NULL, handler);
}


/// Issue a standard HttpRequest::requestPut() call but using
/// and LLSD object as the request body.  Conventions are the
/// same as with that method.  Caller is expected to provide
/// an HttpHeaders object with a correct 'Content-Type:' header.
/// One will not be provided by this call.
///
/// @return				If request is successfully issued, the
///						HttpHandle representing the request.
///						On error, LLCORE_HTTP_HANDLE_INVALID
///						is returned and caller can fetch detailed
///						status with the getStatus() method on the
///						request object.  In case of error, no
///						request is queued and caller may need to
///						perform additional cleanup such as freeing
///						a now-useless HttpHandler object.
///
LLCore::HttpHandle requestPutWithLLSD(LLCore::HttpRequest * request,
	LLCore::HttpRequest::policy_t policy_id,
	LLCore::HttpRequest::priority_t priority,
	const std::string & url,
	const LLSD & body,
	LLCore::HttpOptions * options,
	LLCore::HttpHeaders * headers,
	LLCore::HttpHandler * handler);

inline LLCore::HttpHandle requestPutWithLLSD(LLCore::HttpRequest::ptr_t & request,
	LLCore::HttpRequest::policy_t policy_id,
	LLCore::HttpRequest::priority_t priority,
	const std::string & url,
	const LLSD & body,
	LLCore::HttpOptions::ptr_t & options,
	LLCore::HttpHeaders::ptr_t & headers,
	LLCore::HttpHandler * handler)
{
    return requestPutWithLLSD(request.get(), policy_id, priority,
        url, body, options.get(), headers.get(), handler);
}

inline LLCore::HttpHandle requestPutWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    LLCore::HttpHandler * handler)
{
    return requestPutWithLLSD(request.get(), policy_id, priority,
        url, body, NULL, NULL, handler);
}

/// Issue a standard HttpRequest::requestPatch() call but using
/// and LLSD object as the request body.  Conventions are the
/// same as with that method.  Caller is expected to provide
/// an HttpHeaders object with a correct 'Content-Type:' header.
/// One will not be provided by this call.
///
/// @return				If request is successfully issued, the
///						HttpHandle representing the request.
///						On error, LLCORE_HTTP_HANDLE_INVALID
///						is returned and caller can fetch detailed
///						status with the getStatus() method on the
///						request object.  In case of error, no
///						request is queued and caller may need to
///						perform additional cleanup such as freeing
///						a now-useless HttpHandler object.
///
LLCore::HttpHandle requestPatchWithLLSD(LLCore::HttpRequest * request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    LLCore::HttpOptions * options,
    LLCore::HttpHeaders * headers,
    LLCore::HttpHandler * handler);

inline LLCore::HttpHandle requestPatchWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    LLCore::HttpOptions::ptr_t & options,
    LLCore::HttpHeaders::ptr_t & headers,
    LLCore::HttpHandler * handler)
{
    return requestPatchWithLLSD(request.get(), policy_id, priority,
        url, body, options.get(), headers.get(), handler);
}

inline LLCore::HttpHandle requestPatchWithLLSD(LLCore::HttpRequest::ptr_t & request,
    LLCore::HttpRequest::policy_t policy_id,
    LLCore::HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    LLCore::HttpHandler * handler)
{
    return requestPatchWithLLSD(request.get(), policy_id, priority,
        url, body, NULL, NULL, handler);
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
    LLSD postAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders(), false));
    LLSD postAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
        const std::string & url, LLCore::BufferArray::ptr_t rawbody,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders(), false));
    LLSD postAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return postAndYield(self, request, url, body,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false), headers);
    }

    LLSD postAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::BufferArray::ptr_t &rawbody,
        LLCore::HttpHeaders::ptr_t &headers)
    {
        return postAndYield(self, request, url, rawbody,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false), headers);
    }

    /// Execute a Put transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available.
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    LLSD putAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders(), false));

    /// Execute a Get transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available.
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    LLSD getAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
        const std::string & url,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders(), false));
    LLSD getAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::HttpHeaders::ptr_t &headers)
    {
        return getAndYield(self, request, url,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false),
            headers);
    }

    LLSD getRawAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
        const std::string & url,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders(), false));
    LLSD getRawAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::HttpHeaders::ptr_t &headers)
    {
        return getRawAndYield(self, request, url,
            LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false),
            headers);
    }


    /// Execute a DELETE transaction on the supplied URL and yield execution of 
    /// the coroutine until a result is available.
    /// 
    /// @Note: the request's smart pointer is passed by value so that it will
    /// not be deallocated during the yield.
    LLSD deleteAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
        const std::string & url,
        LLCore::HttpOptions::ptr_t options = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false),
        LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders(), false));

    ///
    void cancelYieldingOperation();


    static LLCore::HttpStatus getStatusFromLLSD(const LLSD &httpResults);

    /// Generic Get and post routines for HTTP via coroutines.
    /// These static methods do all required setup for the GET or POST operation.
    /// When the operation completes successfully they will put the success message in the log at INFO level, 
    /// If the operation fails the failure message is written to the log at WARN level.
    /// 
    static void genericHttpGet(const std::string &url, const std::string &success = std::string(), const std::string &failure = std::string());
    static void genericHttpPost(const std::string &url, const LLSD &postData, const std::string &success, const std::string &failure);

private:
    static LLSD buildImmediateErrorResult(const LLCore::HttpRequest::ptr_t &request, const std::string &url);

    void saveState(LLCore::HttpHandle yieldingHandle, LLCore::HttpRequest::ptr_t &request,
            HttpCoroHandler::ptr_t &handler);
    void cleanState();

    LLSD postAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    LLSD postAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::BufferArray::ptr_t &rawbody,
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    LLSD putAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
        const std::string & url, const LLSD & body,
        LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
        HttpCoroHandler::ptr_t &handler);

    LLSD getAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::HttpOptions::ptr_t &options, 
        LLCore::HttpHeaders::ptr_t &headers, HttpCoroHandler::ptr_t &handler);

    LLSD deleteAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
        const std::string & url, LLCore::HttpOptions::ptr_t &options,
        LLCore::HttpHeaders::ptr_t &headers, HttpCoroHandler::ptr_t &handler);

    static void genericGetCoro(LLCoros::self& self, std::string &url, std::string success, std::string failure);
    static void genericPostCoro(LLCoros::self& self, std::string &url, LLSD postData, std::string success, std::string failure);

    std::string                     mAdapterName;
    LLCore::HttpRequest::priority_t mPriority;
    LLCore::HttpRequest::policy_t   mPolicyId;

    LLCore::HttpHandle              mYieldingHandle;
    LLCore::HttpRequest::wptr_t     mWeakRequest;
    HttpCoroHandler::wptr_t         mWeakHandler;
};


} // end namespace LLCoreHttpUtil

#endif // LL_LLCOREHTTPUTIL_H
