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

#ifndef LL_LLTHROTTLE_H
#define LL_LLTHROTTLE_H

#include "lltimer.h"

const S32 MAX_THROTTLE_SIZE = 32;

class LLDataPacker;

// Single instance of a generic throttle
class LLThrottle
{
public:
	LLThrottle(const F32 throttle = 1.f);
	~LLThrottle() { }

	void setRate(const F32 rate);
	bool checkOverflow(const F32 amount); // I'm about to add an amount, TRUE if would overflow throttle
	bool throttleOverflow(const F32 amount); // I just sent amount, TRUE if that overflowed the throttle

	F32 getAvailable();	// Return the available bits
	F32 getRate() const				{ return mRate; }
private:
	F32 mLookaheadSecs;	// Seconds to look ahead, maximum
	F32	mRate;	// BPS available, dynamically adjusted
	F32	mAvailable;	// Bits available to send right now on each channel
	F64Seconds	mLastSendTime;		// Time since last send on this channel
};

typedef enum e_throttle_categories
{
	TC_RESEND,
	TC_LAND,
	TC_WIND,
	TC_CLOUD,
	TC_TASK,
	TC_TEXTURE,
	TC_ASSET,
	TC_EOF
} EThrottleCats;


class LLThrottleGroup
{
public:
	LLThrottleGroup();
	~LLThrottleGroup() { }

	void	resetDynamicAdjust();
	bool	checkOverflow(S32 throttle_cat, F32 bits);		// I'm about to send bits, TRUE if would overflow channel
	bool	throttleOverflow(S32 throttle_cat, F32 bits);	// I just sent bits, TRUE if that overflowed the channel
	bool	dynamicAdjust();		// Shift bandwidth from idle channels to busy channels, TRUE if adjustment occurred
	bool	setNominalBPS(F32* throttle_vec);				// TRUE if any value was different, resets adjustment system if was different

	S32		getAvailable(S32 throttle_cat);					// Return bits available in the channel

	void packThrottle(LLDataPacker &dp) const;
	void unpackThrottle(LLDataPacker &dp);
public:
	F32		mThrottleTotal[TC_EOF];	// BPS available, sent by viewer, sum for all simulators

protected:
	F32		mNominalBPS[TC_EOF];	// BPS available, adjusted to be just this simulator
	F32		mCurrentBPS[TC_EOF];	// BPS available, dynamically adjusted

	F32		mBitsAvailable[TC_EOF];	// Bits available to send right now on each channel
	F32		mBitsSentThisPeriod[TC_EOF];	// Sent in this dynamic allocation period
	F32		mBitsSentHistory[TC_EOF];		// Sent before this dynamic allocation period, adjusted to one period length

	F64Seconds	mLastSendTime[TC_EOF];		// Time since last send on this channel
	F64Seconds	mDynamicAdjustTime;	// Only dynamic adjust every 2 seconds or so.

};

#endif
