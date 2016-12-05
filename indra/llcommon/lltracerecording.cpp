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

inline F64 lerp(F64 a, F64 b, F64 u) 
{
	return a + ((b - a) * u);
}

namespace LLTrace
{

extern MemStatHandle gTraceMemStat;

///////////////////////////////////////////////////////////////////////
// Recording
///////////////////////////////////////////////////////////////////////

Recording::Recording(EPlayState state) 
:	mElapsedSeconds(0),
	mActiveBuffers(NULL)
{
	claim_alloc(gTraceMemStat, this);
	mBuffers = new AccumulatorBufferGroup();
	claim_alloc(gTraceMemStat, mBuffers);
	setPlayState(state);
}

Recording::Recording( const Recording& other )
:	mActiveBuffers(NULL)
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

	// allow recording destruction without thread recorder running, 
	// otherwise thread shutdown could crash if a recording outlives the thread recorder
	// besides, recording construction and destruction is fine without a recorder...just don't attempt to start one
	if (isStarted() && LLTrace::get_thread_recorder().notNull())
	{
		LLTrace::get_thread_recorder()->deactivate(mBuffers.write());
	}
}

// brings recording to front of recorder stack, with up to date info
void Recording::update()
{
#if LL_TRACE_ENABLED
	if (isStarted())
	{
		mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();

		// must have 
		llassert(mActiveBuffers != NULL 
				&& LLTrace::get_thread_recorder().notNull());

		if(!mActiveBuffers->isCurrent())
		{
			AccumulatorBufferGroup* buffers = mBuffers.write();
			LLTrace::get_thread_recorder()->deactivate(buffers);
			mActiveBuffers = LLTrace::get_thread_recorder()->activate(buffers);
		}

		mSamplingTimer.reset();
	}
#endif
}

void Recording::handleReset()
{
#if LL_TRACE_ENABLED
	mBuffers.write()->reset();

	mElapsedSeconds = F64Seconds(0.0);
	mSamplingTimer.reset();
#endif
}

void Recording::handleStart()
{
#if LL_TRACE_ENABLED
	mSamplingTimer.reset();
	mBuffers.setStayUnique(true);
	// must have thread recorder running on this thread
	llassert(LLTrace::get_thread_recorder().notNull());
	mActiveBuffers = LLTrace::get_thread_recorder()->activate(mBuffers.write());
#endif
}

void Recording::handleStop()
{
#if LL_TRACE_ENABLED
	mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();
	// must have thread recorder running on this thread
	llassert(LLTrace::get_thread_recorder().notNull());
	LLTrace::get_thread_recorder()->deactivate(mBuffers.write());
	mActiveBuffers = NULL;
	mBuffers.setStayUnique(false);
#endif
}

void Recording::handleSplitTo(Recording& other)
{
#if LL_TRACE_ENABLED
	mBuffers.write()->handOffTo(*other.mBuffers.write());
#endif
}

void Recording::appendRecording( Recording& other )
{
#if LL_TRACE_ENABLED
	update();
	other.update();
	mBuffers.write()->append(*other.mBuffers);
	mElapsedSeconds += other.mElapsedSeconds;
#endif
}

bool Recording::hasValue(const StatType<TimeBlockAccumulator>& stat)
{
	update();
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	const TimeBlockAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mStackTimers[stat.getIndex()] : NULL;
	return accumulator.hasValue() || (active_accumulator && active_accumulator->hasValue());
}

F64Seconds Recording::getSum(const StatType<TimeBlockAccumulator>& stat)
{
	update();
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	const TimeBlockAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mStackTimers[stat.getIndex()] : NULL;
	return F64Seconds((F64)(accumulator.mTotalTimeCounter) + (F64)(active_accumulator ? active_accumulator->mTotalTimeCounter : 0))
				/ (F64)LLTrace::BlockTimer::countsPerSecond();
}

F64Seconds Recording::getSum(const StatType<TimeBlockAccumulator::SelfTimeFacet>& stat)
{
	update();
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	const TimeBlockAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mStackTimers[stat.getIndex()] : NULL;
	return F64Seconds(((F64)(accumulator.mSelfTimeCounter) + (F64)(active_accumulator ? active_accumulator->mSelfTimeCounter : 0)) / (F64)LLTrace::BlockTimer::countsPerSecond());
}


