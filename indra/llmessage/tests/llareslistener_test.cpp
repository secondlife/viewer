/**
 * @file   llareslistener_test.cpp
 * @author Mark Palange
 * @date   2009-02-26
 * @brief  Tests of llareslistener.h.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

// Precompiled header
#include "linden_common.h"
// associated header
#include "../llareslistener.h"
// STL headers
#include <iostream>
// std headers
// external library headers
#include <boost/bind.hpp>

// other Linden headers
#include "llsd.h"
#include "llares.h"
#include "../test/lltut.h"
#include "llevents.h"
#include "tests/wrapllerrs.h"

/*****************************************************************************
*   Dummy stuff
*****************************************************************************/
LLAres::LLAres():
    // Simulate this much of the real LLAres constructor: we need an
    // LLAresListener instance.
    mListener(new LLAresListener("LLAres", this))
{}
LLAres::~LLAres() {}
void LLAres::rewriteURI(const std::string &uri,
					LLAres::UriRewriteResponder *resp)
{
	// This is the only LLAres method I chose to implement.
	// The effect is that LLAres returns immediately with
	// a result that is equal to the input uri.
	std::vector<std::string> result;
	result.push_back(uri);
	resp->rewriteResult(result);
}

LLAres::QueryResponder::~QueryResponder() {}
void LLAres::QueryResponder::queryError(int) {}
void LLAres::QueryResponder::queryResult(char const*, size_t) {}
LLQueryResponder::LLQueryResponder() {}
void LLQueryResponder::queryResult(char const*, size_t) {}
void LLQueryResponder::querySuccess() {}
void LLAres::UriRewriteResponder::queryError(int) {}
void LLAres::UriRewriteResponder::querySuccess() {}
void LLAres::UriRewriteResponder::rewriteResult(const std::vector<std::string>& uris) {}

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct data
    {
        LLAres dummyAres;
    };
    typedef test_group<data> llareslistener_group;
    typedef llareslistener_group::object object;
    llareslistener_group llareslistenergrp("llareslistener");

	struct ResponseCallback
	{
		std::vector<std::string> mURIs;
		bool operator()(const LLSD& response)
		{
            mURIs.clear();
            for (LLSD::array_const_iterator ri(response.beginArray()), rend(response.endArray());
                 ri != rend; ++ri)
            {
                mURIs.push_back(*ri);
            }
            return false;
		}
	};

    template<> template<>
    void object::test<1>()
    {
        set_test_name("test event");
		// Tests the success and failure cases, since they both use 
		// the same code paths in the LLAres responder.
		ResponseCallback response;
        std::string pumpname("trigger");
        // Since we're asking LLEventPumps to obtain() the pump by the desired
        // name, it will persist beyond the current scope, so ensure we
        // disconnect from it when 'response' goes away.
        LLTempBoundListener temp(
            LLEventPumps::instance().obtain(pumpname).listen("rewriteURIresponse",
                                                             boost::bind(&ResponseCallback::operator(), &response, _1)));
        // Now build an LLSD request that will direct its response events to
        // that pump.
		const std::string testURI("login.bar.com");
        LLSD request;
        request["op"] = "rewriteURI";
        request["uri"] = testURI;
        request["reply"] = pumpname;
        LLEventPumps::instance().obtain("LLAres").post(request);
		ensure_equals(response.mURIs.size(), 1);
		ensure_equals(response.mURIs.front(), testURI); 
	}

    template<> template<>
    void object::test<2>()
    {
        set_test_name("bad op");
        WrapLL_ERRS capture;
        LLSD request;
        request["op"] = "foo";
        std::string threw;
        try
        {
            LLEventPumps::instance().obtain("LLAres").post(request);
        }
        catch (const WrapLL_ERRS::FatalException& e)
        {
            threw = e.what();
        }
        ensure_contains("LLAresListener bad op", threw, "bad");
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("bad rewriteURI request");
        WrapLL_ERRS capture;
        LLSD request;
        request["op"] = "rewriteURI";
        std::string threw;
        try
        {
            LLEventPumps::instance().obtain("LLAres").post(request);
        }
        catch (const WrapLL_ERRS::FatalException& e)
        {
            threw = e.what();
        }
        ensure_contains("LLAresListener bad req", threw, "missing");
        ensure_contains("LLAresListener bad req", threw, "reply");
        ensure_contains("LLAresListener bad req", threw, "uri");
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("bad rewriteURI request");
        WrapLL_ERRS capture;
        LLSD request;
        request["op"] = "rewriteURI";
        request["reply"] = "nonexistent";
        std::string threw;
        try
        {
            LLEventPumps::instance().obtain("LLAres").post(request);
        }
        catch (const WrapLL_ERRS::FatalException& e)
        {
            threw = e.what();
        }
        ensure_contains("LLAresListener bad req", threw, "missing");
        ensure_contains("LLAresListener bad req", threw, "uri");
        ensure_does_not_contain("LLAresListener bad req", threw, "reply");
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("bad rewriteURI request");
        WrapLL_ERRS capture;
        LLSD request;
        request["op"] = "rewriteURI";
        request["uri"] = "foo.bar.com";
        std::string threw;
        try
        {
            LLEventPumps::instance().obtain("LLAres").post(request);
        }
        catch (const WrapLL_ERRS::FatalException& e)
        {
            threw = e.what();
        }
        ensure_contains("LLAresListener bad req", threw, "missing");
        ensure_contains("LLAresListener bad req", threw, "reply");
        ensure_does_not_contain("LLAresListener bad req", threw, "uri");
    }
}
