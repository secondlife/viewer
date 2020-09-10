/** 
 * @file llvfile.cpp
 * @brief Implementation of virtual file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "linden_common.h"

#include "llvfile.h"

#include "llerror.h"
#include "llthread.h"
#include "lltimer.h"
#include "llfasttimer.h"
#include "llmemory.h"
#include "llvfs.h"

#include <fstream>
#include "lldir.h"

const S32 LLVFile::READ			= 0x00000001;
const S32 LLVFile::WRITE		= 0x00000002;
const S32 LLVFile::READ_WRITE	= 0x00000003;  // LLVFile::READ & LLVFile::WRITE
const S32 LLVFile::APPEND		= 0x00000006;  // 0x00000004 & LLVFile::WRITE

static LLTrace::BlockTimerStatHandle FTM_VFILE_WAIT("VFile Wait");

//----------------------------------------------------------------------------
//CP LLVFSThread* LLVFile::sVFSThread = NULL;
//CP BOOL LLVFile::sAllocdVFSThread = FALSE;
//----------------------------------------------------------------------------

//============================================================================

LLVFile::LLVFile(LLVFS *vfs, const LLUUID &file_id, const LLAssetType::EType file_type, S32 mode)
{
	mFileType =	file_type;
	mFileID =	file_id;
	mPosition = 0;
    mBytesRead = 0;
    mReadComplete = FALSE;
	mMode =		mode;
	//CP mVFS =		vfs;

	//CP mHandle = LLVFSThread::nullHandle();
	//CP mPriority = 128.f;

	//CP mVFS->incLock(mFileID, mFileType, VFSLOCK_OPEN);
}

LLVFile::~LLVFile()
{
    //CP BEGIN
	//if (!isReadComplete())
	//{
	//	if (mHandle != LLVFSThread::nullHandle())
	//	{
	//		if (!(mMode & LLVFile::WRITE))
	//		{
	//			//LL_WARNS() << "Destroying LLVFile with pending async read/write, aborting..." << LL_ENDL;
	//			sVFSThread->setFlags(mHandle, LLVFSThread::FLAG_AUTO_COMPLETE | LLVFSThread::FLAG_ABORT);
	//		}
	//		else // WRITE
	//		{
	//			sVFSThread->setFlags(mHandle, LLVFSThread::FLAG_AUTO_COMPLETE);
	//		}
	//	}
	//}
	//mVFS->decLock(mFileID, mFileType, VFSLOCK_OPEN);
    //CP END
}

const std::string assetTypeToString(LLAssetType::EType at)
{
    /**
     * Make use of the C++17 (or is it 14) feature that allows
     * for inline initialization of an std::map<>
     */
    typedef std::map<LLAssetType::EType, std::string> asset_type_to_name_t;
    asset_type_to_name_t asset_type_to_name =
    {
        { LLAssetType::AT_TEXTURE, "TEXTURE" },
        { LLAssetType::AT_SOUND, "SOUND" },
        { LLAssetType::AT_CALLINGCARD, "CALLINGCARD" },
        { LLAssetType::AT_LANDMARK, "LANDMARK" },
        { LLAssetType::AT_SCRIPT, "SCRIPT" },
        { LLAssetType::AT_CLOTHING, "CLOTHING" },
        { LLAssetType::AT_OBJECT, "OBJECT" },
        { LLAssetType::AT_NOTECARD, "NOTECARD" },
        { LLAssetType::AT_CATEGORY, "CATEGORY" },
        { LLAssetType::AT_LSL_TEXT, "LSL_TEXT" },
        { LLAssetType::AT_LSL_BYTECODE, "LSL_BYTECODE" },
        { LLAssetType::AT_TEXTURE_TGA, "TEXTURE_TGA" },
        { LLAssetType::AT_BODYPART, "BODYPART" },
        { LLAssetType::AT_SOUND_WAV, "SOUND_WAV" },
        { LLAssetType::AT_IMAGE_TGA, "IMAGE_TGA" },
        { LLAssetType::AT_IMAGE_JPEG, "IMAGE_JPEG" },
        { LLAssetType::AT_ANIMATION, "ANIMATION" },
        { LLAssetType::AT_GESTURE, "GESTURE" },
        { LLAssetType::AT_SIMSTATE, "SIMSTATE" },
        { LLAssetType::AT_LINK, "LINK" },
        { LLAssetType::AT_LINK_FOLDER, "LINK_FOLDER" },
        { LLAssetType::AT_MARKETPLACE_FOLDER, "MARKETPLACE_FOLDER" },
        { LLAssetType::AT_WIDGET, "WIDGET" },
        { LLAssetType::AT_PERSON, "PERSON" },
        { LLAssetType::AT_MESH, "MESH" },
        { LLAssetType::AT_UNKNOWN, "UNKNOWN" }
    };

    asset_type_to_name_t::iterator iter = asset_type_to_name.find(at);
    if (iter != asset_type_to_name.end())
    {
        return iter->second;
    }

    return std::string("UNKNOWN");
}

