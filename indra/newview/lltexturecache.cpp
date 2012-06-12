/** 
 * @file lltexturecache.cpp
 * @brief Object which handles local texture caching
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lltexturecache.h"

#include "llapr.h"
#include "lldir.h"
#include "llimage.h"
#include "lllfsthread.h"
#include "llviewercontrol.h"

// Included to allow LLTextureCache::purgeTextures() to pause watchdog timeout
#include "llappviewer.h" 
#include "llmemory.h"

// Cache organization:
// cache/texture.entries
//  Unordered array of Entry structs
// cache/texture.cache
//  First TEXTURE_CACHE_ENTRY_SIZE bytes of each texture in texture.entries in same order
// cache/textures/[0-F]/UUID.texture
//  Actual texture body files

//note: there is no good to define 1024 for TEXTURE_CACHE_ENTRY_SIZE while FIRST_PACKET_SIZE is 600 on sim side.
const S32 TEXTURE_CACHE_ENTRY_SIZE = FIRST_PACKET_SIZE;//1024;
const F32 TEXTURE_CACHE_PURGE_AMOUNT = .20f; // % amount to reduce the cache by when it exceeds its limit
const F32 TEXTURE_CACHE_LRU_SIZE = .10f; // % amount for LRU list (low overhead to regenerate)
const S32 TEXTURE_FAST_CACHE_ENTRY_OVERHEAD = sizeof(S32) * 4; //w, h, c, level
const S32 TEXTURE_FAST_CACHE_ENTRY_SIZE = 16 * 16 * 4 + TEXTURE_FAST_CACHE_ENTRY_OVERHEAD;

class LLTextureCacheWorker : public LLWorkerClass
{
	friend class LLTextureCache;

private:
	class ReadResponder : public LLLFSThread::Responder
	{
	public:
		ReadResponder(LLTextureCache* cache, handle_t handle) : mCache(cache), mHandle(handle) {}
		~ReadResponder() {}
		void completed(S32 bytes)
		{
			mCache->lockWorkers();
			LLTextureCacheWorker* reader = mCache->getReader(mHandle);
			if (reader) reader->ioComplete(bytes);
			mCache->unlockWorkers();
		}
		LLTextureCache* mCache;
		LLTextureCacheWorker::handle_t mHandle;
	};

	class WriteResponder : public LLLFSThread::Responder
	{
	public:
		WriteResponder(LLTextureCache* cache, handle_t handle) : mCache(cache), mHandle(handle) {}
		~WriteResponder() {}
		void completed(S32 bytes)
		{
			mCache->lockWorkers();
			LLTextureCacheWorker* writer = mCache->getWriter(mHandle);
			if (writer) writer->ioComplete(bytes);
			mCache->unlockWorkers();
		}
		LLTextureCache* mCache;
		LLTextureCacheWorker::handle_t mHandle;
	};
	
public:
	LLTextureCacheWorker(LLTextureCache* cache, U32 priority, const LLUUID& id,
						 U8* data, S32 datasize, S32 offset,
						 S32 imagesize, // for writes
						 LLTextureCache::Responder* responder)
		: LLWorkerClass(cache, "LLTextureCacheWorker"),
		  mID(id),
		  mCache(cache),
		  mPriority(priority),
		  mReadData(NULL),
		  mWriteData(data),
		  mDataSize(datasize),
		  mOffset(offset),
		  mImageSize(imagesize),
		  mImageFormat(IMG_CODEC_J2C),
		  mImageLocal(FALSE),
		  mResponder(responder),
		  mFileHandle(LLLFSThread::nullHandle()),
		  mBytesToRead(0),
		  mBytesRead(0)
	{
		mPriority &= LLWorkerThread::PRIORITY_LOWBITS;
	}
	~LLTextureCacheWorker()
	{
		llassert_always(!haveWork());
		FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
	}

	// override this interface
	virtual bool doRead() = 0;
	virtual bool doWrite() = 0;

	virtual bool doWork(S32 param); // Called from LLWorkerThread::processRequest()

	handle_t read() { addWork(0, LLWorkerThread::PRIORITY_HIGH | mPriority); return mRequestHandle; }
	handle_t write() { addWork(1, LLWorkerThread::PRIORITY_HIGH | mPriority); return mRequestHandle; }
	bool complete() { return checkWork(); }
	void ioComplete(S32 bytes)
	{
		mBytesRead = bytes;
		setPriority(LLWorkerThread::PRIORITY_HIGH | mPriority);
	}

private:
	virtual void startWork(S32 param); // called from addWork() (MAIN THREAD)
	virtual void finishWork(S32 param, bool completed); // called from finishRequest() (WORK THREAD)
	virtual void endWork(S32 param, bool aborted); // called from doWork() (MAIN THREAD)

protected:
	LLTextureCache* mCache;
	U32 mPriority;
	LLUUID	mID;
	
	U8* mReadData;
	U8* mWriteData;
	S32 mDataSize;
	S32 mOffset;
	S32 mImageSize;
	EImageCodec mImageFormat;
	BOOL mImageLocal;
	LLPointer<LLTextureCache::Responder> mResponder;
	LLLFSThread::handle_t mFileHandle;
	S32 mBytesToRead;
	LLAtomicS32 mBytesRead;
};

class LLTextureCacheLocalFileWorker : public LLTextureCacheWorker
{
public:
	LLTextureCacheLocalFileWorker(LLTextureCache* cache, U32 priority, const std::string& filename, const LLUUID& id,
						 U8* data, S32 datasize, S32 offset,
						 S32 imagesize, // for writes
						 LLTextureCache::Responder* responder) 
			: LLTextureCacheWorker(cache, priority, id, data, datasize, offset, imagesize, responder),
			mFileName(filename)

	{
	}

	virtual bool doRead();
	virtual bool doWrite();
	
private:
	std::string	mFileName;
};

bool LLTextureCacheLocalFileWorker::doRead()
{
	S32 local_size = LLAPRFile::size(mFileName, mCache->getLocalAPRFilePool());

	if (local_size > 0 && mFileName.size() > 4)
	{
		mDataSize = local_size; // Only a complete file is valid

		std::string extension = mFileName.substr(mFileName.size() - 3, 3);

		mImageFormat = LLImageBase::getCodecFromExtension(extension);

		if (mImageFormat == IMG_CODEC_INVALID)
		{
// 			llwarns << "Unrecognized file extension " << extension << " for local texture " << mFileName << llendl;
			mDataSize = 0; // no data
			return true;
		}
	}
	else
	{
		// file doesn't exist
		mDataSize = 0; // no data
		return true;
	}

#if USE_LFS_READ
	if (mFileHandle == LLLFSThread::nullHandle())
	{
		mImageLocal = TRUE;
		mImageSize = local_size;
		if (!mDataSize || mDataSize + mOffset > local_size)
		{
			mDataSize = local_size - mOffset;
		}
		if (mDataSize <= 0)
		{
			// no more data to read
			mDataSize = 0;
			return true;
		}
		mReadData = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), mDataSize);
		mBytesRead = -1;
		mBytesToRead = mDataSize;
		setPriority(LLWorkerThread::PRIORITY_LOW | mPriority);
		mFileHandle = LLLFSThread::sLocal->read(local_filename, mReadData, mOffset, mDataSize,
												new ReadResponder(mCache, mRequestHandle));
		return false;
	}
	else
	{
		if (mBytesRead >= 0)
		{
			if (mBytesRead != mBytesToRead)
			{
// 				llwarns << "Error reading file from local cache: " << local_filename
// 						<< " Bytes: " << mDataSize << " Offset: " << mOffset
// 						<< " / " << mDataSize << llendl;
				mDataSize = 0; // failed
				FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
				mReadData = NULL;
			}
			return true;
		}
		else
		{
			return false;
		}
	}
#else
	if (!mDataSize || mDataSize > local_size)
	{
		mDataSize = local_size;
	}
	mReadData = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), mDataSize);
	
	S32 bytes_read = LLAPRFile::readEx(mFileName, mReadData, mOffset, mDataSize, mCache->getLocalAPRFilePool());	

	if (bytes_read != mDataSize)
	{
// 		llwarns << "Error reading file from local cache: " << mFileName
// 				<< " Bytes: " << mDataSize << " Offset: " << mOffset
// 				<< " / " << mDataSize << llendl;
		mDataSize = 0;
		FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
		mReadData = NULL;
	}
	else
	{
		mImageSize = local_size;
		mImageLocal = TRUE;
	}
	return true;
#endif
}

bool LLTextureCacheLocalFileWorker::doWrite()
{
	// no writes for local files
	return false;
}

class LLTextureCacheRemoteWorker : public LLTextureCacheWorker
{
public:
	LLTextureCacheRemoteWorker(LLTextureCache* cache, U32 priority, const LLUUID& id,
						 U8* data, S32 datasize, S32 offset,
						 S32 imagesize, // for writes
						 LLPointer<LLImageRaw> raw, S32 discardlevel,
						 LLTextureCache::Responder* responder) 
			: LLTextureCacheWorker(cache, priority, id, data, datasize, offset, imagesize, responder),
			mState(INIT),
			mRawImage(raw),
			mRawDiscardLevel(discardlevel)
	{
	}

	virtual bool doRead();
	virtual bool doWrite();

private:
	enum e_state
	{
		INIT = 0,
		LOCAL = 1,
		CACHE = 2,
		HEADER = 3,
		BODY = 4
	};

	e_state mState;
	LLPointer<LLImageRaw> mRawImage;
	S32 mRawDiscardLevel;
};


//virtual
void LLTextureCacheWorker::startWork(S32 param)
{
}

// This is where a texture is read from the cache system (header and body)
// Current assumption are:
// - the whole data are in a raw form, will be stored at mReadData
// - the size of this raw data is mDataSize and can be smaller than TEXTURE_CACHE_ENTRY_SIZE (the size of a record in the header cache)
// - the code supports offset reading but this is actually never exercised in the viewer
bool LLTextureCacheRemoteWorker::doRead()
{
	bool done = false;
	S32 idx = -1;

	S32 local_size = 0;
	std::string local_filename;
	
	// First state / stage : find out if the file is local
	if (mState == INIT)
	{
#if 0
		std::string filename = mCache->getLocalFileName(mID);	
		// Is it a JPEG2000 file? 
		{
			local_filename = filename + ".j2c";
			local_size = LLAPRFile::size(local_filename, mCache->getLocalAPRFilePool());
			if (local_size > 0)
			{
				mImageFormat = IMG_CODEC_J2C;
			}
		}
		// If not, is it a jpeg file?
		if (local_size == 0)
		{
			local_filename = filename + ".jpg";
			local_size = LLAPRFile::size(local_filename, mCache->getLocalAPRFilePool());
			if (local_size > 0)
			{
				mImageFormat = IMG_CODEC_JPEG;
				mDataSize = local_size; // Only a complete .jpg file is valid
			}
		}
		// Hmm... What about a targa file? (used for UI texture mostly)
		if (local_size == 0)
		{
			local_filename = filename + ".tga";
			local_size = LLAPRFile::size(local_filename, mCache->getLocalAPRFilePool());
			if (local_size > 0)
			{
				mImageFormat = IMG_CODEC_TGA;
				mDataSize = local_size; // Only a complete .tga file is valid
			}
		}
		// Determine the next stage: if we found a file, then LOCAL else CACHE
		mState = (local_size > 0 ? LOCAL : CACHE);

		llassert_always(mState == CACHE) ;
#else
		mState = CACHE;
#endif
	}

	// Second state / stage : if the file is local, load it and leave
	if (!done && (mState == LOCAL))
	{
		llassert(local_size != 0);	// we're assuming there is a non empty local file here...
		if (!mDataSize || mDataSize > local_size)
		{
			mDataSize = local_size;
		}
		// Allocate read buffer
		mReadData = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), mDataSize);
		S32 bytes_read = LLAPRFile::readEx(local_filename, 
											 mReadData, mOffset, mDataSize, mCache->getLocalAPRFilePool());
		if (bytes_read != mDataSize)
		{
 			llwarns << "Error reading file from local cache: " << local_filename
 					<< " Bytes: " << mDataSize << " Offset: " << mOffset
 					<< " / " << mDataSize << llendl;
			mDataSize = 0;
			FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
			mReadData = NULL;
		}
		else
		{
			//llinfos << "texture " << mID.asString() << " found in local_assets" << llendl;
			mImageSize = local_size;
			mImageLocal = TRUE;
		}
		// We're done...
		done = true;
	}

	// Second state / stage : identify the cache or not...
	if (!done && (mState == CACHE))
	{
		LLTextureCache::Entry entry ;
		idx = mCache->getHeaderCacheEntry(mID, entry);
		if (idx < 0)
		{
			// The texture is *not* cached. We're done here...
			mDataSize = 0; // no data 
			done = true;
		}
		else
		{
			mImageSize = entry.mImageSize ;
			// If the read offset is bigger than the header cache, we read directly from the body
			// Note that currently, we *never* read with offset from the cache, so the result is *always* HEADER
			mState = mOffset < TEXTURE_CACHE_ENTRY_SIZE ? HEADER : BODY;
		}
	}

	// Third state / stage : read data from the header cache (texture.entries) file
	if (!done && (mState == HEADER))
	{
		llassert_always(idx >= 0);	// we need an entry here or reading the header makes no sense
		llassert_always(mOffset < TEXTURE_CACHE_ENTRY_SIZE);
		S32 offset = idx * TEXTURE_CACHE_ENTRY_SIZE + mOffset;
		// Compute the size we need to read (in bytes)
		S32 size = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
		size = llmin(size, mDataSize);
		// Allocate the read buffer
		mReadData = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), size);
		S32 bytes_read = LLAPRFile::readEx(mCache->mHeaderDataFileName, 
											 mReadData, offset, size, mCache->getLocalAPRFilePool());
		if (bytes_read != size)
		{
			llwarns << "LLTextureCacheWorker: "  << mID
					<< " incorrect number of bytes read from header: " << bytes_read
					<< " / " << size << llendl;
			FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
			mReadData = NULL;
			mDataSize = -1; // failed
			done = true;
		}
		// If we already read all we expected, we're actually done
		if (mDataSize <= bytes_read)
		{
			done = true;
		}
		else
		{
			mState = BODY;
		}
	}

	// Fourth state / stage : read the rest of the data from the UUID based cached file
	if (!done && (mState == BODY))
	{
		std::string filename = mCache->getTextureFileName(mID);
		S32 filesize = LLAPRFile::size(filename, mCache->getLocalAPRFilePool());

		if (filesize && (filesize + TEXTURE_CACHE_ENTRY_SIZE) > mOffset)
		{
			S32 max_datasize = TEXTURE_CACHE_ENTRY_SIZE + filesize - mOffset;
			mDataSize = llmin(max_datasize, mDataSize);

			S32 data_offset, file_size, file_offset;
			
			// Reserve the whole data buffer first
			U8* data = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), mDataSize);

			// Set the data file pointers taking the read offset into account. 2 cases:
			if (mOffset < TEXTURE_CACHE_ENTRY_SIZE)
			{
				// Offset within the header record. That means we read something from the header cache.
				// Note: most common case is (mOffset = 0), so this is the "normal" code path.
				data_offset = TEXTURE_CACHE_ENTRY_SIZE - mOffset;	// i.e. TEXTURE_CACHE_ENTRY_SIZE if mOffset nul (common case)
				file_offset = 0;
				file_size = mDataSize - data_offset;
				// Copy the raw data we've been holding from the header cache into the new sized buffer
				llassert_always(mReadData);
				memcpy(data, mReadData, data_offset);
				FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
				mReadData = NULL;
			}
			else
			{
				// Offset bigger than the header record. That means we haven't read anything yet.
				data_offset = 0;
				file_offset = mOffset - TEXTURE_CACHE_ENTRY_SIZE;
				file_size = mDataSize;
				// No data from header cache to copy in that case, we skipped it all
			}

			// Now use that buffer as the object read buffer
			llassert_always(mReadData == NULL);
			mReadData = data;

			// Read the data at last
			S32 bytes_read = LLAPRFile::readEx(filename, 
											 mReadData + data_offset,
											 file_offset, file_size,
											 mCache->getLocalAPRFilePool());
			if (bytes_read != file_size)
			{
				llwarns << "LLTextureCacheWorker: "  << mID
						<< " incorrect number of bytes read from body: " << bytes_read
						<< " / " << file_size << llendl;
				FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
				mReadData = NULL;
				mDataSize = -1; // failed
				done = true;
			}
		}
		else
		{
			// No body, we're done.
			mDataSize = llmax(TEXTURE_CACHE_ENTRY_SIZE - mOffset, 0);
			lldebugs << "No body file for: " << filename << llendl;
		}	
		// Nothing else to do at that point...
		done = true;
	}

	// Clean up and exit
	return done;
}

// This is where *everything* about a texture is written down in the cache system (entry map, header and body)
// Current assumption are:
// - the whole data are in a raw form, starting at mWriteData
// - the size of this raw data is mDataSize and can be smaller than TEXTURE_CACHE_ENTRY_SIZE (the size of a record in the header cache)
// - the code *does not* support offset writing so there are no difference between buffer addresses and start of data
bool LLTextureCacheRemoteWorker::doWrite()
{
	bool done = false;
	S32 idx = -1;	

	// First state / stage : check that what we're trying to cache is in an OK shape
	if (mState == INIT)
	{
		llassert_always(mOffset == 0);	// We currently do not support write offsets
		llassert_always(mDataSize > 0); // Things will go badly wrong if mDataSize is nul or negative...
		llassert_always(mImageSize >= mDataSize);
		mState = CACHE;
	}
	
	// No LOCAL state for write(): because it doesn't make much sense to cache a local file...

	// Second state / stage : set an entry in the headers entry (texture.entries) file
	if (!done && (mState == CACHE))
	{
		bool alreadyCached = false;
		LLTextureCache::Entry entry ;

		// Checks if this image is already in the entry list
		idx = mCache->getHeaderCacheEntry(mID, entry);
		if(idx < 0)
		{
			idx = mCache->setHeaderCacheEntry(mID, entry, mImageSize, mDataSize); // create the new entry.
			if(idx >= 0)
			{
				//write to the fast cache.
				llassert_always(mCache->writeToFastCache(idx, mRawImage, mRawDiscardLevel));
			}
		}
		else
		{
			alreadyCached = mCache->updateEntry(idx, entry, mImageSize, mDataSize); // update the existing entry.
		}

		if (idx < 0)
		{
			llwarns << "LLTextureCacheWorker: "  << mID
					<< " Unable to create header entry for writing!" << llendl;
			mDataSize = -1; // failed
			done = true;
		}
		else
		{
			if (alreadyCached && (mDataSize <= TEXTURE_CACHE_ENTRY_SIZE))
			{
				// Small texture already cached case: we're done with writing
				done = true;
			}
			else
			{
				// If the texture has already been cached, we don't resave the header and go directly to the body part
				mState = alreadyCached ? BODY : HEADER;
			}
		}
	}

	// Third stage / state : write the header record in the header file (texture.cache)
	if (!done && (mState == HEADER))
	{
		llassert_always(idx >= 0);	// we need an entry here or storing the header makes no sense
		S32 offset = idx * TEXTURE_CACHE_ENTRY_SIZE;	// skip to the correct spot in the header file
		S32 size = TEXTURE_CACHE_ENTRY_SIZE;			// record size is fixed for the header
		S32 bytes_written;

		if (mDataSize < TEXTURE_CACHE_ENTRY_SIZE)
		{
			// We need to write a full record in the header cache so, if the amount of data is smaller
			// than a record, we need to transfer the data to a buffer padded with 0 and write that
			U8* padBuffer = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), TEXTURE_CACHE_ENTRY_SIZE);
			memset(padBuffer, 0, TEXTURE_CACHE_ENTRY_SIZE);		// Init with zeros
			memcpy(padBuffer, mWriteData, mDataSize);			// Copy the write buffer
			bytes_written = LLAPRFile::writeEx(mCache->mHeaderDataFileName, padBuffer, offset, size, mCache->getLocalAPRFilePool());
			FREE_MEM(LLImageBase::getPrivatePool(), padBuffer);
		}
		else
		{
			// Write the header record (== first TEXTURE_CACHE_ENTRY_SIZE bytes of the raw file) in the header file
			bytes_written = LLAPRFile::writeEx(mCache->mHeaderDataFileName, mWriteData, offset, size, mCache->getLocalAPRFilePool());
		}

		if (bytes_written <= 0)
		{
			llwarns << "LLTextureCacheWorker: "  << mID
					<< " Unable to write header entry!" << llendl;
			mDataSize = -1; // failed
			done = true;
		}

		// If we wrote everything (may be more with padding) in the header cache, 
		// we're done so we don't have a body to store
		if (mDataSize <= bytes_written)
		{
			done = true;
		}
		else
		{
			mState = BODY;
		}
	}
	
	// Fourth stage / state : write the body file, i.e. the rest of the texture in a "UUID" file name
	if (!done && (mState == BODY))
	{
		llassert(mDataSize > TEXTURE_CACHE_ENTRY_SIZE);	// wouldn't make sense to be here otherwise...
		S32 file_size = mDataSize - TEXTURE_CACHE_ENTRY_SIZE;
		
		{
			// build the cache file name from the UUID
			std::string filename = mCache->getTextureFileName(mID);			
// 			llinfos << "Writing Body: " << filename << " Bytes: " << file_offset+file_size << llendl;
			S32 bytes_written = LLAPRFile::writeEx(	filename, 
													mWriteData + TEXTURE_CACHE_ENTRY_SIZE,
													0, file_size,
													mCache->getLocalAPRFilePool());
			if (bytes_written <= 0)
			{
				llwarns << "LLTextureCacheWorker: "  << mID
						<< " incorrect number of bytes written to body: " << bytes_written
						<< " / " << file_size << llendl;
				mDataSize = -1; // failed
				done = true;
			}
		}
		
		// Nothing else to do at that point...
		done = true;
	}
	mRawImage = NULL;

	// Clean up and exit
	return done;
}

//virtual
bool LLTextureCacheWorker::doWork(S32 param)
{
	bool res = false;
	if (param == 0) // read
	{
		res = doRead();
	}
	else if (param == 1) // write
	{
		res = doWrite();
	}
	else
	{
		llassert_always(0);
	}
	return res;
}

//virtual (WORKER THREAD)
void LLTextureCacheWorker::finishWork(S32 param, bool completed)
{
	if (mResponder.notNull())
	{
		bool success = (completed && mDataSize > 0);
		if (param == 0)
		{
			// read
			if (success)
			{
				mResponder->setData(mReadData, mDataSize, mImageSize, mImageFormat, mImageLocal);
				mReadData = NULL; // responder owns data
				mDataSize = 0;
			}
			else
			{
				FREE_MEM(LLImageBase::getPrivatePool(), mReadData);
				mReadData = NULL;
			}
		}
		else
		{
			// write
			mWriteData = NULL; // we never owned data
			mDataSize = 0;
		}
		mCache->addCompleted(mResponder, success);
	}
}

//virtual (MAIN THREAD)
void LLTextureCacheWorker::endWork(S32 param, bool aborted)
{
	if (aborted)
	{
		// Let the destructor handle any cleanup
		return;
	}
	switch(param)
	{
	  default:
	  case 0: // read
	  case 1: // write
	  {
		  if (mDataSize < 0)
		  {
			  // failed
			  mCache->removeFromCache(mID);
		  }
		  break;
	  }
	}
}

//////////////////////////////////////////////////////////////////////////////

LLTextureCache::LLTextureCache(bool threaded)
	: LLWorkerThread("TextureCache", threaded),
	  mWorkersMutex(NULL),
	  mHeaderMutex(NULL),
	  mListMutex(NULL),
	  mFastCacheMutex(NULL),
	  mHeaderAPRFile(NULL),
	  mReadOnly(TRUE), //do not allow to change the texture cache until setReadOnly() is called.
	  mTexturesSizeTotal(0),
	  mDoPurge(FALSE),
	  mFastCachep(NULL),
	  mFastCachePoolp(NULL),
	  mFastCachePadBuffer(NULL)
{
}

LLTextureCache::~LLTextureCache()
{
	clearDeleteList() ;
	writeUpdatedEntries() ;
	delete mFastCachep;
	delete mFastCachePoolp;
	FREE_MEM(LLImageBase::getPrivatePool(), mFastCachePadBuffer);
}

//////////////////////////////////////////////////////////////////////////////

//virtual
S32 LLTextureCache::update(F32 max_time_ms)
{
	static LLFrameTimer timer ;
	static const F32 MAX_TIME_INTERVAL = 300.f ; //seconds.

	S32 res;
	res = LLWorkerThread::update(max_time_ms);

	mListMutex.lock();
	handle_list_t priorty_list = mPrioritizeWriteList; // copy list
	mPrioritizeWriteList.clear();
	responder_list_t completed_list = mCompletedList; // copy list
	mCompletedList.clear();
	mListMutex.unlock();
	
	lockWorkers();
	
	for (handle_list_t::iterator iter1 = priorty_list.begin();
		 iter1 != priorty_list.end(); ++iter1)
	{
		handle_t handle = *iter1;
		handle_map_t::iterator iter2 = mWriters.find(handle);
		if(iter2 != mWriters.end())
		{
			LLTextureCacheWorker* worker = iter2->second;
			worker->setPriority(LLWorkerThread::PRIORITY_HIGH | worker->mPriority);
		}
	}

	unlockWorkers(); 
	
	// call 'completed' with workers list unlocked (may call readComplete() or writeComplete()
	for (responder_list_t::iterator iter1 = completed_list.begin();
		 iter1 != completed_list.end(); ++iter1)
	{
		Responder *responder = iter1->first;
		bool success = iter1->second;
		responder->completed(success);
	}
	
	if(!res && timer.getElapsedTimeF32() > MAX_TIME_INTERVAL)
	{
		timer.reset() ;
		writeUpdatedEntries() ;
	}

	return res;
}

//////////////////////////////////////////////////////////////////////////////
// search for local copy of UUID-based image file
std::string LLTextureCache::getLocalFileName(const LLUUID& id)
{
	// Does not include extension
	std::string idstr = id.asString();
	// TODO: should we be storing cached textures in skin directory?
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_LOCAL_ASSETS, idstr);
	return filename;
}

std::string LLTextureCache::getTextureFileName(const LLUUID& id)
{
	std::string idstr = id.asString();
	std::string delem = gDirUtilp->getDirDelimiter();
	std::string filename = mTexturesDirName + delem + idstr[0] + delem + idstr + ".texture";
	return filename;
}

//debug
BOOL LLTextureCache::isInCache(const LLUUID& id) 
{
	LLMutexLock lock(&mHeaderMutex);
	id_map_t::const_iterator iter = mHeaderIDMap.find(id);
	
	return (iter != mHeaderIDMap.end()) ;
}

//debug
BOOL LLTextureCache::isInLocal(const LLUUID& id) 
{
	S32 local_size = 0;
	std::string local_filename;
	
	std::string filename = getLocalFileName(id);	
	// Is it a JPEG2000 file? 
	{
		local_filename = filename + ".j2c";
		local_size = LLAPRFile::size(local_filename, getLocalAPRFilePool());
		if (local_size > 0)
		{
			return TRUE ;
		}
	}
		
	// If not, is it a jpeg file?		
	{
		local_filename = filename + ".jpg";
		local_size = LLAPRFile::size(local_filename, getLocalAPRFilePool());
		if (local_size > 0)
		{
			return TRUE ;
		}
	}
		
	// Hmm... What about a targa file? (used for UI texture mostly)		
	{
		local_filename = filename + ".tga";
		local_size = LLAPRFile::size(local_filename, getLocalAPRFilePool());
		if (local_size > 0)
		{
			return TRUE ;
		}
	}
		
	return FALSE ;
}
//////////////////////////////////////////////////////////////////////////////

//static
F32 LLTextureCache::sHeaderCacheVersion = 1.7f;
U32 LLTextureCache::sCacheMaxEntries = 1024 * 1024; //~1 million textures.
S64 LLTextureCache::sCacheMaxTexturesSize = 0; // no limit
const char* entries_filename = "texture.entries";
const char* cache_filename = "texture.cache";
const char* old_textures_dirname = "textures";
//change the location of the texture cache to prevent from being deleted by old version viewers.
const char* textures_dirname = "texturecache";
const char* fast_cache_filename = "FastCache.cache";

void LLTextureCache::setDirNames(ELLPath location)
{
	std::string delem = gDirUtilp->getDirDelimiter();

	mHeaderEntriesFileName = gDirUtilp->getExpandedFilename(location, textures_dirname, entries_filename);
	mHeaderDataFileName = gDirUtilp->getExpandedFilename(location, textures_dirname, cache_filename);
	mTexturesDirName = gDirUtilp->getExpandedFilename(location, textures_dirname);
	mFastCacheFileName =  gDirUtilp->getExpandedFilename(location, textures_dirname, fast_cache_filename);
}

void LLTextureCache::purgeCache(ELLPath location)
{
	LLMutexLock lock(&mHeaderMutex);

	if (!mReadOnly)
	{
		setDirNames(location);
		llassert_always(mHeaderAPRFile == NULL);

		//remove the legacy cache if exists
		std::string texture_dir = mTexturesDirName ;
		mTexturesDirName = gDirUtilp->getExpandedFilename(location, old_textures_dirname);
		if(LLFile::isdir(mTexturesDirName))
		{
			std::string file_name = gDirUtilp->getExpandedFilename(location, entries_filename);
			LLAPRFile::remove(file_name, getLocalAPRFilePool());

			file_name = gDirUtilp->getExpandedFilename(location, cache_filename);
			LLAPRFile::remove(file_name, getLocalAPRFilePool());

			purgeAllTextures(true);
		}
		mTexturesDirName = texture_dir ;
	}

	//remove the current texture cache.
	purgeAllTextures(true);
}

//is called in the main thread before initCache(...) is called.
void LLTextureCache::setReadOnly(BOOL read_only)
{
	mReadOnly = read_only ;
}

//called in the main thread.
S64 LLTextureCache::initCache(ELLPath location, S64 max_size, BOOL texture_cache_mismatch)
{
	llassert_always(getPending() == 0) ; //should not start accessing the texture cache before initialized.

	S64 header_size = (max_size / 100) * 36; //0.36 * max_size
	S64 max_entries = header_size / (TEXTURE_CACHE_ENTRY_SIZE + TEXTURE_FAST_CACHE_ENTRY_SIZE);
	sCacheMaxEntries = (S32)(llmin((S64)sCacheMaxEntries, max_entries));
	header_size = sCacheMaxEntries * TEXTURE_CACHE_ENTRY_SIZE;
	max_size -= header_size;
	if (sCacheMaxTexturesSize > 0)
		sCacheMaxTexturesSize = llmin(sCacheMaxTexturesSize, max_size);
	else
		sCacheMaxTexturesSize = max_size;
	max_size -= sCacheMaxTexturesSize;
	
	LL_INFOS("TextureCache") << "Headers: " << sCacheMaxEntries
			<< " Textures size: " << sCacheMaxTexturesSize / (1024 * 1024) << " MB" << LL_ENDL;

	setDirNames(location);
	
	if(texture_cache_mismatch) 
	{
		//if readonly, disable the texture cache,
		//otherwise wipe out the texture cache.
		purgeAllTextures(true); 

		if(mReadOnly)
		{
			return max_size ;
		}
	}
	
	if (!mReadOnly)
	{
		LLFile::mkdir(mTexturesDirName);
		
		const char* subdirs = "0123456789abcdef";
		for (S32 i=0; i<16; i++)
		{
			std::string dirname = mTexturesDirName + gDirUtilp->getDirDelimiter() + subdirs[i];
			LLFile::mkdir(dirname);
		}
	}
	readHeaderCache();
	purgeTextures(true); // calc mTexturesSize and make some room in the texture cache if we need it

	llassert_always(getPending() == 0) ; //should not start accessing the texture cache before initialized.
	openFastCache(true);

	return max_size; // unused cache space
}

//----------------------------------------------------------------------------
// mHeaderMutex must be locked for the following functions!

LLAPRFile* LLTextureCache::openHeaderEntriesFile(bool readonly, S32 offset)
{
	llassert_always(mHeaderAPRFile == NULL);
	apr_int32_t flags = readonly ? APR_READ|APR_BINARY : APR_READ|APR_WRITE|APR_BINARY;
	mHeaderAPRFile = new LLAPRFile(mHeaderEntriesFileName, flags, getLocalAPRFilePool());
	if(offset > 0)
	{
		mHeaderAPRFile->seek(APR_SET, offset);
	}
	return mHeaderAPRFile;
}

void LLTextureCache::closeHeaderEntriesFile()
{
	if(!mHeaderAPRFile)
	{
		return ;
	}

	delete mHeaderAPRFile;
	mHeaderAPRFile = NULL;
}

void LLTextureCache::readEntriesHeader()
{
	// mHeaderEntriesInfo initializes to default values so safe not to read it
	llassert_always(mHeaderAPRFile == NULL);
	if (LLAPRFile::isExist(mHeaderEntriesFileName, getLocalAPRFilePool()))
	{
		LLAPRFile::readEx(mHeaderEntriesFileName, (U8*)&mHeaderEntriesInfo, 0, sizeof(EntriesInfo),
						  getLocalAPRFilePool());
	}
	else //create an empty entries header.
	{
		mHeaderEntriesInfo.mVersion = sHeaderCacheVersion ;
		mHeaderEntriesInfo.mEntries = 0 ;
		writeEntriesHeader() ;
	}
}

void LLTextureCache::writeEntriesHeader()
{
	llassert_always(mHeaderAPRFile == NULL);
	if (!mReadOnly)
	{
		LLAPRFile::writeEx(mHeaderEntriesFileName, (U8*)&mHeaderEntriesInfo, 0, sizeof(EntriesInfo),
						   getLocalAPRFilePool());
	}
}

//mHeaderMutex is locked before calling this.
S32 LLTextureCache::openAndReadEntry(const LLUUID& id, Entry& entry, bool create)
{
	S32 idx = -1;
	
	id_map_t::iterator iter1 = mHeaderIDMap.find(id);
	if (iter1 != mHeaderIDMap.end())
	{
		idx = iter1->second;
	}

	if (idx < 0)
	{
		if (create && !mReadOnly)
		{
			if (mHeaderEntriesInfo.mEntries < sCacheMaxEntries)
			{
				// Add an entry to the end of the list
				idx = mHeaderEntriesInfo.mEntries++;

			}
			else if (!mFreeList.empty())
			{
				idx = *(mFreeList.begin());
				mFreeList.erase(mFreeList.begin());
			}
			else
			{
				// Look for a still valid entry in the LRU
				for (std::set<LLUUID>::iterator iter2 = mLRU.begin(); iter2 != mLRU.end();)
				{
					std::set<LLUUID>::iterator curiter2 = iter2++;
					LLUUID oldid = *curiter2;
					// Erase entry from LRU regardless
					mLRU.erase(curiter2);
					// Look up entry and use it if it is valid
					id_map_t::iterator iter3 = mHeaderIDMap.find(oldid);
					if (iter3 != mHeaderIDMap.end() && iter3->second >= 0)
					{
						idx = iter3->second;
						removeCachedTexture(oldid) ;//remove the existing cached texture to release the entry index.
						break;
					}
				}
				// if (idx < 0) at this point, we will rebuild the LRU 
				//  and retry if called from setHeaderCacheEntry(),
				//  otherwise this shouldn't happen and will trigger an error
			}
			if (idx >= 0)
			{
				entry.mID = id ;
				entry.mImageSize = -1 ; //mark it is a brand-new entry.					
				entry.mBodySize = 0 ;
			}
		}
	}
	else
	{
		// Remove this entry from the LRU if it exists
		mLRU.erase(id);
		// Read the entry
		idx_entry_map_t::iterator iter = mUpdatedEntryMap.find(idx) ;
		if(iter != mUpdatedEntryMap.end())
		{
			entry = iter->second ;
		}
		else
		{
			readEntryFromHeaderImmediately(idx, entry) ;
		}
		if(entry.mImageSize <= entry.mBodySize)//it happens on 64-bit systems, do not know why
		{
			llwarns << "corrupted entry: " << id << " entry image size: " << entry.mImageSize << " entry body size: " << entry.mBodySize << llendl ;

			//erase this entry and the cached texture from the cache.
			std::string tex_filename = getTextureFileName(id);
			removeEntry(idx, entry, tex_filename) ;
			mUpdatedEntryMap.erase(idx) ;
			idx = -1 ;
		}
	}
	return idx;
}

//mHeaderMutex is locked before calling this.
void LLTextureCache::writeEntryToHeaderImmediately(S32& idx, Entry& entry, bool write_header)
{	
	LLAPRFile* aprfile ;
	S32 bytes_written ;
	S32 offset = sizeof(EntriesInfo) + idx * sizeof(Entry);
	if(write_header)
	{
		aprfile = openHeaderEntriesFile(false, 0);		
		bytes_written = aprfile->write((U8*)&mHeaderEntriesInfo, sizeof(EntriesInfo)) ;
		if(bytes_written != sizeof(EntriesInfo))
		{
			clearCorruptedCache() ; //clear the cache.
			idx = -1 ;//mark the idx invalid.
			return ;
		}

		mHeaderAPRFile->seek(APR_SET, offset);
	}
	else
	{
		aprfile = openHeaderEntriesFile(false, offset);
	}
	bytes_written = aprfile->write((void*)&entry, (S32)sizeof(Entry));
	if(bytes_written != sizeof(Entry))
	{
		clearCorruptedCache() ; //clear the cache.
		idx = -1 ;//mark the idx invalid.

		return ;
	}

	closeHeaderEntriesFile();
	mUpdatedEntryMap.erase(idx) ;
}

//mHeaderMutex is locked before calling this.
void LLTextureCache::readEntryFromHeaderImmediately(S32& idx, Entry& entry)
{
	S32 offset = sizeof(EntriesInfo) + idx * sizeof(Entry);
	LLAPRFile* aprfile = openHeaderEntriesFile(true, offset);
	S32 bytes_read = aprfile->read((void*)&entry, (S32)sizeof(Entry));
	closeHeaderEntriesFile();

	if(bytes_read != sizeof(Entry))
	{
		clearCorruptedCache() ; //clear the cache.
		idx = -1 ;//mark the idx invalid.
	}
}

//mHeaderMutex is locked before calling this.
//update an existing entry time stamp, delay writing.
void LLTextureCache::updateEntryTimeStamp(S32 idx, Entry& entry)
{
	static const U32 MAX_ENTRIES_WITHOUT_TIME_STAMP = (U32)(LLTextureCache::sCacheMaxEntries * 0.75f) ;

	if(mHeaderEntriesInfo.mEntries < MAX_ENTRIES_WITHOUT_TIME_STAMP)
	{
		return ; //there are enough empty entry index space, no need to stamp time.
	}

	if (idx >= 0)
	{
		if (!mReadOnly)
		{
			entry.mTime = time(NULL);			
			mUpdatedEntryMap[idx] = entry ;
		}
	}
}

//update an existing entry, write to header file immediately.
bool LLTextureCache::updateEntry(S32& idx, Entry& entry, S32 new_image_size, S32 new_data_size)
{
	S32 new_body_size = llmax(0, new_data_size - TEXTURE_CACHE_ENTRY_SIZE) ;
	
	if(new_image_size == entry.mImageSize && new_body_size == entry.mBodySize)
	{
		return true ; //nothing changed.
	}
	else 
	{
		bool purge = false ;

		lockHeaders() ;

		bool update_header = false ;
		if(entry.mImageSize < 0) //is a brand-new entry
		{
			mHeaderIDMap[entry.mID] = idx;
			mTexturesSizeMap[entry.mID] = new_body_size ;
			mTexturesSizeTotal += new_body_size ;
			
			// Update Header
			update_header = true ;
		}				
		else if (entry.mBodySize != new_body_size)
		{
			//already in mHeaderIDMap.
			mTexturesSizeMap[entry.mID] = new_body_size ;
			mTexturesSizeTotal -= entry.mBodySize ;
			mTexturesSizeTotal += new_body_size ;
		}
		entry.mTime = time(NULL);
		entry.mImageSize = new_image_size ; 
		entry.mBodySize = new_body_size ;
		
		writeEntryToHeaderImmediately(idx, entry, update_header) ;
	
		if (mTexturesSizeTotal > sCacheMaxTexturesSize)
		{
			purge = true;
		}
		
		unlockHeaders() ;

		if (purge)
		{
			mDoPurge = TRUE;
		}
	}

	return false ;
}

U32 LLTextureCache::openAndReadEntries(std::vector<Entry>& entries)
{
	U32 num_entries = mHeaderEntriesInfo.mEntries;

	mHeaderIDMap.clear();
	mTexturesSizeMap.clear();
	mFreeList.clear();
	mTexturesSizeTotal = 0;

	LLAPRFile* aprfile = NULL; 
	if(mUpdatedEntryMap.empty())
	{
		aprfile = openHeaderEntriesFile(true, (S32)sizeof(EntriesInfo));
	}
	else //update the header file first.
	{
		aprfile = openHeaderEntriesFile(false, 0);
		updatedHeaderEntriesFile() ;
		if(!aprfile)
		{
			return 0;
		}
		aprfile->seek(APR_SET, (S32)sizeof(EntriesInfo));
	}
	for (U32 idx=0; idx<num_entries; idx++)
	{
		Entry entry;
		S32 bytes_read = aprfile->read((void*)(&entry), (S32)sizeof(Entry));
		if (bytes_read < sizeof(Entry))
		{
			llwarns << "Corrupted header entries, failed at " << idx << " / " << num_entries << llendl;
			closeHeaderEntriesFile();
			purgeAllTextures(false);
			return 0;
		}
		entries.push_back(entry);
// 		llinfos << "ENTRY: " << entry.mTime << " TEX: " << entry.mID << " IDX: " << idx << " Size: " << entry.mImageSize << llendl;
		if(entry.mImageSize > entry.mBodySize)
		{
			mHeaderIDMap[entry.mID] = idx;
			mTexturesSizeMap[entry.mID] = entry.mBodySize;
			mTexturesSizeTotal += entry.mBodySize;
		}
		else
		{
			mFreeList.insert(idx);
		}
	}
	closeHeaderEntriesFile();
	return num_entries;
}

void LLTextureCache::writeEntriesAndClose(const std::vector<Entry>& entries)
{
	S32 num_entries = entries.size();
	llassert_always(num_entries == mHeaderEntriesInfo.mEntries);
	
	if (!mReadOnly)
	{
		LLAPRFile* aprfile = openHeaderEntriesFile(false, (S32)sizeof(EntriesInfo));
		for (S32 idx=0; idx<num_entries; idx++)
		{
			S32 bytes_written = aprfile->write((void*)(&entries[idx]), (S32)sizeof(Entry));
			if(bytes_written != sizeof(Entry))
			{
				clearCorruptedCache() ; //clear the cache.
				return ;
			}
		}
		closeHeaderEntriesFile();
	}
}

void LLTextureCache::writeUpdatedEntries()
{
	lockHeaders() ;
	if (!mReadOnly && !mUpdatedEntryMap.empty())
	{
		openHeaderEntriesFile(false, 0);
		updatedHeaderEntriesFile() ;
		closeHeaderEntriesFile();
	}
	unlockHeaders() ;
}

//mHeaderMutex is locked and mHeaderAPRFile is created before calling this.
void LLTextureCache::updatedHeaderEntriesFile()
{
	if (!mReadOnly && !mUpdatedEntryMap.empty() && mHeaderAPRFile)
	{
		//entriesInfo
		mHeaderAPRFile->seek(APR_SET, 0);
		S32 bytes_written = mHeaderAPRFile->write((U8*)&mHeaderEntriesInfo, sizeof(EntriesInfo)) ;
		if(bytes_written != sizeof(EntriesInfo))
		{
			clearCorruptedCache() ; //clear the cache.
			return ;
		}
		
		//write each updated entry
		S32 entry_size = (S32)sizeof(Entry) ;
		S32 prev_idx = -1 ;
		S32 delta_idx ;
		for (idx_entry_map_t::iterator iter = mUpdatedEntryMap.begin(); iter != mUpdatedEntryMap.end(); ++iter)
		{
			delta_idx = iter->first - prev_idx - 1;
			prev_idx = iter->first ;
			if(delta_idx)
			{
				mHeaderAPRFile->seek(APR_CUR, delta_idx * entry_size);
			}
			
			bytes_written = mHeaderAPRFile->write((void*)(&iter->second), entry_size);
			if(bytes_written != entry_size)
			{
				clearCorruptedCache() ; //clear the cache.
				return ;
			}
		}
		mUpdatedEntryMap.clear() ;
	}
}
//----------------------------------------------------------------------------

// Called from either the main thread or the worker thread
void LLTextureCache::readHeaderCache()
{
	mHeaderMutex.lock();

	mLRU.clear(); // always clear the LRU

	readEntriesHeader();
	
	if (mHeaderEntriesInfo.mVersion != sHeaderCacheVersion)
	{
		if (!mReadOnly)
		{
			purgeAllTextures(false);
		}
	}
	else
	{
		std::vector<Entry> entries;
		U32 num_entries = openAndReadEntries(entries);
		if (num_entries)
		{
			U32 empty_entries = 0;
			typedef std::pair<U32, S32> lru_data_t;
			std::set<lru_data_t> lru;
			std::set<U32> purge_list;
			for (U32 i=0; i<num_entries; i++)
			{
				Entry& entry = entries[i];
				if (entry.mImageSize <= 0)
				{
					// This will be in the Free List, don't put it in the LRU
					++empty_entries;
				}
				else
				{
					lru.insert(std::make_pair(entry.mTime, i));
					if (entry.mBodySize > 0)
					{
						if (entry.mBodySize > entry.mImageSize)
						{
							// Shouldn't happen, failsafe only
							llwarns << "Bad entry: " << i << ": " << entry.mID << ": BodySize: " << entry.mBodySize << llendl;
							purge_list.insert(i);
						}
					}
				}
			}
			if (num_entries - empty_entries > sCacheMaxEntries)
			{
				// Special case: cache size was reduced, need to remove entries
				// Note: After we prune entries, we will call this again and create the LRU
				U32 entries_to_purge = (num_entries - empty_entries) - sCacheMaxEntries;
				llinfos << "Texture Cache Entries: " << num_entries << " Max: " << sCacheMaxEntries << " Empty: " << empty_entries << " Purging: " << entries_to_purge << llendl;
				// We can exit the following loop with the given condition, since if we'd reach the end of the lru set we'd have:
				// purge_list.size() = lru.size() = num_entries - empty_entries = entries_to_purge + sCacheMaxEntries >= entries_to_purge
				// So, it's certain that iter will never reach lru.end() first.
				std::set<lru_data_t>::iterator iter = lru.begin();
				while (purge_list.size() < entries_to_purge)
				{
					purge_list.insert(iter->second);
					++iter;
				}
			}
			else
			{
				S32 lru_entries = (S32)((F32)sCacheMaxEntries * TEXTURE_CACHE_LRU_SIZE);
				for (std::set<lru_data_t>::iterator iter = lru.begin(); iter != lru.end(); ++iter)
				{
					mLRU.insert(entries[iter->second].mID);
// 					llinfos << "LRU: " << iter->first << " : " << iter->second << llendl;
					if (--lru_entries <= 0)
						break;
				}
			}
			
			if (purge_list.size() > 0)
			{
				for (std::set<U32>::iterator iter = purge_list.begin(); iter != purge_list.end(); ++iter)
				{
					std::string tex_filename = getTextureFileName(entries[*iter].mID);
					removeEntry((S32)*iter, entries[*iter], tex_filename);
				}
				// If we removed any entries, we need to rebuild the entries list,
				// write the header, and call this again
				std::vector<Entry> new_entries;
				for (U32 i=0; i<num_entries; i++)
				{
					const Entry& entry = entries[i];
					if (entry.mImageSize > 0)
					{
						new_entries.push_back(entry);
					}
				}
				llassert_always(new_entries.size() <= sCacheMaxEntries);
				mHeaderEntriesInfo.mEntries = new_entries.size();
				writeEntriesHeader();
				writeEntriesAndClose(new_entries);
				mHeaderMutex.unlock(); // unlock the mutex before calling again
				readHeaderCache(); // repeat with new entries file
				mHeaderMutex.lock();
			}
			else
			{
				//entries are not changed, nothing here.
			}
		}
	}
	mHeaderMutex.unlock();
}

//////////////////////////////////////////////////////////////////////////////

//the header mutex is locked before calling this.
void LLTextureCache::clearCorruptedCache()
{
	llwarns << "the texture cache is corrupted, need to be cleared." << llendl ;

	closeHeaderEntriesFile();//close possible file handler
	purgeAllTextures(false) ; //clear the cache.
	
	if (!mReadOnly) //regenerate the directory tree if not exists.
	{
		LLFile::mkdir(mTexturesDirName);
		
		const char* subdirs = "0123456789abcdef";
		for (S32 i=0; i<16; i++)
		{
			std::string dirname = mTexturesDirName + gDirUtilp->getDirDelimiter() + subdirs[i];
			LLFile::mkdir(dirname);
		}
	}

	return ;
}

void LLTextureCache::purgeAllTextures(bool purge_directories)
{
	if (!mReadOnly)
	{
		const char* subdirs = "0123456789abcdef";
		std::string delem = gDirUtilp->getDirDelimiter();
		std::string mask = "*";
		for (S32 i=0; i<16; i++)
		{
			std::string dirname = mTexturesDirName + delem + subdirs[i];
			llinfos << "Deleting files in directory: " << dirname << llendl;
			gDirUtilp->deleteFilesInDir(dirname, mask);
			if (purge_directories)
			{
				LLFile::rmdir(dirname);
			}
		}
		if (purge_directories)
		{
			gDirUtilp->deleteFilesInDir(mTexturesDirName, mask);
			LLFile::rmdir(mTexturesDirName);
		}		
	}
	mHeaderIDMap.clear();
	mTexturesSizeMap.clear();
	mTexturesSizeTotal = 0;
	mFreeList.clear();
	mTexturesSizeTotal = 0;
	mUpdatedEntryMap.clear();

	// Info with 0 entries
	mHeaderEntriesInfo.mVersion = sHeaderCacheVersion;
	mHeaderEntriesInfo.mEntries = 0;
	writeEntriesHeader();

	llinfos << "The entire texture cache is cleared." << llendl ;
}

void LLTextureCache::purgeTextures(bool validate)
{
	if (mReadOnly)
	{
		return;
	}

	if (!mThreaded)
	{
		// *FIX:Mani - watchdog off.
		LLAppViewer::instance()->pauseMainloopTimeout();
	}
	
	LLMutexLock lock(&mHeaderMutex);

	llinfos << "TEXTURE CACHE: Purging." << llendl;

	// Read the entries list
	std::vector<Entry> entries;
	U32 num_entries = openAndReadEntries(entries);
	if (!num_entries)
	{
		return; // nothing to purge
	}
	
	// Use mTexturesSizeMap to collect UUIDs of textures with bodies
	typedef std::set<std::pair<U32,S32> > time_idx_set_t;
	std::set<std::pair<U32,S32> > time_idx_set;
	for (size_map_t::iterator iter1 = mTexturesSizeMap.begin();
		 iter1 != mTexturesSizeMap.end(); ++iter1)
	{
		if (iter1->second > 0)
		{
			id_map_t::iterator iter2 = mHeaderIDMap.find(iter1->first);
			if (iter2 != mHeaderIDMap.end())
			{
				S32 idx = iter2->second;
				time_idx_set.insert(std::make_pair(entries[idx].mTime, idx));
// 				llinfos << "TIME: " << entries[idx].mTime << " TEX: " << entries[idx].mID << " IDX: " << idx << " Size: " << entries[idx].mImageSize << llendl;
			}
			else
			{
				llerrs << "mTexturesSizeMap / mHeaderIDMap corrupted." << llendl ;
			}
		}
	}
	
	// Validate 1/256th of the files on startup
	U32 validate_idx = 0;
	if (validate)
	{
		validate_idx = gSavedSettings.getU32("CacheValidateCounter");
		U32 next_idx = (validate_idx + 1) % 256;
		gSavedSettings.setU32("CacheValidateCounter", next_idx);
		LL_DEBUGS("TextureCache") << "TEXTURE CACHE: Validating: " << validate_idx << LL_ENDL;
	}

	S64 cache_size = mTexturesSizeTotal;
	S64 purged_cache_size = (sCacheMaxTexturesSize * (S64)((1.f-TEXTURE_CACHE_PURGE_AMOUNT)*100)) / 100;
	S32 purge_count = 0;
	for (time_idx_set_t::iterator iter = time_idx_set.begin();
		 iter != time_idx_set.end(); ++iter)
	{
		S32 idx = iter->second;
		bool purge_entry = false;
		std::string filename = getTextureFileName(entries[idx].mID);
		if (cache_size >= purged_cache_size)
		{
			purge_entry = true;
		}
		else if (validate)
		{
			// make sure file exists and is the correct size
			U32 uuididx = entries[idx].mID.mData[0];
			if (uuididx == validate_idx)
			{
 				LL_DEBUGS("TextureCache") << "Validating: " << filename << "Size: " << entries[idx].mBodySize << LL_ENDL;
				S32 bodysize = LLAPRFile::size(filename, getLocalAPRFilePool());
				if (bodysize != entries[idx].mBodySize)
				{
					LL_WARNS("TextureCache") << "TEXTURE CACHE BODY HAS BAD SIZE: " << bodysize << " != " << entries[idx].mBodySize
							<< filename << LL_ENDL;
					purge_entry = true;
				}
			}
		}
		else
		{
			break;
		}
		
		if (purge_entry)
		{
			purge_count++;
	 		LL_DEBUGS("TextureCache") << "PURGING: " << filename << LL_ENDL;
			cache_size -= entries[idx].mBodySize;
			removeEntry(idx, entries[idx], filename) ;			
		}
	}

	LL_DEBUGS("TextureCache") << "TEXTURE CACHE: Writing Entries: " << num_entries << LL_ENDL;

	writeEntriesAndClose(entries);
	
	// *FIX:Mani - watchdog back on.
	LLAppViewer::instance()->resumeMainloopTimeout();
	
	LL_INFOS("TextureCache") << "TEXTURE CACHE:"
			<< " PURGED: " << purge_count
			<< " ENTRIES: " << num_entries
			<< " CACHE SIZE: " << mTexturesSizeTotal / (1024 * 1024) << " MB"
			<< llendl;
}

//////////////////////////////////////////////////////////////////////////////

// call lockWorkers() first!
LLTextureCacheWorker* LLTextureCache::getReader(handle_t handle)
{
	LLTextureCacheWorker* res = NULL;
	handle_map_t::iterator iter = mReaders.find(handle);
	if (iter != mReaders.end())
	{
		res = iter->second;
	}
	return res;
}

LLTextureCacheWorker* LLTextureCache::getWriter(handle_t handle)
{
	LLTextureCacheWorker* res = NULL;
	handle_map_t::iterator iter = mWriters.find(handle);
	if (iter != mWriters.end())
	{
		res = iter->second;
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////////
// Called from work thread

// Reads imagesize from the header, updates timestamp
S32 LLTextureCache::getHeaderCacheEntry(const LLUUID& id, Entry& entry)
{
	LLMutexLock lock(&mHeaderMutex);	
	S32 idx = openAndReadEntry(id, entry, false);
	if (idx >= 0)
	{		
		updateEntryTimeStamp(idx, entry); // updates time
	}
	return idx;
}

// Writes imagesize to the header, updates timestamp
S32 LLTextureCache::setHeaderCacheEntry(const LLUUID& id, Entry& entry, S32 imagesize, S32 datasize)
{
	mHeaderMutex.lock();
	S32 idx = openAndReadEntry(id, entry, true);
	mHeaderMutex.unlock();

	if (idx >= 0)
	{
		updateEntry(idx, entry, imagesize, datasize);				
	}

	if(idx < 0) // retry
	{
		readHeaderCache(); // We couldn't write an entry, so refresh the LRU
	
		mHeaderMutex.lock();
		llassert_always(!mLRU.empty() || mHeaderEntriesInfo.mEntries < sCacheMaxEntries);
		mHeaderMutex.unlock();

		idx = setHeaderCacheEntry(id, entry, imagesize, datasize); // assert above ensures no inf. recursion
	}
	return idx;
}

//////////////////////////////////////////////////////////////////////////////

// Calls from texture pipeline thread (i.e. LLTextureFetch)

LLTextureCache::handle_t LLTextureCache::readFromCache(const std::string& filename, const LLUUID& id, U32 priority,
													   S32 offset, S32 size, ReadResponder* responder)
{
	// Note: checking to see if an entry exists can cause a stall,
	//  so let the thread handle it
	LLMutexLock lock(&mWorkersMutex);
	LLTextureCacheWorker* worker = new LLTextureCacheLocalFileWorker(this, priority, filename, id,
																	 NULL, size, offset, 0,
																	 responder);
	handle_t handle = worker->read();
	mReaders[handle] = worker;
	return handle;
}

LLTextureCache::handle_t LLTextureCache::readFromCache(const LLUUID& id, U32 priority,
													   S32 offset, S32 size, ReadResponder* responder)
{
	// Note: checking to see if an entry exists can cause a stall,
	//  so let the thread handle it
	LLMutexLock lock(&mWorkersMutex);
	LLTextureCacheWorker* worker = new LLTextureCacheRemoteWorker(this, priority, id,
																  NULL, size, offset,
																  0, NULL, 0, responder);
	handle_t handle = worker->read();
	mReaders[handle] = worker;
	return handle;
}


bool LLTextureCache::readComplete(handle_t handle, bool abort)
{
	lockWorkers();
	handle_map_t::iterator iter = mReaders.find(handle);
	LLTextureCacheWorker* worker = NULL;
	bool complete = false;
	if (iter != mReaders.end())
	{
		worker = iter->second;
		complete = worker->complete();

		if(!complete && abort)
		{
			abortRequest(handle, true) ;
		}
	}
	if (worker && (complete || abort))
	{
		mReaders.erase(iter);
		unlockWorkers();
		worker->scheduleDelete();
	}
	else
	{
		unlockWorkers();
	}
	return (complete || abort);
}

LLTextureCache::handle_t LLTextureCache::writeToCache(const LLUUID& id, U32 priority,
													  U8* data, S32 datasize, S32 imagesize,
													  LLPointer<LLImageRaw> rawimage, S32 discardlevel,
													  WriteResponder* responder)
{
	if (mReadOnly)
	{
		delete responder;
		return LLWorkerThread::nullHandle();
	}
	if (mDoPurge)
	{
		// NOTE: This may cause an occasional hiccup,
		//  but it really needs to be done on the control thread
		//  (i.e. here)		
		purgeTextures(false);
		mDoPurge = FALSE;
	}
	LLMutexLock lock(&mWorkersMutex);
	LLTextureCacheWorker* worker = new LLTextureCacheRemoteWorker(this, priority, id,
																  data, datasize, 0,
																  imagesize, rawimage, discardlevel, responder);
	handle_t handle = worker->write();
	mWriters[handle] = worker;
	return handle;
}

//called in the main thread
LLPointer<LLImageRaw> LLTextureCache::readFromFastCache(const LLUUID& id, S32& discardlevel)
{
	U32 offset;
	{
		LLMutexLock lock(&mHeaderMutex);
		id_map_t::const_iterator iter = mHeaderIDMap.find(id);
		if(iter == mHeaderIDMap.end())
		{
			return NULL; //not in the cache
		}

		offset = iter->second;
	}
	offset *= TEXTURE_FAST_CACHE_ENTRY_SIZE;

	U8* data;
	S32 head[4];
	{
		LLMutexLock lock(&mFastCacheMutex);

		openFastCache();

		mFastCachep->seek(APR_SET, offset);		
	
		llassert_always(mFastCachep->read(head, TEXTURE_FAST_CACHE_ENTRY_OVERHEAD) == TEXTURE_FAST_CACHE_ENTRY_OVERHEAD);
		
		S32 image_size = head[0] * head[1] * head[2];
		if(!image_size) //invalid
		{
			closeFastCache();
			return NULL;
		}
		discardlevel = head[3];
		
		data =  (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), image_size);
		llassert_always(mFastCachep->read(data, image_size) == image_size);
		closeFastCache();
	}
	LLPointer<LLImageRaw> raw = new LLImageRaw(data, head[0], head[1], head[2], true);

	return raw;
}

//return the fast cache location
bool LLTextureCache::writeToFastCache(S32 id, LLPointer<LLImageRaw> raw, S32 discardlevel)
{
	//rescale image if needed
	S32 w, h, c;
	w = raw->getWidth();
	h = raw->getHeight();
	c = raw->getComponents();
	S32 i = 0 ;
	
	while(((w >> i) * (h >> i) * c) > TEXTURE_FAST_CACHE_ENTRY_SIZE - TEXTURE_FAST_CACHE_ENTRY_OVERHEAD)
	{
		++i ;
	}
		
	if(i)
	{
		w >>= i;
		h >>= i;
		if(w * h *c > 0) //valid
		{
			LLPointer<LLImageRaw> newraw = new LLImageRaw(raw->getData(), raw->getWidth(), raw->getHeight(), raw->getComponents());
			newraw->scale(w, h) ;
			raw = newraw;

			discardlevel += i ;
		}
	}
	
	//copy data
	memcpy(mFastCachePadBuffer, &w, sizeof(S32));
	memcpy(mFastCachePadBuffer + sizeof(S32), &h, sizeof(S32));
	memcpy(mFastCachePadBuffer + sizeof(S32) * 2, &c, sizeof(S32));
	memcpy(mFastCachePadBuffer + sizeof(S32) * 3, &discardlevel, sizeof(S32));
	if(w * h * c > 0) //valid
	{
		memcpy(mFastCachePadBuffer + TEXTURE_FAST_CACHE_ENTRY_OVERHEAD, raw->getData(), w * h * c);
	}
	S32 offset = id * TEXTURE_FAST_CACHE_ENTRY_SIZE;

	{
		LLMutexLock lock(&mFastCacheMutex);

		openFastCache();

		mFastCachep->seek(APR_SET, offset);	
		llassert_always(mFastCachep->write(mFastCachePadBuffer, TEXTURE_FAST_CACHE_ENTRY_SIZE) == TEXTURE_FAST_CACHE_ENTRY_SIZE);

		closeFastCache(true);
	}

	return true;
}

void LLTextureCache::openFastCache(bool first_time)
{
	if(!mFastCachep)
	{
		if(first_time)
		{
			if(!mFastCachePadBuffer)
			{
				mFastCachePadBuffer = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), TEXTURE_FAST_CACHE_ENTRY_SIZE);
			}
			mFastCachePoolp = new LLVolatileAPRPool();
			if (LLAPRFile::isExist(mFastCacheFileName, mFastCachePoolp))
			{
				mFastCachep = new LLAPRFile(mFastCacheFileName, APR_READ|APR_WRITE|APR_BINARY, mFastCachePoolp) ;				
			}
			else
			{
				mFastCachep = new LLAPRFile(mFastCacheFileName, APR_CREATE|APR_READ|APR_WRITE|APR_BINARY, mFastCachePoolp) ;
			}
		}
		else
		{
			mFastCachep = new LLAPRFile(mFastCacheFileName, APR_READ|APR_WRITE|APR_BINARY, mFastCachePoolp) ;
		}

		mFastCacheTimer.reset();
	}
	return;
}
	
void LLTextureCache::closeFastCache(bool forced)
{	
	static const F32 timeout = 10.f ; //seconds

	if(!mFastCachep)
	{
		return ;
	}

	if(!forced && mFastCacheTimer.getElapsedTimeF32() < timeout)
	{
		return ;
	}

	delete mFastCachep;
	mFastCachep = NULL;
	return;
}
	
bool LLTextureCache::writeComplete(handle_t handle, bool abort)
{
	lockWorkers();
	handle_map_t::iterator iter = mWriters.find(handle);
	llassert(iter != mWriters.end());
	if (iter != mWriters.end())
	{
		LLTextureCacheWorker* worker = iter->second;
		if (worker->complete() || abort)
		{
			mWriters.erase(handle);
			unlockWorkers();
			worker->scheduleDelete();
			return true;
		}
	}
	unlockWorkers();
	return false;
}

void LLTextureCache::prioritizeWrite(handle_t handle)
{
	// Don't prioritize yet, we might be working on this now
	//   which could create a deadlock
	LLMutexLock lock(&mListMutex);	
	mPrioritizeWriteList.push_back(handle);
}

void LLTextureCache::addCompleted(Responder* responder, bool success)
{
	LLMutexLock lock(&mListMutex);
	mCompletedList.push_back(std::make_pair(responder,success));
}

//////////////////////////////////////////////////////////////////////////////

//called after mHeaderMutex is locked.
void LLTextureCache::removeCachedTexture(const LLUUID& id)
{
	if(mTexturesSizeMap.find(id) != mTexturesSizeMap.end())
	{
		mTexturesSizeTotal -= mTexturesSizeMap[id] ;
		mTexturesSizeMap.erase(id);
	}
	mHeaderIDMap.erase(id);
	LLAPRFile::remove(getTextureFileName(id), getLocalAPRFilePool());		
}

//called after mHeaderMutex is locked.
void LLTextureCache::removeEntry(S32 idx, Entry& entry, std::string& filename)
{
 	bool file_maybe_exists = true;	// Always attempt to remove when idx is invalid.

	if(idx >= 0) //valid entry
	{
		if (entry.mBodySize == 0)	// Always attempt to remove when mBodySize > 0.
		{
		  if (LLAPRFile::isExist(filename, getLocalAPRFilePool()))		// Sanity check. Shouldn't exist when body size is 0.
		  {
			  LL_WARNS("TextureCache") << "Entry has body size of zero but file " << filename << " exists. Deleting this file, too." << LL_ENDL;
		  }
		  else
		  {
			  file_maybe_exists = false;
		  }
		}
		mTexturesSizeTotal -= entry.mBodySize;

		entry.mImageSize = -1;
		entry.mBodySize = 0;
		mHeaderIDMap.erase(entry.mID);
		mTexturesSizeMap.erase(entry.mID);		
		mFreeList.insert(idx);	
	}

	if (file_maybe_exists)
	{
		LLAPRFile::remove(filename, getLocalAPRFilePool());		
	}
}

bool LLTextureCache::removeFromCache(const LLUUID& id)
{
	//llwarns << "Removing texture from cache: " << id << llendl;
	bool ret = false ;
	if (!mReadOnly)
	{
		lockHeaders() ;

		Entry entry;
		S32 idx = openAndReadEntry(id, entry, false);
		std::string tex_filename = getTextureFileName(id);
		removeEntry(idx, entry, tex_filename) ;
		if (idx >= 0)
		{			
			writeEntryToHeaderImmediately(idx, entry);					
			ret = true;
		}

		unlockHeaders() ;
	}
	return ret ;
}

//////////////////////////////////////////////////////////////////////////////

LLTextureCache::ReadResponder::ReadResponder()
	: mImageSize(0),
	  mImageLocal(FALSE)
{
}

void LLTextureCache::ReadResponder::setData(U8* data, S32 datasize, S32 imagesize, S32 imageformat, BOOL imagelocal)
{
	if (mFormattedImage.notNull())
	{
		llassert_always(mFormattedImage->getCodec() == imageformat);
		mFormattedImage->appendData(data, datasize);
	}
	else
	{
		mFormattedImage = LLImageFormatted::createFromType(imageformat);
		mFormattedImage->setData(data,datasize);
	}
	mImageSize = imagesize;
	mImageLocal = imagelocal;
}

//////////////////////////////////////////////////////////////////////////////
