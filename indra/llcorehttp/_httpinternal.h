/**
 * @file _httpinternal.h
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


// --------------------------------------------------------------------
// General library to-do list
//
// - Implement policy classes.  Structure is mostly there just didn't
//   need it for the first consumer.
// - Consider Removing 'priority' from the request interface.  Its use
//   in an always active class can lead to starvation of low-priority
//   requests.  Requires coodination of priority values across all
//   components that share a class.  Changing priority across threads
//   is slightly expensive (relative to gain) and hasn't been completely
//   implemented.  And the major user of priority, texture fetches,
//   may not really need it.
// - Set/get for global policy and policy classes is clumsy.  Rework
//   it heading in a direction that allows for more dynamic behavior.
// - Move HttpOpRequest::prepareRequest() to HttpLibcurl for the
//   pedantic.
// - Update downloader and other long-duration services are going to
//   need a progress notification.  Initial idea is to introduce a
//   'repeating request' which can piggyback on another request and
//   persist until canceled or carrier completes.  Current queue
//   structures allow an HttpOperation object to be enqueued
//   repeatedly, so...
// - Investigate making c-ares' re-implementation of a resolver library
//   more resilient or more intelligent on Mac.  Part of the DNS failure
//   lies in here.  The mechanism also looks a little less dynamic
//   than needed in an environments where networking is changing.
// - Global optimizations:  'borrowing' connections from other classes,
//   HTTP pipelining.
// - Dynamic/control system stuff:  detect problems and self-adjust.
//   This won't help in the face of the router problems we've looked
//   at, however.  Detect starvation due to UDP activity and provide
//   feedback to it.
//
// Integration to-do list
// - LLTextureFetch still needs a major refactor.  The use of
//   LLQueuedThread makes it hard to inspect workers and do the
//   resource waiting we're now doing.  Rebuild along simpler lines
//   some of which are suggested in new commentary at the top of
//   the main source file.
// - Expand areas of usage eventually leading to the removal of LLCurl.
//   Rough order of expansion:
//   .  Mesh fetch
//   .  Avatar names
//   .  Group membership lists
//   .  Caps access in general
//   .  'The rest'
// - Adapt texture cache, image decode and other image consumers to
//   the BufferArray model to reduce data copying.  Alternatively,
//   adapt this library to something else.
//
// --------------------------------------------------------------------


// If '1', internal ready queues will not order ready
// requests by priority, instead it's first-come-first-served.
// Reprioritization requests have the side-effect of then
// putting the modified request at the back of the ready queue.

#define	LLCORE_HTTP_READY_QUEUE_IGNORES_PRIORITY		1


namespace LLCore
{

// Maxium number of policy classes that can be defined.
// *TODO:  Currently limited to the default class, extend.
const int HTTP_POLICY_CLASS_LIMIT = 1;

// Debug/informational tracing.  Used both
// as a global option and in per-request traces.
const int HTTP_TRACE_OFF = 0;
const int HTTP_TRACE_LOW = 1;
const int HTTP_TRACE_CURL_HEADERS = 2;
const int HTTP_TRACE_CURL_BODIES = 3;

const int HTTP_TRACE_MIN = HTTP_TRACE_OFF;
const int HTTP_TRACE_MAX = HTTP_TRACE_CURL_BODIES;

// Request retry limits
//
// At a minimum, retries need to extend past any throttling
// window we're expecting from central services.  In the case
// of Linden services running through the caps routers, there's
// a five-second or so window for throttling with some spillover.
// We want to span a few windows to allow transport to slow
// after onset of the throttles and then recover without a final
// failure.  Other systems may need other constants.
const int HTTP_RETRY_COUNT_DEFAULT = 8;
const int HTTP_RETRY_COUNT_MIN = 0;
const int HTTP_RETRY_COUNT_MAX = 100;

const int HTTP_REDIRECTS_DEFAULT = 10;

// Timeout value used for both connect and protocol exchange.
// Retries and time-on-queue are not included and aren't
// accounted for.
const long HTTP_REQUEST_TIMEOUT_DEFAULT = 30L;
const long HTTP_REQUEST_TIMEOUT_MIN = 0L;
const long HTTP_REQUEST_TIMEOUT_MAX = 3600L;

// Limits on connection counts
const int HTTP_CONNECTION_LIMIT_DEFAULT = 8;
const int HTTP_CONNECTION_LIMIT_MIN = 1;
const int HTTP_CONNECTION_LIMIT_MAX = 256;

// Tuning parameters

// Time worker thread sleeps after a pass through the
// request, ready and active queues.
const int HTTP_SERVICE_LOOP_SLEEP_NORMAL_MS = 2;

// Block allocation size (a tuning parameter) is found
// in bufferarray.h.

// Compatibility controls
const bool HTTP_ENABLE_LINKSYS_WRT54G_V5_DNS_FIX = true;

}  // end namespace LLCore

#endif	// _LLCORE_HTTP_INTERNAL_H_
