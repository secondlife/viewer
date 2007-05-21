/**
 * @file llcurl.h
 * @author Zero / Donovan
 * @date 2006-10-15
 * @brief Curl wrapper
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	
	static void process();
};

namespace boost
{
	void intrusive_ptr_add_ref(LLCurl::Responder* p);
	void intrusive_ptr_release(LLCurl::Responder* p);
};

#endif // LL_LLCURL_H
