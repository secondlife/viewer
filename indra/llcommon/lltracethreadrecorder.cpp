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

	if (!mActiveRecordings.empty())
	{
		std::for_each(mActiveRecordings.begin(), mActiveRecordings.end(), DeletePointer());
		mActiveRecordings.clear();
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
		mActiveRecordings.back()->mPartialRecording.handOffTo(active_recording->mPartialRecording);
	}
	mActiveRecordings.push_back(active_recording);

	mActiveRecordings.back()->mPartialRecording.makePrimary();
}

ThreadRecorder::active_recording_list_t::reverse_iterator ThreadRecorder::bringUpToDate( Recording* recording )
{
	if (mActiveRecordings.empty()) return mActiveRecordings.rend();

	mActiveRecordings.back()->mPartialRecording.flush();

	active_recording_list_t::reverse_iterator it, end_it;
	for (it = mActiveRecordings.rbegin(), end_it = mActiveRecordings.rend();
		it != end_it;
		++it)
	{
		ActiveRecording* cur_recording = *it;

		active_recording_list_t::reverse_iterator next_it = it;
		++next_it;

		// if we have another recording further down in the stack...
		if (next_it != mActiveRecordings.rend())
		{
			// ...push our gathered data down to it
			(*next_it)->mPartialRecording.append(cur_recording->mPartialRecording);
		}

		// copy accumulated measurements into result buffer and clear accumulator (mPartialRecording)
		cur_recording->movePartialToTarget();

		if (cur_recording->mTargetRecording == recording)
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

void ThreadRecorder::deactivate( Recording* recording )
{
	active_recording_list_t::reverse_iterator it = bringUpToDate(recording);
	if (it != mActiveRecordings.rend())
	{
		// and if we've found the recording we wanted to update
		active_recording_list_t::reverse_iterator next_it = it;
		++next_it;
		if (next_it != mActiveRecordings.rend())
		{
			(*next_it)->mPartialRecording.makePrimary();
		}

		active_recording_list_t::iterator recording_to_remove = (++it).base();
		llassert((*recording_to_remove)->mTargetRecording == recording);
		delete *recording_to_remove;
		mActiveRecordings.erase(recording_to_remove);
	}
}

ThreadRecorder::ActiveRecording::ActiveRecording( Recording* target ) 
:	mTargetRecording(target)
{
}

void ThreadRecorder::ActiveRecording::movePartialToTarget()
{
	mTargetRecording->mBuffers.write()->append(mPartialRecording);
	// reset based on self to keep history
	mPartialRecording.reset(&mPartialRecording);
}


///////////////////////////////////////////////////////////////////////
// SlaveThreadRecorder
///////////////////////////////////////////////////////////////////////

SlaveThreadRecorder::SlaveThreadRecorder(MasterThreadRecorder& master)
:	mMasterRecorder(master)
{
	mMasterRecorder.addSlaveThread(this);
}

SlaveThreadRecorder::~SlaveThreadRecorder()
{
	mMasterRecorder.removeSlaveThread(this);
}

void SlaveThreadRecorder::pushToMaster()
{
	mThreadRecording.stop();
	{
		LLMutexLock(mMasterRecorder.getSlaveListMutex());
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

static LLFastTimer::DeclareTimer FTM_PULL_TRACE_DATA_FROM_SLAVES("Pull slave trace data");
void MasterThreadRecorder::pullFromSlaveThreads()
{
	LLFastTimer _(FTM_PULL_TRACE_DATA_FROM_SLAVES);
	if (mActiveRecordings.empty()) return;

	LLMutexLock lock(&mSlaveListMutex);

	RecordingBuffers& target_recording_buffers = mActiveRecordings.back()->mPartialRecording;
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
