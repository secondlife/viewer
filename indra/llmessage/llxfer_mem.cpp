/** 
 * @file llxfer_mem.cpp
 * @brief implementation of LLXfer_Mem class for a single xfer
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llxfer_mem.h"
#include "lluuid.h"
#include "llerror.h"
#include "llmath.h"

///////////////////////////////////////////////////////////

LLXfer_Mem::LLXfer_Mem ()
: LLXfer(-1)
{
	init();
}

///////////////////////////////////////////////////////////

LLXfer_Mem::~LLXfer_Mem ()
{
	free();
}

///////////////////////////////////////////////////////////

void LLXfer_Mem::init ()
{
	mRemoteFilename[0] = '\0';
	mRemotePath = LL_PATH_NONE;
	mDeleteRemoteOnCompletion = FALSE;
}
	
///////////////////////////////////////////////////////////

void LLXfer_Mem::free ()
{
	LLXfer::free();
}

///////////////////////////////////////////////////////////

void LLXfer_Mem::setXferSize (S32 xfer_size)
{	
	mXferSize = xfer_size;

	delete[] mBuffer;
	mBuffer = new char[xfer_size];
	
	mBufferLength = 0;
	mBufferStartOffset = 0;	
	mBufferContainsEOF = TRUE;

//	cout << "starting transfer of size: " << xfer_size << endl;
}

///////////////////////////////////////////////////////////

U64 LLXfer_Mem::registerXfer(U64 xfer_id, const void *datap, const S32 length)
{
	mID = xfer_id;

	if (datap)
	{
		setXferSize(length);
		if (mBuffer)
		{
			memcpy(mBuffer,datap,length);		/* Flawfinder : ignore */
			mBufferLength = length;
		}
		else
		{
			xfer_id = 0;
		}
	}
	
	mStatus = e_LL_XFER_REGISTERED;
	return (xfer_id);
}

S32 LLXfer_Mem::startSend (U64 xfer_id, const LLHost &remote_host)
{
	S32 retval = LL_ERR_NOERR;  // presume success
	
	if (mXferSize <= 0)
	{
		return LL_ERR_FILE_EMPTY;
	}

    mRemoteHost = remote_host;
	mID = xfer_id;
   	mPacketNum = -1;

//	cout << "Sending file: " << getName() << endl;

	mStatus = e_LL_XFER_PENDING;

	return (retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer_Mem::processEOF()
{
	S32 retval = 0;

	mStatus = e_LL_XFER_COMPLETE;

	llinfos << "xfer complete: " << getName() << llendl;

	if (mCallback)
	{
		mCallback((void *)mBuffer,mBufferLength,mCallbackDataHandle,mCallbackResult);
	}

	return(retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer_Mem::initializeRequest(U64 xfer_id,
								  const std::string& remote_filename,
								  ELLPath remote_path,
								  const LLHost& remote_host,
								  BOOL delete_remote_on_completion,
								  void (*callback)(void*,S32,void**,S32),
								  void** user_data)
{
 	S32 retval = 0;  // presume success
	
	mRemoteHost = remote_host;

	// create a temp filename string using a GUID
	mID = xfer_id;
	mCallback = callback;
	mCallbackDataHandle = user_data;
	mCallbackResult = LL_ERR_NOERR;

	strncpy(mRemoteFilename, remote_filename.c_str(), LL_MAX_PATH-1);
	mRemoteFilename[LL_MAX_PATH-1] = '\0'; // stupid strncpy.
	mRemotePath = remote_path;
	mDeleteRemoteOnCompletion = delete_remote_on_completion;

	llinfos << "Requesting file: " << remote_filename << llendl;

	delete [] mBuffer;
	mBuffer = NULL;

	mBufferLength = 0;
	mPacketNum = 0;
 	mStatus = e_LL_XFER_PENDING;
	return retval;
}

//////////////////////////////////////////////////////////

S32 LLXfer_Mem::startDownload()
{
 	S32 retval = 0;  // presume success
	gMessageSystem->newMessageFast(_PREHASH_RequestXfer);
	gMessageSystem->nextBlockFast(_PREHASH_XferID);
	gMessageSystem->addU64Fast(_PREHASH_ID, mID);
	gMessageSystem->addStringFast(_PREHASH_Filename, mRemoteFilename);
	gMessageSystem->addU8("FilePath", (U8) mRemotePath);
	gMessageSystem->addBOOL("DeleteOnCompletion", mDeleteRemoteOnCompletion);
	gMessageSystem->addBOOL("UseBigPackets", BOOL(mChunkSize == LL_XFER_LARGE_PAYLOAD));
	gMessageSystem->addUUIDFast(_PREHASH_VFileID, LLUUID::null);
	gMessageSystem->addS16Fast(_PREHASH_VFileType, -1);

	gMessageSystem->sendReliable(mRemoteHost);		
	mStatus = e_LL_XFER_IN_PROGRESS;

	return (retval);
}

//////////////////////////////////////////////////////////

U32 LLXfer_Mem::getXferTypeTag()
{
	return LLXfer::XFER_MEM;
}













