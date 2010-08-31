/**
 * @file llcurl_stub.cpp
 * @brief stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
 * this source code is governed by the Linden Lab Source Code Disclosure
 * Agreement ("Agreement") previously entered between you and Linden
 * Lab. By accessing, using, copying, modifying or distributing this
 * software, you acknowledge that you have been informed of your
 * obligations under the Agreement and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llcurl.h"

LLCurl::Responder::Responder()
	: mReferenceCount(0)
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

namespace boost
{
	void intrusive_ptr_add_ref(LLCurl::Responder* p)
	{
		++p->mReferenceCount;
	}

	void intrusive_ptr_release(LLCurl::Responder* p)
	{
		if(p && 0 == --p->mReferenceCount)
		{
			delete p;
		}
	}
};

