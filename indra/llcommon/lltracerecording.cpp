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
#include "llfasttimer.h"
#include "lltracethreadrecorder.h"
#include "llthread.h"

namespace LLTrace
{

extern MemStatHandle gTraceMemStat;

///////////////////////////////////////////////////////////////////////
// Recording
///////////////////////////////////////////////////////////////////////

Recording::Recording(EPlayState state) 
:	mElapsedSeconds(0),
	mInHandOff(false)
{
	claim_alloc(gTraceMemStat, this);
	mBuffers = new AccumulatorBufferGroup();
	claim_alloc(gTraceMemStat, mBuffers);
	setPlayState(state);
}

Recording::Recording( const Recording& other )
{
	claim_alloc(gTraceMemStat, this);
	*this = other;
}

Recording& Recording::operator = (const Recording& other)
{
	// this will allow us to seamlessly start without affecting any data we've acquired from other
	setPlayState(PAUSED);

	const_cast<Recording&>(other).update();
	EPlayState other_play_state = other.getPlayState();

	mBuffers = other.mBuffers;

	// above call will clear mElapsedSeconds as a side effect, so copy it here
	mElapsedSeconds = other.mElapsedSeconds;
	mSamplingTimer = other.mSamplingTimer;

	setPlayState(other_play_state);

	return *this;
}


Recording::~Recording()
{
	disclaim_alloc(gTraceMemStat, this);
	disclaim_alloc(gTraceMemStat, mBuffers);

	if (isStarted() && LLTrace::get_thread_recorder().notNull())
	{
		LLTrace::get_thread_recorder()->deactivate(mBuffers.write());
	}
}

void Recording::update()
{
	if (isStarted())
	{
		mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();
		AccumulatorBufferGroup* buffers = mBuffers.write();
		LLTrace::get_thread_recorder()->bringUpToDate(buffers);

		mSamplingTimer.reset();
	}
}

void Recording::handleReset()
{
	mBuffers.write()->reset();

	mElapsedSeconds = F64Seconds(0.0);
	mSamplingTimer.reset();
}

void Recording::handleStart()
{
	mSamplingTimer.reset();
	mBuffers.setStayUnique(true);
	LLTrace::get_thread_recorder()->activate(mBuffers.write(), mInHandOff);
	mInHandOff = false;
}

void Recording::handleStop()
{
	mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();
	LLTrace::get_thread_recorder()->deactivate(mBuffers.write());
	mBuffers.setStayUnique(false);
}

void Recording::handleSplitTo(Recording& other)
{
	other.mInHandOff = true;
	mBuffers.write()->handOffTo(*other.mBuffers.write());
}

void Recording::appendRecording( Recording& other )
{
	update();
	other.update();
	mBuffers.write()->append(*other.mBuffers);
	mElapsedSeconds += other.mElapsedSeconds;
}

F64Seconds Recording::getSum(const StatType<TimeBlockAccumulator>& stat)
{
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	return F64Seconds((F64)(accumulator.mTotalTimeCounter) 
				/ (F64)LLTrace::TimeBlock::countsPerSecond());
}

F64Seconds Recording::getSum(const StatType<TimeBlockAccumulator::SelfTimeFacet>& stat)
{
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	return F64Seconds((F64)(accumulator.mSelfTimeCounter) / (F64)LLTrace::TimeBlock::countsPerSecond());
}


S32 Recording::getSum(const StatType<TimeBlockAccumulator::CallCountFacet>& stat)
{
	return mBuffers->mStackTimers[stat.getIndex()].mCalls;
}

F64Seconds Recording::getPerSec(const StatType<TimeBlockAccumulator>& stat)
{
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];

	return F64Seconds((F64)(accumulator.mTotalTimeCounter) 
				/ ((F64)LLTrace::TimeBlock::countsPerSecond() * mElapsedSeconds.value()));
}

F64Seconds Recording::getPerSec(const StatType<TimeBlockAccumulator::SelfTimeFacet>& stat)
{
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];

	return F64Seconds((F64)(accumulator.mSelfTimeCounter) 
			/ ((F64)LLTrace::TimeBlock::countsPerSecond() * mElapsedSeconds.value()));
}

F32 Recording::getPerSec(const StatType<TimeBlockAccumulator::CallCountFacet>& stat)
{
	return (F32)mBuffers->mStackTimers[stat.getIndex()].mCalls / mElapsedSeconds.value();
}

bool Recording::hasValue(const StatType<MemAccumulator>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mSize.hasValue();
}

