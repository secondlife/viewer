/** 
 * @file llworkerthread.h
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLWORKERTHREAD_H
#define LL_LLWORKERTHREAD_H

#include <queue>
#include <string>
#include <map>
#include <set>

#include "llqueuedthread.h"
#include "llapr.h"

#define USE_FRAME_CALLBACK_MANAGER 0

//============================================================================

class LLWorkerClass;

//============================================================================
// Note: ~LLWorkerThread is O(N) N=# of worker threads, assumed to be small
//   It is assumed that LLWorkerThreads are rarely created/destroyed.

class LL_COMMON_API LLWorkerThread : public LLQueuedThread
{
	friend class LLWorkerClass;
public:
	class WorkRequest : public LLQueuedThread::QueuedRequest
	{
	protected:
		virtual ~WorkRequest(); // use deleteRequest()
		
	public:
		WorkRequest(handle_t handle, U32 priority, LLWorkerClass* workerclass, S32 param);

		S32 getParam()
		{
			return mParam;
		}
		LLWorkerClass* getWorkerClass()
		{
			return mWorkerClass;
		}

		/*virtual*/ bool processRequest();
		/*virtual*/ void finishRequest(bool completed);
		/*virtual*/ void deleteRequest();
		
	private:
		LLWorkerClass* mWorkerClass;
		S32 mParam;
	};

protected:
	void clearDeleteList() ;

private:
	typedef std::list<LLWorkerClass*> delete_list_t;
	delete_list_t mDeleteList;
	LLMutex* mDeleteMutex;
	
public:
	LLWorkerThread(const std::string& name, bool threaded = true);
	~LLWorkerThread();

	/*virtual*/ S32 update(U32 max_time_ms);
	
	handle_t addWorkRequest(LLWorkerClass* workerclass, S32 param, U32 priority = PRIORITY_NORMAL);
	
	S32 getNumDeletes() { return (S32)mDeleteList.size(); } // debug

private:
	void deleteWorker(LLWorkerClass* workerclass); // schedule for deletion
	
};

//============================================================================

// This is a base class which any class with worker functions should derive from.
// Example Usage:
//  LLMyWorkerClass* foo = new LLMyWorkerClass();
//  foo->fetchData(); // calls addWork()
//  while(1) // main loop
//  {
//     if (foo->hasData()) // calls checkWork()
//        foo->processData();
//  }
//
// WorkerClasses only have one set of work functions. If they need to do multiple
//  background tasks, use 'param' to switch amnong them.
// Only one background task can be active at a time (per instance).
//  i.e. don't call addWork() if haveWork() returns true

class LL_COMMON_API LLWorkerClass
{
	friend class LLWorkerThread;
	friend class LLWorkerThread::WorkRequest;

public:
	typedef LLWorkerThread::handle_t handle_t;
	enum FLAGS
	{
		WCF_HAVE_WORK = 0x01,
		WCF_WORKING = 0x02,
		WCF_WORK_FINISHED = 0x10,
		WCF_WORK_ABORTED = 0x20,
		WCF_DELETE_REQUESTED = 0x40,
		WCF_ABORT_REQUESTED = 0x80
	};
	
public:
	LLWorkerClass(LLWorkerThread* workerthread, const std::string& name);
	virtual ~LLWorkerClass();

	// pure virtual, called from WORKER THREAD, returns TRUE if done
	virtual bool doWork(S32 param)=0; // Called from WorkRequest::processRequest()
	// virtual, called from finishRequest() after completed or aborted
	virtual void finishWork(S32 param, bool completed); // called from finishRequest() (WORK THREAD)
	// virtual, returns true if safe to delete the worker
	virtual bool deleteOK(); // called from update() (WORK THREAD)
	
	// schedlueDelete(): schedules deletion once aborted or completed
	void scheduleDelete();
	
	bool haveWork() { return getFlags(WCF_HAVE_WORK); } // may still be true if aborted
	bool isWorking() { return getFlags(WCF_WORKING); }
	bool wasAborted() { return getFlags(WCF_ABORT_REQUESTED); }

	// setPriority(): changes the priority of a request
	void setPriority(U32 priority);
	U32  getPriority() { return mRequestPriority; }
		
	const std::string& getName() const { return mWorkerClassName; }

protected:
	// called from WORKER THREAD
	void setWorking(bool working);
	
	// Call from doWork only to avoid eating up cpu time.
	// Returns true if work has been aborted
	// yields the current thread and calls mWorkerThread->checkPause()
	bool yield();
	
	void setWorkerThread(LLWorkerThread* workerthread);

	// addWork(): calls startWork, adds doWork() to queue
	void addWork(S32 param, U32 priority = LLWorkerThread::PRIORITY_NORMAL);

	// abortWork(): requests that work be aborted
	void abortWork(bool autocomplete);
	
	// checkWork(): if doWork is complete or aborted, call endWork() and return true
	bool checkWork(bool aborting = false);

private:
	void setFlags(U32 flags) { mWorkFlags = mWorkFlags | flags; }
	void clearFlags(U32 flags) { mWorkFlags = mWorkFlags & ~flags; }
	U32  getFlags() { return mWorkFlags; }
public:
	bool getFlags(U32 flags) { return mWorkFlags & flags ? true : false; }
	
private:
	// pure virtuals
	virtual void startWork(S32 param)=0; // called from addWork() (MAIN THREAD)
	virtual void endWork(S32 param, bool aborted)=0; // called from doWork() (MAIN THREAD)
	
protected:
	LLWorkerThread* mWorkerThread;
	std::string mWorkerClassName;
	handle_t mRequestHandle;
	U32 mRequestPriority; // last priority set

private:
	LLMutex mMutex;
	LLAtomicU32 mWorkFlags;
};

//============================================================================


#endif // LL_LLWORKERTHREAD_H
