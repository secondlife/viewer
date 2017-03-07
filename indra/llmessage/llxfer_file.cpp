/** 
 * @file llxfer_file.cpp
 * @brief implementation of LLXfer_File class for a single xfer (file)
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

	LLFile::remove(mTempFilename, ENOENT);

	if (mDeleteLocalOnCompletion)
	{
		LL_DEBUGS() << "Removing file: " << mLocalFilename << LL_ENDL;
		LLFile::remove(mLocalFilename, ENOENT);
	}
	else
	{
		LL_DEBUGS() << "Keeping local file: " << mLocalFilename << LL_ENDL;
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

	LL_INFOS() << "Requesting xfer from " << remote_host << " for file: " << mLocalFilename << LL_ENDL;

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
		LL_WARNS() << "Couldn't create file to be received!" << LL_ENDL;
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
		LL_INFOS() << "Warning: " << mLocalFilename << " not found." << LL_ENDL;
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
			LL_ERRS() << "Overwriting open file pointer!" << LL_ENDL;
		}
		mFp = LLFile::fopen(mTempFilename,"a+b");		/* Flawfinder : ignore */

		if (mFp)
		{
			if (fwrite(mBuffer,1,mBufferLength,mFp) != mBufferLength)
			{
				LL_WARNS() << "Short write" << LL_ENDL;
			}
			
//			LL_INFOS() << "******* wrote " << mBufferLength << " bytes of file xfer" << LL_ENDL;
			fclose(mFp);
			mFp = NULL;
			
			mBufferLength = 0;
		}
		else
		{
			LL_WARNS() << "LLXfer_File::flush() unable to open " << mTempFilename << " for writing!" << LL_ENDL;
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

	LLFile::remove(mLocalFilename, ENOENT);

	if (!mCallbackResult)
	{
		if (LLFile::rename(mTempFilename,mLocalFilename))
		{
#if !LL_WINDOWS
			S32 error_number = errno;
			LL_INFOS() << "Rename failure (" << error_number << ") - "
					<< mTempFilename << " to " << mLocalFilename << LL_ENDL;
			if(EXDEV == error_number)
			{
				if(copy_file(mTempFilename, mLocalFilename) == 0)
				{
					LL_INFOS() << "Rename across mounts; copying+unlinking the file instead." << LL_ENDL;
					unlink(mTempFilename.c_str());
				}
				else
				{
					LL_WARNS() << "Copy failure - " << mTempFilename << " to "
							<< mLocalFilename << LL_ENDL;
				}
			}
			else
			{
				//LLFILE* fp = LLFile::fopen(mTempFilename, "r");
				//LL_WARNS() << "File " << mTempFilename << " does "
				//		<< (!fp ? "not" : "" ) << " exit." << LL_ENDL;
				//if(fp) fclose(fp);
				//fp = LLFile::fopen(mLocalFilename, "r");
				//LL_WARNS() << "File " << mLocalFilename << " does "
				//		<< (!fp ? "not" : "" ) << " exit." << LL_ENDL;
				//if(fp) fclose(fp);
				LL_WARNS() << "Rename fatally failed, can only handle EXDEV ("
						<< EXDEV << ")" << LL_ENDL;
			}
#else
			LL_WARNS() << "Rename failure - " << mTempFilename << " to "
					<< mLocalFilename << LL_ENDL;
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
