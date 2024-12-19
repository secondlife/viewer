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

///////////////////////////////////////////////////////////////////////
// Recording
///////////////////////////////////////////////////////////////////////

Recording::Recording(EPlayState state)
:   mElapsedSeconds(0),
    mActiveBuffers(NULL)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mBuffers = new AccumulatorBufferGroup();
    setPlayState(state);
}

Recording::Recording( const Recording& other )
:   mActiveBuffers(NULL)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    *this = other;
}

Recording& Recording::operator = (const Recording& other)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;

    // allow recording destruction without thread recorder running,
    // otherwise thread shutdown could crash if a recording outlives the thread recorder
    // besides, recording construction and destruction is fine without a recorder...just don't attempt to start one
    if (isStarted() && LLTrace::get_thread_recorder() != NULL)
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
        LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
        mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();

        // must have
        llassert(mActiveBuffers != NULL
                && LLTrace::get_thread_recorder() != NULL);

        if (!mActiveBuffers->isCurrent() && LLTrace::get_thread_recorder() != NULL)
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
#if LL_TRACE_ENABLED
    mBuffers.write()->reset();

    mElapsedSeconds = F64Seconds(0.0);
    mSamplingTimer.reset();
#endif
}

void Recording::handleStart()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
#if LL_TRACE_ENABLED
    mSamplingTimer.reset();
    mBuffers.setStayUnique(true);
    // must have thread recorder running on this thread
    llassert(LLTrace::get_thread_recorder() != NULL);
    mActiveBuffers = LLTrace::get_thread_recorder()->activate(mBuffers.write());
#endif
}

void Recording::handleStop()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
#if LL_TRACE_ENABLED
    mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();
    // must have thread recorder running on this thread
    llassert(LLTrace::get_thread_recorder() != NULL);
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
    return (F32)(accumulator.mCalls + (active_accumulator ? active_accumulator->mCalls : 0)) / (F32)mElapsedSeconds.value();
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
        F64 t = 0.0;
        S32 div = accumulator.getSampleCount() + active_accumulator->getSampleCount();
        if (div > 0)
        {
            t = (F64)active_accumulator->getSampleCount() / (F64)div;
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
        return sqrt(sum_of_squares / (F64)(accumulator.getSamplingTime() + active_accumulator->getSamplingTime()));
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
        F64 t = 0.0;
        S32 div = accumulator.getSampleCount() + active_accumulator->getSampleCount();
        if (div > 0)
        {
            t = (F64)active_accumulator->getSampleCount() / (F64)div;
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
        return sqrt(sum_of_squares / (F64)(accumulator.getSampleCount() + active_accumulator->getSampleCount()));
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

PeriodicRecording::PeriodicRecording( size_t num_periods, EPlayState state)
:   mAutoResize(num_periods == 0),
    mCurPeriod(0),
    mNumRecordedPeriods(0),
    // This guarantee that mRecordingPeriods cannot be empty is essential for
    // code in several methods.
    mRecordingPeriods(num_periods ? num_periods : 1)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    setPlayState(state);
}

PeriodicRecording::~PeriodicRecording()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
}

void PeriodicRecording::nextPeriod()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    if (mAutoResize)
    {
        mRecordingPeriods.push_back(Recording());
    }

    Recording& old_recording = getCurRecording();
    inci(mCurPeriod);
    old_recording.splitTo(getCurRecording());

    // Since mRecordingPeriods always has at least one entry, we can always
    // safely subtract 1 from its size().
    mNumRecordedPeriods = llmin(mRecordingPeriods.size() - 1, mNumRecordedPeriods + 1);
}

void PeriodicRecording::appendRecording(Recording& recording)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    getCurRecording().appendRecording(recording);
    nextPeriod();
}

