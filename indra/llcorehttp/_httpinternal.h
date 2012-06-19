/**
 * @file httpinternal.h
 * @brief Implementation constants and magic numbers
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef	_LLCORE_HTTP_INTERNAL_H_
#define	_LLCORE_HTTP_INTERNAL_H_


// If you find this included in a public interface header,
// something wrong is probably happening.


namespace LLCore
{

// Maxium number of policy classes that can be defined.
// *FIXME:  Currently limited to the default class, extend.
const int POLICY_CLASS_LIMIT = 1;

// Debug/informational tracing.  Used both
// as a global option and in per-request traces.
const int TRACE_OFF = 0;
const int TRACE_LOW = 1;
const int TRACE_CURL_HEADERS = 2;
const int TRACE_CURL_BODIES = 3;

const int TRACE_MIN = TRACE_OFF;
const int TRACE_MAX = TRACE_CURL_BODIES;

// Request retry limits
const int DEFAULT_RETRY_COUNT = 5;
const int LIMIT_RETRY_MIN = 0;
const int LIMIT_RETRY_MAX = 100;

const int DEFAULT_HTTP_REDIRECTS = 10;

// Timeout value used for both connect and protocol exchange.
// Retries and time-on-queue are not included and aren't
// accounted for.
const long DEFAULT_TIMEOUT = 30L;
const long LIMIT_TIMEOUT_MIN = 0L;
const long LIMIT_TIMEOUT_MAX = 3600L;

// Limits on connection counts
const int DEFAULT_CONNECTIONS = 8;
const int LIMIT_CONNECTIONS_MIN = 1;
const int LIMIT_CONNECTIONS_MAX = 256;

// Tuning parameters

// Time worker thread sleeps after a pass through the
// request, ready and active queues.
const int LOOP_SLEEP_NORMAL_MS = 2;

// Block allocation size (a tuning parameter) is found
// in bufferarray.h.

}  // end namespace LLCore

#endif	// _LLCORE_HTTP_INTERNAL_H_
