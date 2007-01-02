/** 
 * @file llthrottle.cpp
 * @brief LLThrottle class used for network bandwidth control.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llthrottle.h"
#include "llmath.h"
#include "lldatapacker.h"
#include "message.h"


LLThrottle::LLThrottle(const F32 rate)
{
	mRate = rate;
	mAvailable = 0.f;
	mLookaheadSecs = 0.25f;
	mLastSendTime = LLMessageSystem::getMessageTimeSeconds(TRUE);
}


void LLThrottle::setRate(const F32 rate)
{
	// Need to accumulate available bits when adjusting the rate.
	mAvailable = getAvailable();
	mLastSendTime = LLMessageSystem::getMessageTimeSeconds();
	mRate = rate;
}

F32 LLThrottle::getAvailable()
{
	// use a temporary bits_available
	// since we don't want to change mBitsAvailable every time
	F32 elapsed_time = (F32)(LLMessageSystem::getMessageTimeSeconds() - mLastSendTime);
	return mAvailable + (mRate * elapsed_time);
}

BOOL LLThrottle::checkOverflow(const F32 amount)
{
	BOOL retval = TRUE;

	F32 lookahead_amount = mRate * mLookaheadSecs;

	// use a temporary bits_available
	// since we don't want to change mBitsAvailable every time
	F32 elapsed_time =  (F32)(LLMessageSystem::getMessageTimeSeconds() - mLastSendTime);
	F32 amount_available = mAvailable + (mRate * elapsed_time);

	if ((amount_available >= lookahead_amount) || (amount_available > amount))
	{
		// ...enough space to send this message
		// Also do if > lookahead so we can use if amount > capped amount.
		retval = FALSE;
	}
	
	return retval;
}

BOOL LLThrottle::throttleOverflow(const F32 amount)
{
	F32 elapsed_time;
	F32 lookahead_amount;
	BOOL retval = TRUE;

	lookahead_amount = mRate * mLookaheadSecs;

	F64 mt_sec = LLMessageSystem::getMessageTimeSeconds();
	elapsed_time = (F32)(mt_sec - mLastSendTime);
	mLastSendTime = mt_sec;

	mAvailable += mRate * elapsed_time;

	if (mAvailable >= lookahead_amount)
	{
		// ...channel completely open, so allow send regardless
		// of size.  This allows sends on very low BPS channels.
		mAvailable = lookahead_amount;
		retval = FALSE;
	}
	else if (mAvailable > amount)
	{
		// ...enough space to send this message
		retval = FALSE;
	}

	// We actually already sent the bits.
	mAvailable -= amount;

	// What if bitsavailable goes negative?
	// That's OK, because it means someone is banging on the channel,
	// so we need some time to recover.

	return retval;
}



const F32 THROTTLE_LOOKAHEAD_TIME = 1.f;	// seconds

// Make sure that we don't set above these
// values, even if the client asks to be set
// higher
// Note that these values are replicated on the 
// client side to set max bandwidth throttling there,
// in llviewerthrottle.cpp. These values are the sum
// of the top two tiers of bandwidth there.

F32 gThrottleMaximumBPS[TC_EOF] =
{
	150000.f, // TC_RESEND
	170000.f, // TC_LAND
	34000.f, // TC_WIND
	34000.f, // TC_CLOUD
	446000.f, // TC_TASK
	446000.f, // TC_TEXTURE
	220000.f, // TC_ASSET
};

// Start low until viewer informs us of capability
// Asset and resend get high values, since they
// aren't used JUST by the viewer necessarily.
// This is a HACK and should be dealt with more properly on
// circuit creation.

F32 gThrottleDefaultBPS[TC_EOF] =
{
	100000.f, // TC_RESEND
	4000.f, // TC_LAND
	4000.f, // TC_WIND
	4000.f, // TC_CLOUD
	4000.f, // TC_TASK
	4000.f, // TC_TEXTURE
	100000.f, // TC_ASSET
};

// Don't throttle down lower than this
// This potentially wastes 50 kbps, but usually
// wont.
F32 gThrottleMinimumBPS[TC_EOF] =
{
	10000.f,	// TC_RESEND
	10000.f,	// TC_LAND
	 4000.f,	// TC_WIND
	 4000.f,	// TC_CLOUD
	20000.f,	// TC_TASK
	10000.f,	// TC_TEXTURE
	10000.f,	// TC_ASSET
};

const char* THROTTLE_NAMES[TC_EOF] =
{
	"Resend ",
	"Land   ",
	"Wind   ",
	"Cloud  ",
	"Task   ",
	"Texture",
	"Asset  "
};

LLThrottleGroup::LLThrottleGroup()
{
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		mThrottleTotal[i]	= gThrottleDefaultBPS[i];
		mNominalBPS[i]		= gThrottleDefaultBPS[i];
	}

	resetDynamicAdjust();
}

void LLThrottleGroup::packThrottle(LLDataPacker &dp) const
{
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		dp.packF32(mThrottleTotal[i], "Throttle");
	}
}

void LLThrottleGroup::unpackThrottle(LLDataPacker &dp)
{
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		F32 temp_throttle;
		dp.unpackF32(temp_throttle, "Throttle");
		temp_throttle = llclamp(temp_throttle, 0.f, 2250000.f);
		mThrottleTotal[i] = temp_throttle;
		if(mThrottleTotal[i] > gThrottleMaximumBPS[i])
		{
			mThrottleTotal[i] = gThrottleMaximumBPS[i];
		}
	}
}

// Call this whenever mNominalBPS changes.  Need to reset
// the measurement systems.  In the future, we should look
// into NOT resetting the system.
void LLThrottleGroup::resetDynamicAdjust()
{
	F64 mt_sec = LLMessageSystem::getMessageTimeSeconds();
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		mCurrentBPS[i]		= mNominalBPS[i];
		mBitsAvailable[i]	= mNominalBPS[i] * THROTTLE_LOOKAHEAD_TIME;
		mLastSendTime[i] = mt_sec;
		mBitsSentThisPeriod[i] = 0;
		mBitsSentHistory[i] = 0;
	}
	mDynamicAdjustTime = mt_sec;
}


BOOL LLThrottleGroup::setNominalBPS(F32* throttle_vec)
{
	BOOL changed = FALSE;
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		if (mNominalBPS[i] != throttle_vec[i])
		{
			changed = TRUE;
			mNominalBPS[i] = throttle_vec[i];
		}
	}

	// If we changed the nominal settings, reset the dynamic
	// adjustment subsystem.
	if (changed)
	{
		resetDynamicAdjust();
	}

	return changed;
}


BOOL LLThrottleGroup::checkOverflow(S32 throttle_cat, F32 bits)
{
	BOOL retval = TRUE;

	F32 category_bps = mCurrentBPS[throttle_cat];
	F32 lookahead_bits = category_bps * THROTTLE_LOOKAHEAD_TIME;

	// use a temporary bits_available
	// since we don't want to change mBitsAvailable every time
	F32 elapsed_time = (F32)(LLMessageSystem::getMessageTimeSeconds() - mLastSendTime[throttle_cat]);
	F32 bits_available = mBitsAvailable[throttle_cat] + (category_bps * elapsed_time);

	if (bits_available >= lookahead_bits)
	{
		// ...channel completely open, so allow send regardless
		// of size.  This allows sends on very low BPS channels.
		mBitsAvailable[throttle_cat] = lookahead_bits;
		retval = FALSE;
	}
	else if ( bits_available > bits )
	{
		// ...enough space to send this message
		retval = FALSE;
	}
	
	return retval;
}

BOOL LLThrottleGroup::throttleOverflow(S32 throttle_cat, F32 bits)
{
	F32 elapsed_time;
	F32 category_bps;
	F32 lookahead_bits;
	BOOL retval = TRUE;

	category_bps = mCurrentBPS[throttle_cat];
	lookahead_bits = category_bps * THROTTLE_LOOKAHEAD_TIME;

	F64 mt_sec = LLMessageSystem::getMessageTimeSeconds();
	elapsed_time = (F32)(mt_sec - mLastSendTime[throttle_cat]);
	mLastSendTime[throttle_cat] = mt_sec;
	mBitsAvailable[throttle_cat] += category_bps * elapsed_time;

	if (mBitsAvailable[throttle_cat] >= lookahead_bits)
	{
		// ...channel completely open, so allow send regardless
		// of size.  This allows sends on very low BPS channels.
		mBitsAvailable[throttle_cat] = lookahead_bits;
		retval = FALSE;
	}
	else if ( mBitsAvailable[throttle_cat] > bits )
	{
		// ...enough space to send this message
		retval = FALSE;
	}

	// We actually already sent the bits.
	mBitsAvailable[throttle_cat] -= bits;

	mBitsSentThisPeriod[throttle_cat] += bits;

	// What if bitsavailable goes negative?
	// That's OK, because it means someone is banging on the channel,
	// so we need some time to recover.

	return retval;
}


BOOL LLThrottleGroup::dynamicAdjust()
{
	const F32 DYNAMIC_ADJUST_TIME = 1.0f;		// seconds
	const F32 CURRENT_PERIOD_WEIGHT = .25f;		// how much weight to give to last period while determining BPS utilization
	const F32 BUSY_PERCENT = 0.75f;		// if use more than this fraction of BPS, you are busy
	const F32 IDLE_PERCENT = 0.70f;		// if use less than this fraction, you are "idle"
	const F32 TRANSFER_PERCENT = 0.90f;	// how much unused bandwidth to take away each adjustment
	const F32 RECOVER_PERCENT = 0.25f;	// how much to give back during recovery phase

	S32 i;

	F64 mt_sec = LLMessageSystem::getMessageTimeSeconds();

	// Only dynamically adjust every few seconds
	if ((mt_sec - mDynamicAdjustTime) < DYNAMIC_ADJUST_TIME)
	{
		return FALSE;
	}
	mDynamicAdjustTime = mt_sec;

	S32 total = 0;
	// Update historical information
	for (i = 0; i < TC_EOF; i++)
	{
		if (mBitsSentHistory[i] == 0)
		{
			// first run, just copy current period
			mBitsSentHistory[i] = mBitsSentThisPeriod[i];
		}
		else
		{
			// have some history, so weight accordingly
			mBitsSentHistory[i] = (1.f - CURRENT_PERIOD_WEIGHT) * mBitsSentHistory[i] 
				+ CURRENT_PERIOD_WEIGHT * mBitsSentThisPeriod[i];
		}

		mBitsSentThisPeriod[i] = 0;
		total += llround(mBitsSentHistory[i]);
	}

	// Look for busy channels
	// TODO: Fold into loop above.
	BOOL channels_busy = FALSE;
	F32  busy_nominal_sum = 0;
	BOOL channel_busy[TC_EOF];
	BOOL channel_idle[TC_EOF];
	BOOL channel_over_nominal[TC_EOF];

	for (i = 0; i < TC_EOF; i++)
	{
		// Is this a busy channel?
		if (mBitsSentHistory[i] >= BUSY_PERCENT * DYNAMIC_ADJUST_TIME * mCurrentBPS[i])
		{
			// this channel is busy
			channels_busy = TRUE;
			busy_nominal_sum += mNominalBPS[i];		// use for allocation of pooled idle bandwidth
			channel_busy[i] = TRUE;
		}
		else
		{
			channel_busy[i] = FALSE;
		}

		// Is this an idle channel?
		if ((mBitsSentHistory[i] < IDLE_PERCENT * DYNAMIC_ADJUST_TIME * mCurrentBPS[i]) &&
			(mBitsAvailable[i] > 0))
		{
			channel_idle[i] = TRUE;
		}
		else
		{
			channel_idle[i] = FALSE;
		}

		// Is this an overpumped channel?
		if (mCurrentBPS[i] > mNominalBPS[i])
		{
			channel_over_nominal[i] = TRUE;
		}
		else
		{
			channel_over_nominal[i] = FALSE;
		}

		//if (total)
		//{
		//	llinfos << i << ": B" << channel_busy[i] << " I" << channel_idle[i] << " N" << channel_over_nominal[i];
		//	llcont << " Nom: " << mNominalBPS[i] << " Cur: " << mCurrentBPS[i] << " BS: " << mBitsSentHistory[i] << llendl;
		//}
	}

	if (channels_busy)
	{
		// Some channels are busy.  Let's see if we can get them some bandwidth.
		F32 used_bps;
		F32 avail_bps;
		F32 transfer_bps;

		F32 pool_bps = 0;

		for (i = 0; i < TC_EOF; i++)
		{
			if (channel_idle[i] || channel_over_nominal[i] )
			{
				// Either channel i is idle, or has been overpumped.
				// Therefore it's a candidate to give up some bandwidth.
				// Figure out how much bandwidth it has been using, and how
				// much is available to steal.
				used_bps = mBitsSentHistory[i] / DYNAMIC_ADJUST_TIME;

				// CRO make sure to keep a minimum amount of throttle available
				// CRO NB: channels set to < MINIMUM_BPS will never give up bps, 
				// which is correct I think
				if (used_bps < gThrottleMinimumBPS[i])
				{
					used_bps = gThrottleMinimumBPS[i];
				}

				if (channel_over_nominal[i])
				{
					F32 unused_current = mCurrentBPS[i] - used_bps;
					avail_bps = llmax(mCurrentBPS[i] - mNominalBPS[i], unused_current);
				}
				else
				{
					avail_bps = mCurrentBPS[i] - used_bps;
				}

				//llinfos << i << " avail " << avail_bps << llendl;

				// Historically, a channel could have used more than its current share,
				// even if it's idle right now.
				// Make sure we don't steal too much.
				if (avail_bps < 0)
				{
					continue;
				}

				// Transfer some bandwidth from this channel into the global pool.
				transfer_bps = avail_bps * TRANSFER_PERCENT;
				mCurrentBPS[i] -= transfer_bps;
				pool_bps += transfer_bps;
			}
		}

		//llinfos << "Pool BPS: " << pool_bps << llendl;
		// Now redistribute the bandwidth to busy channels.
		F32 unused_bps = 0.f;

		for (i = 0; i < TC_EOF; i++)
		{
			if (channel_busy[i])
			{
				F32 add_amount = pool_bps * (mNominalBPS[i] / busy_nominal_sum);
				//llinfos << "Busy " << i << " gets " << pool_bps << llendl;
				mCurrentBPS[i] += add_amount;

				// CRO: make sure this doesn't get too huge
				// JC - Actually, need to let mCurrentBPS go less than nominal, otherwise
				// you aren't allowing bandwidth to actually be moved from one channel
				// to another.  
				// FIXME: If clamping high end, would be good to re-
				// allocate to other channels in the above code.
				const F32 MAX_BPS = 4 * mNominalBPS[i];
				if (mCurrentBPS[i] > MAX_BPS)
				{
					F32 overage = mCurrentBPS[i] - MAX_BPS;
					mCurrentBPS[i] -= overage;
					unused_bps += overage;
				}

				// Paranoia
				if (mCurrentBPS[i] < gThrottleMinimumBPS[i])
				{
					mCurrentBPS[i] = gThrottleMinimumBPS[i];
				}
			}
		}

		// For fun, add the overage back in to objects
		if (unused_bps > 0.f)
		{
			mCurrentBPS[TC_TASK] += unused_bps;
		}
	}
	else
	{
		// No one is busy.
		// Make the channel allocations seek toward nominal.

		// Look for overpumped channels
		F32 starved_nominal_sum = 0;
		F32 avail_bps = 0;
		F32 transfer_bps = 0;
		F32 pool_bps = 0;
		for (i = 0; i < TC_EOF; i++)
		{
			if (mCurrentBPS[i] > mNominalBPS[i])
			{
				avail_bps = (mCurrentBPS[i] - mNominalBPS[i]);
				transfer_bps = avail_bps * RECOVER_PERCENT;

				mCurrentBPS[i] -= transfer_bps;
				pool_bps += transfer_bps;
			}
		}

		// Evenly distribute bandwidth to channels currently
		// using less than nominal.
		for (i = 0; i < TC_EOF; i++)
		{
			if (mCurrentBPS[i] < mNominalBPS[i])
			{
				// We're going to weight allocations by nominal BPS.
				starved_nominal_sum += mNominalBPS[i];
			}
		}

		for (i = 0; i < TC_EOF; i++)
		{
			if (mCurrentBPS[i] < mNominalBPS[i])
			{
				// Distribute bandwidth according to nominal allocation ratios.
				mCurrentBPS[i] += pool_bps * (mNominalBPS[i] / starved_nominal_sum);
			}
		}
	}
	return TRUE;
}
