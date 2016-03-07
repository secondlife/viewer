/** 
 * @file llhttpconstants.cpp
 * @brief Implementation of the HTTP request / response constant lookups
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013-2014, Linden Research, Inc.
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
#include "lltimer.h"

// for curl_getdate() (apparently parsing RFC 1123 dates is hard)
#include <curl/curl.h>

// Outgoing headers. Do *not* use these to check incoming headers.
// For incoming headers, use the lower-case headers, below.
const std::string HTTP_OUT_HEADER_ACCEPT("Accept");
const std::string HTTP_OUT_HEADER_ACCEPT_CHARSET("Accept-Charset");
const std::string HTTP_OUT_HEADER_ACCEPT_ENCODING("Accept-Encoding");
const std::string HTTP_OUT_HEADER_ACCEPT_LANGUAGE("Accept-Language");
const std::string HTTP_OUT_HEADER_ACCEPT_RANGES("Accept-Ranges");
const std::string HTTP_OUT_HEADER_AGE("Age");
const std::string HTTP_OUT_HEADER_ALLOW("Allow");
const std::string HTTP_OUT_HEADER_AUTHORIZATION("Authorization");
const std::string HTTP_OUT_HEADER_CACHE_CONTROL("Cache-Control");
const std::string HTTP_OUT_HEADER_CONNECTION("Connection");
const std::string HTTP_OUT_HEADER_CONTENT_DESCRIPTION("Content-Description");
const std::string HTTP_OUT_HEADER_CONTENT_ENCODING("Content-Encoding");
const std::string HTTP_OUT_HEADER_CONTENT_ID("Content-ID");
const std::string HTTP_OUT_HEADER_CONTENT_LANGUAGE("Content-Language");
const std::string HTTP_OUT_HEADER_CONTENT_LENGTH("Content-Length");
const std::string HTTP_OUT_HEADER_CONTENT_LOCATION("Content-Location");
const std::string HTTP_OUT_HEADER_CONTENT_MD5("Content-MD5");
const std::string HTTP_OUT_HEADER_CONTENT_RANGE("Content-Range");
const std::string HTTP_OUT_HEADER_CONTENT_TRANSFER_ENCODING("Content-Transfer-Encoding");
const std::string HTTP_OUT_HEADER_CONTENT_TYPE("Content-Type");
const std::string HTTP_OUT_HEADER_COOKIE("Cookie");
const std::string HTTP_OUT_HEADER_DATE("Date");
const std::string HTTP_OUT_HEADER_DESTINATION("Destination");
const std::string HTTP_OUT_HEADER_ETAG("ETag");
const std::string HTTP_OUT_HEADER_EXPECT("Expect");
const std::string HTTP_OUT_HEADER_EXPIRES("Expires");
const std::string HTTP_OUT_HEADER_FROM("From");
const std::string HTTP_OUT_HEADER_HOST("Host");
const std::string HTTP_OUT_HEADER_IF_MATCH("If-Match");
const std::string HTTP_OUT_HEADER_IF_MODIFIED_SINCE("If-Modified-Since");
const std::string HTTP_OUT_HEADER_IF_NONE_MATCH("If-None-Match");
const std::string HTTP_OUT_HEADER_IF_RANGE("If-Range");
const std::string HTTP_OUT_HEADER_IF_UNMODIFIED_SINCE("If-Unmodified-Since");
const std::string HTTP_OUT_HEADER_KEEP_ALIVE("Keep-Alive");
const std::string HTTP_OUT_HEADER_LAST_MODIFIED("Last-Modified");
const std::string HTTP_OUT_HEADER_LOCATION("Location");
const std::string HTTP_OUT_HEADER_MAX_FORWARDS("Max-Forwards");
const std::string HTTP_OUT_HEADER_MIME_VERSION("MIME-Version");
const std::string HTTP_OUT_HEADER_PRAGMA("Pragma");
const std::string HTTP_OUT_HEADER_PROXY_AUTHENTICATE("Proxy-Authenticate");
const std::string HTTP_OUT_HEADER_PROXY_AUTHORIZATION("Proxy-Authorization");
const std::string HTTP_OUT_HEADER_RANGE("Range");
const std::string HTTP_OUT_HEADER_REFERER("Referer");
const std::string HTTP_OUT_HEADER_RETRY_AFTER("Retry-After");
const std::string HTTP_OUT_HEADER_SERVER("Server");
const std::string HTTP_OUT_HEADER_SET_COOKIE("Set-Cookie");
const std::string HTTP_OUT_HEADER_TE("TE");
const std::string HTTP_OUT_HEADER_TRAILER("Trailer");
const std::string HTTP_OUT_HEADER_TRANSFER_ENCODING("Transfer-Encoding");
const std::string HTTP_OUT_HEADER_UPGRADE("Upgrade");
const std::string HTTP_OUT_HEADER_USER_AGENT("User-Agent");
const std::string HTTP_OUT_HEADER_VARY("Vary");
const std::string HTTP_OUT_HEADER_VIA("Via");
const std::string HTTP_OUT_HEADER_WARNING("Warning");
const std::string HTTP_OUT_HEADER_WWW_AUTHENTICATE("WWW-Authenticate");

// Incoming headers are normalized to lower-case.
const std::string HTTP_IN_HEADER_ACCEPT_LANGUAGE("accept-language");
const std::string HTTP_IN_HEADER_CACHE_CONTROL("cache-control");
const std::string HTTP_IN_HEADER_CONTENT_LENGTH("content-length");
const std::string HTTP_IN_HEADER_CONTENT_LOCATION("content-location");
const std::string HTTP_IN_HEADER_CONTENT_TYPE("content-type");
const std::string HTTP_IN_HEADER_HOST("host");
const std::string HTTP_IN_HEADER_LOCATION("location");
const std::string HTTP_IN_HEADER_RETRY_AFTER("retry-after");
const std::string HTTP_IN_HEADER_SET_COOKIE("set-cookie");
const std::string HTTP_IN_HEADER_USER_AGENT("user-agent");
const std::string HTTP_IN_HEADER_X_FORWARDED_FOR("x-forwarded-for");

const std::string HTTP_CONTENT_LLSD_XML("application/llsd+xml");
const std::string HTTP_CONTENT_OCTET_STREAM("application/octet-stream");
const std::string HTTP_CONTENT_VND_LL_MESH("application/vnd.ll.mesh");
const std::string HTTP_CONTENT_XML("application/xml");
const std::string HTTP_CONTENT_JSON("application/json");
const std::string HTTP_CONTENT_TEXT_HTML("text/html");
const std::string HTTP_CONTENT_TEXT_HTML_UTF8("text/html; charset=utf-8");
const std::string HTTP_CONTENT_TEXT_PLAIN_UTF8("text/plain; charset=utf-8");
const std::string HTTP_CONTENT_TEXT_LLSD("text/llsd");
const std::string HTTP_CONTENT_TEXT_XML("text/xml");
const std::string HTTP_CONTENT_TEXT_LSL("text/lsl");
const std::string HTTP_CONTENT_TEXT_PLAIN("text/plain");
const std::string HTTP_CONTENT_IMAGE_X_J2C("image/x-j2c");
const std::string HTTP_CONTENT_IMAGE_J2C("image/j2c");
const std::string HTTP_CONTENT_IMAGE_JPEG("image/jpeg");
const std::string HTTP_CONTENT_IMAGE_PNG("image/png");
const std::string HTTP_CONTENT_IMAGE_BMP("image/bmp");

const std::string HTTP_NO_CACHE("no-cache");
const std::string HTTP_NO_CACHE_CONTROL("no-cache, max-age=0");

const std::string HTTP_VERB_INVALID("(invalid)");
const std::string HTTP_VERB_HEAD("HEAD");
const std::string HTTP_VERB_GET("GET");
const std::string HTTP_VERB_PUT("PUT");
const std::string HTTP_VERB_POST("POST");
const std::string HTTP_VERB_DELETE("DELETE");
const std::string HTTP_VERB_MOVE("MOVE");
const std::string HTTP_VERB_OPTIONS("OPTIONS");
const std::string HTTP_VERB_PATCH("PATCH");
const std::string HTTP_VERB_COPY("COPY");

const std::string& httpMethodAsVerb(EHTTPMethod method)
{
	static const std::string VERBS[] =
	{
		HTTP_VERB_INVALID,
		HTTP_VERB_HEAD,
		HTTP_VERB_GET,
		HTTP_VERB_PUT,
		HTTP_VERB_POST,
		HTTP_VERB_DELETE,
		HTTP_VERB_MOVE,
		HTTP_VERB_OPTIONS,
		HTTP_VERB_PATCH,
		HTTP_VERB_COPY
	};
	if(((S32)method <=0) || ((S32)method >= HTTP_METHOD_COUNT))
	{
		return VERBS[0];
	}
	return VERBS[method];
}

bool isHttpInformationalStatus(S32 status)
{
	// Check for status 1xx.
	return((100 <= status) && (status < 200));
}

bool isHttpGoodStatus(S32 status)
{
	// Check for status 2xx.
	return((200 <= status) && (status < 300));
}

bool isHttpRedirectStatus(S32 status)
{
	// Check for status 3xx.
	return((300 <= status) && (status < 400));
}

bool isHttpClientErrorStatus(S32 status)
{
	// Status 499 is sometimes used for re-interpreted status 2xx errors
	// based on body content.  Treat these as potentially retryable 'server' status errors,
	// since we do not have enough context to know if this will always fail.
	if (HTTP_INTERNAL_ERROR == status) return false;

	// Check for status 5xx.
	return((400 <= status) && (status < 500));
}

bool isHttpServerErrorStatus(S32 status)
{
	// Status 499 is sometimes used for re-interpreted status 2xx errors.
	// Allow retry of these, since we don't have enough information in this
	// context to know if this will always fail.
	if (HTTP_INTERNAL_ERROR == status) return true;

	// Check for status 5xx.
	return((500 <= status) && (status < 600));
}

// Parses 'Retry-After' header contents and returns seconds until retry should occur.
bool getSecondsUntilRetryAfter(const std::string& retry_after, F32& seconds_to_wait)
{
	// *TODO:  This needs testing!   Not in use yet.
	// Examples of Retry-After headers:
	// Retry-After: Fri, 31 Dec 1999 23:59:59 GMT
	// Retry-After: 120

	// Check for number of seconds version, first:
	char* end = 0;
	// Parse as double
	double seconds = std::strtod(retry_after.c_str(), &end);
	if ( end != 0 && *end == 0 )
	{
		// Successful parse
		seconds_to_wait = (F32) seconds;
		return true;
	}

	// Parse rfc1123 date.
	time_t date = curl_getdate(retry_after.c_str(), NULL );
	if (-1 == date) return false;

	seconds_to_wait = (F64)date - LLTimer::getTotalSeconds();

	return true;
}

