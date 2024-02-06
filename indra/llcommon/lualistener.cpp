/**
 * @file   lualistener.cpp
 * @author Nat Goodspeed
 * @date   2024-02-06
 * @brief  Implementation for lualistener.
 * 
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lualistener.h"
// STL headers
// std headers
#include <cstdlib>                  // std::rand()
#include <cstring>                  // std::memcpy()
// external library headers
#include "luau/lua.h"
// other Linden headers
#include "llerror.h"
#include "llleaplistener.h"
#include "lua_function.h"

LuaListener::LuaListener(lua_State* L):
    super(getUniqueKey()),
    mListener(
        new LLLeapListener(
            [L](LLEventPump& pump, const std::string& listener)
            { return connect(L, pump, listener); }))
{
    mReplyConnection = connect(L, mReplyPump, "LuaListener");
}

LuaListener::~LuaListener()
{
    LL_DEBUGS("Lua") << "~LuaListener('" << mReplyPump.getName() << "')" << LL_ENDL;
}

int LuaListener::getUniqueKey()
{
    // Find a random key that does NOT already correspond to a LuaListener
    // instance. Passing a duplicate key to LLInstanceTracker would do Bad
    // Things.
    int key;
    do
    {
        key = std::rand();
    } while (LuaListener::getInstance(key));
    // This is theoretically racy, if we were instantiating new
    // LuaListeners on multiple threads. Don't.
    return key;
}

LLBoundListener LuaListener::connect(lua_State* L, LLEventPump& pump, const std::string& listener)
{
    return pump.listen(
        listener,
        [L, pumpname=pump.getName()](const LLSD& data)
        { return call_lua(L, pumpname, data); });
}

bool LuaListener::call_lua(lua_State* L, const std::string& pump, const LLSD& data)
{
    LL_INFOS("Lua") << "LuaListener::call_lua('" << pump << "', " << data << ")" << LL_ENDL;
    if (! lua_checkstack(L, 3))
    {
        LL_WARNS("Lua") << "Cannot extend Lua stack to call listen_events() callback"
                        << LL_ENDL;
        return false;
    }
    // push the registered Lua callback function stored in our registry as
    // "event.function"
    lua_getfield(L, LUA_REGISTRYINDEX, "event.function");
    llassert(lua_isfunction(L, -1));
    // pass pump name
    lua_pushstdstring(L, pump);
    // then the data blob
    lua_pushllsd(L, data);
    // call the registered Lua listener function; allow it to return bool;
    // no message handler
    auto status = lua_pcall(L, 2, 1, 0);
    bool result{ false };
    if (status != LUA_OK)
    {
        LL_WARNS("Lua") << "Error in listen_events() callback: "
                        << lua_tostdstring(L, -1) << LL_ENDL;
    }
    else
    {
        result = lua_toboolean(L, -1);
    }
    // discard either the error message or the bool return value
    lua_pop(L, 1);
    return result;
}

std::string LuaListener::getCommandName() const
{
    return mListener->getPumpName();
}
