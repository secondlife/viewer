/**
 * @file   llluamanager_test.cpp
 * @author Nat Goodspeed
 * @date   2023-09-28
 * @brief  Test for llluamanager.
 * 
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Copyright (c) 2023, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
//#include "llviewerprecompiledheaders.h"
// associated header
#include "../newview/llluamanager.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"
#include "llapp.h"
#include "llevents.h"
#include "lleventcoro.h"
#include "stringize.h"
#include "../llcommon/tests/StringVec.h"

class LLTestApp : public LLApp
{
public:
    bool init()    override { return true; }
    bool cleanup() override { return true; }
    bool frame()   override { return true; }
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llluamanager_data
    {
        // We need an LLApp instance because LLLUAmanager uses coroutines,
        // which suspend, and when a coroutine suspends it checks LLApp state,
        // and if it's not APP_STATUS_RUNNING the coroutine terminates.
        LLTestApp mApp;
    };
    typedef test_group<llluamanager_data> llluamanager_group;
    typedef llluamanager_group::object object;
    llluamanager_group llluamanagergrp("llluamanager");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("test post_on(), listen_events(), await_event()");
        StringVec posts;
        LLEventStream replypump("testpump");
        LLTempBoundListener conn(
            replypump.listen("test<1>",
                             [&posts](const LLSD& post)
                             {
                                 posts.push_back(post.asString());
                                 return false;
                             }));
        const std::string lua(
            "-- test post_on,listen_events,await_event\n"
            "post_on('testpump', 'entry')\n"
            "callback = function(pump, data)\n"
            "    -- just echo the data we received\n"
            "    post_on('testpump', data)\n"
            "end\n"
            "post_on('testpump', 'listen_events()')\n"
            "replypump, cmdpump = listen_events(callback)\n"
            "post_on('testpump', replypump)\n"
            "post_on('testpump', 'await_event()')\n"
            "await_event(replypump)\n"
            "post_on('testpump', 'exit')\n"
        );
        LLLUAmanager::runScriptLine(lua);
        StringVec expected{
            "entry",
            "listen_events()",
            "",
            "await_event()",
            "message",
            "exit"
        };
        for (int i = 0; i < 10 && posts.size() <= 2 && posts[2].empty(); ++i)
        {
            llcoro::suspend();
        }
        expected[2] = posts.at(2);
        LL_DEBUGS() << "Found pumpname '" << expected[2] << "'" << LL_ENDL;
        LLEventPump& luapump{ LLEventPumps::instance().obtain(expected[2]) };
        LL_DEBUGS() << "Found pump '" << luapump.getName() << "', type '"
                    << LLError::Log::classname(luapump)
                    << "': post('" << expected[4] << "')" << LL_ENDL;
        luapump.post(expected[4]);
        llcoro::suspend();
        ensure_equals("post_on() sequence", posts, expected);
    }
} // namespace tut
