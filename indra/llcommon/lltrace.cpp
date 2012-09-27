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
#include "llthread.h"

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


BlockTimer::Recorder::StackEntry BlockTimer::sCurRecorder;



MasterThreadTrace& getMasterThreadTrace()
{
	llassert(gMasterThreadTrace != NULL);
	return *gMasterThreadTrace;
}



///////////////////////////////////////////////////////////////////////
// Sampler
///////////////////////////////////////////////////////////////////////


void Sampler::stop()
{
	getThreadTrace()->deactivate(this);
}

void Sampler::resume()
{
	getThreadTrace()->activate(this);
}

class ThreadTrace* Sampler::getThreadTrace()
{
	return LLThread::getTraceData();
}

///////////////////////////////////////////////////////////////////////
// MasterThreadTrace
///////////////////////////////////////////////////////////////////////

ThreadTrace::ThreadTrace()
{
	mPrimarySampler.makePrimary();
}

ThreadTrace::ThreadTrace( const ThreadTrace& other ) :	mPrimarySampler(other.mPrimarySampler)
{
	mPrimarySampler.makePrimary();
}

void ThreadTrace::activate( Sampler* sampler )
{
	flushPrimary();
	mActiveSamplers.push_back(sampler);
}

void ThreadTrace::deactivate( Sampler* sampler )
{
	sampler->mergeFrom(mPrimarySampler);

	// TODO: replace with intrusive list
	std::list<Sampler*>::iterator found_it = std::find(mActiveSamplers.begin(), mActiveSamplers.end(), sampler);
	if (found_it != mActiveSamplers.end())
	{
		mActiveSamplers.erase(found_it);
	}
}

void ThreadTrace::flushPrimary()
{
	for (std::list<Sampler*>::iterator it = mActiveSamplers.begin(), end_it = mActiveSamplers.end();
		it != end_it;
		++it)
	{
		(*it)->mergeFrom(mPrimarySampler);
	}
	mPrimarySampler.reset();
}






///////////////////////////////////////////////////////////////////////
// SlaveThreadTrace
///////////////////////////////////////////////////////////////////////

void SlaveThreadTrace::pushToMaster()
{
	mSharedData.copyFrom(mPrimarySampler);
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
		it->mSlaveTrace->mSharedData.copyTo(it->mSamplerStorage);
	}
}

void MasterThreadTrace::addSlaveThread( class SlaveThreadTrace* child )
{
	LLMutexLock lock(&mSlaveListMutex);

	mSlaveThreadTraces.push_back(SlaveThreadTraceProxy(child));
}

void MasterThreadTrace::removeSlaveThread( class SlaveThreadTrace* child )
{
	LLMutexLock lock(&mSlaveListMutex);

	for (slave_thread_trace_list_t::iterator it = mSlaveThreadTraces.begin(), end_it = mSlaveThreadTraces.end();
		it != end_it;
		++it)
	{
		if (it->mSlaveTrace == child)
		{
			mSlaveThreadTraces.erase(it);
			break;
		}
	}
}

void MasterThreadTrace::pushToMaster()
{

}

MasterThreadTrace::MasterThreadTrace()
{
	LLThread::setTraceData(this);
}


}

