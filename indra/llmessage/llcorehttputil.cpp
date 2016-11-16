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
#include "llsd.h"
#include "llsdjson.h"
#include "llsdserialize.h"
#include "reader.h" // JSON
#include "writer.h" // JSON
#include "llvfile.h"

#include "message.h" // for getting the port


using namespace LLCore;

namespace LLCoreHttpUtil
{

const F32 HTTP_REQUEST_EXPIRY_SECS = 60.0f;

namespace 
{
    const std::string   HTTP_LOGBODY_KEY("HTTPLogBodyOnError");

    BoolSettingQuery_t  mBoolSettingGet;
    BoolSettingUpdate_t mBoolSettingPut;

    inline bool getBoolSetting(const std::string &keyname)
    {
        if (!mBoolSettingGet || mBoolSettingGet.empty())
            return(false);
        return mBoolSettingGet(HTTP_LOGBODY_KEY);
    }

}

void setPropertyMethods(BoolSettingQuery_t queryfn, BoolSettingUpdate_t updatefn)
{
    mBoolSettingGet = queryfn;
    mBoolSettingPut = updatefn;

    if (mBoolSettingPut && !mBoolSettingPut.empty())
    {
        mBoolSettingPut(HTTP_LOGBODY_KEY, false, "Log the entire HTTP body in the case of an HTTP error.");
    }
}


void logMessageSuccess(std::string logAuth, std::string url, std::string message)
{
    LL_INFOS() << logAuth << " Success '" << message << "' for " << url << LL_ENDL;
}

void logMessageFail(std::string logAuth, std::string url, std::string message)
{
    LL_WARNS() << logAuth << " Failure '" << message << "' for " << url << LL_ENDL;
}

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
    const HttpOptions::ptr_t &options,
    const HttpHeaders::ptr_t &headers,
    const HttpHandler::ptr_t &handler)
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
    const HttpOptions::ptr_t &options,
    const HttpHeaders::ptr_t &headers,
    const HttpHandler::ptr_t &handler)
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
    const HttpOptions::ptr_t &options,
    const HttpHeaders::ptr_t &headers,
    const HttpHandler::ptr_t &handler)
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
        bool parseSuccess(false);
        result = LLSD::emptyMap();
        LLCore::HttpStatus::type_enum_t errType = status.getType();

        LL_WARNS()
            << "\n--------------------------------------------------------------------------\n"
            << " Error[" << status.toTerseString() << "] cannot access url '" << response->getRequestURL()
            << "' because " << status.toString()
            << "\n--------------------------------------------------------------------------"
            << LL_ENDL;
        if ((errType >= 400) && (errType < 500))
        {
            LLSD body = this->parseBody(response, parseSuccess);
            if (!body.isUndefined())
            {
                if (!body.isMap())
                {
                    result[HttpCoroutineAdapter::HTTP_RESULTS_CONTENT] = body;
                }
                else
                {
                    result = body;
                }
            }

        }
    }
    else
    {
        result = this->handleSuccess(response, status);
    }

    buildStatusEntry(response, status, result);

    if (!status)
    {
        LLSD &httpStatus = result[HttpCoroutineAdapter::HTTP_RESULTS];

        LLCore::BufferArray *body = response->getBody();
        LLCore::BufferArrayStream bas(body);
        LLSD::String bodyData;
        bodyData.reserve(response->getBodySize());
        bas >> std::noskipws;
        bodyData.assign(std::istream_iterator<U8>(bas), std::istream_iterator<U8>());
        httpStatus["error_body"] = LLSD(bodyData);
        if (getBoolSetting(HTTP_LOGBODY_KEY))
        {
            // commenting out, but keeping since this can be useful for debugging
            LL_WARNS() << "Returned body=" << std::endl << httpStatus["error_body"].asString() << LL_ENDL;
        }
    }

    mReplyPump.post(result);
}

