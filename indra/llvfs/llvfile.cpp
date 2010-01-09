/** 
 * @file llvfile.cpp
 * @brief Implementation of virtual file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llvfile.h"

#include "llerror.h"
#include "llthread.h"
#include "llstat.h"
#include "llvfs.h"

const S32 LLVFile::READ			= 0x00000001;
const S32 LLVFile::WRITE		= 0x00000002;
const S32 LLVFile::READ_WRITE	= 0x00000003;  // LLVFile::READ & LLVFile::WRITE
const S32 LLVFile::APPEND		= 0x00000006;  // 0x00000004 & LLVFile::WRITE

static LLFastTimerUtil::DeclareTimer FTM_VFILE_WAIT("VFile Wait");

//----------------------------------------------------------------------------
LLVFSThread* LLVFile::sVFSThread = NULL;
BOOL LLVFile::sAllocdVFSThread = FALSE;
//----------------------------------------------------------------------------

//============================================================================

LLVFile::LLVFile(LLVFS *vfs, const LLUUID &file_id, const LLAssetType::EType file_type, S32 mode)
{
	mFileType =	file_type;

	mFileID =	file_id;
	mPosition = 0;
	mMode =		mode;
	mVFS =		vfs;

	mBytesRead = 0;
	mHandle = LLVFSThread::nullHandle();
	mPriority = 128.f;

	mVFS->incLock(mFileID, mFileType, VFSLOCK_OPEN);
}

LLVFile::~LLVFile()
{
	if (!isReadComplete())
	{
		if (mHandle != LLVFSThread::nullHandle())
		{
			if (!(mMode & LLVFile::WRITE))
			{
				//llwarns << "Destroying LLVFile with pending async read/write, aborting..." << llendl;
				sVFSThread->setFlags(mHandle, LLVFSThread::FLAG_AUTO_COMPLETE | LLVFSThread::FLAG_ABORT);
			}
			else // WRITE
			{
				sVFSThread->setFlags(mHandle, LLVFSThread::FLAG_AUTO_COMPLETE);
			}
		}
	}
	mVFS->decLock(mFileID, mFileType, VFSLOCK_OPEN);
}

BOOL LLVFile::read(U8 *buffer, S32 bytes, BOOL async, F32 priority)
{
	if (! (mMode & READ))
	{
		llwarns << "Attempt to read from file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << llendl;
		return FALSE;
	}

	if (mHandle != LLVFSThread::nullHandle())
	{
		llwarns << "Attempt to read from vfile object " << mFileID << " with pending async operation" << llendl;
		return FALSE;
	}
	mPriority = priority;
	
	BOOL success = TRUE;

	// We can't do a read while there are pending async writes
	waitForLock(VFSLOCK_APPEND);
	
	// *FIX: (???)
	if (async)
	{
		mHandle = sVFSThread->read(mVFS, mFileID, mFileType, buffer, mPosition, bytes, threadPri());
	}
	else
	{
		// We can't do a read while there are pending async writes on this file
		mBytesRead = sVFSThread->readImmediate(mVFS, mFileID, mFileType, buffer, mPosition, bytes);
		mPosition += mBytesRead;
		if (! mBytesRead)
		{
			success = FALSE;
		}
	}

	return success;
}

//static
U8* LLVFile::readFile(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type, S32* bytes_read)
{
	U8 *data;
	LLVFile file(vfs, uuid, type, LLVFile::READ);
	S32 file_size = file.getSize();
	if (file_size == 0)
	{
		// File is empty.
		data = NULL;
	}
	else
	{
		data = new U8[file_size];
		file.read(data, file_size);	/* Flawfinder: ignore */ 
		
		if (file.getLastBytesRead() != (S32)file_size)
		{
			delete[] data;
			data = NULL;
			file_size = 0;
		}
	}
	if (bytes_read)
	{
		*bytes_read = file_size;
	}
	return data;
}
	
void LLVFile::setReadPriority(const F32 priority)
{
	mPriority = priority;
	if (mHandle != LLVFSThread::nullHandle())
	{
		sVFSThread->setPriority(mHandle, threadPri());
	}
}

BOOL LLVFile::isReadComplete()
{
	BOOL res = TRUE;
	if (mHandle != LLVFSThread::nullHandle())
	{
		LLVFSThread::Request* req = (LLVFSThread::Request*)sVFSThread->getRequest(mHandle);
		LLVFSThread::status_t status = req->getStatus();
		if (status == LLVFSThread::STATUS_COMPLETE)
		{
			mBytesRead = req->getBytesRead();
			mPosition += mBytesRead;
			sVFSThread->completeRequest(mHandle);
			mHandle = LLVFSThread::nullHandle();
		}
		else
		{
			res = FALSE;
		}
	}
	return res;
}

S32 LLVFile::getLastBytesRead()
{
	return mBytesRead;
}

BOOL LLVFile::eof()
{
	return mPosition >= getSize();
}

