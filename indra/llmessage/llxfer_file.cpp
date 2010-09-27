/** 
 * @file llxfer_file.cpp
 * @brief implementation of LLXfer_File class for a single xfer (file)
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#if !LL_WINDOWS
#include <errno.h>
#include <unistd.h>
#endif

#include "llxfer_file.h"
#include "lluuid.h"
#include "llerror.h"
#include "llmath.h"
#include "llstring.h"
#include "lldir.h"

// size of chunks read from/written to disk
const U32 LL_MAX_XFER_FILE_BUFFER = 65536;

// local function to copy a file
S32 copy_file(const std::string& from, const std::string& to);

///////////////////////////////////////////////////////////

LLXfer_File::LLXfer_File (S32 chunk_size)
: LLXfer(chunk_size)
{
	init(LLStringUtil::null, FALSE, chunk_size);
}

LLXfer_File::LLXfer_File (const std::string& local_filename, BOOL delete_local_on_completion, S32 chunk_size)
: LLXfer(chunk_size)
{
	init(local_filename, delete_local_on_completion, chunk_size);
}

///////////////////////////////////////////////////////////

LLXfer_File::~LLXfer_File ()
{
	cleanup();
}

///////////////////////////////////////////////////////////

void LLXfer_File::init (const std::string& local_filename, BOOL delete_local_on_completion, S32 chunk_size)
{

	mFp = NULL;
	mLocalFilename.clear();
	mRemoteFilename.clear();
	mRemotePath = LL_PATH_NONE;
	mTempFilename.clear();
	mDeleteLocalOnCompletion = FALSE;
	mDeleteRemoteOnCompletion = FALSE;

	if (!local_filename.empty())
	{
		mLocalFilename =  local_filename.substr(0,LL_MAX_PATH-1);

		// You can only automatically delete .tmp file as a safeguard against nasty messages.
		std::string exten = mLocalFilename.substr(mLocalFilename.length()-4, 4);
		mDeleteLocalOnCompletion = (delete_local_on_completion && exten == ".tmp");
	}
}
	
///////////////////////////////////////////////////////////

void LLXfer_File::cleanup ()
{
	if (mFp)
	{
		fclose(mFp);
		mFp = NULL;
	}

	LLFile::remove(mTempFilename);

	if (mDeleteLocalOnCompletion)
	{
		lldebugs << "Removing file: " << mLocalFilename << llendl;
		LLFile::remove(mLocalFilename);
	}
	else
	{
		lldebugs << "Keeping local file: " << mLocalFilename << llendl;
	}

	LLXfer::cleanup();
}

///////////////////////////////////////////////////////////

S32 LLXfer_File::initializeRequest(U64 xfer_id,
				   const std::string& local_filename,
				   const std::string& remote_filename,
				   ELLPath remote_path,
				   const LLHost& remote_host,
				   BOOL delete_remote_on_completion,
				   void (*callback)(void**,S32,LLExtStat),
				   void** user_data)
{
 	S32 retval = 0;  // presume success
	
	mID = xfer_id;
	mLocalFilename = local_filename;
	mRemoteFilename = remote_filename;
	mRemotePath = remote_path;
	mRemoteHost = remote_host;
	mDeleteRemoteOnCompletion = delete_remote_on_completion;

	mTempFilename = gDirUtilp->getTempFilename();

	mCallback = callback;
	mCallbackDataHandle = user_data;
	mCallbackResult = LL_ERR_NOERR;

	llinfos << "Requesting xfer from " << remote_host << " for file: " << mLocalFilename << llendl;

	if (mBuffer)
	{
		delete(mBuffer);
		mBuffer = NULL;
	}

	mBuffer = new char[LL_MAX_XFER_FILE_BUFFER];
	mBufferLength = 0;

	mPacketNum = 0;

 	mStatus = e_LL_XFER_PENDING;
	return retval;
}

///////////////////////////////////////////////////////////

S32 LLXfer_File::startDownload()
{
 	S32 retval = 0;  // presume success
	mFp = LLFile::fopen(mTempFilename,"w+b");		/* Flawfinder : ignore */
	if (mFp)
	{
		fclose(mFp);
		mFp = NULL;

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
	}
	else
	{
		llwarns << "Couldn't create file to be received!" << llendl;
		retval = -1;
	}

	return (retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer_File::startSend (U64 xfer_id, const LLHost &remote_host)
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
	
	mFp = LLFile::fopen(mLocalFilename,"rb");		/* Flawfinder : ignore */
	if (mFp)
	{
		fseek(mFp,0,SEEK_END);
	
		S32 file_size = ftell(mFp);
		if (file_size <= 0)
		{
			return LL_ERR_FILE_EMPTY;
		}
		setXferSize(file_size);

		fseek(mFp,0,SEEK_SET);
	}
	else
	{
		llinfos << "Warning: " << mLocalFilename << " not found." << llendl;
		return (LL_ERR_FILE_NOT_FOUND);
	}

	mStatus = e_LL_XFER_PENDING;

	return (retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer_File::getMaxBufferSize ()
{
	return(LL_MAX_XFER_FILE_BUFFER);
}

///////////////////////////////////////////////////////////

S32 LLXfer_File::suck(S32 start_position)
{
	S32 retval = 0;

	if (mFp)
	{
		// grab a buffer from the right place in the file
		fseek (mFp,start_position,SEEK_SET);
		
		mBufferLength = (U32)fread(mBuffer,1,LL_MAX_XFER_FILE_BUFFER,mFp);
		mBufferStartOffset = start_position;
			
		if (feof(mFp))
		{
			mBufferContainsEOF = TRUE;
		}
		else
		{
			mBufferContainsEOF = FALSE;
		}		
	}
	else
	{
		retval = -1;
	}

	return (retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer_File::flush()
{
	S32 retval = 0;
	if (mBufferLength)
	{
		if (mFp)
		{
			llerrs << "Overwriting open file pointer!" << llendl;
		}
		mFp = LLFile::fopen(mTempFilename,"a+b");		/* Flawfinder : ignore */

		if (mFp)
		{
			if (fwrite(mBuffer,1,mBufferLength,mFp) != mBufferLength)
			{
				llwarns << "Short write" << llendl;
			}
			
//			llinfos << "******* wrote " << mBufferLength << " bytes of file xfer" << llendl;
			fclose(mFp);
			mFp = NULL;
			
			mBufferLength = 0;
		}
		else
		{
			llwarns << "LLXfer_File::flush() unable to open " << mTempFilename << " for writing!" << llendl;
			retval = LL_ERR_CANNOT_OPEN_FILE;
		}
	}
	return (retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer_File::processEOF()
{
	S32 retval = 0;
	mStatus = e_LL_XFER_COMPLETE;

	S32 flushval = flush();

	// If we have no other errors, our error becomes the error generated by
	// flush.
	if (!mCallbackResult)
	{
		mCallbackResult = flushval;
	}

	LLFile::remove(mLocalFilename);

	if (!mCallbackResult)
	{
		if (LLFile::rename(mTempFilename,mLocalFilename))
		{
#if !LL_WINDOWS
			S32 error_number = errno;
			llinfos << "Rename failure (" << error_number << ") - "
					<< mTempFilename << " to " << mLocalFilename << llendl;
			if(EXDEV == error_number)
			{
				if(copy_file(mTempFilename, mLocalFilename) == 0)
				{
					llinfos << "Rename across mounts; copying+unlinking the file instead." << llendl;
					unlink(mTempFilename.c_str());
				}
				else
				{
					llwarns << "Copy failure - " << mTempFilename << " to "
							<< mLocalFilename << llendl;
				}
			}
			else
			{
				//LLFILE* fp = LLFile::fopen(mTempFilename, "r");
				//llwarns << "File " << mTempFilename << " does "
				//		<< (!fp ? "not" : "" ) << " exit." << llendl;
				//if(fp) fclose(fp);
				//fp = LLFile::fopen(mLocalFilename, "r");
				//llwarns << "File " << mLocalFilename << " does "
				//		<< (!fp ? "not" : "" ) << " exit." << llendl;
				//if(fp) fclose(fp);
				llwarns << "Rename fatally failed, can only handle EXDEV ("
						<< EXDEV << ")" << llendl;
			}
#else
			llwarns << "Rename failure - " << mTempFilename << " to "
					<< mLocalFilename << llendl;
#endif
		}
	}

	if (mFp)
	{
		fclose(mFp);
		mFp = NULL;
	}

	retval = LLXfer::processEOF();

	return(retval);
}

///////////////////////////////////////////////////////////

BOOL LLXfer_File::matchesLocalFilename(const std::string& filename) 
{
	return (filename == mLocalFilename);
}

///////////////////////////////////////////////////////////

BOOL LLXfer_File::matchesRemoteFilename(const std::string& filename, ELLPath remote_path) 
{
	return ((filename == mRemoteFilename) && (remote_path == mRemotePath));
}


///////////////////////////////////////////////////////////

std::string LLXfer_File::getFileName() 
{
	return mLocalFilename;
}

///////////////////////////////////////////////////////////

// hacky - doesn't matter what this is
// as long as it's different from the other classes
U32 LLXfer_File::getXferTypeTag()
{
	return LLXfer::XFER_FILE;
}

///////////////////////////////////////////////////////////

#if !LL_WINDOWS

// This is really close to, but not quite a general purpose copy
// function. It does not really spam enough information, but is useful
// for this cpp file, because this should never be called in a
// production environment.
S32 copy_file(const std::string& from, const std::string& to)
{
	S32 rv = 0;
	LLFILE* in = LLFile::fopen(from, "rb");	/*Flawfinder: ignore*/
	LLFILE* out = LLFile::fopen(to, "wb");	/*Flawfinder: ignore*/
	if(in && out)
	{
		S32 read = 0;
		const S32 COPY_BUFFER_SIZE = 16384;
		U8 buffer[COPY_BUFFER_SIZE];
		while(((read = fread(buffer, 1, sizeof(buffer), in)) > 0)
			  && (fwrite(buffer, 1, read, out) == (U32)read));		/* Flawfinder : ignore */
		if(ferror(in) || ferror(out)) rv = -2;
	}
	else
	{
		rv = -1;
	}
	if(in) fclose(in);
	if(out) fclose(out);
	return rv;
}
#endif