void HttpCoroHandler::buildStatusEntry(LLCore::HttpResponse *response, LLCore::HttpStatus status, LLSD &result)
{
    LLSD httpresults = LLSD::emptyMap();

    writeStatusCodes(status, response->getRequestURL(), httpresults);

    LLSD httpHeaders = LLSD::emptyMap();
    LLCore::HttpHeaders::ptr_t hdrs = response->getHeaders();

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
/// If the LLSD retrieved from through the HTTP connection is not in the form
/// of a LLSD::map it will be returned as in an llsd["content"] element.
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
    virtual LLSD parseBody(LLCore::HttpResponse *response, bool &success);
};

//-------------------------------------------------------------------------
HttpCoroLLSDHandler::HttpCoroLLSDHandler(LLEventStream &reply):
    HttpCoroHandler(reply)
{
}
    

LLSD HttpCoroLLSDHandler::handleSuccess(LLCore::HttpResponse * response, LLCore::HttpStatus &status)
{
    LLSD result;

//    const bool emit_parse_errors = false;
    bool success(false);

    result = parseBody(response, success);

#if 0
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
#endif

    if (!success)
    {   
#if 1
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
#endif
        // If we've gotten to this point and the result LLSD is still undefined 
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

LLSD HttpCoroLLSDHandler::parseBody(LLCore::HttpResponse *response, bool &success)
{
    success = true;
    if (response->getBodySize() == 0)
        return LLSD();

    LLSD result;

    if (!LLCoreHttpUtil::responseToLLSD(response, true, result))
    {
        success = false;
        return LLSD();
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
    virtual LLSD parseBody(LLCore::HttpResponse *response, bool &success);
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

#if 1
    // This is the slower implementation.  It is safe vis-a-vi the const_cast<> and modification
    // of a LLSD managed array but contains an extra (potentially large) copy.
    // 
    // *TODO: https://jira.secondlife.com/browse/MAINT-5221
    
    LLSD::Binary data;
    data.reserve(size);
    bas >> std::noskipws;
    data.assign(std::istream_iterator<U8>(bas), std::istream_iterator<U8>());

    result[HttpCoroutineAdapter::HTTP_RESULTS_RAW] = data;

#else
    // This is disabled because it's dangerous.  See the other case for an 
    // alternate implementation.
    // We create a new LLSD::Binary object and assign it to the result map.
    // The LLSD has created it's own copy so we retrieve it asBinary and const cast 
    // the reference so that we can modify it.
    // *TODO: This is potentially dangerous... but I am trying to avoid a potentially 
    // large copy.
    result[HttpCoroutineAdapter::HTTP_RESULTS_RAW] = LLSD::Binary();
    LLSD::Binary &data = const_cast<LLSD::Binary &>( result[HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary() );

    data.reserve(size);
    bas >> std::noskipws;
    data.assign(std::istream_iterator<U8>(bas), std::istream_iterator<U8>());
#endif

    return result;
}

LLSD HttpCoroRawHandler::parseBody(LLCore::HttpResponse *response, bool &success)
{
    success = true;
    return LLSD();
}

//========================================================================
/// The HttpCoroJSONHandler is a specialization of the LLCore::HttpHandler for 
/// interacting with coroutines. 
/// 
/// In addition to the normal "http_results" the returned LLSD will contain 
/// JSON entries will be converted into an LLSD map.  All results are considered 
/// strings
///                      
class HttpCoroJSONHandler : public HttpCoroHandler
{
public:
    HttpCoroJSONHandler(LLEventStream &reply);

    virtual LLSD handleSuccess(LLCore::HttpResponse * response, LLCore::HttpStatus &status);
    virtual LLSD parseBody(LLCore::HttpResponse *response, bool &success);
};

//-------------------------------------------------------------------------
HttpCoroJSONHandler::HttpCoroJSONHandler(LLEventStream &reply) :
    HttpCoroHandler(reply)
{
}

LLSD HttpCoroJSONHandler::handleSuccess(LLCore::HttpResponse * response, LLCore::HttpStatus &status)
{
    LLSD result = LLSD::emptyMap();

    BufferArray * body(response->getBody());
    if (!body || !body->size())
    {
        return result;
    }

    LLCore::BufferArrayStream bas(body);
    Json::Value jsonRoot;

    try
    {
        bas >> jsonRoot;
    }
    catch (std::runtime_error e)
    {   // deserialization failed.  Record the reason and pass back an empty map for markup.
        status = LLCore::HttpStatus(499, std::string(e.what()));
        return result;
    }

    // Convert the JSON structure to LLSD
    result = LlsdFromJson(jsonRoot);

    return result;
}

LLSD HttpCoroJSONHandler::parseBody(LLCore::HttpResponse *response, bool &success)
{
    success = true;
    BufferArray * body(response->getBody());
    if (!body || !body->size())
    {
        return LLSD();
    }

    LLCore::BufferArrayStream bas(body);
    Json::Value jsonRoot;

    try
    {
        bas >> jsonRoot;
    }
    catch (std::runtime_error e)
    {   
        success = false;
        return LLSD();
    }

    // Convert the JSON structure to LLSD
    return LlsdFromJson(jsonRoot);
}

//========================================================================
HttpRequestPumper::HttpRequestPumper(const LLCore::HttpRequest::ptr_t &request) :
    mHttpRequest(request)
{
    mBoundListener = LLEventPumps::instance().obtain("mainloop").
        listen(LLEventPump::ANONYMOUS, boost::bind(&HttpRequestPumper::pollRequest, this, _1));
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
    cancelSuspendedOperation();
}

LLSD HttpCoroutineAdapter::postAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName, true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroLLSDHandler(replyPump));

    return postAndSuspend_(request, url, body, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::postAndSuspend_(LLCore::HttpRequest::ptr_t &request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    HttpRequestPumper pumper(request);

    checkDefaultHeaders(headers);

    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = requestPostWithLLSD(request,
        mPolicyId, mPriority, url, body, options, headers,
        handler);

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = llcoro::suspendUntilEventOn(handler->getReplyPump());
    cleanState();

    return results;
}

LLSD HttpCoroutineAdapter::postAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, LLCore::BufferArray::ptr_t rawbody,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName, true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroLLSDHandler(replyPump));

    return postAndSuspend_(request, url, rawbody, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::postRawAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, LLCore::BufferArray::ptr_t rawbody,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName, true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroRawHandler(replyPump));

    return postAndSuspend_(request, url, rawbody, options, headers, httpHandler);
}

// *TODO: This functionality could be moved into the LLCore::Http library itself 
// by having the CURL layer read the file directly.
LLSD HttpCoroutineAdapter::postFileAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, std::string fileName,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLCore::BufferArray::ptr_t fileData(new LLCore::BufferArray);

    // scoping for our streams so that they go away when we no longer need them.
    {
        LLCore::BufferArrayStream outs(fileData.get());
        llifstream ins(fileName.c_str(), std::iostream::binary | std::iostream::out);

        if (ins.is_open())
        {

            ins.seekg(0, std::ios::beg);
            ins >> std::noskipws;

            std::copy(std::istream_iterator<U8>(ins), std::istream_iterator<U8>(),
                    std::ostream_iterator<U8>(outs));

            ins.close();
        }
    }

    return postAndSuspend(request, url, fileData, options, headers);
}

// *TODO: This functionality could be moved into the LLCore::Http library itself 
// by having the CURL layer read the file directly.
LLSD HttpCoroutineAdapter::postFileAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, LLUUID assetId, LLAssetType::EType assetType,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLCore::BufferArray::ptr_t fileData(new LLCore::BufferArray);

    // scoping for our streams so that they go away when we no longer need them.
    {
        LLCore::BufferArrayStream outs(fileData.get());
        LLVFile vfile(gVFS, assetId, assetType, LLVFile::READ);

        S32 fileSize = vfile.getSize();
        U8* fileBuffer;
        fileBuffer = new U8[fileSize];
        vfile.read(fileBuffer, fileSize);
        
        outs.write((char*)fileBuffer, fileSize);
        delete[] fileBuffer;
    }

    return postAndSuspend(request, url, fileData, options, headers);
}

