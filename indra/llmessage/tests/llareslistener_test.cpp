/**
 * @file   llareslistener_test.cpp
 * @author Mark Palange
 * @date   2009-02-26
 * @brief  Tests of llareslistener.h.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "../test/lldoctest.h"
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
TEST_SUITE("UnknownSuite") {

struct data
{

        LLAres dummyAres;
    
};

TEST_CASE_FIXTURE(data, "test_1")
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

TEST_CASE_FIXTURE(data, "test_2")
{

        set_test_name("bad op");
        WrapLLErrs capture;
        LLSD request;
        request["op"] = "foo";
        std::string threw = capture.catch_llerrs([&request](){
                LLEventPumps::instance().obtain("LLAres").post(request);
            
}

TEST_CASE_FIXTURE(data, "test_3")
{

        set_test_name("bad rewriteURI request");
        WrapLLErrs capture;
        LLSD request;
        request["op"] = "rewriteURI";
        std::string threw = capture.catch_llerrs([&request](){
                LLEventPumps::instance().obtain("LLAres").post(request);
            
}

TEST_CASE_FIXTURE(data, "test_4")
{

        set_test_name("bad rewriteURI request");
        WrapLLErrs capture;
        LLSD request;
        request["op"] = "rewriteURI";
        request["reply"] = "nonexistent";
        std::string threw = capture.catch_llerrs([&request](){
                LLEventPumps::instance().obtain("LLAres").post(request);
            
}

TEST_CASE_FIXTURE(data, "test_5")
{

        set_test_name("bad rewriteURI request");
        WrapLLErrs capture;
        LLSD request;
        request["op"] = "rewriteURI";
        request["uri"] = "foo.bar.com";
        std::string threw = capture.catch_llerrs([&request](){
                LLEventPumps::instance().obtain("LLAres").post(request);
            
}

} // TEST_SUITE

