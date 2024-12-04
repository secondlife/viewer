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
#include <iomanip>                  // std::quoted()
// external library headers
#include "luau/lua.h"
// other Linden headers
#include "llerror.h"
#include "llleaplistener.h"
#include "lua_function.h"

const int MAX_QSIZE = 1000;

std::ostream& operator<<(std::ostream& out, const LuaListener& self)
{
    return out << "LuaListener(" << std::quoted(self.mCoroName) << ", "
               << self.getReplyName() << ", " << self.getCommandName() << ")";
}

LuaListener::LuaListener(lua_State* L):
    mCoroName(LLCoros::getName()),
    mListener(new LLLeapListener(
        "LuaListener",
        [this](const std::string& pump, const LLSD& data)
        { return queueEvent(pump, data); })),
    // Listen for shutdown events.
    mShutdownConnection(
        LLCoros::getStopListener(
            LLEventPump::ANONYMOUS,
            mCoroName,
            [this](const LLSD&)
            {
                // If a Lua script is still blocked in getNext() during
                // viewer shutdown, close the queue to wake up getNext().
                mQueue.close();
            }))
{
    LL_DEBUGS("Lua") << "LuaListener(" << std::quoted(mCoroName) << ")" << LL_ENDL;
}

LuaListener::~LuaListener()
{
    LL_DEBUGS("Lua") << "~LuaListener(" << std::quoted(mCoroName) << ")" << LL_ENDL;
}

std::string LuaListener::getReplyName() const
{
    return mListener->getReplyPump().getName();
}

std::string LuaListener::getCommandName() const
{
    return mListener->getPumpName();
}

bool LuaListener::queueEvent(const std::string& pump, const LLSD& data)
{
    // Our Lua script might be stalled, or just fail to retrieve events. Don't
    // grow this queue indefinitely. But don't set MAX_QSIZE as the queue
    // capacity or we'd block the post() call trying to propagate this event!
    if (auto size = mQueue.size(); size > MAX_QSIZE)
    {
        LL_WARNS("Lua") << "LuaListener queue for " << mCoroName
                        << " exceeds " << MAX_QSIZE << ": " << size
                        << " -- discarding event" << LL_ENDL;
    }
    else
    {
        mQueue.push(decltype(mQueue)::value_type(pump, data));
    }
    return false;
}

LuaListener::PumpData LuaListener::getNext()
{
    try
    {
        LLCoros::TempStatus status("get_event_next()");
        return mQueue.pop();
    }
    catch (const LLThreadSafeQueueInterrupt&)
    {
        // mQueue has been closed. The only way that happens is when we detect
        // viewer shutdown. Terminate the calling Lua coroutine.
        LLCoros::checkStop();
        return {};
    }
}