LLSD HttpCoroutineAdapter::postJsonAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName, true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroJSONHandler(replyPump));

    LLCore::BufferArray::ptr_t rawbody(new LLCore::BufferArray);

    {
        LLCore::BufferArrayStream outs(rawbody.get());
        Json::Value root = LlsdToJson(body);
        Json::FastWriter writer;

        LL_WARNS("Http::post") << "JSON Generates: \"" << writer.write(root) << "\"" << LL_ENDL;

        outs << writer.write(root);
    }

    return postAndSuspend_(request, url, rawbody, options, headers, httpHandler);
}


LLSD HttpCoroutineAdapter::postAndSuspend_(LLCore::HttpRequest::ptr_t &request,
    const std::string & url, LLCore::BufferArray::ptr_t &rawbody,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    HttpRequestPumper pumper(request);

    checkDefaultHeaders(headers);

    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = request->requestPost(mPolicyId, mPriority, url, rawbody.get(),
        options, headers, handler);

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = llcoro::suspendUntilEventOn(handler->getReplyPump());
    cleanState();

    return results;
}

LLSD HttpCoroutineAdapter::putAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroLLSDHandler(replyPump));

    return putAndSuspend_(request, url, body, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::putJsonAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName, true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroJSONHandler(replyPump));

    LLCore::BufferArray::ptr_t rawbody(new LLCore::BufferArray);

    {
        LLCore::BufferArrayStream outs(rawbody.get());
        Json::Value root = LlsdToJson(body);
        Json::FastWriter writer;

        LL_WARNS("Http::put") << "JSON Generates: \"" << writer.write(root) << "\"" << LL_ENDL;
        outs << writer.write(root);
    }

    return putAndSuspend_(request, url, rawbody, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::putAndSuspend_(LLCore::HttpRequest::ptr_t &request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    HttpRequestPumper pumper(request);

    checkDefaultHeaders(headers);

    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = requestPutWithLLSD(request,
        mPolicyId, mPriority, url, body, options, headers,
        handler);

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = llcoro::suspendUntilEventOn(handler->getReplyPump());
    cleanState();

    return results;
}

LLSD HttpCoroutineAdapter::putAndSuspend_(LLCore::HttpRequest::ptr_t &request,
    const std::string & url, const LLCore::BufferArray::ptr_t & rawbody,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    HttpRequestPumper pumper(request);

    checkDefaultHeaders(headers);

    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = request->requestPut(mPolicyId, mPriority, 
        url, rawbody.get(), options, headers, handler);

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = llcoro::suspendUntilEventOn(handler->getReplyPump());
    cleanState();

    return results;
}


LLSD HttpCoroutineAdapter::getAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroLLSDHandler(replyPump));

    return getAndSuspend_(request, url, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::getRawAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroRawHandler(replyPump));

    return getAndSuspend_(request, url, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::getJsonAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroJSONHandler(replyPump));

    return getAndSuspend_(request, url, options, headers, httpHandler);
}


