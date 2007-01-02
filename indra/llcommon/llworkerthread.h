/** 
 * @file llworkerthread.h
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLWORKERTHREAD_H
#define LL_LLWORKERTHREAD_H

#include <queue>
#include <string>
#include <map>
#include <set>

#include "llqueuedthread.h"

#define USE_FRAME_CALLBACK_MANAGER 0

//============================================================================

class LLWorkerClass;

//============================================================================
// Note: ~LLWorkerThread is O(N) N=# of worker threads, assumed to be small
//   It is assumed that LLWorkerThreads are rarely created/destroyed.

class LLWorkerThread : public LLQueuedThread
{
public:
	class Request : public LLQueuedThread::QueuedRequest
	{
	protected:
		~Request() {}; // use deleteRequest()
		
	public:
		Request(handle_t handle, U32 priority, LLWorkerClass* workerclass, S32 param);

		S32 getParam()
		{
			return mParam;
		}
		LLWorkerClass* getWorkerClass()
		{
			return mWorkerClass;
		}

		/*virtual*/ void deleteRequest();
		
	private:
		LLWorkerClass* mWorkerClass;
		S32 mParam;
	};

public:
	LLWorkerThread(bool threaded = true, bool runalways = true);
	~LLWorkerThread();	

protected:
	/*virtual*/ bool processRequest(QueuedRequest* req);
	
public:
	handle_t add(LLWorkerClass* workerclass, S32 param, U32 priority = PRIORITY_NORMAL);

	static void initClass(bool local_is_threaded = true, bool local_run_always = true); // Setup sLocal
	static S32 updateClass(U32 ms_elapsed);
	static S32 getAllPending();
	static void pauseAll();
	static void waitOnAllPending();
	static void cleanupClass();		// Delete sLocal

public:
	static LLWorkerThread* sLocal;		// Default worker thread
	static std::set<LLWorkerThread*> sThreadList; // array of threads (includes sLocal)
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

class LLWorkerClass
{
public:
	typedef LLWorkerThread::handle_t handle_t;
	enum FLAGS
	{
		WCF_WORKING = 0x01,
		WCF_ABORT_REQUESTED = 0x80
	};
	
public:
	LLWorkerClass(LLWorkerThread* workerthread, const std::string& name);
	virtual ~LLWorkerClass();

	// pure virtual, called from WORKER THREAD, returns TRUE if done
	virtual bool doWork(S32 param)=0; // Called from LLWorkerThread::processRequest()

	// called from WORKER THREAD
	void setWorking(bool working) { working ? setFlags(WCF_WORKING) : clearFlags(WCF_WORKING); }
	
	bool isWorking() { return getFlags(WCF_WORKING); }
	bool wasAborted() { return getFlags(WCF_ABORT_REQUESTED); }
		
	const std::string& getName() const { return mWorkerClassName; }

protected:
	// Call from doWork only to avoid eating up cpu time.
	// Returns true if work has been aborted
	// yields the current thread and calls mWorkerThread->checkPause()
	bool yield();
	
	void setWorkerThread(LLWorkerThread* workerthread);

	// addWork(): calls startWork, adds doWork() to queue
	void addWork(S32 param, U32 priority = LLWorkerThread::PRIORITY_NORMAL);

	// abortWork(): requests that work be aborted
	void abortWork();
	
	// checkWork(): if doWork is complete or aborted, call endWork() and return true
	bool checkWork();

	// haveWork(): return true if mWorkHandle != null
	bool haveWork() { return mWorkHandle != LLWorkerThread::nullHandle(); }

	// killWork(): aborts work and waits for the abort to process
	void killWork();

	// setPriority(): changes the priority of a request
	void setPriority(U32 priority);
	
private:
	void setFlags(U32 flags) { mWorkFlags = mWorkFlags | flags; }
	void clearFlags(U32 flags) { mWorkFlags = mWorkFlags & ~flags; }
	U32  getFlags() { return mWorkFlags; }
	bool getFlags(U32 flags) { return mWorkFlags & flags ? true : false; }
	
private:
	// pure virtuals
	virtual void startWork(S32 param)=0; // called from addWork() (MAIN THREAD)
	virtual void endWork(S32 param, bool aborted)=0; // called from doWork() (MAIN THREAD)
	
protected:
	LLWorkerThread* mWorkerThread;
	std::string mWorkerClassName;
	handle_t mWorkHandle;

private:
	LLAtomicU32 mWorkFlags;
};

//============================================================================


#endif // LL_LLWORKERTHREAD_H