S32 Recording::getSum(const StatType<TimeBlockAccumulator::CallCountFacet>& stat)
{
	update();
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	const TimeBlockAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mStackTimers[stat.getIndex()] : NULL;
	return accumulator.mCalls + (active_accumulator ? active_accumulator->mCalls : 0);
}

F64Seconds Recording::getPerSec(const StatType<TimeBlockAccumulator>& stat)
{
	update();
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	const TimeBlockAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mStackTimers[stat.getIndex()] : NULL;

	return F64Seconds((F64)(accumulator.mTotalTimeCounter + (active_accumulator ? active_accumulator->mTotalTimeCounter : 0)) 
				/ ((F64)LLTrace::BlockTimer::countsPerSecond() * mElapsedSeconds.value()));
}

F64Seconds Recording::getPerSec(const StatType<TimeBlockAccumulator::SelfTimeFacet>& stat)
{
	update();
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	const TimeBlockAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mStackTimers[stat.getIndex()] : NULL;

	return F64Seconds((F64)(accumulator.mSelfTimeCounter + (active_accumulator ? active_accumulator->mSelfTimeCounter : 0))
			/ ((F64)LLTrace::BlockTimer::countsPerSecond() * mElapsedSeconds.value()));
}

F32 Recording::getPerSec(const StatType<TimeBlockAccumulator::CallCountFacet>& stat)
{
	update();
	const TimeBlockAccumulator& accumulator = mBuffers->mStackTimers[stat.getIndex()];
	const TimeBlockAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mStackTimers[stat.getIndex()] : NULL;
	return (F32)(accumulator.mCalls + (active_accumulator ? active_accumulator->mCalls : 0)) / mElapsedSeconds.value();
}

bool Recording::hasValue(const StatType<MemAccumulator>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return accumulator.mSize.hasValue() || (active_accumulator && active_accumulator->mSize.hasValue() ? active_accumulator->mSize.hasValue() : false);
}

F64Kilobytes Recording::getMin(const StatType<MemAccumulator>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return F64Bytes(llmin(accumulator.mSize.getMin(), (active_accumulator && active_accumulator->mSize.hasValue() ? active_accumulator->mSize.getMin() : F32_MAX)));
}

F64Kilobytes Recording::getMean(const StatType<MemAccumulator>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	
	if (active_accumulator && active_accumulator->mSize.hasValue())
	{
        F32 t = 0.0f;
        S32 div = accumulator.mSize.getSampleCount() + active_accumulator->mSize.getSampleCount();
        if (div > 0)
        {
            t = active_accumulator->mSize.getSampleCount() / div;
        }
		return F64Bytes(lerp(accumulator.mSize.getMean(), active_accumulator->mSize.getMean(), t));
	}
	else
	{
		return F64Bytes(accumulator.mSize.getMean());
	}
}

F64Kilobytes Recording::getMax(const StatType<MemAccumulator>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return F64Bytes(llmax(accumulator.mSize.getMax(), active_accumulator && active_accumulator->mSize.hasValue() ? active_accumulator->mSize.getMax() : F32_MIN));
}

F64Kilobytes Recording::getStandardDeviation(const StatType<MemAccumulator>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	if (active_accumulator && active_accumulator->hasValue())
	{
		F64 sum_of_squares = SampleAccumulator::mergeSumsOfSquares(accumulator.mSize, active_accumulator->mSize);
		return F64Bytes(sqrtf(sum_of_squares / (accumulator.mSize.getSamplingTime().value() + active_accumulator->mSize.getSamplingTime().value())));
	}
	else
	{
		return F64Bytes(accumulator.mSize.getStandardDeviation());
	}
}

F64Kilobytes Recording::getLastValue(const StatType<MemAccumulator>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return F64Bytes(active_accumulator ? active_accumulator->mSize.getLastValue() : accumulator.mSize.getLastValue());
}

bool Recording::hasValue(const StatType<MemAccumulator::AllocationFacet>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return accumulator.mAllocations.hasValue() || (active_accumulator ? active_accumulator->mAllocations.hasValue() : false);
}

F64Kilobytes Recording::getSum(const StatType<MemAccumulator::AllocationFacet>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return F64Bytes(accumulator.mAllocations.getSum() + (active_accumulator ? active_accumulator->mAllocations.getSum() : 0));
}

