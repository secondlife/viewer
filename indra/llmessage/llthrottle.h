/** 
 * @file llthrottle.h
 * @brief LLThrottle class used for network bandwidth control
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
	BOOL checkOverflow(const F32 amount); // I'm about to add an amount, TRUE if would overflow throttle
	BOOL throttleOverflow(const F32 amount); // I just sent amount, TRUE if that overflowed the throttle

	F32 getAvailable();	// Return the available bits
	F32 getRate() const				{ return mRate; }
private:
	F32 mLookaheadSecs;	// Seconds to look ahead, maximum
	F32	mRate;	// BPS available, dynamically adjusted
	F32	mAvailable;	// Bits available to send right now on each channel
	F64	mLastSendTime;		// Time since last send on this channel
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
	BOOL	checkOverflow(S32 throttle_cat, F32 bits);		// I'm about to send bits, TRUE if would overflow channel
	BOOL	throttleOverflow(S32 throttle_cat, F32 bits);	// I just sent bits, TRUE if that overflowed the channel
	BOOL	dynamicAdjust();		// Shift bandwidth from idle channels to busy channels, TRUE if adjustment occurred
	BOOL	setNominalBPS(F32* throttle_vec);				// TRUE if any value was different, resets adjustment system if was different

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

	F64		mLastSendTime[TC_EOF];		// Time since last send on this channel
	F64		mDynamicAdjustTime;	// Only dynamic adjust every 2 seconds or so.

};

#endif
