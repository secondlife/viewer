/** 
 * @file llcorehttputil.cpp
 * @date 2014-08-25
 * @brief Implementation of adapter and utility classes expanding the llcorehttp interfaces.
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

#include "linden_common.h"

#include <sstream>
#include <algorithm>
#include <iterator>
#include "llcorehttputil.h"
#include "llhttpconstants.h"
#include "llsdserialize.h"

using namespace LLCore;


namespace LLCoreHttpUtil
{
//=========================================================================
/// The HttpRequestPumper is a utility class. When constructed it will poll the 
/// supplied HttpRequest once per frame until it is destroyed.
/// 
class HttpRequestPumper
{
public:
    HttpRequestPumper(const LLCore::HttpRequest::ptr_t &request);
    ~HttpRequestPumper();

private:
    bool                       pollRequest(const LLSD&);

    LLTempBoundListener        mBoundListener;
    LLCore::HttpRequest::ptr_t mHttpRequest;
};


//=========================================================================
// *TODO:  Currently converts only from XML content.  A mode
// to convert using fromBinary() might be useful as well.  Mesh
// headers could use it.
bool responseToLLSD(HttpResponse * response, bool log, LLSD & out_llsd)
{
    // Convert response to LLSD
    BufferArray * body(response->getBody());
    if (!body || !body->size())
    {
        return false;
    }

    LLCore::BufferArrayStream bas(body);
    LLSD body_llsd;
    S32 parse_status(LLSDSerialize::fromXML(body_llsd, bas, log));
    if (LLSDParser::PARSE_FAILURE == parse_status){
        return false;
    }
    out_llsd = body_llsd;
    return true;
}


HttpHandle requestPostWithLLSD(HttpRequest * request,
    HttpRequest::policy_t policy_id,
    HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    HttpOptions * options,
    HttpHeaders * headers,
    HttpHandler * handler)
{
    HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

    BufferArray * ba = new BufferArray();
    BufferArrayStream bas(ba);
    LLSDSerialize::toXML(body, bas);

    handle = request->requestPost(policy_id,
        priority,
        url,
        ba,
        options,
        headers,
        handler);
    ba->release();
    return handle;
}


HttpHandle requestPutWithLLSD(HttpRequest * request,
    HttpRequest::policy_t policy_id,
    HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    HttpOptions * options,
    HttpHeaders * headers,
    HttpHandler * handler)
{
    HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

    BufferArray * ba = new BufferArray();
    BufferArrayStream bas(ba);
    LLSDSerialize::toXML(body, bas);

    handle = request->requestPut(policy_id,
        priority,
        url,
        ba,
        options,
        headers,
        handler);
    ba->release();
    return handle;
}

HttpHandle requestPatchWithLLSD(HttpRequest * request,
    HttpRequest::policy_t policy_id,
    HttpRequest::priority_t priority,
    const std::string & url,
    const LLSD & body,
    HttpOptions * options,
    HttpHeaders * headers,
    HttpHandler * handler)
{
    HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

    BufferArray * ba = new BufferArray();
    BufferArrayStream bas(ba);
    LLSDSerialize::toXML(body, bas);

    handle = request->requestPatch(policy_id,
        priority,
        url,
        ba,
        options,
        headers,
        handler);
    ba->release();
    return handle;
}


std::string responseToString(LLCore::HttpResponse * response)
{
    static const std::string empty("[Empty]");

    if (!response)
    {
        return empty;
    }

    BufferArray * body(response->getBody());
    if (!body || !body->size())
    {
        return empty;
    }

    // Attempt to parse as LLSD regardless of content-type
    LLSD body_llsd;
    if (responseToLLSD(response, false, body_llsd))
    {
        std::ostringstream tmp;

        LLSDSerialize::toPrettyNotation(body_llsd, tmp);
        std::size_t temp_len(tmp.tellp());

        if (temp_len)
        {
            return tmp.str().substr(0, std::min(temp_len, std::size_t(1024)));
        }
    }
    else
    {
        // *TODO:  More elaborate forms based on Content-Type as needed.
        char content[1024];

        size_t len(body->read(0, content, sizeof(content)));
        if (len)
        {
            return std::string(content, 0, len);
        }
    }

    // Default
    return empty;
}

//========================================================================

HttpCoroHandler::HttpCoroHandler(LLEventStream &reply) :
    mReplyPump(reply)
{
}

void HttpCoroHandler::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
{
    LLSD result;

    LLCore::HttpStatus status = response->getStatus();

    if (status == LLCore::HttpStatus(LLCore::HttpStatus::LLCORE, LLCore::HE_HANDLE_NOT_FOUND))
    {   // A response came in for a canceled request and we have not processed the 
        // cancel yet.  Patience!
        return;
    }

    if (!status)
    {
        result = LLSD::emptyMap();
        LL_WARNS()
            << "\n--------------------------------------------------------------------------\n"
            << " Error[" << status.toULong() << "] cannot access url '" << response->getRequestURL()
            << "' because " << status.toString()
            << "\n--------------------------------------------------------------------------"
            << LL_ENDL;

    }
    else
    {
        result = this->handleSuccess(response, status);
    }

    buildStatusEntry(response, status, result);
    mReplyPump.post(result);
}

void HttpCoroHandler::buildStatusEntry(LLCore::HttpResponse *response, LLCore::HttpStatus status, LLSD &result)
{
    LLSD httpresults = LLSD::emptyMap();

    writeStatusCodes(status, response->getRequestURL(), httpresults);

    LLSD httpHeaders = LLSD::emptyMap();
    LLCore::HttpHeaders * hdrs = response->getHeaders();

    if (hdrs)
    {
        for (LLCore::HttpHeaders::iterator it = hdrs->begin(); it != hdrs->end(); ++it)
        {
            if (!(*it).second.empty())
            {
                httpHeaders[(*it).first] = (*it).second;
            }
            else
            {
                httpHeaders[(*it).first] = static_cast<LLSD::Boolean>(true);
            }
        }
    }

    httpresults[HttpCoroutineAdapter::HTTP_RESULTS_HEADERS] = httpHeaders;
    result[HttpCoroutineAdapter::HTTP_RESULTS] = httpresults;
}

void HttpCoroHandler::writeStatusCodes(LLCore::HttpStatus status, const std::string &url, LLSD &result)
{
    result[HttpCoroutineAdapter::HTTP_RESULTS_SUCCESS] = static_cast<LLSD::Boolean>(status);
    result[HttpCoroutineAdapter::HTTP_RESULTS_TYPE] = static_cast<LLSD::Integer>(status.getType());
    result[HttpCoroutineAdapter::HTTP_RESULTS_STATUS] = static_cast<LLSD::Integer>(status.getStatus());
    result[HttpCoroutineAdapter::HTTP_RESULTS_MESSAGE] = static_cast<LLSD::String>(status.getMessage());
    result[HttpCoroutineAdapter::HTTP_RESULTS_URL] = static_cast<LLSD::String>(url);

}

//=========================================================================
/// The HttpCoroLLSDHandler is a specialization of the LLCore::HttpHandler for 
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
class HttpCoroLLSDHandler : public HttpCoroHandler
{
public:
    HttpCoroLLSDHandler(LLEventStream &reply);

protected:
    virtual LLSD handleSuccess(LLCore::HttpResponse * response, LLCore::HttpStatus &status);
};

//-------------------------------------------------------------------------
HttpCoroLLSDHandler::HttpCoroLLSDHandler(LLEventStream &reply):
    HttpCoroHandler(reply)
{
}
    

LLSD HttpCoroLLSDHandler::handleSuccess(LLCore::HttpResponse * response, LLCore::HttpStatus &status)
{
    LLSD result;

    const bool emit_parse_errors = false;

    bool parsed = !((response->getBodySize() == 0) ||
        !LLCoreHttpUtil::responseToLLSD(response, emit_parse_errors, result));

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

            // Replace the status with a new one indicating the failure.
            status = LLCore::HttpStatus(499, "Failed to deserialize LLSD.");
        }
    }

    if (result.isUndefined())
    {   // If we've gotten to this point and the result LLSD is still undefined 
        // either there was an issue deserializing the body or the response was
        // blank.  Create an empty map to hold the result either way.
        result = LLSD::emptyMap();
    }
    else if (!result.isMap())
    {   // The results are not themselves a map.  Move them down so that 
        // this method can return a map to the caller.
        // *TODO: Should it always do this?
        LLSD newResult = LLSD::emptyMap();
        newResult[HttpCoroutineAdapter::HTTP_RESULTS_CONTENT] = result;
        result = newResult;
    }

    return result;
}

//========================================================================
/// The HttpCoroRawHandler is a specialization of the LLCore::HttpHandler for 
/// interacting with coroutines. 
/// 
/// In addition to the normal "http_results" the returned LLSD will contain 
/// an entry keyed with "raw" containing the unprocessed results of the HTTP
/// call.
///                      
class HttpCoroRawHandler : public HttpCoroHandler
{
public:
    HttpCoroRawHandler(LLEventStream &reply);

    virtual LLSD handleSuccess(LLCore::HttpResponse * response, LLCore::HttpStatus &status);
};

//-------------------------------------------------------------------------
HttpCoroRawHandler::HttpCoroRawHandler(LLEventStream &reply):
    HttpCoroHandler(reply)
{
}

LLSD HttpCoroRawHandler::handleSuccess(LLCore::HttpResponse * response, LLCore::HttpStatus &status)
{
    LLSD result = LLSD::emptyMap();

    BufferArray * body(response->getBody());
    if (!body || !body->size())
    {
        return result;
    }

    size_t size = body->size();

    LLCore::BufferArrayStream bas(body);

    // We create a new LLSD::Binary object and assign it to the result map.
    // The LLSD has created it's own copy so we retrieve it asBinary and const cast 
    // the reference so that we can modify it.
    result[HttpCoroutineAdapter::HTTP_RESULTS_RAW] = LLSD::Binary();
    LLSD::Binary &data = const_cast<LLSD::Binary &>( result[HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary() );

    data.reserve(size);
    bas >> std::noskipws;
    data.assign(std::istream_iterator<U8>(bas), std::istream_iterator<U8>());

    return result;
}

//========================================================================
HttpRequestPumper::HttpRequestPumper(const LLCore::HttpRequest::ptr_t &request) :
    mHttpRequest(request)
{
    mBoundListener = LLEventPumps::instance().obtain("mainloop").
        listen(LLEventPump::inventName(), boost::bind(&HttpRequestPumper::pollRequest, this, _1));
}

HttpRequestPumper::~HttpRequestPumper()
{
    if (mBoundListener.connected())
    {
        mBoundListener.disconnect();
    }
}

bool HttpRequestPumper::pollRequest(const LLSD&)
{
    if (mHttpRequest->getStatus() != HttpStatus(HttpStatus::LLCORE, HE_OP_CANCELED))
    {
        mHttpRequest->update(0L);
    }
    return false;
}

//========================================================================
const std::string HttpCoroutineAdapter::HTTP_RESULTS("http_result");
const std::string HttpCoroutineAdapter::HTTP_RESULTS_SUCCESS("success");
const std::string HttpCoroutineAdapter::HTTP_RESULTS_TYPE("type");
const std::string HttpCoroutineAdapter::HTTP_RESULTS_STATUS("status");
const std::string HttpCoroutineAdapter::HTTP_RESULTS_MESSAGE("message");
const std::string HttpCoroutineAdapter::HTTP_RESULTS_URL("url");
const std::string HttpCoroutineAdapter::HTTP_RESULTS_HEADERS("headers");
const std::string HttpCoroutineAdapter::HTTP_RESULTS_CONTENT("content");
const std::string HttpCoroutineAdapter::HTTP_RESULTS_RAW("raw");

HttpCoroutineAdapter::HttpCoroutineAdapter(const std::string &name,
    LLCore::HttpRequest::policy_t policyId, LLCore::HttpRequest::priority_t priority) :
    mAdapterName(name),
    mPolicyId(policyId),
    mPriority(priority),
    mYieldingHandle(LLCORE_HTTP_HANDLE_INVALID),
    mWeakRequest(),
    mWeakHandler()
{
}

HttpCoroutineAdapter::~HttpCoroutineAdapter()
{
    cancelYieldingOperation();
}

LLSD HttpCoroutineAdapter::postAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName, true);
    HttpCoroHandler::ptr_t httpHandler = HttpCoroHandler::ptr_t(new HttpCoroLLSDHandler(replyPump));

    return postAndYield_(self, request, url, body, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::postAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    //LL_INFOS() << "Requesting transaction " << transactionId << LL_ENDL;
    HttpRequestPumper pumper(request);
    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = requestPostWithLLSD(request,
        mPolicyId, mPriority, url, body, options, headers,
        handler.get());

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = waitForEventOn(self, handler->getReplyPump());
    cleanState();

    return results;
}

LLSD HttpCoroutineAdapter::postAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
    const std::string & url, LLCore::BufferArray::ptr_t rawbody,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName, true);
    HttpCoroHandler::ptr_t httpHandler = HttpCoroHandler::ptr_t(new HttpCoroLLSDHandler(replyPump));

    return postAndYield_(self, request, url, rawbody, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::postAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
    const std::string & url, LLCore::BufferArray::ptr_t &rawbody,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    //LL_INFOS() << "Requesting transaction " << transactionId << LL_ENDL;
    HttpRequestPumper pumper(request);
    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = request->requestPost(mPolicyId, mPriority, url, rawbody.get(),
        options.get(), headers.get(), handler.get());

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = waitForEventOn(self, handler->getReplyPump());
    cleanState();

    //LL_INFOS() << "Results for transaction " << transactionId << LL_ENDL;
    return results;
}

LLSD HttpCoroutineAdapter::putAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler = HttpCoroHandler::ptr_t(new HttpCoroLLSDHandler(replyPump));

    return putAndYield_(self, request, url, body, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::putAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    //LL_INFOS() << "Requesting transaction " << transactionId << LL_ENDL;
    HttpRequestPumper pumper(request);
    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = requestPutWithLLSD(request,
        mPolicyId, mPriority, url, body, options, headers,
        handler.get());

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = waitForEventOn(self, handler->getReplyPump());
    cleanState();
    //LL_INFOS() << "Results for transaction " << transactionId << LL_ENDL;
    return results;
}

LLSD HttpCoroutineAdapter::getAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
    const std::string & url,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler = HttpCoroHandler::ptr_t(new HttpCoroLLSDHandler(replyPump));

    return getAndYield_(self, request, url, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::getRawAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
    const std::string & url,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler = HttpCoroHandler::ptr_t(new HttpCoroRawHandler(replyPump));

    return getAndYield_(self, request, url, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::getAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
    const std::string & url,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers, 
    HttpCoroHandler::ptr_t &handler)
{
    //LL_INFOS() << "Requesting transaction " << transactionId << LL_ENDL;
    HttpRequestPumper pumper(request);
    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = request->requestGet(mPolicyId, mPriority,
        url, options.get(), headers.get(), handler.get());

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = waitForEventOn(self, handler->getReplyPump());
    cleanState();
    //LL_INFOS() << "Results for transaction " << transactionId << LL_ENDL;
    return results;
}


LLSD HttpCoroutineAdapter::deleteAndYield(LLCoros::self & self, LLCore::HttpRequest::ptr_t request,
    const std::string & url,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler = HttpCoroHandler::ptr_t(new HttpCoroLLSDHandler(replyPump));

    return deleteAndYield_(self, request, url, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::deleteAndYield_(LLCoros::self & self, LLCore::HttpRequest::ptr_t &request,
    const std::string & url, LLCore::HttpOptions::ptr_t &options, 
    LLCore::HttpHeaders::ptr_t &headers, HttpCoroHandler::ptr_t &handler)
{
    //LL_INFOS() << "Requesting transaction " << transactionId << LL_ENDL;
    HttpRequestPumper pumper(request);
    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = request->requestDelete(mPolicyId, mPriority,
        url, options.get(), headers.get(), handler.get());

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = waitForEventOn(self, handler->getReplyPump());
    cleanState();
    //LL_INFOS() << "Results for transaction " << transactionId << LL_ENDL;
    return results;
}


void HttpCoroutineAdapter::cancelYieldingOperation()
{
    LLCore::HttpRequest::ptr_t request = mWeakRequest.lock();
    HttpCoroHandler::ptr_t handler = mWeakHandler.lock();
    if ((request) && (handler) && (mYieldingHandle != LLCORE_HTTP_HANDLE_INVALID))
    {
        cleanState();
        LL_INFOS() << "Canceling yielding request!" << LL_ENDL;
        request->requestCancel(mYieldingHandle, handler.get());
    }
}

void HttpCoroutineAdapter::saveState(LLCore::HttpHandle yieldingHandle, 
    LLCore::HttpRequest::ptr_t &request, HttpCoroHandler::ptr_t &handler)
{
    mWeakRequest = request;
    mWeakHandler = handler;
    mYieldingHandle = yieldingHandle;
}

void HttpCoroutineAdapter::cleanState()
{
    mWeakRequest.reset();
    mWeakHandler.reset();
    mYieldingHandle = LLCORE_HTTP_HANDLE_INVALID;
}

/*static*/
LLSD HttpCoroutineAdapter::buildImmediateErrorResult(const LLCore::HttpRequest::ptr_t &request, 
    const std::string &url) 
{
    LLCore::HttpStatus status = request->getStatus();
    LL_WARNS() << "Error posting to " << url << " Status=" << status.getStatus() <<
        " message = " << status.getMessage() << LL_ENDL;

    // Mimic the status results returned from an http error that we had 
    // to wait on 
    LLSD httpresults = LLSD::emptyMap();

    HttpCoroHandler::writeStatusCodes(status, url, httpresults);

    LLSD errorres = LLSD::emptyMap();
    errorres["http_result"] = httpresults;

    return errorres;
}

