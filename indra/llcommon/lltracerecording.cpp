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
	mCountsFloat.write()->reset(other ? other->mCountsFloat : LLCopyOnWritePointer<AccumulatorBuffer<CountAccumulator<F64> > >());
	mMeasurementsFloat.write()->reset(other ? other->mMeasurementsFloat : LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<F64> > >());
	mCounts.write()->reset(other ? other->mCounts : LLCopyOnWritePointer<AccumulatorBuffer<CountAccumulator<S64> > >());
	mMeasurements.write()->reset(other ? other->mMeasurements : LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<S64> > >());
	mStackTimers.write()->reset(other ? other->mStackTimers : LLCopyOnWritePointer<AccumulatorBuffer<TimeBlockAccumulator> >());
	mMemStats.write()->reset(other ? other->mMemStats : LLCopyOnWritePointer<AccumulatorBuffer<MemStatAccumulator> >());
}

///////////////////////////////////////////////////////////////////////
// Recording
///////////////////////////////////////////////////////////////////////

Recording::Recording() 
:	mElapsedSeconds(0)
{}

Recording::Recording( const Recording& other )
:	RecordingBuffers(other),
	mElapsedSeconds(other.mElapsedSeconds),
	mSamplingTimer(other.mSamplingTimer)
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
	mCurPeriod(0)
{
	if (mAutoResize) 
	{
		num_periods = 1;
	}
	if (num_periods)
	{
		mRecordingPeriods.resize(num_periods);
	}
	setPlayState(state);
}

void PeriodicRecording::nextPeriod()
{
	if (mAutoResize)
	{
		mRecordingPeriods.push_back(Recording());
	}

	Recording& old_recording = getCurRecording();

	mCurPeriod = mRecordingPeriods.empty()
				? mCurPeriod + 1
				: (mCurPeriod + 1) % mRecordingPeriods.size();
	old_recording.splitTo(getCurRecording());
}


void PeriodicRecording::appendPeriodicRecording( PeriodicRecording& other )
{
	if (other.mRecordingPeriods.empty()) return;

	EPlayState play_state = getPlayState();
	stop();

	EPlayState other_play_state = other.getPlayState();
	other.pause();

	U32 other_recording_count = other.mRecordingPeriods.size();

	Recording& other_oldest_recording = other.mRecordingPeriods[(other.mCurPeriod + 1) % other.mRecordingPeriods.size()];

	// if I have a recording of any length, then close it off and start a fresh one
	if (getCurRecording().getDuration().value())
	{
		nextPeriod();
	}
	getCurRecording().appendRecording(other_oldest_recording);

	if (other_recording_count > 1)
	{
		if (mAutoResize)
		{
			for (S32 other_index = (other.mCurPeriod + 2) % other_recording_count; 
				other_index != other.mCurPeriod; 
				other_index = (other_index + 1) % other_recording_count)
			{
				llassert(other.mRecordingPeriods[other_index].getDuration() != 0.f 
							&& (mRecordingPeriods.empty() 
								|| other.mRecordingPeriods[other_index].getDuration() != mRecordingPeriods.back().getDuration()));
				mRecordingPeriods.push_back(other.mRecordingPeriods[other_index]);
			}

			mCurPeriod = mRecordingPeriods.size() - 1;
		}
		else
		{
			size_t num_to_copy = llmin(	mRecordingPeriods.size(), other.mRecordingPeriods.size() - 1);
			std::vector<Recording>::iterator src_it = other.mRecordingPeriods.begin() 
														+ (	(other.mCurPeriod + 1									// oldest period
																+ (other.mRecordingPeriods.size() - num_to_copy))	// minus room for copy
															% other.mRecordingPeriods.size());
			std::vector<Recording>::iterator dest_it = mRecordingPeriods.begin() + ((mCurPeriod + 1) % mRecordingPeriods.size());

			for(S32 i = 0; i < num_to_copy; i++)
			{
				*dest_it = *src_it;

				if (++src_it == other.mRecordingPeriods.end())
				{
					src_it = other.mRecordingPeriods.begin();
				}

				if (++dest_it == mRecordingPeriods.end())
				{
					dest_it = mRecordingPeriods.begin();
				}
			}
		
			mCurPeriod = (mCurPeriod + num_to_copy) % mRecordingPeriods.size();
		}
	}

	nextPeriod();

	setPlayState(play_state);
	other.setPlayState(other_play_state);
}

