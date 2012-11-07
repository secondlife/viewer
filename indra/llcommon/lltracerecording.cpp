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
	other.mMeasurementsFloat.write()->reset(mMeasurementsFloat);
	other.mMeasurements.write()->reset(mMeasurements);
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

void Recording::appendRecording( const Recording& other )
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
	return (*mCountsFloat)[stat.getIndex()].getSum();
}

S64 Recording::getSum( const TraceType<CountAccumulator<S64> >& stat ) const
{
	return (*mCounts)[stat.getIndex()].getSum();
}

F64 Recording::getSum( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return (F64)(*mMeasurementsFloat)[stat.getIndex()].getSum();
}

S64 Recording::getSum( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return (S64)(*mMeasurements)[stat.getIndex()].getSum();
}



F64 Recording::getPerSec( const TraceType<CountAccumulator<F64> >& stat ) const
{
	F64 sum = (*mCountsFloat)[stat.getIndex()].getSum();
	return  (sum != 0.0) 
		? (sum / mElapsedSeconds)
		: 0.0;
}

F64 Recording::getPerSec( const TraceType<CountAccumulator<S64> >& stat ) const
{
	S64 sum = (*mCounts)[stat.getIndex()].getSum();
	return (sum != 0) 
		? ((F64)sum / mElapsedSeconds)
		: 0.0;
}

U32 Recording::getSampleCount( const TraceType<CountAccumulator<F64> >& stat ) const
{
	return (*mCountsFloat)[stat.getIndex()].getSampleCount();
}

U32 Recording::getSampleCount( const TraceType<CountAccumulator<S64> >& stat ) const
{
	return (*mMeasurementsFloat)[stat.getIndex()].getSampleCount();
}


F64 Recording::getPerSec( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	F64 sum = (*mMeasurementsFloat)[stat.getIndex()].getSum();
	return  (sum != 0.0) 
		? (sum / mElapsedSeconds)
		: 0.0;
}

F64 Recording::getPerSec( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	S64 sum = (*mMeasurements)[stat.getIndex()].getSum();
	return (sum != 0) 
		? ((F64)sum / mElapsedSeconds)
		: 0.0;
}

F64 Recording::getMin( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return (*mMeasurementsFloat)[stat.getIndex()].getMin();
}

S64 Recording::getMin( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return (*mMeasurements)[stat.getIndex()].getMin();
}

F64 Recording::getMax( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return (*mMeasurementsFloat)[stat.getIndex()].getMax();
}

S64 Recording::getMax( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return (*mMeasurements)[stat.getIndex()].getMax();
}

F64 Recording::getMean( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return (*mMeasurementsFloat)[stat.getIndex()].getMean();
}

F64 Recording::getMean( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return (*mMeasurements)[stat.getIndex()].getMean();
}

F64 Recording::getStandardDeviation( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return (*mMeasurementsFloat)[stat.getIndex()].getStandardDeviation();
}

F64 Recording::getStandardDeviation( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return (*mMeasurements)[stat.getIndex()].getStandardDeviation();
}

F64 Recording::getLastValue( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return (*mMeasurementsFloat)[stat.getIndex()].getLastValue();
}

S64 Recording::getLastValue( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return (*mMeasurements)[stat.getIndex()].getLastValue();
}

U32 Recording::getSampleCount( const TraceType<MeasurementAccumulator<F64> >& stat ) const
{
	return (*mMeasurementsFloat)[stat.getIndex()].getSampleCount();
}

U32 Recording::getSampleCount( const TraceType<MeasurementAccumulator<S64> >& stat ) const
{
	return (*mMeasurements)[stat.getIndex()].getSampleCount();
}



///////////////////////////////////////////////////////////////////////
// PeriodicRecording
///////////////////////////////////////////////////////////////////////

PeriodicRecording::PeriodicRecording( S32 num_periods, EStopWatchState state) 
:	mNumPeriods(num_periods),
	mCurPeriod(0),
	mTotalValid(false),
	mRecordingPeriods( new Recording[num_periods])
{
	llassert(mNumPeriods > 0);
	initTo(state);
}

PeriodicRecording::~PeriodicRecording()
{
	delete[] mRecordingPeriods;
}