F64Kilobytes Recording::getMin(const StatType<MemAccumulator>& stat)
{
	return F64Bytes(mBuffers->mMemStats[stat.getIndex()].mSize.getMin());
}

F64Kilobytes Recording::getMean(const StatType<MemAccumulator>& stat)
{
	return F64Bytes(mBuffers->mMemStats[stat.getIndex()].mSize.getMean());
}

F64Kilobytes Recording::getMax(const StatType<MemAccumulator>& stat)
{
	return F64Bytes(mBuffers->mMemStats[stat.getIndex()].mSize.getMax());
}

F64Kilobytes Recording::getStandardDeviation(const StatType<MemAccumulator>& stat)
{
	return F64Bytes(mBuffers->mMemStats[stat.getIndex()].mSize.getStandardDeviation());
}

F64Kilobytes Recording::getLastValue(const StatType<MemAccumulator>& stat)
{
	return F64Bytes(mBuffers->mMemStats[stat.getIndex()].mSize.getLastValue());
}

F64Kilobytes Recording::getSum(const StatType<MemAccumulator::AllocationFacet>& stat)
{
	return F64Bytes(mBuffers->mMemStats[stat.getIndex()].mAllocations.getSum());
}

F64Kilobytes Recording::getPerSec(const StatType<MemAccumulator::AllocationFacet>& stat)
{
	return F64Bytes(mBuffers->mMemStats[stat.getIndex()].mAllocations.getSum() / mElapsedSeconds.value());
}

S32 Recording::getSampleCount(const StatType<MemAccumulator::AllocationFacet>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mAllocations.getSampleCount();
}

F64Kilobytes Recording::getSum(const StatType<MemAccumulator::DeallocationFacet>& stat)
{
	return F64Bytes(mBuffers->mMemStats[stat.getIndex()].mDeallocations.getSum());
}

F64Kilobytes Recording::getPerSec(const StatType<MemAccumulator::DeallocationFacet>& stat)
{
	return F64Bytes(mBuffers->mMemStats[stat.getIndex()].mDeallocations.getSum() / mElapsedSeconds.value());
}

S32 Recording::getSampleCount(const StatType<MemAccumulator::DeallocationFacet>& stat)
{
	return mBuffers->mMemStats[stat.getIndex()].mDeallocations.getSampleCount();
}

F64 Recording::getSum( const StatType<CountAccumulator>& stat )
{
	return mBuffers->mCounts[stat.getIndex()].getSum();
}

F64 Recording::getSum( const StatType<EventAccumulator>& stat )
{
	return (F64)mBuffers->mEvents[stat.getIndex()].getSum();
}

F64 Recording::getPerSec( const StatType<CountAccumulator>& stat )
{
	F64 sum = mBuffers->mCounts[stat.getIndex()].getSum();
	return  sum / mElapsedSeconds.value();
}

S32 Recording::getSampleCount( const StatType<CountAccumulator>& stat )
{
	return mBuffers->mCounts[stat.getIndex()].getSampleCount();
}

bool Recording::hasValue(const StatType<SampleAccumulator>& stat)
{
	return mBuffers->mSamples[stat.getIndex()].hasValue();
}

F64 Recording::getMin( const StatType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getMin();
}

F64 Recording::getMax( const StatType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getMax();
}

F64 Recording::getMean( const StatType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getMean();
}

F64 Recording::getStandardDeviation( const StatType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getStandardDeviation();
}

F64 Recording::getLastValue( const StatType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getLastValue();
}

S32 Recording::getSampleCount( const StatType<SampleAccumulator>& stat )
{
	return mBuffers->mSamples[stat.getIndex()].getSampleCount();
}

bool Recording::hasValue(const StatType<EventAccumulator>& stat)
{
	return mBuffers->mEvents[stat.getIndex()].hasValue();
}

F64 Recording::getMin( const StatType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getMin();
}

F64 Recording::getMax( const StatType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getMax();
}

F64 Recording::getMean( const StatType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getMean();
}

F64 Recording::getStandardDeviation( const StatType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getStandardDeviation();
}

F64 Recording::getLastValue( const StatType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getLastValue();
}

S32 Recording::getSampleCount( const StatType<EventAccumulator>& stat )
{
	return mBuffers->mEvents[stat.getIndex()].getSampleCount();
}

///////////////////////////////////////////////////////////////////////
// PeriodicRecording
///////////////////////////////////////////////////////////////////////

