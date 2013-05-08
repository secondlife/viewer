/** 
 * @file llhttpclientadapter.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llhttpclientadapter.h"
#include "llhttpclient.h"

LLHTTPClientAdapter::~LLHTTPClientAdapter() 
{
}

void LLHTTPClientAdapter::get(const std::string& url, LLCurl::ResponderPtr responder) 
{
	LLSD empty_pragma_header;
	// Pragma is required to stop curl adding "no-cache"
	// Space is required to stop llurlrequest from turnning off proxying
	empty_pragma_header["Pragma"] = " "; 
	LLHTTPClient::get(url, responder, empty_pragma_header);
}

void LLHTTPClientAdapter::get(const std::string& url, LLCurl::ResponderPtr responder, const LLSD& headers) 
{
	LLSD empty_pragma_header = headers;
	if (!empty_pragma_header.has("Pragma"))
	{
		// as above
		empty_pragma_header["Pragma"] = " ";
	}
	LLHTTPClient::get(url, responder, empty_pragma_header);
}

void LLHTTPClientAdapter::put(const std::string& url, const LLSD& body, LLCurl::ResponderPtr responder) 
{
	LLHTTPClient::put(url, body, responder);
}