F64Kilobytes Recording::getPerSec(const StatType<MemAccumulator::AllocationFacet>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return F64Bytes((accumulator.mAllocations.getSum() + (active_accumulator ? active_accumulator->mAllocations.getSum() : 0)) / mElapsedSeconds.value());
}

S32 Recording::getSampleCount(const StatType<MemAccumulator::AllocationFacet>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return accumulator.mAllocations.getSampleCount() + (active_accumulator ? active_accumulator->mAllocations.getSampleCount() : 0);
}

bool Recording::hasValue(const StatType<MemAccumulator::DeallocationFacet>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return accumulator.mDeallocations.hasValue() || (active_accumulator ? active_accumulator->mDeallocations.hasValue() : false);
}


F64Kilobytes Recording::getSum(const StatType<MemAccumulator::DeallocationFacet>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return F64Bytes(accumulator.mDeallocations.getSum() + (active_accumulator ? active_accumulator->mDeallocations.getSum() : 0));
}

F64Kilobytes Recording::getPerSec(const StatType<MemAccumulator::DeallocationFacet>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return F64Bytes((accumulator.mDeallocations.getSum() + (active_accumulator ? active_accumulator->mDeallocations.getSum() : 0)) / mElapsedSeconds.value());
}

S32 Recording::getSampleCount(const StatType<MemAccumulator::DeallocationFacet>& stat)
{
	update();
	const MemAccumulator& accumulator = mBuffers->mMemStats[stat.getIndex()];
	const MemAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mMemStats[stat.getIndex()] : NULL;
	return accumulator.mDeallocations.getSampleCount() + (active_accumulator ? active_accumulator->mDeallocations.getSampleCount() : 0);
}

bool Recording::hasValue(const StatType<CountAccumulator>& stat)
{
	update();
	const CountAccumulator& accumulator = mBuffers->mCounts[stat.getIndex()];
	const CountAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mCounts[stat.getIndex()] : NULL;
	return accumulator.hasValue() || (active_accumulator ? active_accumulator->hasValue() : false);
}

F64 Recording::getSum(const StatType<CountAccumulator>& stat)
{
	update();
	const CountAccumulator& accumulator = mBuffers->mCounts[stat.getIndex()];
	const CountAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mCounts[stat.getIndex()] : NULL;
	return accumulator.getSum() + (active_accumulator ? active_accumulator->getSum() : 0);
}

F64 Recording::getPerSec( const StatType<CountAccumulator>& stat )
{
	update();
	const CountAccumulator& accumulator = mBuffers->mCounts[stat.getIndex()];
	const CountAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mCounts[stat.getIndex()] : NULL;
	F64 sum = accumulator.getSum() + (active_accumulator ? active_accumulator->getSum() : 0);
	return sum / mElapsedSeconds.value();
}

S32 Recording::getSampleCount( const StatType<CountAccumulator>& stat )
{
	update();
	const CountAccumulator& accumulator = mBuffers->mCounts[stat.getIndex()];
	const CountAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mCounts[stat.getIndex()] : NULL;
	return accumulator.getSampleCount() + (active_accumulator ? active_accumulator->getSampleCount() : 0);
}

bool Recording::hasValue(const StatType<SampleAccumulator>& stat)
{
	update();
	const SampleAccumulator& accumulator = mBuffers->mSamples[stat.getIndex()];
	const SampleAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mSamples[stat.getIndex()] : NULL;
	return accumulator.hasValue() || (active_accumulator && active_accumulator->hasValue());
}

F64 Recording::getMin( const StatType<SampleAccumulator>& stat )
{
	update();
	const SampleAccumulator& accumulator = mBuffers->mSamples[stat.getIndex()];
	const SampleAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mSamples[stat.getIndex()] : NULL;
	return llmin(accumulator.getMin(), active_accumulator && active_accumulator->hasValue() ? active_accumulator->getMin() : F32_MAX);
}

F64 Recording::getMax( const StatType<SampleAccumulator>& stat )
{
	update();
	const SampleAccumulator& accumulator = mBuffers->mSamples[stat.getIndex()];
	const SampleAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mSamples[stat.getIndex()] : NULL;
	return llmax(accumulator.getMax(), active_accumulator && active_accumulator->hasValue() ? active_accumulator->getMax() : F32_MIN);
}

