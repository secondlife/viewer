/**
 * @file _httppolicyglobal.cpp
 * @brief Definitions for internal class defining global policy option.
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

#include "_httppolicyglobal.h"

#include "_httpinternal.h"


namespace LLCore
{


HttpPolicyGlobal::HttpPolicyGlobal()
	: mSetMask(0UL),
	  mConnectionLimit(HTTP_CONNECTION_LIMIT_DEFAULT),
	  mTrace(HTTP_TRACE_OFF),
	  mUseLLProxy(0)
{}


HttpPolicyGlobal::~HttpPolicyGlobal()
{}


HttpPolicyGlobal & HttpPolicyGlobal::operator=(const HttpPolicyGlobal & other)
{
	if (this != &other)
	{
		mSetMask = other.mSetMask;
		mConnectionLimit = other.mConnectionLimit;
		mCAPath = other.mCAPath;
		mCAFile = other.mCAFile;
		mHttpProxy = other.mHttpProxy;
		mTrace = other.mTrace;
		mUseLLProxy = other.mUseLLProxy;
	}
	return *this;
}


HttpStatus HttpPolicyGlobal::set(HttpRequest::EGlobalPolicy opt, long value)
{
	switch (opt)
	{
	case HttpRequest::GP_CONNECTION_LIMIT:
		mConnectionLimit = llclamp(value, long(HTTP_CONNECTION_LIMIT_MIN), long(HTTP_CONNECTION_LIMIT_MAX));
		break;

	case HttpRequest::GP_TRACE:
		mTrace = llclamp(value, long(HTTP_TRACE_MIN), long(HTTP_TRACE_MAX));
		break;

	case HttpRequest::GP_LLPROXY:
		mUseLLProxy = llclamp(value, 0L, 1L);
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	mSetMask |= 1UL << int(opt);
	return HttpStatus();
}


HttpStatus HttpPolicyGlobal::set(HttpRequest::EGlobalPolicy opt, const std::string & value)
{
	switch (opt)
	{
	case HttpRequest::GP_CA_PATH:
		mCAPath = value;
		break;

	case HttpRequest::GP_CA_FILE:
		mCAFile = value;
		break;

	case HttpRequest::GP_HTTP_PROXY:
		mCAFile = value;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}
	
	mSetMask |= 1UL << int(opt);
	return HttpStatus();
}


HttpStatus HttpPolicyGlobal::get(HttpRequest::EGlobalPolicy opt, long * value)
{
	static const HttpStatus not_set(HttpStatus::LLCORE, HE_OPT_NOT_SET);
	long * src(NULL);
	
	switch (opt)
	{
	case HttpRequest::GP_CONNECTION_LIMIT:
		src = &mConnectionLimit;
		break;

	case HttpRequest::GP_TRACE:
		src = &mTrace;
		break;

	case HttpRequest::GP_LLPROXY:
		src = &mUseLLProxy;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	if (! (mSetMask & (1UL << int(opt))))
		return not_set;

	*value = *src;
	return HttpStatus();
}


HttpStatus HttpPolicyGlobal::get(HttpRequest::EGlobalPolicy opt, const std::string ** value)
{
	static const HttpStatus not_set(HttpStatus::LLCORE, HE_OPT_NOT_SET);
	const std::string * src(NULL);

	switch (opt)
	{
	case HttpRequest::GP_CA_PATH:
		src = &mCAPath;
		break;

	case HttpRequest::GP_CA_FILE:
		src = &mCAFile;
		break;

	case HttpRequest::GP_HTTP_PROXY:
		src = &mHttpProxy;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}
	
	if (! (mSetMask & (1UL << int(opt))))
		return not_set;

	*value = src;
	return HttpStatus();
}

}  // end namespace LLCore
