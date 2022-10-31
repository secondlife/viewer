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

#include "lltraceaccumulators.h"
#include "lltrace.h"
#include "lltracethreadrecorder.h"

namespace LLTrace
{

extern MemStatHandle gTraceMemStat;


///////////////////////////////////////////////////////////////////////
// AccumulatorBufferGroup
///////////////////////////////////////////////////////////////////////

AccumulatorBufferGroup::AccumulatorBufferGroup() 
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    claim_alloc(gTraceMemStat, mCounts.capacity() * sizeof(CountAccumulator));
    claim_alloc(gTraceMemStat, mSamples.capacity() * sizeof(SampleAccumulator));
    claim_alloc(gTraceMemStat, mEvents.capacity() * sizeof(EventAccumulator));
    claim_alloc(gTraceMemStat, mStackTimers.capacity() * sizeof(TimeBlockAccumulator));
    claim_alloc(gTraceMemStat, mMemStats.capacity() * sizeof(MemAccumulator));
}

AccumulatorBufferGroup::AccumulatorBufferGroup(const AccumulatorBufferGroup& other)
:   mCounts(other.mCounts),
    mSamples(other.mSamples),
    mEvents(other.mEvents),
    mStackTimers(other.mStackTimers),
    mMemStats(other.mMemStats)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    claim_alloc(gTraceMemStat, mCounts.capacity() * sizeof(CountAccumulator));
    claim_alloc(gTraceMemStat, mSamples.capacity() * sizeof(SampleAccumulator));
    claim_alloc(gTraceMemStat, mEvents.capacity() * sizeof(EventAccumulator));
    claim_alloc(gTraceMemStat, mStackTimers.capacity() * sizeof(TimeBlockAccumulator));
    claim_alloc(gTraceMemStat, mMemStats.capacity() * sizeof(MemAccumulator));
}

AccumulatorBufferGroup::~AccumulatorBufferGroup()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    disclaim_alloc(gTraceMemStat, mCounts.capacity() * sizeof(CountAccumulator));
    disclaim_alloc(gTraceMemStat, mSamples.capacity() * sizeof(SampleAccumulator));
    disclaim_alloc(gTraceMemStat, mEvents.capacity() * sizeof(EventAccumulator));
    disclaim_alloc(gTraceMemStat, mStackTimers.capacity() * sizeof(TimeBlockAccumulator));
    disclaim_alloc(gTraceMemStat, mMemStats.capacity() * sizeof(MemAccumulator));
}

void AccumulatorBufferGroup::handOffTo(AccumulatorBufferGroup& other)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    other.mCounts.reset(&mCounts);
    other.mSamples.reset(&mSamples);
    other.mEvents.reset(&mEvents);
    other.mStackTimers.reset(&mStackTimers);
    other.mMemStats.reset(&mMemStats);
}

void AccumulatorBufferGroup::makeCurrent()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mCounts.makeCurrent();
    mSamples.makeCurrent();
    mEvents.makeCurrent();
    mStackTimers.makeCurrent();
    mMemStats.makeCurrent();

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

//static
void AccumulatorBufferGroup::clearCurrent()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    AccumulatorBuffer<CountAccumulator>::clearCurrent();    
    AccumulatorBuffer<SampleAccumulator>::clearCurrent();
    AccumulatorBuffer<EventAccumulator>::clearCurrent();
    AccumulatorBuffer<TimeBlockAccumulator>::clearCurrent();
    AccumulatorBuffer<MemAccumulator>::clearCurrent();
}

bool AccumulatorBufferGroup::isCurrent() const
{
    return mCounts.isCurrent();
}

void AccumulatorBufferGroup::append( const AccumulatorBufferGroup& other )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mCounts.addSamples(other.mCounts, SEQUENTIAL);
    mSamples.addSamples(other.mSamples, SEQUENTIAL);
    mEvents.addSamples(other.mEvents, SEQUENTIAL);
    mMemStats.addSamples(other.mMemStats, SEQUENTIAL);
    mStackTimers.addSamples(other.mStackTimers, SEQUENTIAL);
}

void AccumulatorBufferGroup::merge( const AccumulatorBufferGroup& other)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mCounts.addSamples(other.mCounts, NON_SEQUENTIAL);
    mSamples.addSamples(other.mSamples, NON_SEQUENTIAL);
    mEvents.addSamples(other.mEvents, NON_SEQUENTIAL);
    mMemStats.addSamples(other.mMemStats, NON_SEQUENTIAL);
    // for now, hold out timers from merge, need to be displayed per thread
    //mStackTimers.addSamples(other.mStackTimers, NON_SEQUENTIAL);
}

void AccumulatorBufferGroup::reset(AccumulatorBufferGroup* other)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    mCounts.reset(other ? &other->mCounts : NULL);
    mSamples.reset(other ? &other->mSamples : NULL);
    mEvents.reset(other ? &other->mEvents : NULL);
    mStackTimers.reset(other ? &other->mStackTimers : NULL);
    mMemStats.reset(other ? &other->mMemStats : NULL);
}

