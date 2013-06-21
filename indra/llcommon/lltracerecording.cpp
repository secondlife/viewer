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
{}

void RecordingBuffers::handOffTo(RecordingBuffers& other)
{
	other.mCounts.reset(&mCounts);
	other.mSamples.reset(&mSamples);
	other.mEvents.reset(&mEvents);
	other.mStackTimers.reset(&mStackTimers);
	other.mMemStats.reset(&mMemStats);
}

void RecordingBuffers::makePrimary()
{
	mCounts.makePrimary();
	mSamples.makePrimary();
	mEvents.makePrimary();
	mStackTimers.makePrimary();
	mMemStats.makePrimary();

	ThreadRecorder* thread_recorder = get_thread_recorder().get();
	AccumulatorBuffer<TimeBlockAccumulator>& timer_accumulator_buffer = mStackTimers;
	// update stacktimer parent pointers
	for (S32 i = 0, end_i = mStackTimers.size(); i < end_i; i++)
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
	return mCounts.isPrimary();
}

void RecordingBuffers::append( const RecordingBuffers& other )
{
	mCounts.addSamples(other.mCounts);
	mSamples.addSamples(other.mSamples);
	mEvents.addSamples(other.mEvents);
	mMemStats.addSamples(other.mMemStats);
	mStackTimers.addSamples(other.mStackTimers);
}

void RecordingBuffers::merge( const RecordingBuffers& other)
{
	mCounts.addSamples(other.mCounts, false);
	mSamples.addSamples(other.mSamples, false);
	mEvents.addSamples(other.mEvents, false);
	mMemStats.addSamples(other.mMemStats, false);
	// for now, hold out timers from merge, need to be displayed per thread
	//mStackTimers.addSamples(other.mStackTimers, false);
}

void RecordingBuffers::reset(RecordingBuffers* other)
{
	mCounts.reset(other ? &other->mCounts : NULL);
	mSamples.reset(other ? &other->mSamples : NULL);
	mEvents.reset(other ? &other->mEvents : NULL);
	mStackTimers.reset(other ? &other->mStackTimers : NULL);
	mMemStats.reset(other ? &other->mMemStats : NULL);
}

void RecordingBuffers::flush()
{
	LLUnitImplicit<F64, LLUnits::Seconds> time_stamp = LLTimer::getTotalSeconds();

	mSamples.flush(time_stamp);
}

///////////////////////////////////////////////////////////////////////
// Recording
///////////////////////////////////////////////////////////////////////

Recording::Recording() 
:	mElapsedSeconds(0)
{
	mBuffers = new RecordingBuffers();
}

Recording::Recording( const Recording& other )
{
	*this = other;
}

Recording& Recording::operator = (const Recording& other)
{
	// this will allow us to seamlessly start without affecting any data we've acquired from other
	setPlayState(PAUSED);

	Recording& mutable_other = const_cast<Recording&>(other);
	mutable_other.update();
	EPlayState other_play_state = other.getPlayState();

	mBuffers = mutable_other.mBuffers;

	LLStopWatchControlsMixin<Recording>::setPlayState(other_play_state);

	// above call will clear mElapsedSeconds as a side effect, so copy it here
	mElapsedSeconds = other.mElapsedSeconds;
	mSamplingTimer = other.mSamplingTimer;
	return *this;
}


Recording::~Recording()
{
	if (isStarted() && LLTrace::get_thread_recorder().notNull())
	{
		LLTrace::get_thread_recorder()->deactivate(this);
	}
}

void Recording::update()
{
	if (isStarted())
	{
		mBuffers.write()->flush();
		LLTrace::get_thread_recorder()->bringUpToDate(this);
		mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();
		mSamplingTimer.reset();
	}
}

void Recording::handleReset()
{
	mBuffers.write()->reset();

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
	mBuffers.write()->flush();
	LLTrace::get_thread_recorder()->deactivate(this);
}

void Recording::handleSplitTo(Recording& other)
{
	mBuffers.write()->handOffTo(*other.mBuffers.write());
}

void Recording::appendRecording( const Recording& other )
{
	update();
	mBuffers.write()->append(*other.mBuffers);
	mElapsedSeconds += other.mElapsedSeconds;
}

void Recording::mergeRecording( const Recording& other)
{
	update();
	mBuffers.write()->merge(*other.mBuffers);
}

LLUnit<F64, LLUnits::Seconds> Recording::getSum(const TraceType<TimeBlockAccumulator>& stat)
{
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	return (F64)(accumulator.mTotalTimeCounter - accumulator.mStartTotalTimeCounter) 
				/ (F64)LLTrace::TimeBlock::countsPerSecond();
}

