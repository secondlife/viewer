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
 
#ifndef LL_LLCURL_H
#define LL_LLCURL_H

#include "linden_common.h"

#include <sstream>
#include <string>
#include <vector>

#include <boost/intrusive_ptr.hpp>
#include <curl/curl.h>

// #include "llhttpclient.h"

class LLCurl
{
public:
	class Multi;

	class Responder
	{
	public:
		Responder();
		virtual ~Responder();

		virtual void error(U32 status, const std::stringstream& content);	// called with bad status codes
		
		virtual void result(const std::stringstream& content);
		
		virtual void completed(U32 status, const std::stringstream& content);
			/**< The default implemetnation calls
				either:
				* result(), or
				* error() 
			*/
			
	public: /* but not really -- don't touch this */
		U32 mReferenceCount;
	};
	typedef boost::intrusive_ptr<Responder>	ResponderPtr;
	
	class Easy
	{
	public:
		Easy();
		~Easy();
		
		void get(const std::string& url, ResponderPtr);
		void getByteRange(const std::string& url, S32 offset, S32 length, ResponderPtr);

		void perform();

	private:
		void prep(const std::string& url, ResponderPtr);
		void report(CURLcode);
		
		CURL*				mHandle;
		struct curl_slist*	mHeaders;
		
		std::string			mURL;
		std::string			mRange;
		std::stringstream	mRequest;

		std::stringstream	mOutput;
		char				mErrorBuffer[CURL_ERROR_SIZE];

		std::stringstream	mHeaderOutput; // Debug
		
		ResponderPtr		mResponder;

		friend class Multi;
	};


	class Multi
	{
	public:
		Multi();
		~Multi();

		void get(const std::string& url, ResponderPtr);
		void getByteRange(const std::string& url, S32 offset, S32 length, ResponderPtr);

		void process();
		
	private:
		Easy* easyAlloc();
		void easyFree(Easy*);
		
		CURLM* mHandle;
		
		typedef std::vector<Easy*>	EasyList;
		EasyList mFreeEasy;
	};


	static void get(const std::string& url, ResponderPtr);
	static void getByteRange(const std::string& url, S32 offset, S32 length, ResponderPtr responder);
	
    static void initClass(); // *NOTE:Mani - not thread safe!
	static void process();
	static void cleanup(); // *NOTE:Mani - not thread safe!
};

namespace boost
{
	void intrusive_ptr_add_ref(LLCurl::Responder* p);
	void intrusive_ptr_release(LLCurl::Responder* p);
};

#endif // LL_LLCURL_H
