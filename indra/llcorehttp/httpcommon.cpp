/**
 * @file httpcommon.cpp
 * @brief
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2014, Linden Research, Inc.
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

#include "linden_common.h"      // Modifies curl/curl.h interfaces
#include "httpcommon.h"
#include "llhttpconstants.h"
#include "llmutex.h"
#include "llthread.h"
#include <curl/curl.h>
#include <string>
#include <sstream>


namespace LLCore
{

HttpStatus::type_enum_t EXT_CURL_EASY;
HttpStatus::type_enum_t EXT_CURL_MULTI;
HttpStatus::type_enum_t LLCORE;

HttpStatus::operator U32() const
{
    // Effectively, concatenate mType (high) with mStatus (low).
    static const int shift(sizeof(mDetails->mStatus) * 8);

    U32 result(U32(mDetails->mType) << shift | U32((int)mDetails->mStatus));
    return result;
}


std::string HttpStatus::toHex() const
{
    std::ostringstream result;
    result.width(8);
    result.fill('0');
    result << std::hex << operator U32();
    return result.str();
}


std::string HttpStatus::toString() const
{
    static const std::vector<std::string> llcore_errors =
        {
            "",
            "HTTP error reply status",
            "Services shutting down",
            "Operation canceled",
            "Invalid Content-Range header encountered",
            "Request handle not found",
            "Invalid datatype for argument or option",
            "Option has not been explicitly set",
            "Option is not dynamic and must be set early",
            "Invalid HTTP status code received from server",
            "Could not allocate required resource"
        };

    static const std::map<type_enum_t, std::string> http_errors =
        {
            { HTTP_CONTINUE,                        "Continue" },
            { HTTP_SWITCHING_PROTOCOLS,             "Switching Protocols" },
            { HTTP_OK,                              "OK" },
            { HTTP_CREATED,                         "Created" },
            { HTTP_ACCEPTED,                        "Accepted" },
            { HTTP_NON_AUTHORITATIVE_INFORMATION,   "Non-Authoritative Information" },
            { HTTP_NO_CONTENT,                      "No Content" },
            { HTTP_RESET_CONTENT,                   "Reset Content" },
            { HTTP_PARTIAL_CONTENT,                 "Partial Content" },
            { HTTP_MULTIPLE_CHOICES,                "Multiple Choices" },
            { HTTP_MOVED_PERMANENTLY,               "Moved Permanently" },
            { HTTP_FOUND,                           "Found" },
            { HTTP_SEE_OTHER,                       "See Other" },
            { HTTP_NOT_MODIFIED,                    "Not Modified" },
            { HTTP_USE_PROXY,                       "Use Proxy" },
            { HTTP_TEMPORARY_REDIRECT,              "Temporary Redirect" },
            { HTTP_BAD_REQUEST,                     "Bad Request" },
            { HTTP_UNAUTHORIZED,                    "Unauthorized" },
            { HTTP_PAYMENT_REQUIRED,                "Payment Required" },
            { HTTP_FORBIDDEN,                       "Forbidden" },
            { HTTP_NOT_FOUND,                       "Not Found" },
            { HTTP_METHOD_NOT_ALLOWED,              "Method Not Allowed" },
            { HTTP_NOT_ACCEPTABLE,                  "Not Acceptable" },
            { HTTP_PROXY_AUTHENTICATION_REQUIRED,   "Proxy Authentication Required" },
            { HTTP_REQUEST_TIME_OUT,                "Request Time-out" },
            { HTTP_CONFLICT,                        "Conflict" },
            { HTTP_GONE,                            "Gone" },
            { HTTP_LENGTH_REQUIRED,                 "Length Required" },
            { HTTP_PRECONDITION_FAILED,             "Precondition Failed" },
            { HTTP_REQUEST_ENTITY_TOO_LARGE,        "Request Entity Too Large" },
            { HTTP_REQUEST_URI_TOO_LARGE,           "Request-URI Too Large" },
            { HTTP_UNSUPPORTED_MEDIA_TYPE,          "Unsupported Media Type" },
            { HTTP_REQUESTED_RANGE_NOT_SATISFIABLE, "Requested range not satisfiable" },
            { HTTP_EXPECTATION_FAILED,              "Expectation Failed" },
            { 499,                                  "Linden Catch-All" },
            { HTTP_INTERNAL_SERVER_ERROR,           "Internal Server Error" },
            { HTTP_NOT_IMPLEMENTED,                 "Not Implemented" },
            { HTTP_BAD_GATEWAY,                     "Bad Gateway" },
            { HTTP_SERVICE_UNAVAILABLE,             "Service Unavailable" },
            { HTTP_GATEWAY_TIME_OUT,                "Gateway Time-out" },
            { HTTP_VERSION_NOT_SUPPORTED,           "HTTP Version not supported" }
        };

    if (*this)
    {
        return LLStringUtil::null;
    }

    switch (getType())
    {
    case EXT_CURL_EASY:
        return std::string(curl_easy_strerror(CURLcode(getStatus())));

    case EXT_CURL_MULTI:
        return std::string(curl_multi_strerror(CURLMcode(getStatus())));

    case LLCORE:
        if (getStatus() >= 0 && std::size_t(getStatus()) < llcore_errors.size())
        {
            return llcore_errors[getStatus()];
        }
        break;

    default:
        if (isHttpStatus())
        {
            // special handling for status 499 "Linden Catchall"
            if ((getType() == 499) && (!getMessage().empty()))
                return getMessage();

            auto it = http_errors.find(getType());
            if (it != http_errors.end())
            {
                return it->second;
            }
        }
        break;
    }

    return "Unknown error";
}


std::string HttpStatus::toTerseString() const
{
    std::ostringstream result;

    unsigned int error_value((unsigned short)getStatus());

    switch (getType())
    {
    case EXT_CURL_EASY:
        result << "Easy_";
        break;

    case EXT_CURL_MULTI:
        result << "Multi_";
        break;

    case LLCORE:
        result << "Core_";
        break;

    default:
        if (isHttpStatus())
        {
            result << "Http_";
            error_value = getType();
        }
        else
        {
            result << "Unknown_";
        }
        break;
    }

    result << error_value;
    return result.str();
}


// Pass true on statuses that might actually be cleared by a
// retry.  Library failures, calling problems, etc. aren't
// going to be fixed by squirting bits all over the Net.
//
// HE_INVALID_HTTP_STATUS is special.  As of 7.37.0, there are
// some scenarios where response processing in libcurl appear
// to go wrong and response data is corrupted.  A side-effect
// of this is that the HTTP status is read as 0 from the library.
// See libcurl bug report 1420 (https://sourceforge.net/p/curl/bugs/1420/)
// for details.
bool HttpStatus::isRetryable() const
{
    static const HttpStatus cant_connect(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_CONNECT);
    static const HttpStatus cant_res_proxy(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_RESOLVE_PROXY);
    static const HttpStatus cant_res_host(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_RESOLVE_HOST);
    static const HttpStatus send_error(HttpStatus::EXT_CURL_EASY, CURLE_SEND_ERROR);
    static const HttpStatus recv_error(HttpStatus::EXT_CURL_EASY, CURLE_RECV_ERROR);
    static const HttpStatus upload_failed(HttpStatus::EXT_CURL_EASY, CURLE_UPLOAD_FAILED);
    static const HttpStatus op_timedout(HttpStatus::EXT_CURL_EASY, CURLE_OPERATION_TIMEDOUT);
    static const HttpStatus post_error(HttpStatus::EXT_CURL_EASY, CURLE_HTTP_POST_ERROR);
    static const HttpStatus partial_file(HttpStatus::EXT_CURL_EASY, CURLE_PARTIAL_FILE);
    static const HttpStatus inv_cont_range(HttpStatus::LLCORE, HE_INV_CONTENT_RANGE_HDR);
    static const HttpStatus inv_status(HttpStatus::LLCORE, HE_INVALID_HTTP_STATUS);

    // *DEBUG:  For "[curl:bugs] #1420" tests.
    // Disable the '*this == inv_status' test and look for 'Core_9'
    // failures in log files.

    return ((isHttpStatus() && getType() >= 499 && getType() <= 599) || // Include special 499 in retryables
            *this == cant_connect ||    // Connection reset/endpoint problems
            *this == cant_res_proxy ||  // DNS problems
            *this == cant_res_host ||   // DNS problems
            *this == send_error ||      // General socket problems
            *this == recv_error ||      // General socket problems
            *this == upload_failed ||   // Transport problem
            *this == op_timedout ||     // Timer expired
            *this == post_error ||      // Transport problem
            *this == partial_file ||    // Data inconsistency in response
            // *DEBUG:  Comment out 'inv_status' test for [curl:bugs] #1420 testing.
            *this == inv_status ||      // Inv status can reflect internal state problem in libcurl
            *this == inv_cont_range);   // Short data read disagrees with content-range
}

namespace LLHttp
{
namespace
{
CURL *getCurlTemplateHandle()
{
    static CURL *curlpTemplateHandle = NULL;

    if (curlpTemplateHandle == NULL)
    {   // Late creation of the template curl handle
        curlpTemplateHandle = curl_easy_init();
        if (curlpTemplateHandle == NULL)
        {
            LL_WARNS() << "curl error calling curl_easy_init()" << LL_ENDL;
        }
        else
        {
            CURLcode result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
            check_curl_code(result, CURLOPT_IPRESOLVE);
            result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_NOSIGNAL, 1);
            check_curl_code(result, CURLOPT_NOSIGNAL);
            result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_NOPROGRESS, 1);
            check_curl_code(result, CURLOPT_NOPROGRESS);

#if LIBCURL_VERSION_MAJOR > 7
            result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_ENCODING, nullptr);
#else
            result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_ENCODING, "");
#endif

            check_curl_code(result, CURLOPT_ENCODING);
            result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_AUTOREFERER, 1);
            check_curl_code(result, CURLOPT_AUTOREFERER);
            result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_FOLLOWLOCATION, 1);
            check_curl_code(result, CURLOPT_FOLLOWLOCATION);
            result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_SSL_VERIFYPEER, 1);
            check_curl_code(result, CURLOPT_SSL_VERIFYPEER);
            result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_SSL_VERIFYHOST, 0);
            check_curl_code(result, CURLOPT_SSL_VERIFYHOST);

            // The Linksys WRT54G V5 router has an issue with frequent
            // DNS lookups from LAN machines.  If they happen too often,
            // like for every HTTP request, the router gets annoyed after
            // about 700 or so requests and starts issuing TCP RSTs to
            // new connections.  Reuse the DNS lookups for even a few
            // seconds and no RSTs.
            result = curl_easy_setopt(curlpTemplateHandle, CURLOPT_DNS_CACHE_TIMEOUT, 15);
            check_curl_code(result, CURLOPT_DNS_CACHE_TIMEOUT);
        }
    }

    return curlpTemplateHandle;
}

LLMutex *getCurlMutex()
{
    static LLMutex* sHandleMutexp = NULL;

    if (!sHandleMutexp)
    {
        sHandleMutexp = new LLMutex();
    }

    return sHandleMutexp;
}

void deallocateEasyCurl(CURL *curlp)
{
    LLMutexLock lock(getCurlMutex());

    curl_easy_cleanup(curlp);
}


}

void initialize()
{
    // Do not change this "unless you are familiar with and mean to control
    // internal operations of libcurl"
    // - http://curl.haxx.se/libcurl/c/curl_global_init.html
    CURLcode code = curl_global_init(CURL_GLOBAL_ALL);

    check_curl_code(code, CURL_GLOBAL_ALL);

}


void cleanup()
{
    curl_global_cleanup();
}


CURL_ptr createEasyHandle()
{
    LLMutexLock lock(getCurlMutex());

    CURL* handle = curl_easy_duphandle(getCurlTemplateHandle());

    return CURL_ptr(handle, &deallocateEasyCurl);
}

std::string getCURLVersion()
{
    return std::string(curl_version());
}

void check_curl_code(CURLcode code, int curl_setopt_option)
{
    if (CURLE_OK != code)
    {
        // Comment from old llcurl code which may no longer apply:
        //
        // linux appears to throw a curl error once per session for a bad initialization
        // at a pretty random time (when enabling cookies).
        LL_WARNS() << "libcurl error detected:  " << curl_easy_strerror(code)
            << ", curl_easy_setopt option:  " << curl_setopt_option
            << LL_ENDL;
    }

}

}
} // end namespace LLCore