F64 Recording::getMean( const StatType<SampleAccumulator>& stat )
{
	update();
	const SampleAccumulator& accumulator = mBuffers->mSamples[stat.getIndex()];
	const SampleAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mSamples[stat.getIndex()] : NULL;
	if (active_accumulator && active_accumulator->hasValue())
	{
        F32 t = 0.0f;
        S32 div = accumulator.getSampleCount() + active_accumulator->getSampleCount();
        if (div > 0)
        {
            t = active_accumulator->getSampleCount() / div;
        }
		return lerp(accumulator.getMean(), active_accumulator->getMean(), t);
	}
	else
	{
		return accumulator.getMean();
	}
}

F64 Recording::getStandardDeviation( const StatType<SampleAccumulator>& stat )
{
	update();
	const SampleAccumulator& accumulator = mBuffers->mSamples[stat.getIndex()];
	const SampleAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mSamples[stat.getIndex()] : NULL;

	if (active_accumulator && active_accumulator->hasValue())
	{
		F64 sum_of_squares = SampleAccumulator::mergeSumsOfSquares(accumulator, *active_accumulator);
		return sqrtf(sum_of_squares / (accumulator.getSamplingTime() + active_accumulator->getSamplingTime()));
	}
	else
	{
		return accumulator.getStandardDeviation();
	}
}

F64 Recording::getLastValue( const StatType<SampleAccumulator>& stat )
{
	update();
	const SampleAccumulator& accumulator = mBuffers->mSamples[stat.getIndex()];
	const SampleAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mSamples[stat.getIndex()] : NULL;
	return (active_accumulator && active_accumulator->hasValue() ? active_accumulator->getLastValue() : accumulator.getLastValue());
}

S32 Recording::getSampleCount( const StatType<SampleAccumulator>& stat )
{
	update();
	const SampleAccumulator& accumulator = mBuffers->mSamples[stat.getIndex()];
	const SampleAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mSamples[stat.getIndex()] : NULL;
	return accumulator.getSampleCount() + (active_accumulator && active_accumulator->hasValue() ? active_accumulator->getSampleCount() : 0);
}

bool Recording::hasValue(const StatType<EventAccumulator>& stat)
{
	update();
	const EventAccumulator& accumulator = mBuffers->mEvents[stat.getIndex()];
	const EventAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mEvents[stat.getIndex()] : NULL;
	return accumulator.hasValue() || (active_accumulator && active_accumulator->hasValue());
}

F64 Recording::getSum( const StatType<EventAccumulator>& stat)
{
	update();
	const EventAccumulator& accumulator = mBuffers->mEvents[stat.getIndex()];
	const EventAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mEvents[stat.getIndex()] : NULL;
	return (F64)(accumulator.getSum() + (active_accumulator && active_accumulator->hasValue() ? active_accumulator->getSum() : 0));
}

F64 Recording::getMin( const StatType<EventAccumulator>& stat )
{
	update();
	const EventAccumulator& accumulator = mBuffers->mEvents[stat.getIndex()];
	const EventAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mEvents[stat.getIndex()] : NULL;
	return llmin(accumulator.getMin(), active_accumulator && active_accumulator->hasValue() ? active_accumulator->getMin() : F32_MAX);
}

F64 Recording::getMax( const StatType<EventAccumulator>& stat )
{
	update();
	const EventAccumulator& accumulator = mBuffers->mEvents[stat.getIndex()];
	const EventAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mEvents[stat.getIndex()] : NULL;
	return llmax(accumulator.getMax(), active_accumulator && active_accumulator->hasValue() ? active_accumulator->getMax() : F32_MIN);
}

F64 Recording::getMean( const StatType<EventAccumulator>& stat )
{
	update();
	const EventAccumulator& accumulator = mBuffers->mEvents[stat.getIndex()];
	const EventAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mEvents[stat.getIndex()] : NULL;
	if (active_accumulator && active_accumulator->hasValue())
	{
		F32 t = 0.0f;
        S32 div = accumulator.getSampleCount() + active_accumulator->getSampleCount();
        if (div > 0)
        {
            t = active_accumulator->getSampleCount() / div;
        }
		return lerp(accumulator.getMean(), active_accumulator->getMean(), t);
	}
	else
	{
		return accumulator.getMean();
	}
}

