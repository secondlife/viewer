/**
 * @file   llsdmessage_test.cpp
 * @author Nat Goodspeed
 * @date   2008-12-22
 * @brief  Test of llsdmessage.h
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

#if LL_WINDOWS
#pragma warning (disable : 4675) // "resolved by ADL" -- just as I want!
#endif

// Precompiled header
#include "linden_common.h"
// associated header
#include "llsdmessage.h"
// STL headers
#include <iostream>
// std headers
#include <stdexcept>
#include <typeinfo>
// external library headers
// other Linden headers
#include "../test/lltut.h"
#include "../test/catch_and_store_what_in.h"
#include "llsdserialize.h"
#include "llevents.h"
#include "stringize.h"
#include "llhost.h"
#include "tests/networkio.h"
#include "tests/commtest.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llsdmessage_data: public commtest_data
    {
        LLEventPump& httpPump;

        llsdmessage_data():
            httpPump(pumps.obtain("LLHTTPClient"))
        {
            LLCurl::initClass();
            LLSDMessage::link();
        }
    };
    typedef test_group<llsdmessage_data> llsdmessage_group;
    typedef llsdmessage_group::object llsdmessage_object;
    llsdmessage_group llsdmgr("llsdmessage");

    template<> template<>
    void llsdmessage_object::test<1>()
    {
        std::string threw;
        // This should fail...
        try
        {
            LLSDMessage localListener;
        }
        CATCH_AND_STORE_WHAT_IN(threw, LLEventPump::DupPumpName)
        ensure("second LLSDMessage should throw", ! threw.empty());
    }

    template<> template<>
    void llsdmessage_object::test<2>()
    {
        LLSD request, body;
        body["data"] = "yes";
        request["payload"] = body;
        request["reply"] = replyPump.getName();
        request["error"] = errorPump.getName();
        bool threw = false;
        try
        {
            httpPump.post(request);
        }
        catch (const LLSDMessage::ArgError&)
        {
            threw = true;
        }
        ensure("missing URL", threw);
    }

    template<> template<>
    void llsdmessage_object::test<3>()
    {
        LLSD request, body;
        body["data"] = "yes";
        request["url"] = server + "got-message";
        request["payload"] = body;
        request["reply"] = replyPump.getName();
        request["error"] = errorPump.getName();
        httpPump.post(request);
        ensure("got response", netio.pump());
        ensure("success response", success);
        ensure_equals(result["reply"].asString(), "success");

        body["status"] = 499;
        body["reason"] = "custom error message";
        request["url"] = server + "fail";
        request["payload"] = body;
        httpPump.post(request);
        ensure("got response", netio.pump());
        ensure("failure response", ! success);
        ensure_equals(result["status"].asInteger(), body["status"].asInteger());
        ensure_equals(result["reason"].asString(),  body["reason"].asString());
    }
} // namespace tut
