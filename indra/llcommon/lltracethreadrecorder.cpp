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
	//NB: the ordering of initialization in this function is very fragile due to a large number of implicit dependencies
	get_thread_recorder() = this;

	mRootTimerData = new CurTimerData();
	mRootTimerData->mTimerData = &TimeBlock::getRootTimer();
	TimeBlock::sCurTimerData = mRootTimerData;

	mNumTimeBlockTreeNodes = AccumulatorBuffer<TimeBlockAccumulator>::getDefaultBuffer()->size();
	mTimeBlockTreeNodes = new TimeBlockTreeNode[mNumTimeBlockTreeNodes];

	mFullRecording.start();

	TimeBlock& root_timer = TimeBlock::getRootTimer();

	// initialize time block parent pointers
	for (LLInstanceTracker<TimeBlock>::instance_iter it = LLInstanceTracker<TimeBlock>::beginInstances(), end_it = LLInstanceTracker<TimeBlock>::endInstances(); 
		it != end_it; 
		++it)
	{
		TimeBlock& time_block = *it;
		TimeBlockTreeNode& tree_node = mTimeBlockTreeNodes[it->getIndex()];
		tree_node.mBlock = &time_block;
		tree_node.mParent = &root_timer;

		it->getPrimaryAccumulator()->mParent = &root_timer;
	}

	mRootTimer = new BlockTimer(root_timer);
	mRootTimerData->mCurTimer = mRootTimer;

	TimeBlock::getRootTimer().getPrimaryAccumulator()->mActiveCount = 1;
}

ThreadRecorder::~ThreadRecorder()
{
	delete mRootTimer;

	while(mActiveRecordings.size())
	{
		mActiveRecordings.front().mTargetRecording->stop();
	}
	get_thread_recorder() = NULL;
	TimeBlock::sCurTimerData = NULL;
	delete mRootTimerData;
	delete[] mTimeBlockTreeNodes;
}

TimeBlockTreeNode* ThreadRecorder::getTimeBlockTreeNode(S32 index)
{
	if (0 <= index && index < mNumTimeBlockTreeNodes)
	{
		return &mTimeBlockTreeNodes[index];
	}
	return NULL;
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

AccumulatorBuffer<CountAccumulator<F64> > 		gCountsFloat;
AccumulatorBuffer<MeasurementAccumulator<F64> >	gMeasurementsFloat;
AccumulatorBuffer<CountAccumulator<S64> >		gCounts;
AccumulatorBuffer<MeasurementAccumulator<S64> >	gMeasurements;
AccumulatorBuffer<TimeBlockAccumulator>			gStackTimers;
AccumulatorBuffer<MemStatAccumulator>			gMemStats;

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
		mSharedData.appendFrom(mFullRecording);
	}
	mFullRecording.start();
}

void SlaveThreadRecorder::SharedData::appendFrom( const Recording& source )
{
	LLMutexLock lock(&mRecordingMutex);
	mRecording.appendRecording(source);
}

void SlaveThreadRecorder::SharedData::appendTo( Recording& sink )
{
	LLMutexLock lock(&mRecordingMutex);
	sink.appendRecording(mRecording);
}

void SlaveThreadRecorder::SharedData::mergeFrom( const Recording& source )
{
	LLMutexLock lock(&mRecordingMutex);
	mRecording.mergeRecording(source);
}

void SlaveThreadRecorder::SharedData::mergeTo( Recording& sink )
{
	LLMutexLock lock(&mRecordingMutex);
	sink.mergeRecording(mRecording);
}

void SlaveThreadRecorder::SharedData::reset()
{
	LLMutexLock lock(&mRecordingMutex);
	mRecording.reset();
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
		// ignore block timing info for now
		(*it)->mSharedData.mergeTo(target_recording);
		(*it)->mSharedData.reset();
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
