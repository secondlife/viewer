/**
 * @file   llcapabilitylistener_test.cpp
 * @author Nat Goodspeed
 * @date   2008-12-31
 * @brief  Test for llcapabilitylistener.cpp.
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

// Precompiled header
#include "../llviewerprecompiledheaders.h"
// Own header
#include "../llcapabilitylistener.h"
// STL headers
#include <stdexcept>
#include <map>
#include <vector>
// std headers
// external library headers
#include "boost/bind.hpp"
// other Linden headers
#include "../test/lltut.h"
#include "../llcapabilityprovider.h"
#include "lluuid.h"
#include "tests/networkio.h"
#include "tests/commtest.h"
#include "tests/wrapllerrs.h"
#include "message.h"
#include "stringize.h"

#if defined(LL_WINDOWS)
#pragma warning(disable: 4355)      // using 'this' in base-class ctor initializer expr
#endif

/*****************************************************************************
*   TestCapabilityProvider
*****************************************************************************/
struct TestCapabilityProvider: public LLCapabilityProvider
{
    TestCapabilityProvider(const LLHost& host):
        mHost(host)
    {}

    std::string getCapability(const std::string& cap) const
    {
        CapMap::const_iterator found = mCaps.find(cap);
        if (found != mCaps.end())
            return found->second;
        // normal LLViewerRegion lookup failure mode
        return "";
    }
    void setCapability(const std::string& cap, const std::string& url)
    {
        mCaps[cap] = url;
    }
    const LLHost& getHost() const { return mHost; }
    std::string getDescription() const { return "TestCapabilityProvider"; }

    LLHost mHost;
    typedef std::map<std::string, std::string> CapMap;
    CapMap mCaps;
};