LLSD HttpCoroutineAdapter::getAndSuspend_(LLCore::HttpRequest::ptr_t &request,
    const std::string & url,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers, 
    HttpCoroHandler::ptr_t &handler)
{
    HttpRequestPumper pumper(request);
    checkDefaultHeaders(headers);

    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = request->requestGet(mPolicyId, mPriority,
        url, options, headers, handler);

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = llcoro::suspendUntilEventOn(handler->getReplyPump());
    cleanState();

    return results;
}


LLSD HttpCoroutineAdapter::deleteAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroLLSDHandler(replyPump));

    return deleteAndSuspend_(request, url, options, headers, httpHandler);
}

LLSD HttpCoroutineAdapter::deleteJsonAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, 
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroJSONHandler(replyPump));

    return deleteAndSuspend_(request, url, options, headers, httpHandler);
}


LLSD HttpCoroutineAdapter::deleteAndSuspend_(LLCore::HttpRequest::ptr_t &request,
    const std::string & url, LLCore::HttpOptions::ptr_t &options, 
    LLCore::HttpHeaders::ptr_t &headers, HttpCoroHandler::ptr_t &handler)
{
    HttpRequestPumper pumper(request);

    checkDefaultHeaders(headers);
    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = request->requestDelete(mPolicyId, mPriority,
        url, options, headers, handler);

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = llcoro::suspendUntilEventOn(handler->getReplyPump());
    cleanState();

    return results;
}

