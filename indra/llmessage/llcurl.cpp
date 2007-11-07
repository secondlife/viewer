/**
 * @file llcurl.h
 * @author Zero / Donovan
 * @date 2006-10-15
 * @brief Curl wrapper
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#include "llcurl.h"

#include <iomanip>

#include "llsdserialize.h"

//////////////////////////////////////////////////////////////////////////////
/*
	The trick to getting curl to do keep-alives is to reuse the
	same easy handle for the requests.  It appears that curl
	keeps a pool of connections alive for each easy handle, but
	doesn't share them between easy handles.  Therefore it is
	important to keep a pool of easy handles and reuse them,
	rather than create and destroy them with each request.  This
	code does this.

	Furthermore, it would behoove us to keep track of which
	hosts an easy handle was used for and pick an easy handle
	that matches the next request.  This code does not current
	do this.
 */

using namespace std;
	
LLCurl::Responder::Responder()
	: mReferenceCount(0)
{
}
LLCurl::Responder::~Responder()
{
}

// virtual
void LLCurl::Responder::error(U32 status, const std::stringstream& content)
{
	llinfos << "LLCurl::Responder::error " << status << ": " << content.str() << llendl;
}

// virtual
void LLCurl::Responder::result(const std::stringstream& content)
{
}

// virtual
void LLCurl::Responder::completed(U32 status, const std::stringstream& content)
{
	if (200 <= status &&  status < 300)
	{
		result(content);
	}
	else
	{
		error(status, content);
	}
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

//////////////////////////////////////////////////////////////////////////////

size_t
curlOutputCallback(void* data, size_t size, size_t nmemb, void* user_data)
{
	stringstream& output = *(stringstream*)user_data;
	
	size_t n = size * nmemb;
	output.write((const char*)data, n);
	if (!((istream&)output).good()) {
		std::cerr << "WHAT!?!?!? istream side bad" << std::endl;
	}
	if (!((ostream&)output).good()) {
		std::cerr << "WHAT!?!?!? ostream side bad" << std::endl;
	}

	return n;
}

// Only used if request contained a body (post or put), Not currently implemented.
// size_t
// curlRequestCallback(void* data, size_t size, size_t nmemb, void* user_data)
// {
// 	stringstream& request = *(stringstream*)user_data;
	
// 	size_t n = size * nmemb;
// 	request.read((char*)data, n);
// 	return request.gcount();
// }





LLCurl::Easy::Easy()
{
	mHeaders = 0;
	mHeaders = curl_slist_append(mHeaders, "Connection: keep-alive");
	mHeaders = curl_slist_append(mHeaders, "Keep-alive: 300");
	mHeaders = curl_slist_append(mHeaders, "Content-Type: application/xml");
		// FIXME: shouldn't be there for GET/DELETE
		// FIXME: should have ACCEPT headers
		
	mHandle = curl_easy_init();
}

LLCurl::Easy::~Easy()
{
	curl_easy_cleanup(mHandle);
	curl_slist_free_all(mHeaders);
}

void
LLCurl::Easy::get(const string& url, ResponderPtr responder)
{
	prep(url, responder);
	curl_easy_setopt(mHandle, CURLOPT_HTTPGET, 1);
}

void
LLCurl::Easy::getByteRange(const string& url, S32 offset, S32 length, ResponderPtr responder)
{
	mRange = llformat("Range: bytes=%d-%d", offset,offset+length-1);
	mHeaders = curl_slist_append(mHeaders, mRange.c_str());
	prep(url, responder);
	curl_easy_setopt(mHandle, CURLOPT_HTTPGET, 1);
}

void
LLCurl::Easy::perform()
{
	report(curl_easy_perform(mHandle));
}

void
LLCurl::Easy::prep(const std::string& url, ResponderPtr responder)
{
#if !LL_DARWIN
 	curl_easy_reset(mHandle); // SJB: doesn't exisit on OSX 10.3.9
#else
	// SJB: equivalent? fast?
 	curl_easy_cleanup(mHandle);
 	mHandle = curl_easy_init();
#endif
	
	curl_easy_setopt(mHandle, CURLOPT_PRIVATE, this);

//	curl_easy_setopt(mHandle, CURLOPT_VERBOSE, 1); // usefull for debugging
	curl_easy_setopt(mHandle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, &curlOutputCallback);
	curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, &mOutput);
#if 1 // For debug
	curl_easy_setopt(mHandle, CURLOPT_HEADERFUNCTION, &curlOutputCallback);
	curl_easy_setopt(mHandle, CURLOPT_HEADERDATA, &mHeaderOutput);
#endif
	curl_easy_setopt(mHandle, CURLOPT_ERRORBUFFER, &mErrorBuffer);
	curl_easy_setopt(mHandle, CURLOPT_ENCODING, "");
	curl_easy_setopt(mHandle, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(mHandle, CURLOPT_HTTPHEADER, mHeaders);

	mOutput.str("");
	if (!((istream&)mOutput).good()) {
		std::cerr << "WHAT!?!?!? istream side bad" << std::endl;
	}
	if (!((ostream&)mOutput).good()) {
		std::cerr << "WHAT!?!?!? ostream side bad" << std::endl;
	}

	mURL = url;
	curl_easy_setopt(mHandle, CURLOPT_URL, mURL.c_str());

	mResponder = responder;
}

void
LLCurl::Easy::report(CURLcode code)
{
	if (!mResponder) return;
	
	long responseCode;
	
	if (code == CURLE_OK)
	{
		curl_easy_getinfo(mHandle, CURLINFO_RESPONSE_CODE, &responseCode);
	}
	else
	{
		responseCode = 499;
	}
	
	mResponder->completed(responseCode, mOutput);
	mResponder = NULL;
}






LLCurl::Multi::Multi()
{
	mHandle = curl_multi_init();
}

LLCurl::Multi::~Multi()
{
	// FIXME: should clean up excess handles in mFreeEasy
	curl_multi_cleanup(mHandle);
}


void
LLCurl::Multi::get(const std::string& url, ResponderPtr responder)
{
	LLCurl::Easy* easy = easyAlloc();
	easy->get(url, responder);
	curl_multi_add_handle(mHandle, easy->mHandle);
}
	
void
LLCurl::Multi::getByteRange(const std::string& url, S32 offset, S32 length, ResponderPtr responder)
{
	LLCurl::Easy* easy = easyAlloc();
	easy->getByteRange(url, offset, length, responder);
	curl_multi_add_handle(mHandle, easy->mHandle);
}
	
void
LLCurl::Multi::process()
{
	int count;
	for (int call_count = 0; call_count < 5; call_count += 1)
	{
		if (CURLM_CALL_MULTI_PERFORM != curl_multi_perform(mHandle, &count))
		{
			break;
		}
	}
		
	CURLMsg* msg;
	int msgs_in_queue;
	while ((msg = curl_multi_info_read(mHandle, &msgs_in_queue)))
	{
		if (msg->msg != CURLMSG_DONE) continue;
		Easy* easy = 0;
		curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &easy);
		if (!easy) continue;
		easy->report(msg->data.result);
		
		curl_multi_remove_handle(mHandle, easy->mHandle);
		easyFree(easy);
	}
}



