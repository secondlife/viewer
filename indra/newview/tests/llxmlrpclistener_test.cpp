/*
 * @file   llxmlrpclistener_test.cpp
 * @author Nat Goodspeed
 * @date   2009-03-20
 * @brief  Test for llxmlrpclistener.
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

// Precompiled header
#include "../llviewerprecompiledheaders.h"
// associated header
#include "../llxmlrpclistener.h"
// STL headers
#include <iomanip>
// std headers
// external library headers
// other Linden headers
#include "../test/lldoctest.h"
#include "../llxmlrpctransaction.h"
#include "llevents.h"
#include "lleventfilter.h"
#include "llsd.h"
#include "llhost.h"
#include "llcontrol.h"
#include "tests/wrapllerrs.h"
#include "tests/commtest.h"

LLControlGroup gSavedSettings("Global");

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct data
{

        data():
            pumps(LLEventPumps::instance()),
            uri(std::string("http://") +
                LLHost("127.0.0.1", commtest_data::getport("PORT")).getString())
        {
            // These variables are required by machinery used by
            // LLXMLRPCTransaction. The values reflect reality for this test
            // executable; hopefully these values are correct.
            gSavedSettings.declareBOOL("BrowserProxyEnabled", FALSE, "", LLControlVariable::PERSIST_NO); // don't persist
            gSavedSettings.declareBOOL("NoVerifySSLCert", TRUE, "", LLControlVariable::PERSIST_NO); // don't persist
        
};

TEST_CASE_FIXTURE(data, "test_1")
{

        set_test_name("request validation");
        WrapLLErrs capture;
        LLSD request;
        request["uri"] = uri;
        std::string threw = capture.catch_llerrs([&pumps, &request](){
                pumps.obtain("LLXMLRPCTransaction").post(request);
            
}

TEST_CASE_FIXTURE(data, "test_2")
{

        set_test_name("param types validation");
        WrapLLErrs capture;
        LLSD request;
        request["uri"] = uri;
        request["method"] = "hello";
        request["reply"] = "reply";
        LLSD& params(request["params"]);
        params["who"]["specifically"] = "world"; // LLXMLRPCListener only handles scalar params
        std::string threw = capture.catch_llerrs([&pumps, &request](){
                pumps.obtain("LLXMLRPCTransaction").post(request);
            
}

TEST_CASE_FIXTURE(data, "test_3")
{

        set_test_name("success case");
        LLSD request;
        request["uri"] = uri;
        request["method"] = "hello";
        request["reply"] = "reply";
        LLSD& params(request["params"]);
        params["who"] = "world";
        // Set up a timeout filter so we don't spin forever waiting.
        LLEventTimeout watchdog;
        // Connect the timeout filter to the reply pump.
        LLTempBoundListener temp(
            pumps.obtain("reply").
            listen("watchdog", boost::bind(&LLEventTimeout::post, boost::ref(watchdog), _1)));
        // Now connect our target listener to the timeout filter.
        watchdog.listen("captureReply", boost::bind(&data::captureReply, this, _1));
        // Kick off the request...
        reply.clear();
        pumps.obtain("LLXMLRPCTransaction").post(request);
        // Set the timer
        F32 timeout(10);
        watchdog.eventAfter(timeout, LLSD().with("timeout", 0));
        // and pump "mainloop" until we get something, whether from
        // LLXMLRPCListener or from the watchdog filter.
        LLTimer timer;
        F32 start = timer.getElapsedTimeF32();
        LLEventPump& mainloop(pumps.obtain("mainloop"));
        while (reply.isUndefined())
        {
            mainloop.post(LLSD());
        
}

TEST_CASE_FIXTURE(data, "test_4")
{

        set_test_name("bogus method");
        LLSD request;
        request["uri"] = uri;
        request["method"] = "goodbye";
        request["reply"] = "reply";
        LLSD& params(request["params"]);
        params["who"] = "world";
        // Set up a timeout filter so we don't spin forever waiting.
        LLEventTimeout watchdog;
        // Connect the timeout filter to the reply pump.
        LLTempBoundListener temp(
            pumps.obtain("reply").
            listen("watchdog", boost::bind(&LLEventTimeout::post, boost::ref(watchdog), _1)));
        // Now connect our target listener to the timeout filter.
        watchdog.listen("captureReply", boost::bind(&data::captureReply, this, _1));
        // Kick off the request...
        reply.clear();
        pumps.obtain("LLXMLRPCTransaction").post(request);
        // Set the timer
        F32 timeout(10);
        watchdog.eventAfter(timeout, LLSD().with("timeout", 0));
        // and pump "mainloop" until we get something, whether from
        // LLXMLRPCListener or from the watchdog filter.
        LLTimer timer;
        F32 start = timer.getElapsedTimeF32();
        LLEventPump& mainloop(pumps.obtain("mainloop"));
        while (reply.isUndefined())
        {
            mainloop.post(LLSD());
        
}

TEST_CASE_FIXTURE(data, "test_5")
{

        set_test_name("bad type");
        LLSD request;
        request["uri"] = uri;
        request["method"] = "getdict";
        request["reply"] = "reply";
        (void)request["params"];
        // Set up a timeout filter so we don't spin forever waiting.
        LLEventTimeout watchdog;
        // Connect the timeout filter to the reply pump.
        LLTempBoundListener temp(
            pumps.obtain("reply").
            listen("watchdog", boost::bind(&LLEventTimeout::post, boost::ref(watchdog), _1)));
        // Now connect our target listener to the timeout filter.
        watchdog.listen("captureReply", boost::bind(&data::captureReply, this, _1));
        // Kick off the request...
        reply.clear();
        pumps.obtain("LLXMLRPCTransaction").post(request);
        // Set the timer
        F32 timeout(10);
        watchdog.eventAfter(timeout, LLSD().with("timeout", 0));
        // and pump "mainloop" until we get something, whether from
        // LLXMLRPCListener or from the watchdog filter.
        LLTimer timer;
        F32 start = timer.getElapsedTimeF32();
        LLEventPump& mainloop(pumps.obtain("mainloop"));
        while (reply.isUndefined())
        {
            mainloop.post(LLSD());
        
}

} // TEST_SUITE
