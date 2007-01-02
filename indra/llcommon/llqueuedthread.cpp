/** 
 * @file llqueuedthread.cpp
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llqueuedthread.h"
#include "llstl.h"

//============================================================================

// MAIN THREAD
LLQueuedThread::LLQueuedThread(const std::string& name, bool threaded, bool runalways) :
	LLThread(name),
	mThreaded(threaded),
	mRunAlways(runalways),
	mIdleThread(TRUE),
	mNextHandle(0)
{
	if (mThreaded)
	{
		start();
	}
}

// MAIN THREAD
LLQueuedThread::~LLQueuedThread()
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
	while ( (req = (QueuedRequest*)mRequestHash.pop_element()) )
	{
		req->deleteRequest();
	}
	
	// ~LLThread() will be called here
}

//----------------------------------------------------------------------------

// MAIN THREAD
void LLQueuedThread::update(U32 ms_elapsed)
{
	updateQueue(0);
}

void LLQueuedThread::updateQueue(S32 inc)
{
	// If mRunAlways == TRUE, unpause the thread whenever we put something into the queue.
	// If mRunAlways == FALSE, we only unpause the thread when updateQueue() is called from the main loop (i.e. between rendered frames)
	
	if (inc == 0) // Frame Update
	{
		if (mThreaded)
		{
			unpause();
			wake(); // Wake the thread up if necessary.
		}
		else
		{
			while (processNextRequest() > 0)
				;
		}
	}
	else
	{
		// Something has been added to the queue
		if (mRunAlways)
		{
			if (mThreaded)
			{
				wake(); // Wake the thread up if necessary.
			}
			else
			{
				while(processNextRequest() > 0)
					;
			}
		}
	}
}

//virtual
// May be called from any thread
S32 LLQueuedThread::getPending(bool child_thread)
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
		updateQueue(0);

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
	unlockData();
	return mNextHandle++;
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

	updateQueue(1);

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
		updateQueue(0); // unpauses
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

LLQueuedThread::status_t LLQueuedThread::abortRequest(handle_t handle, U32 flags)
{
	status_t res = STATUS_EXPIRED;
	lockData();
	QueuedRequest* req = (QueuedRequest*)mRequestHash.find(handle);
	if (req)
	{
		res = req->abortRequest(flags);
		if ((flags & AUTO_COMPLETE) && (res == STATUS_COMPLETE))
		{
			mRequestHash.erase(handle);
			req->deleteRequest();
// 			check();
		}
#if _DEBUG
// 		llinfos << llformat("LLQueuedThread::Aborted req [%08d]",handle) << llendl;
#endif
	}
	unlockData();
	return res;
}

// MAIN thread
LLQueuedThread::status_t LLQueuedThread::setFlags(handle_t handle, U32 flags)
{
	status_t res = STATUS_EXPIRED;
	lockData();
	QueuedRequest* req = (QueuedRequest*)mRequestHash.find(handle);
	if (req)
	{
		res = req->setFlags(flags);
	}
	unlockData();
	return res;
}

void LLQueuedThread::setPriority(handle_t handle, U32 priority)
{
	lockData();
	QueuedRequest* req = (QueuedRequest*)mRequestHash.find(handle);
	if (req && (req->getStatus() == STATUS_QUEUED))
	{
		llverify(mRequestQueue.erase(req) == 1);
		req->setPriority(priority);
		mRequestQueue.insert(req);
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
		llassert(req->getStatus() != STATUS_QUEUED && req->getStatus() != STATUS_ABORT);
#if _DEBUG
//  		llinfos << llformat("LLQueuedThread::Completed req [%08d]",handle) << llendl;
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

int LLQueuedThread::processNextRequest()
{
	QueuedRequest *req = 0;
	// Get next request from pool
	lockData();
	while(1)
	{
		if (!mRequestQueue.empty())
		{
			req = *mRequestQueue.begin();
			mRequestQueue.erase(mRequestQueue.begin());
		}
		if (req && req->getStatus() == STATUS_ABORT)
		{
			req->setStatus(STATUS_ABORTED);
			req = 0;
		}
		else
		{
			llassert (!req || req->getStatus() == STATUS_QUEUED)
			break;
		}
	}
	if (req)
	{
		req->setStatus(STATUS_INPROGRESS);
	}
	unlockData();

	// This is the only place we will cal req->setStatus() after
	// it has initially been seet to STATUS_QUEUED, so it is
	// safe to access req.
	if (req)
	{
		// process request
		bool complete = processRequest(req);

		if (complete)
		{
			lockData();
			req->setStatus(STATUS_COMPLETE);
			req->finishRequest();
			if (req->getFlags() & AUTO_COMPLETE)
			{
				llverify(mRequestHash.erase(req))
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
		}
	}

	int res;
	if (getPending(true) == 0)
	{
		if (isQuitting())
		{
			res = -1; // exit thread
		}
		else
		{
			res = 0;
		}
	}
	else
	{
		res = 1;
	}
	return res;
}

bool LLQueuedThread::runCondition()
{
	// mRunCondition must be locked here
	return (mRequestQueue.empty() && mIdleThread) ? FALSE : TRUE;
}

void LLQueuedThread::run()
{
	llinfos << "QUEUED THREAD STARTING" << llendl;

	while (1)
	{
		// this will block on the condition until runCondition() returns true, the thread is unpaused, or the thread leaves the RUNNING state.
		checkPause();
		
		if(isQuitting())
			break;

		//llinfos << "QUEUED THREAD RUNNING, queue size = " << mRequestQueue.size() << llendl;

		mIdleThread = FALSE;
		
		int res = processNextRequest();
		if (res == 0)
		{
			mIdleThread = TRUE;
		}
		
		if (res < 0) // finished working and want to exit
		{
			break;
		}
	}

	llinfos << "QUEUED THREAD " << mName << " EXITING." << llendl;
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
	if (mStatus != STATUS_DELETE)
	{
		llerrs << "Attemt to directly delete a  LLQueuedThread::QueuedRequest; use deleteRequest()" << llendl;
	}
}

//virtual
void LLQueuedThread::QueuedRequest::finishRequest()
{
}

//virtual
void LLQueuedThread::QueuedRequest::deleteRequest()
{
	setStatus(STATUS_DELETE);
	delete this;
}