LLSD HttpCoroutineAdapter::patchAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroLLSDHandler(replyPump));

    return patchAndSuspend_(request, url, body, options, headers, httpHandler);
}


LLSD HttpCoroutineAdapter::patchAndSuspend_(LLCore::HttpRequest::ptr_t &request,
    const std::string & url, const LLSD & body,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    HttpRequestPumper pumper(request);

    checkDefaultHeaders(headers);

    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    LLCore::HttpHandle hhandle = requestPatchWithLLSD(request,
        mPolicyId, mPriority, url, body, options, headers,
        handler);

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = llcoro::suspendUntilEventOn(handler->getReplyPump());
    cleanState();

    return results;
}

LLSD HttpCoroutineAdapter::copyAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, const std::string dest,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroLLSDHandler(replyPump));

    if (!headers)
        headers.reset(new LLCore::HttpHeaders);
    headers->append(HTTP_OUT_HEADER_DESTINATION, dest);

    return copyAndSuspend_(request, url, options, headers, httpHandler);
}


LLSD HttpCoroutineAdapter::copyAndSuspend_(LLCore::HttpRequest::ptr_t &request,
    const std::string & url, 
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    HttpRequestPumper pumper(request);

    checkDefaultHeaders(headers);

    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    // 
    LLCore::HttpHandle hhandle = request->requestCopy(mPolicyId, mPriority, url,
        options, headers, handler);

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = llcoro::suspendUntilEventOn(handler->getReplyPump());
    cleanState();

    return results;
}

LLSD HttpCoroutineAdapter::moveAndSuspend(LLCore::HttpRequest::ptr_t request,
    const std::string & url, const std::string dest,
    LLCore::HttpOptions::ptr_t options, LLCore::HttpHeaders::ptr_t headers)
{
    LLEventStream  replyPump(mAdapterName + "Reply", true);
    HttpCoroHandler::ptr_t httpHandler(new HttpCoroLLSDHandler(replyPump));

    if (!headers)
        headers.reset(new LLCore::HttpHeaders);
    headers->append(HTTP_OUT_HEADER_DESTINATION, dest);

    return moveAndSuspend_(request, url, options, headers, httpHandler);
}


LLSD HttpCoroutineAdapter::moveAndSuspend_(LLCore::HttpRequest::ptr_t &request,
    const std::string & url,
    LLCore::HttpOptions::ptr_t &options, LLCore::HttpHeaders::ptr_t &headers,
    HttpCoroHandler::ptr_t &handler)
{
    HttpRequestPumper pumper(request);

    checkDefaultHeaders(headers);

    // The HTTPCoroHandler does not self delete, so retrieval of a the contained 
    // pointer from the smart pointer is safe in this case.
    // 
    LLCore::HttpHandle hhandle = request->requestMove(mPolicyId, mPriority, url,
        options, headers, handler);

    if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
    {
        return HttpCoroutineAdapter::buildImmediateErrorResult(request, url);
    }

    saveState(hhandle, request, handler);
    LLSD results = llcoro::suspendUntilEventOn(handler->getReplyPump());
    cleanState();

    return results;
}


void HttpCoroutineAdapter::checkDefaultHeaders(LLCore::HttpHeaders::ptr_t &headers)
{
    if (!headers)
        headers.reset(new LLCore::HttpHeaders);
    if (!headers->find(HTTP_OUT_HEADER_ACCEPT))
    {
        headers->append(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_LLSD_XML);
    }
    if (!headers->find(HTTP_OUT_HEADER_CONTENT_TYPE))
    {
        headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_LLSD_XML);
    }

    if (!headers->find("X-SecondLife-UDP-Listen-Port") && gMessageSystem)
    {
        headers->append("X-SecondLife-UDP-Listen-Port", llformat("%d", gMessageSystem->mPort));
    }
}


