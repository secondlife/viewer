/**
 * @file   llsdmessage_tut.cpp
 * @author Nat Goodspeed
 * @date   2008-12-22
 * @brief  Test of llsdmessage.h
 * 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * Copyright (c) 2008, Linden Research, Inc.
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
// external library headers
// other Linden headers
#include "../test/lltut.h"
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
            LLSDMessage::link();
        }
    };
    typedef test_group<llsdmessage_data> llsdmessage_group;
    typedef llsdmessage_group::object llsdmessage_object;
    llsdmessage_group llsdmgr("llsdmessage");

    template<> template<>
    void llsdmessage_object::test<1>()
    {
        bool threw = false;
        // This should fail...
        try
        {
            LLSDMessage localListener;
        }
        catch (const LLEventPump::DupPumpName&)
        {
            threw = true;
        }
        ensure("second LLSDMessage should throw", threw);
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
        ensure_equals(result.asString(), "success");

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
