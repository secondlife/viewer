/** 
 * @file llvfsthread.cpp
 * @brief LLVFSThread implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llmath.h"
#include "llvfsthread.h"
#include "llstl.h"

//============================================================================

/*static*/ std::string LLVFSThread::sDataPath = "";

/*static*/ LLVFSThread* LLVFSThread::sLocal = NULL;

//============================================================================
// Run on MAIN thread
//static
void LLVFSThread::initClass(bool local_is_threaded, bool local_run_always)
{
	llassert(sLocal == NULL);
	sLocal = new LLVFSThread(local_is_threaded, local_run_always);
}

//static
S32 LLVFSThread::updateClass(U32 ms_elapsed)
{
	sLocal->update(ms_elapsed);
	return sLocal->getPending();
}

//static
void LLVFSThread::cleanupClass()
{
	sLocal->setQuitting();
	while (sLocal->getPending())
	{
		sLocal->update(0);
	}
	delete sLocal;
	sLocal = 0;
}

//----------------------------------------------------------------------------

LLVFSThread::LLVFSThread(bool threaded, bool runalways) :
	LLQueuedThread("VFS", threaded, runalways)
{
}

LLVFSThread::~LLVFSThread()
{
	// ~LLQueuedThread() will be called here
}

//----------------------------------------------------------------------------

LLVFSThread::handle_t LLVFSThread::read(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,
										U8* buffer, S32 offset, S32 numbytes, U32 priority, U32 flags)
{
	handle_t handle = generateHandle();

	priority = llmax(priority, (U32)PRIORITY_LOW); // All reads are at least PRIORITY_LOW
	Request* req = new Request(handle, priority, flags, FILE_READ, vfs, file_id, file_type,
							   buffer, offset, numbytes);

	bool res = addRequest(req);
	if (!res)
	{
		llerrs << "LLVFSThread::read called after LLVFSThread::cleanupClass()" << llendl;
		req->deleteRequest();
		handle = nullHandle();
	}

	return handle;
}

S32 LLVFSThread::readImmediate(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,
							   U8* buffer, S32 offset, S32 numbytes)
{
	handle_t handle = generateHandle();

	Request* req = new Request(handle, PRIORITY_IMMEDIATE, 0, FILE_READ, vfs, file_id, file_type,
							   buffer, offset, numbytes);
	
	S32 res = addRequest(req) ? 1 : 0;
	if (res == 0)
	{
		llerrs << "LLVFSThread::read called after LLVFSThread::cleanupClass()" << llendl;
		req->deleteRequest();
	}
	else
	{
		llverify(waitForResult(handle, false) == true);
		res = req->getBytesRead();
		completeRequest(handle);
	}
	return res;
}

LLVFSThread::handle_t LLVFSThread::write(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,
										 U8* buffer, S32 offset, S32 numbytes, U32 flags)
{
	handle_t handle = generateHandle();

	Request* req = new Request(handle, 0, flags, FILE_WRITE, vfs, file_id, file_type,
							   buffer, offset, numbytes);

	bool res = addRequest(req);
	if (!res)
	{
		llerrs << "LLVFSThread::read called after LLVFSThread::cleanupClass()" << llendl;
		req->deleteRequest();
		handle = nullHandle();
	}
	
	return handle;
}

S32 LLVFSThread::writeImmediate(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,
								 U8* buffer, S32 offset, S32 numbytes)
{
	handle_t handle = generateHandle();

	Request* req = new Request(handle, PRIORITY_IMMEDIATE, 0, FILE_WRITE, vfs, file_id, file_type,
							   buffer, offset, numbytes);

	S32 res = addRequest(req) ? 1 : 0;
	if (res == 0)
	{
		llerrs << "LLVFSThread::read called after LLVFSThread::cleanupClass()" << llendl;
		req->deleteRequest();
	}
	else
	{
		llverify(waitForResult(handle, false) == true);
		res = req->getBytesRead();
		completeRequest(handle);
	}
	return res;
}


LLVFSThread::handle_t LLVFSThread::rename(LLVFS* vfs, const LLUUID &file_id, const LLAssetType::EType file_type,
										  const LLUUID &new_id, const LLAssetType::EType new_type, U32 flags)
{
	handle_t handle = generateHandle();

	LLUUID* new_idp = new LLUUID(new_id); // deleted with Request
	// new_type is passed as "numbytes"
	Request* req = new Request(handle, 0, flags, FILE_RENAME, vfs, file_id, file_type,
							   (U8*)new_idp, 0, (S32)new_type);

	bool res = addRequest(req);
	if (!res)
	{
		llerrs << "LLVFSThread::read called after LLVFSThread::cleanupClass()" << llendl;
		req->deleteRequest();
		handle = nullHandle();
	}
	
	return handle;
}