const std::string idToFilepath(const std::string id, LLAssetType::EType at)
{
    /**
     * For the moment this is just {UUID}_{ASSET_TYPE}.txt but of
     * course,  will be greatly expanded upon
     */
    std::ostringstream ss;
    ss << "00vfs_";
    ss << id;
    ss << "_";
    ss << assetTypeToString(at);
    ss << ".txt";

    const std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, ss.str());

    return filepath;
}

BOOL LLVFile::read(U8 *buffer, S32 bytes, BOOL async, F32 priority)
{
    //CP BEGIN
	//if (! (mMode & READ))
	//{
	//	LL_WARNS() << "Attempt to read from file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;
	//	return FALSE;
	//}

	//if (mHandle != LLVFSThread::nullHandle())
	//{
	//	LL_WARNS() << "Attempt to read from vfile object " << mFileID << " with pending async operation" << LL_ENDL;
	//	return FALSE;
	//}
	//mPriority = priority;
	//CP END

	BOOL success = TRUE;

    mReadComplete = FALSE;

    std::string id;
    mFileID.toString(id);
    const std::string filename = idToFilepath(id, mFileType);

    std::ifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        file.seekg(mPosition, std::ios::beg);

        file.read((char*)buffer, bytes);

        if (file)
        {
            mBytesRead = bytes;
        }
        else
        {
            mBytesRead = file.gcount();
        }

        file.close();

        mPosition += mBytesRead;
        if (!mBytesRead)
        {
            success = FALSE;
        }

        mReadComplete = TRUE;
    }

    return success;

	// We can't do a read while there are pending async writes
	//CP waitForLock(VFSLOCK_APPEND);

    //CP BEGIN
	// *FIX: (?)
	//if (async)
	//{
	//	mHandle = sVFSThread->read(mVFS, mFileID, mFileType, buffer, mPosition, bytes, threadPri());
	//}
	//else
	//{
	//	// We can't do a read while there are pending async writes on this file
	//	mBytesRead = sVFSThread->readImmediate(mVFS, mFileID, mFileType, buffer, mPosition, bytes);
	//	mPosition += mBytesRead;
	//	if (! mBytesRead)
	//	{
	//		success = FALSE;
	//	}
	//}

	//return success;

    //CP END
}

//CP BEGIN
////static
//U8* LLVFile::readFile(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type, S32* bytes_read)
//{
//	U8 *data;
//	LLVFile file(vfs, uuid, type, LLVFile::READ);
//	S32 file_size = file.getSize();
//	if (file_size == 0)
//	{
//		// File is empty.
//		data = NULL;
//	}
//	else
//	{		
//		data = (U8*) ll_aligned_malloc<16>(file_size);
//		file.read(data, file_size);	/* Flawfinder: ignore */ 
//		
//		if (file.getLastBytesRead() != (S32)file_size)
//		{
//			ll_aligned_free<16>(data);
//			data = NULL;
//			file_size = 0;
//		}
//	}
//	if (bytes_read)
//	{
//		*bytes_read = file_size;
//	}
//	return data;
//}
//CP END

//CP BEGIN
//void LLVFile::setReadPriority(const F32 priority)
//{
//	mPriority = priority;
//	if (mHandle != LLVFSThread::nullHandle())
//	{
//		sVFSThread->setPriority(mHandle, threadPri());
//	}
//}
//CP END