void AccumulatorBufferGroup::sync()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    if (isCurrent())
    {
        F64SecondsImplicit time_stamp = LLTimer::getTotalSeconds();

        mSamples.sync(time_stamp);
        mMemStats.sync(time_stamp);
    }
}

F64 SampleAccumulator::mergeSumsOfSquares(const SampleAccumulator& a, const SampleAccumulator& b)
{
    const F64 epsilon = 0.0000001;

    if (a.getSamplingTime() > epsilon && b.getSamplingTime() > epsilon)
    {
        // combine variance (and hence standard deviation) of 2 different sized sample groups using
        // the following formula: http://www.mrc-bsu.cam.ac.uk/cochrane/handbook/chapter_7/7_7_3_8_combining_groups.htm
        F64 n_1 = a.getSamplingTime(),
            n_2 = b.getSamplingTime();
        F64 m_1 = a.getMean(),
            m_2 = b.getMean();
        F64 v_1 = a.getSumOfSquares() / a.getSamplingTime(),
            v_2 = b.getSumOfSquares() / b.getSamplingTime();
        if (n_1 < epsilon)
        {
            return b.getSumOfSquares();
        }
        else
        {
            return a.getSamplingTime()
                * ((((n_1 - epsilon) * v_1)
                + ((n_2 - epsilon) * v_2)
                + (((n_1 * n_2) / (n_1 + n_2))
                * ((m_1 * m_1) + (m_2 * m_2) - (2.f * m_1 * m_2))))
                / (n_1 + n_2 - epsilon));
        }
    }

    return a.getSumOfSquares();
}


void SampleAccumulator::addSamples( const SampleAccumulator& other, EBufferAppendType append_type )
{
    if (append_type == NON_SEQUENTIAL)
    {
        return;
    }

    if (!mHasValue)
    {
        *this = other;

        if (append_type == NON_SEQUENTIAL)
        {
            // restore own last value state
            mLastValue = NaN;
            mHasValue = false;
        }
    }
    else if (other.mHasValue)
    {
        mSum += other.mSum;

        if (other.mMin < mMin) { mMin = other.mMin; }
        if (other.mMax > mMax) { mMax = other.mMax; }

        mSumOfSquares = mergeSumsOfSquares(*this, other);

        if (append_type == SEQUENTIAL)
        {
            mLastValue = other.mLastValue;
            mLastSampleTimeStamp = other.mLastSampleTimeStamp;
        }
    }
}

void SampleAccumulator::reset( const SampleAccumulator* other )
{
    mLastValue = other ? other->mLastValue : NaN;
    mHasValue = other ? other->mHasValue : false;
    mNumSamples = 0;
    mSum = 0;
    mMin = mLastValue;
    mMax = mLastValue;
    mMean = mLastValue;
    llassert(!mHasValue || mMean < 0 || mMean >= 0);
    mSumOfSquares = 0;
    mLastSampleTimeStamp = LLTimer::getTotalSeconds();
    mTotalSamplingTime = 0;
}

F64 EventAccumulator::mergeSumsOfSquares(const EventAccumulator& a, const EventAccumulator& b)
{
    if (a.mNumSamples && b.mNumSamples)
    {
        // combine variance (and hence standard deviation) of 2 different sized sample groups using
        // the following formula: http://www.mrc-bsu.cam.ac.uk/cochrane/handbook/chapter_7/7_7_3_8_combining_groups.htm
        F64 n_1 = a.mNumSamples,
            n_2 = b.mNumSamples;
        F64 m_1 = a.mMean,
            m_2 = b.mMean;
        F64 v_1 = a.mSumOfSquares / a.mNumSamples,
            v_2 = b.mSumOfSquares / b.mNumSamples;
        return (F64)a.mNumSamples
            * ((((n_1 - 1.f) * v_1)
            + ((n_2 - 1.f) * v_2)
            + (((n_1 * n_2) / (n_1 + n_2))
            * ((m_1 * m_1) + (m_2 * m_2) - (2.f * m_1 * m_2))))
            / (n_1 + n_2 - 1.f));
    }

    return a.mSumOfSquares;
}

void EventAccumulator::addSamples( const EventAccumulator& other, EBufferAppendType append_type )
{
    if (other.mNumSamples)
    {
        if (!mNumSamples)
        {
            *this = other;
        }
        else
        {
            mSum += other.mSum;

            // NOTE: both conditions will hold first time through
            if (other.mMin < mMin) { mMin = other.mMin; }
            if (other.mMax > mMax) { mMax = other.mMax; }

            mSumOfSquares = mergeSumsOfSquares(*this, other);

            F64 weight = (F64)mNumSamples / (F64)(mNumSamples + other.mNumSamples);
            mNumSamples += other.mNumSamples;
            mMean = mMean * weight + other.mMean * (1.f - weight);
            if (append_type == SEQUENTIAL) mLastValue = other.mLastValue;
        }
    }
}

void EventAccumulator::reset( const EventAccumulator* other )
{
    mNumSamples = 0;
    mSum = 0;
    mMin = F32(NaN);
    mMax = F32(NaN);
    mMean = NaN;
    mSumOfSquares = 0;
    mLastValue = other ? other->mLastValue : NaN;
}


}
