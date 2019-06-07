/**
 * @file   lllogin_test.cpp
 * @author Mark Palange
 * @date   2009-02-26
 * @brief  Tests of lllogin.cpp.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2009-2010, Linden Research, Inc.
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

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

// Precompiled header
#include "linden_common.h"
// associated header
#include "../lllogin.h"
// STL headers
// std headers
#include <iostream>
#include <chrono>
// external library headers
// other Linden headers
#include "llsd.h"
#include "../../../test/lltut.h"
//#define DEBUG_ON
#include "../../../test/debug.h"
#include "llevents.h"
#include "lleventcoro.h"
#include "stringize.h"

#if LL_WINDOWS
#define skipwin(arg) skip(arg)
#define skipmac(arg)
#define skiplinux(arg)
#elif LL_DARWIN
#define skipwin(arg)
#define skipmac(arg) skip(arg)
#define skiplinux(arg)
#elif LL_LINUX
#define skipwin(arg)
#define skipmac(arg)
#define skiplinux(arg) skip(arg)
#endif

/*****************************************************************************
*   Helper classes
*****************************************************************************/
// This is a listener to receive results from lllogin.
class LoginListener: public LLEventTrackable
{
    std::string mName;
    LLSD mLastEvent;
    size_t mCalls{ 0 };
    Debug mDebug;
public:
    LoginListener(const std::string& name) : 
        mName(name),
        mDebug(stringize(*this))
    {}

    bool call(const LLSD& event)
    {
        mDebug(STRINGIZE("LoginListener called!: " << event));
        
        mLastEvent = event;
        ++mCalls;
        return false;
    }

    LLBoundListener listenTo(LLEventPump& pump)
    {
        return pump.listen(mName, boost::bind(&LoginListener::call, this, _1));
    }

    LLSD lastEvent() const { return mLastEvent; }

    size_t getCalls() const { return mCalls; }

    LLSD waitFor(size_t prevcalls, F32 seconds) const
    {
        // remember when we started waiting
        auto start = std::chrono::system_clock::now();
        // Break loop when the LLEventPump on which we're listening calls call()
        while (getCalls() <= prevcalls)
        {
            // but if we've been spinning here too long, test failed
            // how long have we been here, anyway?
            auto now = std::chrono::system_clock::now();
            // the default ratio for duration is seconds
            std::chrono::duration<F32> elapsed = (now - start);
            if (elapsed.count() > seconds)
            {
                tut::fail("LoginListener::waitFor() timed out");
            }
            // haven't yet received the new call, nor have we timed out --
            // just wait
            llcoro::suspend();
        }
        // oh good, we've gotten at least one new call! Return its event.
        return lastEvent();
    }

    friend std::ostream& operator<<(std::ostream& out, const LoginListener& listener)
    {
        return out << "LoginListener(" << listener.mName << ')';
    }
};

class LLXMLRPCListener: public LLEventTrackable
{
	std::string mName;
	LLSD mEvent;
	bool mImmediateResponse;
	LLSD mResponse;
    Debug mDebug;

public:
	LLXMLRPCListener(const std::string& name, 
					 bool i = false,
					 const LLSD& response = LLSD()
					 ) : 
		mName(name),
		mImmediateResponse(i),
		mResponse(response),
        mDebug(stringize(*this))
	{
		if(mResponse.isUndefined())
		{
			mResponse["status"] = "Complete"; // StatusComplete
			mResponse["errorcode"] = 0;
			mResponse["error"] = "dummy response";
			mResponse["transfer_rate"] = 0;
			mResponse["responses"]["login"] = true;
		}
	}

	void setResponse(const LLSD& r) 
	{ 
		mResponse = r; 
	}

	bool handle_event(const LLSD& event)
	{
		mDebug(STRINGIZE("LLXMLRPCListener called!: " << event));
		mEvent = event;
		if(mImmediateResponse)
		{
			sendReply();
		}
		return false;
	}

	void sendReply()
	{
		LLEventPumps::instance().obtain(mEvent["reply"]).post(mResponse);
	}

	LLBoundListener listenTo(LLEventPump& pump)
    {
        return pump.listen(mName, boost::bind(&LLXMLRPCListener::handle_event, this, _1));
	}

    friend std::ostream& operator<<(std::ostream& out, const LLXMLRPCListener& listener)
    {
        return out << "LLXMLRPCListener(" << listener.mName << ')';
    }
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llviewerlogin_data
    {
        llviewerlogin_data() :
            pumps(LLEventPumps::instance())
        {}
        ~llviewerlogin_data()
        {
            pumps.clear();
        }
        LLEventPumps& pumps;
    };

