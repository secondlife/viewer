/** 
 * @file 
 * @brief 
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
