/**
 * @file _httppolicyglobal.cpp
 * @brief Definitions for internal class defining global policy option.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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
	: mConnectionLimit(HTTP_CONNECTION_LIMIT_DEFAULT),
	  mTrace(HTTP_TRACE_OFF),
	  mUseLLProxy(0)
{}


HttpPolicyGlobal::~HttpPolicyGlobal()
{}


HttpPolicyGlobal & HttpPolicyGlobal::operator=(const HttpPolicyGlobal & other)
{
	if (this != &other)
	{
		mConnectionLimit = other.mConnectionLimit;
		mCAPath = other.mCAPath;
		mCAFile = other.mCAFile;
		mHttpProxy = other.mHttpProxy;
		mTrace = other.mTrace;
		mUseLLProxy = other.mUseLLProxy;
	}
	return *this;
}


HttpStatus HttpPolicyGlobal::set(HttpRequest::EPolicyOption opt, long value)
{
	switch (opt)
	{
	case HttpRequest::PO_CONNECTION_LIMIT:
		mConnectionLimit = llclamp(value, long(HTTP_CONNECTION_LIMIT_MIN), long(HTTP_CONNECTION_LIMIT_MAX));
		break;

	case HttpRequest::PO_TRACE:
		mTrace = llclamp(value, long(HTTP_TRACE_MIN), long(HTTP_TRACE_MAX));
		break;

	case HttpRequest::PO_LLPROXY:
		mUseLLProxy = llclamp(value, 0L, 1L);
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	return HttpStatus();
}


HttpStatus HttpPolicyGlobal::set(HttpRequest::EPolicyOption opt, const std::string & value)
{
	switch (opt)
	{
	case HttpRequest::PO_CA_PATH:
        LL_DEBUGS("CoreHttp") << "Setting global CA Path to " << value << LL_ENDL;
		mCAPath = value;
		break;

	case HttpRequest::PO_CA_FILE:
        LL_DEBUGS("CoreHttp") << "Setting global CA File to " << value << LL_ENDL;
		mCAFile = value;
		break;

	case HttpRequest::PO_HTTP_PROXY:
        LL_DEBUGS("CoreHttp") << "Setting global Proxy to " << value << LL_ENDL;
		mHttpProxy = value;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}
	
	return HttpStatus();
}

HttpStatus HttpPolicyGlobal::set(HttpRequest::EPolicyOption opt, HttpRequest::policyCallback_t value)
{
	switch (opt)
	{
	case HttpRequest::PO_SSL_VERIFY_CALLBACK:
		mSslCtxCallback = value;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	return HttpStatus();
}

HttpStatus HttpPolicyGlobal::get(HttpRequest::EPolicyOption opt, long * value) const
{
	switch (opt)
	{
	case HttpRequest::PO_CONNECTION_LIMIT:
		*value = mConnectionLimit;
		break;

	case HttpRequest::PO_TRACE:
		*value = mTrace;
		break;

	case HttpRequest::PO_LLPROXY:
		*value = mUseLLProxy;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	return HttpStatus();
}


HttpStatus HttpPolicyGlobal::get(HttpRequest::EPolicyOption opt, std::string * value) const
{
	switch (opt)
	{
	case HttpRequest::PO_CA_PATH:
		*value = mCAPath;
		break;

	case HttpRequest::PO_CA_FILE:
		*value = mCAFile;
		break;

	case HttpRequest::PO_HTTP_PROXY:
		*value = mHttpProxy;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}
	
	return HttpStatus();
}


HttpStatus HttpPolicyGlobal::get(HttpRequest::EPolicyOption opt, HttpRequest::policyCallback_t * value) const
{
	switch (opt)
	{
	case HttpRequest::PO_SSL_VERIFY_CALLBACK:
		*value = mSslCtxCallback;
		break;

	default:
		return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
	}

	return HttpStatus();
}

}  // end namespace LLCore