PeriodicRecording::PeriodicRecording( S32 num_periods, EPlayState state) 
:	mAutoResize(num_periods == 0),
	mCurPeriod(0),
	mNumPeriods(0),
	mRecordingPeriods(num_periods ? num_periods : 1)
{
	setPlayState(state);
	claim_alloc(gTraceMemStat, this);
}

PeriodicRecording::~PeriodicRecording()
{
	disclaim_alloc(gTraceMemStat, this);
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

	mNumPeriods = llmin((S32)mRecordingPeriods.size(), mNumPeriods + 1);
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
	
	const S32 other_recording_slots = other.mRecordingPeriods.size();
	const S32 other_num_recordings = other.getNumRecordedPeriods();
	const S32 other_current_recording_index = other.mCurPeriod;
	const S32 other_oldest_recording_index = (other_current_recording_index + other_recording_slots - other_num_recordings + 1) % other_recording_slots;

	// append first recording into our current slot
	getCurRecording().appendRecording(other.mRecordingPeriods[other_oldest_recording_index]);

	// from now on, add new recordings for everything after the first
	S32 other_index = (other_oldest_recording_index + 1) % other_recording_slots;

	if (mAutoResize)
	{
		// push back recordings for everything in the middle
		S32 other_index = (other_oldest_recording_index + 1) % other_recording_slots;
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
		S32 num_to_copy = llmin((S32)mRecordingPeriods.size(), (S32)other_num_recordings);

		std::vector<Recording>::iterator src_it = other.mRecordingPeriods.begin() + other_index ;
		std::vector<Recording>::iterator dest_it = mRecordingPeriods.begin() + mCurPeriod;

		// already consumed the first recording from other, so start counting at 1
		for(S32 i = 1; i < num_to_copy; i++)
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
		mNumPeriods = llmin((S32)mRecordingPeriods.size(), mNumPeriods + num_to_copy - 1);
	}

	// end with fresh period, otherwise next appendPeriodicRecording() will merge the first
	// recording period with the last one appended here
	nextPeriod();
	getCurRecording().setPlayState(getPlayState());
}

F64Seconds PeriodicRecording::getDuration() const
{
	F64Seconds duration;
	S32 num_periods = mRecordingPeriods.size();
	for (S32 i = 1; i <= num_periods; i++)
	{
		S32 index = (mCurPeriod + num_periods - i) % num_periods;
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

Recording& PeriodicRecording::getPrevRecording( S32 offset )
{
	S32 num_periods = mRecordingPeriods.size();
	offset = llclamp(offset, 0, num_periods - 1);
	return mRecordingPeriods[(mCurPeriod + num_periods - offset) % num_periods];
}

const Recording& PeriodicRecording::getPrevRecording( S32 offset ) const
{
	S32 num_periods = mRecordingPeriods.size();
	offset = llclamp(offset, 0, num_periods - 1);
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
	getCurRecording().stop();

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
	mNumPeriods = 0;
	getCurRecording().setPlayState(getPlayState());
}

void PeriodicRecording::handleSplitTo(PeriodicRecording& other)
{
	getCurRecording().splitTo(other.getCurRecording());
}

F64 PeriodicRecording::getPeriodMin( const StatType<EventAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	bool has_value = false;
	F64 min_val = std::numeric_limits<F64>::max();
	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		if (recording.hasValue(stat))
		{
			min_val = llmin(min_val, recording.getMin(stat));
			has_value = true;
		}
	}

	return has_value 
			? min_val 
			: NaN;
}

F64 PeriodicRecording::getPeriodMax( const StatType<EventAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	bool has_value = false;
	F64 max_val = std::numeric_limits<F64>::min();
	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		if (recording.hasValue(stat))
		{
			max_val = llmax(max_val, recording.getMax(stat));
			has_value = true;
		}
	}

	return has_value 
			? max_val 
			: NaN;
}

// calculates means using aggregates per period
F64 PeriodicRecording::getPeriodMean( const StatType<EventAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64 mean = 0;
	S32 valid_period_count = 0;

	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		if (recording.hasValue(stat))
		{
			mean += recording.getMean(stat);
			valid_period_count++;
		}
	}

	return valid_period_count 
			? mean / (F64)valid_period_count
			: NaN;
}


F64 PeriodicRecording::getPeriodStandardDeviation( const StatType<EventAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64 period_mean = getPeriodMean(stat, num_periods);
	F64 sum_of_squares = 0;
	S32 valid_period_count = 0;

	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		if (recording.hasValue(stat))
		{
			F64 delta = recording.getMean(stat) - period_mean;
			sum_of_squares += delta * delta;
			valid_period_count++;
		}
	}

	return valid_period_count
			? sqrt((F64)sum_of_squares / (F64)valid_period_count)
			: NaN;
}