LLUnit<F64, LLUnits::Seconds> Recording::getSum(const TraceType<TimeBlockAccumulator::SelfTimeFacet>& stat)
{
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	return (F64)(accumulator.mSelfTimeCounter) / (F64)LLTrace::TimeBlock::countsPerSecond();
}


U32 Recording::getSum(const TraceType<TimeBlockAccumulator::CallCountFacet>& stat)
{
	return mBuffers->mStackTimers[stat.getIndex()].mCalls;
}

LLUnit<F64, LLUnits::Seconds> Recording::getPerSec(const TraceType<TimeBlockAccumulator>& stat)
{
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];

	return (F64)(accumulator.mTotalTimeCounter - accumulator.mStartTotalTimeCounter) 
				/ ((F64)LLTrace::TimeBlock::countsPerSecond() * mElapsedSeconds.value());
}

LLUnit<F64, LLUnits::Seconds> Recording::getPerSec(const TraceType<TimeBlockAccumulator::SelfTimeFacet>& stat)
{
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];

	return (F64)(accumulator.mSelfTimeCounter) 
			/ ((F64)LLTrace::TimeBlock::countsPerSecond() * mElapsedSeconds.value());
}

F32 Recording::getPerSec(const TraceType<TimeBlockAccumulator::CallCountFacet>& stat)
{
	return (F32)mBuffers->mStackTimers[stat.getIndex()].mCalls / mElapsedSeconds.value();
}

LLUnit<F64, LLUnits::Bytes> Recording::getMin(const TraceType<MemStatAccumulator>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mSize.getMin();
}

LLUnit<F64, LLUnits::Bytes> Recording::getMean(const TraceType<MemStatAccumulator>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mSize.getMean();
}

LLUnit<F64, LLUnits::Bytes> Recording::getMax(const TraceType<MemStatAccumulator>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mSize.getMax();
}

LLUnit<F64, LLUnits::Bytes> Recording::getStandardDeviation(const TraceType<MemStatAccumulator>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mSize.getStandardDeviation();
}

LLUnit<F64, LLUnits::Bytes> Recording::getLastValue(const TraceType<MemStatAccumulator>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mSize.getLastValue();
}

LLUnit<F64, LLUnits::Bytes> Recording::getMin(const TraceType<MemStatAccumulator::ChildMemFacet>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mChildSize.getMin();
}

LLUnit<F64, LLUnits::Bytes> Recording::getMean(const TraceType<MemStatAccumulator::ChildMemFacet>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mChildSize.getMean();
}

LLUnit<F64, LLUnits::Bytes> Recording::getMax(const TraceType<MemStatAccumulator::ChildMemFacet>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mChildSize.getMax();
}

LLUnit<F64, LLUnits::Bytes> Recording::getStandardDeviation(const TraceType<MemStatAccumulator::ChildMemFacet>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mChildSize.getStandardDeviation();
}

LLUnit<F64, LLUnits::Bytes> Recording::getLastValue(const TraceType<MemStatAccumulator::ChildMemFacet>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mChildSize.getLastValue();
}

U32 Recording::getSum(const TraceType<MemStatAccumulator::AllocationCountFacet>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mAllocatedCount;
}

U32 Recording::getSum(const TraceType<MemStatAccumulator::DeallocationCountFacet>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mAllocatedCount;
}


F64 Recording::getSum( const TraceType<CountAccumulator>& stat )
{
	return mBuffers->mCounts[stat.getIndex()].getSum();
}

F64 Recording::getSum( const TraceType<EventAccumulator>& stat )
{
	return (F64)mBuffers->mEvents[stat.getIndex()].getSum();
}

F64 Recording::getPerSec( const TraceType<CountAccumulator>& stat )
{
	F64 sum = mBuffers->mCounts[stat.getIndex()].getSum();
	return  (sum != 0.0) 
		? (sum / mElapsedSeconds.value())
		: 0.0;
}

U32 Recording::getSampleCount( const TraceType<CountAccumulator>& stat )
{
	return mBuffers->mCounts[stat.getIndex()].getSampleCount();
}

F64 Recording::getMin( const TraceType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getMin();
}

F64 Recording::getMax( const TraceType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getMax();
}

F64 Recording::getMean( const TraceType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getMean();
}

F64 Recording::getStandardDeviation( const TraceType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getStandardDeviation();
}

F64 Recording::getLastValue( const TraceType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getLastValue();
}

U32 Recording::getSampleCount( const TraceType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getSampleCount();
}

