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
	mCountsFloat(new AccumulatorBuffer<CountAccumulator<F64> >()),
	mMeasurementsFloat(new AccumulatorBuffer<MeasurementAccumulator<F64> >()),
	mCounts(new AccumulatorBuffer<CountAccumulator<S64> >()),
	mMeasurements(new AccumulatorBuffer<MeasurementAccumulator<S64> >()),
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
	mCountsFloat.write()->reset();
	mMeasurementsFloat.write()->reset();
	mCounts.write()->reset();
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
	mCountsFloat.write()->makePrimary();
	mMeasurementsFloat.write()->makePrimary();
	mCounts.write()->makePrimary();
	mMeasurements.write()->makePrimary();
	mStackTimers.write()->makePrimary();
}

bool Recording::isPrimary() const
{
	return mCounts->isPrimary();
}

void Recording::mergeRecording( const Recording& other )
{
	mCountsFloat.write()->addSamples(*other.mCountsFloat);
	mMeasurementsFloat.write()->addSamples(*other.mMeasurementsFloat);
	mCounts.write()->addSamples(*other.mCounts);
	mMeasurements.write()->addSamples(*other.mMeasurements);
	mStackTimers.write()->addSamples(*other.mStackTimers);
	mElapsedSeconds += other.mElapsedSeconds;
}

F64 Recording::getSum( const TraceType<CountAccumulator<F64> >& stat ) const
{
	return stat.getAccumulator(mCountsFloat).getSum();
}

S64 Recording::getSum( const TraceType<CountAccumulator<S64> >& stat ) const
{
	return stat.getAccumulator(mCounts).getSum();
}

F64 Recording::getSum( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return (F64)stat.getAccumulator(mMeasurementsFloat).getSum();
}

S64 Recording::getSum( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return (S64)stat.getAccumulator(mMeasurements).getSum();
}



F64 Recording::getPerSec( const TraceType<CountAccumulator<F64> >& stat ) const
{
	return stat.getAccumulator(mCountsFloat).getSum() / mElapsedSeconds;
}

F64 Recording::getPerSec( const TraceType<CountAccumulator<S64> >& stat ) const
{
	return (F64)stat.getAccumulator(mCounts).getSum() / mElapsedSeconds;
}

F64 Recording::getPerSec( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return stat.getAccumulator(mMeasurementsFloat).getSum() / mElapsedSeconds;
}

F64 Recording::getPerSec( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return (F64)stat.getAccumulator(mMeasurements).getSum() / mElapsedSeconds;
}

F64 Recording::getMin( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return stat.getAccumulator(mMeasurementsFloat).getMin();
}

S64 Recording::getMin( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return stat.getAccumulator(mMeasurements).getMin();
}

F64 Recording::getMax( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return stat.getAccumulator(mMeasurementsFloat).getMax();
}

S64 Recording::getMax( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return stat.getAccumulator(mMeasurements).getMax();
}

F64 Recording::getMean( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return stat.getAccumulator(mMeasurementsFloat).getMean();
}

F64 Recording::getMean( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return stat.getAccumulator(mMeasurements).getMean();
}

F64 Recording::getStandardDeviation( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return stat.getAccumulator(mMeasurementsFloat).getStandardDeviation();
}

F64 Recording::getStandardDeviation( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return stat.getAccumulator(mMeasurements).getStandardDeviation();
}

F64 Recording::getLastValue( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return stat.getAccumulator(mMeasurementsFloat).getLastValue();
}

S64 Recording::getLastValue( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return stat.getAccumulator(mMeasurements).getLastValue();
}

U32 Recording::getSampleCount( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return stat.getAccumulator(mMeasurementsFloat).getSampleCount();
}

U32 Recording::getSampleCount( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return stat.getAccumulator(mMeasurements).getSampleCount();
}



///////////////////////////////////////////////////////////////////////
// PeriodicRecording
///////////////////////////////////////////////////////////////////////

