/**
 * @file _httppolicyclass.cpp
 * @brief Definitions for internal class defining class policy option.
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

#include "_httppolicyclass.h"

#include "_httpinternal.h"


namespace LLCore
{


HttpPolicyClass::HttpPolicyClass()
	: mSetMask(0UL),
	  mConnectionLimit(HTTP_CONNECTION_LIMIT_DEFAULT),
	  mPerHostConnectionLimit(HTTP_CONNECTION_LIMIT_DEFAULT),
	  mPipelining(0)
{}


HttpPolicyClass::~HttpPolicyClass()
{}


HttpPolicyClass & HttpPolicyClass::operator=(const HttpPolicyClass & other)
{
	if (this != &other)
	{
		mSetMask = other.mSetMask;
		mConnectionLimit = other.mConnectionLimit;
		mPerHostConnectionLimit = other.mPerHostConnectionLimit;
		mPipelining = other.mPipelining;
	}
	return *this;
}


HttpPolicyClass::HttpPolicyClass(const HttpPolicyClass & other)
	: mSetMask(other.mSetMask),
	  mConnectionLimit(other.mConnectionLimit),
	  mPerHostConnectionLimit(other.mPerHostConnectionLimit),
	  mPipelining(other.mPipelining)
{}


HttpStatus HttpPolicyClass::set(HttpRequest::EClassPolicy opt, long value)
{
	switch (opt)
	{
	case HttpRequest::CP_CONNECTION_LIMIT:
		mConnectionLimit = llclamp(value, long(HTTP_CONNECTION_LIMIT_MIN), long(HTTP_CONNECTION_LIMIT_MAX));
		break;

	case HttpRequest::CP_PER_HOST_CONNECTION_LIMIT:
		mPerHostConnectionLimit = llclamp(value, long(HTTP_CONNECTION_LIMIT_MIN), mConnectionLimit);
		break;

	case HttpRequest::CP_ENABLE_PIPELINING:
		mPipelining = llclamp(value, 0L, 1L);
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	mSetMask |= 1UL << int(opt);
	return HttpStatus();
}


HttpStatus HttpPolicyClass::get(HttpRequest::EClassPolicy opt, long * value)
{
	static const HttpStatus not_set(HttpStatus::LLCORE, HE_OPT_NOT_SET);
	long * src(NULL);
	
	switch (opt)
	{
	case HttpRequest::CP_CONNECTION_LIMIT:
		src = &mConnectionLimit;
		break;

	case HttpRequest::CP_PER_HOST_CONNECTION_LIMIT:
		src = &mPerHostConnectionLimit;
		break;

	case HttpRequest::CP_ENABLE_PIPELINING:
		src = &mPipelining;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	if (! (mSetMask & (1UL << int(opt))))
		return not_set;

	*value = *src;
	return HttpStatus();
}


}  // end namespace LLCore
