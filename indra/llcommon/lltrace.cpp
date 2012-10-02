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
#include "lltracesampler.h"

namespace LLTrace
{

static MasterThreadTrace* gMasterThreadTrace = NULL;

void init()
{
	gMasterThreadTrace = new MasterThreadTrace();
}

void cleanup()
{
	delete gMasterThreadTrace;
	gMasterThreadTrace = NULL;
}

LLThreadLocalPointer<ThreadTrace>& get_thread_trace()
{
	static LLThreadLocalPointer<ThreadTrace> s_trace_data;
	return s_trace_data;

}

BlockTimer::Recorder::StackEntry BlockTimer::sCurRecorder;



MasterThreadTrace& getMasterThreadTrace()
{
	llassert(gMasterThreadTrace != NULL);
	return *gMasterThreadTrace;
}

///////////////////////////////////////////////////////////////////////
// MasterThreadTrace
///////////////////////////////////////////////////////////////////////

ThreadTrace::ThreadTrace()
{
	get_thread_trace() = this;
	mPrimarySampler.makePrimary();
	mTotalSampler.start();
}

ThreadTrace::ThreadTrace( const ThreadTrace& other ) 
:	mPrimarySampler(other.mPrimarySampler),
	mTotalSampler(other.mTotalSampler)
{
	get_thread_trace() = this;
	mPrimarySampler.makePrimary();
	mTotalSampler.start();
}

ThreadTrace::~ThreadTrace()
{
	get_thread_trace() = NULL;
}

//TODO: remove this and use llviewerstats sampler
Sampler* ThreadTrace::getPrimarySampler()
{
	return &mPrimarySampler;
}

void ThreadTrace::activate( Sampler* sampler )
{
	for (std::list<Sampler*>::iterator it = mActiveSamplers.begin(), end_it = mActiveSamplers.end();
		it != end_it;
		++it)
	{
		(*it)->mMeasurements.write()->mergeSamples(*mPrimarySampler.mMeasurements);
	}
	mPrimarySampler.mMeasurements.write()->reset();

	sampler->initDeltas(mPrimarySampler);

	mActiveSamplers.push_front(sampler);
}

//TODO: consider merging results down the list to one past the buffered item.
// this would require 2 buffers per sampler, to separate current total from running total

void ThreadTrace::deactivate( Sampler* sampler )
{
	sampler->mergeDeltas(mPrimarySampler);

	// TODO: replace with intrusive list
	std::list<Sampler*>::iterator found_it = std::find(mActiveSamplers.begin(), mActiveSamplers.end(), sampler);
	if (found_it != mActiveSamplers.end())
	{
		mActiveSamplers.erase(found_it);
	}
}

///////////////////////////////////////////////////////////////////////
// SlaveThreadTrace
///////////////////////////////////////////////////////////////////////

SlaveThreadTrace::SlaveThreadTrace()
:	ThreadTrace(getMasterThreadTrace())
{
	getMasterThreadTrace().addSlaveThread(this);
}

SlaveThreadTrace::~SlaveThreadTrace()
{
	getMasterThreadTrace().removeSlaveThread(this);
}

void SlaveThreadTrace::pushToMaster()
{
	mTotalSampler.stop();
	{
		LLMutexLock(getMasterThreadTrace().getSlaveListMutex());
		mSharedData.copyFrom(mTotalSampler);
	}
	mTotalSampler.start();
}

void SlaveThreadTrace::SharedData::copyFrom( const Sampler& source )
{
	LLMutexLock lock(&mSamplerMutex);
	mSampler.mergeSamples(source);
}

void SlaveThreadTrace::SharedData::copyTo( Sampler& sink )
{
	LLMutexLock lock(&mSamplerMutex);
	sink.mergeSamples(mSampler);
}




///////////////////////////////////////////////////////////////////////
// MasterThreadTrace
///////////////////////////////////////////////////////////////////////

void MasterThreadTrace::pullFromSlaveThreads()
{
	LLMutexLock lock(&mSlaveListMutex);

	for (slave_thread_trace_list_t::iterator it = mSlaveThreadTraces.begin(), end_it = mSlaveThreadTraces.end();
		it != end_it;
		++it)
	{
		(*it)->mSlaveTrace->mSharedData.copyTo((*it)->mSamplerStorage);
	}
}

void MasterThreadTrace::addSlaveThread( class SlaveThreadTrace* child )
{
	LLMutexLock lock(&mSlaveListMutex);

	mSlaveThreadTraces.push_back(new SlaveThreadTraceProxy(child));
}

void MasterThreadTrace::removeSlaveThread( class SlaveThreadTrace* child )
{
	LLMutexLock lock(&mSlaveListMutex);

	for (slave_thread_trace_list_t::iterator it = mSlaveThreadTraces.begin(), end_it = mSlaveThreadTraces.end();
		it != end_it;
		++it)
	{
		if ((*it)->mSlaveTrace == child)
		{
			mSlaveThreadTraces.erase(it);
			break;
		}
	}
}

void MasterThreadTrace::pushToMaster()
{}

MasterThreadTrace::MasterThreadTrace()
{}

///////////////////////////////////////////////////////////////////////
// MasterThreadTrace::SlaveThreadTraceProxy
///////////////////////////////////////////////////////////////////////

MasterThreadTrace::SlaveThreadTraceProxy::SlaveThreadTraceProxy( class SlaveThreadTrace* trace) 
:	mSlaveTrace(trace)
{}

}
