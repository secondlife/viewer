/** 
 * @file lliohttpserver_tut.cpp
 * @date   May 2006
 * @brief HTTP server unit tests
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include <tut/tut.h>
#include "lltut.h"

#include "llbufferstream.h"
#include "lliohttpserver.h"
#include "llsdhttpserver.h"
#include "llsdserialize.h"

#include "llpipeutil.h"


namespace tut
{
	class HTTPServiceTestData
	{
	public:
		class DelayedEcho : public LLHTTPNode
		{
			HTTPServiceTestData* mTester;

		public:
			DelayedEcho(HTTPServiceTestData* tester) : mTester(tester) { }

			void post(ResponsePtr response, const LLSD& context, const LLSD& input) const
			{
				ensure("response already set", mTester->mResponse == ResponsePtr(NULL));
				mTester->mResponse = response;
				mTester->mResult = input;
			}
		};

		class WireHello : public LLIOPipe
		{
		protected:
			virtual EStatus process_impl(
				const LLChannelDescriptors& channels,
				buffer_ptr_t& buffer,
				bool& eos,
				LLSD& context,
				LLPumpIO* pump)
			{
				if(!eos) return STATUS_BREAK;
				LLSD sd = "yo!";
				LLBufferStream ostr(channels, buffer.get());
				ostr << LLSDXMLStreamer(sd);
				return STATUS_DONE;
			}
		};

		HTTPServiceTestData()
			: mResponse(NULL)
		{
			LLHTTPStandardServices::useServices();
			LLHTTPRegistrar::buildAllServices(mRoot);
			mRoot.addNode("/delayed/echo", new DelayedEcho(this));
			mRoot.addNode("/wire/hello", new LLHTTPNodeForPipe<WireHello>);
		}
		
		LLHTTPNode mRoot;
		LLHTTPNode::ResponsePtr mResponse;
		LLSD mResult;

		void pumpPipe(LLPumpIO* pump, S32 iterations)
		{
			while(iterations > 0)
			{
				pump->pump();
				pump->callback();
				--iterations;
			}
		}

		std::string makeRequest(
			const std::string& name,
			const std::string& httpRequest,
			bool timeout = false)
		{
			LLPipeStringInjector* injector = new LLPipeStringInjector(httpRequest);
			LLPipeStringExtractor* extractor = new LLPipeStringExtractor();
			
			apr_pool_t* pool;
			apr_pool_create(&pool, NULL);

			LLPumpIO* pump;
			pump = new LLPumpIO(pool);

			LLPumpIO::chain_t chain;
			LLSD context;

			chain.push_back(LLIOPipe::ptr_t(injector));
			LLCreateHTTPPipe(chain, mRoot, LLSD());
			chain.push_back(LLIOPipe::ptr_t(extractor));

			pump->addChain(chain, DEFAULT_CHAIN_EXPIRY_SECS);

			pumpPipe(pump, 10);
			if(mResponse && (! timeout)) 
			{
				mResponse->result(mResult);
				mResponse = NULL;
			}
			pumpPipe(pump, 10);
			
			std::string httpResult = extractor->string();

			chain.clear();
			delete pump;
			apr_pool_destroy(pool);

			if(mResponse && timeout)
			{
				mResponse->result(mResult);
				mResponse = NULL;
			}
			
			return httpResult;
		}
		
		std::string httpGET(const std::string& uri, 
							bool timeout = false)
		{
			std::string httpRequest = "GET " + uri + " HTTP/1.0\r\n\r\n";
			return makeRequest(uri, httpRequest, timeout);
		}
		
		std::string httpPOST(const std::string& uri,
			const std::string& body,
			bool timeout,
			const std::string& evilExtra = "")
		{
			std::ostringstream httpRequest;
			httpRequest << "POST " + uri + " HTTP/1.0\r\n";
			httpRequest << "Content-Length: " << body.size() << "\r\n";
			httpRequest << "\r\n";
			httpRequest << body;
			httpRequest << evilExtra;
				
			return makeRequest(uri, httpRequest.str(), timeout);
		}

		std::string httpPOST(const std::string& uri,
			const std::string& body,
			const std::string& evilExtra = "")
		{
			bool timeout = false;
			return httpPOST(uri, body, timeout, evilExtra);
		}
	};

	typedef test_group<HTTPServiceTestData>		HTTPServiceTestGroup;
	typedef HTTPServiceTestGroup::object		HTTPServiceTestObject;
	HTTPServiceTestGroup httpServiceTestGroup("http service");

	template<> template<>
	void HTTPServiceTestObject::test<1>()
	{
		std::string result = httpGET("web/hello");
		
		ensure_starts_with("web/hello status", result,
			"HTTP/1.0 200 OK\r\n");
						
		ensure_contains("web/hello content type", result,
			"Content-Type: application/xml\r\n");

		ensure_contains("web/hello content length", result,
			"Content-Length: 36\r\n");

		ensure_contains("web/hello content", result,
			"\r\n"
			"<llsd><string>hello</string></llsd>"
			);
	}
	
	template<> template<>
	void HTTPServiceTestObject::test<2>()
	{
		// test various HTTP errors
		
		std::string actual;

		actual = httpGET("web/missing");
		ensure_starts_with("web/missing 404", actual,
			"HTTP/1.0 404 Not Found\r\n");

		actual = httpGET("web/echo");			
		ensure_starts_with("web/echo 405", actual,
			"HTTP/1.0 405 Method Not Allowed\r\n");
	}
	
	template<> template<>
	void HTTPServiceTestObject::test<3>()
	{
		// test POST & content-length handling
		
		std::string result;
		
		result = httpPOST("web/echo",
			"<llsd><integer>42</integer></llsd>");
			
		ensure_starts_with("web/echo status", result,
			"HTTP/1.0 200 OK\r\n");
						
		ensure_contains("web/echo content type", result,
			"Content-Type: application/xml\r\n");

		ensure_contains("web/echo content length", result,
			"Content-Length: 35\r\n");

		ensure_contains("web/hello content", result,
			"\r\n"
			"<llsd><integer>42</integer></llsd>"
			);

/* TO DO: this test doesn't pass!!
		
		result = httpPOST("web/echo",
			"<llsd><string>evil</string></llsd>",
			"really!  evil!!!");
			
		ensure_equals("web/echo evil result", result,
			"HTTP/1.0 200 OK\r\n"
			"Content-Length: 34\r\n"
			"\r\n"
			"<llsd><string>evil</string></llsd>"
			);
*/
	}

	template<> template<>
	void HTTPServiceTestObject::test<4>()
	{
		// test calling things based on pipes

		std::string result;

		result = httpGET("wire/hello");

		ensure_contains("wire/hello", result, "yo!");
	}

    template<> template<>
	void HTTPServiceTestObject::test<5>()
    {
		// test timeout before async response
		std::string result;

		bool timeout = true;
		result = httpPOST("delayed/echo",
			"<llsd><string>agent99</string></llsd>", timeout);

		ensure_equals("timeout delayed/echo status", result, std::string(""));
	}

	template<> template<>
	void HTTPServiceTestObject::test<6>()
	{
		// test delayed service		
		std::string result;
		
		result = httpPOST("delayed/echo",
			"<llsd><string>agent99</string></llsd>");
			
		ensure_starts_with("delayed/echo status", result,
			"HTTP/1.0 200 OK\r\n");
						
		ensure_contains("delayed/echo content", result,
			"\r\n"
			"<llsd><string>agent99</string></llsd>"
			);
	}

	template<> template<>
	void HTTPServiceTestObject::test<7>()
	{
		// test large request
		std::stringstream stream;

		//U32 size = 36 * 1024 * 1024;
		//U32 size = 36 * 1024;
		//std::vector<char> data(size);
		//memset(&(data[0]), '1', size);
		//data[size - 1] = '\0';

		
		//std::string result = httpPOST("web/echo", &(data[0]));

		stream << "<llsd><array>";
		for(U32 i = 0; i < 1000000; ++i)
		{
			stream << "<integer>42</integer>";
		}
		stream << "</array></llsd>";
		llinfos << "HTTPServiceTestObject::test<7>"
				<< stream.str().length() << llendl;
		std::string result = httpPOST("web/echo", stream.str());
		ensure_starts_with("large echo status", result, "HTTP/1.0 200 OK\r\n");
	}

	/* TO DO:
		test generation of not found and method not allowed errors
	*/
}
