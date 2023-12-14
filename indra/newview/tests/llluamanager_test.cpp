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
#include <vector>
// external library headers
// other Linden headers
#include "../llcommon/tests/StringVec.h"
#include "../test/lltut.h"
#include "llapp.h"
#include "lldate.h"
#include "llevents.h"
#include "lleventcoro.h"
#include "llsdutil.h"
#include "lluri.h"
#include "lluuid.h"
#include "stringize.h"

class LLTestApp : public LLApp
{
public:
    bool init()    override { return true; }
    bool cleanup() override { return true; }
    bool frame()   override { return true; }
};

template <typename CALLABLE>
auto listener(CALLABLE&& callable)
{
    return [callable=std::forward<CALLABLE>(callable)]
    (const LLSD& data)
    {
        callable(data);
        return false;
    };
}

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
                             listener([&posts](const LLSD& data)
                             { posts.push_back(data.asString()); })));
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

    void from_lua(const std::string& desc, const std::string_view& construct, const LLSD& expect)
    {
        LLSD fromlua;
        LLEventStream replypump("testpump");
        LLTempBoundListener conn(
            replypump.listen("llluamanager_test",
                             listener([&fromlua](const LLSD& data){ fromlua = data; })));
        const std::string lua(stringize(
            "-- test LLSD synthesized by Lua\n",
            // we expect the caller's Lua snippet to construct a Lua object
            // called 'data'
            construct, "\n"
            "post_on('testpump', data)\n"
        ));
        LLLUAmanager::runScriptLine(lua);
        // At this point LLLUAmanager::runScriptLine() has launched a new C++
        // coroutine to run the passed Lua snippet, but that coroutine hasn't
        // yet had a chance to run. Poke the coroutine scheduler until the Lua
        // script has sent its data.
        for (int i = 0; i < 10 && fromlua.isUndefined(); ++i)
        {
            llcoro::suspend();
        }
        // We woke up again ourselves because the coroutine running Lua has
        // finished.
        ensure_equals(desc, fromlua, expect);
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("LLSD from Lua");
        from_lua("nil", "data = nil", LLSD());
        from_lua("true", "data = true", true);
        from_lua("false", "data = false", false);
        from_lua("int", "data = 17", 17);
        from_lua("real", "data = 3.14", 3.14);
        from_lua("string", "data = 'string'", "string");
        // can't synthesize Lua userdata in Lua code: that can only be
        // constructed by a C function
        from_lua("empty table", "data = {}", LLSD());
        from_lua("nested empty table", "data = { 1, 2, 3, {}, 5 }",
                 llsd::array(1, 2, 3, LLSD(), 5));
        from_lua("nested non-empty table", "data = { 1, 2, 3, {a=0, b=1}, 5 }",
                 llsd::array(1, 2, 3, llsd::map("a", 0, "b", 1), 5));
    }

    void round_trip(const std::string& desc, const LLSD& send, const LLSD& expect)
    {
        LLSD reply;
        LLEventStream replypump("testpump");
        LLTempBoundListener conn(
            replypump.listen("llluamanager_test",
                             listener([&reply](const LLSD& post){ reply = post; })));
        const std::string lua(
            "-- test LLSD round trip\n"
            "callback = function(pump, data)\n"
            "    -- just echo the data we received\n"
            "    post_on('testpump', data)\n"
            "end\n"
            "replypump, cmdpump = listen_events(callback)\n"
            "post_on('testpump', replypump)\n"
            "await_event(replypump)\n"
        );
        LLLUAmanager::runScriptLine(lua);
        // At this point LLLUAmanager::runScriptLine() has launched a new C++
        // coroutine to run the passed Lua snippet, but that coroutine hasn't
        // yet had a chance to run. Poke the coroutine scheduler until the Lua
        // script has sent its reply pump name.
        for (int i = 0; i < 10 && reply.isUndefined(); ++i)
        {
            llcoro::suspend();
        }
        // We woke up again ourselves because the coroutine running Lua has
        // reached the await_event() call, which suspends the calling C++
        // coroutine (including the Lua code running on it) until we post
        // something to that reply pump.
        auto luapump{ reply.asString() };
        reply.clear();
        LLEventPumps::instance().post(luapump, send);
        // The C++ coroutine running the Lua script is now ready to run. Run
        // it so it will echo the LLSD back to us.
        llcoro::suspend();
        ensure_equals(desc, reply, expect);
    }

    // Define an RTItem to be used for round-trip LLSD testing: what it is,
    // what we send to Lua, what we expect to get back. They could be the
    // same.
    struct RTItem
    {
        RTItem(const std::string& name, const LLSD& send, const LLSD& expect):
            mName(name),
            mSend(send),
            mExpect(expect)
        {}
        RTItem(const std::string& name, const LLSD& both):
            mName(name),
            mSend(both),
            mExpect(both)
        {}

        std::string mName;
        LLSD mSend, mExpect;
    };

    template<> template<>
    void object::test<3>()
    {
        set_test_name("LLSD round trip");
        LLSD::Binary binary{ 3, 1, 4, 1, 5, 9, 2, 6, 5 };
        const char* uuid{ "01234567-abcd-0123-4567-0123456789ab" };
        const char* date{ "2023-10-04T21:06:00Z" };
        const char* uri{ "https://secondlife.com/index.html" };
        std::vector<RTItem> items{
            RTItem("undefined", LLSD()),
            RTItem("true", true),
            RTItem("false", false),
            RTItem("int", 17),
            RTItem("real", 3.14),
            RTItem("int real", 27.0, 27),
            RTItem("string", "string"),
            RTItem("binary", binary),
            RTItem("empty array", LLSD::emptyArray(), LLSD()),
            RTItem("empty map", LLSD::emptyMap(), LLSD()),
            RTItem("UUID", LLUUID(uuid), uuid),
            RTItem("date", LLDate(date), date),
            RTItem("uri", LLURI(uri), uri)
            };
        // scalars
        for (const auto& item: items)
        {
            round_trip(item.mName, item.mSend, item.mExpect);
        }

        // array
        LLSD send_array{ LLSD::emptyArray() }, expect_array{ LLSD::emptyArray() };
        for (const auto& item: items)
        {
            send_array.append(item.mSend);
            expect_array.append(item.mExpect);
        }
        // exercise the array tail trimming below
        send_array.append(items[0].mSend);
        expect_array.append(items[0].mExpect);
        // Lua takes a table value of nil to mean: don't store this key. An
        // LLSD array containing undefined entries (converted to nil) leaves
        // "holes" in the Lua table. These will be converted back to undefined
        // LLSD entries -- except at the end. Trailing undefined entries are
        // simply omitted from the table -- so the table converts back to a
        // shorter LLSD array. We've constructed send_array and expect_array
        // according to 'items' above -- but truncate from expect_array any
        // trailing entries whose mSend will map to Lua nil.
        while (expect_array.size() > 0 &&
               send_array[expect_array.size() - 1].isUndefined())
        {
            expect_array.erase(expect_array.size() - 1);
        }
        round_trip("array", send_array, expect_array);

        // map
        LLSD send_map{ LLSD::emptyMap() }, expect_map{ LLSD::emptyMap() };
        for (const auto& item: items)
        {
            send_map[item.mName] = item.mSend;
            // see comment in the expect_array truncation loop above --
            // Lua never stores table entries with nil values
            if (item.mSend.isDefined())
            {
                expect_map[item.mName] = item.mExpect;
            }
        }
        round_trip("map", send_map, expect_map);

        // deeply nested map: exceed Lua's default stack space (20),
        // i.e. verify that we have the right checkstack() calls
        for (int i = 0; i < 20; ++i)
        {
            LLSD new_send_map{ send_map }, new_expect_map{ expect_map };
            new_send_map["nested map"] = send_map;
            new_expect_map["nested map"] = expect_map;
            send_map = new_send_map;
            expect_map = new_expect_map;
        }
        round_trip("nested map", send_map, expect_map);
    }
} // namespace tut
