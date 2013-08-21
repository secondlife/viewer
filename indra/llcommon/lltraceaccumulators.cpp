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
#include "lltracethreadrecorder.h"

namespace LLTrace
{


///////////////////////////////////////////////////////////////////////
// AccumulatorBufferGroup
///////////////////////////////////////////////////////////////////////

AccumulatorBufferGroup::AccumulatorBufferGroup() 
{}

void AccumulatorBufferGroup::handOffTo(AccumulatorBufferGroup& other)
{
	other.mCounts.reset(&mCounts);
	other.mSamples.reset(&mSamples);
	other.mEvents.reset(&mEvents);
	other.mStackTimers.reset(&mStackTimers);
	other.mMemStats.reset(&mMemStats);
}

void AccumulatorBufferGroup::makePrimary()
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

//static
void AccumulatorBufferGroup::clearPrimary()
{
	AccumulatorBuffer<CountAccumulator>::clearPrimary();	
	AccumulatorBuffer<SampleAccumulator>::clearPrimary();
	AccumulatorBuffer<EventAccumulator>::clearPrimary();
	AccumulatorBuffer<TimeBlockAccumulator>::clearPrimary();
	AccumulatorBuffer<MemStatAccumulator>::clearPrimary();
}

bool AccumulatorBufferGroup::isPrimary() const
{
	return mCounts.isPrimary();
}

void AccumulatorBufferGroup::append( const AccumulatorBufferGroup& other )
{
	mCounts.addSamples(other.mCounts, SEQUENTIAL);
	mSamples.addSamples(other.mSamples, SEQUENTIAL);
	mEvents.addSamples(other.mEvents, SEQUENTIAL);
	mMemStats.addSamples(other.mMemStats, SEQUENTIAL);
	mStackTimers.addSamples(other.mStackTimers, SEQUENTIAL);
}

void AccumulatorBufferGroup::merge( const AccumulatorBufferGroup& other)
{
	mCounts.addSamples(other.mCounts, NON_SEQUENTIAL);
	mSamples.addSamples(other.mSamples, NON_SEQUENTIAL);
	mEvents.addSamples(other.mEvents, NON_SEQUENTIAL);
	mMemStats.addSamples(other.mMemStats, NON_SEQUENTIAL);
	// for now, hold out timers from merge, need to be displayed per thread
	//mStackTimers.addSamples(other.mStackTimers, NON_SEQUENTIAL);
}

void AccumulatorBufferGroup::reset(AccumulatorBufferGroup* other)
{
	mCounts.reset(other ? &other->mCounts : NULL);
	mSamples.reset(other ? &other->mSamples : NULL);
	mEvents.reset(other ? &other->mEvents : NULL);
	mStackTimers.reset(other ? &other->mStackTimers : NULL);
	mMemStats.reset(other ? &other->mMemStats : NULL);
}

void AccumulatorBufferGroup::sync()
{
	if (isPrimary())
	{
		F64SecondsImplicit time_stamp = LLTimer::getTotalSeconds();

		mSamples.sync(time_stamp);
		mMemStats.sync(time_stamp);
	}
}

void SampleAccumulator::addSamples( const SampleAccumulator& other, EBufferAppendType append_type )
{
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

		F64 epsilon = 0.0000001;

		if (other.mTotalSamplingTime > epsilon)
		{
			llassert(mTotalSamplingTime > 0);
			// combine variance (and hence standard deviation) of 2 different sized sample groups using
			// the following formula: http://www.mrc-bsu.cam.ac.uk/cochrane/handbook/chapter_7/7_7_3_8_combining_groups.htm
			F64 n_1 = mTotalSamplingTime,
				n_2 = other.mTotalSamplingTime;
			F64 m_1 = mMean,
				m_2 = other.mMean;
			F64 v_1 = mSumOfSquares / mTotalSamplingTime,
				v_2 = other.mSumOfSquares / other.mTotalSamplingTime;
			if (n_1 < epsilon)
			{
				mSumOfSquares = other.mSumOfSquares;
			}
			else
			{
				mSumOfSquares =	mTotalSamplingTime
					* ((((n_1 - epsilon) * v_1)
					+ ((n_2 - epsilon) * v_2)
					+ (((n_1 * n_2) / (n_1 + n_2))
					* ((m_1 * m_1) + (m_2 * m_2) - (2.f * m_1 * m_2))))
					/ (n_1 + n_2 - epsilon));
			}

			F64 weight = mTotalSamplingTime / (mTotalSamplingTime + other.mTotalSamplingTime);
			mNumSamples += other.mNumSamples;
			mTotalSamplingTime += other.mTotalSamplingTime;
			mMean = (mMean * weight) + (other.mMean * (1.0 - weight));
			llassert(mMean < 0 || mMean >= 0);
		}
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
	LL_ERRS_IF(mHasValue && !(mMean < 0) && !(mMean >= 0)) << "Invalid mean after capturing value" << LL_ENDL;
	mSumOfSquares = 0;
	mLastSampleTimeStamp = LLTimer::getTotalSeconds();
	mTotalSamplingTime = 0;
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

			// combine variance (and hence standard deviation) of 2 different sized sample groups using
			// the following formula: http://www.mrc-bsu.cam.ac.uk/cochrane/handbook/chapter_7/7_7_3_8_combining_groups.htm
			F64 n_1 = (F64)mNumSamples,
				n_2 = (F64)other.mNumSamples;
			F64 m_1 = mMean,
				m_2 = other.mMean;
			F64 v_1 = mSumOfSquares / mNumSamples,
				v_2 = other.mSumOfSquares / other.mNumSamples;
			mSumOfSquares = (F64)mNumSamples
				* ((((n_1 - 1.f) * v_1)
				+ ((n_2 - 1.f) * v_2)
				+ (((n_1 * n_2) / (n_1 + n_2))
				* ((m_1 * m_1) + (m_2 * m_2) - (2.f * m_1 * m_2))))
				/ (n_1 + n_2 - 1.f));

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
	mSum = NaN;
	mMin = NaN;
	mMax = NaN;
	mMean = NaN;
	mSumOfSquares = 0;
	mLastValue = other ? other->mLastValue : NaN;
}


}