void HttpCoroutineAdapter::cancelSuspendedOperation()
{
    LLCore::HttpRequest::ptr_t request = mWeakRequest.lock();
    HttpCoroHandler::ptr_t handler = mWeakHandler.lock();
    if ((request) && (handler) && (mYieldingHandle != LLCORE_HTTP_HANDLE_INVALID))
    {
        cleanState();
        LL_INFOS() << "Canceling yielding request!" << LL_ENDL;
        request->requestCancel(mYieldingHandle, handler);
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
void HttpCoroutineAdapter::callbackHttpGet(const std::string &url, LLCore::HttpRequest::policy_t policyId, completionCallback_t success, completionCallback_t failure)
{
    LLCoros::instance().launch("HttpCoroutineAdapter::genericGetCoro",
        boost::bind(&HttpCoroutineAdapter::trivialGetCoro, url, policyId, success, failure));
}

/*static*/
void HttpCoroutineAdapter::messageHttpGet(const std::string &url, const std::string &success, const std::string &failure)
{
    completionCallback_t cbSuccess = (success.empty()) ? NULL : 
        static_cast<completionCallback_t>(boost::bind(&logMessageSuccess, "HttpCoroutineAdapter", url, success));
    completionCallback_t cbFailure = (failure.empty()) ? NULL :
        static_cast<completionCallback_t>(boost::bind(&logMessageFail, "HttpCoroutineAdapter", url, failure));
    callbackHttpGet(url, cbSuccess, cbFailure);
}

/*static*/
void HttpCoroutineAdapter::trivialGetCoro(std::string url, LLCore::HttpRequest::policy_t policyId, completionCallback_t success, completionCallback_t failure)
{
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericGetCoro", policyId));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);

    LL_INFOS("HttpCoroutineAdapter", "genericGetCoro") << "Generic GET for " << url << LL_ENDL;

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {   
        if (failure)
        {
            failure(httpResults);
        }
    }
    else
    {
        if (success)
        {
            success(result);
        }
    }
}

/*static*/
void HttpCoroutineAdapter::callbackHttpPost(const std::string &url, LLCore::HttpRequest::policy_t policyId, const LLSD &postData, completionCallback_t success, completionCallback_t failure)
{
    LLCoros::instance().launch("HttpCoroutineAdapter::genericPostCoro",
        boost::bind(&HttpCoroutineAdapter::trivialPostCoro, url, policyId, postData, success, failure));
}

/*static*/
void HttpCoroutineAdapter::messageHttpPost(const std::string &url, const LLSD &postData, const std::string &success, const std::string &failure)
{
    completionCallback_t cbSuccess = (success.empty()) ? NULL :
        static_cast<completionCallback_t>(boost::bind(&logMessageSuccess, "HttpCoroutineAdapter", url, success));
    completionCallback_t cbFailure = (failure.empty()) ? NULL :
        static_cast<completionCallback_t>(boost::bind(&logMessageFail, "HttpCoroutineAdapter", url, failure));

    callbackHttpPost(url, postData, cbSuccess, cbFailure);
}

/*static*/
void HttpCoroutineAdapter::trivialPostCoro(std::string url, LLCore::HttpRequest::policy_t policyId, LLSD postData, completionCallback_t success, completionCallback_t failure)
{
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", policyId));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);

    LL_INFOS("HttpCoroutineAdapter", "genericPostCoro") << "Generic POST for " << url << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        // If a failure routine is provided do it.
        if (failure)
        {
            failure(httpResults);
        }
    }
    else
    {
        // If a success routine is provided do it.
        if (success)
        {
            success(result);
        }
    }
}



} // end namespace LLCoreHttpUtil

