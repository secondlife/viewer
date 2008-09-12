/** 
 * @file llxfer_vfile.cpp
 * @brief implementation of LLXfer_VFile class for a single xfer (vfile).
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#include "linden_common.h"

#include "llxfer_vfile.h"
#include "lluuid.h"
#include "llerror.h"
#include "llmath.h"
#include "llvfile.h"
#include "llvfs.h"
#include "lldir.h"

// size of chunks read from/written to disk
const U32 LL_MAX_XFER_FILE_BUFFER = 65536;

///////////////////////////////////////////////////////////

LLXfer_VFile::LLXfer_VFile ()
: LLXfer(-1)
{
	init(NULL, LLUUID::null, LLAssetType::AT_NONE);
}

LLXfer_VFile::LLXfer_VFile (LLVFS *vfs, const LLUUID &local_id, LLAssetType::EType type)
: LLXfer(-1)
{
	init(vfs, local_id, type);
}

///////////////////////////////////////////////////////////

LLXfer_VFile::~LLXfer_VFile ()
{
	cleanup();
}

///////////////////////////////////////////////////////////

void LLXfer_VFile::init (LLVFS *vfs, const LLUUID &local_id, LLAssetType::EType type)
{

	mVFS = vfs;
	mLocalID = local_id;
	mType = type;

	mVFile = NULL;

	std::string id_string;
	mLocalID.toString(id_string);

	mName = llformat("VFile %s:%s", id_string.c_str(), LLAssetType::lookup(mType));
}
	
///////////////////////////////////////////////////////////

void LLXfer_VFile::cleanup ()
{
	LLVFile file(mVFS, mTempID, mType, LLVFile::WRITE);
	file.remove();

	delete mVFile;
	mVFile = NULL;

	LLXfer::cleanup();
}

///////////////////////////////////////////////////////////

S32 LLXfer_VFile::initializeRequest(U64 xfer_id,
									LLVFS* vfs,
									const LLUUID& local_id,
									const LLUUID& remote_id,
									LLAssetType::EType type,
									const LLHost& remote_host,
									void (*callback)(void**,S32,LLExtStat),
									void** user_data)
{
 	S32 retval = 0;  // presume success
	
	mRemoteHost = remote_host;

	mVFS = vfs;
	mLocalID = local_id;
	mRemoteID = remote_id;
	mType = type;

	mID = xfer_id;
	mCallback = callback;
	mCallbackDataHandle = user_data;
	mCallbackResult = LL_ERR_NOERR;

	std::string id_string;
	mLocalID.toString(id_string);

	mName = llformat("VFile %s:%s", id_string.c_str(), LLAssetType::lookup(mType));

	llinfos << "Requesting " << mName << llendl;

	if (mBuffer)
	{
		delete[] mBuffer;
		mBuffer = NULL;
	}

	mBuffer = new char[LL_MAX_XFER_FILE_BUFFER];

	mBufferLength = 0;
	mPacketNum = 0;
	mTempID.generate();
 	mStatus = e_LL_XFER_PENDING;
	return retval;
}

//////////////////////////////////////////////////////////

S32 LLXfer_VFile::startDownload()
{
 	S32 retval = 0;  // presume success
	LLVFile file(mVFS, mTempID, mType, LLVFile::APPEND);

	gMessageSystem->newMessageFast(_PREHASH_RequestXfer);
	gMessageSystem->nextBlockFast(_PREHASH_XferID);
	gMessageSystem->addU64Fast(_PREHASH_ID, mID);
	gMessageSystem->addStringFast(_PREHASH_Filename, "");
	gMessageSystem->addU8("FilePath", (U8) LL_PATH_NONE);
	gMessageSystem->addBOOL("DeleteOnCompletion", FALSE);
	gMessageSystem->addBOOL("UseBigPackets", BOOL(mChunkSize == LL_XFER_LARGE_PAYLOAD));
	gMessageSystem->addUUIDFast(_PREHASH_VFileID, mRemoteID);
	gMessageSystem->addS16Fast(_PREHASH_VFileType, (S16)mType);

	gMessageSystem->sendReliable(mRemoteHost);		
	mStatus = e_LL_XFER_IN_PROGRESS;

	return (retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer_VFile::startSend (U64 xfer_id, const LLHost &remote_host)
{
	S32 retval = LL_ERR_NOERR;  // presume success
	
    mRemoteHost = remote_host;
	mID = xfer_id;
   	mPacketNum = -1;

//	cout << "Sending file: " << mLocalFilename << endl;

	delete [] mBuffer;
	mBuffer = new char[LL_MAX_XFER_FILE_BUFFER];

	mBufferLength = 0;
	mBufferStartOffset = 0;	
	
	delete mVFile;
	mVFile = NULL;
	if(mVFS->getExists(mLocalID, mType))
	{
		mVFile = new LLVFile(mVFS, mLocalID, mType, LLVFile::READ);

		if (mVFile->getSize() <= 0)
		{
			delete mVFile;
			mVFile = NULL;

			return LL_ERR_FILE_EMPTY;
		}
	}

	if(mVFile)
	{
		setXferSize(mVFile->getSize());
		mStatus = e_LL_XFER_PENDING;
	}
	else
	{
		retval = LL_ERR_FILE_NOT_FOUND;
	}

	return (retval);
}

///////////////////////////////////////////////////////////
void LLXfer_VFile::setXferSize (S32 xfer_size)
{	
	LLXfer::setXferSize(xfer_size);

	// Don't do this on the server side, where we have a persistent mVFile
	// It would be nice if LLXFers could tell which end of the pipe they were
	if (! mVFile)
	{
		LLVFile file(mVFS, mTempID, mType, LLVFile::APPEND);
		file.setMaxSize(xfer_size);
	}
}

///////////////////////////////////////////////////////////

S32 LLXfer_VFile::getMaxBufferSize ()
{
	return(LL_MAX_XFER_FILE_BUFFER);
}

///////////////////////////////////////////////////////////

S32 LLXfer_VFile::suck(S32 start_position)
{
	S32 retval = 0;

	if (mVFile)
	{
		// grab a buffer from the right place in the file
		if (! mVFile->seek(start_position, 0))
		{
			llwarns << "VFile Xfer Can't seek to position " << start_position << ", file length " << mVFile->getSize() << llendl;
			llwarns << "While sending file " << mLocalID << llendl;
			return -1;
		}
		
		if (mVFile->read((U8*)mBuffer, LL_MAX_XFER_FILE_BUFFER))		/* Flawfinder : ignore */
		{
			mBufferLength = mVFile->getLastBytesRead();
			mBufferStartOffset = start_position;

			mBufferContainsEOF = mVFile->eof();
		}
		else
		{
			retval = -1;
		}
	}
	else
	{
		retval = -1;
	}

	return (retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer_VFile::flush()
{
	S32 retval = 0;
	if (mBufferLength)
	{
		LLVFile file(mVFS, mTempID, mType, LLVFile::APPEND);

		file.write((U8*)mBuffer, mBufferLength);
			
		mBufferLength = 0;
	}
	return (retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer_VFile::processEOF()
{
	S32 retval = 0;
	mStatus = e_LL_XFER_COMPLETE;

	flush();

	if (!mCallbackResult)
	{
		LLVFile file(mVFS, mTempID, mType, LLVFile::WRITE);
		if (! file.rename(mLocalID, mType))
		{
			llinfos << "copy from temp file failed: unable to rename to " << mLocalID << llendl;
		}

	}

	if (mVFile)
	{
		delete mVFile;
		mVFile = NULL;
	}

	retval = LLXfer::processEOF();

	return(retval);
}

////////////////////////////////////////////////////////////

BOOL LLXfer_VFile::matchesLocalFile(const LLUUID &id, LLAssetType::EType type)
{
	return (id == mLocalID && type == mType);
}

//////////////////////////////////////////////////////////

BOOL LLXfer_VFile::matchesRemoteFile(const LLUUID &id, LLAssetType::EType type)
{
	return (id == mRemoteID && type == mType);
}

//////////////////////////////////////////////////////////

std::string LLXfer_VFile::getFileName() 
{
	return mName;
}

//////////////////////////////////////////////////////////

// hacky - doesn't matter what this is
// as long as it's different from the other classes
U32 LLXfer_VFile::getXferTypeTag()
{
	return LLXfer::XFER_VFILE;
}
