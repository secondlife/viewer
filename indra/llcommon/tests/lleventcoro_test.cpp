/**
 * @file   coroutine_test.cpp
 * @author Nat Goodspeed
 * @date   2009-04-22
 * @brief  Test for coroutine.
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

#define BOOST_RESULT_OF_USE_TR1 1
#include <boost/bind.hpp>
#include <boost/range.hpp>
#include <boost/utility.hpp>

#include "linden_common.h"

#include <iostream>
#include <string>
#include <typeinfo>

#include "../test/lldoctest.h"
#include "../test/lltestapp.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llevents.h"
#include "llcoros.h"
#include "lleventfilter.h"
#include "lleventcoro.h"
#include "../test/debug.h"
#include "../test/sync.h"

using namespace llcoro;

/*****************************************************************************
*   Test helpers
*****************************************************************************/
/// Simulate an event API whose response is immediate: sent on receipt of the
/// initial request, rather than after some delay. This is the case that
/// distinguishes postAndSuspend() from calling post(), then calling
/// suspendUntilEventOn().
class ImmediateAPI
{
public:
    ImmediateAPI(Sync& sync):
        mPump("immediate", true),
        mSync(sync)
    {
        mPump.listen("API", boost::bind(&ImmediateAPI::operator(), this, _1));
    }

    LLEventPump& getPump() { return mPump; }

    // Invoke this with an LLSD map containing:
    // ["value"]: Integer value. We will reply with ["value"] + 1.
    // ["reply"]: Name of LLEventPump on which to send response.
    bool operator()(const LLSD& event) const
    {
        mSync.bump();
        LLSD::Integer value(event["value"]);
        LLEventPumps::instance().obtain(event["reply"]).post(value + 1);
        return false;
    }

private:
    LLEventStream mPump;
    Sync& mSync;
};

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct test_data
{

        Sync mSync;
        ImmediateAPI immediateAPI{mSync
};

TEST_CASE_FIXTURE(test_data, "test_1")
{

        set_test_name("explicit_wait");
        DEBUG;

        // Construct the coroutine instance that will run explicit_wait.
        std::shared_ptr<LLCoros::Promise<std::string>> respond;
        LLCoros::instance().launch("test<1>",
                                   [this, &respond](){ explicit_wait(respond); 
}

TEST_CASE_FIXTURE(test_data, "test_2")
{

        set_test_name("waitForEventOn1");
        DEBUG;
        LLCoros::instance().launch("test<2>", [this](){ waitForEventOn1(); 
}

TEST_CASE_FIXTURE(test_data, "test_3")
{

        set_test_name("coroPump");
        DEBUG;
        LLCoros::instance().launch("test<3>", [this](){ coroPump(); 
}

TEST_CASE_FIXTURE(test_data, "test_4")
{

        set_test_name("postAndWait1");
        DEBUG;
        LLCoros::instance().launch("test<4>", [this](){ postAndWait1(); 
}

TEST_CASE_FIXTURE(test_data, "test_5")
{

        set_test_name("coroPumpPost");
        DEBUG;
        LLCoros::instance().launch("test<5>", [this](){ coroPumpPost(); 
}

TEST_CASE_FIXTURE(test_data, "test_6")
{

        set_test_name("LLEventMailDrop");
        tut::test<LLEventMailDrop>();
    
}

TEST_CASE_FIXTURE(test_data, "test_7")
{

        set_test_name("LLEventLogProxyFor<LLEventMailDrop>");
        tut::test< LLEventLogProxyFor<LLEventMailDrop> >();
    
}

} // TEST_SUITE

