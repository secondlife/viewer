/** 
 * @file llworkerthread.cpp
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
#include "llworkerthread.h"
#include "llstl.h"

#if USE_FRAME_CALLBACK_MANAGER
#include "llframecallbackmanager.h"
#endif

//============================================================================
// Run on MAIN thread

LLWorkerThread::LLWorkerThread(const std::string& name, bool threaded, bool should_pause) :
	LLQueuedThread(name, threaded, should_pause)
{
	mDeleteMutex = new LLMutex(NULL);

	if(!mLocalAPRFilePoolp)
	{
		mLocalAPRFilePoolp = new LLVolatileAPRPool() ;
	}
}

LLWorkerThread::~LLWorkerThread()
{
	// Delete any workers in the delete queue (should be safe - had better be!)
	if (!mDeleteList.empty())
	{
		llwarns << "Worker Thread: " << mName << " destroyed with " << mDeleteList.size()
				<< " entries in delete list." << llendl;
	}

	delete mDeleteMutex;
	
	// ~LLQueuedThread() will be called here
}

//called only in destructor.
void LLWorkerThread::clearDeleteList()
{
	// Delete any workers in the delete queue (should be safe - had better be!)
	if (!mDeleteList.empty())
	{
		llwarns << "Worker Thread: " << mName << " destroyed with " << mDeleteList.size()
				<< " entries in delete list." << llendl;

		mDeleteMutex->lock();
		for (delete_list_t::iterator iter = mDeleteList.begin(); iter != mDeleteList.end(); ++iter)
		{
			(*iter)->mRequestHandle = LLWorkerThread::nullHandle();
			(*iter)->clearFlags(LLWorkerClass::WCF_HAVE_WORK);
			delete *iter ;
		}
		mDeleteList.clear() ;
		mDeleteMutex->unlock() ;
	}
}

// virtual
S32 LLWorkerThread::update(F32 max_time_ms)
{
	S32 res = LLQueuedThread::update(max_time_ms);
	// Delete scheduled workers
	std::vector<LLWorkerClass*> delete_list;
	std::vector<LLWorkerClass*> abort_list;
	mDeleteMutex->lock();
	for (delete_list_t::iterator iter = mDeleteList.begin();
		 iter != mDeleteList.end(); )
	{
		delete_list_t::iterator curiter = iter++;
		LLWorkerClass* worker = *curiter;
		if (worker->deleteOK())
		{
			if (worker->getFlags(LLWorkerClass::WCF_WORK_FINISHED))
			{
				delete_list.push_back(worker);
				mDeleteList.erase(curiter);
			}
			else if (!worker->getFlags(LLWorkerClass::WCF_ABORT_REQUESTED))
			{
				abort_list.push_back(worker);
			}
		}
	}
	mDeleteMutex->unlock();	
	// abort and delete after releasing mutex
	for (std::vector<LLWorkerClass*>::iterator iter = abort_list.begin();
		 iter != abort_list.end(); ++iter)
	{
		(*iter)->abortWork(false);
	}
	for (std::vector<LLWorkerClass*>::iterator iter = delete_list.begin();
		 iter != delete_list.end(); ++iter)
	{
		LLWorkerClass* worker = *iter;
		if (worker->mRequestHandle)
		{
			// Finished but not completed
			completeRequest(worker->mRequestHandle);
			worker->mRequestHandle = LLWorkerThread::nullHandle();
			worker->clearFlags(LLWorkerClass::WCF_HAVE_WORK);
		}
		delete *iter;
	}
	// delete and aborted entries mean there's still work to do
	res += delete_list.size() + abort_list.size();
	return res;
}

//----------------------------------------------------------------------------

LLWorkerThread::handle_t LLWorkerThread::addWorkRequest(LLWorkerClass* workerclass, S32 param, U32 priority)
{
	handle_t handle = generateHandle();
	
	WorkRequest* req = new WorkRequest(handle, priority, workerclass, param);

	bool res = addRequest(req);
	if (!res)
	{
		llerrs << "add called after LLWorkerThread::cleanupClass()" << llendl;
		req->deleteRequest();
		handle = nullHandle();
	}
	
	return handle;
}

void LLWorkerThread::deleteWorker(LLWorkerClass* workerclass)
{
	mDeleteMutex->lock();
	mDeleteList.push_back(workerclass);
	mDeleteMutex->unlock();
}

//============================================================================
// Runs on its OWN thread

LLWorkerThread::WorkRequest::WorkRequest(handle_t handle, U32 priority, LLWorkerClass* workerclass, S32 param) :
	LLQueuedThread::QueuedRequest(handle, priority),
	mWorkerClass(workerclass),
	mParam(param)
{
}

LLWorkerThread::WorkRequest::~WorkRequest()
{
}

// virtual (required for access by LLWorkerThread)
void LLWorkerThread::WorkRequest::deleteRequest()
{
	LLQueuedThread::QueuedRequest::deleteRequest();
}	

// virtual
bool LLWorkerThread::WorkRequest::processRequest()
{
	LLWorkerClass* workerclass = getWorkerClass();
	workerclass->setWorking(true);
	bool complete = workerclass->doWork(getParam());
	workerclass->setWorking(false);
	return complete;
}

// virtual
void LLWorkerThread::WorkRequest::finishRequest(bool completed)
{
	LLWorkerClass* workerclass = getWorkerClass();
	workerclass->finishWork(getParam(), completed);
	U32 flags = LLWorkerClass::WCF_WORK_FINISHED | (completed ? 0 : LLWorkerClass::WCF_WORK_ABORTED);
	workerclass->setFlags(flags);
}

//============================================================================
// LLWorkerClass:: operates in main thread

LLWorkerClass::LLWorkerClass(LLWorkerThread* workerthread, const std::string& name)
	: mWorkerThread(workerthread),
	  mWorkerClassName(name),
	  mRequestHandle(LLWorkerThread::nullHandle()),
	  mRequestPriority(LLWorkerThread::PRIORITY_NORMAL),
	  mMutex(NULL),
	  mWorkFlags(0)
{
	if (!mWorkerThread)
	{
		llerrs << "LLWorkerClass() called with NULL workerthread: " << name << llendl;
	}
}

LLWorkerClass::~LLWorkerClass()
{
	llassert_always(!(mWorkFlags & WCF_WORKING));
	llassert_always(mWorkFlags & WCF_DELETE_REQUESTED);
	llassert_always(!mMutex.isLocked());
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		LLWorkerThread::WorkRequest* workreq = (LLWorkerThread::WorkRequest*)mWorkerThread->getRequest(mRequestHandle);
		if (!workreq)
		{
			llerrs << "LLWorkerClass destroyed with stale work handle" << llendl;
		}
		if (workreq->getStatus() != LLWorkerThread::STATUS_ABORTED &&
			workreq->getStatus() != LLWorkerThread::STATUS_COMPLETE)
		{
			llerrs << "LLWorkerClass destroyed with active worker! Worker Status: " << workreq->getStatus() << llendl;
		}
	}
}

void LLWorkerClass::setWorkerThread(LLWorkerThread* workerthread)
{
	mMutex.lock();
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		llerrs << "LLWorkerClass attempt to change WorkerThread with active worker!" << llendl;
	}
	mWorkerThread = workerthread;
	mMutex.unlock();
}

//----------------------------------------------------------------------------

//virtual
void LLWorkerClass::finishWork(S32 param, bool success)
{
}

//virtual
bool LLWorkerClass::deleteOK()
{
	return true; // default always OK
}

//----------------------------------------------------------------------------

// Called from worker thread
void LLWorkerClass::setWorking(bool working)
{
	mMutex.lock();
	if (working)
	{
		llassert_always(!(mWorkFlags & WCF_WORKING));
		setFlags(WCF_WORKING);
	}
	else
	{
		llassert_always((mWorkFlags & WCF_WORKING));
		clearFlags(WCF_WORKING);
	}
	mMutex.unlock();
}

//----------------------------------------------------------------------------

bool LLWorkerClass::yield()
{
	LLThread::yield();
	mWorkerThread->checkPause();
	bool res;
	mMutex.lock();
	res = (getFlags() & WCF_ABORT_REQUESTED) ? true : false;
	mMutex.unlock();
	return res;
}

//----------------------------------------------------------------------------

// calls startWork, adds doWork() to queue
void LLWorkerClass::addWork(S32 param, U32 priority)
{
	mMutex.lock();
	llassert_always(!(mWorkFlags & (WCF_WORKING|WCF_HAVE_WORK)));
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		llerrs << "LLWorkerClass attempt to add work with active worker!" << llendl;
	}
#if _DEBUG
// 	llinfos << "addWork: " << mWorkerClassName << " Param: " << param << llendl;
#endif
	startWork(param);
	clearFlags(WCF_WORK_FINISHED|WCF_WORK_ABORTED);
	setFlags(WCF_HAVE_WORK);
	mRequestHandle = mWorkerThread->addWorkRequest(this, param, priority);
	mMutex.unlock();
}

void LLWorkerClass::abortWork(bool autocomplete)
{
	mMutex.lock();
#if _DEBUG
// 	LLWorkerThread::WorkRequest* workreq = mWorkerThread->getRequest(mRequestHandle);
// 	if (workreq)
// 		llinfos << "abortWork: " << mWorkerClassName << " Param: " << workreq->getParam() << llendl;
#endif
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		mWorkerThread->abortRequest(mRequestHandle, autocomplete);
		mWorkerThread->setPriority(mRequestHandle, LLQueuedThread::PRIORITY_IMMEDIATE);
		setFlags(WCF_ABORT_REQUESTED);
	}
	mMutex.unlock();
}

// if doWork is complete or aborted, call endWork() and return true
bool LLWorkerClass::checkWork(bool aborting)
{
	LLMutexLock lock(&mMutex);
	bool complete = false, abort = false;
	if (mRequestHandle != LLWorkerThread::nullHandle())
	{
		LLWorkerThread::WorkRequest* workreq = (LLWorkerThread::WorkRequest*)mWorkerThread->getRequest(mRequestHandle);
		if(!workreq)
		{
			if(mWorkerThread->isQuitting() || mWorkerThread->isStopped()) //the mWorkerThread is not running
			{
				mRequestHandle = LLWorkerThread::nullHandle();
				clearFlags(WCF_HAVE_WORK);
			}
			else
			{
				llassert_always(workreq);
			}
			return true ;
		}

		LLQueuedThread::status_t status = workreq->getStatus();
		if (status == LLWorkerThread::STATUS_ABORTED)
		{
			complete = true;
			abort = true;
		}
		else if (status == LLWorkerThread::STATUS_COMPLETE)
		{
			complete = true;
		}
		else
		{
			llassert_always(!aborting || (workreq->getFlags() & LLQueuedThread::FLAG_ABORT));
		}
		if (complete)
		{
			llassert_always(!(getFlags(WCF_WORKING)));
			endWork(workreq->getParam(), abort);
			mWorkerThread->completeRequest(mRequestHandle);
			mRequestHandle = LLWorkerThread::nullHandle();
			clearFlags(WCF_HAVE_WORK);
		}
	}
	else
	{
		complete = true;
	}
	return complete;
}

void LLWorkerClass::scheduleDelete()
{
	bool do_delete = false;
	mMutex.lock();
	if (!(getFlags(WCF_DELETE_REQUESTED)))
	{
		setFlags(WCF_DELETE_REQUESTED);
		do_delete = true;
	}
	mMutex.unlock();
	if (do_delete)
	{
		mWorkerThread->deleteWorker(this);
	}
}

void LLWorkerClass::setPriority(U32 priority)
{
	mMutex.lock();
	if (mRequestHandle != LLWorkerThread::nullHandle() && mRequestPriority != priority)
	{
		mRequestPriority = priority;
		mWorkerThread->setPriority(mRequestHandle, priority);
	}
	mMutex.unlock();
}

//============================================================================

