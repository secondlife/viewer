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

#include "lltracesampler.h"
#include "lltrace.h"
#include "llthread.h"

namespace LLTrace
{

///////////////////////////////////////////////////////////////////////
// Sampler
///////////////////////////////////////////////////////////////////////

Sampler::Sampler() 
:	mElapsedSeconds(0),
	mIsStarted(false),
	mRatesStart(new AccumulatorBuffer<RateAccumulator<F32> >()),
	mRates(new AccumulatorBuffer<RateAccumulator<F32> >()),
	mMeasurements(new AccumulatorBuffer<MeasurementAccumulator<F32> >()),
	mStackTimers(new AccumulatorBuffer<TimerAccumulator>()),
	mStackTimersStart(new AccumulatorBuffer<TimerAccumulator>())
{
}

Sampler::~Sampler()
{
}

void Sampler::start()
{
	reset();
	resume();
}

void Sampler::reset()
{
	mRates.write()->reset();
	mMeasurements.write()->reset();
	mStackTimers.write()->reset();

	mElapsedSeconds = 0.0;
	mSamplingTimer.reset();
}

void Sampler::resume()
{
	if (!mIsStarted)
	{
		mSamplingTimer.reset();
		LLTrace::get_thread_trace()->activate(this);
		mIsStarted = true;
	}
}

void Sampler::stop()
{
	if (mIsStarted)
	{
		mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();
		LLTrace::get_thread_trace()->deactivate(this);
		mIsStarted = false;
	}
}


void Sampler::makePrimary()
{
	mRates.write()->makePrimary();
	mMeasurements.write()->makePrimary();
	mStackTimers.write()->makePrimary();
}

bool Sampler::isPrimary()
{
	return mRates->isPrimary();
}

void Sampler::mergeSamples( const Sampler& other )
{
	mRates.write()->mergeSamples(*other.mRates);
	mMeasurements.write()->mergeSamples(*other.mMeasurements);
	mStackTimers.write()->mergeSamples(*other.mStackTimers);
}

void Sampler::initDeltas( const Sampler& other )
{
	mRatesStart.write()->copyFrom(*other.mRates);
	mStackTimersStart.write()->copyFrom(*other.mStackTimers);
}


void Sampler::mergeDeltas( const Sampler& other )
{
	mRates.write()->mergeDeltas(*mRatesStart, *other.mRates);
	mStackTimers.write()->mergeDeltas(*mStackTimersStart, *other.mStackTimers);
	mMeasurements.write()->mergeSamples(*other.mMeasurements);
}


F32 Sampler::getSum( Rate<F32>& stat )
{
	return stat.getAccumulator(mRates).getSum();
}

F32 Sampler::getSum( Measurement<F32>& stat )
{
	return stat.getAccumulator(mMeasurements).getSum();
}


F32 Sampler::getPerSec( Rate<F32>& stat )
{
	return stat.getAccumulator(mRates).getSum() / mElapsedSeconds;
}

F32 Sampler::getMin( Measurement<F32>& stat )
{
	return stat.getAccumulator(mMeasurements).getMin();
}

F32 Sampler::getMax( Measurement<F32>& stat )
{
	return stat.getAccumulator(mMeasurements).getMax();
}

F32 Sampler::getMean( Measurement<F32>& stat )
{
	return stat.getAccumulator(mMeasurements).getMean();
}

F32 Sampler::getStandardDeviation( Measurement<F32>& stat )
{
	return stat.getAccumulator(mMeasurements).getStandardDeviation();
}







}
