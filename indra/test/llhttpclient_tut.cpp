/** 
 * @file llhttpclient_tut.cpp
 * @brief Testing the HTTP client classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

/**
 *
 * These classes test the HTTP client framework.
 *
 */

#include <tut/tut.hpp>
#include "linden_common.h"

// These are too slow on Windows to actually include in the build. JC
#if !LL_WINDOWS

#include "lltut.h"
#include "llhttpclient.h"
#include "llformat.h"
#include "llpipeutil.h"
#include "llpumpio.h"

#include "llsdhttpserver.h"
#include "lliohttpserver.h"
#include "lliosocket.h"

namespace tut
{
	LLSD storage;
	
	class LLSDStorageNode : public LLHTTPNode
	{
	public:
		LLSD simpleGet() const					{ return storage; }
		LLSD simplePut(const LLSD& value) const	{ storage = value; return LLSD(); }
	};

	class ErrorNode : public LLHTTPNode
	{
	public:
		void get(ResponsePtr r, const LLSD& context) const
			{ r->status(599, "Intentional error"); }
		void post(ResponsePtr r, const LLSD& context, const LLSD& input) const
			{ r->status(input["status"], input["reason"]); }
	};

	class TimeOutNode : public LLHTTPNode
	{
	public:
		void get(ResponsePtr r, const LLSD& context) const
		{
            /* do nothing, the request will eventually time out */ 
		}
	};

	LLHTTPRegistration<LLSDStorageNode> gStorageNode("/test/storage");
	LLHTTPRegistration<ErrorNode>		gErrorNode("/test/error");
	LLHTTPRegistration<TimeOutNode>		gTimeOutNode("/test/timeout");

	struct HTTPClientTestData
	{
	public:
		HTTPClientTestData()
		{
			apr_pool_create(&mPool, NULL);
			mServerPump = new LLPumpIO(mPool);
			mClientPump = new LLPumpIO(mPool);
			
			LLHTTPClient::setPump(*mClientPump);
		}
		
		~HTTPClientTestData()
		{
			delete mServerPump;
			delete mClientPump;
			apr_pool_destroy(mPool);
		}

		void setupTheServer()
		{
			LLHTTPNode& root = LLIOHTTPServer::create(mPool, *mServerPump, 8888);

			LLHTTPStandardServices::useServices();
			LLHTTPRegistrar::buildAllServices(root);
		}
		
		void runThePump(float timeout = 100.0f)
		{
			LLTimer timer;
			timer.setTimerExpirySec(timeout);

			while(!mSawCompleted && !mSawCompletedHeader && !timer.hasExpired())
			{
				if (mServerPump)
				{
					mServerPump->pump();
					mServerPump->callback();
				}
				if (mClientPump)
				{
					mClientPump->pump();
					mClientPump->callback();
				}
			}
		}

		void killServer()
		{
			delete mServerPump;
			mServerPump = NULL;
		}
	
	private:
		apr_pool_t* mPool;
		LLPumpIO* mServerPump;
		LLPumpIO* mClientPump;

		
	protected:
		void ensureStatusOK()
		{
			if (mSawError)
			{
				std::string msg =
					llformat("error() called when not expected, status %d",
						mStatus); 
				fail(msg);
			}
		}
	
		void ensureStatusError()
		{
			if (!mSawError)
			{
				fail("error() wasn't called");
			}
		}
		
		LLSD getResult()
		{
			return mResult;
		}
		LLSD getHeader()
		{
			return mHeader;
		}
	
	protected:
		bool mSawError;
		U32 mStatus;
		std::string mReason;
		bool mSawCompleted;
		bool mSawCompletedHeader;
		LLSD mResult;
		LLSD mHeader;
		bool mResultDeleted;

		class Result : public LLHTTPClient::Responder
		{
		protected:
			Result(HTTPClientTestData& client)
				: mClient(client)
			{
			}
		
		public:
			static boost::intrusive_ptr<Result> build(HTTPClientTestData& client)
			{
				return boost::intrusive_ptr<Result>(new Result(client));
			}
			
			~Result()
			{
				mClient.mResultDeleted = true;
			}
			
			virtual void error(U32 status, const std::string& reason)
			{
				mClient.mSawError = true;
				mClient.mStatus = status;
				mClient.mReason = reason;
			}

			virtual void result(const LLSD& content)
			{
				mClient.mResult = content;
			}

			virtual void completed(
							U32 status, const std::string& reason,
							const LLSD& content)
			{
				LLHTTPClient::Responder::completed(status, reason, content);
				
				mClient.mSawCompleted = true;
			}