void PeriodicRecording::appendPeriodicRecording( PeriodicRecording& other )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    if (other.mRecordingPeriods.empty()) return;

    getCurRecording().update();
    other.getCurRecording().update();

    const auto other_num_recordings = other.getNumRecordedPeriods();
    const auto other_current_recording_index = other.mCurPeriod;
    const auto other_oldest_recording_index = other.previ(other_current_recording_index, other_num_recordings);

    // append first recording into our current slot
    getCurRecording().appendRecording(other.mRecordingPeriods[other_oldest_recording_index]);

    // from now on, add new recordings for everything after the first
    auto other_index = other.nexti(other_oldest_recording_index);

    if (mAutoResize)
    {
        // push back recordings for everything in the middle
        while (other_index != other_current_recording_index)
        {
            mRecordingPeriods.push_back(other.mRecordingPeriods[other_index]);
            other.inci(other_index);
        }

        // add final recording, if it wasn't already added as the first
        if (other_num_recordings > 1)
        {
            mRecordingPeriods.push_back(other.mRecordingPeriods[other_current_recording_index]);
        }

        // mRecordingPeriods is never empty()
        mCurPeriod = mRecordingPeriods.size() - 1;
        mNumRecordedPeriods = mCurPeriod;
    }
    else
    {
        auto num_to_copy = llmin(mRecordingPeriods.size(), other_num_recordings);
        // already consumed the first recording from other, so start counting at 1
        for (size_t n = 1, srci = other_index, dsti = mCurPeriod;
             n < num_to_copy;
             ++n, other.inci(srci), inci(dsti))
        {
            mRecordingPeriods[dsti] = other.mRecordingPeriods[srci];
        }

        // want argument to % to be positive, otherwise result could be negative and thus out of bounds
        llassert(num_to_copy >= 1);
        // advance to last recording period copied, and make that our current period
        inci(mCurPeriod, num_to_copy - 1);
        mNumRecordedPeriods = llmin(mRecordingPeriods.size() - 1, mNumRecordedPeriods + num_to_copy - 1);
    }

    // end with fresh period, otherwise next appendPeriodicRecording() will merge the first
    // recording period with the last one appended here
    nextPeriod();
    getCurRecording().setPlayState(getPlayState());
}

F64Seconds PeriodicRecording::getDuration() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    F64Seconds duration;
    for (size_t n = 0; n < mRecordingPeriods.size(); ++n)
    {
        duration += mRecordingPeriods[nexti(mCurPeriod, n)].getDuration();
    }
    return duration;
}

LLTrace::Recording PeriodicRecording::snapshotCurRecording() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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

Recording& PeriodicRecording::getPrevRecording( size_t offset )
{
    // reuse const implementation, but return non-const reference
    return const_cast<Recording&>(
        const_cast<const PeriodicRecording*>(this)->getPrevRecording(offset));
}

const Recording& PeriodicRecording::getPrevRecording( size_t offset ) const
{
    return mRecordingPeriods[previ(mCurPeriod, offset)];
}

void PeriodicRecording::handleStart()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    getCurRecording().start();
}

void PeriodicRecording::handleStop()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    getCurRecording().pause();
}

void PeriodicRecording::handleReset()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    getCurRecording().stop();

    if (mAutoResize)
    {
        mRecordingPeriods.clear();
        mRecordingPeriods.push_back(Recording());
    }
    else
    {
        for (Recording& rec : mRecordingPeriods)
        {
            rec.reset();
        }
    }
    mCurPeriod = 0;
    mNumRecordedPeriods = 0;
    getCurRecording().setPlayState(getPlayState());
}

void PeriodicRecording::handleSplitTo(PeriodicRecording& other)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    getCurRecording().splitTo(other.getCurRecording());
}

F64 PeriodicRecording::getPeriodMin( const StatType<EventAccumulator>& stat, size_t num_periods /*= std::numeric_limits<size_t>::max()*/ )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    num_periods = llmin(num_periods, getNumRecordedPeriods());

    bool has_value = false;
    F64 min_val = std::numeric_limits<F64>::max();
    for (size_t i = 1; i <= num_periods; i++)
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

F64 PeriodicRecording::getPeriodMax( const StatType<EventAccumulator>& stat, size_t num_periods /*= std::numeric_limits<size_t>::max()*/ )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    num_periods = llmin(num_periods, getNumRecordedPeriods());

    bool has_value = false;
    F64 max_val = std::numeric_limits<F64>::min();
    for (size_t i = 1; i <= num_periods; i++)
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
F64 PeriodicRecording::getPeriodMean( const StatType<EventAccumulator>& stat, size_t num_periods /*= std::numeric_limits<size_t>::max()*/ )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    num_periods = llmin(num_periods, getNumRecordedPeriods());

    F64 mean = 0;
    S32 valid_period_count = 0;

    for (size_t i = 1; i <= num_periods; i++)
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

F64 PeriodicRecording::getPeriodStandardDeviation( const StatType<EventAccumulator>& stat, size_t num_periods /*= std::numeric_limits<size_t>::max()*/ )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    num_periods = llmin(num_periods, getNumRecordedPeriods());

    F64 period_mean = getPeriodMean(stat, num_periods);
    F64 sum_of_squares = 0;
    S32 valid_period_count = 0;

    for (size_t i = 1; i <= num_periods; i++)
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

