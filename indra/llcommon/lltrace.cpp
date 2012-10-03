/** 
 * @file lltrace.cpp
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

#include "lltrace.h"
#include "lltracerecording.h"

namespace LLTrace
{

static MasterThreadRecorder* gMasterThreadRecorder = NULL;

void init()
{
	gMasterThreadRecorder = new MasterThreadRecorder();
}

void cleanup()
{
	delete gMasterThreadRecorder;
	gMasterThreadRecorder = NULL;
}

LLThreadLocalPointer<ThreadRecorder>& get_thread_recorder()
{
	static LLThreadLocalPointer<ThreadRecorder> s_thread_recorder;
	return s_thread_recorder;

}

BlockTimer::Recorder::StackEntry BlockTimer::sCurRecorder;



MasterThreadRecorder& getMasterThreadRecorder()
{
	llassert(gMasterThreadRecorder != NULL);
	return *gMasterThreadRecorder;
}

///////////////////////////////////////////////////////////////////////
// ThreadRecorder
///////////////////////////////////////////////////////////////////////

ThreadRecorder::ThreadRecorder()
:	mPrimaryRecording(NULL)
{
	get_thread_recorder() = this;
	mFullRecording.start();
}

ThreadRecorder::ThreadRecorder( const ThreadRecorder& other ) 
:	mFullRecording(other.mFullRecording),
	mPrimaryRecording(NULL)
{
	get_thread_recorder() = this;
	mFullRecording.start();
}

ThreadRecorder::~ThreadRecorder()
{
	get_thread_recorder() = NULL;
}

//TODO: remove this and use llviewerstats recording
Recording* ThreadRecorder::getPrimaryRecording()
{
	return mPrimaryRecording;
}

void ThreadRecorder::activate( Recording* recording )
{
	mActiveRecordings.push_front(ActiveRecording(mPrimaryRecording, recording));
	mActiveRecordings.front().mBaseline.makePrimary();
	mPrimaryRecording = &mActiveRecordings.front().mBaseline;
}

//TODO: consider merging results down the list to one past the buffered item.
// this would require 2 buffers per sampler, to separate current total from running total

void ThreadRecorder::deactivate( Recording* recording )
{
	for (std::list<ActiveRecording>::iterator it = mActiveRecordings.begin(), end_it = mActiveRecordings.end();
		it != end_it;
		++it)
	{
		std::list<ActiveRecording>::iterator next_it = it;
		if (++next_it != mActiveRecordings.end())
		{
			next_it->mergeMeasurements((*it));
		}

		it->flushAccumulators(mPrimaryRecording);

		if (it->mTargetRecording == recording)
		{
			if (next_it != mActiveRecordings.end())
			{
				next_it->mBaseline.makePrimary();
				mPrimaryRecording = &next_it->mBaseline;
			}
			mActiveRecordings.erase(it);
			break;
		}
	}
}

ThreadRecorder::ActiveRecording::ActiveRecording( Recording* source, Recording* target ) 
:	mTargetRecording(target)
{
	// take snapshots of current values rates and timers
	if (source)
	{
		mBaseline.mRates.write()->copyFrom(*source->mRates);
		mBaseline.mStackTimers.write()->copyFrom(*source->mStackTimers);
	}
}

void ThreadRecorder::ActiveRecording::mergeMeasurements(ThreadRecorder::ActiveRecording& other)
{
	mBaseline.mMeasurements.write()->mergeSamples(*other.mBaseline.mMeasurements);
}

void ThreadRecorder::ActiveRecording::flushAccumulators(Recording* current)
{
	// accumulate statistics-like measurements
	mTargetRecording->mMeasurements.write()->mergeSamples(*mBaseline.mMeasurements);
	// for rate-like measurements, merge total change since baseline
	mTargetRecording->mRates.write()->mergeDeltas(*mBaseline.mRates, *current->mRates);
	mTargetRecording->mStackTimers.write()->mergeDeltas(*mBaseline.mStackTimers, *current->mStackTimers);
	// reset baselines
	mBaseline.mRates.write()->copyFrom(*current->mRates);
	mBaseline.mStackTimers.write()->copyFrom(*current->mStackTimers);
}

///////////////////////////////////////////////////////////////////////
// SlaveThreadRecorder
///////////////////////////////////////////////////////////////////////

SlaveThreadRecorder::SlaveThreadRecorder()
:	ThreadRecorder(getMasterThreadRecorder())
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
	LLMutexLock lock(&mRecorderMutex);
	mRecorder.mergeSamples(source);
}

void SlaveThreadRecorder::SharedData::copyTo( Recording& sink )
{
	LLMutexLock lock(&mRecorderMutex);
	sink.mergeSamples(mRecorder);
}

///////////////////////////////////////////////////////////////////////
// MasterThreadRecorder
///////////////////////////////////////////////////////////////////////

void MasterThreadRecorder::pullFromSlaveThreads()
{
	LLMutexLock lock(&mSlaveListMutex);

	for (slave_thread_recorder_list_t::iterator it = mSlaveThreadRecorders.begin(), end_it = mSlaveThreadRecorders.end();
		it != end_it;
		++it)
	{
		(*it)->mRecorder->mSharedData.copyTo((*it)->mSlaveRecording);
	}
}

void MasterThreadRecorder::addSlaveThread( class SlaveThreadRecorder* child )
{
	LLMutexLock lock(&mSlaveListMutex);

	mSlaveThreadRecorders.push_back(new SlaveThreadRecorderProxy(child));
}

void MasterThreadRecorder::removeSlaveThread( class SlaveThreadRecorder* child )
{
	LLMutexLock lock(&mSlaveListMutex);

	for (slave_thread_recorder_list_t::iterator it = mSlaveThreadRecorders.begin(), end_it = mSlaveThreadRecorders.end();
		it != end_it;
		++it)
	{
		if ((*it)->mRecorder == child)
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

///////////////////////////////////////////////////////////////////////
// MasterThreadRecorder::SlaveThreadTraceProxy
///////////////////////////////////////////////////////////////////////

MasterThreadRecorder::SlaveThreadRecorderProxy::SlaveThreadRecorderProxy( class SlaveThreadRecorder* recorder) 
:	mRecorder(recorder)
{}

}
