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

#include "llevents.h"               // LLTempBoundListener
#include "llsd.h"
#include "llthreadsafequeue.h"
#include <iosfwd>                   // std::ostream
#include <memory>                   // std::unique_ptr
#include <string>
#include <utility>                  // std::pair

struct lua_State;
class LLLeapListener;

/**
 * LuaListener is based on LLLeap. It serves an analogous function.
 *
 * Like LLLeap, each LuaListener instance also has an associated
 * LLLeapListener to respond to LLEventPump management commands.
 */
class LuaListener
{
public:
    LuaListener(lua_State* L);

    LuaListener(const LuaListener&) = delete;
    LuaListener& operator=(const LuaListener&) = delete;

    ~LuaListener();

    std::string getReplyName() const;
    std::string getCommandName() const;

    /**
     * LuaListener enqueues reply events from its LLLeapListener on mQueue.
     * Call getNext() to retrieve the next such event. Blocks the calling
     * coroutine if the queue is empty.
     */
    using PumpData = std::pair<std::string, LLSD>;
    PumpData getNext();

    friend std::ostream& operator<<(std::ostream& out, const LuaListener& self);

private:
    bool queueEvent(const std::string& pump, const LLSD& data);

    LLThreadSafeQueue<PumpData> mQueue;

    std::string mCoroName;
    std::unique_ptr<LLLeapListener> mListener;
    LLTempBoundListener mShutdownConnection;
};

#endif /* ! defined(LL_LUALISTENER_H) */
