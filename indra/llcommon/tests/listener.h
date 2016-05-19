/**
 * @file   listener.h
 * @author Nat Goodspeed
 * @date   2009-03-06
 * @brief  Useful for tests of the LLEventPump family of classes
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

#if ! defined(LL_LISTENER_H)
#define LL_LISTENER_H

#include "llsd.h"
#include "llevents.h"
#include "tests/StringVec.h"
#include <iostream>

/*****************************************************************************
*   test listener class
*****************************************************************************/
class Listener;
std::ostream& operator<<(std::ostream&, const Listener&);

/// Bear in mind that this is strictly for testing
class Listener
{
public:
    /// Every Listener is instantiated with a name
    Listener(const std::string& name):
        mName(name)
    {
//      std::cout << *this << ": ctor\n";
    }
/*==========================================================================*|
    // These methods are only useful when trying to track Listener instance
    // lifespan
    Listener(const Listener& that):
        mName(that.mName),
        mLastEvent(that.mLastEvent)
    {
        std::cout << *this << ": copy\n";
    }
    virtual ~Listener()
    {
        std::cout << *this << ": dtor\n";
    }
|*==========================================================================*/
    /// You can request the name
    std::string getName() const { return mName; }
    /// This is a typical listener method that returns 'false' when done,
    /// allowing subsequent listeners on the LLEventPump to process the
    /// incoming event.
    bool call(const LLSD& event)
    {
//      std::cout << *this << "::call(" << event << ")\n";
        mLastEvent = event;
        return false;
    }
    /// This is an alternate listener that returns 'true' when done, which
    /// stops processing of the incoming event.
    bool callstop(const LLSD& event)
    {
//      std::cout << *this << "::callstop(" << event << ")\n";
        mLastEvent = event;
        return true;
    }
    /// ListenMethod can represent either call() or callstop().
    typedef bool (Listener::*ListenMethod)(const LLSD&);
    /**
     * This helper method is only because our test code makes so many
     * repetitive listen() calls to ListenerMethods. In real code, you should
     * call LLEventPump::listen() directly so it can examine the specific
     * object you pass to boost::bind().
     */
    LLBoundListener listenTo(LLEventPump& pump,
                             ListenMethod method=&Listener::call,
                             const LLEventPump::NameList& after=LLEventPump::empty,
                             const LLEventPump::NameList& before=LLEventPump::empty)
    {
        return pump.listen(getName(), boost::bind(method, this, _1), after, before);
    }
    /// Both call() and callstop() set mLastEvent. Retrieve it.
    LLSD getLastEvent() const
    {
//      std::cout << *this << "::getLastEvent() -> " << mLastEvent << "\n";
        return mLastEvent;
    }
    /// Reset mLastEvent to a known state.
    void reset(const LLSD& to = LLSD())
    {
//      std::cout << *this << "::reset(" << to << ")\n";
        mLastEvent = to;
    }

private:
    std::string mName;
    LLSD mLastEvent;
};

std::ostream& operator<<(std::ostream& out, const Listener& listener)
{
    out << "Listener(" << listener.getName() /* << "@" << &listener */ << ')';
    return out;
}

/**
 * This class tests the relative order in which various listeners on a given
 * LLEventPump are called. Each listen() call binds a particular string, which
 * we collect for later examination. The actual event is ignored.
 */
struct Collect
{
    bool add(const std::string& bound, const LLSD& event)
    {
        result.push_back(bound);
        return false;
    }
    void clear() { result.clear(); }
    StringVec result;
};

#endif /* ! defined(LL_LISTENER_H) */
