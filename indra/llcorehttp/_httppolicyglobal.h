/**
 * @file _httppolicyglobal.h
 * @brief Declarations for internal class defining global policy option.
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

#ifndef	_LLCORE_HTTP_POLICY_GLOBAL_H_
#define	_LLCORE_HTTP_POLICY_GLOBAL_H_


#include "httprequest.h"


namespace LLCore
{

class HttpPolicyGlobal
{
public:
	HttpPolicyGlobal();
	~HttpPolicyGlobal();

	HttpPolicyGlobal & operator=(const HttpPolicyGlobal &);
	
private:
	HttpPolicyGlobal(const HttpPolicyGlobal &);			// Not defined

public:
	HttpStatus set(HttpRequest::EGlobalPolicy opt, long value);
	HttpStatus set(HttpRequest::EGlobalPolicy opt, const std::string & value);
	HttpStatus get(HttpRequest::EGlobalPolicy opt, long * value);
	HttpStatus get(HttpRequest::EGlobalPolicy opt, const std::string ** value);
	
public:
	unsigned long		mSetMask;
	long				mConnectionLimit;
	std::string			mCAPath;
	std::string			mCAFile;
	std::string			mHttpProxy;
	long				mTrace;
	long				mUseLLProxy;
};  // end class HttpPolicyGlobal

}  // end namespace LLCore

#endif // _LLCORE_HTTP_POLICY_GLOBAL_H_
