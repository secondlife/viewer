/**
 * @file httpcommon.cpp
 * @brief 
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

#include "httpcommon.h"

#include <curl/curl.h>
#include <string>


namespace LLCore
{

HttpStatus::type_enum_t EXT_CURL_EASY;
HttpStatus::type_enum_t EXT_CURL_MULTI;
HttpStatus::type_enum_t LLCORE;

std::string HttpStatus::toString() const
{
	static const char * llcore_errors[] =
		{
			"",
			"Services shutting down",
			"Operation canceled",
			"Invalid Content-Range header encountered"
		};
	static const int llcore_errors_count(sizeof(llcore_errors) / sizeof(llcore_errors[0]));
	
	if (*this)
	{
		return std::string("");
	}

	switch (mType)
	{
	case EXT_CURL_EASY:
		return std::string(curl_easy_strerror(CURLcode(mStatus)));

	case EXT_CURL_MULTI:
		return std::string(curl_multi_strerror(CURLMcode(mStatus)));

	case LLCORE:
		if (mStatus >= 0 && mStatus < llcore_errors_count)
		{
			return std::string(llcore_errors[mStatus]);
		}
		break;
		
	}
	return std::string("Unknown error");
}
		
} // end namespace LLCore