F64 Recording::getStandardDeviation( const StatType<EventAccumulator>& stat )
{
	update();
	const EventAccumulator& accumulator = mBuffers->mEvents[stat.getIndex()];
	const EventAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mEvents[stat.getIndex()] : NULL;

	if (active_accumulator && active_accumulator->hasValue())
	{
		F64 sum_of_squares = EventAccumulator::mergeSumsOfSquares(accumulator, *active_accumulator);
		return sqrtf(sum_of_squares / (accumulator.getSampleCount() + active_accumulator->getSampleCount()));
	}
	else
	{
		return accumulator.getStandardDeviation();
	}
}

F64 Recording::getLastValue( const StatType<EventAccumulator>& stat )
{
	update();
	const EventAccumulator& accumulator = mBuffers->mEvents[stat.getIndex()];
	const EventAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mEvents[stat.getIndex()] : NULL;
	return active_accumulator ? active_accumulator->getLastValue() : accumulator.getLastValue();
}

S32 Recording::getSampleCount( const StatType<EventAccumulator>& stat )
{
	update();
	const EventAccumulator& accumulator = mBuffers->mEvents[stat.getIndex()];
	const EventAccumulator* active_accumulator = mActiveBuffers ? &mActiveBuffers->mEvents[stat.getIndex()] : NULL;
	return accumulator.getSampleCount() + (active_accumulator ? active_accumulator->getSampleCount() : 0);
}

///////////////////////////////////////////////////////////////////////
// PeriodicRecording
///////////////////////////////////////////////////////////////////////

PeriodicRecording::PeriodicRecording( S32 num_periods, EPlayState state) 
:	mAutoResize(num_periods == 0),
	mCurPeriod(0),
	mNumRecordedPeriods(0),
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

	mNumRecordedPeriods = llmin((S32)mRecordingPeriods.size() - 1, mNumRecordedPeriods + 1);
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
	const S32 other_oldest_recording_index = (other_current_recording_index + other_recording_slots - other_num_recordings) % other_recording_slots;

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
		mNumRecordedPeriods = mRecordingPeriods.size() - 1;
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
		mNumRecordedPeriods = llmin((S32)mRecordingPeriods.size() - 1, mNumRecordedPeriods + num_to_copy - 1);
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
	mNumRecordedPeriods = 0;
	getCurRecording().setPlayState(getPlayState());
}

void PeriodicRecording::handleSplitTo(PeriodicRecording& other)
{
	getCurRecording().splitTo(other.getCurRecording());
}

F64 PeriodicRecording::getPeriodMin( const StatType<EventAccumulator>& stat, S32 num_periods /*= S32_MAX*/ )
{
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
	num_periods = llmin(num_periods, getNumRecordedPeriods());

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
		mPlayState = STARTED;
		break;
	case PAUSED:
		handleStart();
		mPlayState = STARTED;
		break;
	case STARTED:
		break;
	default:
		llassert(false);
		break;
	}
}

void LLStopWatchControlsMixinCommon::stop()
{
	switch (mPlayState)
	{
	case STOPPED:
		break;
	case PAUSED:
		mPlayState = STOPPED;
		break;
	case STARTED:
		handleStop();
		mPlayState = STOPPED;
		break;
	default:
		llassert(false);
		break;
	}
}

void LLStopWatchControlsMixinCommon::pause()
{
	switch (mPlayState)
	{
	case STOPPED:
		// stay stopped, don't go to pause
		break;
	case PAUSED:
		break;
	case STARTED:
		handleStop();
		mPlayState = PAUSED;
		break;
	default:
		llassert(false);
		break;
	}
}

void LLStopWatchControlsMixinCommon::unpause()
{
	switch (mPlayState)
	{
	case STOPPED:
		// stay stopped, don't start
		break;
	case PAUSED:
		handleStart();
		mPlayState = STARTED;
		break;
	case STARTED:
		break;
	default:
		llassert(false);
		break;
	}
}

void LLStopWatchControlsMixinCommon::resume()
{
	switch (mPlayState)
	{
	case STOPPED:
		handleStart();
		mPlayState = STARTED;
		break;
	case PAUSED:
		handleStart();
		mPlayState = STARTED;
		break;
	case STARTED:
		break;
	default:
		llassert(false);
		break;
	}
}

void LLStopWatchControlsMixinCommon::restart()
{
	switch (mPlayState)
	{
	case STOPPED:
		handleReset();
		handleStart();
		mPlayState = STARTED;
		break;
	case PAUSED:
		handleReset();
		handleStart();
		mPlayState = STARTED;
		break;
	case STARTED:
		handleReset();
		break;
	default:
		llassert(false);
		break;
	}
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
