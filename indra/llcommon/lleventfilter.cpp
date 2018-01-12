/**
 * @file   lleventfilter.cpp
 * @author Nat Goodspeed
 * @date   2009-03-05
 * @brief  Implementation for lleventfilter.
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

/*****************************************************************************
*   LLEventFilter
*****************************************************************************/
LLEventFilter::LLEventFilter(LLEventPump& source, const std::string& name, bool tweak):
    LLEventStream(name, tweak),
    mSource(source.listen(getName(), boost::bind(&LLEventFilter::post, this, _1)))
{
}

/*****************************************************************************
*   LLEventMatching
*****************************************************************************/
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

/*****************************************************************************
*   LLEventTimeoutBase
*****************************************************************************/
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

bool LLEventTimeoutBase::running() const
{
    return mMainloop.connected();
}

/*****************************************************************************
*   LLEventTimeout
*****************************************************************************/
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

/*****************************************************************************
*   LLEventBatch
*****************************************************************************/
LLEventBatch::LLEventBatch(std::size_t size):
    LLEventFilter("batch"),
    mBatchSize(size)
{}

LLEventBatch::LLEventBatch(LLEventPump& source, std::size_t size):
    LLEventFilter(source, "batch"),
    mBatchSize(size)
{}

void LLEventBatch::flush()
{
    // copy and clear mBatch BEFORE posting to avoid weird circularity effects
    LLSD batch(mBatch);
    mBatch.clear();
    LLEventStream::post(batch);
}

bool LLEventBatch::post(const LLSD& event)
{
    mBatch.append(event);
    // calling setSize(same) performs the very check we want
    setSize(mBatchSize);
    return false;
}

void LLEventBatch::setSize(std::size_t size)
{
    mBatchSize = size;
    // changing the size might mean that we have to flush NOW
    if (mBatch.size() >= mBatchSize)
    {
        flush();
    }
}

/*****************************************************************************
*   LLEventThrottleBase
*****************************************************************************/
LLEventThrottleBase::LLEventThrottleBase(F32 interval):
    LLEventFilter("throttle"),
    mInterval(interval),
    mPosts(0)
{}

LLEventThrottleBase::LLEventThrottleBase(LLEventPump& source, F32 interval):
    LLEventFilter(source, "throttle"),
    mInterval(interval),
    mPosts(0)
{}

void LLEventThrottleBase::flush()
{
    // flush() is a no-op unless there's something pending.
    // Don't test mPending because there's no requirement that the consumer
    // post() anything but an isUndefined(). This is what mPosts is for.
    if (mPosts)
    {
        mPosts = 0;
        alarmCancel();
        // This is not to set our alarm; we are not yet requesting
        // any notification. This is just to track whether subsequent post()
        // calls fall within this mInterval or not.
        timerSet(mInterval);
        // copy and clear mPending BEFORE posting to avoid weird circularity
        // effects
        LLSD pending = mPending;
        mPending.clear();
        LLEventStream::post(pending);
    }
}

LLSD LLEventThrottleBase::pending() const
{
    return mPending;
}

bool LLEventThrottleBase::post(const LLSD& event)
{
    // Always capture most recent post() event data. If caller wants to
    // aggregate multiple events, let them retrieve pending() and modify
    // before calling post().
    mPending = event;
    // Always increment mPosts. Unless we count this call, flush() does
    // nothing.
    ++mPosts;
    // We reset mTimer on every flush() call to let us know if we're still
    // within the same mInterval. So -- are we?
    F32 timeRemaining = timerGetRemaining();
    if (! timeRemaining)
    {
        // more than enough time has elapsed, immediately flush()
        flush();
    }
    else
    {
        // still within mInterval of the last flush() call: have to defer
        if (! alarmRunning())
        {
            // timeRemaining tells us how much longer it will be until
            // mInterval seconds since the last flush() call. At that time,
            // flush() deferred events.
            alarmActionAfter(timeRemaining, boost::bind(&LLEventThrottleBase::flush, this));
        }
    }
    return false;
}

void LLEventThrottleBase::setInterval(F32 interval)
{
    F32 oldInterval = mInterval;
    mInterval = interval;
    // If we are not now within oldInterval of the last flush(), we're done:
    // this will only affect behavior starting with the next flush().
    F32 timeRemaining = timerGetRemaining();
    if (timeRemaining)
    {
        // We are currently within oldInterval of the last flush(). Figure out
        // how much time remains until (the new) mInterval of the last
        // flush(). Bt we don't actually store a timestamp for the last
        // flush(); it's implicit. There are timeRemaining seconds until what
        // used to be the end of the interval. Move that endpoint by the
        // difference between the new interval and the old.
        timeRemaining += (mInterval - oldInterval);
        // If we're called with a larger interval, the difference is positive
        // and timeRemaining increases.
        // If we're called with a smaller interval, the difference is negative
        // and timeRemaining decreases. The interesting case is when it goes
        // nonpositive: when the new interval means we can flush immediately.
        if (timeRemaining <= 0.0f)
        {
            flush();
        }
        else
        {
            // immediately reset mTimer
            timerSet(timeRemaining);
            // and if mAlarm is running, reset that too
            if (alarmRunning())
            {
                alarmActionAfter(timeRemaining, boost::bind(&LLEventThrottleBase::flush, this));
            }
        }
    }
}

F32 LLEventThrottleBase::getDelay() const
{
    return timerGetRemaining();
}

/*****************************************************************************
*   LLEventThrottle implementation
*****************************************************************************/
LLEventThrottle::LLEventThrottle(F32 interval):
    LLEventThrottleBase(interval)
{}

LLEventThrottle::LLEventThrottle(LLEventPump& source, F32 interval):
    LLEventThrottleBase(source, interval)
{}

void LLEventThrottle::alarmActionAfter(F32 interval, const LLEventTimeoutBase::Action& action)
{
    mAlarm.actionAfter(interval, action);
}

bool LLEventThrottle::alarmRunning() const
{
    return mAlarm.running();
}

void LLEventThrottle::alarmCancel()
{
    return mAlarm.cancel();
}

void LLEventThrottle::timerSet(F32 interval)
{
    mTimer.setTimerExpirySec(interval);
}

F32  LLEventThrottle::timerGetRemaining() const
{
    return mTimer.getRemainingTimeF32();
}

/*****************************************************************************
*   LLEventBatchThrottle
*****************************************************************************/
LLEventBatchThrottle::LLEventBatchThrottle(F32 interval, std::size_t size):
    LLEventThrottle(interval),
    mBatchSize(size)
{}

LLEventBatchThrottle::LLEventBatchThrottle(LLEventPump& source, F32 interval, std::size_t size):
    LLEventThrottle(source, interval),
    mBatchSize(size)
{}

bool LLEventBatchThrottle::post(const LLSD& event)
{
    // simply retrieve pending value and append the new event to it
    LLSD partial = pending();
    partial.append(event);
    bool ret = LLEventThrottle::post(partial);
    // The post() call above MIGHT have called flush() already. If it did,
    // then pending() was reset to empty. If it did not, though, but the batch
    // size has grown to the limit, flush() anyway. If there's a limit at all,
    // of course. Calling setSize(same) performs the very check we want.
    setSize(mBatchSize);
    return ret;
}

void LLEventBatchThrottle::setSize(std::size_t size)
{
    mBatchSize = size;
    // Changing the size might mean that we have to flush NOW. Don't forget
    // that 0 means unlimited.
    if (mBatchSize && pending().size() >= mBatchSize)
    {
        flush();
    }
}
