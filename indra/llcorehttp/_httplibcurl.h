/**
 * @file _httplibcurl.h
 * @brief Declarations for internal class providing libcurl transport.
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

#ifndef	_LLCORE_HTTP_LIBCURL_H_
#define	_LLCORE_HTTP_LIBCURL_H_

#include "linden_common.h"		// Modifies curl/curl.h interfaces

#include <curl/curl.h>
#include <curl/multi.h>

#include <set>


namespace LLCore
{


class HttpService;
class HttpPolicy;
class HttpOpRequest;
class HttpHeaders;


/// Implements libcurl-based transport for an HttpService instance.
class HttpLibcurl
{
public:
	HttpLibcurl(HttpService * service);
	virtual ~HttpLibcurl();

private:
	HttpLibcurl(const HttpLibcurl &);			// Not defined
	void operator=(const HttpLibcurl &);		// Not defined

public:
	static void init();
	static void term();

	void processTransport();
	void addOp(HttpOpRequest * op);

	int activeCount() const;

protected:
	void completeRequest(CURLM * multi_handle, CURL * handle, CURLcode status);
	
protected:
	typedef std::set<HttpOpRequest *> active_set_t;
	
protected:
	HttpService *		mService;				// Simple reference, not owner
	active_set_t		mActiveOps;
	CURLM *				mMultiHandles[1];
};  // end class HttpLibcurl


// ---------------------------------------
// Free functions
// ---------------------------------------


curl_slist * append_headers_to_slist(const HttpHeaders *, curl_slist * slist);


}  // end namespace LLCore

#endif // _LLCORE_HTTP_LIBCURL_H_
