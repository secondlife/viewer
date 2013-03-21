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
#include "llfasttimer.h"
#include "lltracerecording.h"
#include "lltracethreadrecorder.h"
#include "llthread.h"

namespace LLTrace
{


///////////////////////////////////////////////////////////////////////
// RecordingBuffers
///////////////////////////////////////////////////////////////////////

RecordingBuffers::RecordingBuffers() 
:	mCountsFloat(new AccumulatorBuffer<CountAccumulator<F64> >()),
	mMeasurementsFloat(new AccumulatorBuffer<MeasurementAccumulator<F64> >()),
	mCounts(new AccumulatorBuffer<CountAccumulator<S64> >()),
	mMeasurements(new AccumulatorBuffer<MeasurementAccumulator<S64> >()),
	mStackTimers(new AccumulatorBuffer<TimeBlockAccumulator>()),
	mMemStats(new AccumulatorBuffer<MemStatAccumulator>())
{}

void RecordingBuffers::handOffTo(RecordingBuffers& other)
{
	other.mCountsFloat.write()->reset(mCountsFloat);
	other.mMeasurementsFloat.write()->reset(mMeasurementsFloat);
	other.mCounts.write()->reset(mCounts);
	other.mMeasurements.write()->reset(mMeasurements);
	other.mStackTimers.write()->reset(mStackTimers);
	other.mMemStats.write()->reset(mMemStats);
}

void RecordingBuffers::makePrimary()
{
	mCountsFloat.write()->makePrimary();
	mMeasurementsFloat.write()->makePrimary();
	mCounts.write()->makePrimary();
	mMeasurements.write()->makePrimary();
	mStackTimers.write()->makePrimary();
	mMemStats.write()->makePrimary();

	ThreadRecorder* thread_recorder = get_thread_recorder().get();
	AccumulatorBuffer<TimeBlockAccumulator>& timer_accumulator_buffer = *mStackTimers.write();
	// update stacktimer parent pointers
	for (S32 i = 0, end_i = mStackTimers->size(); i < end_i; i++)
	{
		TimeBlockTreeNode* tree_node = thread_recorder->getTimeBlockTreeNode(i);
		if (tree_node)
		{
			timer_accumulator_buffer[i].mParent = tree_node->mParent;
		}
	}
}

bool RecordingBuffers::isPrimary() const
{
	return mCounts->isPrimary();
}

void RecordingBuffers::makeUnique()
{
	mCountsFloat.makeUnique();
	mMeasurementsFloat.makeUnique();
	mCounts.makeUnique();
	mMeasurements.makeUnique();
	mStackTimers.makeUnique();
	mMemStats.makeUnique();
}

void RecordingBuffers::appendBuffers( const RecordingBuffers& other )
{
	mCountsFloat.write()->addSamples(*other.mCountsFloat);
	mMeasurementsFloat.write()->addSamples(*other.mMeasurementsFloat);
	mCounts.write()->addSamples(*other.mCounts);
	mMeasurements.write()->addSamples(*other.mMeasurements);
	mMemStats.write()->addSamples(*other.mMemStats);
	mStackTimers.write()->addSamples(*other.mStackTimers);
}

void RecordingBuffers::mergeBuffers( const RecordingBuffers& other)
{
	mCountsFloat.write()->addSamples(*other.mCountsFloat);
	mMeasurementsFloat.write()->addSamples(*other.mMeasurementsFloat);
	mCounts.write()->addSamples(*other.mCounts);
	mMeasurements.write()->addSamples(*other.mMeasurements);
	mMemStats.write()->addSamples(*other.mMemStats);
}

void RecordingBuffers::resetBuffers(RecordingBuffers* other)
{
	mCountsFloat.write()->reset(other ? other->mCountsFloat : NULL);
	mMeasurementsFloat.write()->reset(other ? other->mMeasurementsFloat : NULL);
	mCounts.write()->reset(other ? other->mCounts : NULL);
	mMeasurements.write()->reset(other ? other->mMeasurements : NULL);
	mStackTimers.write()->reset(other ? other->mStackTimers : NULL);
	mMemStats.write()->reset(other ? other->mMemStats : NULL);
}

///////////////////////////////////////////////////////////////////////
// Recording
///////////////////////////////////////////////////////////////////////

Recording::Recording() 
:	mElapsedSeconds(0)
{}

Recording::Recording( const Recording& other )
{
	LLStopWatchControlsMixin<Recording>::setPlayState(other.getPlayState());
}


Recording::~Recording()
{
	stop();
	llassert(isStopped());
}

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
	resetBuffers();

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
	LLTrace::TimeBlock::processTimes();
	LLTrace::get_thread_recorder()->deactivate(this);
}