BOOL LLVFile::write(const U8 *buffer, S32 bytes)
{
	if (! (mMode & WRITE))
	{
		llwarns << "Attempt to write to file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << llendl;
	}
	if (mHandle != LLVFSThread::nullHandle())
	{
		llerrs << "Attempt to write to vfile object " << mFileID << " with pending async operation" << llendl;
		return FALSE;
	}
	BOOL success = TRUE;
	
	// *FIX: allow async writes? potential problem wit mPosition...
	if (mMode == APPEND) // all appends are async (but WRITEs are not)
	{	
		U8* writebuf = new U8[bytes];
		memcpy(writebuf, buffer, bytes);
		S32 offset = -1;
		mHandle = sVFSThread->write(mVFS, mFileID, mFileType,
									writebuf, offset, bytes,
									LLVFSThread::FLAG_AUTO_COMPLETE | LLVFSThread::FLAG_AUTO_DELETE);
		mHandle = LLVFSThread::nullHandle(); // FLAG_AUTO_COMPLETE means we don't track this
	}
	else
	{
		// We can't do a write while there are pending reads or writes on this file
		waitForLock(VFSLOCK_READ);
		waitForLock(VFSLOCK_APPEND);

		S32 pos = (mMode & APPEND) == APPEND ? -1 : mPosition;

		S32 wrote = sVFSThread->writeImmediate(mVFS, mFileID, mFileType, (U8*)buffer, pos, bytes);

		mPosition += wrote;
		
		if (wrote < bytes)
		{	
			llwarns << "Tried to write " << bytes << " bytes, actually wrote " << wrote << llendl;

			success = FALSE;
		}
	}
	return success;
}

//static
BOOL LLVFile::writeFile(const U8 *buffer, S32 bytes, LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type)
{
	LLVFile file(vfs, uuid, type, LLVFile::WRITE);
	file.setMaxSize(bytes);
	return file.write(buffer, bytes);
}

BOOL LLVFile::seek(S32 offset, S32 origin)
{
	if (mMode == APPEND)
	{
		llwarns << "Attempt to seek on append-only file" << llendl;
		return FALSE;
	}

	if (-1 == origin)
	{
		origin = mPosition;
	}

	S32 new_pos = origin + offset;

	S32 size = getSize(); // Calls waitForLock(VFSLOCK_APPEND)

	if (new_pos > size)
	{
		llwarns << "Attempt to seek past end of file" << llendl;

		mPosition = size;
		return FALSE;
	}
	else if (new_pos < 0)
	{
		llwarns << "Attempt to seek past beginning of file" << llendl;

		mPosition = 0;
		return FALSE;
	}

	mPosition = new_pos;
	return TRUE;
}

S32 LLVFile::tell() const
{
	return mPosition;
}

S32 LLVFile::getSize()
{
	waitForLock(VFSLOCK_APPEND);
	S32 size = mVFS->getSize(mFileID, mFileType);

	return size;
}

S32 LLVFile::getMaxSize()
{
	S32 size = mVFS->getMaxSize(mFileID, mFileType);

	return size;
}

BOOL LLVFile::setMaxSize(S32 size)
{
	if (! (mMode & WRITE))
	{
		llwarns << "Attempt to change size of file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << llendl;

		return FALSE;
	}

	if (!mVFS->checkAvailable(size))
	{
		LLFastTimer t(FTM_VFILE_WAIT);
		S32 count = 0;
		while (sVFSThread->getPending() > 1000)
		{
			if (count % 100 == 0)
			{
				llinfos << "VFS catching up... Pending: " << sVFSThread->getPending() << llendl;
			}
			if (sVFSThread->isPaused())
			{
				sVFSThread->update(0);
			}
			ms_sleep(10);
		}
	}
	return mVFS->setMaxSize(mFileID, mFileType, size);
}

BOOL LLVFile::rename(const LLUUID &new_id, const LLAssetType::EType new_type)
{
	if (! (mMode & WRITE))
	{
		llwarns << "Attempt to rename file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << llendl;

		return FALSE;
	}

	if (mHandle != LLVFSThread::nullHandle())
	{
		llwarns << "Renaming file with pending async read" << llendl;
	}

	waitForLock(VFSLOCK_READ);
	waitForLock(VFSLOCK_APPEND);

	// we need to release / replace our own lock
	// since the renamed file will inherit locks from the new name
	mVFS->decLock(mFileID, mFileType, VFSLOCK_OPEN);
	mVFS->renameFile(mFileID, mFileType, new_id, new_type);
	mVFS->incLock(new_id, new_type, VFSLOCK_OPEN);
	
	mFileID = new_id;
	mFileType = new_type;

	return TRUE;
}

BOOL LLVFile::remove()
{
// 	llinfos << "Removing file " << mFileID << llendl;
	
	if (! (mMode & WRITE))
	{
		// Leaving paranoia warning just because this should be a very infrequent
		// operation.
		llwarns << "Remove file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << llendl;
	}

	if (mHandle != LLVFSThread::nullHandle())
	{
		llwarns << "Removing file with pending async read" << llendl;
	}
	
	// why not seek back to the beginning of the file too?
	mPosition = 0;

	waitForLock(VFSLOCK_READ);
	waitForLock(VFSLOCK_APPEND);
	mVFS->removeFile(mFileID, mFileType);

	return TRUE;
}

// static
void LLVFile::initClass(LLVFSThread* vfsthread)
{
	if (!vfsthread)
	{
		if (LLVFSThread::sLocal != NULL)
		{
			vfsthread = LLVFSThread::sLocal;
		}
		else
		{
			vfsthread = new LLVFSThread();
			sAllocdVFSThread = TRUE;
		}
	}
	sVFSThread = vfsthread;
}

// static
void LLVFile::cleanupClass()
{
	if (sAllocdVFSThread)
	{
		delete sVFSThread;
	}
	sVFSThread = NULL;
}

bool LLVFile::isLocked(EVFSLock lock)
{
	return mVFS->isLocked(mFileID, mFileType, lock) ? true : false;
}

void LLVFile::waitForLock(EVFSLock lock)
{
	LLFastTimer t(FTM_VFILE_WAIT);
	// spin until the lock clears
	while (isLocked(lock))
	{
		if (sVFSThread->isPaused())
		{
			sVFSThread->update(0);
		}
		ms_sleep(1);
	}
}
