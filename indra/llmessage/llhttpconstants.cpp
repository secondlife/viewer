/** 
 * @file llhttpconstants.cpp
 * @brief Implementation of the HTTP request / response constant lookups
 *
 * $LicenseInfo:firstyear=2013&license=viewergpl$
 * 
 * Copyright (c) 2013, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llhttpconstants.h"
#include "lltimer.h"

// for curl_getdate() (apparently parsing RFC 1123 dates is hard)
#include <curl/curl.h>

const std::string HTTP_HEADER_ACCEPT("Accept");
const std::string HTTP_HEADER_ACCEPT_CHARSET("Accept-Charset");
const std::string HTTP_HEADER_ACCEPT_ENCODING("Accept-Encoding");
const std::string HTTP_HEADER_ACCEPT_LANGUAGE("Accept-Language");
const std::string HTTP_HEADER_ACCEPT_RANGES("Accept-Ranges");
const std::string HTTP_HEADER_AGE("Age");
const std::string HTTP_HEADER_ALLOW("Allow");
const std::string HTTP_HEADER_AUTHORIZATION("Authorization");
const std::string HTTP_HEADER_CACHE_CONTROL("Cache-Control");
const std::string HTTP_HEADER_CONNECTION("Connection");
const std::string HTTP_HEADER_CONTENT_DESCRIPTION("Content-Description");
const std::string HTTP_HEADER_CONTENT_ENCODING("Content-Encoding");
const std::string HTTP_HEADER_CONTENT_ID("Content-ID");
const std::string HTTP_HEADER_CONTENT_LANGUAGE("Content-Language");
const std::string HTTP_HEADER_CONTENT_LENGTH("Content-Length");
const std::string HTTP_HEADER_CONTENT_LOCATION("Content-Location");
const std::string HTTP_HEADER_CONTENT_MD5("Content-MD5");
const std::string HTTP_HEADER_CONTENT_RANGE("Content-Range");
const std::string HTTP_HEADER_CONTENT_TRANSFER_ENCODING("Content-Transfer-Encoding");
const std::string HTTP_HEADER_CONTENT_TYPE("Content-Type");
const std::string HTTP_HEADER_COOKIE("Cookie");
const std::string HTTP_HEADER_DATE("Date");
const std::string HTTP_HEADER_DESTINATION("Destination");
const std::string HTTP_HEADER_ETAG("ETag");
const std::string HTTP_HEADER_EXPECT("Expect");
const std::string HTTP_HEADER_EXPIRES("Expires");
const std::string HTTP_HEADER_FROM("From");
const std::string HTTP_HEADER_HOST("Host");
const std::string HTTP_HEADER_IF_MATCH("If-Match");
const std::string HTTP_HEADER_IF_MODIFIED_SINCE("If-Modified-Since");
const std::string HTTP_HEADER_IF_NONE_MATCH("If-None-Match");
const std::string HTTP_HEADER_IF_RANGE("If-Range");
const std::string HTTP_HEADER_IF_UNMODIFIED_SINCE("If-Unmodified-Since");
const std::string HTTP_HEADER_KEEP_ALIVE("Keep-Alive");
const std::string HTTP_HEADER_LAST_MODIFIED("Last-Modified");
const std::string HTTP_HEADER_LOCATION("Location");
const std::string HTTP_HEADER_MAX_FORWARDS("Max-Forwards");
const std::string HTTP_HEADER_MIME_VERSION("MIME-Version");
const std::string HTTP_HEADER_PRAGMA("Pragma");
const std::string HTTP_HEADER_PROXY_AUTHENTICATE("Proxy-Authenticate");
const std::string HTTP_HEADER_PROXY_AUTHORIZATION("Proxy-Authorization");
const std::string HTTP_HEADER_RANGE("Range");
const std::string HTTP_HEADER_REFERER("Referer");
const std::string HTTP_HEADER_RETRY_AFTER("Retry-After");
const std::string HTTP_HEADER_SERVER("Server");
const std::string HTTP_HEADER_SET_COOKIE("Set-Cookie");
const std::string HTTP_HEADER_TE("TE");
const std::string HTTP_HEADER_TRAILER("Trailer");
const std::string HTTP_HEADER_TRANSFER_ENCODING("Transfer-Encoding");
const std::string HTTP_HEADER_UPGRADE("Upgrade");
const std::string HTTP_HEADER_USER_AGENT("User-Agent");
const std::string HTTP_HEADER_VARY("Vary");
const std::string HTTP_HEADER_VIA("Via");
const std::string HTTP_HEADER_WARNING("Warning");
const std::string HTTP_HEADER_WWW_AUTHENTICATE("WWW-Authenticate");


// Sadly, our proxied headers do not follow http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
// We need to deal with lowercase headers
const std::string HTTP_HEADER_LOWER_ACCEPT_LANGUAGE("accept-language");
const std::string HTTP_HEADER_LOWER_CACHE_CONTROL("cache-control");
const std::string HTTP_HEADER_LOWER_CONTENT_LENGTH("content-length");
const std::string HTTP_HEADER_LOWER_CONTENT_TYPE("content-type");
const std::string HTTP_HEADER_LOWER_HOST("host");
const std::string HTTP_HEADER_LOWER_USER_AGENT("user-agent");
const std::string HTTP_HEADER_LOWER_X_FORWARDED_FOR("x-forwarded-for");

const std::string HTTP_CONTENT_LLSD_XML("application/llsd+xml");
const std::string HTTP_CONTENT_OCTET_STREAM("application/octet-stream");
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

const std::string HTTP_VERB_INVALID("(invalid)");
const std::string HTTP_VERB_HEAD("HEAD");
const std::string HTTP_VERB_GET("GET");
const std::string HTTP_VERB_PUT("PUT");
const std::string HTTP_VERB_POST("POST");
const std::string HTTP_VERB_DELETE("DELETE");
const std::string HTTP_VERB_MOVE("MOVE");
const std::string HTTP_VERB_OPTIONS("OPTIONS");

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
		HTTP_VERB_OPTIONS
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

	seconds_to_wait = (F32)date - (F32)LLTimer::getTotalSeconds();
	return true;
}

