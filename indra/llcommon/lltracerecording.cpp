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

#include "lltrace.h"
#include "lltracerecording.h"
#include "lltracethreadrecorder.h"
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

void Recording::update()
{
	if (isStarted())
{
		LLTrace::get_thread_recorder()->update(this);
		mElapsedSeconds = 0.0;
		mSamplingTimer.reset();
	}
}

void Recording::handleReset()
{
	mRates.write()->reset();
	mMeasurements.write()->reset();
	mStackTimers.write()->reset();

	mElapsedSeconds = 0.0;
	mSamplingTimer.reset();
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

bool Recording::isPrimary() const
{
	return mRates->isPrimary();
}

void Recording::mergeRecording( const Recording& other )
{
	mRates.write()->addSamples(*other.mRates);
	mMeasurements.write()->addSamples(*other.mMeasurements);
	mStackTimers.write()->addSamples(*other.mStackTimers);
}

void Recording::mergeRecordingDelta(const Recording& baseline, const Recording& target)
{
	mRates.write()->addDeltas(*baseline.mRates, *target.mRates);
	mStackTimers.write()->addDeltas(*baseline.mStackTimers, *target.mStackTimers);
}

}
