/**
 * @file llcurl_stub.cpp
 * @brief stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "linden_common.h"
#include "llcurl.h"

LLCurl::Responder::Responder()
{
}

void LLCurl::Responder::completed(U32 status, std::basic_string<char, std::char_traits<char>, std::allocator<char> > const &reason,
								  LLSD const& mContent)
{
	if (isGoodStatus(status))
	{
		result(mContent);
	}
	else
	{
		errorWithContent(status, reason, mContent);
	}
}

void LLCurl::Responder::completedHeader(unsigned,
										std::basic_string<char, std::char_traits<char>, std::allocator<char> > const&,
										LLSD const&)
{
}

void LLCurl::Responder::completedRaw(unsigned,
									 std::basic_string<char, std::char_traits<char>, std::allocator<char> > const&,
									 LLChannelDescriptors const&,
									 boost::shared_ptr<LLBufferArray> const&)
{
}

void LLCurl::Responder::errorWithContent(unsigned,
							  std::basic_string<char, std::char_traits<char>, std::allocator<char> > const&,
							  LLSD const&)
{
}

LLCurl::Responder::~Responder ()
{
}

void LLCurl::Responder::error(unsigned,
							  std::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
{
}

void LLCurl::Responder::result(LLSD const&)
{
}

