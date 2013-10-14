/**
 * @file _httpopcancel.h
 * @brief Internal declarations for the HttpOpCancel subclass
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

#ifndef	_LLCORE_HTTP_OPCANCEL_H_
#define	_LLCORE_HTTP_OPCANCEL_H_


#include "linden_common.h"		// Modifies curl/curl.h interfaces

#include "httpcommon.h"

#include <curl/curl.h>

#include "_httpoperation.h"
#include "_refcounted.h"


namespace LLCore
{


/// HttpOpCancel requests that a previously issued request
/// be canceled, if possible.  This includes active requests
/// that may be in the middle of an HTTP transaction.  Any
/// completed request will not be canceled and will return
/// its final status unchanged and *this* request will complete
/// with an HE_HANDLE_NOT_FOUND error status.

class HttpOpCancel : public HttpOperation
{
public:
	/// @param	handle	Handle of previously-issued request to
	///					be canceled.
	HttpOpCancel(HttpHandle handle);

protected:
	virtual ~HttpOpCancel();							// Use release()
	
private:
	HttpOpCancel(const HttpOpCancel &);					// Not defined
	void operator=(const HttpOpCancel &);				// Not defined

public:
	virtual void stageFromRequest(HttpService *);
			
public:
	// Request data
	HttpHandle			mHandle;
};  // end class HttpOpCancel


}   // end namespace LLCore

#endif	// _LLCORE_HTTP_OPCANCEL_H_

