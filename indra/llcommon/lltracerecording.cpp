/** 
 * @file lltracesampler.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "linden_common.h"

#include "lltracerecording.h"
#include "lltrace.h"
#include "llthread.h"

namespace LLTrace
{

///////////////////////////////////////////////////////////////////////
// Recording
///////////////////////////////////////////////////////////////////////

Recording::Recording() 
:	mElapsedSeconds(0),
	mRates(new AccumulatorBuffer<RateAccumulator<F32> >()),
	mMeasurements(new AccumulatorBuffer<MeasurementAccumulator<F32> >()),
	mStackTimers(new AccumulatorBuffer<TimerAccumulator>())
{}

Recording::~Recording()
{}

void Recording::handleReset()
{
	mRates.write()->reset();
	mMeasurements.write()->reset();
	mStackTimers.write()->reset();

	mElapsedSeconds = 0.0;
	mSamplingTimer.reset();
}

void Recording::update()
{
	if (mIsStarted)
	{
		LLTrace::get_thread_recorder()->update(this);
		mElapsedSeconds = 0.0;
		mSamplingTimer.reset();
	}
}

void Recording::handleStart()
{
		mSamplingTimer.reset();
		LLTrace::get_thread_recorder()->activate(this);
}

void Recording::handleStop()
	{
		mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();
		LLTrace::get_thread_recorder()->deactivate(this);
	}

void Recording::handleSplitTo(Recording& other)
{
	stop();
	other.restart();
}


void Recording::makePrimary()
{
	mRates.write()->makePrimary();
	mMeasurements.write()->makePrimary();
	mStackTimers.write()->makePrimary();
}

bool Recording::isPrimary()
{
	return mRates->isPrimary();
}

void Recording::mergeSamples( const Recording& other )
{
	mRates.write()->mergeSamples(*other.mRates);
	mMeasurements.write()->mergeSamples(*other.mMeasurements);
	mStackTimers.write()->mergeSamples(*other.mStackTimers);
}

void Recording::mergeDeltas(const Recording& baseline, const Recording& target)
{
	mRates.write()->mergeDeltas(*baseline.mRates, *target.mRates);
	mStackTimers.write()->mergeDeltas(*baseline.mStackTimers, *target.mStackTimers);
}


F32 Recording::getSum(const Rate<F32>& stat)
{
	return stat.getAccumulator(mRates).getSum();
}

F32 Recording::getPerSec(const Rate<F32>& stat)
{
	return stat.getAccumulator(mRates).getSum() / mElapsedSeconds;
}

F32 Recording::getSum(const Measurement<F32>& stat)
{
	return stat.getAccumulator(mMeasurements).getSum();
}

F32 Recording::getMin(const Measurement<F32>& stat)
{
	return stat.getAccumulator(mMeasurements).getMin();
}

F32 Recording::getMax(const Measurement<F32>& stat)
{
	return stat.getAccumulator(mMeasurements).getMax();
}

F32 Recording::getMean(const Measurement<F32>& stat)
{
	return stat.getAccumulator(mMeasurements).getMean();
}

F32 Recording::getStandardDeviation(const Measurement<F32>& stat)
{
	return stat.getAccumulator(mMeasurements).getStandardDeviation();
}

F32 Recording::getSum(const Count<F32>& stat)
{
	return getSum(stat.mTotal);
}

F32 Recording::getPerSec(const Count<F32>& stat)
{
	return getPerSec(stat.mTotal);
}

F32 Recording::getIncrease(const Count<F32>& stat)
{
	return getSum(stat.mIncrease);
}

F32 Recording::getIncreasePerSec(const Count<F32>& stat)
{
	return getPerSec(stat.mIncrease);
}

F32 Recording::getDecrease(const Count<F32>& stat)
{
	return getSum(stat.mDecrease);
}

F32 Recording::getDecreasePerSec(const Count<F32>& stat)
{
	return getPerSec(stat.mDecrease);
}

F32 Recording::getChurn(const Count<F32>& stat)
{
	return getIncrease(stat) + getDecrease(stat);
}

F32 Recording::getChurnPerSec(const Count<F32>& stat)
{
	return getIncreasePerSec(stat) + getDecreasePerSec(stat);
}


}