F64 PeriodicRecording::getPeriodMin( const StatType<SampleAccumulator>& stat, size_t num_periods /*= std::numeric_limits<size_t>::max()*/ )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    num_periods = llmin(num_periods, getNumRecordedPeriods());

    bool has_value = false;
    F64 min_val = std::numeric_limits<F64>::max();
    for (size_t i = 1; i <= num_periods; i++)
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

F64 PeriodicRecording::getPeriodMax(const StatType<SampleAccumulator>& stat, size_t num_periods /*= std::numeric_limits<size_t>::max()*/)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    num_periods = llmin(num_periods, getNumRecordedPeriods());

    bool has_value = false;
    F64 max_val = std::numeric_limits<F64>::min();
    for (size_t i = 1; i <= num_periods; i++)
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


F64 PeriodicRecording::getPeriodMean( const StatType<SampleAccumulator>& stat, size_t num_periods /*= std::numeric_limits<size_t>::max()*/ )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    num_periods = llmin(num_periods, getNumRecordedPeriods());

    S32 valid_period_count = 0;
    F64 mean = 0;

    for (size_t i = 1; i <= num_periods; i++)
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

F64 PeriodicRecording::getPeriodMedian( const StatType<SampleAccumulator>& stat, size_t num_periods /*= std::numeric_limits<size_t>::max()*/ )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    num_periods = llmin(num_periods, getNumRecordedPeriods());

    std::vector<F64> buf;
    for (size_t i = 1; i <= num_periods; i++)
    {
        Recording& recording = getPrevRecording(i);
        if (recording.getDuration() > (F32Seconds)0.f)
        {
            if (recording.hasValue(stat))
            {
                buf.push_back(recording.getMean(stat));
            }
        }
    }
    if (buf.size()==0)
    {
        return 0.0f;
    }
    std::sort(buf.begin(), buf.end());

    return F64((buf.size() % 2 == 0) ? (buf[buf.size() / 2 - 1] + buf[buf.size() / 2]) / 2 : buf[buf.size() / 2]);
}

F64 PeriodicRecording::getPeriodStandardDeviation( const StatType<SampleAccumulator>& stat, size_t num_periods /*= std::numeric_limits<size_t>::max()*/ )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    num_periods = llmin(num_periods, getNumRecordedPeriods());

    F64 period_mean = getPeriodMean(stat, num_periods);
    S32 valid_period_count = 0;
    F64 sum_of_squares = 0;

    for (size_t i = 1; i <= num_periods; i++)
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

///////////////////////////////////////////////////////////////////////
// ExtendableRecording
///////////////////////////////////////////////////////////////////////

void ExtendableRecording::extend()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    // push the data back to accepted recording
    mAcceptedRecording.appendRecording(mPotentialRecording);
    // flush data, so we can start from scratch
    mPotentialRecording.reset();
}

void ExtendableRecording::handleStart()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mPotentialRecording.start();
}

void ExtendableRecording::handleStop()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mPotentialRecording.pause();
}

void ExtendableRecording::handleReset()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mAcceptedRecording.reset();
    mPotentialRecording.reset();
}

void ExtendableRecording::handleSplitTo(ExtendableRecording& other)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mPotentialRecording.splitTo(other.mPotentialRecording);
}

///////////////////////////////////////////////////////////////////////
// ExtendablePeriodicRecording
///////////////////////////////////////////////////////////////////////

ExtendablePeriodicRecording::ExtendablePeriodicRecording()
:   mAcceptedRecording(0),
    mPotentialRecording(0)
{}

void ExtendablePeriodicRecording::extend()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    // push the data back to accepted recording
    mAcceptedRecording.appendPeriodicRecording(mPotentialRecording);
    // flush data, so we can start from scratch
    mPotentialRecording.reset();
}

void ExtendablePeriodicRecording::handleStart()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mPotentialRecording.start();
}

void ExtendablePeriodicRecording::handleStop()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mPotentialRecording.pause();
}

void ExtendablePeriodicRecording::handleReset()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mAcceptedRecording.reset();
    mPotentialRecording.reset();
}

void ExtendablePeriodicRecording::handleSplitTo(ExtendablePeriodicRecording& other)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mPotentialRecording.splitTo(other.mPotentialRecording);
}

PeriodicRecording& get_frame_recording()
{
    static thread_local PeriodicRecording sRecording(200, PeriodicRecording::STARTED);
    return sRecording;
}

}

void LLStopWatchControlsMixinCommon::start()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    handleReset();
}

void LLStopWatchControlsMixinCommon::setPlayState( EPlayState state )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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
