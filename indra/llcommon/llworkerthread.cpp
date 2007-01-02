/** 
 * @file llworkerthread.cpp
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llworkerthread.h"
#include "llstl.h"

#if USE_FRAME_CALLBACK_MANAGER
#include "llframecallbackmanager.h"
#endif

//============================================================================

/*static*/ LLWorkerThread* LLWorkerThread::sLocal = NULL;
/*static*/ std::set<LLWorkerThread*> LLWorkerThread::sThreadList;

//============================================================================
// Run on MAIN thread

//static
void LLWorkerThread::initClass(bool local_is_threaded, bool local_run_always)
{
	if (!sLocal)
	{
		sLocal = new LLWorkerThread(local_is_threaded, local_run_always);
	}
}

//static
void LLWorkerThread::cleanupClass()
{
	if (sLocal)
	{
		while (sLocal->getPending())
		{
			sLocal->update(0);
		}
		delete sLocal;
		sLocal = NULL;
		llassert(sThreadList.size() == 0);
	}
}

//static
S32 LLWorkerThread::updateClass(U32 ms_elapsed)
{
	for (std::set<LLWorkerThread*>::iterator iter = sThreadList.begin(); iter != sThreadList.end(); iter++)
	{
		(*iter)->update(ms_elapsed);
	}
	return getAllPending();
}

//static
S32 LLWorkerThread::getAllPending()
{
	S32 res = 0;
	for (std::set<LLWorkerThread*>::iterator iter = sThreadList.begin(); iter != sThreadList.end(); iter++)
	{
		res += (*iter)->getPending();
	}
	return res;
}

//static
void LLWorkerThread::pauseAll()
{
	for (std::set<LLWorkerThread*>::iterator iter = sThreadList.begin(); iter != sThreadList.end(); iter++)
	{
		(*iter)->pause();
	}
}

//static
void LLWorkerThread::waitOnAllPending()
{
	for (std::set<LLWorkerThread*>::iterator iter = sThreadList.begin(); iter != sThreadList.end(); iter++)
	{
		(*iter)->waitOnPending();
	}
}

//----------------------------------------------------------------------------

LLWorkerThread::LLWorkerThread(bool threaded, bool runalways) :
	LLQueuedThread("Worker", threaded, runalways)
{
	sThreadList.insert(this);
}

LLWorkerThread::~LLWorkerThread()
{
	llverify(sThreadList.erase(this) == 1);
	// ~LLQueuedThread() will be called here
}

//----------------------------------------------------------------------------


LLWorkerThread::handle_t LLWorkerThread::add(LLWorkerClass* workerclass, S32 param, U32 priority)
{
	handle_t handle = generateHandle();
	
	Request* req = new Request(handle, priority, workerclass, param);

	bool res = addRequest(req);
	if (!res)
	{
		llerrs << "add called after LLWorkerThread::cleanupClass()" << llendl;
		req->deleteRequest();
		handle = nullHandle();
	}
	
	return handle;
}

//============================================================================
// Runs on its OWN thread

bool LLWorkerThread::processRequest(QueuedRequest* qreq)
{
	Request *req = (Request*)qreq;

	req->getWorkerClass()->setWorking(true);

	bool complete = req->getWorkerClass()->doWork(req->getParam());

	req->getWorkerClass()->setWorking(false);

	LLThread::yield(); // worker thread should yield after each request
	
	return complete;
}

//============================================================================

LLWorkerThread::Request::Request(handle_t handle, U32 priority, LLWorkerClass* workerclass, S32 param) :
	LLQueuedThread::QueuedRequest(handle, priority),
	mWorkerClass(workerclass),
	mParam(param)
{
}

void LLWorkerThread::Request::deleteRequest()
{
	LLQueuedThread::QueuedRequest::deleteRequest();
}	

//============================================================================
// LLWorkerClass:: operates in main thread

