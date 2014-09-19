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

#include "httpcommon.h"

#include <curl/curl.h>
#include <string>
#include <sstream>


namespace LLCore
{

HttpStatus::type_enum_t EXT_CURL_EASY;
HttpStatus::type_enum_t EXT_CURL_MULTI;
HttpStatus::type_enum_t LLCORE;

HttpStatus::operator unsigned long() const
{
	static const int shift(sizeof(unsigned long) * 4);

	unsigned long result(((unsigned long) mType) << shift | (unsigned long) (int) mStatus);
	return result;
}


std::string HttpStatus::toHex() const
{
	std::ostringstream result;
	result.width(8);
	result.fill('0');
	result << std::hex << operator unsigned long();
	return result.str();
}


std::string HttpStatus::toString() const
{
	static const char * llcore_errors[] =
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
	static const int llcore_errors_count(sizeof(llcore_errors) / sizeof(llcore_errors[0]));

	static const struct
	{
		type_enum_t		mCode;
		const char *	mText;
	}
	http_errors[] =
		{
			// Keep sorted by mCode, we binary search this list.
			{ 100, "Continue" },
			{ 101, "Switching Protocols" },
			{ 200, "OK" },
			{ 201, "Created" },
			{ 202, "Accepted" },
			{ 203, "Non-Authoritative Information" },
			{ 204, "No Content" },
			{ 205, "Reset Content" },
			{ 206, "Partial Content" },
			{ 300, "Multiple Choices" },
			{ 301, "Moved Permanently" },
			{ 302, "Found" },
			{ 303, "See Other" },
			{ 304, "Not Modified" },
			{ 305, "Use Proxy" },
			{ 307, "Temporary Redirect" },
			{ 400, "Bad Request" },
			{ 401, "Unauthorized" },
			{ 402, "Payment Required" },
			{ 403, "Forbidden" },
			{ 404, "Not Found" },
			{ 405, "Method Not Allowed" },
			{ 406, "Not Acceptable" },
			{ 407, "Proxy Authentication Required" },
			{ 408, "Request Time-out" },
			{ 409, "Conflict" },
			{ 410, "Gone" },
			{ 411, "Length Required" },
			{ 412, "Precondition Failed" },
			{ 413, "Request Entity Too Large" },
			{ 414, "Request-URI Too Large" },
			{ 415, "Unsupported Media Type" },
			{ 416, "Requested range not satisfiable" },
			{ 417, "Expectation Failed" },
			{ 499, "Linden Catch-All" },
			{ 500, "Internal Server Error" },
			{ 501, "Not Implemented" },
			{ 502, "Bad Gateway" },
			{ 503, "Service Unavailable" },
			{ 504, "Gateway Time-out" },
			{ 505, "HTTP Version not supported" }
		};
	static const int http_errors_count(sizeof(http_errors) / sizeof(http_errors[0]));
	
	if (*this)
	{
		return std::string("");
	}
	switch (mType)
	{
	case EXT_CURL_EASY:
		return std::string(curl_easy_strerror(CURLcode(mStatus)));

	case EXT_CURL_MULTI:
		return std::string(curl_multi_strerror(CURLMcode(mStatus)));

	case LLCORE:
		if (mStatus >= 0 && mStatus < llcore_errors_count)
		{
			return std::string(llcore_errors[mStatus]);
		}
		break;

	default:
		if (isHttpStatus())
		{
			// Binary search for the error code and string
			int bottom(0), top(http_errors_count);
			while (true)
			{
				int at((bottom + top) / 2);
				if (mType == http_errors[at].mCode)
				{
					return std::string(http_errors[at].mText);
				}
				if (at == bottom)
				{
					break;
				}
				else if (mType < http_errors[at].mCode)
				{
					top = at;
				}
				else
				{
					bottom = at;
				}
			}
		}
		break;
	}
	return std::string("Unknown error");
}


std::string HttpStatus::toTerseString() const
{
	std::ostringstream result;

	unsigned int error_value((unsigned short) mStatus);
	
	switch (mType)
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
			error_value = mType;
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

	return ((isHttpStatus() && mType >= 499 && mType <= 599) ||	// Include special 499 in retryables
			*this == cant_connect ||	// Connection reset/endpoint problems
			*this == cant_res_proxy ||	// DNS problems
			*this == cant_res_host ||	// DNS problems
			*this == send_error ||		// General socket problems 
			*this == recv_error ||		// General socket problems 
			*this == upload_failed ||	// Transport problem
			*this == op_timedout ||		// Timer expired
			*this == post_error ||		// Transport problem
			*this == partial_file ||	// Data inconsistency in response
			*this == inv_cont_range ||	// Short data read disagrees with content-range
			*this == inv_status);		// Inv status can reflect internal state problem in libcurl
}

} // end namespace LLCore

