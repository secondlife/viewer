/** 
 * @file llhttpclientadapter_test.cpp
 * @brief Tests for LLHTTPClientAdapter
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

#include "llhttpclientadapter.h"

#include "../test/lltut.h"
#include "llhttpclient.h"
#include "llcurl_stub.cpp"

float const HTTP_REQUEST_EXPIRY_SECS = 1.0F;

std::vector<std::string> get_urls;
std::vector< LLCurl::ResponderPtr > get_responders;
void LLHTTPClient::get(const std::string& url, LLCurl::ResponderPtr responder, const LLSD& headers, const F32 timeout, bool follow_redirects)
{
	get_urls.push_back(url);
	get_responders.push_back(responder);
}

std::vector<std::string> put_urls;
std::vector<LLSD> put_body;
std::vector<LLSD> put_headers;
std::vector<LLCurl::ResponderPtr> put_responders;

void LLHTTPClient::put(const std::string& url, const LLSD& body, LLCurl::ResponderPtr responder, const LLSD& headers, const F32 timeout)
{
	put_urls.push_back(url);
	put_responders.push_back(responder);
	put_body.push_back(body);
	put_headers.push_back(headers);

}

std::vector<std::string> delete_urls;
std::vector<LLCurl::ResponderPtr> delete_responders;

void LLHTTPClient::del(
	const std::string& url,
	LLCurl::ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	delete_urls.push_back(url);
	delete_responders.push_back(responder);
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
			put_headers.clear();
			delete_urls.clear();
			delete_responders.clear();
		}
	};

	typedef test_group<LLHTTPClientAdapterData> factory;
	typedef factory::object object;
}

namespace
{
	tut::factory tf("LLHTTPClientAdapterData");
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

		LLCurl::ResponderPtr responder = new LLCurl::Responder();

		adapter.get("Made up URL", responder);
		ensure_equals(get_urls.size(), 1);
		ensure_equals(get_urls[0], "Made up URL");
	}

	// Ensure the responder matches the one passed to get
	template<> template<>
	void object::test<3>()
	{
		LLHTTPClientAdapter adapter;
		LLCurl::ResponderPtr responder = new LLCurl::Responder();

		adapter.get("Made up URL", responder);

		ensure_equals(get_responders.size(), 1);
		ensure_equals(get_responders[0].get(), responder.get());
	}
	
	// Ensure the correct url is used in the put
	template<> template<>
	void object::test<4>()
	{
		LLHTTPClientAdapter adapter;

		LLCurl::ResponderPtr responder = new LLCurl::Responder();

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

		LLCurl::ResponderPtr responder = new LLCurl::Responder();

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

		LLCurl::ResponderPtr responder = new LLCurl::Responder();

		LLSD body;
		body["TestBody"] = "Foobar";

		adapter.put("Made up URL", body, responder);

		ensure_equals(put_body.size(), 1);
		ensure_equals(put_body[0]["TestBody"].asString(), "Foobar");
	}

	// Ensure that headers are passed through put properly
	template<> template<>
	void object::test<7>()
	{
		LLHTTPClientAdapter adapter;

		LLCurl::ResponderPtr responder = new LLCurl::Responder();

		LLSD body = LLSD::emptyMap();
		body["TestBody"] = "Foobar";

		LLSD headers = LLSD::emptyMap();
		headers["booger"] = "omg";

		adapter.put("Made up URL", body, responder, headers);

		ensure_equals("Header count", put_headers.size(), 1);
		ensure_equals(
			"First header",
			put_headers[0]["booger"].asString(),
			"omg");
	}

	// Ensure that del() passes appropriate arguments to the LLHTTPClient
	template<> template<>
	void object::test<8>()
	{
		LLHTTPClientAdapter adapter;

		LLCurl::ResponderPtr responder = new LLCurl::Responder();

		adapter.del("Made up URL", responder);

		ensure_equals("URL count", delete_urls.size(), 1);
		ensure_equals("Received URL", delete_urls[0], "Made up URL");

		ensure_equals("Responder count", delete_responders.size(), 1);
		//ensure_equals("Responder", delete_responders[0], responder);
	}
}

