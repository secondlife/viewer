/** 
 * @file llxfer_vfile.cpp
 * @brief implementation of LLXfer_VFile class for a single xfer (vfile).
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

#include "llxfer_vfile.h"
#include "lluuid.h"
#include "llerror.h"
#include "llmath.h"
#include "llfilesystem.h"
#include "lldir.h"

// size of chunks read from/written to disk
const U32 LL_MAX_XFER_FILE_BUFFER = 65536;

///////////////////////////////////////////////////////////

LLXfer_VFile::LLXfer_VFile ()
: LLXfer(-1)
{
    init(LLUUID::null, LLAssetType::AT_NONE);
}

LLXfer_VFile::LLXfer_VFile (const LLUUID &local_id, LLAssetType::EType type)
: LLXfer(-1)
{
    init(local_id, type);
}

///////////////////////////////////////////////////////////

LLXfer_VFile::~LLXfer_VFile ()
{
    cleanup();
}

///////////////////////////////////////////////////////////

void LLXfer_VFile::init (const LLUUID &local_id, LLAssetType::EType type)
{
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
    if (mTempID.notNull() &&
        mDeleteTempFile)
    {
        if (LLFileSystem::getExists(mTempID, mType))
        {
            LLFileSystem file(mTempID, mType, LLFileSystem::WRITE);
            file.remove();
        }
        else
        {
            LL_WARNS("Xfer") << "LLXfer_VFile::cleanup() can't open to delete cache file " << mTempID << "." << LLAssetType::lookup(mType)      
                << ", mRemoteID is " << mRemoteID << LL_ENDL;
        }
    }

    delete mVFile;
    mVFile = NULL;

    LLXfer::cleanup();
}

///////////////////////////////////////////////////////////

S32 LLXfer_VFile::initializeRequest(U64 xfer_id,
                                    const LLUUID& local_id,
                                    const LLUUID& remote_id,
                                    LLAssetType::EType type,
                                    const LLHost& remote_host,
                                    void (*callback)(void**,S32,LLExtStat),
                                    void** user_data)
{
    S32 retval = 0;  // presume success
    
    mRemoteHost = remote_host;

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

    LL_INFOS("Xfer") << "Requesting " << mName << LL_ENDL;

    if (mBuffer)
    {
        delete[] mBuffer;
        mBuffer = NULL;
    }

    mBuffer = new char[LL_MAX_XFER_FILE_BUFFER];

    mBufferLength = 0;
    mPacketNum = 0;
    mTempID.generate();
    mDeleteTempFile = TRUE;
    mStatus = e_LL_XFER_PENDING;
    return retval;
}

//////////////////////////////////////////////////////////

S32 LLXfer_VFile::startDownload()
{
    S32 retval = 0;  // presume success

    // Don't need to create the file here, it will happen when data arrives

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

//  cout << "Sending file: " << mLocalFilename << endl;

    delete [] mBuffer;
    mBuffer = new char[LL_MAX_XFER_FILE_BUFFER];

    mBufferLength = 0;
    mBufferStartOffset = 0; 
    
    delete mVFile;
    mVFile = NULL;
    if(LLFileSystem::getExists(mLocalID, mType))
    {
        mVFile = new LLFileSystem(mLocalID, mType, LLFileSystem::READ);

        if (mVFile->getSize() <= 0)
        {
            LL_WARNS("Xfer") << "LLXfer_VFile::startSend() cache file " << mLocalID << "." << LLAssetType::lookup(mType)        
                << " has unexpected file size of " << mVFile->getSize() << LL_ENDL;
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
        LL_WARNS("Xfer") << "LLXfer_VFile::startSend() can't read cache file " << mLocalID << "." << LLAssetType::lookup(mType) << LL_ENDL;
        retval = LL_ERR_FILE_NOT_FOUND;
    }

    return (retval);
}

///////////////////////////////////////////////////////////

void LLXfer_VFile::closeFileHandle()
{
    if (mVFile)
    {
        delete mVFile;
        mVFile = NULL;
    }
}

///////////////////////////////////////////////////////////

S32 LLXfer_VFile::reopenFileHandle()
{
    S32 retval = LL_ERR_NOERR;  // presume success

    if (mVFile == NULL)
    {
        if (LLFileSystem::getExists(mLocalID, mType))
        {
            mVFile = new LLFileSystem(mLocalID, mType, LLFileSystem::READ);
        }
        else
        {
            LL_WARNS("Xfer") << "LLXfer_VFile::reopenFileHandle() can't read cache file " << mLocalID << "." << LLAssetType::lookup(mType) << LL_ENDL;
            retval = LL_ERR_FILE_NOT_FOUND;
        }
    }

    return retval;
}


///////////////////////////////////////////////////////////

void LLXfer_VFile::setXferSize (S32 xfer_size)
{   
    LLXfer::setXferSize(xfer_size);

    // Don't do this on the server side, where we have a persistent mVFile
    // It would be nice if LLXFers could tell which end of the pipe they were
    if (! mVFile)
    {
        LLFileSystem file(mTempID, mType, LLFileSystem::APPEND);
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
            LL_WARNS("Xfer") << "VFile Xfer Can't seek to position " << start_position << ", file length " << mVFile->getSize() << LL_ENDL;
            LL_WARNS("Xfer") << "While sending file " << mLocalID << LL_ENDL;
            return -1;
        }
        
        if (mVFile->read((U8*)mBuffer, LL_MAX_XFER_FILE_BUFFER))        /* Flawfinder : ignore */
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
        LLFileSystem file(mTempID, mType, LLFileSystem::APPEND);

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
        if (LLFileSystem::getExists(mTempID, mType))
        {
            LLFileSystem file(mTempID, mType, LLFileSystem::WRITE);
            if (!file.rename(mLocalID, mType))
            {
                LL_WARNS("Xfer") << "Cache rename of temp file failed: unable to rename " << mTempID << " to " << mLocalID << LL_ENDL;
            }
            else
            {                                   
                // Rename worked: the original file is gone.   Clear mDeleteTempFile
                // so we don't attempt to delete the file in cleanup()
                mDeleteTempFile = FALSE;
            }
        }
        else
        {
            LL_WARNS("Xfer") << "LLXfer_VFile::processEOF() can't open for renaming cache file " << mTempID << "." << LLAssetType::lookup(mType) << LL_ENDL;
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