BOOL LLVFile::isReadComplete()
{
    if (mReadComplete)
    {
        return TRUE;
    }

    return FALSE;

    //CP BEGIN
	//BOOL res = TRUE;
	//if (mHandle != LLVFSThread::nullHandle())
	//{
	//	LLVFSThread::Request* req = (LLVFSThread::Request*)sVFSThread->getRequest(mHandle);
	//	LLVFSThread::status_t status = req->getStatus();
	//	if (status == LLVFSThread::STATUS_COMPLETE)
	//	{
	//		mBytesRead = req->getBytesRead();
	//		mPosition += mBytesRead;
	//		sVFSThread->completeRequest(mHandle);
	//		mHandle = LLVFSThread::nullHandle();
	//	}
	//	else
	//	{
	//		res = FALSE;
	//	}
	//}
	//return res;
    //CP END
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
    std::string id_str;
    mFileID.toString(id_str);
    const std::string filename = idToFilepath(id_str, mFileType);

    BOOL success = FALSE;

    if (mMode == APPEND)
    {
        std::ofstream ofs(filename, std::ios::app | std::ios::binary);
        if (ofs)
        {
            ofs.write((const char*)buffer, bytes);

            success = TRUE;
        }
    }
    else
    {
        std::ofstream ofs(filename, std::ios::binary);
        if (ofs)
        {
            ofs.write((const char*)buffer, bytes);

            mPosition += bytes;

            success = TRUE;
        }
    }

    return success;

                                                                 //CP BEGIN
	//if (! (mMode & WRITE))
	//{
	//	LL_WARNS() << "Attempt to write to file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;
	//}
	//if (mHandle != LLVFSThread::nullHandle())
	//{
	//	LL_ERRS() << "Attempt to write to vfile object " << mFileID << " with pending async operation" << LL_ENDL;
	//	return FALSE;
	//}
	//BOOL success = TRUE;
	//
	//// *FIX: allow async writes? potential problem wit mPosition...
	//if (mMode == APPEND) // all appends are async (but WRITEs are not)
	//{	
	//	U8* writebuf = new U8[bytes];
	//	memcpy(writebuf, buffer, bytes);
	//	S32 offset = -1;
	//	mHandle = sVFSThread->write(mVFS, mFileID, mFileType,
	//								writebuf, offset, bytes,
	//								LLVFSThread::FLAG_AUTO_COMPLETE | LLVFSThread::FLAG_AUTO_DELETE);
	//	mHandle = LLVFSThread::nullHandle(); // FLAG_AUTO_COMPLETE means we don't track this
	//}
	//else
	//{
	//	// We can't do a write while there are pending reads or writes on this file
	//	waitForLock(VFSLOCK_READ);
	//	waitForLock(VFSLOCK_APPEND);

	//	S32 pos = (mMode & APPEND) == APPEND ? -1 : mPosition;

	//	S32 wrote = sVFSThread->writeImmediate(mVFS, mFileID, mFileType, (U8*)buffer, pos, bytes);

	//	mPosition += wrote;
	//	
	//	if (wrote < bytes)
	//	{	
	//		LL_WARNS() << "Tried to write " << bytes << " bytes, actually wrote " << wrote << LL_ENDL;

	//		success = FALSE;
	//	}
	//}
	//return success;
    //CP END
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
    //CP BEG
	//if (mMode == APPEND)
	//{
	//	LL_WARNS() << "Attempt to seek on append-only file" << LL_ENDL;
	//	return FALSE;
	//}
    //CP END

	if (-1 == origin)
	{
		origin = mPosition;
	}

	S32 new_pos = origin + offset;

	S32 size = getSize(); // Calls waitForLock(VFSLOCK_APPEND)

	if (new_pos > size)
	{
		LL_WARNS() << "Attempt to seek past end of file" << LL_ENDL;

		mPosition = size;
		return FALSE;
	}
	else if (new_pos < 0)
	{
		LL_WARNS() << "Attempt to seek past beginning of file" << LL_ENDL;

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
    std::string id_str;
    mFileID.toString(id_str);
    const std::string filename = idToFilepath(id_str, mFileType);

    S32 file_size = 0;
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
    }
    return file_size;

    //CP BEG
	//waitForLock(VFSLOCK_APPEND);
	//S32 size = mVFS->getSize(mFileID, mFileType);

	//return size;
    //CP END
}

S32 LLVFile::getMaxSize()
{
    // offer up a huge size since we don't care what the max is 
    return INT_MAX;

    //CP BEG
	//S32 size = mVFS->getMaxSize(mFileID, mFileType);

	//return size;
    //CP END
}

BOOL LLVFile::setMaxSize(S32 size)
{
    // we don't care what the max size is so we do nothing
    // and return true to indicate all was okay
    return TRUE;

    //CP BEG
	//if (! (mMode & WRITE))
	//{
	//	LL_WARNS() << "Attempt to change size of file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;

	//	return FALSE;
	//}

	//if (!mVFS->checkAvailable(size))
	//{
	//	//LL_RECORD_BLOCK_TIME(FTM_VFILE_WAIT);
	//	S32 count = 0;
	//	while (sVFSThread->getPending() > 1000)
	//	{
	//		if (count % 100 == 0)
	//		{
	//			LL_INFOS() << "VFS catching up... Pending: " << sVFSThread->getPending() << LL_ENDL;
	//		}
	//		if (sVFSThread->isPaused())
	//		{
	//			sVFSThread->update(0);
	//		}
	//		ms_sleep(10);
	//	}
	//}
	//return mVFS->setMaxSize(mFileID, mFileType, size);
    //CP END
}