void PeriodicRecording::nextPeriod()
{
	EStopWatchState play_state = getPlayState();
	Recording& old_recording = getCurRecordingPeriod();
	mCurPeriod = (mCurPeriod + 1) % mNumPeriods;
	old_recording.splitTo(getCurRecordingPeriod());

	switch(play_state)
	{
	case STOPPED:
		getCurRecordingPeriod().stop();
		break;
	case PAUSED:
		getCurRecordingPeriod().pause();
		break;
	case STARTED:
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
			mTotalRecording.appendRecording(mRecordingPeriods[i % mNumPeriods]);
		}
	}
	mTotalValid = true;
	return mTotalRecording;
}

void PeriodicRecording::start()
{
	getCurRecordingPeriod().start();
}

void PeriodicRecording::stop()
{
	getCurRecordingPeriod().stop();
}

void PeriodicRecording::pause()
{
	getCurRecordingPeriod().pause();
}

void PeriodicRecording::resume()
{
	getCurRecordingPeriod().resume();
}

void PeriodicRecording::restart()
{
	getCurRecordingPeriod().restart();
}

void PeriodicRecording::reset()
{
	getCurRecordingPeriod().reset();
}

void PeriodicRecording::splitTo(PeriodicRecording& other)
{
	getCurRecordingPeriod().splitTo(other.getCurRecordingPeriod());
}

void PeriodicRecording::splitFrom(PeriodicRecording& other)
{
	getCurRecordingPeriod().splitFrom(other.getCurRecordingPeriod());
}


///////////////////////////////////////////////////////////////////////
// ExtendableRecording
///////////////////////////////////////////////////////////////////////

void ExtendableRecording::extend()
{
	mAcceptedRecording.appendRecording(mPotentialRecording);
	mPotentialRecording.reset();
}

void ExtendableRecording::start()
{
	mPotentialRecording.start();
}

void ExtendableRecording::stop()
{
	mPotentialRecording.stop();
}

void ExtendableRecording::pause()
{
	mPotentialRecording.pause();
}

void ExtendableRecording::resume()
{
	mPotentialRecording.resume();
}

void ExtendableRecording::restart()
{
	mAcceptedRecording.reset();
	mPotentialRecording.restart();
}

void ExtendableRecording::reset()
{
	mAcceptedRecording.reset();
	mPotentialRecording.reset();
}

void ExtendableRecording::splitTo(ExtendableRecording& other)
{
	mPotentialRecording.splitTo(other.mPotentialRecording);
}

void ExtendableRecording::splitFrom(ExtendableRecording& other)
{
	mPotentialRecording.splitFrom(other.mPotentialRecording);
}

PeriodicRecording& get_frame_recording()
{
	static LLThreadLocalPointer<PeriodicRecording> sRecording(new PeriodicRecording(64, PeriodicRecording::STARTED));
	return *sRecording;
}

}

void LLStopWatchControlsMixinCommon::start()
{
	switch (mState)
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
	default:
		llassert(false);
		break;
	}
	mState = STARTED;
}

void LLStopWatchControlsMixinCommon::stop()
{
	switch (mState)
	{
	case STOPPED:
		break;
	case PAUSED:
		handleStop();
		break;
	case STARTED:
		handleStop();
		break;
	default:
		llassert(false);
		break;
	}
	mState = STOPPED;
}

void LLStopWatchControlsMixinCommon::pause()
{
	switch (mState)
	{
	case STOPPED:
		break;
	case PAUSED:
		break;
	case STARTED:
		handleStop();
		break;
	default:
		llassert(false);
		break;
	}
	mState = PAUSED;
}

void LLStopWatchControlsMixinCommon::resume()
{
	switch (mState)
	{
	case STOPPED:
		handleStart();
		break;
	case PAUSED:
		handleStart();
		break;
	case STARTED:
		break;
	default:
		llassert(false);
		break;
	}
	mState = STARTED;
}

void LLStopWatchControlsMixinCommon::restart()
{
	switch (mState)
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
	default:
		llassert(false);
		break;
	}
	mState = STARTED;
}

void LLStopWatchControlsMixinCommon::reset()
{
	handleReset();
}

void LLStopWatchControlsMixinCommon::initTo( EStopWatchState state )
{
	switch(state)
	{
	case STOPPED:
		break;
	case PAUSED:
		break;
	case STARTED:
		handleStart();
		break;
	default:
		llassert(false);
		break;
	}

	mState = state;
}