LLUnit<LLUnits::Seconds, F64> PeriodicRecording::getDuration()
{
	LLUnit<LLUnits::Seconds, F64> duration;
	size_t num_periods = mRecordingPeriods.size();
	for (size_t i = 1; i <= num_periods; i++)
	{
		size_t index = (mCurPeriod + num_periods - i) % num_periods;
		duration += mRecordingPeriods[index].getDuration();
	}
	return duration;
}


LLTrace::Recording PeriodicRecording::snapshotCurRecording() const
{
	Recording recording_copy(getCurRecording());
	recording_copy.stop();
	return recording_copy;
}


Recording& PeriodicRecording::getLastRecording()
{
	U32 num_periods = mRecordingPeriods.size();
	return mRecordingPeriods[(mCurPeriod + num_periods - 1) % num_periods];
}

const Recording& PeriodicRecording::getLastRecording() const
{
	return getPrevRecording(1);
}

Recording& PeriodicRecording::getCurRecording()
{
	return mRecordingPeriods[mCurPeriod];
}

const Recording& PeriodicRecording::getCurRecording() const
{
	return mRecordingPeriods[mCurPeriod];
}

Recording& PeriodicRecording::getPrevRecording( U32 offset )
{
	U32 num_periods = mRecordingPeriods.size();
	offset = llclamp(offset, 0u, num_periods - 1);
	return mRecordingPeriods[(mCurPeriod + num_periods - offset) % num_periods];
}

const Recording& PeriodicRecording::getPrevRecording( U32 offset ) const
{
	U32 num_periods = mRecordingPeriods.size();
	offset = llclamp(offset, 0u, num_periods - 1);
	return mRecordingPeriods[(mCurPeriod + num_periods - offset) % num_periods];
}

void PeriodicRecording::handleStart()
{
	getCurRecording().start();
}

void PeriodicRecording::handleStop()
{
	getCurRecording().pause();
}

void PeriodicRecording::handleReset()
{
	mRecordingPeriods.clear();
	mRecordingPeriods.push_back(Recording());
	mCurPeriod = 0;
}

void PeriodicRecording::handleSplitTo(PeriodicRecording& other)
{
	getCurRecording().handOffTo(other.getCurRecording());
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

void ExtendableRecording::handleStart()
{
	mPotentialRecording.start();
}

void ExtendableRecording::handleStop()
{
	mPotentialRecording.pause();
}

void ExtendableRecording::handleReset()
{
	mAcceptedRecording.reset();
	mPotentialRecording.reset();
}

void ExtendableRecording::handleSplitTo(ExtendableRecording& other)
{
	mPotentialRecording.handOffTo(other.mPotentialRecording);
}


///////////////////////////////////////////////////////////////////////
// ExtendablePeriodicRecording
///////////////////////////////////////////////////////////////////////


ExtendablePeriodicRecording::ExtendablePeriodicRecording() 
:	mAcceptedRecording(0), 
	mPotentialRecording(0)
{}

void ExtendablePeriodicRecording::extend()
{
	llassert(mPotentialRecording.getPlayState() == getPlayState());
	// stop recording to get latest data
	mPotentialRecording.pause();
	// push the data back to accepted recording
	mAcceptedRecording.appendPeriodicRecording(mPotentialRecording);
	// flush data, so we can start from scratch
	mPotentialRecording.reset();
	// go back to play state we were in initially
	mPotentialRecording.setPlayState(getPlayState());
}


void ExtendablePeriodicRecording::handleStart()
{
	mPotentialRecording.start();
}

void ExtendablePeriodicRecording::handleStop()
{
	mPotentialRecording.pause();
}

void ExtendablePeriodicRecording::handleReset()
{
	mAcceptedRecording.reset();
	mPotentialRecording.reset();
}

void ExtendablePeriodicRecording::handleSplitTo(ExtendablePeriodicRecording& other)
{
	mPotentialRecording.splitTo(other.mPotentialRecording);
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