BOOL LLVFile::rename(const LLUUID &new_id, const LLAssetType::EType new_type)
{
    std::string old_id_str;
    mFileID.toString(old_id_str);
    const std::string old_filename = idToFilepath(old_id_str, mFileType);

    std::string new_id_str;
    new_id.toString(new_id_str);
    const std::string new_filename = idToFilepath(new_id_str, new_type);

    if (std::rename(old_filename.c_str(), new_filename.c_str()))
    {
        // We would like to return FALSE here indicating the operation
        // failed but the original code does not and doing so seems to
        // break a lot of things so we go with the flow... 
        //return FALSE;
    }
    mFileID = new_id;
    mFileType = new_type;

    return TRUE;
    //CP BEG
	//if (! (mMode & WRITE))
	//{
	//	LL_WARNS() << "Attempt to rename file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;

	//	return FALSE;
	//}

	//if (mHandle != LLVFSThread::nullHandle())
	//{
	//	LL_WARNS() << "Renaming file with pending async read" << LL_ENDL;
	//}

	//waitForLock(VFSLOCK_READ);
	//waitForLock(VFSLOCK_APPEND);

	//// we need to release / replace our own lock
	//// since the renamed file will inherit locks from the new name
	//mVFS->decLock(mFileID, mFileType, VFSLOCK_OPEN);
	//mVFS->renameFile(mFileID, mFileType, new_id, new_type);
	//mVFS->incLock(new_id, new_type, VFSLOCK_OPEN);
	//
	//mFileID = new_id;
	//mFileType = new_type;

	//return TRUE;
    //CP END
}

BOOL LLVFile::remove()
{
    std::string id_str;
    mFileID.toString(id_str);
    const std::string filename = idToFilepath(id_str, mFileType);

    std::remove(filename.c_str());
    // TODO: check if file was not removed and return false - maybe we don't care?

    return TRUE;

    //CP BEG
    // 	LL_INFOS() << "Removing file " << mFileID << LL_ENDL;
	//if (! (mMode & WRITE))
	//{
	//	// Leaving paranoia warning just because this should be a very infrequent
	//	// operation.
	//	LL_WARNS() << "Remove file " << mFileID << " opened with mode " << std::hex << mMode << std::dec << LL_ENDL;
	//}

	//if (mHandle != LLVFSThread::nullHandle())
	//{
	//	LL_WARNS() << "Removing file with pending async read" << LL_ENDL;
	//}
	//
	//// why not seek back to the beginning of the file too?
	//mPosition = 0;

	//waitForLock(VFSLOCK_READ);
	//waitForLock(VFSLOCK_APPEND);
	//mVFS->removeFile(mFileID, mFileType);

	//return TRUE;
    //CP END
}

// static
void LLVFile::initClass(LLVFSThread* vfsthread)
{
    //CP BEG
	//if (!vfsthread)
	//{
	//	if (LLVFSThread::sLocal != NULL)
	//	{
	//		vfsthread = LLVFSThread::sLocal;
	//	}
	//	else
	//	{
	//		vfsthread = new LLVFSThread();
	//		sAllocdVFSThread = TRUE;
	//	}
	//}
	//sVFSThread = vfsthread;
    //CP END
}

// static
void LLVFile::cleanupClass()
{
    //CP BEG
	//if (sAllocdVFSThread)
	//{
	//	delete sVFSThread;
	//}
	//sVFSThread = NULL;
    //CP END
}

bool LLVFile::isLocked(EVFSLock lock)
{
    // I don't think we care about this test since there is no locking
    return FALSE;

	//CP return mVFS->isLocked(mFileID, mFileType, lock) ? true : false;
}

void LLVFile::waitForLock(EVFSLock lock)
{
    //CP BEG
	////LL_RECORD_BLOCK_TIME(FTM_VFILE_WAIT);
	//// spin until the lock clears
	//while (isLocked(lock))
	//{
	//	if (sVFSThread->isPaused())
	//	{
	//		sVFSThread->update(0);
	//	}
	//	ms_sleep(1);
	//}
    //CP END
}