			virtual void completedHeader(
				U32 status, const std::string& reason,
				const LLSD& content)
			{
				mClient.mHeader = content;
				mClient.mSawCompletedHeader = true;
			}

		private:
			HTTPClientTestData& mClient;
		};

		friend class Result;

	protected:
		LLHTTPClient::ResponderPtr newResult()
		{
			mSawError = false;
			mStatus = 0;
			mSawCompleted = false;
			mSawCompletedHeader = false;
			mResult.clear();
			mHeader.clear();
			mResultDeleted = false;
			
			return Result::build(*this);
		}
	};
	
	
	typedef test_group<HTTPClientTestData>	HTTPClientTestGroup;
	typedef HTTPClientTestGroup::object		HTTPClientTestObject;
	HTTPClientTestGroup httpClientTestGroup("http_client");

	template<> template<>
	void HTTPClientTestObject::test<1>()
	{
		LLHTTPClient::get("http://www.google.com/", newResult());
		runThePump();
		ensureStatusOK();
		ensure("result object wasn't destroyed", mResultDeleted);
	}

	template<> template<>
	void HTTPClientTestObject::test<2>()
	{
		skip("error test depends on dev's local ISP not supplying \"helpful\" search page");
		LLHTTPClient::get("http://www.invalid", newResult());
		runThePump();
		ensureStatusError();
	}

	template<> template<>
		void HTTPClientTestObject::test<3>()
	{
		LLSD sd;

		sd["list"][0]["one"] = 1;
		sd["list"][0]["two"] = 2;
		sd["list"][1]["three"] = 3;
		sd["list"][1]["four"] = 4;
		
		setupTheServer();

		LLHTTPClient::post("http://localhost:8888/web/echo", sd, newResult());
		runThePump();
		ensureStatusOK();
		ensure_equals("echoed result matches", getResult(), sd);
	}

	template<> template<>
		void HTTPClientTestObject::test<4>()
	{
		LLSD sd;

		sd["message"] = "This is my test message.";

		setupTheServer();
		LLHTTPClient::put("http://localhost:8888/test/storage", sd, newResult());
		runThePump();
		ensureStatusOK();

		LLHTTPClient::get("http://localhost:8888/test/storage", newResult());
		runThePump();
		ensureStatusOK();
		ensure_equals("echoed result matches", getResult(), sd);
	
	}

	template<> template<>
		void HTTPClientTestObject::test<5>()
	{
		LLSD sd;
		sd["status"] = 543;
		sd["reason"] = "error for testing";

		setupTheServer();

		LLHTTPClient::post("http://localhost:8888/test/error", sd, newResult());
		runThePump();
		ensureStatusError();
		ensure_contains("reason", mReason, sd["reason"]);
	}

	template<> template<>
		void HTTPClientTestObject::test<6>()
	{
		setupTheServer();

		LLHTTPClient::get("http://localhost:8888/test/timeout", newResult());
		runThePump(1.0f);
		killServer();
		runThePump();
		ensureStatusError();
		ensure_equals("reason", mReason, "STATUS_ERROR");
	}

	template<> template<>
		void HTTPClientTestObject::test<7>()
	{
		// Can not use the little mini server.  The blocking request
		// won't ever let it run.  Instead get from a known LLSD
		// source and compare results with the non-blocking get which
		// is tested against the mini server earlier.
		skip("secondlife.com is not reliable enough for unit tests.");


		LLSD expected;

		LLHTTPClient::get("http://secondlife.com/xmlhttp/homepage.php", newResult());
		runThePump();
		ensureStatusOK();
		expected = getResult();

		LLSD result;
		result = LLHTTPClient::blockingGet("http://secondlife.com/xmlhttp/homepage.php");
		LLSD body = result["body"];
		ensure_equals("echoed result matches", body.size(), expected.size());
	}
	template<> template<>
		void HTTPClientTestObject::test<8>()
	{
		// This is testing for the presence of the Header in the returned results
		// from an HTTP::get call.
		LLHTTPClient::get("http://www.google.com/", newResult());
		runThePump();
		ensureStatusOK();
		LLSD header = getHeader();
		ensure_equals("got a header", header.emptyMap().asBoolean(), FALSE);
	}
	template<> template<>
	void HTTPClientTestObject::test<9>()
	{
		LLHTTPClient::head("http://www.google.com/", newResult());
		runThePump();
		ensureStatusOK();
		ensure("result object wasn't destroyed", mResultDeleted);
	}
}

#endif	// !LL_WINDOWS
