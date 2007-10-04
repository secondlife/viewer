/** 
 * @file lllfsthread.h
 * @brief LLLFSThread base class
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