/*****************************************************************************
*   Dummy LLMessageSystem methods
*****************************************************************************/
/*==========================================================================*|
// This doesn't work because we're already linking in llmessage.a, and we get
// duplicate-symbol errors from the linker. Perhaps if I wanted to go through
// the exercise of providing dummy versions of every single symbol defined in
// message.o -- maybe some day.
typedef std::vector< std::pair<std::string, std::string> > StringPairVector;
StringPairVector call_history;

S32 LLMessageSystem::sendReliable(const LLHost& host)
{
    call_history.push_back(StringPairVector::value_type("sendReliable", stringize(host)));
    return 0;
}
|*==========================================================================*/

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llcapears_data: public commtest_data
    {
        TestCapabilityProvider provider;
        LLCapabilityListener regionListener;
        LLEventPump& regionPump;

        llcapears_data():
            provider(host),
            regionListener("testCapabilityListener", NULL, provider, LLUUID(), LLUUID()),
            regionPump(regionListener.getCapAPI())
        {
            LLCurl::initClass();
            provider.setCapability("good", server + "capability-test");
            provider.setCapability("fail", server + "fail");
        }
    };
    typedef test_group<llcapears_data> llcapears_group;
    typedef llcapears_group::object llcapears_object;
    llcapears_group llsdmgr("llcapabilitylistener");

    template<> template<>
    void llcapears_object::test<1>()
    {
        LLSD request, body;
        body["data"] = "yes";
        request["payload"] = body;
        request["reply"] = replyPump.getName();
        request["error"] = errorPump.getName();
        std::string threw;
        try
        {
            WrapLL_ERRS capture;
            regionPump.post(request);
        }
        catch (const WrapLL_ERRS::FatalException& e)
        {
            threw = e.what();
        }
        ensure_contains("missing capability name", threw, "without 'message' key");
    }

    template<> template<>
    void llcapears_object::test<2>()
    {
        LLSD request, body;
        body["data"] = "yes";
        request["message"] = "good";
        request["payload"] = body;
        request["reply"] = replyPump.getName();
        request["error"] = errorPump.getName();
        regionPump.post(request);
        ensure("got response", netio.pump());
        ensure("success response", success);
        ensure_equals(result["reply"].asString(), "success");

        body["status"] = 499;
        body["reason"] = "custom error message";
        request["message"] = "fail";
        request["payload"] = body;
        regionPump.post(request);
        ensure("got response", netio.pump());
        ensure("failure response", ! success);
        ensure_equals(result["status"].asInteger(), body["status"].asInteger());
        ensure_equals(result["reason"].asString(),  body["reason"].asString());
    }

    template<> template<>
    void llcapears_object::test<3>()
    {
        LLSD request, body;
        body["data"] = "yes";
        request["message"] = "unknown";
        request["payload"] = body;
        request["reply"] = replyPump.getName();
        request["error"] = errorPump.getName();
        std::string threw;
        try
        {
            WrapLL_ERRS capture;
            regionPump.post(request);
        }
        catch (const WrapLL_ERRS::FatalException& e)
        {
            threw = e.what();
        }
        ensure_contains("bad capability name", threw, "unsupported capability");
    }

    struct TestMapper: public LLCapabilityListener::CapabilityMapper
    {
        // Instantiator gets to specify whether mapper expects a reply.
        // I'd really like to be able to test CapabilityMapper::buildMessage()
        // functionality, too, but -- even though LLCapabilityListener accepts
        // the LLMessageSystem* that it passes to CapabilityMapper --
        // LLMessageSystem::sendReliable(const LLHost&) isn't virtual, so it's
        // not helpful to pass a subclass instance. I suspect that making any
        // LLMessageSystem methods virtual would provoke howls of outrage,
        // given how heavily it's used. Nor can I just provide a local
        // definition of LLMessageSystem::sendReliable(const LLHost&) because
        // we're already linking in the rest of message.o via llmessage.a, and
        // that produces duplicate-symbol link errors.
        TestMapper(const std::string& replyMessage = std::string()):
            LLCapabilityListener::CapabilityMapper("test", replyMessage)
        {}
        virtual void buildMessage(LLMessageSystem* msg,
                                  const LLUUID& agentID,
                                  const LLUUID& sessionID,
                                  const std::string& capabilityName,
                                  const LLSD& payload) const
        {
            msg->newMessageFast(_PREHASH_SetStartLocationRequest);
            msg->nextBlockFast( _PREHASH_AgentData);
            msg->addUUIDFast(_PREHASH_AgentID, agentID);
            msg->addUUIDFast(_PREHASH_SessionID, sessionID);
            msg->nextBlockFast( _PREHASH_StartLocationData);
            // corrected by sim
            msg->addStringFast(_PREHASH_SimName, "");
            msg->addU32Fast(_PREHASH_LocationID, payload["HomeLocation"]["LocationId"].asInteger());
/*==========================================================================*|
            msg->addVector3Fast(_PREHASH_LocationPos,
                                ll_vector3_from_sdmap(payload["HomeLocation"]["LocationPos"]));
            msg->addVector3Fast(_PREHASH_LocationLookAt,
                                ll_vector3_from_sdmap(payload["HomeLocation"]["LocationLookAt"]));
|*==========================================================================*/
        }
    };

    template<> template<>
    void llcapears_object::test<4>()
    {
        TestMapper testMapper("WantReply");
        LLSD request, body;
        body["data"] = "yes";
        request["message"] = "test";
        request["payload"] = body;
        request["reply"] = replyPump.getName();
        request["error"] = errorPump.getName();
        std::string threw;
        try
        {
            WrapLL_ERRS capture;
            regionPump.post(request);
        }
        catch (const WrapLL_ERRS::FatalException& e)
        {
            threw = e.what();
        }
        ensure_contains("capability mapper wants reply", threw, "unimplemented support for reply message");
    }

    template<> template<>
    void llcapears_object::test<5>()
    {
        TestMapper testMapper;
        std::string threw;
        try
        {
            TestMapper testMapper2;
        }
        catch (const std::runtime_error& e)
        {
            threw = e.what();
        }
        ensure_contains("no dup cap mapper", threw, "DupCapMapper");
    }
}
