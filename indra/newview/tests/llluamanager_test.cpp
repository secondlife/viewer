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
#include "llcontrol.h"
#include "lldate.h"
#include "llevents.h"
#include "lleventcoro.h"
#include "llsdutil.h"
#include "lluri.h"
#include "lluuid.h"
#include "lua_function.h"
#include "lualistener.h"
#include "stringize.h"

class LLTestApp : public LLApp
{
public:
    bool init()    override { return true; }
    bool cleanup() override { return true; }
    bool frame()   override { return true; }
};

LLControlGroup gSavedSettings("Global");

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llluamanager_data
    {
        llluamanager_data()
        {
            // Load gSavedSettings from source tree
            // indra/newview/tests/llluamanager_test.cpp =>
            // indra/newview
            auto newview{ fsyspath(__FILE__).parent_path().parent_path() };
            fsyspath settings{ newview / "app_settings" / "settings.xml" };
            // true suppresses implicit declare; implicit declare requires
            // that every variable in settings.xml has a Comment, which many don't.
            gSavedSettings.loadFromFile(settings, true);
            // At test time, since we don't have the app bundle available,
            // extend LuaRequirePath to include the require directory in the
            // source tree.
            std::string require{ fsyspath(newview / "scripts" / "lua" / "require") };
            auto paths{ gSavedSettings.getLLSD("LuaRequirePath") };
            bool found = false;
            for (const auto& path : llsd::inArray(paths))
            {
                if (path.asString() == require)
                {
                    found = true;
                    break;
                }
            }
            if (! found)
            {
                paths.append(require);
                gSavedSettings.setLLSD("LuaRequirePath", paths);
            }
        }
        // We need an LLApp instance because LLLUAmanager uses coroutines,
        // which suspend, and when a coroutine suspends it checks LLApp state,
        // and if it's not APP_STATUS_RUNNING the coroutine terminates.
        LLTestApp mApp;
    };
    typedef test_group<llluamanager_data> llluamanager_group;
    typedef llluamanager_group::object object;
    llluamanager_group llluamanagergrp("llluamanager");

    static struct LuaExpr
    {
        std::string desc, expr;
        LLSD expect;
    } lua_expressions[] = {
        { "nil", "nil", LLSD() },
        { "true", "true", true },
        { "false", "false", false },
        { "int", "17", 17 },
        { "real", "3.14", 3.14 },
        { "string", "'string'", "string" },
        // can't synthesize Lua userdata in Lua code: that can only be
        // constructed by a C function
        { "empty table", "{}", LLSD() },
        { "nested empty table", "{ 1, 2, 3, {}, 5 }",
                 llsd::array(1, 2, 3, LLSD(), 5) },
        { "nested non-empty table", "{ 1, 2, 3, {a=0, b=1}, 5 }",
                 llsd::array(1, 2, 3, llsd::map("a", 0, "b", 1), 5) },
    };

    template<> template<>
    void object::test<1>()
    {
        set_test_name("test Lua results");
        for (auto& luax : lua_expressions)
        {
            auto [count, result] =
                LLLUAmanager::waitScriptLine("return " + luax.expr);
            auto desc{ stringize("waitScriptLine(", luax.desc, "): ") };
            // if count < 0, report Lua error message
            ensure_equals(desc + result.asString(), count, 1);
            ensure_equals(desc + "result", result, luax.expect);
        }
    }

    void from_lua(const std::string& desc, std::string_view construct, const LLSD& expect)
    {
        LLSD fromlua;
        LLStreamListener pump("testpump",
                              [&fromlua](const LLSD& data){ fromlua = data; });
        const std::string lua(stringize(
            "data = ", construct, "\n"
            "LL.post_on('testpump', data)\n"
        ));
        auto [count, result] = LLLUAmanager::waitScriptLine(lua);
        // We woke up again ourselves because the coroutine running Lua has
        // finished. But our Lua chunk didn't actually return anything, so we
        // expect count to be 0 and result to be undefined.
        ensure_equals(desc + ": " + result.asString(), count, 0);
        ensure_equals(desc, fromlua, expect);
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("LLSD from post_on()");
        for (auto& luax : lua_expressions)
        {
            from_lua(luax.desc, luax.expr, luax.expect);
        }
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("test post_on(), get_event_pumps(), get_event_next()");
        StringVec posts;
        LLStreamListener pump("testpump",
                              [&posts](const LLSD& data)
                              { posts.push_back(data.asString()); });
        const std::string lua(
            "-- test post_on,get_event_pumps,get_event_next\n"
            "LL.post_on('testpump', 'entry')\n"
            "LL.post_on('testpump', 'get_event_pumps()')\n"
            "replypump, cmdpump = LL.get_event_pumps()\n"
            "LL.post_on('testpump', replypump)\n"
            "LL.post_on('testpump', 'get_event_next()')\n"
            "pump, data = LL.get_event_next()\n"
            "LL.post_on('testpump', data)\n"
            "LL.post_on('testpump', 'exit')\n"
        );
        // It's important to let the startScriptLine() coroutine run
        // concurrently with ours until we've had a chance to post() our
        // reply.
        auto future = LLLUAmanager::startScriptLine(lua);
        StringVec expected{
            "entry",
            "get_event_pumps()",
            "",
            "get_event_next()",
            "message",
            "exit"
        };
        expected[2] = posts.at(2);
        LL_DEBUGS() << "Found pumpname '" << expected[2] << "'" << LL_ENDL;
        LLEventPump& luapump{ LLEventPumps::instance().obtain(expected[2]) };
        LL_DEBUGS() << "Found pump '" << luapump.getName() << "', type '"
                    << LLError::Log::classname(luapump)
                    << "': post('" << expected[4] << "')" << LL_ENDL;
        luapump.post(expected[4]);
        auto [count, result] = future.get();
        ensure_equals("post_on(): " + result.asString(), count, 0);
        ensure_equals("post_on() sequence", posts, expected);
    }

    void round_trip(const std::string& desc, const LLSD& send, const LLSD& expect)
    {
        LLEventMailDrop testpump("testpump");
        const std::string lua(
            "-- test LLSD round trip\n"
            "replypump, cmdpump = LL.get_event_pumps()\n"
            "LL.post_on('testpump', replypump)\n"
            "pump, data = LL.get_event_next()\n"
            "return data\n"
        );
        auto future = LLLUAmanager::startScriptLine(lua);
        // We woke up again ourselves because the coroutine running Lua has
        // reached the get_event_next() call, which suspends the calling C++
        // coroutine (including the Lua code running on it) until we post
        // something to that reply pump.
        auto luapump{ llcoro::suspendUntilEventOn(testpump).asString() };
        LLEventPumps::instance().post(luapump, send);
        // The C++ coroutine running the Lua script is now ready to run. Run
        // it so it will echo the LLSD back to us.
        auto [count, result] = future.get();
        ensure_equals(stringize("round_trip(", desc, "): ", result.asString()), count, 1);
        ensure_equals(desc, result, expect);
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
    void object::test<4>()
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
            expect_array.erase(LLSD::Integer(expect_array.size() - 1));
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

    template<> template<>
    void object::test<5>()
    {
        set_test_name("leap.request() from main thread");
        const std::string lua(
            "-- leap.request() from main thread\n"
            "\n"
            "leap = require 'leap'\n"
            "\n"
            "return {\n"
            "    a=leap.request('echo', {data='a'}).data,\n"
            "    b=leap.request('echo', {data='b'}).data\n"
            "}\n"
        );

        LLStreamListener pump(
            "echo",
            [](const LLSD& data)
            {
                LL_DEBUGS("Lua") << "echo pump got: " << data << LL_ENDL;
                sendReply(data, data);
            });

        auto [count, result] = LLLUAmanager::waitScriptLine(lua);
        ensure_equals("Lua script didn't return item", count, 1);
        ensure_equals("echo failed", result, llsd::map("a", "a", "b", "b"));
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("interleave leap.request() responses");
        const std::string lua(
            "-- interleave leap.request() responses\n"
            "\n"
            "fiber = require('fiber')\n"
            "leap = require('leap')\n"
            "local function debug(...) end\n"
            "-- debug = require('printf')\n"
            "\n"
            "-- negative priority ensures catchall is always last\n"
            "catchall = leap.WaitFor(-1, 'catchall')\n"
            "function catchall:filter(pump, data)\n"
            "    debug('catchall:filter(%s, %s)', pump, data)\n"
            "    return data\n"
            "end\n"
            "\n"
            "-- but first, catch events with 'special' key\n"
            "catch_special = leap.WaitFor(2, 'catch_special')\n"
            "function catch_special:filter(pump, data)\n"
            "    debug('catch_special:filter(%s, %s)', pump, data)\n"
            "    return if data['special'] ~= nil then data else nil\n"
            "end\n"
            "\n"
            "function drain(waitfor)\n"
            "    debug('%s start', waitfor.name)\n"
            "    -- It seems as though we ought to be able to code this loop\n"
            "    -- over waitfor:wait() as:\n"
            "    -- for item in waitfor.wait, waitfor do\n"
            "    -- However, that seems to stitch a detour through C code into\n"
            "    -- the coroutine call stack, which prohibits coroutine.yield():\n"
            "    -- 'attempt to yield across metamethod/C-call boundary'\n"
            "    -- So we resort to two different calls to waitfor:wait().\n"
            "    local item = waitfor:wait()\n"
            "    while item do\n"
            "        debug('%s caught %s', waitfor.name, item)\n"
            "        item = waitfor:wait()\n"
            "    end\n"
            "    debug('%s done', waitfor.name)\n"
            "end\n"
            "\n"
            "function requester(name)\n"
            "    debug('requester(%s) start', name)\n"
            "    local response = leap.request('testpump', {name=name})\n"
            "    debug('requester(%s) got %s', name, response)\n"
            "    -- verify that the correct response was dispatched to this coroutine\n"
            "    assert(response.name == name)\n"
            "end\n"
            "\n"
            "-- fiber.print_all()\n"
            "fiber.launch('catchall', drain, catchall)\n"
            "fiber.launch('catch_special', drain, catch_special)\n"
            "fiber.launch('requester(a)', requester, 'a')\n"
            "fiber.launch('requester(b)', requester, 'b')\n"
            // A script can normally count on an implicit fiber.run() call
            // because fiber.lua calls LL.atexit(fiber.run). But atexit()
            // functions are called by ~LuaState(), which (in the code below)
            // won't be called until *after* we expect to interact with the
            // various fibers. So make an explicit call for test purposes.
            "fiber.run()\n"
        );

        LLSD requests;
        LLStreamListener pump(
            "testpump",
            [&requests](const LLSD& data)
            {
                LL_DEBUGS("Lua") << "testpump got: " << data << LL_ENDL;
                requests.append(data);
            });

        auto future = LLLUAmanager::startScriptLine(lua);
        // LuaState::expr() periodically interrupts a running chunk to ensure
        // the rest of our coroutines get cycles. Nonetheless, for this test
        // we have to wait until both requester() coroutines have posted and
        // are waiting for a reply.
        for (unsigned count=0; count < 100; ++count)
        {
            if (requests.size() == 2)
                break;
            llcoro::suspend();
        }
        ensure_equals("didn't get both requests", requests.size(), 2);
        auto replyname{ requests[0]["reply"].asString() };
        auto& replypump{ LLEventPumps::instance().obtain(replyname) };
        // moreover, we expect they arrived in the order they were created
        ensure_equals("a wasn't first",  requests[0]["name"].asString(), "a");
        ensure_equals("b wasn't second", requests[1]["name"].asString(), "b");
        replypump.post(llsd::map("special", "K"));
        // respond to requester(b) FIRST
        replypump.post(requests[1]);
        replypump.post(llsd::map("name", "not special"));
        // now respond to requester(a)
        replypump.post(requests[0]);
        // tell leap we're done
        replypump.post(LLSD());
        auto [count, result] = future.get();
        ensure_equals("leap.lua: " + result.asString(), count, 0);
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("stop hanging Lua script");
        const std::string lua(
            "-- hanging Lua script should terminate\n"
            "\n"
            "LL.get_event_next()\n"
        );
        auto future = LLLUAmanager::startScriptLine(lua);
        // Poke LLTestApp to send its preliminary shutdown message.
        mApp.setQuitting();
        // but now we have to give the startScriptLine() coroutine a chance to run
        auto [count, result] = future.get();
        ensure_equals("killed Lua script terminated normally", count, -1);
        ensure_contains("unexpected killed Lua script error",
                        result.asString(), "viewer is stopping");
    }

    template<> template<>
    void object::test<8>()
    {
        set_test_name("stop looping Lua script");
        const std::string desc("looping Lua script should terminate");
        const std::string lua(
            "-- " + desc + "\n"
            "\n"
            "while true do\n"
            "    x = 1\n"
            "end\n"
        );
        auto [count, result] = LLLUAmanager::waitScriptLine(lua);
        // We expect the above erroneous script has been forcibly terminated
        // because it ran too long without doing any actual work.
        ensure_equals(desc + " count: " + result.asString(), count, -1);
        ensure_contains(desc + " result", result.asString(), "terminated");
    }

    template <typename T>
    struct Visible
    {
        Visible(T name): name(name)
        {
            LL_INFOS() << "Visible<" << LLError::Log::classname<T>() << ">('" << name << "')" << LL_ENDL;
        }
        Visible(const Visible&) = delete;
        Visible& operator=(const Visible&) = delete;
        ~Visible()
        {
            LL_INFOS() << "~Visible<" << LLError::Log::classname<T>() << ">('" << name << "')" << LL_ENDL;
        }
        T name;
    };

    template<> template<>
    void object::test<9>()
    {
        set_test_name("track distinct lua_emplace<T>() types");
        LuaState L;
        lua_emplace<Visible<std::string>>(L, "std::string 0");
        int st0tag = lua_userdatatag(L, -1);
        lua_emplace<Visible<const char*>>(L, "const char* 0");
        int cp0tag = lua_userdatatag(L, -1);
        lua_emplace<Visible<std::string>>(L, "std::string 1");
        int st1tag = lua_userdatatag(L, -1);
        lua_emplace<Visible<const char*>>(L, "const char* 1");
        int cp1tag = lua_userdatatag(L, -1);
        lua_settop(L, 0);
        ensure_equals("lua_emplace<std::string>() tags diverge", st0tag, st1tag);
        ensure_equals("lua_emplace<const char*>() tags diverge", cp0tag, cp1tag);
        ensure_not_equals("lua_emplace<>() tags collide", st0tag, cp0tag);
    }
} // namespace tut
