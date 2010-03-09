/** 
 * @file lltexturecache.cpp
 * @brief Object which handles local texture caching
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "lltexturecache.h"

#include "llapr.h"
#include "lldir.h"
#include "llimage.h"
#include "lllfsthread.h"
#include "llviewercontrol.h"

// Included to allow LLTextureCache::purgeTextures() to pause watchdog timeout
#include "llappviewer.h" 

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
		delete[] mReadData;
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
		mReadData = new U8[mDataSize];
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
				delete[] mReadData;
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
	mReadData = new U8[mDataSize];
	
	S32 bytes_read = LLAPRFile::readEx(mFileName, mReadData, mOffset, mDataSize, mCache->getLocalAPRFilePool());	

	if (bytes_read != mDataSize)
	{
// 		llwarns << "Error reading file from local cache: " << mFileName
// 				<< " Bytes: " << mDataSize << " Offset: " << mOffset
// 				<< " / " << mDataSize << llendl;
		mDataSize = 0;
		delete[] mReadData;
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
						 LLTextureCache::Responder* responder) 
			: LLTextureCacheWorker(cache, priority, id, data, datasize, offset, imagesize, responder),
			mState(INIT)
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
		mReadData = new U8[mDataSize];
		S32 bytes_read = LLAPRFile::readEx(local_filename, 
											 mReadData, mOffset, mDataSize, mCache->getLocalAPRFilePool());
		if (bytes_read != mDataSize)
		{
 			llwarns << "Error reading file from local cache: " << local_filename
 					<< " Bytes: " << mDataSize << " Offset: " << mOffset
 					<< " / " << mDataSize << llendl;
			mDataSize = 0;
			delete[] mReadData;
			mReadData = NULL;
		}
		else
		{
			mImageSize = local_size;
			mImageLocal = TRUE;
		}
		// We're done...
		done = true;
	}

	// Second state / stage : identify the cache or not...
	if (!done && (mState == CACHE))
	{
		idx = mCache->getHeaderCacheEntry(mID, mImageSize);
		if (idx < 0)
		{
			// The texture is *not* cached. We're done here...
			mDataSize = 0; // no data 
			done = true;
		}
		else
		{
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
		mReadData = new U8[size];
		S32 bytes_read = LLAPRFile::readEx(mCache->mHeaderDataFileName, 
											 mReadData, offset, size, mCache->getLocalAPRFilePool());
		if (bytes_read != size)
		{
			llwarns << "LLTextureCacheWorker: "  << mID
					<< " incorrect number of bytes read from header: " << bytes_read
					<< " / " << size << llendl;
			delete[] mReadData;
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
			U8* data = new U8[mDataSize];

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
				delete[] mReadData;
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
				delete[] mReadData;
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
		mState = CACHE;
	}
	
	// No LOCAL state for write(): because it doesn't make much sense to cache a local file...

	// Second state / stage : set an entry in the headers entry (texture.entries) file
	if (!done && (mState == CACHE))
	{
		bool alreadyCached = false;
		S32 cur_imagesize = 0;
		// Checks if this image is already in the entry list
		idx = mCache->getHeaderCacheEntry(mID, cur_imagesize);
		if (idx >= 0 && (cur_imagesize >= 0))
		{
			alreadyCached = true;	// already there and non empty
		}
		idx = mCache->setHeaderCacheEntry(mID, mImageSize); // create or touch the entry
		if (idx < 0)
		{
			llwarns << "LLTextureCacheWorker: "  << mID
					<< " Unable to create header entry for writing!" << llendl;
			mDataSize = -1; // failed
			done = true;
		}
		else
		{
			if (cur_imagesize > 0 && (mImageSize != cur_imagesize))
			{
				alreadyCached = false; // re-write the header if the size changed in all cases
			}
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
			U8* padBuffer = new U8[TEXTURE_CACHE_ENTRY_SIZE];
			memset(padBuffer, 0, TEXTURE_CACHE_ENTRY_SIZE);		// Init with zeros
			memcpy(padBuffer, mWriteData, mDataSize);			// Copy the write buffer
			bytes_written = LLAPRFile::writeEx(mCache->mHeaderDataFileName, padBuffer, offset, size, mCache->getLocalAPRFilePool());
			delete [] padBuffer;
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
		if ((file_size > 0) && mCache->updateTextureEntryList(mID, file_size))
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
		else
		{
			mDataSize = 0; // no data written
		}
		// Nothing else to do at that point...
		done = true;
	}

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
				delete[] mReadData;
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
	  mHeaderAPRFile(NULL),
	  mReadOnly(FALSE),
	  mTexturesSizeTotal(0),
	  mDoPurge(FALSE)
{
}

LLTextureCache::~LLTextureCache()
{
	clearDeleteList() ;
	writeUpdatedEntries() ;
}

//////////////////////////////////////////////////////////////////////////////

//virtual
S32 LLTextureCache::update(U32 max_time_ms)
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

bool LLTextureCache::updateTextureEntryList(const LLUUID& id, S32 bodysize)
{
	bool res = false;
	bool purge = false;
	{
		LLMutexLock lock(&mHeaderMutex);
		size_map_t::iterator iter1 = mTexturesSizeMap.find(id);
		if (iter1 == mTexturesSizeMap.end() || iter1->second < bodysize)
		{
			llassert_always(bodysize > 0);

			S32 oldbodysize = 0;
			if (iter1 != mTexturesSizeMap.end())
			{
				oldbodysize = iter1->second;
			}
						
			Entry entry;
			S32 idx = openAndReadEntry(id, entry, false);
			if (idx < 0)
			{
				llwarns << "Failed to open entry: " << id << llendl;	
				removeHeaderCacheEntry(id);
				LLAPRFile::remove(getTextureFileName(id), getLocalAPRFilePool());
				return false;
			}			
			else if (oldbodysize != entry.mBodySize)
			{
				// TODO: change to llwarns
				llerrs << "Entry mismatch in mTextureSizeMap / mHeaderIDMap"
					   << " idx=" << idx << " oldsize=" << oldbodysize << " entrysize=" << entry.mBodySize << llendl;
			}
			entry.mBodySize = bodysize;
			writeEntryAndClose(idx, entry);
			
			mTexturesSizeTotal -= oldbodysize;
			mTexturesSizeTotal += bodysize;
			
			if (mTexturesSizeTotal > sCacheMaxTexturesSize)
			{
				purge = true;
			}
			res = true;
		}
	}
	if (purge)
	{
		mDoPurge = TRUE;
	}
	return res;
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
const S32 MAX_REASONABLE_FILE_SIZE = 512*1024*1024; // 512 MB
F32 LLTextureCache::sHeaderCacheVersion = 1.4f;
U32 LLTextureCache::sCacheMaxEntries = MAX_REASONABLE_FILE_SIZE / TEXTURE_CACHE_ENTRY_SIZE;
S64 LLTextureCache::sCacheMaxTexturesSize = 0; // no limit
const char* entries_filename = "texture.entries";
const char* cache_filename = "texture.cache";
const char* textures_dirname = "textures";

void LLTextureCache::setDirNames(ELLPath location)
{
	std::string delem = gDirUtilp->getDirDelimiter();
	mHeaderEntriesFileName = gDirUtilp->getExpandedFilename(location, entries_filename);
	mHeaderDataFileName = gDirUtilp->getExpandedFilename(location, cache_filename);
	mTexturesDirName = gDirUtilp->getExpandedFilename(location, textures_dirname);
}

void LLTextureCache::purgeCache(ELLPath location)
{
	LLMutexLock lock(&mHeaderMutex);

	if (!mReadOnly)
	{
		setDirNames(location);
		llassert_always(mHeaderAPRFile == NULL);
		LLAPRFile::remove(mHeaderEntriesFileName, getLocalAPRFilePool());
		LLAPRFile::remove(mHeaderDataFileName, getLocalAPRFilePool());
	}
	purgeAllTextures(true);
}

S64 LLTextureCache::initCache(ELLPath location, S64 max_size, BOOL read_only)
{
	mReadOnly = read_only;
	
	S64 header_size = (max_size * 2) / 10;
	S64 max_entries = header_size / TEXTURE_CACHE_ENTRY_SIZE;
	sCacheMaxEntries = (S32)(llmin((S64)sCacheMaxEntries, max_entries));
	header_size = sCacheMaxEntries * TEXTURE_CACHE_ENTRY_SIZE;
	max_size -= header_size;
	if (sCacheMaxTexturesSize > 0)
		sCacheMaxTexturesSize = llmin(sCacheMaxTexturesSize, max_size);
	else
		sCacheMaxTexturesSize = max_size;
	max_size -= sCacheMaxTexturesSize;
	
	LL_INFOS("TextureCache") << "Headers: " << sCacheMaxEntries
			<< " Textures size: " << sCacheMaxTexturesSize/(1024*1024) << " MB" << LL_ENDL;

	setDirNames(location);
	
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

	return max_size; // unused cache space
}

//----------------------------------------------------------------------------
// mHeaderMutex must be locked for the following functions!

LLAPRFile* LLTextureCache::openHeaderEntriesFile(bool readonly, S32 offset)
{
	llassert_always(mHeaderAPRFile == NULL);
	apr_int32_t flags = readonly ? APR_READ|APR_BINARY : APR_READ|APR_WRITE|APR_BINARY;
	mHeaderAPRFile = new LLAPRFile(mHeaderEntriesFileName, flags, getLocalAPRFilePool());
	mHeaderAPRFile->seek(APR_SET, offset);
	return mHeaderAPRFile;
}

void LLTextureCache::closeHeaderEntriesFile()
{
	llassert_always(mHeaderAPRFile != NULL);
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

static S32 mHeaderEntriesMaxWriteIdx = 0;

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
						mHeaderIDMap.erase(oldid);
						mTexturesSizeMap.erase(oldid);
						break;
					}
				}
				// if (idx < 0) at this point, we will rebuild the LRU 
				//  and retry if called from setHeaderCacheEntry(),
				//  otherwise this shouldn't happen and will trigger an error
			}
			if (idx >= 0)
			{
				// Set the header index
				mHeaderIDMap[id] = idx;
				llassert_always(mTexturesSizeMap.erase(id) == 0);
				// Initialize the entry (will get written later)
				entry.init(id, time(NULL));
				// Update Header
				writeEntriesHeader();
				
				//the new entry, write immediately.
				// Write Entry
				S32 offset = sizeof(EntriesInfo) + idx * sizeof(Entry);
				LLAPRFile* aprfile = openHeaderEntriesFile(false, offset);
				S32 bytes_written = aprfile->write((void*)&entry, (S32)sizeof(Entry));
				llassert_always(bytes_written == sizeof(Entry));
				mHeaderEntriesMaxWriteIdx = llmax(mHeaderEntriesMaxWriteIdx, idx);
				closeHeaderEntriesFile();
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
			S32 offset = sizeof(EntriesInfo) + idx * sizeof(Entry);
			LLAPRFile* aprfile = openHeaderEntriesFile(true, offset);
			S32 bytes_read = aprfile->read((void*)&entry, (S32)sizeof(Entry));
			llassert_always(bytes_read == sizeof(Entry));			
			closeHeaderEntriesFile();
		}
		llassert_always(entry.mImageSize == 0 || entry.mImageSize == -1 || entry.mImageSize > entry.mBodySize);
	}
	return idx;
}

void LLTextureCache::writeEntryAndClose(S32 idx, Entry& entry)
{
	if (idx >= 0)
	{
		if (!mReadOnly)
		{
			entry.mTime = time(NULL);
			llassert_always(entry.mImageSize == 0 || entry.mImageSize == -1 || entry.mImageSize > entry.mBodySize);
			if (entry.mBodySize > 0)
			{
				mTexturesSizeMap[entry.mID] = entry.mBodySize;
			}
// 			llinfos << "Updating TE: " << idx << ": " << id << " Size: " << entry.mBodySize << " Time: " << entry.mTime << llendl;
			mHeaderEntriesMaxWriteIdx = llmax(mHeaderEntriesMaxWriteIdx, idx);
			mUpdatedEntryMap[idx] = entry ;
		}
	}
}

U32 LLTextureCache::openAndReadEntries(std::vector<Entry>& entries)
{
	U32 num_entries = mHeaderEntriesInfo.mEntries;

	mHeaderIDMap.clear();
	mTexturesSizeMap.clear();
	mFreeList.clear();
	mTexturesSizeTotal = 0;

	LLAPRFile* aprfile = openHeaderEntriesFile(false, (S32)sizeof(EntriesInfo));
	updatedHeaderEntriesFile() ;
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
		if (entry.mImageSize < 0)
		{
			mFreeList.insert(idx);
		}
		else
		{
			mHeaderIDMap[entry.mID] = idx;
			if (entry.mBodySize > 0)
			{
				mTexturesSizeMap[entry.mID] = entry.mBodySize;
				mTexturesSizeTotal += entry.mBodySize;
			}
			llassert_always(entry.mImageSize == 0 || entry.mImageSize > entry.mBodySize);
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
			llassert_always(bytes_written == sizeof(Entry));
		}
		mHeaderEntriesMaxWriteIdx = llmax(mHeaderEntriesMaxWriteIdx, num_entries-1);
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
		llassert_always(bytes_written == sizeof(EntriesInfo));
		
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
			llassert_always(bytes_written == entry_size);
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
			std::set<LLUUID> purge_list;
			for (U32 i=0; i<num_entries; i++)
			{
				Entry& entry = entries[i];
				const LLUUID& id = entry.mID;
				if (entry.mImageSize < 0)
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
							llwarns << "Bad entry: " << i << ": " << id << ": BodySize: " << entry.mBodySize << llendl;
							purge_list.insert(entry.mID);
							entry.mImageSize = -1; // empty/available
						}
					}
				}
			}
			if (num_entries > sCacheMaxEntries)
			{
				// Special case: cache size was reduced, need to remove entries
				// Note: After we prune entries, we will call this again and create the LRU
				U32 entries_to_purge = (num_entries-empty_entries) - sCacheMaxEntries;
				llinfos << "Texture Cache Entries: " << num_entries << " Max: " << sCacheMaxEntries << " Empty: " << empty_entries << " Purging: " << entries_to_purge << llendl;
				if (entries_to_purge > 0)
				{
					for (std::set<lru_data_t>::iterator iter = lru.begin(); iter != lru.end(); ++iter)
					{
						S32 idx = iter->second;
						if (entries[idx].mImageSize >= 0)
						{
							purge_list.insert(entries[idx].mID);
							entries[idx].mImageSize = -1;
							if (purge_list.size() >= entries_to_purge)
								break;
						}
					}
				}
				llassert_always(purge_list.size() >= entries_to_purge);
			}
			else
			{
				S32 lru_entries = (S32)((F32)sCacheMaxEntries * TEXTURE_CACHE_LRU_SIZE);
				for (std::set<lru_data_t>::iterator iter = lru.begin(); iter != lru.end(); ++iter)
				{
					S32 idx = iter->second;
					const LLUUID& id = entries[idx].mID;
					mLRU.insert(id);
// 					llinfos << "LRU: " << iter->first << " : " << iter->second << llendl;
					if (--lru_entries <= 0)
						break;
				}
			}
			
			if (purge_list.size() > 0)
			{
				for (std::set<LLUUID>::iterator iter = purge_list.begin(); iter != purge_list.end(); ++iter)
				{
					const LLUUID& id = *iter;
					bool res = removeHeaderCacheEntry(id); // sets entry size on disk to -1
					llassert_always(res);
					LLAPRFile::remove(getTextureFileName(id), getLocalAPRFilePool());
				}
				// If we removed any entries, we need to rebuild the entries list,
				// write the header, and call this again
				std::vector<Entry> new_entries;
				for (U32 i=0; i<num_entries; i++)
				{
					const Entry& entry = entries[i];
					if (entry.mImageSize >=0)
					{
						new_entries.push_back(entry);
					}
				}
				llassert_always(new_entries.size() <= sCacheMaxEntries);
				mHeaderEntriesInfo.mEntries = new_entries.size();
				writeEntriesAndClose(new_entries);
				mHeaderMutex.unlock(); // unlock the mutex before calling again
				readHeaderCache(); // repeat with new entries file
				mHeaderMutex.lock();
			}
			else
			{
				writeEntriesAndClose(entries);
			}
		}
	}
	mHeaderMutex.unlock();
}

//////////////////////////////////////////////////////////////////////////////

void LLTextureCache::purgeAllTextures(bool purge_directories)
{
	if (!mReadOnly)
	{
		const char* subdirs = "0123456789abcdef";
		std::string delem = gDirUtilp->getDirDelimiter();
		std::string mask = delem + "*";
		for (S32 i=0; i<16; i++)
		{
			std::string dirname = mTexturesDirName + delem + subdirs[i];
			llinfos << "Deleting files in directory: " << dirname << llendl;
			gDirUtilp->deleteFilesInDir(dirname,mask);
			if (purge_directories)
			{
				LLFile::rmdir(dirname);
			}
		}
		if (purge_directories)
		{
			LLFile::rmdir(mTexturesDirName);
		}
	}
	mHeaderIDMap.clear();
	mTexturesSizeMap.clear();
	mTexturesSizeTotal = 0;
	mFreeList.clear();
	mTexturesSizeTotal = 0;

	// Info with 0 entries
	mHeaderEntriesInfo.mVersion = sHeaderCacheVersion;
	mHeaderEntriesInfo.mEntries = 0;
	writeEntriesHeader();
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
		writeEntriesAndClose(entries);
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
		}
	}
	
	// Validate 1/256th of the files on startup
	U32 validate_idx = 0;
	if (validate)
	{
		validate_idx = gSavedSettings.getU32("CacheValidateCounter");
		U32 next_idx = (++validate_idx) % 256;
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
			LLAPRFile::remove(filename, getLocalAPRFilePool());
			cache_size -= entries[idx].mBodySize;
			mTexturesSizeTotal -= entries[idx].mBodySize;
			entries[idx].mBodySize = 0;
			mTexturesSizeMap.erase(entries[idx].mID);
		}
	}

	LL_DEBUGS("TextureCache") << "TEXTURE CACHE: Writing Entries: " << num_entries << LL_ENDL;

	writeEntriesAndClose(entries);
	
	// *FIX:Mani - watchdog back on.
	LLAppViewer::instance()->resumeMainloopTimeout();
	
	LL_INFOS("TextureCache") << "TEXTURE CACHE:"
			<< " PURGED: " << purge_count
			<< " ENTRIES: " << num_entries
			<< " CACHE SIZE: " << mTexturesSizeTotal / 1024*1024 << " MB"
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
S32 LLTextureCache::getHeaderCacheEntry(const LLUUID& id, S32& imagesize)
{
	LLMutexLock lock(&mHeaderMutex);
	Entry entry;
	S32 idx = openAndReadEntry(id, entry, false);
	if (idx >= 0)
	{
		imagesize = entry.mImageSize;
		writeEntryAndClose(idx, entry); // updates time
	}
	return idx;
}

// Writes imagesize to the header, updates timestamp
S32 LLTextureCache::setHeaderCacheEntry(const LLUUID& id, S32 imagesize)
{
	mHeaderMutex.lock();
	llassert_always(imagesize >= 0);
	Entry entry;
	S32 idx = openAndReadEntry(id, entry, true);
	if (idx >= 0)
	{
		entry.mImageSize = imagesize;
		writeEntryAndClose(idx, entry);
		mHeaderMutex.unlock();
	}
	else // retry
	{
		mHeaderMutex.unlock();
		readHeaderCache(); // We couldn't write an entry, so refresh the LRU
		mHeaderMutex.lock();
		llassert_always(!mLRU.empty() || mHeaderEntriesInfo.mEntries < sCacheMaxEntries);
		mHeaderMutex.unlock();
		idx = setHeaderCacheEntry(id, imagesize); // assert above ensures no inf. recursion
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
																  0, responder);
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
																  imagesize, responder);
	handle_t handle = worker->write();
	mWriters[handle] = worker;
	return handle;
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

// Called from MAIN thread (endWork())
// Ensure that mHeaderMutex is locked first!
bool LLTextureCache::removeHeaderCacheEntry(const LLUUID& id)
{
	Entry entry;
	S32 idx = openAndReadEntry(id, entry, false);
	if (idx >= 0)
	{
		entry.mImageSize = -1;
		entry.mBodySize = 0;
		writeEntryAndClose(idx, entry);
		mFreeList.insert(idx);
		mHeaderIDMap.erase(id);
		mTexturesSizeMap.erase(id);
		return true;
	}
	return false;
}

void LLTextureCache::removeFromCache(const LLUUID& id)
{
	//llwarns << "Removing texture from cache: " << id << llendl;
	if (!mReadOnly)
	{
		LLMutexLock lock(&mHeaderMutex);
		removeHeaderCacheEntry(id);
		LLAPRFile::remove(getTextureFileName(id), getLocalAPRFilePool());
	}
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
