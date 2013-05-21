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
	set_thread_recorder(this);
	TimeBlock& root_time_block = TimeBlock::getRootTimeBlock();

	ThreadTimerStack* timer_stack = ThreadTimerStack::getInstance();
	timer_stack->mTimeBlock = &root_time_block;
	timer_stack->mActiveTimer = NULL;

	mNumTimeBlockTreeNodes = AccumulatorBuffer<TimeBlockAccumulator>::getDefaultBuffer()->size();
	mTimeBlockTreeNodes = new TimeBlockTreeNode[mNumTimeBlockTreeNodes];

	mThreadRecording.start();

	// initialize time block parent pointers
	for (LLInstanceTracker<TimeBlock>::instance_iter it = LLInstanceTracker<TimeBlock>::beginInstances(), end_it = LLInstanceTracker<TimeBlock>::endInstances(); 
		it != end_it; 
		++it)
	{
		TimeBlock& time_block = *it;
		TimeBlockTreeNode& tree_node = mTimeBlockTreeNodes[it->getIndex()];
		tree_node.mBlock = &time_block;
		tree_node.mParent = &root_time_block;

		it->getPrimaryAccumulator()->mParent = &root_time_block;
	}

	mRootTimer = new BlockTimer(root_time_block);
	timer_stack->mActiveTimer = mRootTimer;

	TimeBlock::getRootTimeBlock().getPrimaryAccumulator()->mActiveCount = 1;
}

ThreadRecorder::~ThreadRecorder()
{
	delete mRootTimer;

	while(mActiveRecordings.size())
	{
		mActiveRecordings.front()->mTargetRecording->stop();
	}
	set_thread_recorder(NULL);
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
	ActiveRecording* active_recording = new ActiveRecording(recording);
	if (!mActiveRecordings.empty())
	{
		mActiveRecordings.front()->mPartialRecording.handOffTo(active_recording->mPartialRecording);
	}
	mActiveRecordings.push_front(active_recording);

	mActiveRecordings.front()->mPartialRecording.makePrimary();
}

ThreadRecorder::active_recording_list_t::iterator ThreadRecorder::update( Recording* recording )
{
	active_recording_list_t::iterator it, end_it;
	for (it = mActiveRecordings.begin(), end_it = mActiveRecordings.end();
		it != end_it;
		++it)
	{
		active_recording_list_t::iterator next_it = it;
		++next_it;

		// if we have another recording further down in the stack...
		if (next_it != mActiveRecordings.end())
		{
			// ...push our gathered data down to it
			(*next_it)->mPartialRecording.append((*it)->mPartialRecording);
		}

		// copy accumulated measurements into result buffer and clear accumulator (mPartialRecording)
		(*it)->moveBaselineToTarget();

		if ((*it)->mTargetRecording == recording)
		{
			// found the recording, so return it
			break;
		}
	}

	if (it == end_it)
	{
		llwarns << "Recording not active on this thread" << llendl;
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
	active_recording_list_t::iterator it = update(recording);
	if (it != mActiveRecordings.end())
	{
		// and if we've found the recording we wanted to update
		active_recording_list_t::iterator next_it = it;
		++next_it;
		if (next_it != mActiveRecordings.end())
		{
			(*next_it)->mTargetRecording->mBuffers.write()->makePrimary();
		}

		delete *it;
		mActiveRecordings.erase(it);
	}
}

ThreadRecorder::ActiveRecording::ActiveRecording( Recording* target ) 
:	mTargetRecording(target)
{
}

void ThreadRecorder::ActiveRecording::moveBaselineToTarget()
{
	mTargetRecording->mBuffers.write()->append(mPartialRecording);
	mPartialRecording.reset();
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
	mThreadRecording.stop();
	{
		LLMutexLock(getMasterThreadRecorder().getSlaveListMutex());
		mSharedData.appendFrom(mThreadRecording);
	}
	mThreadRecording.start();
}

void SlaveThreadRecorder::SharedData::appendFrom( const Recording& source )
{
	LLMutexLock lock(&mRecordingMutex);
	appendRecording(source);
}

void SlaveThreadRecorder::SharedData::appendTo( Recording& sink )
{
	LLMutexLock lock(&mRecordingMutex);
	sink.appendRecording(*this);
}

void SlaveThreadRecorder::SharedData::mergeFrom( const RecordingBuffers& source )
{
	LLMutexLock lock(&mRecordingMutex);
	mBuffers.write()->merge(source);
}

void SlaveThreadRecorder::SharedData::mergeTo( RecordingBuffers& sink )
{
	LLMutexLock lock(&mRecordingMutex);
	sink.merge(*mBuffers);
}

void SlaveThreadRecorder::SharedData::reset()
{
	LLMutexLock lock(&mRecordingMutex);
	Recording::reset();
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

	RecordingBuffers& target_recording_buffers = mActiveRecordings.front()->mPartialRecording;
	for (slave_thread_recorder_list_t::iterator it = mSlaveThreadRecorders.begin(), end_it = mSlaveThreadRecorders.end();
		it != end_it;
		++it)
	{
		// ignore block timing info for now
		(*it)->mSharedData.mergeTo(target_recording_buffers);
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
