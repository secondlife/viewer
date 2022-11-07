/** 
 * @file lleventtimer.h
 * @brief Cross-platform objects for doing timing 
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_EVENTTIMER_H                 
#define LL_EVENTTIMER_H

#include "stdtypes.h"
#include "lldate.h"
#include "llinstancetracker.h"
#include "lltimer.h"

// class for scheduling a function to be called at a given frequency (approximate, inprecise)
class LL_COMMON_API LLEventTimer : public LLInstanceTracker<LLEventTimer>
{
public:

    LLEventTimer(F32 period);   // period is the amount of time between each call to tick() in seconds
    LLEventTimer(const LLDate& time);
    virtual ~LLEventTimer();

    //function to be called at the supplied frequency
    // Normally return FALSE; TRUE will delete the timer after the function returns.
    virtual BOOL tick() = 0;

    static void updateClass();

    /// Schedule recurring calls to generic callable every period seconds.
    /// Returns a pointer; if you delete it, cancels the recurring calls.
    template <typename CALLABLE>
    static LLEventTimer* run_every(F32 period, const CALLABLE& callable);

    /// Schedule a future call to generic callable. Returns a pointer.
    /// CAUTION: The object referenced by that pointer WILL BE DELETED once
    /// the callback has been called! LLEventTimer::getInstance(pointer) (NOT
    /// pointer->getInstance(pointer)!) can be used to test whether the
    /// pointer is still valid. If it is, deleting it will cancel the
    /// callback.
    template <typename CALLABLE>
    static LLEventTimer* run_at(const LLDate& time, const CALLABLE& callable);

    /// Like run_at(), but after a time delta rather than at a timestamp.
    /// Same CAUTION.
    template <typename CALLABLE>
    static LLEventTimer* run_after(F32 interval, const CALLABLE& callable);

protected:
    LLTimer mEventTimer;
    F32 mPeriod;

private:
    template <typename CALLABLE>
    class Generic;
};

template <typename CALLABLE>
class LLEventTimer::Generic: public LLEventTimer
{
public:
    // making TIME generic allows engaging either LLEventTimer constructor
    template <typename TIME>
    Generic(const TIME& time, bool once, const CALLABLE& callable):
        LLEventTimer(time),
        mOnce(once),
        mCallable(callable)
    {}
    BOOL tick() override
    {
        mCallable();
        // true tells updateClass() to delete this instance
        return mOnce;
    }

private:
    bool mOnce;
    CALLABLE mCallable;
};

template <typename CALLABLE>
LLEventTimer* LLEventTimer::run_every(F32 period, const CALLABLE& callable)
{
    // return false to schedule recurring calls
    return new Generic<CALLABLE>(period, false, callable);
}

template <typename CALLABLE>
LLEventTimer* LLEventTimer::run_at(const LLDate& time, const CALLABLE& callable)
{
    // return true for one-shot callback
    return new Generic<CALLABLE>(time, true, callable);
}

template <typename CALLABLE>
LLEventTimer* LLEventTimer::run_after(F32 interval, const CALLABLE& callable)
{
    // one-shot callback after specified interval
    return new Generic<CALLABLE>(interval, true, callable);
}

#endif //LL_EVENTTIMER_H
