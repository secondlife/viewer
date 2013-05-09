/** 
 * @file llregionpresenceverifier.cpp
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
