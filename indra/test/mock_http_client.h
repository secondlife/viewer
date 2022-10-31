/** 
 * @file mock_http_client.cpp
 * @brief Framework for testing HTTP requests
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
                    llformat("httpFailure() called when not expected, status %d",
                        mStatus); 
                fail(msg);
            }
        }
    
        void ensureStatusError()
        {
            if (!mSawError)
            {
                fail("httpFailure() wasn't called");
            }
        }
        
        LLSD getResult()
        {
            return mResult;
        }
    
    protected:
        bool mSawError;
        S32 mStatus;
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
            
        protected:
            virtual void httpFailure()
            {
                mClient.mSawError = true;
                mClient.mStatus = getStatus();
                mClient.mReason = getReason();
            }

            virtual void httpSuccess()
            {
                mClient.mResult = getContent();
            }

            virtual void httpCompleted()
            {
                LLHTTPClient::Responder::httpCompleted();
                
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
