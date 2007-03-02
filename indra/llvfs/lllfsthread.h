/** 
 * @file lllfsthread.h
 * @brief LLLFSThread base class
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLLFSTHREAD_H
#define LL_LLLFSTHREAD_H

#include <queue>
#include <string>
#include <map>
#include <set>

#include "llapr.h"

#include "llqueuedthread.h"

//============================================================================
// Threaded Local File System
//============================================================================

class LLLFSThread : public LLQueuedThread
{
	//------------------------------------------------------------------------
public:
	enum operation_t {
		FILE_READ,
		FILE_WRITE,
		FILE_RENAME,
		FILE_REMOVE
	};

	//------------------------------------------------------------------------
public:

	class Responder : public LLThreadSafeRefCount
	{
	public:
		virtual ~Responder();
		virtual void completed(S32 bytes) = 0;
	};

	class Request : public QueuedRequest
	{
	protected:
		virtual ~Request(); // use deleteRequest()
		
	public:
		Request(LLLFSThread* thread,
				handle_t handle, U32 priority, 
				operation_t op, const LLString& filename,
				U8* buffer, S32 offset, S32 numbytes,
				Responder* responder);

		S32 getBytes()
		{
			return mBytes;
		}
		S32 getBytesRead()
		{
			return mBytesRead;
		}
		S32 getOperation()
		{
			return mOperation;
		}
		U8* getBuffer()
		{
			return mBuffer;
		}
		const LLString& getFilename()
		{
			return mFileName;
		}
		
		/*virtual*/ bool processRequest();
		/*virtual*/ void finishRequest(bool completed);
		/*virtual*/ void deleteRequest();
		
	private:
		LLLFSThread* mThread;
		operation_t mOperation;
		
		LLString mFileName;
		
		U8* mBuffer;	// dest for reads, source for writes, new UUID for rename
		S32 mOffset;	// offset into file, -1 = append (WRITE only)
		S32 mBytes;		// bytes to read from file, -1 = all
		S32 mBytesRead;	// bytes read from file

		LLPointer<Responder> mResponder;
	};

	//------------------------------------------------------------------------
public:
	LLLFSThread(bool threaded = TRUE);
	~LLLFSThread();	

	// Return a Request handle
	handle_t read(const LLString& filename,	/* Flawfinder: ignore */ 
				  U8* buffer, S32 offset, S32 numbytes,
				  Responder* responder, U32 pri=0);
	handle_t write(const LLString& filename,
				   U8* buffer, S32 offset, S32 numbytes,
				   Responder* responder, U32 pri=0);
	
	// Misc
	U32 priorityCounter() { return mPriorityCounter-- & PRIORITY_LOWBITS; } // Use to order IO operations
	
	// static initializers
	static void initClass(bool local_is_threaded = TRUE); // Setup sLocal
	static S32 updateClass(U32 ms_elapsed);
	static void cleanupClass();		// Delete sLocal

	
private:
	U32 mPriorityCounter;
	
public:
	static LLLFSThread* sLocal;		// Default local file thread
};

//============================================================================


#endif // LL_LLLFSTHREAD_H
