/** 
 * @file llhttpstatuscodes.h
 * @brief Constants for HTTP status codes
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_HTTP_STATUS_CODES_H
#define LL_HTTP_STATUS_CODES_H

#include "stdtypes.h"

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
const S32 HTTP_INTERNAL_ERROR = 499;

#endif