F64 Recording::getMin( const TraceType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getMin();
}

F64 Recording::getMax( const TraceType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getMax();
}

F64 Recording::getMean( const TraceType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getMean();
}

F64 Recording::getStandardDeviation( const TraceType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getStandardDeviation();
}

F64 Recording::getLastValue( const TraceType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getLastValue();
}

U32 Recording::getSampleCount( const TraceType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getSampleCount();
}

///////////////////////////////////////////////////////////////////////
// PeriodicRecording
///////////////////////////////////////////////////////////////////////

PeriodicRecording::PeriodicRecording( U32 num_periods, EPlayState state) 
:	mAutoResize(num_periods == 0),
	mCurPeriod(0),
	mNumPeriods(0),
	mRecordingPeriods(num_periods ? num_periods : 1)
{
	setPlayState(state);
}

void PeriodicRecording::nextPeriod()
{
	if (mAutoResize)
	{
		mRecordingPeriods.push_back(Recording());
	}

	Recording& old_recording = getCurRecording();
	mCurPeriod = (mCurPeriod + 1) % mRecordingPeriods.size();
	old_recording.splitTo(getCurRecording());

	mNumPeriods = llmin(mRecordingPeriods.size(), mNumPeriods + 1);
}

void PeriodicRecording::appendRecording(Recording& recording)
{
	getCurRecording().appendRecording(recording);
	nextPeriod();
}


void PeriodicRecording::appendPeriodicRecording( PeriodicRecording& other )
{
	if (other.mRecordingPeriods.empty()) return;

	getCurRecording().update();
	other.getCurRecording().update();
	
	const U32 other_recording_slots = other.mRecordingPeriods.size();
	const U32 other_num_recordings = other.getNumRecordedPeriods();
	const U32 other_current_recording_index = other.mCurPeriod;
	const U32 other_oldest_recording_index = (other_current_recording_index + other_recording_slots - other_num_recordings + 1) % other_recording_slots;

	// append first recording into our current slot
	getCurRecording().appendRecording(other.mRecordingPeriods[other_oldest_recording_index]);

	// from now on, add new recordings for everything after the first
	U32 other_index = (other_oldest_recording_index + 1) % other_recording_slots;

	if (mAutoResize)
	{
		// append first recording into our current slot
		getCurRecording().appendRecording(other.mRecordingPeriods[other_oldest_recording_index]);

		// push back recordings for everything in the middle
		U32 other_index = (other_oldest_recording_index + 1) % other_recording_slots;
		while (other_index != other_current_recording_index)
		{
			mRecordingPeriods.push_back(other.mRecordingPeriods[other_index]);
			other_index = (other_index + 1) % other_recording_slots;
		}

		// add final recording, if it wasn't already added as the first
		if (other_num_recordings > 1)
		{
			mRecordingPeriods.push_back(other.mRecordingPeriods[other_current_recording_index]);
		}

		mCurPeriod = mRecordingPeriods.size() - 1;
		mNumPeriods = mRecordingPeriods.size();
	}
	else
	{
		size_t num_to_copy = llmin(	mRecordingPeriods.size(), (size_t)other_num_recordings);

		std::vector<Recording>::iterator src_it = other.mRecordingPeriods.begin() + other_index ;
		std::vector<Recording>::iterator dest_it = mRecordingPeriods.begin() + mCurPeriod;

		// already consumed the first recording from other, so start counting at 1
		for(size_t i = 1; i < num_to_copy; i++)
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
		
		// want argument to % to be positive, otherwise result could be negative and thus out of bounds
		llassert(num_to_copy >= 1);
		// advance to last recording period copied, and make that our current period
		mCurPeriod = (mCurPeriod + num_to_copy - 1) % mRecordingPeriods.size();
		mNumPeriods = llmin(mRecordingPeriods.size(), mNumPeriods + num_to_copy - 1);
	}

	// end with fresh period, otherwise next appendPeriodicRecording() will merge the first
	// recording period with the last one appended here
	nextPeriod();
	getCurRecording().setPlayState(getPlayState());
}

LLUnit<F64, LLUnits::Seconds> PeriodicRecording::getDuration() const
{
	LLUnit<F64, LLUnits::Seconds> duration;
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
	return getPrevRecording(1);
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
	if (mAutoResize)
	{
		mRecordingPeriods.clear();
		mRecordingPeriods.push_back(Recording());
	}
	else
	{
		for (std::vector<Recording>::iterator it = mRecordingPeriods.begin(), end_it = mRecordingPeriods.end();
			it != end_it;
			++it)
		{
			it->reset();
		}
	}
	mCurPeriod = 0;
	getCurRecording().setPlayState(getPlayState());
}

