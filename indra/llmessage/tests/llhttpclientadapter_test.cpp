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

#include "llhttpclientadapter.h"

#include "../test/lltut.h"
#include "llhttpclient.h"
#include "llcurl_stub.cpp"

float const HTTP_REQUEST_EXPIRY_SECS = 1.0F;

std::vector<std::string> get_urls;
std::vector<boost::intrusive_ptr<LLCurl::Responder> > get_responders;
void LLHTTPClient::get(const std::string& url, boost::intrusive_ptr<LLCurl::Responder> responder, const LLSD& headers, const F32 timeout)
{
	get_urls.push_back(url);
	get_responders.push_back(responder);
}

std::vector<std::string> put_urls;
std::vector<LLSD> put_body;
std::vector<boost::intrusive_ptr<LLCurl::Responder> > put_responders;

void LLHTTPClient::put(std::string const &url, LLSD const &body, boost::intrusive_ptr<LLCurl::Responder> responder,float) 
{
	put_urls.push_back(url);
	put_responders.push_back(responder);
	put_body.push_back(body);

}


namespace tut
{
	struct LLHTTPClientAdapterData
	{
		LLHTTPClientAdapterData()
		{
			get_urls.clear();
			get_responders.clear();
			put_urls.clear();
			put_responders.clear();
			put_body.clear();
		}
	};

	typedef test_group<LLHTTPClientAdapterData> factory;
	typedef factory::object object;
}

namespace
{
	tut::factory tf("LLHTTPClientAdapterData test");
}

namespace tut
{
	// Ensure we can create the object
	template<> template<>
	void object::test<1>()
	{
		LLHTTPClientAdapter adapter;
	}

	// Does the get pass the appropriate arguments to the LLHTTPClient
	template<> template<>
	void object::test<2>()
	{
		LLHTTPClientAdapter adapter;

		boost::intrusive_ptr<LLCurl::Responder> responder = new LLCurl::Responder();

		adapter.get("Made up URL", responder);
		ensure_equals(get_urls.size(), 1);
		ensure_equals(get_urls[0], "Made up URL");
	}

	// Ensure the responder matches the one passed to get
	template<> template<>
	void object::test<3>()
	{
		LLHTTPClientAdapter adapter;
		boost::intrusive_ptr<LLCurl::Responder> responder = new LLCurl::Responder();

		adapter.get("Made up URL", responder);

		ensure_equals(get_responders.size(), 1);
		ensure_equals(get_responders[0].get(), responder.get());
	}
	
	// Ensure the correct url is used in the put
	template<> template<>
	void object::test<4>()
	{
		LLHTTPClientAdapter adapter;

		boost::intrusive_ptr<LLCurl::Responder> responder = new LLCurl::Responder();

		LLSD body;
		body["TestBody"] = "Foobar";

		adapter.put("Made up URL", body, responder);
		ensure_equals(put_urls.size(), 1);
		ensure_equals(put_urls[0], "Made up URL");
	}

	// Ensure the correct responder is used by put
	template<> template<>
	void object::test<5>()
	{
		LLHTTPClientAdapter adapter;

		boost::intrusive_ptr<LLCurl::Responder> responder = new LLCurl::Responder();

		LLSD body;
		body["TestBody"] = "Foobar";

		adapter.put("Made up URL", body, responder);

		ensure_equals(put_responders.size(), 1);
		ensure_equals(put_responders[0].get(), responder.get());
	}

	// Ensure the message body is passed through the put properly
	template<> template<>
	void object::test<6>()
	{
		LLHTTPClientAdapter adapter;

		boost::intrusive_ptr<LLCurl::Responder> responder = new LLCurl::Responder();

		LLSD body;
		body["TestBody"] = "Foobar";

		adapter.put("Made up URL", body, responder);

		ensure_equals(put_body.size(), 1);
		ensure_equals(put_body[0]["TestBody"].asString(), "Foobar");
	}
}

