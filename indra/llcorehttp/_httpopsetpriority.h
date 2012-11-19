/**
 * @file _httpsetpriority.h
 * @brief Internal declarations for HttpSetPriority
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

#ifndef	_LLCORE_HTTP_SETPRIORITY_H_
#define	_LLCORE_HTTP_SETPRIORITY_H_


#include "httpcommon.h"
#include "httprequest.h"
#include "_httpoperation.h"
#include "_refcounted.h"


namespace LLCore
{


/// HttpOpSetPriority is an immediate request that
/// searches the various queues looking for a given
/// request handle and changing it's priority if
/// found.
///
/// *NOTE:  This will very likely be removed in the near future
/// when priority is removed from the library.

class HttpOpSetPriority : public HttpOperation
{
public:
	HttpOpSetPriority(HttpHandle handle, HttpRequest::priority_t priority);

protected:
	virtual ~HttpOpSetPriority();

private:
	HttpOpSetPriority(const HttpOpSetPriority &);			// Not defined
	void operator=(const HttpOpSetPriority &);				// Not defined

public:
	virtual void stageFromRequest(HttpService *);

protected:
	// Request Data
	HttpHandle					mHandle;
	HttpRequest::priority_t		mPriority;
}; // end class HttpOpSetPriority

}  // end namespace LLCore

#endif	// _LLCORE_HTTP_SETPRIORITY_H_

