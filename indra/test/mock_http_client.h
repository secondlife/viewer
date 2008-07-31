/** 
 * @file mock_http_client.cpp
 * @brief Framework for testing HTTP requests
 * Copyright (c) 2007, Linden Research, Inc.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * $/LicenseInfo$
 */

#include "llsdhttpserver.h"
#include "lliohttpserver.h"
#include "llhttpclient.h"
#include "llformat.h"
#include "llpipeutil.h"
#include "llpumpio.h"

namespace tut
{
	struct MockHttpClient
	{
	public:
		MockHttpClient()
		{
			apr_pool_create(&mPool, NULL);
			mServerPump = new LLPumpIO(mPool);
			mClientPump = new LLPumpIO(mPool);
			
			LLHTTPClient::setPump(*mClientPump);
		}
		
		~MockHttpClient()
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

			while(!mSawCompleted && !timer.hasExpired())
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
	
	protected:
		bool mSawError;
		U32 mStatus;
		std::string mReason;
		bool mSawCompleted;
		LLSD mResult;
		bool mResultDeleted;

		class Result : public LLHTTPClient::Responder
		{
		protected:
			Result(MockHttpClient& client)
				: mClient(client)
			{
			}
		
		public:
			static boost::intrusive_ptr<Result> build(MockHttpClient& client)
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

		private:
			MockHttpClient& mClient;
		};

		friend class Result;

	protected:

		void reset()
		{
			mSawError = false;
			mStatus = 0;
			mSawCompleted = false;
			mResult.clear();
			mResultDeleted = false;
		}

		LLHTTPClient::ResponderPtr newResult()
		{
			reset();
			return Result::build(*this);
		}
	};
}
