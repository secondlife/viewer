/**
 * @file llhttpclient_test.cpp
 * @brief Testing the HTTP client classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

/**
 *
 * These classes test the HTTP client framework.
 *
 */

#include <tut/tut.hpp>
#include "linden_common.h"

#include "lltut.h"
#include "llhttpclient.h"
#include "llformat.h"
#include "llpipeutil.h"
#include "llproxy.h"
#include "llpumpio.h"

#include "lliosocket.h"
#include "llstring.h"
#include "stringize.h"
#include "llcleanup.h"

namespace tut
{
    struct HTTPClientTestData
    {
    public:
        HTTPClientTestData():
            PORT(LLStringUtil::getenv("PORT")),
            // Turning NULL PORT into empty string doesn't make things work;
            // that's just to keep this initializer from blowing up. We test
            // PORT separately in the constructor body.
            local_server(STRINGIZE("http://127.0.0.1:" << PORT << "/"))
        {
            ensure("Set environment variable PORT to local test server port", !PORT.empty());
            apr_pool_create(&mPool, NULL);
            LLCurl::initClass(false);
            mClientPump = new LLPumpIO(mPool);

            LLHTTPClient::setPump(*mClientPump);
        }

        ~HTTPClientTestData()
        {
            delete mClientPump;
            SUBSYSTEM_CLEANUP(LLProxy);
            apr_pool_destroy(mPool);
        }

        void runThePump(float timeout = 100.0f)
        {
            LLTimer timer;
            timer.setTimerExpirySec(timeout);

            while(!mSawCompleted && !mSawCompletedHeader && !timer.hasExpired())
            {
                LLFrameTimer::updateFrameTime();
                if (mClientPump)
                {
                    mClientPump->pump();
                    mClientPump->callback();
                }
            }
        }

        const std::string PORT;
        const std::string local_server;

    private:
        apr_pool_t* mPool;
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
            static Result* build(HTTPClientTestData& client)
            {
                return new Result(client);
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
                mClient.mSawCompletedHeader = true;
                mClient.mHeader = getResponseHeaders();
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


    typedef test_group<HTTPClientTestData>  HTTPClientTestGroup;
    typedef HTTPClientTestGroup::object     HTTPClientTestObject;
    HTTPClientTestGroup httpClientTestGroup("http_client");

    template<> template<>
    void HTTPClientTestObject::test<1>()
    {
        LLHTTPClient::get(local_server, newResult());
        runThePump();
        ensureStatusOK();
        ensure("result object wasn't destroyed", mResultDeleted);
    }

    template<> template<>
    void HTTPClientTestObject::test<2>()
    {
        // Please nobody listen on this particular port...
        LLHTTPClient::get("http://127.0.0.1:7950", newResult());
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

        LLHTTPClient::post(local_server + "web/echo", sd, newResult());
        runThePump();
        ensureStatusOK();
        ensure_equals("echoed result matches", getResult(), sd);
    }

    template<> template<>
        void HTTPClientTestObject::test<4>()
    {
        LLSD sd;

        sd["message"] = "This is my test message.";

        LLHTTPClient::put(local_server + "test/storage", sd, newResult());
        runThePump();
        ensureStatusOK();

        LLHTTPClient::get(local_server + "test/storage", newResult());
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

        LLHTTPClient::post(local_server + "test/error", sd, newResult());
        runThePump();
        ensureStatusError();
        ensure_contains("reason", mReason, sd["reason"]);
    }

    template<> template<>
        void HTTPClientTestObject::test<6>()
    {
        const F32 timeout = 1.0f;
        LLHTTPClient::get(local_server + "test/timeout", newResult(), LLSD(), timeout);
        runThePump(timeout * 5.0f);
        ensureStatusError();
        ensure_equals("reason", mReason, "STATUS_EXPIRED");
    }

    template<> template<>
        void HTTPClientTestObject::test<7>()
    {
        LLHTTPClient::get(local_server, newResult());
        runThePump();
        ensureStatusOK();
        LLSD expected = getResult();

        LLSD result;
        result = LLHTTPClient::blockingGet(local_server);
        LLSD body = result["body"];
        ensure_equals("echoed result matches", body.size(), expected.size());
    }
    template<> template<>
        void HTTPClientTestObject::test<8>()
    {
        // This is testing for the presence of the Header in the returned results
        // from an HTTP::get call.
        LLHTTPClient::get(local_server, newResult());
        runThePump();
        ensureStatusOK();
        LLSD header = getHeader();
        ensure("got a header", ! header.emptyMap().asBoolean());
    }
    template<> template<>
    void HTTPClientTestObject::test<9>()
    {
        LLHTTPClient::head(local_server, newResult());
        runThePump();
        ensureStatusOK();
        ensure("result object wasn't destroyed", mResultDeleted);
    }
}
