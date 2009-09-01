/**
 * @file   lleventfilter.cpp
 * @author Nat Goodspeed
 * @date   2009-03-05
 * @brief  Implementation for lleventfilter.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lleventfilter.h"
// STL headers
// std headers
// external library headers
#include <boost/bind.hpp>
// other Linden headers
#include "llerror.h"                // LL_ERRS
#include "llsdutil.h"               // llsd_matches()

LLEventFilter::LLEventFilter(LLEventPump& source, const std::string& name, bool tweak):
    LLEventStream(name, tweak)
{
    source.listen(getName(), boost::bind(&LLEventFilter::post, this, _1));
}

LLEventMatching::LLEventMatching(const LLSD& pattern):
    LLEventFilter("matching"),
    mPattern(pattern)
{
}

LLEventMatching::LLEventMatching(LLEventPump& source, const LLSD& pattern):
    LLEventFilter(source, "matching"),
    mPattern(pattern)
{
}

bool LLEventMatching::post(const LLSD& event)
{
    if (! llsd_matches(mPattern, event).empty())
        return false;

    return LLEventStream::post(event);
}

LLEventTimeoutBase::LLEventTimeoutBase():
    LLEventFilter("timeout")
{
}

LLEventTimeoutBase::LLEventTimeoutBase(LLEventPump& source):
    LLEventFilter(source, "timeout")
{
}

void LLEventTimeoutBase::actionAfter(F32 seconds, const Action& action)
{
    setCountdown(seconds);
    mAction = action;
    if (! mMainloop.connected())
    {
        LLEventPump& mainloop(LLEventPumps::instance().obtain("mainloop"));
        mMainloop = mainloop.listen(getName(), boost::bind(&LLEventTimeoutBase::tick, this, _1));
    }
}

class ErrorAfter
{
public:
    ErrorAfter(const std::string& message): mMessage(message) {}

    void operator()()
    {
        LL_ERRS("LLEventTimeout") << mMessage << LL_ENDL;
    }

private:
    std::string mMessage;
};

void LLEventTimeoutBase::errorAfter(F32 seconds, const std::string& message)
{
    actionAfter(seconds, ErrorAfter(message));
}

class EventAfter
{
public:
    EventAfter(LLEventPump& pump, const LLSD& event):
        mPump(pump),
        mEvent(event)
    {}

    void operator()()
    {
        mPump.post(mEvent);
    }

private:
    LLEventPump& mPump;
    LLSD mEvent;
};

void LLEventTimeoutBase::eventAfter(F32 seconds, const LLSD& event)
{
    actionAfter(seconds, EventAfter(*this, event));
}

bool LLEventTimeoutBase::post(const LLSD& event)
{
    cancel();
    return LLEventStream::post(event);
}

void LLEventTimeoutBase::cancel()
{
    mMainloop.disconnect();
}

bool LLEventTimeoutBase::tick(const LLSD&)
{
    if (countdownElapsed())
    {
        cancel();
        mAction();
    }
    return false;                   // show event to other listeners
}

LLEventTimeout::LLEventTimeout() {}

LLEventTimeout::LLEventTimeout(LLEventPump& source):
    LLEventTimeoutBase(source)
{
}

void LLEventTimeout::setCountdown(F32 seconds)
{
    mTimer.setTimerExpirySec(seconds);
}

bool LLEventTimeout::countdownElapsed() const
{
    return mTimer.hasExpired();
}