    typedef test_group<llviewerlogin_data> llviewerlogin_group;
    typedef llviewerlogin_group::object llviewerlogin_object;
    llviewerlogin_group llviewerlogingrp("LLViewerLogin");

    template<> template<>
    void llviewerlogin_object::test<1>()
    {
        DEBUG;
		// Testing login with an immediate response from XMLPRC
		// The response will come before the post request exits.
		// This tests an edge case of the login state handling.
		LLEventStream xmlrpcPump("LLXMLRPCTransaction"); // Dummy XMLRPC pump

		bool respond_immediately = true;

		// Have dummy XMLRPC respond immediately.
		LLXMLRPCListener dummyXMLRPC("dummy_xmlrpc", respond_immediately);
		LLTempBoundListener conn1 = dummyXMLRPC.listenTo(xmlrpcPump);

		LLLogin login;

		LoginListener listener("test_ear");
		LLTempBoundListener conn2 = listener.listenTo(login.getEventPump());

		LLSD credentials;
		credentials["first"] = "foo";
		credentials["last"] = "bar";
		credentials["passwd"] = "secret";

		login.connect("login.bar.com", credentials);
		llcoro::suspend();

		ensure_equals("Online state", listener.lastEvent()["state"].asString(), "online");
	}

    template<> template<>
    void llviewerlogin_object::test<2>()
    {
        DEBUG;
		// Test completed response, that fails to login.
		set_test_name("LLLogin valid response, failure (eg. bad credentials)");

		// Testing normal login procedure.
		LLEventStream xmlrpcPump("LLXMLRPCTransaction"); // Dummy XMLRPC pump

		LLXMLRPCListener dummyXMLRPC("dummy_xmlrpc");
		LLTempBoundListener conn1 = dummyXMLRPC.listenTo(xmlrpcPump);

		LLLogin login;
		LoginListener listener("test_ear");
		LLTempBoundListener conn2 = listener.listenTo(login.getEventPump());

		LLSD credentials;
		credentials["first"] = "who";
		credentials["last"] = "what";
		credentials["passwd"] = "badpasswd";

		login.connect("login.bar.com", credentials);
		llcoro::suspend();

		ensure_equals("Auth state", listener.lastEvent()["change"].asString(), "authenticating"); 

		auto prev = listener.getCalls();

		// Send the failed auth request reponse
		LLSD data;
		data["status"] = "Complete";
		data["errorcode"] = 0;
		data["error"] = "dummy response";
		data["transfer_rate"] = 0;
		data["responses"]["login"] = "false";
		dummyXMLRPC.setResponse(data);
		dummyXMLRPC.sendReply();
		// we happen to know LLLogin uses a 10-second timeout to try to sync
		// with SLVersionChecker -- allow at least that much time before
		// giving up
		listener.waitFor(prev, 11.0);

		ensure_equals("Failed to offline", listener.lastEvent()["state"].asString(), "offline");
	}

    template<> template<>
    void llviewerlogin_object::test<3>()
    {
        DEBUG;
		// Test incomplete response, that end the attempt.
		set_test_name("LLLogin valid response, failure (eg. bad credentials)");

		// Testing normal login procedure.
		LLEventStream xmlrpcPump("LLXMLRPCTransaction"); // Dummy XMLRPC pump

		LLXMLRPCListener dummyXMLRPC("dummy_xmlrpc");
		LLTempBoundListener conn1 = dummyXMLRPC.listenTo(xmlrpcPump);

		LLLogin login;
		LoginListener listener("test_ear");
		LLTempBoundListener conn2 = listener.listenTo(login.getEventPump());

		LLSD credentials;
		credentials["first"] = "these";
		credentials["last"] = "don't";
		credentials["passwd"] = "matter";

		login.connect("login.bar.com", credentials);
		llcoro::suspend();

		ensure_equals("Auth state", listener.lastEvent()["change"].asString(), "authenticating"); 

		auto prev = listener.getCalls();

		// Send the failed auth request reponse
		LLSD data;
		data["status"] = "OtherError";
		data["errorcode"] = 0;
		data["error"] = "dummy response";
		data["transfer_rate"] = 0;
		dummyXMLRPC.setResponse(data);
		dummyXMLRPC.sendReply();
		// we happen to know LLLogin uses a 10-second timeout to try to sync
		// with SLVersionChecker -- allow at least that much time before
		// giving up
		listener.waitFor(prev, 11.0);

		ensure_equals("Failed to offline", listener.lastEvent()["state"].asString(), "offline");
	}
}
