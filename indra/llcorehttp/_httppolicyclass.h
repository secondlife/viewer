/**
 * @file _httppolicyclass.h
 * @brief Declarations for internal class defining policy class options.
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

#ifndef	_LLCORE_HTTP_POLICY_CLASS_H_
#define	_LLCORE_HTTP_POLICY_CLASS_H_


#include "httprequest.h"


namespace LLCore
{

class HttpPolicyClass
{
public:
	HttpPolicyClass();
	~HttpPolicyClass();

	HttpPolicyClass & operator=(const HttpPolicyClass &);
	HttpPolicyClass(const HttpPolicyClass &);			// Not defined

public:
	HttpStatus set(HttpRequest::EClassPolicy opt, long value);
	HttpStatus get(HttpRequest::EClassPolicy opt, long * value);
	
public:
	unsigned long				mSetMask;
	long						mConnectionLimit;
	long						mPerHostConnectionLimit;
	long						mPipelining;
};  // end class HttpPolicyClass

}  // end namespace LLCore

#endif // _LLCORE_HTTP_POLICY_CLASS_H_
