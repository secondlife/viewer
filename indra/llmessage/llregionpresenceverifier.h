/** 
 * @file llregionpresenceverifier.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
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

/* Macro Definitions */
#ifndef LL_LLREGIONPRESENCEVERIFIER_H
#define LL_LLREGIONPRESENCEVERIFIER_H

#include "llhttpclient.h"
#include <string>
#include "llsd.h"
#include <boost/intrusive_ptr.hpp>

class LLHTTPClientInterface;

class LLRegionPresenceVerifier
{
public:
	class Response
	{
	public:
		virtual ~Response() = 0;

		virtual bool checkValidity(const LLSD& content) const = 0;
		virtual void onRegionVerified(const LLSD& region_details) = 0;
		virtual void onRegionVerificationFailed() = 0;

		virtual LLHTTPClientInterface& getHttpClient() = 0;

	public: /* but not really -- don't touch this */
		U32 mReferenceCount;		
	};

	typedef boost::intrusive_ptr<Response> ResponsePtr;

	class RegionResponder : public LLHTTPClient::Responder
	{
	public:
		RegionResponder(const std::string& uri, ResponsePtr data,
						S32 retry_count);
		virtual ~RegionResponder(); 
		virtual void result(const LLSD& content);
		virtual void error(U32 status, const std::string& reason);

	private:
		ResponsePtr mSharedData;
		std::string mUri;
		S32 mRetryCount;
	};

	class VerifiedDestinationResponder : public LLHTTPClient::Responder
	{
	public:
		VerifiedDestinationResponder(const std::string& uri, ResponsePtr data,
									 const LLSD& content, S32 retry_count);
		virtual ~VerifiedDestinationResponder();
		virtual void result(const LLSD& content);
		virtual void error(U32 status, const std::string& reason);
		
	private:
		void retry();
		ResponsePtr mSharedData;
		LLSD mContent;
		std::string mUri;
		S32 mRetryCount;
	};
};

namespace boost
{
	void intrusive_ptr_add_ref(LLRegionPresenceVerifier::Response* p);
	void intrusive_ptr_release(LLRegionPresenceVerifier::Response* p);
};

#endif //LL_LLREGIONPRESENCEVERIFIER_H
