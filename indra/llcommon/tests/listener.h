/**
 * @file   listener.h
 * @author Nat Goodspeed
 * @date   2009-03-06
 * @brief  Useful for tests of the LLEventPump family of classes
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LISTENER_H)
#define LL_LISTENER_H

#include "llsd.h"
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
    typedef std::vector<std::string> StringList;
    StringList result;
};

std::ostream& operator<<(std::ostream& out, const Collect::StringList& strings)
{
    out << '(';
    Collect::StringList::const_iterator begin(strings.begin()), end(strings.end());
    if (begin != end)
    {
        out << '"' << *begin << '"';
        while (++begin != end)
        {
            out << ", \"" << *begin << '"';
        }
    }
    out << ')';
    return out;
}

#endif /* ! defined(LL_LISTENER_H) */
