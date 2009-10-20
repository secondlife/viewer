/** 
 * @file 
 * @brief 
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2001-2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

