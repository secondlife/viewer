/** 
 * @file 
 * @brief 
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

#include "../test/lltut.h"
#include "llregionpresenceverifier.h"
#include "llcurl_stub.cpp"
#include "llhost.cpp"
#include "net.cpp"
#include "lltesthttpclientadapter.cpp"

class LLTestResponse : public LLRegionPresenceVerifier::Response
{
public:

	virtual bool checkValidity(const LLSD& content) const
	{
		return true;
	}

	virtual void onRegionVerified(const LLSD& region_details)
	{
	}

	virtual void onRegionVerificationFailed()
	{
	}
	
	virtual LLHTTPClientInterface& getHttpClient()
	{
		return mHttpInterface;
	}

	LLTestHTTPClientAdapter mHttpInterface;
};

namespace tut
{
	struct LLRegionPresenceVerifierData
	{
		LLRegionPresenceVerifierData() :
			mResponse(new LLTestResponse()),
			mResponder("", LLRegionPresenceVerifier::ResponsePtr(mResponse),
					   LLSD(), 3)
		{
		}
		
		LLTestResponse* mResponse;
		LLRegionPresenceVerifier::VerifiedDestinationResponder mResponder;
	};

	typedef test_group<LLRegionPresenceVerifierData> factory;
	typedef factory::object object;
}

namespace
{
	tut::factory tf("LLRegionPresenceVerifier test");
}

namespace tut
{
	// Test that VerifiedDestinationResponder does retry
    // on error when shouldRetry returns true.
	template<> template<>
	void object::test<1>()
	{
		mResponder.error(500, "Internal server error");
		ensure_equals(mResponse->mHttpInterface.mGetUrl.size(), 1);
	}

	// Test that VerifiedDestinationResponder only retries
	// on error until shouldRetry returns false.
	template<> template<>
	void object::test<2>()
	{
		mResponder.error(500, "Internal server error");
		mResponder.error(500, "Internal server error");
		mResponder.error(500, "Internal server error");
		mResponder.error(500, "Internal server error");
		ensure_equals(mResponse->mHttpInterface.mGetUrl.size(), 3);
	}
}

