/** 
 * @file lllfsthread.cpp
 * @brief LLLFSThread base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llmath.h"
#include "lllfsthread.h"
#include "llstl.h"
#include "llapr.h"

//============================================================================

/*static*/ LLLFSThread* LLLFSThread::sLocal = NULL;

//============================================================================
// Run on MAIN thread
//static
void LLLFSThread::initClass(bool local_is_threaded, bool local_run_always)
{
	llassert(sLocal == NULL);
	sLocal = new LLLFSThread(local_is_threaded, local_run_always);
}

//static
S32 LLLFSThread::updateClass(U32 ms_elapsed)
{
	sLocal->update(ms_elapsed);
	return sLocal->getPending();
}

//static
void LLLFSThread::cleanupClass()
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

LLLFSThread::LLLFSThread(bool threaded, bool runalways) :
	LLQueuedThread("LFS", threaded, runalways)
{
}

LLLFSThread::~LLLFSThread()
{
	// ~LLQueuedThread() will be called here
}

//----------------------------------------------------------------------------

LLLFSThread::handle_t LLLFSThread::read(const LLString& filename,	/* Flawfinder: ignore */ 
										U8* buffer, S32 offset, S32 numbytes, U32 priority, U32 flags)
{
	handle_t handle = generateHandle();

	priority = llmax(priority, (U32)PRIORITY_LOW); // All reads are at least PRIORITY_LOW
	Request* req = new Request(handle, priority, flags,
							   FILE_READ, filename,
							   buffer, offset, numbytes);

	bool res = addRequest(req);
	if (!res)
	{
		llerrs << "LLLFSThread::read called after LLLFSThread::cleanupClass()" << llendl;
		req->deleteRequest();
		handle = nullHandle();
	}

	return handle;
}

