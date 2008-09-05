/** 
 * @file llstat.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "linden_common.h"

#include "llstat.h"
#include "llframetimer.h"
#include "timing.h"



U64 LLStatAccum::sScaleTimes[IMPL_NUM_SCALES] =
{
	USEC_PER_SEC / 10,				// 100 millisec
	USEC_PER_SEC * 1,				// seconds
	USEC_PER_SEC * 60,				// minutes
	USEC_PER_SEC * 60 * 2			// two minutes
#if ENABLE_LONG_TIME_STATS
	// enable these when more time scales are desired
	USEC_PER_SEC * 60*60,			// hours
	USEC_PER_SEC * 24*60*60,		// days
	USEC_PER_SEC * 7*24*60*60,		// weeks
#endif
};



LLStatAccum::LLStatAccum(bool useFrameTimer)
	: mUseFrameTimer(useFrameTimer),
	  mRunning(FALSE),
	  mLastSampleValue(0.0),
	  mLastSampleValid(FALSE)
{
}

LLStatAccum::~LLStatAccum()
{
}



void LLStatAccum::reset(U64 when)
{
	mRunning = TRUE;
	mLastTime = when;

	for (int i = 0; i < IMPL_NUM_SCALES; ++i)
	{
		mBuckets[i].accum = 0.0;
		mBuckets[i].endTime = when + sScaleTimes[i];
		mBuckets[i].lastValid = FALSE;
	}
}

void LLStatAccum::sum(F64 value)
{
	sum(value, getCurrentUsecs());
}

void LLStatAccum::sum(F64 value, U64 when)
{
	if (!mRunning)
	{
		reset(when);
		return;
	}
	if (when < mLastTime)
	{
		// This happens a LOT on some dual core systems.
		lldebugs << "LLStatAccum::sum clock has gone backwards from "
			<< mLastTime << " to " << when << ", resetting" << llendl;

		reset(when);
		return;
	}

	// how long is this value for
	U64 timeSpan = when - mLastTime;

	for (int i = 0; i < IMPL_NUM_SCALES; ++i)
	{
		Bucket& bucket = mBuckets[i];

		if (when < bucket.endTime)
		{
			bucket.accum += value;
		}
		else
		{
			U64 timeScale = sScaleTimes[i];

			U64 timeLeft = when - bucket.endTime;
				// how much time is left after filling this bucket
			
			if (timeLeft < timeScale)
			{
				F64 valueLeft = value * timeLeft / timeSpan;

				bucket.lastValid = TRUE;
				bucket.lastAccum = bucket.accum + (value - valueLeft);
				bucket.accum = valueLeft;
				bucket.endTime += timeScale;
			}
			else
			{
				U64 timeTail = timeLeft % timeScale;

				bucket.lastValid = TRUE;
				bucket.lastAccum = value * timeScale / timeSpan;
				bucket.accum = value * timeTail / timeSpan;
				bucket.endTime += (timeLeft - timeTail) + timeScale;
			}
		}
	}

	mLastTime = when;
}


F32 LLStatAccum::meanValue(TimeScale scale) const
{
	if (!mRunning)
	{
		return 0.0;
	}
	if (scale < 0 || scale >= IMPL_NUM_SCALES)
	{
		llwarns << "llStatAccum::meanValue called for unsupported scale: "
			<< scale << llendl;
		return 0.0;
	}

	const Bucket& bucket = mBuckets[scale];

	F64 value = bucket.accum;
	U64 timeLeft = bucket.endTime - mLastTime;
	U64 scaleTime = sScaleTimes[scale];

	if (bucket.lastValid)
	{
		value += bucket.lastAccum * timeLeft / scaleTime;
	}
	else if (timeLeft < scaleTime)
	{
		value *= scaleTime / (scaleTime - timeLeft);
	}
	else
	{
		value = 0.0;
	}

	return (F32)(value / scaleTime);
}


U64 LLStatAccum::getCurrentUsecs() const
{
	if (mUseFrameTimer)
	{
		return LLFrameTimer::getTotalTime();
	}
	else
	{
		return totalTime();
	}
}


// ------------------------------------------------------------------------

LLStatRate::LLStatRate(bool use_frame_timer)
	: LLStatAccum(use_frame_timer)
{
}

void LLStatRate::count(U32 value)
{
	sum((F64)value * sScaleTimes[SCALE_SECOND]);
}


void LLStatRate::mark()
 { 
	// Effectively the same as count(1), but sets mLastSampleValue
	U64 when = getCurrentUsecs();

	if ( mRunning 
		 && (when > mLastTime) )
	{	// Set mLastSampleValue to the time from the last mark()
		F64 duration = ((F64)(when - mLastTime)) / sScaleTimes[SCALE_SECOND];
		if ( duration > 0.0 )
		{
			mLastSampleValue = 1.0 / duration;
		}
		else
		{
			mLastSampleValue = 0.0;
		}
	}

	sum( (F64) sScaleTimes[SCALE_SECOND], when);
 }


// ------------------------------------------------------------------------


LLStatMeasure::LLStatMeasure(bool use_frame_timer)
	: LLStatAccum(use_frame_timer)
{
}

void LLStatMeasure::sample(F64 value)
{
	U64 when = getCurrentUsecs();

	if (mLastSampleValid)
	{
		F64 avgValue = (value + mLastSampleValue) / 2.0;
		F64 interval = (F64)(when - mLastTime);

		sum(avgValue * interval, when);
	}
	else
	{
		reset(when);
	}

	mLastSampleValid = TRUE;
	mLastSampleValue = value;
}


// ------------------------------------------------------------------------

LLStatTime::LLStatTime(bool use_frame_timer)
	: LLStatAccum(use_frame_timer),
	  mFrameNumber(0),
	  mTotalTimeInFrame(0)
{
	mFrameNumber = LLFrameTimer::getFrameCount();
}

void LLStatTime::start()
{
	// Reset frame accumluation if the frame number has changed
	U32 frame_number = LLFrameTimer::getFrameCount();
	if ( frame_number != mFrameNumber)
	{
		mFrameNumber = frame_number;
		mTotalTimeInFrame = 0;
	}

	sum(0.0);
}

void LLStatTime::stop()
{
	U64 end_time = getCurrentUsecs();
	U64 duration = end_time - mLastTime;
	sum(F64(duration), end_time);
	mTotalTimeInFrame += duration;
}


// ------------------------------------------------------------------------

LLTimer LLStat::sTimer;
LLFrameTimer LLStat::sFrameTimer;

LLStat::LLStat(const U32 num_bins, const BOOL use_frame_timer)
{
	llassert(num_bins > 0);
	U32 i;
	mUseFrameTimer = use_frame_timer;
	mNumValues = 0;
	mLastValue = 0.f;
	mLastTime = 0.f;
	mNumBins = num_bins;
	mCurBin = (mNumBins-1);
	mNextBin = 0;
	mBins      = new F32[mNumBins];
	mBeginTime = new F64[mNumBins];
	mTime      = new F64[mNumBins];
	mDT        = new F32[mNumBins];
	for (i = 0; i < mNumBins; i++)
	{
		mBins[i]      = 0.f;
		mBeginTime[i] = 0.0;
		mTime[i]      = 0.0;
		mDT[i]        = 0.f;
	}
}

LLStat::~LLStat()
{
	delete[] mBins;
	delete[] mBeginTime;
	delete[] mTime;
	delete[] mDT;
}

void LLStat::reset()
{
	U32 i;

	mNumValues = 0;
	mLastValue = 0.f;
	mCurBin = (mNumBins-1);
	delete[] mBins;
	delete[] mBeginTime;
	delete[] mTime;
	delete[] mDT;
	mBins      = new F32[mNumBins];
	mBeginTime = new F64[mNumBins];
	mTime      = new F64[mNumBins];
	mDT        = new F32[mNumBins];
	for (i = 0; i < mNumBins; i++)
	{
		mBins[i]      = 0.f;
		mBeginTime[i] = 0.0;
		mTime[i]      = 0.0;
		mDT[i]        = 0.f;
	}
}

void LLStat::setBeginTime(const F64 time)
{
	mBeginTime[mNextBin] = time;
}

void LLStat::addValueTime(const F64 time, const F32 value)
{
	if (mNumValues < mNumBins)
	{
		mNumValues++;
	}

	// Increment the bin counters.
	mCurBin++;
	if ((U32)mCurBin == mNumBins)
	{
		mCurBin = 0;
	}
	mNextBin++;
	if ((U32)mNextBin == mNumBins)
	{
		mNextBin = 0;
	}

	mBins[mCurBin] = value;
	mTime[mCurBin] = time;
	mDT[mCurBin] = (F32)(mTime[mCurBin] - mBeginTime[mCurBin]);
	//this value is used to prime the min/max calls
	mLastTime = mTime[mCurBin];
	mLastValue = value;

	// Set the begin time for the next stat segment.
	mBeginTime[mNextBin] = mTime[mCurBin];
	mTime[mNextBin] = mTime[mCurBin];
	mDT[mNextBin] = 0.f;
}

void LLStat::start()
{
	if (mUseFrameTimer)
	{
		mBeginTime[mNextBin] = sFrameTimer.getElapsedSeconds();
	}
	else
	{
		mBeginTime[mNextBin] = sTimer.getElapsedTimeF64();
	}
}

void LLStat::addValue(const F32 value)
{
	if (mNumValues < mNumBins)
	{
		mNumValues++;
	}

	// Increment the bin counters.
	mCurBin++;
	if ((U32)mCurBin == mNumBins)
	{
		mCurBin = 0;
	}
	mNextBin++;
	if ((U32)mNextBin == mNumBins)
	{
		mNextBin = 0;
	}

	mBins[mCurBin] = value;
	if (mUseFrameTimer)
	{
		mTime[mCurBin] = sFrameTimer.getElapsedSeconds();
	}
	else
	{
		mTime[mCurBin] = sTimer.getElapsedTimeF64();
	}
	mDT[mCurBin] = (F32)(mTime[mCurBin] - mBeginTime[mCurBin]);

	//this value is used to prime the min/max calls
	mLastTime = mTime[mCurBin];
	mLastValue = value;

	// Set the begin time for the next stat segment.
	mBeginTime[mNextBin] = mTime[mCurBin];
	mTime[mNextBin] = mTime[mCurBin];
	mDT[mNextBin] = 0.f;
}


F32 LLStat::getMax() const
{
	U32 i;
	F32 current_max = mLastValue;
	if (mNumBins == 0)
	{
		current_max = 0.f;
	}
	else
	{
		for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
		{
			// Skip the bin we're currently filling.
			if (i == (U32)mNextBin)
			{
				continue;
			}
			if (mBins[i] > current_max)
			{
				current_max = mBins[i];
			}
		}
	}
	return current_max;
}

F32 LLStat::getMean() const
{
	U32 i;
	F32 current_mean = 0.f;
	U32 samples = 0;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		current_mean += mBins[i];
		samples++;
	}

	// There will be a wrap error at 2^32. :)
	if (samples != 0)
	{
		current_mean /= samples;
	}
	else
	{
		current_mean = 0.f;
	}
	return current_mean;
}

F32 LLStat::getMin() const
{
	U32 i;
	F32 current_min = mLastValue;

	if (mNumBins == 0)
	{
		current_min = 0.f;
	}
	else
	{
		for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
		{
			// Skip the bin we're currently filling.
			if (i == (U32)mNextBin)
			{
				continue;
			}
			if (mBins[i] < current_min)
			{
				current_min = mBins[i];
			}
		}
	}
	return current_min;
}

F32 LLStat::getSum() const
{
	U32 i;
	F32 sum = 0.f;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		sum += mBins[i];
	}

	return sum;
}

F32 LLStat::getSumDuration() const
{
	U32 i;
	F32 sum = 0.f;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		sum += mDT[i];
	}

	return sum;
}

F32 LLStat::getPrev(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}
	return mBins[bin];
}

F32 LLStat::getPrevPerSec(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}
	return mBins[bin] / mDT[bin];
}

F64 LLStat::getPrevBeginTime(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}

	return mBeginTime[bin];
}

F64 LLStat::getPrevTime(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}

	return mTime[bin];
}

F32 LLStat::getBin(S32 bin) const
{
	return mBins[bin];
}

F32 LLStat::getBinPerSec(S32 bin) const
{
	return mBins[bin] / mDT[bin];
}

F64 LLStat::getBinBeginTime(S32 bin) const
{
	return mBeginTime[bin];
}

F64 LLStat::getBinTime(S32 bin) const
{
	return mTime[bin];
}

F32 LLStat::getCurrent() const
{
	return mBins[mCurBin];
}

F32 LLStat::getCurrentPerSec() const
{
	return mBins[mCurBin] / mDT[mCurBin];
}

F64 LLStat::getCurrentBeginTime() const
{
	return mBeginTime[mCurBin];
}

F64 LLStat::getCurrentTime() const
{
	return mTime[mCurBin];
}

F32 LLStat::getCurrentDuration() const
{
	return mDT[mCurBin];
}

F32 LLStat::getMeanPerSec() const
{
	U32 i;
	F32 value = 0.f;
	F32 dt    = 0.f;

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		value += mBins[i];
		dt    += mDT[i];
	}

	if (dt > 0.f)
	{
		return value/dt;
	}
	else
	{
		return 0.f;
	}
}

F32 LLStat::getMeanDuration() const
{
	F32 dur = 0.0f;
	U32 count = 0;
	for (U32 i=0; (i < mNumBins) && (i < mNumValues); i++)
	{
		if (i == (U32)mNextBin)
		{
			continue;
		}
		dur += mDT[i];
		count++;
	}

	if (count > 0)
	{
		dur /= F32(count);
		return dur;
	}
	else
	{
		return 0.f;
	}
}

F32 LLStat::getMaxPerSec() const
{
	U32 i;
	F32 value;

	if (mNextBin != 0)
	{
		value = mBins[0]/mDT[0];
	}
	else if (mNumValues > 0)
	{
		value = mBins[1]/mDT[1];
	}
	else
	{
		value = 0.f;
	}

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		value = llmax(value, mBins[i]/mDT[i]);
	}
	return value;
}

F32 LLStat::getMinPerSec() const
{
	U32 i;
	F32 value;
	
	if (mNextBin != 0)
	{
		value = mBins[0]/mDT[0];
	}
	else if (mNumValues > 0)
	{
		value = mBins[1]/mDT[1];
	}
	else
	{
		value = 0.f;
	}

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		value = llmin(value, mBins[i]/mDT[i]);
	}
	return value;
}

F32 LLStat::getMinDuration() const
{
	F32 dur = 0.0f;
	for (U32 i=0; (i < mNumBins) && (i < mNumValues); i++)
	{
		dur = llmin(dur, mDT[i]);
	}
	return dur;
}

U32 LLStat::getNumValues() const
{
	return mNumValues;
}

S32 LLStat::getNumBins() const
{
	return mNumBins;
}

S32 LLStat::getCurBin() const
{
	return mCurBin;
}

S32 LLStat::getNextBin() const
{
	return mNextBin;
}

F64 LLStat::getLastTime() const
{
	return mLastTime;
}
