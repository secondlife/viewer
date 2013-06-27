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
	mCounts.addSamples(other.mCounts);
	mSamples.addSamples(other.mSamples);
	mEvents.addSamples(other.mEvents);
	mMemStats.addSamples(other.mMemStats);
	mStackTimers.addSamples(other.mStackTimers);
}

void AccumulatorBufferGroup::merge( const AccumulatorBufferGroup& other)
{
	mCounts.addSamples(other.mCounts, false);
	mSamples.addSamples(other.mSamples, false);
	mEvents.addSamples(other.mEvents, false);
	mMemStats.addSamples(other.mMemStats, false);
	// for now, hold out timers from merge, need to be displayed per thread
	//mStackTimers.addSamples(other.mStackTimers, false);
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
	LLUnitImplicit<F64, LLUnits::Seconds> time_stamp = LLTimer::getTotalSeconds();

	mSamples.sync(time_stamp);
	mMemStats.sync(time_stamp);
}

}