S32 LLLFSThread::readImmediate(const LLString& filename,
							   U8* buffer, S32 offset, S32 numbytes)
{
	handle_t handle = generateHandle();

	Request* req = new Request(handle, PRIORITY_IMMEDIATE, 0,
							   FILE_READ, filename,
							   buffer, offset, numbytes);
	
	S32 res = addRequest(req) ? 1 : 0;
	if (res == 0)
	{
		llerrs << "LLLFSThread::read called after LLLFSThread::cleanupClass()" << llendl;
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

LLLFSThread::handle_t LLLFSThread::write(const LLString& filename,
										 U8* buffer, S32 offset, S32 numbytes, U32 flags)
{
	handle_t handle = generateHandle();

	Request* req = new Request(handle, 0, flags,
							   FILE_WRITE, filename,
							   buffer, offset, numbytes);

	bool res = addRequest(req);
	if (!res)
	{
		llerrs << "LLLFSThread::read called after LLLFSThread::cleanupClass()" << llendl;
		req->deleteRequest();
		handle = nullHandle();
	}
	
	return handle;
}

S32 LLLFSThread::writeImmediate(const LLString& filename,
								 U8* buffer, S32 offset, S32 numbytes)
{
	handle_t handle = generateHandle();

	Request* req = new Request(handle, PRIORITY_IMMEDIATE, 0,
							   FILE_WRITE, filename,
							   buffer, offset, numbytes);

	S32 res = addRequest(req) ? 1 : 0;
	if (res == 0)
	{
		llerrs << "LLLFSThread::write called after LLLFSThread::cleanupClass()" << llendl;
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


LLLFSThread::handle_t LLLFSThread::rename(const LLString& filename, const LLString& newname, U32 flags)
{
	handle_t handle = generateHandle();

	LLString* new_name_str = new LLString(newname); // deleted with Request
	Request* req = new Request(handle, 0, flags,
							   FILE_RENAME, filename,
							   (U8*)new_name_str, 0, 0);

	bool res = addRequest(req);
	if (!res)
	{
		llerrs << "LLLFSThread::rename called after LLLFSThread::cleanupClass()" << llendl;
		req->deleteRequest();
		handle = nullHandle();
	}
	
	return handle;
}

LLLFSThread::handle_t LLLFSThread::remove(const LLString& filename, U32 flags)
{
	handle_t handle = generateHandle();

	Request* req = new Request(handle, 0, flags,
							   FILE_RENAME, filename,
							   NULL, 0, 0);

	bool res = addRequest(req);
	if (!res)
	{
		llerrs << "LLLFSThread::remove called after LLLFSThread::cleanupClass()" << llendl;
		req->deleteRequest();
		handle = nullHandle();
	}
	
	return handle;
}

//============================================================================
// Runs on its OWN thread

bool LLLFSThread::processRequest(QueuedRequest* qreq)
{
	Request *req = (Request*)qreq;

	bool complete = req->processIO();

	return complete;
}

//============================================================================

LLLFSThread::Request::Request(handle_t handle, U32 priority, U32 flags,
							  operation_t op, const LLString& filename,
							  U8* buffer, S32 offset, S32 numbytes) :
	QueuedRequest(handle, priority, flags),
	mOperation(op),
	mFileName(filename),
	mBuffer(buffer),
	mOffset(offset),
	mBytes(numbytes),
	mBytesRead(0)
{
	llassert(mBuffer);

	if (numbytes <= 0 && mOperation != FILE_RENAME && mOperation != FILE_REMOVE)
	{
		llwarns << "LLLFSThread: Request with numbytes = " << numbytes << llendl;
	}
}

void LLLFSThread::Request::finishRequest()
{
}

void LLLFSThread::Request::deleteRequest()
{
	if (getStatus() == STATUS_QUEUED || getStatus() == STATUS_ABORT)
	{
		llerrs << "Attempt to delete a queued LLLFSThread::Request!" << llendl;
	}	
	if (mOperation == FILE_WRITE)
	{
		if (mFlags & AUTO_DELETE)
		{
			delete mBuffer;
		}
	}
	else if (mOperation == FILE_RENAME)
	{
		LLString* new_name = (LLString*)mBuffer;
		delete new_name;
	}
	LLQueuedThread::QueuedRequest::deleteRequest();
}

bool LLLFSThread::Request::processIO()
{
	bool complete = false;
	if (mOperation ==  FILE_READ)
	{
		llassert(mOffset >= 0);
		apr_file_t* filep = ll_apr_file_open(mFileName, LL_APR_RB);
		if (!filep)
		{
			llwarns << "LLLFS: Unable to read file: " << mFileName << llendl;
			mBytesRead = 0; // fail
			return true;
		}
		if (mOffset < 0)
			ll_apr_file_seek(filep, APR_END, 0);
		else
			ll_apr_file_seek(filep, APR_SET, mOffset);
		mBytesRead = ll_apr_file_read(filep, mBuffer, mBytes );
		apr_file_close(filep);
		complete = true;
		//llinfos << llformat("LLLFSThread::READ '%s': %d bytes",mFileName.c_str(),mBytesRead) << llendl;
	}
	else if (mOperation ==  FILE_WRITE)
	{
		apr_file_t* filep = ll_apr_file_open(mFileName, LL_APR_WB);
		if (!filep)
		{
			llwarns << "LLLFS: Unable to write file: " << mFileName << llendl;
			mBytesRead = 0; // fail
			return true;
		}
		if (mOffset < 0)
			ll_apr_file_seek(filep, APR_END, 0);
		else
			ll_apr_file_seek(filep, APR_SET, mOffset);
		mBytesRead = ll_apr_file_write(filep, mBuffer, mBytes );
		complete = true;
		apr_file_close(filep);
		//llinfos << llformat("LLLFSThread::WRITE '%s': %d bytes",mFileName.c_str(),mBytesRead) << llendl;
	}
	else if (mOperation ==  FILE_RENAME)
	{
		LLString* new_name = (LLString*)mBuffer;
		ll_apr_file_rename(mFileName, *new_name);
		complete = true;
		//llinfos << llformat("LLLFSThread::RENAME '%s': '%s'",mFileName.c_str(),new_name->c_str()) << llendl;
	}
	else if (mOperation ==  FILE_REMOVE)
	{
		ll_apr_file_remove(mFileName);
		complete = true;
		//llinfos << llformat("LLLFSThread::REMOVE '%s'",mFileName.c_str()) << llendl;
	}
	else
	{
		llerrs << llformat("LLLFSThread::unknown operation: %d", mOperation) << llendl;
	}
	return complete;
}

//============================================================================
