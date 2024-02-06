/**
 * @file   lualistener.h
 * @author Nat Goodspeed
 * @date   2024-02-06
 * @brief  Define LuaListener class
 * 
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LUALISTENER_H)
#define LL_LUALISTENER_H

#include "llevents.h"
#include "llinstancetracker.h"
#include "lluuid.h"
#include <memory>                   // std::unique_ptr

#ifdef LL_TEST
#include "lleventfilter.h"
#endif

class lua_State;
class LLLeapListener;

/**
 * LuaListener is based on LLLeap. It serves an analogous function.
 *
 * Each LuaListener instance has an int key, generated randomly to
 * inconvenience malicious Lua scripts wanting to mess with others. The idea
 * is that a given lua_State stores in its Registry:
 * - "event.listener": the int key of the corresponding LuaListener, if any
 * - "event.function": the Lua function to be called with incoming events
 * The original thought was that LuaListener would itself store the Lua
 * function -- but surprisingly, there is no C/C++ type in the API that stores
 * a Lua function.
 *
 * (We considered storing in "event.listener" the LuaListener pointer itself
 * as a light userdata, but the problem would be if Lua code overwrote that.
 * We want to prevent any Lua script from crashing the viewer, intentionally
 * or otherwise. Safer to use a key lookup.)
 *
 * Like LLLeap, each LuaListener instance also has an associated
 * LLLeapListener to respond to LLEventPump management commands.
 */
class LuaListener: public LLInstanceTracker<LuaListener, int>
{
    using super = LLInstanceTracker<LuaListener, int>;
public:
    LuaListener(lua_State* L);

    LuaListener(const LuaListener&) = delete;
    LuaListener& operator=(const LuaListener&) = delete;

    ~LuaListener();

    std::string getReplyName() const { return mReplyPump.getName(); }
    std::string getCommandName() const;

private:
    static int getUniqueKey();

    static LLBoundListener connect(lua_State* L, LLEventPump& pump, const std::string& listener);

    static bool call_lua(lua_State* L, const std::string& pump, const LLSD& data);

#ifndef LL_TEST
    LLEventStream mReplyPump{ LLUUID::generateNewID().asString() };
#else
    LLEventLogProxyFor<LLEventStream> mReplyPump{ "luapump", false };
#endif
    LLTempBoundListener mReplyConnection;
    std::unique_ptr<LLLeapListener> mListener;
};

#endif /* ! defined(LL_LUALISTENER_H) */
