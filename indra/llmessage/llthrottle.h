/**
 * @file llthrottle.h
 * @brief LLThrottle class used for network bandwidth control
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#pragma once

#include "lltimer.h"

#include <array>

const S32 MAX_THROTTLE_SIZE = 32;

class LLDataPacker;

// Single instance of a generic throttle
class LLThrottle
{
public:
    LLThrottle(const F32 throttle = 1.f);
    ~LLThrottle() = default;

    void setRate(const F32 rate);
    bool checkOverflow(const F32 amount); // I'm about to add an amount, true if would overflow throttle
    bool throttleOverflow(const F32 amount); // I just sent amount, true if that overflowed the throttle

    F32 getAvailable(); // Return the available bits
    F32 getRate() const             { return mRate; }
private:
    F32 mLookaheadSecs; // Seconds to look ahead, maximum
    F32 mRate;  // BPS available, dynamically adjusted
    F32 mAvailable; // Bits available to send right now on each channel
    F64Seconds  mLastSendTime;      // Time since last send on this channel
};

enum EThrottleCats
{
    TC_RESEND,
    TC_LAND,
    TC_WIND,
    TC_CLOUD,
    TC_TASK,
    TC_TEXTURE,
    TC_ASSET,
    TC_EOF
};


class LLThrottleGroup
{
public:
    LLThrottleGroup();
    ~LLThrottleGroup() = default;

    void    resetDynamicAdjust();
    bool    checkOverflow(S32 throttle_cat, F32 bits);      // I'm about to send bits, true if would overflow channel
    bool    throttleOverflow(S32 throttle_cat, F32 bits);   // I just sent bits, true if that overflowed the channel
    bool    dynamicAdjust();        // Shift bandwidth from idle channels to busy channels, true if adjustment occurred
    bool    setNominalBPS(F32* throttle_vec);               // true if any value was different, resets adjustment system if was different

    S32     getAvailable(S32 throttle_cat);                 // Return bits available in the channel

    void packThrottle(LLDataPacker &dp) const;
    void unpackThrottle(LLDataPacker &dp);
public:
    std::array<F32, TC_EOF>     mThrottleTotal; // BPS available, sent by viewer, sum for all simulators

protected:
    std::array<F32, TC_EOF>     mNominalBPS;    // BPS available, adjusted to be just this simulator
    std::array<F32, TC_EOF>     mCurrentBPS;    // BPS available, dynamically adjusted

    std::array<F32, TC_EOF>     mBitsAvailable; // Bits available to send right now on each channel
    std::array<F32, TC_EOF>     mBitsSentThisPeriod;    // Sent in this dynamic allocation period
    std::array<F32, TC_EOF>     mBitsSentHistory;       // Sent before this dynamic allocation period, adjusted to one period length

    std::array<F64Seconds, TC_EOF>  mLastSendTime;      // Time since last send on this channel
    F64Seconds  mDynamicAdjustTime; // Only dynamic adjust every 2 seconds or so.

};