LLCurl::Easy*
LLCurl::Multi::easyAlloc()
{
	Easy* easy = 0;
	
	if (mFreeEasy.empty())
	{
		easy = new Easy();
	}
	else
	{
		easy = mFreeEasy.back();
		mFreeEasy.pop_back();
	}

	return easy;
}

void
LLCurl::Multi::easyFree(Easy* easy)
{
	if (mFreeEasy.size() < 5)
	{
		mFreeEasy.push_back(easy);
	}
	else
	{
		delete easy;
	}
}



namespace
{
	static LLCurl::Multi* sMainMulti = 0;
	
	LLCurl::Multi*
	mainMulti()
	{
		if (!sMainMulti) {
			sMainMulti = new LLCurl::Multi();
		}
		return sMainMulti;
	}

	void freeMulti()
	{
		delete sMainMulti;
		sMainMulti = NULL;
	}
}

void
LLCurl::get(const std::string& url, ResponderPtr responder)
{
	mainMulti()->get(url, responder);
}
	
void
LLCurl::getByteRange(const std::string& url, S32 offset, S32 length, ResponderPtr responder)
{
	mainMulti()->getByteRange(url, offset, length, responder);
}
	
void LLCurl::initClass()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

void
LLCurl::process()
{
	mainMulti()->process();
}

void LLCurl::cleanup()
{
	freeMulti();
	curl_global_cleanup();
}