/*static*/
LLCore::HttpStatus HttpCoroutineAdapter::getStatusFromLLSD(const LLSD &httpResults)
{
    LLCore::HttpStatus::type_enum_t type = static_cast<LLCore::HttpStatus::type_enum_t>(httpResults[HttpCoroutineAdapter::HTTP_RESULTS_TYPE].asInteger());
    short code = static_cast<short>(httpResults[HttpCoroutineAdapter::HTTP_RESULTS_STATUS].asInteger());

    return LLCore::HttpStatus(type, code);
}

/*static*/
void HttpCoroutineAdapter::genericHttpGet(const std::string &url, const std::string &success, const std::string &failure)
{
    LLCoros::instance().launch("HttpCoroutineAdapter::genericGetCoro",
        boost::bind(&HttpCoroutineAdapter::genericGetCoro, _1, url, success, failure));
}

/*static*/
void HttpCoroutineAdapter::genericGetCoro(LLCoros::self& self, std::string &url, std::string success, std::string failure)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericGetCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LL_INFOS("HttpCoroutineAdapter", "genericGetCoro") << "Generic GET for " << url << LL_ENDL;

    LLSD result = httpAdapter->getAndYield(self, httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status)
    {   
        LL_INFOS("HttpCoroutineAdapter", "genericGetCoro") << "Success for " << url << std::endl <<
            "Message: '" << success << "'" << LL_ENDL;
    }
    else
    {
        LL_WARNS("HttpCoroutineAdapter", "genericGetCoro") << "Failure for " << url << std::endl <<
            "Message: '" << failure << "'" << std::endl <<
            "Status: " << status.toTerseString() << ": " << status.getMessage() << LL_ENDL;
    }
}

/*static*/
void HttpCoroutineAdapter::genericHttpPost(const std::string &url, const LLSD &postData, const std::string &success, const std::string &failure)
{
    LLCoros::instance().launch("HttpCoroutineAdapter::genericPostCoro",
        boost::bind(&HttpCoroutineAdapter::genericPostCoro, _1, url, postData, success, failure));
}

/*static*/
void HttpCoroutineAdapter::genericPostCoro(LLCoros::self& self, std::string &url, LLSD postData, std::string success, std::string failure)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LL_INFOS("HttpCoroutineAdapter", "genericPostCoro") << "Generic POST for " << url << LL_ENDL;

    LLSD result = httpAdapter->postAndYield(self, httpRequest, url, postData);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status)
    {
        LL_INFOS("HttpCoroutineAdapter", "genericPostCoro") << "Success for " << url << std::endl <<
            "Message: '" << success << "'" << LL_ENDL;
    }
    else
    {
        LL_WARNS("HttpCoroutineAdapter", "genericPostCoro") << "Failure for " << url << std::endl <<
            "Message: '" << failure << "'" << std::endl <<
            "Status: " << status.toTerseString() << ": " << status.getMessage() << LL_ENDL;
    }
}



} // end namespace LLCoreHttpUtil