PeriodicRecording::PeriodicRecording( S32 num_periods ) 
:	mNumPeriods(num_periods),
	mCurPeriod(0),
	mTotalValid(false),
	mRecordingPeriods( new Recording[num_periods])
{
	llassert(mNumPeriods > 0);
}

PeriodicRecording::~PeriodicRecording()
{
	delete[] mRecordingPeriods;
}


void PeriodicRecording::nextPeriod()
{
	EPlayState play_state = getPlayState();
	getCurRecordingPeriod().stop();
	mCurPeriod = (mCurPeriod + 1) % mNumPeriods;
	switch(play_state)
	{
	case STOPPED:
		break;
	case PAUSED:
		getCurRecordingPeriod().pause();
		break;
	case STARTED:
		getCurRecordingPeriod().start();
		break;
	}
	// new period, need to recalculate total
	mTotalValid = false;
}

Recording& PeriodicRecording::getTotalRecording()
{
	if (!mTotalValid)
	{
		mTotalRecording.reset();
		for (S32 i = mCurPeriod + 1; i < mCurPeriod + mNumPeriods; i++)
		{
			mTotalRecording.mergeRecording(mRecordingPeriods[i % mNumPeriods]);
		}
	}
	mTotalValid = true;
	return mTotalRecording;
}

void PeriodicRecording::handleStart()
{
	getCurRecordingPeriod().handleStart();
}

void PeriodicRecording::handleStop()
{
	getCurRecordingPeriod().handleStop();
}

void PeriodicRecording::handleReset()
{
	getCurRecordingPeriod().handleReset();
}

void PeriodicRecording::handleSplitTo( PeriodicRecording& other )
{
	getCurRecordingPeriod().handleSplitTo(other.getCurRecordingPeriod());
}


///////////////////////////////////////////////////////////////////////
// ExtendableRecording
///////////////////////////////////////////////////////////////////////

void ExtendableRecording::extend()
{
	mAcceptedRecording.mergeRecording(mPotentialRecording);
	mPotentialRecording.reset();
}

void ExtendableRecording::handleStart()
{
	mPotentialRecording.handleStart();
}

void ExtendableRecording::handleStop()
{
	mPotentialRecording.handleStop();
}

void ExtendableRecording::handleReset()
{
	mAcceptedRecording.handleReset();
	mPotentialRecording.handleReset();
}

void ExtendableRecording::handleSplitTo( ExtendableRecording& other )
{
	mPotentialRecording.handleSplitTo(other.mPotentialRecording);
}

PeriodicRecording& get_frame_recording()
{
	static PeriodicRecording sRecording(64);
	return sRecording;
}

}

void LLVCRControlsMixinCommon::start()
{
	switch (mPlayState)
	{
	case STOPPED:
		handleReset();
		handleStart();
		break;
	case PAUSED:
		handleStart();
		break;
	case STARTED:
		handleReset();
		break;
	}
	mPlayState = STARTED;
}

void LLVCRControlsMixinCommon::stop()
{
	switch (mPlayState)
	{
	case STOPPED:
		break;
	case PAUSED:
		handleStop();
		break;
	case STARTED:
		handleStop();
		break;
	}
	mPlayState = STOPPED;
}

void LLVCRControlsMixinCommon::pause()
{
	switch (mPlayState)
	{
	case STOPPED:
		break;
	case PAUSED:
		break;
	case STARTED:
		handleStop();
		break;
	}
	mPlayState = PAUSED;
}

void LLVCRControlsMixinCommon::resume()
{
	switch (mPlayState)
	{
	case STOPPED:
		handleStart();
		break;
	case PAUSED:
		handleStart();
		break;
	case STARTED:
		break;
	}
	mPlayState = STARTED;
}

void LLVCRControlsMixinCommon::restart()
{
	switch (mPlayState)
	{
	case STOPPED:
		handleReset();
		handleStart();
		break;
	case PAUSED:
		handleReset();
		handleStart();
		break;
	case STARTED:
		handleReset();
		break;
	}
	mPlayState = STARTED;
}

void LLVCRControlsMixinCommon::reset()
{
	handleReset();
}