//============================================================================
// Runs on its OWN thread

bool LLVFSThread::processRequest(QueuedRequest* qreq)
{
	Request *req = (Request*)qreq;

	bool complete = req->processIO();

	return complete;
}

//============================================================================

LLVFSThread::Request::Request(handle_t handle, U32 priority, U32 flags,
							  operation_t op, LLVFS* vfs,
							  const LLUUID &file_id, const LLAssetType::EType file_type,
							  U8* buffer, S32 offset, S32 numbytes) :
	QueuedRequest(handle, priority, flags),
	mOperation(op),
	mVFS(vfs),
	mFileID(file_id),
	mFileType(file_type),
	mBuffer(buffer),
	mOffset(offset),
	mBytes(numbytes),
	mBytesRead(0)
{
	llassert(mBuffer);

	if (numbytes <= 0 && mOperation != FILE_RENAME)
	{
		llwarns << "LLVFSThread: Request with numbytes = " << numbytes 
			<< " operation = " << op
			<< " offset " << offset 
			<< " file_type " << file_type << llendl;
	}
	if (mOperation == FILE_WRITE)
	{
		S32 blocksize =  mVFS->getMaxSize(mFileID, mFileType);
		if (blocksize < 0)
		{
			llwarns << "VFS write to temporary block (shouldn't happen)" << llendl;
		}
		mVFS->incLock(mFileID, mFileType, VFSLOCK_APPEND);
	}
	else if (mOperation == FILE_RENAME)
	{
		mVFS->incLock(mFileID, mFileType, VFSLOCK_APPEND);
	}
	else // if (mOperation == FILE_READ)
	{
		mVFS->incLock(mFileID, mFileType, VFSLOCK_READ);
	}
}

// dec locks as soon as a request finishes
void LLVFSThread::Request::finishRequest()
{
	if (mOperation == FILE_WRITE)
	{
		mVFS->decLock(mFileID, mFileType, VFSLOCK_APPEND);
	}
	else if (mOperation == FILE_RENAME)
	{
		mVFS->decLock(mFileID, mFileType, VFSLOCK_APPEND);
	}
	else // if (mOperation == FILE_READ)
	{
		mVFS->decLock(mFileID, mFileType, VFSLOCK_READ);
	}
}

void LLVFSThread::Request::deleteRequest()
{
	if (getStatus() == STATUS_QUEUED || getStatus() == STATUS_ABORT)
	{
		llerrs << "Attempt to delete a queued LLVFSThread::Request!" << llendl;
	}	
	if (mOperation == FILE_WRITE)
	{
		if (mFlags & AUTO_DELETE)
		{
			delete [] mBuffer;
		}
	}
	else if (mOperation == FILE_RENAME)
	{
		LLUUID* new_idp = (LLUUID*)mBuffer;
		delete new_idp;
	}
	LLQueuedThread::QueuedRequest::deleteRequest();
}

bool LLVFSThread::Request::processIO()
{
	bool complete = false;
	if (mOperation ==  FILE_READ)
	{
		llassert(mOffset >= 0);
		mBytesRead = mVFS->getData(mFileID, mFileType, mBuffer, mOffset, mBytes);
		complete = true;
		//llinfos << llformat("LLVFSThread::READ '%s': %d bytes arg:%d",getFilename(),mBytesRead) << llendl;
	}
	else if (mOperation ==  FILE_WRITE)
	{
		mBytesRead = mVFS->storeData(mFileID, mFileType, mBuffer, mOffset, mBytes);
		complete = true;
		//llinfos << llformat("LLVFSThread::WRITE '%s': %d bytes arg:%d",getFilename(),mBytesRead) << llendl;
	}
	else if (mOperation ==  FILE_RENAME)
	{
		LLUUID* new_idp = (LLUUID*)mBuffer;
		LLAssetType::EType new_type = (LLAssetType::EType)mBytes;
		mVFS->renameFile(mFileID, mFileType, *new_idp, new_type);
		complete = true;
		//llinfos << llformat("LLVFSThread::WRITE '%s': %d bytes arg:%d",getFilename(),mBytesRead) << llendl;
	}
	else
	{
		llerrs << llformat("LLVFSThread::unknown operation: %d", mOperation) << llendl;
	}
	return complete;
}

//============================================================================
