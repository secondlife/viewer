/**
 * @file   lllogin_test.cpp
 * @author Mark Palange
 * @date   2009-02-26
 * @brief  Tests of lllogin.cpp.
 * 
 * $LicenseInfo:firstyear=2009&license=internal$
 * Copyright (c) 2009, Linden Research, Inc.
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
// external library headers
// other Linden headers
#include "llsd.h"
#include "../../../test/lltut.h"
//#define DEBUG_ON
#include "../../../test/debug.h"
#include "llevents.h"
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
		return false;
	}

    LLBoundListener listenTo(LLEventPump& pump)
    {
        return pump.listen(mName, boost::bind(&LoginListener::call, this, _1));
	}

	LLSD lastEvent() const { return mLastEvent; }

    friend std::ostream& operator<<(std::ostream& out, const LoginListener& listener)
    {
        return out << "LoginListener(" << listener.mName << ')';
    }
};

class LLAresListener: public LLEventTrackable
{
	std::string mName;
	LLSD mEvent;
	bool mImmediateResponse;
	bool mMultipleURIResponse;
    Debug mDebug;
	
public:
	LLAresListener(const std::string& name, 
				   bool i = false,
				   bool m = false
				   ) : 
		mName(name),
		mImmediateResponse(i),
		mMultipleURIResponse(m),
        mDebug(stringize(*this))
	{}

	bool handle_event(const LLSD& event)
	{
		mDebug(STRINGIZE("LLAresListener called!: " << event));
		mEvent = event;
		if(mImmediateResponse)
		{
			sendReply();
		}
		return false;
	}

	void sendReply()
	{
		if(mEvent["op"].asString() == "rewriteURI")
		{
			LLSD result;
			if(mMultipleURIResponse)
			{
				result.append(LLSD("login.foo.com"));
			}
			result.append(mEvent["uri"]);
			LLEventPumps::instance().obtain(mEvent["reply"]).post(result);
		}
	}

	LLBoundListener listenTo(LLEventPump& pump)
    {
        return pump.listen(mName, boost::bind(&LLAresListener::handle_event, this, _1));
	}

    friend std::ostream& operator<<(std::ostream& out, const LLAresListener& listener)
    {
        return out << "LLAresListener(" << listener.mName << ')';
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
		LLEventPumps& pumps;
	};

    typedef test_group<llviewerlogin_data> llviewerlogin_group;
    typedef llviewerlogin_group::object llviewerlogin_object;
    llviewerlogin_group llviewerlogingrp("llviewerlogin");

    template<> template<>
    void llviewerlogin_object::test<1>()
    {
        DEBUG;
		// Testing login with immediate responses from Ares and XMLPRC
		// The response from both requests will come before the post request exits.
		// This tests an edge case of the login state handling.
		LLEventStream llaresPump("LLAres"); // Dummy LLAres pump.
		LLEventStream xmlrpcPump("LLXMLRPCTransaction"); // Dummy XMLRPC pump

		bool respond_immediately = true;
		// Have 'dummy ares' respond immediately. 
		LLAresListener dummyLLAres("dummy_llares", respond_immediately);
		dummyLLAres.listenTo(llaresPump);

		// Have dummy XMLRPC respond immediately.
		LLXMLRPCListener dummyXMLRPC("dummy_xmlrpc", respond_immediately);
		dummyXMLRPC.listenTo(xmlrpcPump);

		LLLogin login;

		LoginListener listener("test_ear");
		listener.listenTo(login.getEventPump());

		LLSD credentials;
		credentials["first"] = "foo";
		credentials["last"] = "bar";
		credentials["passwd"] = "secret";

		login.connect("login.bar.com", credentials);

		ensure_equals("Online state", listener.lastEvent()["state"].asString(), "online");
	}

    template<> template<>
    void llviewerlogin_object::test<2>()
    {
        DEBUG;
		// Tests a successful login in with delayed responses. 
		// Also includes 'failure' that cause the login module
		// to re-attempt connection, once from a basic failure
		// and once from the 'indeterminate' response.

		set_test_name("LLLogin multiple srv uris w/ success");

		// Testing normal login procedure.
		LLEventStream llaresPump("LLAres"); // Dummy LLAres pump.
		LLEventStream xmlrpcPump("LLXMLRPCTransaction"); // Dummy XMLRPC pump

		bool respond_immediately = false;
		bool multiple_addresses = true;
		LLAresListener dummyLLAres("dummy_llares", respond_immediately, multiple_addresses);
		dummyLLAres.listenTo(llaresPump);

		LLXMLRPCListener dummyXMLRPC("dummy_xmlrpc");
		dummyXMLRPC.listenTo(xmlrpcPump);

		LLLogin login;

		LoginListener listener("test_ear");
		listener.listenTo(login.getEventPump());

		LLSD credentials;
		credentials["first"] = "foo";
		credentials["last"] = "bar";
		credentials["passwd"] = "secret";

		login.connect("login.bar.com", credentials);

		ensure_equals("SRV state", listener.lastEvent()["change"].asString(), "srvrequest"); 

		dummyLLAres.sendReply();

		// Test Authenticating State prior to first response.
		ensure_equals("Auth state 1", listener.lastEvent()["change"].asString(), "authenticating"); 
		ensure_equals("Attempt 1", listener.lastEvent()["data"]["attempt"].asInteger(), 1); 
		ensure_equals("URI 1", listener.lastEvent()["data"]["request"]["uri"].asString(), "login.foo.com"); 

		// First send emulated LLXMLRPCListener failure,
		// this should return login to the authenticating step and increase the attempt 
		// count.
		LLSD data;
		data["status"] = "OtherError"; 
		data["errorcode"] = 0;
		data["error"] = "dummy response";
		data["transfer_rate"] = 0;
		dummyXMLRPC.setResponse(data);
		dummyXMLRPC.sendReply();

		ensure_equals("Fail back to authenticate 1", listener.lastEvent()["change"].asString(), "authenticating"); 
		ensure_equals("Attempt 2", listener.lastEvent()["data"]["attempt"].asInteger(), 2); 
		ensure_equals("URI 2", listener.lastEvent()["data"]["request"]["uri"].asString(), "login.bar.com"); 

		// Now send the 'indeterminate' response.
		data.clear();
		data["status"] = "Complete"; // StatusComplete
		data["errorcode"] = 0;
		data["error"] = "dummy response";
		data["transfer_rate"] = 0;
		data["responses"]["login"] = "indeterminate";
		data["responses"]["next_url"] = "login.indeterminate.com";			
		data["responses"]["next_method"] = "test_login_method"; 			
		dummyXMLRPC.setResponse(data);
		dummyXMLRPC.sendReply();

		ensure_equals("Fail back to authenticate 2", listener.lastEvent()["change"].asString(), "authenticating"); 
		ensure_equals("Attempt 3", listener.lastEvent()["data"]["attempt"].asInteger(), 3); 
		ensure_equals("URI 3", listener.lastEvent()["data"]["request"]["uri"].asString(), "login.indeterminate.com"); 
		ensure_equals("Method 3", listener.lastEvent()["data"]["request"]["method"].asString(), "test_login_method"); 

		// Finally let the auth succeed.
		data.clear();
		data["status"] = "Complete"; // StatusComplete
		data["errorcode"] = 0;
		data["error"] = "dummy response";
		data["transfer_rate"] = 0;
		data["responses"]["login"] = "true";
		dummyXMLRPC.setResponse(data);
		dummyXMLRPC.sendReply();

		ensure_equals("Success state", listener.lastEvent()["state"].asString(), "online");

		login.disconnect();

		ensure_equals("Disconnected state", listener.lastEvent()["state"].asString(), "offline");
	}

    template<> template<>
    void llviewerlogin_object::test<3>()
    {
        DEBUG;
		// Test completed response, that fails to login.
		set_test_name("LLLogin valid response, failure (eg. bad credentials)");

		// Testing normal login procedure.
		LLEventStream llaresPump("LLAres"); // Dummy LLAres pump.
		LLEventStream xmlrpcPump("LLXMLRPCTransaction"); // Dummy XMLRPC pump

		LLAresListener dummyLLAres("dummy_llares");
		dummyLLAres.listenTo(llaresPump);

		LLXMLRPCListener dummyXMLRPC("dummy_xmlrpc");
		dummyXMLRPC.listenTo(xmlrpcPump);

		LLLogin login;
		LoginListener listener("test_ear");
		listener.listenTo(login.getEventPump());

		LLSD credentials;
		credentials["first"] = "who";
		credentials["last"] = "what";
		credentials["passwd"] = "badpasswd";

		login.connect("login.bar.com", credentials);

		ensure_equals("SRV state", listener.lastEvent()["change"].asString(), "srvrequest"); 

		dummyLLAres.sendReply();

		ensure_equals("Auth state", listener.lastEvent()["change"].asString(), "authenticating"); 

		// Send the failed auth request reponse
		LLSD data;
		data["status"] = "Complete";
		data["errorcode"] = 0;
		data["error"] = "dummy response";
		data["transfer_rate"] = 0;
		data["responses"]["login"] = "false";
		dummyXMLRPC.setResponse(data);
		dummyXMLRPC.sendReply();

		ensure_equals("Failed to offline", listener.lastEvent()["state"].asString(), "offline");
	}

    template<> template<>
    void llviewerlogin_object::test<4>()
    {
        DEBUG;
		// Test incomplete response, that end the attempt.
		set_test_name("LLLogin valid response, failure (eg. bad credentials)");

		// Testing normal login procedure.
		LLEventStream llaresPump("LLAres"); // Dummy LLAres pump.
		LLEventStream xmlrpcPump("LLXMLRPCTransaction"); // Dummy XMLRPC pump

		LLAresListener dummyLLAres("dummy_llares");
		dummyLLAres.listenTo(llaresPump);

		LLXMLRPCListener dummyXMLRPC("dummy_xmlrpc");
		dummyXMLRPC.listenTo(xmlrpcPump);

		LLLogin login;
		LoginListener listener("test_ear");
		listener.listenTo(login.getEventPump());

		LLSD credentials;
		credentials["first"] = "these";
		credentials["last"] = "don't";
		credentials["passwd"] = "matter";

		login.connect("login.bar.com", credentials);

		ensure_equals("SRV state", listener.lastEvent()["change"].asString(), "srvrequest"); 

		dummyLLAres.sendReply();

		ensure_equals("Auth state", listener.lastEvent()["change"].asString(), "authenticating"); 

		// Send the failed auth request reponse
		LLSD data;
		data["status"] = "OtherError";
		data["errorcode"] = 0;
		data["error"] = "dummy response";
		data["transfer_rate"] = 0;
		dummyXMLRPC.setResponse(data);
		dummyXMLRPC.sendReply();

		ensure_equals("Failed to offline", listener.lastEvent()["state"].asString(), "offline");
	}

	template<> template<>
    void llviewerlogin_object::test<5>()
    {
        DEBUG;
		// Test SRV request timeout.
		set_test_name("LLLogin SRV timeout testing");

		// Testing normal login procedure.
		LLEventStream llaresPump("LLAres"); // Dummy LLAres pump.

		// LLAresListener dummyLLAres("dummy_llares");
		// dummyLLAres.listenTo(llaresPump);

		LLLogin login;
		LoginListener listener("test_ear");
		listener.listenTo(login.getEventPump());

		LLSD credentials;
		credentials["first"] = "these";
		credentials["last"] = "don't";
		credentials["passwd"] = "matter";
		credentials["cfg_srv_timeout"] = 0.0f;

		login.connect("login.bar.com", credentials);

		ensure_equals("SRV State", listener.lastEvent()["change"].asString(), "srvrequest"); 

		// Get the mainloop eventpump, which needs a pinging in order to drive the 
		// SRV timeout.
		LLEventPump& mainloop(LLEventPumps::instance().obtain("mainloop"));
		LLSD frame_event;
		mainloop.post(frame_event);

		// In this state we have NOT sent a reply from LLAresListener -- in
		// fact there's no such object. Nonetheless, we expect the timeout to
		// have stepped the login module forward to try to authenticate with
		// the original URI.
		ensure_equals("Auth state", listener.lastEvent()["change"].asString(), "authenticating"); 
		ensure_equals("Attempt", listener.lastEvent()["data"]["attempt"].asInteger(), 1); 
		ensure_equals("URI", listener.lastEvent()["data"]["request"]["uri"].asString(), "login.bar.com"); 
	}
}
