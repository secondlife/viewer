/** 
 * @file llvfsthread.h
 * @brief LLVFSThread definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLVFSTHREAD_H
#define LL_LLVFSTHREAD_H

#include <queue>
#include <string>
#include <map>
#include <set>

#include "llqueuedthread.h"

#include "llvfs.h"

//============================================================================

class LLVFSThread : public LLQueuedThread
{
	//------------------------------------------------------------------------
public:
	enum operation_t {
		FILE_READ,
		FILE_WRITE,
		FILE_RENAME
	};

	//------------------------------------------------------------------------
public:

	class Request : public QueuedRequest
	{
	protected:
		~Request() {}; // use deleteRequest()
		
	public:
		Request(handle_t handle, U32 priority, U32 flags,
				operation_t op, LLVFS* vfs,
				const LLUUID &file_id, const LLAssetType::EType file_type,
				U8* buffer, S32 offset, S32 numbytes);

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
		LLVFS* getVFS()
		{
			return mVFS;
		}
		std::string getFilename()
		{
			std::string tstring;
			mFileID.toString(tstring);
			return tstring;
		}
		
		/*virtual*/ bool processRequest();
		/*virtual*/ void finishRequest(bool completed);
		/*virtual*/ void deleteRequest();
		
	private:
		operation_t mOperation;
		
		LLVFS* mVFS;
		LLUUID mFileID;
		LLAssetType::EType mFileType;
		
		U8* mBuffer;	// dest for reads, source for writes, new UUID for rename
		S32 mOffset;	// offset into file, -1 = append (WRITE only)
		S32 mBytes;		// bytes to read from file, -1 = all (new mFileType for rename)
		S32	mBytesRead;	// bytes read from file
	};

	//------------------------------------------------------------------------
public:
	static std::string sDataPath;
	static LLVFSThread* sLocal;		// Default worker thread
	
public:
	LLVFSThread(bool threaded = TRUE);
	~LLVFSThread();	

	// Return a Request handle
	handle_t read(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,	/* Flawfinder: ignore */
				  U8* buffer, S32 offset, S32 numbytes, U32 pri=PRIORITY_NORMAL, U32 flags = 0);
	handle_t write(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,
				   U8* buffer, S32 offset, S32 numbytes, U32 flags);
	// SJB: rename seems to have issues, especially when threaded
// 	handle_t rename(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,
// 					const LLUUID &new_id, const LLAssetType::EType new_type, U32 flags);
	// Return number of bytes read
	S32 readImmediate(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,
					  U8* buffer, S32 offset, S32 numbytes);
	S32 writeImmediate(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,
					   U8* buffer, S32 offset, S32 numbytes);

	/*virtual*/ bool processRequest(QueuedRequest* req);

public:
	static void initClass(bool local_is_threaded = TRUE); // Setup sLocal
	static S32 updateClass(U32 ms_elapsed);
	static void cleanupClass();		// Delete sLocal
	static void setDataPath(const std::string& path) { sDataPath = path; }
};

//============================================================================


#endif // LL_LLVFSTHREAD_H