void Recording::handleSplitTo(Recording& other)
{
	stop();
	other.restart();
	handOffTo(other);
}

void Recording::appendRecording( const Recording& other )
{
	appendBuffers(other);
	mElapsedSeconds += other.mElapsedSeconds;
}

void Recording::mergeRecording( const Recording& other)
{
	mergeBuffers(other);
}

LLUnit<LLUnits::Seconds, F64> Recording::getSum(const TraceType<TimeBlockAccumulator>& stat) const
{
	const TimeBlockAccumulator& accumulator = (*mStackTimers)[stat.getIndex()];
	return (F64)(accumulator.mTotalTimeCounter - accumulator.mStartTotalTimeCounter) 
				/ (F64)LLTrace::TimeBlock::countsPerSecond();
}

LLUnit<LLUnits::Seconds, F64> Recording::getSum(const TraceType<TimeBlockAccumulator::SelfTimeAspect>& stat) const
{
	const TimeBlockAccumulator& accumulator = (*mStackTimers)[stat.getIndex()];
	return (F64)(accumulator.mSelfTimeCounter) / (F64)LLTrace::TimeBlock::countsPerSecond();
}


U32 Recording::getSum(const TraceType<TimeBlockAccumulator::CallCountAspect>& stat) const
{
	return (*mStackTimers)[stat.getIndex()].mCalls;
}

LLUnit<LLUnits::Seconds, F64> Recording::getPerSec(const TraceType<TimeBlockAccumulator>& stat) const
{
	const TimeBlockAccumulator& accumulator = (*mStackTimers)[stat.getIndex()];

	return (F64)(accumulator.mTotalTimeCounter - accumulator.mStartTotalTimeCounter) 
				/ ((F64)LLTrace::TimeBlock::countsPerSecond() * mElapsedSeconds);
}

LLUnit<LLUnits::Seconds, F64> Recording::getPerSec(const TraceType<TimeBlockAccumulator::SelfTimeAspect>& stat) const
{
	const TimeBlockAccumulator& accumulator = (*mStackTimers)[stat.getIndex()];

	return (F64)(accumulator.mSelfTimeCounter) 
			/ ((F64)LLTrace::TimeBlock::countsPerSecond() * mElapsedSeconds);
}

F32 Recording::getPerSec(const TraceType<TimeBlockAccumulator::CallCountAspect>& stat) const
{
	return (F32)(*mStackTimers)[stat.getIndex()].mCalls / mElapsedSeconds;
}

LLUnit<LLUnits::Bytes, U32> Recording::getSum(const TraceType<MemStatAccumulator>& stat) const
{
	return (*mMemStats)[stat.getIndex()].mAllocatedCount;
}

