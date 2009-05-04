/** 
 * @file 
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
#include <boost/shared_ptr.hpp>

class LLHTTPClientInterface;

class LLRegionPresenceVerifier
{
public:
	class Response
	{
	public:
		virtual ~Response() {}

		virtual bool checkValidity(const LLSD& content) const = 0;
		virtual void onRegionVerified(const LLSD& region_details) = 0;

		virtual void decrementRetries() = 0;

		virtual LLHTTPClientInterface& getHttpClient() = 0;
		virtual std::string getRegionUri() const = 0;
		virtual bool shouldRetry() const = 0;

		virtual void onCompletedRegionRequest() {}
	};

	typedef boost::shared_ptr<Response> ResponsePtr;

	class RegionResponder : public LLHTTPClient::Responder
	{
	public:
		RegionResponder(ResponsePtr data);
		virtual void result(const LLSD& content);
		virtual void completed(
			U32 status,
			const std::string& reason,
			const LLSD& content);

	private:
		ResponsePtr mSharedData;
	};

	class VerifiedDestinationResponder : public LLHTTPClient::Responder
	{
	public:
		VerifiedDestinationResponder(ResponsePtr data, const LLSD& content);
		virtual void result(const LLSD& content);
		virtual void error(U32 status, const std::string& reason);
	private:
		void retry();
		ResponsePtr mSharedData;
		LLSD mContent;
	};
};


#endif //LL_LLREGIONPRESENCEVERIFIER_H
