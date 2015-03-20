/**
* @file llhttpsdhandler.h
* @brief Public-facing declarations for the HttpHandler class
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

#ifndef	_LLHTTPSDHANDLER_H_
#define	_LLHTTPSDHANDLER_H_
#include "httpcommon.h"
#include "httphandler.h"
#include "lluri.h"

/// 
///
class LLHttpSDHandler : public LLCore::HttpHandler
{
public:
	LLHttpSDHandler(const LLURI &uri);

	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);
	
	inline const LLURI &getUri() const
	{
		return mUri;
	}

protected:
	virtual void onSuccess(LLCore::HttpResponse * response, LLSD &content) = 0;
	virtual void onFailure(LLCore::HttpResponse * response, LLCore::HttpStatus status) = 0;

private:
	LLURI	mUri;
};


#endif