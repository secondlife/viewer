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


namespace LLCore
{


HttpPolicyGlobal::HttpPolicyGlobal()
	: mValidMask(0UL),
	  mConnectionLimit(32L)
{}


HttpPolicyGlobal::~HttpPolicyGlobal()
{}


HttpStatus HttpPolicyGlobal::set(HttpRequest::EGlobalPolicy opt, long value)
{
	switch (opt)
	{
	case HttpRequest::GP_CONNECTION_LIMIT:
		mConnectionLimit = value;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	mValidMask |= 1UL << int(opt);
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
	
	mValidMask |= 1UL << int(opt);
	return HttpStatus();
}


HttpStatus HttpPolicyGlobal::get(HttpRequest::EGlobalPolicy opt, long & value)
{
	static const HttpStatus not_set(HttpStatus::LLCORE, HE_OPT_NOT_SET);
	
	switch (opt)
	{
	case HttpRequest::GP_CONNECTION_LIMIT:
		if (! (mValidMask & (1UL << int(opt))))
			return not_set;
		value = mConnectionLimit;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	return HttpStatus();
}


HttpStatus HttpPolicyGlobal::get(HttpRequest::EGlobalPolicy opt, std::string & value)
{
	static const HttpStatus not_set(HttpStatus::LLCORE, HE_OPT_NOT_SET);
	
	switch (opt)
	{
	case HttpRequest::GP_CA_PATH:
		if (! (mValidMask & (1UL << int(opt))))
			return not_set;
		value = mCAPath;
		break;

	case HttpRequest::GP_CA_FILE:
		if (! (mValidMask & (1UL << int(opt))))
			return not_set;
		value = mCAFile;
		break;

	case HttpRequest::GP_HTTP_PROXY:
		if (! (mValidMask & (1UL << int(opt))))
			return not_set;
		value = mHttpProxy;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}
	
	return HttpStatus();
}

}  // end namespace LLCore