LLWorkerClass::LLWorkerClass(LLWorkerThread* workerthread, const std::string& name)
	: mWorkerThread(workerthread),
	  mWorkerClassName(name),
	  mWorkHandle(LLWorkerThread::nullHandle()),
	  mWorkFlags(0)
{
	if (!mWorkerThread)
	{
		mWorkerThread = LLWorkerThread::sLocal;
	}
}
LLWorkerClass::~LLWorkerClass()
{
	if (mWorkHandle != LLWorkerThread::nullHandle())
	{
		LLWorkerThread::Request* workreq = (LLWorkerThread::Request*)mWorkerThread->getRequest(mWorkHandle);
		if (!workreq)
		{
			llerrs << "LLWorkerClass destroyed with stale work handle" << llendl;
		}
		if (workreq->getStatus() != LLWorkerThread::STATUS_ABORT &&
			workreq->getStatus() != LLWorkerThread::STATUS_ABORTED &&
			workreq->getStatus() != LLWorkerThread::STATUS_COMPLETE)
		{
			llerrs << "LLWorkerClass destroyed with active worker! Worker Status: " << workreq->getStatus() << llendl;
		}
	}
}

void LLWorkerClass::setWorkerThread(LLWorkerThread* workerthread)
{
	if (mWorkHandle != LLWorkerThread::nullHandle())
	{
		llerrs << "LLWorkerClass attempt to change WorkerThread with active worker!" << llendl;
	}
	mWorkerThread = workerthread;
}

//----------------------------------------------------------------------------

bool LLWorkerClass::yield()
{
	llassert(mWorkFlags & WCF_WORKING);
	LLThread::yield();
	mWorkerThread->checkPause();
	return (getFlags() & WCF_ABORT_REQUESTED) ? true : false;
}

//----------------------------------------------------------------------------

// calls startWork, adds doWork() to queue
void LLWorkerClass::addWork(S32 param, U32 priority)
{
	if (mWorkHandle != LLWorkerThread::nullHandle())
	{
		llerrs << "LLWorkerClass attempt to add work with active worker!" << llendl;
	}
#if _DEBUG
// 	llinfos << "addWork: " << mWorkerClassName << " Param: " << param << llendl;
#endif
	startWork(param);
	mWorkHandle = mWorkerThread->add(this, param, priority);
}

void LLWorkerClass::abortWork()
{
#if _DEBUG
// 	LLWorkerThread::Request* workreq = mWorkerThread->getRequest(mWorkHandle);
// 	if (workreq)
// 		llinfos << "abortWork: " << mWorkerClassName << " Param: " << workreq->getParam() << llendl;
#endif
	mWorkerThread->abortRequest(mWorkHandle);
	setFlags(WCF_ABORT_REQUESTED);
}

// if doWork is complete or aborted, call endWork() and return true
bool LLWorkerClass::checkWork()
{
	bool complete = false, abort = false;
	LLWorkerThread::Request* workreq = (LLWorkerThread::Request*)mWorkerThread->getRequest(mWorkHandle);
	llassert(workreq);
	if (getFlags(WCF_ABORT_REQUESTED) || workreq->getStatus() == LLWorkerThread::STATUS_ABORTED)
	{
		complete = true;
		abort = true;
	}
	else if (workreq->getStatus() == LLWorkerThread::STATUS_COMPLETE)
	{
		complete = true;
	}
	if (complete)
	{
#if _DEBUG
// 		llinfos << "endWork: " << mWorkerClassName << " Param: " << workreq->getParam() << llendl;
#endif
		endWork(workreq->getParam(), abort);
		mWorkerThread->completeRequest(mWorkHandle);
		mWorkHandle = LLWorkerThread::nullHandle();
	}
	return complete;
}

void LLWorkerClass::killWork()
{
	if (haveWork())
	{
		abortWork();
		bool paused = mWorkerThread->isPaused();
		while (!checkWork())
		{
			mWorkerThread->updateQueue(0);
		}
		if (paused)
		{
			mWorkerThread->pause();
		}
	}
}

void LLWorkerClass::setPriority(U32 priority)
{
	if (haveWork())
	{
		mWorkerThread->setPriority(mWorkHandle, priority);
	}
}

//============================================================================

