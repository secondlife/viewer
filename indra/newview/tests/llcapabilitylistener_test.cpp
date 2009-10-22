/**
 * @file   llcapabilitylistener_test.cpp
 * @author Nat Goodspeed
 * @date   2008-12-31
 * @brief  Test for llcapabilitylistener.cpp.
 * 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
#include "llerrorcontrol.h"
#include "tests/networkio.h"
#include "tests/commtest.h"
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
    LLHost getHost() const { return mHost; }
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
            provider.setCapability("good", server + "capability-test");
            provider.setCapability("fail", server + "fail");
        }
    };
    typedef test_group<llcapears_data> llcapears_group;
    typedef llcapears_group::object llcapears_object;
    llcapears_group llsdmgr("llcapabilitylistener");

    struct CaptureError: public LLError::OverrideFatalFunction
    {
        CaptureError():
            LLError::OverrideFatalFunction(boost::bind(&CaptureError::operator(), this, _1))
        {
            LLError::setPrintLocation(false);
        }

        struct FatalException: public std::runtime_error
        {
            FatalException(const std::string& what): std::runtime_error(what) {}
        };

        void operator()(const std::string& message)
        {
            error = message;
            throw FatalException(message);
        }

        std::string error;
    };

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
            CaptureError capture;
            regionPump.post(request);
        }
        catch (const CaptureError::FatalException& e)
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
        ensure_equals(result.asString(), "success");

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
            CaptureError capture;
            regionPump.post(request);
        }
        catch (const CaptureError::FatalException& e)
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
            CaptureError capture;
            regionPump.post(request);
        }
        catch (const CaptureError::FatalException& e)
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