LLUnit<LLUnits::Bytes, F32> Recording::getPerSec(const TraceType<MemStatAccumulator>& stat) const
{
	return (F32)(*mMemStats)[stat.getIndex()].mAllocatedCount / mElapsedSeconds;
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

PeriodicRecording::PeriodicRecording( U32 num_periods, EPlayState state) 
:	mAutoResize(num_periods == 0),
	mCurPeriod(0),
	mTotalValid(false)
{
	if (num_periods)
	{
		mRecordingPeriods.resize(num_periods);
	}
	setPlayState(state);
}

void PeriodicRecording::nextPeriod()
{
	EPlayState play_state = getPlayState();
	Recording& old_recording = getCurRecordingPeriod();
	if (mAutoResize)
	{
		mRecordingPeriods.push_back(Recording());
	}
	U32 num_periods = mRecordingPeriods.size();
	mCurPeriod = (num_periods > 0) 
				? (mCurPeriod + 1) % num_periods 
				: mCurPeriod + 1;
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
		U32 num_periods = mRecordingPeriods.size();

		if (num_periods)
		{
			for (S32 i = mCurPeriod + 1; i < mCurPeriod + num_periods; i++)
			{
				mTotalRecording.appendRecording(mRecordingPeriods[i % num_periods]);
			}
		}
		else
		{
			for (S32 i = 0; i < mCurPeriod; i++)
			{
				mTotalRecording.appendRecording(mRecordingPeriods[i]);
			}
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
	// stop recording to get latest data
	mPotentialRecording.stop();
	// push the data back to accepted recording
	mAcceptedRecording.appendRecording(mPotentialRecording);
	// flush data, so we can start from scratch
	mPotentialRecording.reset();
	// go back to play state we were in initially
	mPotentialRecording.setPlayState(getPlayState());
}

void ExtendableRecording::start()
{
	LLStopWatchControlsMixin<ExtendableRecording>::start();
	mPotentialRecording.start();
}

void ExtendableRecording::stop()
{
	LLStopWatchControlsMixin<ExtendableRecording>::stop();
	mPotentialRecording.stop();
}

void ExtendableRecording::pause()
{
	LLStopWatchControlsMixin<ExtendableRecording>::pause();
	mPotentialRecording.pause();
}

void ExtendableRecording::resume()
{
	LLStopWatchControlsMixin<ExtendableRecording>::resume();
	mPotentialRecording.resume();
}

void ExtendableRecording::restart()
{
	LLStopWatchControlsMixin<ExtendableRecording>::restart();
	mAcceptedRecording.reset();
	mPotentialRecording.restart();
}

void ExtendableRecording::reset()
{
	LLStopWatchControlsMixin<ExtendableRecording>::reset();
	mAcceptedRecording.reset();
	mPotentialRecording.reset();
}

void ExtendableRecording::splitTo(ExtendableRecording& other)
{
	LLStopWatchControlsMixin<ExtendableRecording>::splitTo(other);
	mPotentialRecording.splitTo(other.mPotentialRecording);
}

void ExtendableRecording::splitFrom(ExtendableRecording& other)
{
	LLStopWatchControlsMixin<ExtendableRecording>::splitFrom(other);
	mPotentialRecording.splitFrom(other.mPotentialRecording);
}

PeriodicRecording& get_frame_recording()
{
	static LLThreadLocalPointer<PeriodicRecording> sRecording(new PeriodicRecording(1000, PeriodicRecording::STARTED));
	return *sRecording;
}

}

void LLStopWatchControlsMixinCommon::start()
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
	default:
		llassert(false);
		break;
	}
	mPlayState = STARTED;
}

void LLStopWatchControlsMixinCommon::stop()
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
	default:
		llassert(false);
		break;
	}
	mPlayState = STOPPED;
}

void LLStopWatchControlsMixinCommon::pause()
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
	default:
		llassert(false);
		break;
	}
	mPlayState = PAUSED;
}

void LLStopWatchControlsMixinCommon::resume()
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
	default:
		llassert(false);
		break;
	}
	mPlayState = STARTED;
}

void LLStopWatchControlsMixinCommon::restart()
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
	default:
		llassert(false);
		break;
	}
	mPlayState = STARTED;
}

void LLStopWatchControlsMixinCommon::reset()
{
	handleReset();
}

void LLStopWatchControlsMixinCommon::setPlayState( EPlayState state )
{
	switch(state)
	{
	case STOPPED:
		stop();
		break;
	case PAUSED:
		pause();
		break;
	case STARTED:
		start();
		break;
	default:
		llassert(false);
		break;
	}

	mPlayState = state;
}
