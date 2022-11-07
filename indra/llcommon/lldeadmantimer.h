/** 
* @file   lldeadmantimer.h
* @brief  Interface to a simple event timer with a deadman's switch
* @author monty@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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

#ifndef LL_DEADMANTIMER_H
#define LL_DEADMANTIMER_H


#include "linden_common.h"

#include "lltimer.h"
#include "llprocinfo.h"


/// @file lldeadmantimer.h
///
/// There are interesting user-experienced events in the viewer that
/// would seem to have well-defined start and stop points but which
/// actually lack such milestones in the code.  Such events (like
/// time to load meshes after logging in, initial inventory load,
/// display name fetch) can be defined somewhat after-the-fact by
/// noticing when we no longer perform operations towards their
/// completion.  This class is intended to help in such applications.
///
/// What it implements is a deadman's switch (also known as a
/// keepalive switch and a doorbell switch).  The basic operation is
/// as follows:
///
/// * LLDeadmanTimer is instantiated with a horizon value in seconds,
///   one for each event of interest.
/// * When an event starts, @see start() is invoked to begin a
///   timing operation.
/// * As operations are performed in service of the event (issuing
///   HTTP requests, receiving responses), @see ringBell() is invoked
///   to inform the timer that the operation is still active.
/// * If the operation is canceled or otherwise terminated, @see
///   stop() can be called to end the timing operation.
/// * Concurrent with the ringBell() calls, the program makes
///   periodic (shorter than the horizon but not too short) calls
///   to @see isExpired() to see if the event has expired due to
///   either a stop() call or lack of activity (defined as a ringBell()
///   call in the previous 'horizon' seconds).  If it has expired,
///   the caller also receives start, stop and count values for the
///   event which the application can then report in whatever manner
///   it sees fit.
/// * The timer becomes passive after an isExpired() call that returns
///   true.  It can then be restarted with a new start() call.
///
/// Threading:  Instances are not thread-safe.  They also use
/// timing code from lltimer.h which is also unsafe.
///
/// Allocation:  Not refcounted, may be stack or heap allocated.
///

class LL_COMMON_API LLDeadmanTimer
{
public:
    /// Public types

    /// Low-level time type chosen for compatibility with
    /// LLTimer::getCurrentClockCount() which is the basis
    /// of time operations in this class.  This is likely
    /// to change in a future version in a move to TSC-based
    /// timing.
    typedef U64 time_type;
    
public:
    /// Construct and initialize an LLDeadmanTimer
    ///
    /// @param horizon  Time, in seconds, after the last @see ringBell()
    ///                 call at which point the timer will consider itself
    ///                 expired.
    ///
    /// @param inc_cpu  If true, gather system and user cpu stats while
    ///                 running the timer.  This does require more syscalls
    ///                 during updates.  If false, cpu usage data isn't
    ///                 collected and will be zero if queried.
    LLDeadmanTimer(F64 horizon, bool inc_cpu);

    ~LLDeadmanTimer() 
        {}
    
private:
    LLDeadmanTimer(const LLDeadmanTimer &);             // Not defined
    void operator=(const LLDeadmanTimer &);             // Not defined

public:
    /// Get the current time.  Zero-basis for this time
    /// representation is not defined and is different on
    /// different platforms.  Do not attempt to compute
    /// negative times relative to the first value returned,
    /// there may not be enough 'front porch' on the range
    /// to prevent wraparound.
    ///
    /// Note:  Implementation is expected to change in a
    /// future release as well.
    ///
    static time_type getNow();

    /// Begin timing.  If the timer is already active, it is reset
    /// and timing begins now.
    ///
    /// @param now      Current time as returned by @see
    ///                 LLTimer::getCurrentClockCount().  If zero,
    ///                 method will lookup current time.
    ///
    void start(time_type now);

    /// End timing.  Actively declare the end of the event independent
    /// of the deadman's switch operation.  @see isExpired() will return
    /// true and appropriate values will be returned.
    ///
    /// @param now      Current time as returned by @see
    ///                 LLTimer::getCurrentClockCount().  If zero,
    ///                 method will lookup current time.
    ///
    void stop(time_type now);

    /// Declare that something interesting happened.  This has two
    /// effects on an unexpired-timer.  1)  The expiration time
    /// is extended for 'horizon' seconds after the 'now' value.
    /// 2)  An internal counter associated with the event is incremented
    /// by the @ref count parameter.  This count is returned via the
    /// @see isExpired() method.
    ///
    /// @param now      Current time as returned by @see
    ///                 LLTimer::getCurrentClockCount().  If zero,
    ///                 method will lookup current time.
    ///
    /// @param count    Count of events to be associated with
    ///                 this bell ringing.
    ///
    void ringBell(time_type now, unsigned int count);
    
    /// Checks the status of the timer.  If the timer has expired,
    /// also returns various timer-related stats.  Unlike ringBell(),
    /// does not extend the horizon, it only checks for expiration.
    ///
    /// @param now      Current time as returned by @see
    ///                 LLTimer::getCurrentClockCount().  If zero,
    ///                 method will lookup current time.
    ///
    /// @param started  If expired, the starting time of the event is
    ///                 returned to the caller via this reference.
    ///
    /// @param stopped  If expired, the ending time of the event is
    ///                 returned to the caller via this reference.
    ///                 Ending time will be that provided in the
    ///                 stop() method or the last ringBell() call
    ///                 leading to expiration, whichever (stop() call
    ///                 or notice of expiration) happened first.
    ///
    /// @param count    If expired, the number of ringBell() calls
    ///                 made prior to expiration.
    ///
    /// @param user_cpu Amount of CPU spent in user mode by the process
    ///                 during the event.  Value in microseconds and will
    ///                 read zero if not enabled by the constructor.
    ///
    /// @param sys_cpu  Amount of CPU spent in system mode by the process.
    ///
    /// @return         true if the timer has expired, false otherwise.
    ///                 If true, it also returns the started,
    ///                 stopped and count values otherwise these are
    ///                 left unchanged.
    ///
    bool isExpired(time_type now, F64 & started, F64 & stopped, U64 & count,
                   U64 & user_cpu, U64 & sys_cpu);

    /// Identical to the six-arugment form except it does without the
    /// CPU time return if the caller isn't interested in it.
    bool isExpired(time_type now, F64 & started, F64 & stopped, U64 & count);

protected:
    time_type                   mHorizon;
    bool                        mActive;
    bool                        mDone;
    time_type                   mStarted;
    time_type                   mExpires;
    time_type                   mStopped;
    time_type                   mCount;

    const bool                  mIncCPU;        // Include CPU metrics in timer
    LLProcInfo::time_type       mUStartCPU;
    LLProcInfo::time_type       mUEndCPU;
    LLProcInfo::time_type       mSStartCPU;
    LLProcInfo::time_type       mSEndCPU;
};
    

#endif  // LL_DEADMANTIMER_H
