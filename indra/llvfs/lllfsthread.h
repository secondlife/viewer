/** 
 * @file lllfsthread.h
 * @brief LLLFSThread base class
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLLFSTHREAD_H
#define LL_LLLFSTHREAD_H

#include <queue>
#include <string>
#include <map>
#include <set>

#include "llpointer.h"
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
	protected:
		~Responder();
	public:
		virtual void completed(S32 bytes) = 0;
	};

	class Request : public QueuedRequest
	{
	protected:
		virtual ~Request(); // use deleteRequest()
		
	public:
		Request(LLLFSThread* thread,
				handle_t handle, U32 priority, 
				operation_t op, const std::string& filename,
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
		const std::string& getFilename()
		{
			return mFileName;
		}
		
		/*virtual*/ bool processRequest();
		/*virtual*/ void finishRequest(bool completed);
		/*virtual*/ void deleteRequest();
		
	private:
		LLLFSThread* mThread;
		operation_t mOperation;
		
		std::string mFileName;
		
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
	handle_t read(const std::string& filename,	/* Flawfinder: ignore */ 
				  U8* buffer, S32 offset, S32 numbytes,
				  Responder* responder, U32 pri=0);
	handle_t write(const std::string& filename,
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
