/** 
 * @file llhttpconstants.h
 * @brief Constants for HTTP requests and responses
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2013, Linden Research, Inc.
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

#ifndef LL_HTTP_CONSTANTS_H
#define LL_HTTP_CONSTANTS_H

#include "stdtypes.h"

/////// HTTP STATUS CODES ///////

// Standard errors from HTTP spec:
// http://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html#sec6.1
const S32 HTTP_CONTINUE = 100;
const S32 HTTP_SWITCHING_PROTOCOLS = 101;

// Success
const S32 HTTP_OK = 200;
const S32 HTTP_CREATED = 201;
const S32 HTTP_ACCEPTED = 202;
const S32 HTTP_NON_AUTHORITATIVE_INFORMATION = 203;
const S32 HTTP_NO_CONTENT = 204;
const S32 HTTP_RESET_CONTENT = 205;
const S32 HTTP_PARTIAL_CONTENT = 206;

// Redirection
const S32 HTTP_MULTIPLE_CHOICES = 300;
const S32 HTTP_MOVED_PERMANENTLY = 301;
const S32 HTTP_FOUND = 302;
const S32 HTTP_SEE_OTHER = 303;
const S32 HTTP_NOT_MODIFIED = 304;
const S32 HTTP_USE_PROXY = 305;
const S32 HTTP_TEMPORARY_REDIRECT = 307;

// Client Error
const S32 HTTP_BAD_REQUEST = 400;
const S32 HTTP_UNAUTHORIZED = 401;
const S32 HTTP_PAYMENT_REQUIRED = 402;
const S32 HTTP_FORBIDDEN = 403;
const S32 HTTP_NOT_FOUND = 404;
const S32 HTTP_METHOD_NOT_ALLOWED = 405;
const S32 HTTP_NOT_ACCEPTABLE = 406;
const S32 HTTP_PROXY_AUTHENTICATION_REQUIRED = 407;
const S32 HTTP_REQUEST_TIME_OUT = 408;
const S32 HTTP_CONFLICT = 409;
const S32 HTTP_GONE = 410;
const S32 HTTP_LENGTH_REQUIRED = 411;
const S32 HTTP_PRECONDITION_FAILED = 412;
const S32 HTTP_REQUEST_ENTITY_TOO_LARGE = 413;
const S32 HTTP_REQUEST_URI_TOO_LARGE = 414;
const S32 HTTP_UNSUPPORTED_MEDIA_TYPE = 415;
const S32 HTTP_REQUESTED_RANGE_NOT_SATISFIABLE = 416;
const S32 HTTP_EXPECTATION_FAILED = 417;

// Server Error
const S32 HTTP_INTERNAL_SERVER_ERROR = 500;
const S32 HTTP_NOT_IMPLEMENTED = 501;
const S32 HTTP_BAD_GATEWAY = 502;
const S32 HTTP_SERVICE_UNAVAILABLE = 503;
const S32 HTTP_GATEWAY_TIME_OUT = 504;
const S32 HTTP_VERSION_NOT_SUPPORTED = 505;

// We combine internal process errors with status codes
// These status codes should not be sent over the wire
//   and indicate something went wrong internally.
// If you get these they are not normal.
const S32 HTTP_INTERNAL_CURL_ERROR = 498;
const S32 HTTP_INTERNAL_ERROR = 499;


////// HTTP Methods //////

extern const std::string HTTP_VERB_INVALID;
extern const std::string HTTP_VERB_HEAD;
extern const std::string HTTP_VERB_GET;
extern const std::string HTTP_VERB_PUT;
extern const std::string HTTP_VERB_POST;
extern const std::string HTTP_VERB_DELETE;
extern const std::string HTTP_VERB_MOVE;
extern const std::string HTTP_VERB_OPTIONS;

enum EHTTPMethod
{
	HTTP_INVALID = 0,
	HTTP_HEAD,
	HTTP_GET,
	HTTP_PUT,
	HTTP_POST,
	HTTP_DELETE,
	HTTP_MOVE, // Caller will need to set 'Destination' header
	HTTP_OPTIONS,
	HTTP_METHOD_COUNT
};

const std::string& httpMethodAsVerb(EHTTPMethod method);
bool isHttpInformationalStatus(S32 status);
bool isHttpGoodStatus(S32 status);
bool isHttpRedirectStatus(S32 status);
bool isHttpClientErrorStatus(S32 status);
bool isHttpServerErrorStatus(S32 status);

// Parses 'Retry-After' header contents and returns seconds until retry should occur.
bool getSecondsUntilRetryAfter(const std::string& retry_after, F32& seconds_to_wait);

//// HTTP Headers /////

extern const std::string HTTP_HEADER_ACCEPT;
extern const std::string HTTP_HEADER_ACCEPT_CHARSET;
extern const std::string HTTP_HEADER_ACCEPT_ENCODING;
extern const std::string HTTP_HEADER_ACCEPT_LANGUAGE;
extern const std::string HTTP_HEADER_ACCEPT_RANGES;
extern const std::string HTTP_HEADER_AGE;
extern const std::string HTTP_HEADER_ALLOW;
extern const std::string HTTP_HEADER_AUTHORIZATION;
extern const std::string HTTP_HEADER_CACHE_CONTROL;
extern const std::string HTTP_HEADER_CONNECTION;
extern const std::string HTTP_HEADER_CONTENT_DESCRIPTION;
extern const std::string HTTP_HEADER_CONTENT_ENCODING;
extern const std::string HTTP_HEADER_CONTENT_ID;
extern const std::string HTTP_HEADER_CONTENT_LANGUAGE;
extern const std::string HTTP_HEADER_CONTENT_LENGTH;
extern const std::string HTTP_HEADER_CONTENT_LOCATION;
extern const std::string HTTP_HEADER_CONTENT_MD5;
extern const std::string HTTP_HEADER_CONTENT_RANGE;
extern const std::string HTTP_HEADER_CONTENT_TRANSFER_ENCODING;
extern const std::string HTTP_HEADER_CONTENT_TYPE;
extern const std::string HTTP_HEADER_COOKIE;
extern const std::string HTTP_HEADER_DATE;
extern const std::string HTTP_HEADER_DESTINATION;
extern const std::string HTTP_HEADER_ETAG;
extern const std::string HTTP_HEADER_EXPECT;
extern const std::string HTTP_HEADER_EXPIRES;
extern const std::string HTTP_HEADER_FROM;
extern const std::string HTTP_HEADER_HOST;
extern const std::string HTTP_HEADER_IF_MATCH;
extern const std::string HTTP_HEADER_IF_MODIFIED_SINCE;
extern const std::string HTTP_HEADER_IF_NONE_MATCH;
extern const std::string HTTP_HEADER_IF_RANGE;
extern const std::string HTTP_HEADER_IF_UNMODIFIED_SINCE;
extern const std::string HTTP_HEADER_KEEP_ALIVE;
extern const std::string HTTP_HEADER_LAST_MODIFIED;
extern const std::string HTTP_HEADER_LOCATION;
extern const std::string HTTP_HEADER_MAX_FORWARDS;
extern const std::string HTTP_HEADER_MIME_VERSION;
extern const std::string HTTP_HEADER_PRAGMA;
extern const std::string HTTP_HEADER_PROXY_AUTHENTICATE;
extern const std::string HTTP_HEADER_PROXY_AUTHORIZATION;
extern const std::string HTTP_HEADER_RANGE;
extern const std::string HTTP_HEADER_REFERER;
extern const std::string HTTP_HEADER_RETRY_AFTER;
extern const std::string HTTP_HEADER_SERVER;
extern const std::string HTTP_HEADER_SET_COOKIE;
extern const std::string HTTP_HEADER_TE;
extern const std::string HTTP_HEADER_TRAILER;
extern const std::string HTTP_HEADER_TRANSFER_ENCODING;
extern const std::string HTTP_HEADER_UPGRADE;
extern const std::string HTTP_HEADER_USER_AGENT;
extern const std::string HTTP_HEADER_VARY;
extern const std::string HTTP_HEADER_VIA;
extern const std::string HTTP_HEADER_WARNING;
extern const std::string HTTP_HEADER_WWW_AUTHENTICATE;

// Sadly, our proxied headers do not follow http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
// We need to deal with lowercase headers
extern const std::string HTTP_HEADER_LOWER_ACCEPT_LANGUAGE;
extern const std::string HTTP_HEADER_LOWER_CACHE_CONTROL;
extern const std::string HTTP_HEADER_LOWER_CONTENT_LENGTH;
extern const std::string HTTP_HEADER_LOWER_CONTENT_TYPE;
extern const std::string HTTP_HEADER_LOWER_HOST;
extern const std::string HTTP_HEADER_LOWER_USER_AGENT;
extern const std::string HTTP_HEADER_LOWER_X_FORWARDED_FOR;

//// HTTP Content Types ////

extern const std::string HTTP_CONTENT_LLSD_XML;
extern const std::string HTTP_CONTENT_OCTET_STREAM;
extern const std::string HTTP_CONTENT_XML;
extern const std::string HTTP_CONTENT_JSON;
extern const std::string HTTP_CONTENT_TEXT_HTML;
extern const std::string HTTP_CONTENT_TEXT_HTML_UTF8;
extern const std::string HTTP_CONTENT_TEXT_PLAIN_UTF8;
extern const std::string HTTP_CONTENT_TEXT_LLSD;
extern const std::string HTTP_CONTENT_TEXT_XML;
extern const std::string HTTP_CONTENT_TEXT_LSL;
extern const std::string HTTP_CONTENT_TEXT_PLAIN;
extern const std::string HTTP_CONTENT_IMAGE_X_J2C;
extern const std::string HTTP_CONTENT_IMAGE_J2C;
extern const std::string HTTP_CONTENT_IMAGE_JPEG;
extern const std::string HTTP_CONTENT_IMAGE_PNG;
extern const std::string HTTP_CONTENT_IMAGE_BMP;

#endif
