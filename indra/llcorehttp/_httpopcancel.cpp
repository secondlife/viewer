/**
 * @file _httpopcancel.cpp
 * @brief Definitions for internal class HttpOpCancel
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

#include "_httpopcancel.h"

#include "httpcommon.h"
#include "httphandler.h"
#include "httpresponse.h"

#include "_httpservice.h"


namespace LLCore
{


// ==================================
// HttpOpCancel
// ==================================


HttpOpCancel::HttpOpCancel(HttpHandle handle)
	: HttpOperation(),
	  mHandle(handle)
{}


HttpOpCancel::~HttpOpCancel()
{}


// Immediately search for the request on various queues
// and cancel operations if found.  Return the status of
// the search and cancel as the status of this request.
// The canceled request will return a canceled status to
// its handler.
void HttpOpCancel::stageFromRequest(HttpService * service)
{
	if (! service->cancel(mHandle))
	{
		mStatus = HttpStatus(HttpStatus::LLCORE, HE_HANDLE_NOT_FOUND);
	}
	
	addAsReply();
}


}   // end namespace LLCore

		