void PeriodicRecording::handleSplitTo(PeriodicRecording& other)
{
	getCurRecording().splitTo(other.getCurRecording());
}


F64 PeriodicRecording::getPeriodMean( const TraceType<EventAccumulator>& stat, size_t num_periods /*= U32_MAX*/ )
{
	size_t total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64 mean = 0;
	if (num_periods <= 0) { return mean; }

	S32 total_sample_count = 0;

	for (S32 i = 1; i <= num_periods; i++)
	{
		S32 index = (mCurPeriod + total_periods - i) % total_periods;
		if (mRecordingPeriods[index].getDuration() > 0.f)
		{
			S32 period_sample_count = mRecordingPeriods[index].getSampleCount(stat);
			mean += mRecordingPeriods[index].getMean(stat) * period_sample_count;
			total_sample_count += period_sample_count;
		}
	}

	if (total_sample_count)
	{
		mean = mean / total_sample_count;
	}
	return mean;
}

F64 PeriodicRecording::getPeriodMin( const TraceType<EventAccumulator>& stat, size_t num_periods /*= U32_MAX*/ )
{
	size_t total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64 min_val = std::numeric_limits<F64>::max();
	for (S32 i = 1; i <= num_periods; i++)
	{
		S32 index = (mCurPeriod + total_periods - i) % total_periods;
		min_val = llmin(min_val, mRecordingPeriods[index].getMin(stat));
	}
	return min_val;
}

F64 PeriodicRecording::getPeriodMax( const TraceType<EventAccumulator>& stat, size_t num_periods /*= U32_MAX*/ )
{
	size_t total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64 max_val = std::numeric_limits<F64>::min();
	for (S32 i = 1; i <= num_periods; i++)
	{
		S32 index = (mCurPeriod + total_periods - i) % total_periods;
		max_val = llmax(max_val, mRecordingPeriods[index].getMax(stat));
	}
	return max_val;
}

F64 PeriodicRecording::getPeriodMin( const TraceType<SampleAccumulator>& stat, size_t num_periods /*= U32_MAX*/ )
{
	size_t total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64 min_val = std::numeric_limits<F64>::max();
	for (S32 i = 1; i <= num_periods; i++)
	{
		S32 index = (mCurPeriod + total_periods - i) % total_periods;
		min_val = llmin(min_val, mRecordingPeriods[index].getMin(stat));
	}
	return min_val;
}

F64 PeriodicRecording::getPeriodMax(const TraceType<SampleAccumulator>& stat, size_t num_periods /*= U32_MAX*/)
{
	size_t total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64 max_val = std::numeric_limits<F64>::min();
	for (S32 i = 1; i <= num_periods; i++)
	{
		S32 index = (mCurPeriod + total_periods - i) % total_periods;
		max_val = llmax(max_val, mRecordingPeriods[index].getMax(stat));
	}
	return max_val;
}


F64 PeriodicRecording::getPeriodMean( const TraceType<SampleAccumulator>& stat, size_t num_periods /*= U32_MAX*/ )
{
	size_t total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	LLUnit<F64, LLUnits::Seconds> total_duration = 0.f;

	F64 mean = 0;
	if (num_periods <= 0) { return mean; }

	for (S32 i = 1; i <= num_periods; i++)
	{
		S32 index = (mCurPeriod + total_periods - i) % total_periods;
		if (mRecordingPeriods[index].getDuration() > 0.f)
		{
			LLUnit<F64, LLUnits::Seconds> recording_duration = mRecordingPeriods[index].getDuration();
			mean += mRecordingPeriods[index].getMean(stat) * recording_duration.value();
			total_duration += recording_duration;
		}
	}

	if (total_duration.value())
	{
		mean = mean / total_duration;
	}
	return mean;
}



///////////////////////////////////////////////////////////////////////
// ExtendableRecording
///////////////////////////////////////////////////////////////////////

void ExtendableRecording::extend()
{
	// stop recording to get latest data
	mPotentialRecording.update();
	// push the data back to accepted recording
	mAcceptedRecording.appendRecording(mPotentialRecording);
	// flush data, so we can start from scratch
	mPotentialRecording.reset();
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
	mPotentialRecording.splitTo(other.mPotentialRecording);
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
	// push the data back to accepted recording
	mAcceptedRecording.appendPeriodicRecording(mPotentialRecording);
	// flush data, so we can start from scratch
	mPotentialRecording.reset();
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
