/** 
 * @file llqueuedthread.cpp
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
#include "llqueuedthread.h"

#include "llstl.h"
#include "lltimer.h"	// ms_sleep()

//============================================================================

// MAIN THREAD
LLQueuedThread::LLQueuedThread(const std::string& name, bool threaded, bool should_pause) :
	LLThread(name),
	mThreaded(threaded),
	mIdleThread(TRUE),
	mNextHandle(0),
	mStarted(FALSE)
{
	if (mThreaded)
	{
		if(should_pause)
		{
			pause() ; //call this before start the thread.
		}

		start();
	}
}

// MAIN THREAD
LLQueuedThread::~LLQueuedThread()
{
	if (!mThreaded)
	{
		endThread();
	}
	shutdown();
	// ~LLThread() will be called here
}

void LLQueuedThread::shutdown()
{
	setQuitting();

	unpause(); // MAIN THREAD
	if (mThreaded)
	{
		S32 timeout = 100;
		for ( ; timeout>0; timeout--)
		{
			if (isStopped())
			{
				break;
			}
			ms_sleep(100);
			LLThread::yield();
		}
		if (timeout == 0)
		{
			llwarns << "~LLQueuedThread (" << mName << ") timed out!" << llendl;
		}
	}
	else
	{
		mStatus = STOPPED;
	}

	QueuedRequest* req;
	S32 active_count = 0;
	while ( (req = (QueuedRequest*)mRequestHash.pop_element()) )
	{
		if (req->getStatus() == STATUS_QUEUED || req->getStatus() == STATUS_INPROGRESS)
		{
			++active_count;
			req->setStatus(STATUS_ABORTED); // avoid assert in deleteRequest
		}
		req->deleteRequest();
	}
	if (active_count)
	{
		llwarns << "~LLQueuedThread() called with active requests: " << active_count << llendl;
	}
}

//----------------------------------------------------------------------------

// MAIN THREAD
// virtual
S32 LLQueuedThread::update(F32 max_time_ms)
{
	if (!mStarted)
	{
		if (!mThreaded)
		{
			startThread();
			mStarted = TRUE;
		}
	}
	return updateQueue(max_time_ms);
}

S32 LLQueuedThread::updateQueue(F32 max_time_ms)
{
	F64 max_time = (F64)max_time_ms * .001;
	LLTimer timer;
	S32 pending = 1;

	// Frame Update
	if (mThreaded)
	{
		pending = getPending();
		if(pending > 0)
		{
			unpause();
		}
	}
	else
	{
		while (pending > 0)
		{
			pending = processNextRequest();
			if (max_time && timer.getElapsedTimeF64() > max_time)
				break;
		}
	}
	return pending;
}

void LLQueuedThread::incQueue()
{
	// Something has been added to the queue
	if (!isPaused())
	{
		if (mThreaded)
		{
			wake(); // Wake the thread up if necessary.
		}
	}
}

//virtual
// May be called from any thread
S32 LLQueuedThread::getPending()
{
	S32 res;
	lockData();
	res = mRequestQueue.size();
	unlockData();
	return res;
}

// MAIN thread
void LLQueuedThread::waitOnPending()
{
	while(1)
	{
		update(0);

		if (mIdleThread)
		{
			break;
		}
		if (mThreaded)
		{
			yield();
		}
	}
	return;
}

// MAIN thread
void LLQueuedThread::printQueueStats()
{
	lockData();
	if (!mRequestQueue.empty())
	{
		QueuedRequest *req = *mRequestQueue.begin();
		llinfos << llformat("Pending Requests:%d Current status:%d", mRequestQueue.size(), req->getStatus()) << llendl;
	}
	else
	{
		llinfos << "Queued Thread Idle" << llendl;
	}
	unlockData();
}

// MAIN thread
LLQueuedThread::handle_t LLQueuedThread::generateHandle()
{
	lockData();
	while ((mNextHandle == nullHandle()) || (mRequestHash.find(mNextHandle)))
	{
		mNextHandle++;
	}
	const LLQueuedThread::handle_t res = mNextHandle++;
	unlockData();
	return res;
}

// MAIN thread
bool LLQueuedThread::addRequest(QueuedRequest* req)
{
	if (mStatus == QUITTING)
	{
		return false;
	}
	
	lockData();
	req->setStatus(STATUS_QUEUED);
	mRequestQueue.insert(req);
	mRequestHash.insert(req);
#if _DEBUG
// 	llinfos << llformat("LLQueuedThread::Added req [%08d]",handle) << llendl;
#endif
	unlockData();

	incQueue();

	return true;
}

// MAIN thread
bool LLQueuedThread::waitForResult(LLQueuedThread::handle_t handle, bool auto_complete)
{
	llassert (handle != nullHandle())
	bool res = false;
	bool waspaused = isPaused();
	bool done = false;
	while(!done)
	{
		update(0); // unpauses
		lockData();
		QueuedRequest* req = (QueuedRequest*)mRequestHash.find(handle);
		if (!req)
		{
			done = true; // request does not exist
		}
		else if (req->getStatus() == STATUS_COMPLETE)
		{
			res = true;
			if (auto_complete)
			{
				mRequestHash.erase(handle);
				req->deleteRequest();
// 				check();
			}
			done = true;
		}
		unlockData();
		
		if (!done && mThreaded)
		{
			yield();
		}
	}
	if (waspaused)
	{
		pause();
	}
	return res;
}

// MAIN thread
LLQueuedThread::QueuedRequest* LLQueuedThread::getRequest(handle_t handle)
{
	if (handle == nullHandle())
	{
		return 0;
	}
	lockData();
	QueuedRequest* res = (QueuedRequest*)mRequestHash.find(handle);
	unlockData();
	return res;
}

LLQueuedThread::status_t LLQueuedThread::getRequestStatus(handle_t handle)
{
	status_t res = STATUS_EXPIRED;
	lockData();
	QueuedRequest* req = (QueuedRequest*)mRequestHash.find(handle);
	if (req)
	{
		res = req->getStatus();
	}
	unlockData();
	return res;
}

void LLQueuedThread::abortRequest(handle_t handle, bool autocomplete)
{
	lockData();
	QueuedRequest* req = (QueuedRequest*)mRequestHash.find(handle);
	if (req)
	{
		req->setFlags(FLAG_ABORT | (autocomplete ? FLAG_AUTO_COMPLETE : 0));
	}
	unlockData();
}

// MAIN thread
void LLQueuedThread::setFlags(handle_t handle, U32 flags)
{
	lockData();
	QueuedRequest* req = (QueuedRequest*)mRequestHash.find(handle);
	if (req)
	{
		req->setFlags(flags);
	}
	unlockData();
}

void LLQueuedThread::setPriority(handle_t handle, U32 priority)
{
	lockData();
	QueuedRequest* req = (QueuedRequest*)mRequestHash.find(handle);
	if (req)
	{
		if(req->getStatus() == STATUS_INPROGRESS)
		{
			// not in list
			req->setPriority(priority);
		}
		else if(req->getStatus() == STATUS_QUEUED)
		{
			// remove from list then re-insert
			llverify(mRequestQueue.erase(req) == 1);
			req->setPriority(priority);
			mRequestQueue.insert(req);
		}
	}
	unlockData();
}

bool LLQueuedThread::completeRequest(handle_t handle)
{
	bool res = false;
	lockData();
	QueuedRequest* req = (QueuedRequest*)mRequestHash.find(handle);
	if (req)
	{
		llassert_always(req->getStatus() != STATUS_QUEUED);
		llassert_always(req->getStatus() != STATUS_INPROGRESS);
#if _DEBUG
// 		llinfos << llformat("LLQueuedThread::Completed req [%08d]",handle) << llendl;
#endif
		mRequestHash.erase(handle);
		req->deleteRequest();
// 		check();
		res = true;
	}
	unlockData();
	return res;
}

bool LLQueuedThread::check()
{
#if 0 // not a reliable check once mNextHandle wraps, just for quick and dirty debugging
	for (int i=0; i<REQUEST_HASH_SIZE; i++)
	{
		LLSimpleHashEntry<handle_t>* entry = mRequestHash.get_element_at_index(i);
		while (entry)
		{
			if (entry->getHashKey() > mNextHandle)
			{
				llerrs << "Hash Error" << llendl;
				return false;
			}
			entry = entry->getNextEntry();
		}
	}
#endif
	return true;
}		
	
//============================================================================
// Runs on its OWN thread

S32 LLQueuedThread::processNextRequest()
{
	QueuedRequest *req;
	// Get next request from pool
	lockData();
	while(1)
	{
		req = NULL;
		if (mRequestQueue.empty())
		{
			break;
		}
		req = *mRequestQueue.begin();
		mRequestQueue.erase(mRequestQueue.begin());
		if ((req->getFlags() & FLAG_ABORT) || (mStatus == QUITTING))
		{
			req->setStatus(STATUS_ABORTED);
			req->finishRequest(false);
			if (req->getFlags() & FLAG_AUTO_COMPLETE)
			{
				mRequestHash.erase(req);
				req->deleteRequest();
// 				check();
			}
			continue;
		}
		llassert_always(req->getStatus() == STATUS_QUEUED);
		break;
	}
	U32 start_priority = 0 ;
	if (req)
	{
		req->setStatus(STATUS_INPROGRESS);
		start_priority = req->getPriority();
	}
	unlockData();

	// This is the only place we will call req->setStatus() after
	// it has initially been seet to STATUS_QUEUED, so it is
	// safe to access req.
	if (req)
	{
		// process request		
		bool complete = req->processRequest();

		if (complete)
		{
			lockData();
			req->setStatus(STATUS_COMPLETE);
			req->finishRequest(true);
			if (req->getFlags() & FLAG_AUTO_COMPLETE)
			{
				mRequestHash.erase(req);
				req->deleteRequest();
// 				check();
			}
			unlockData();
		}
		else
		{
			lockData();
			req->setStatus(STATUS_QUEUED);
			mRequestQueue.insert(req);
			unlockData();
			if (mThreaded && start_priority < PRIORITY_NORMAL)
			{
				ms_sleep(1); // sleep the thread a little
			}
		}
	}

	S32 pending = getPending();

	return pending;
}

// virtual
bool LLQueuedThread::runCondition()
{
	// mRunCondition must be locked here
	if (mRequestQueue.empty() && mIdleThread)
		return false;
	else
		return true;
}

// virtual
void LLQueuedThread::run()
{
	// call checPause() immediately so we don't try to do anything before the class is fully constructed
	checkPause();
	startThread();
	mStarted = TRUE;
	
	while (1)
	{
		// this will block on the condition until runCondition() returns true, the thread is unpaused, or the thread leaves the RUNNING state.
		checkPause();
		
		if (isQuitting())
		{
			endThread();
			break;
		}

		mIdleThread = FALSE;

		threadedUpdate();
		
		int res = processNextRequest();
		if (res == 0)
		{
			mIdleThread = TRUE;
			ms_sleep(1);
		}
		//LLThread::yield(); // thread should yield after each request		
	}
	llinfos << "LLQueuedThread " << mName << " EXITING." << llendl;
}

// virtual
void LLQueuedThread::startThread()
{
}

// virtual
void LLQueuedThread::endThread()
{
}

// virtual
void LLQueuedThread::threadedUpdate()
{
}

//============================================================================

LLQueuedThread::QueuedRequest::QueuedRequest(LLQueuedThread::handle_t handle, U32 priority, U32 flags) :
	LLSimpleHashEntry<LLQueuedThread::handle_t>(handle),
	mStatus(STATUS_UNKNOWN),
	mPriority(priority),
	mFlags(flags)
{
}

LLQueuedThread::QueuedRequest::~QueuedRequest()
{
	llassert_always(mStatus == STATUS_DELETE);
}

//virtual
void LLQueuedThread::QueuedRequest::finishRequest(bool completed)
{
}

//virtual
void LLQueuedThread::QueuedRequest::deleteRequest()
{
	llassert_always(mStatus != STATUS_INPROGRESS);
	setStatus(STATUS_DELETE);
	delete this;
}