F64 PeriodicRecording::getPeriodMin( const StatType<SampleAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	bool has_value = false;
	F64 min_val = std::numeric_limits<F64>::max();
	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		if (recording.hasValue(stat))
		{
			min_val = llmin(min_val, recording.getMin(stat));
			has_value = true;
		}
	}

	return has_value 
			? min_val 
			: NaN;
}

F64 PeriodicRecording::getPeriodMax(const StatType<SampleAccumulator>& stat, S32 num_periods /*= S32_MAX*/)
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	bool has_value = false;
	F64 max_val = std::numeric_limits<F64>::min();
	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		if (recording.hasValue(stat))
		{
			max_val = llmax(max_val, recording.getMax(stat));
			has_value = true;
		}
	}

	return has_value 
			? max_val 
			: NaN;
}


F64 PeriodicRecording::getPeriodMean( const StatType<SampleAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	S32 valid_period_count = 0;
	F64 mean = 0;

	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		if (recording.hasValue(stat))
		{
			mean += recording.getMean(stat);
			valid_period_count++;
		}
	}

	return valid_period_count
			? mean / F64(valid_period_count)
			: NaN;
}

F64 PeriodicRecording::getPeriodStandardDeviation( const StatType<SampleAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64 period_mean = getPeriodMean(stat, num_periods);
	S32 valid_period_count = 0;
	F64 sum_of_squares = 0;

	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		if (recording.hasValue(stat))
		{
			F64 delta = recording.getMean(stat) - period_mean;
			sum_of_squares += delta * delta;
			valid_period_count++;
		}
	}

	return valid_period_count
			? sqrt(sum_of_squares / (F64)valid_period_count)
			: NaN;
}


F64Kilobytes PeriodicRecording::getPeriodMin( const StatType<MemAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64Kilobytes min_val(std::numeric_limits<F64>::max());
	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		min_val = llmin(min_val, recording.getMin(stat));
	}

	return min_val;
}

F64Kilobytes PeriodicRecording::getPeriodMin(const MemStatHandle& stat, S32 num_periods)
{
	return getPeriodMin(static_cast<const StatType<MemAccumulator>&>(stat), num_periods);
}

F64Kilobytes PeriodicRecording::getPeriodMax(const StatType<MemAccumulator>& stat, S32 num_periods /*= S32_MAX*/)
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64Kilobytes max_val(0.0);
	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		max_val = llmax(max_val, recording.getMax(stat));
	}

	return max_val;
}

F64Kilobytes PeriodicRecording::getPeriodMax(const MemStatHandle& stat, S32 num_periods)
{
	return getPeriodMax(static_cast<const StatType<MemAccumulator>&>(stat), num_periods);
}

F64Kilobytes PeriodicRecording::getPeriodMean( const StatType<MemAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64Kilobytes mean(0);

	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		mean += recording.getMean(stat);
	}

	return mean / F64(num_periods);
}

F64Kilobytes PeriodicRecording::getPeriodMean(const MemStatHandle& stat, S32 num_periods)
{
	return getPeriodMean(static_cast<const StatType<MemAccumulator>&>(stat), num_periods);
}

F64Kilobytes PeriodicRecording::getPeriodStandardDeviation( const StatType<MemAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	S32 total_periods = mRecordingPeriods.size();
	num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

	F64Kilobytes period_mean = getPeriodMean(stat, num_periods);
	S32 valid_period_count = 0;
	F64 sum_of_squares = 0;

	for (S32 i = 1; i <= num_periods; i++)
	{
		Recording& recording = getPrevRecording(i);
		if (recording.hasValue(stat))
		{
			F64Kilobytes delta = recording.getMean(stat) - period_mean;
			sum_of_squares += delta.value() * delta.value();
			valid_period_count++;
		}
	}

	return F64Kilobytes(valid_period_count
			? sqrt(sum_of_squares / (F64)valid_period_count)
			: NaN);
}

F64Kilobytes PeriodicRecording::getPeriodStandardDeviation(const MemStatHandle& stat, S32 num_periods)
{
	return getPeriodStandardDeviation(static_cast<const StatType<MemAccumulator>&>(stat), num_periods);
}

///////////////////////////////////////////////////////////////////////
// ExtendableRecording
///////////////////////////////////////////////////////////////////////

void ExtendableRecording::extend()
{
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
	static LLThreadLocalPointer<PeriodicRecording> sRecording(new PeriodicRecording(200, PeriodicRecording::STARTED));
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
