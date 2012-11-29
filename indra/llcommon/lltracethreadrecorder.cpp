/** 
 * @file lltracethreadrecorder.cpp
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

#include "lltracethreadrecorder.h"
#include "llfasttimer.h"

namespace LLTrace
{


///////////////////////////////////////////////////////////////////////
// ThreadRecorder
///////////////////////////////////////////////////////////////////////

ThreadRecorder::ThreadRecorder()
{
	get_thread_recorder() = this;
	mFullRecording.start();

	mRootTimerData = new CurTimerData();
	mRootTimerData->mTimerData = &BlockTimer::getRootTimer();
	mRootTimerData->mTimerTreeData = new TimerTreeNode[AccumulatorBuffer<TimerAccumulator>::getDefaultBuffer().size()];
	BlockTimer::sCurTimerData = mRootTimerData;

	mRootTimer = new Time(BlockTimer::getRootTimer());
	mRootTimerData->mCurTimer = mRootTimer;

	mRootTimerData->mTimerTreeData[BlockTimer::getRootTimer().getIndex()].mActiveCount = 1;
}

ThreadRecorder::~ThreadRecorder()
{
	delete mRootTimer;

	while(mActiveRecordings.size())
	{
		mActiveRecordings.front().mTargetRecording->stop();
	}
	get_thread_recorder() = NULL;
	BlockTimer::sCurTimerData = NULL;
	delete [] mRootTimerData->mTimerTreeData;
	delete mRootTimerData;
}

void ThreadRecorder::activate( Recording* recording )
{
	mActiveRecordings.push_front(ActiveRecording(recording));
	mActiveRecordings.front().mBaseline.makePrimary();
}

std::list<ThreadRecorder::ActiveRecording>::iterator ThreadRecorder::update( Recording* recording )
{
	std::list<ActiveRecording>::iterator it, end_it;
	for (it = mActiveRecordings.begin(), end_it = mActiveRecordings.end();
		it != end_it;
		++it)
	{
		std::list<ActiveRecording>::iterator next_it = it;
		++next_it;

		// if we have another recording further down in the stack...
		if (next_it != mActiveRecordings.end())
		{
			// ...push our gathered data down to it
			next_it->mBaseline.appendRecording(it->mBaseline);
		}

		// copy accumulated measurements into result buffer and clear accumulator (mBaseline)
		it->moveBaselineToTarget();

		if (it->mTargetRecording == recording)
		{
			// found the recording, so return it
			break;
		}
	}

	if (it == end_it)
	{
		llerrs << "Recording not active on this thread" << llendl;
	}

	return it;
}

void ThreadRecorder::deactivate( Recording* recording )
{
	std::list<ActiveRecording>::iterator it = update(recording);
	if (it != mActiveRecordings.end())
	{
		// and if we've found the recording we wanted to update
		std::list<ActiveRecording>::iterator next_it = it;
		++next_it;
		if (next_it != mActiveRecordings.end())
		{
			next_it->mTargetRecording->makePrimary();
		}

		mActiveRecordings.erase(it);
	}
}

ThreadRecorder::ActiveRecording::ActiveRecording( Recording* target ) 
:	mTargetRecording(target)
{
}

void ThreadRecorder::ActiveRecording::moveBaselineToTarget()
{
	mTargetRecording->mMeasurementsFloat.write()->addSamples(*mBaseline.mMeasurementsFloat);
	mTargetRecording->mCountsFloat.write()->addSamples(*mBaseline.mCountsFloat);
	mTargetRecording->mMeasurements.write()->addSamples(*mBaseline.mMeasurements);
	mTargetRecording->mCounts.write()->addSamples(*mBaseline.mCounts);
	mTargetRecording->mStackTimers.write()->addSamples(*mBaseline.mStackTimers);
	mBaseline.mMeasurementsFloat.write()->reset();
	mBaseline.mCountsFloat.write()->reset();
	mBaseline.mMeasurements.write()->reset();
	mBaseline.mCounts.write()->reset();
	mBaseline.mStackTimers.write()->reset();
}


///////////////////////////////////////////////////////////////////////
// SlaveThreadRecorder
///////////////////////////////////////////////////////////////////////

SlaveThreadRecorder::SlaveThreadRecorder()
{
	getMasterThreadRecorder().addSlaveThread(this);
}

SlaveThreadRecorder::~SlaveThreadRecorder()
{
	getMasterThreadRecorder().removeSlaveThread(this);
}

void SlaveThreadRecorder::pushToMaster()
{
	mFullRecording.stop();
	{
		LLMutexLock(getMasterThreadRecorder().getSlaveListMutex());
		mSharedData.copyFrom(mFullRecording);
	}
	mFullRecording.start();
}

void SlaveThreadRecorder::SharedData::copyFrom( const Recording& source )
{
	LLMutexLock lock(&mRecordingMutex);
	mRecording.appendRecording(source);
}

void SlaveThreadRecorder::SharedData::copyTo( Recording& sink )
{
	LLMutexLock lock(&mRecordingMutex);
	sink.appendRecording(mRecording);
}

///////////////////////////////////////////////////////////////////////
// MasterThreadRecorder
///////////////////////////////////////////////////////////////////////

LLFastTimer::DeclareTimer FTM_PULL_TRACE_DATA_FROM_SLAVES("Pull slave trace data");
void MasterThreadRecorder::pullFromSlaveThreads()
{
	LLFastTimer _(FTM_PULL_TRACE_DATA_FROM_SLAVES);
	if (mActiveRecordings.empty()) return;

	LLMutexLock lock(&mSlaveListMutex);

	Recording& target_recording = mActiveRecordings.front().mBaseline;
	for (slave_thread_recorder_list_t::iterator it = mSlaveThreadRecorders.begin(), end_it = mSlaveThreadRecorders.end();
		it != end_it;
		++it)
	{
		(*it)->mSharedData.copyTo(target_recording);
	}
}

void MasterThreadRecorder::addSlaveThread( class SlaveThreadRecorder* child )
{
	LLMutexLock lock(&mSlaveListMutex);

	mSlaveThreadRecorders.push_back(child);
}

void MasterThreadRecorder::removeSlaveThread( class SlaveThreadRecorder* child )
{
	LLMutexLock lock(&mSlaveListMutex);

	for (slave_thread_recorder_list_t::iterator it = mSlaveThreadRecorders.begin(), end_it = mSlaveThreadRecorders.end();
		it != end_it;
		++it)
	{
		if ((*it) == child)
		{
			mSlaveThreadRecorders.erase(it);
			break;
		}
	}
}

void MasterThreadRecorder::pushToMaster()
{}

MasterThreadRecorder::MasterThreadRecorder()
{}

}
