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

#define USE_LFS_READ 0
#define USE_LFS_WRITE 0

// Note: first 4 bytes store file size, rest is j2c data
const S32 TEXTURE_CACHE_ENTRY_SIZE = FIRST_PACKET_SIZE; //1024;

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

bool LLTextureCacheRemoteWorker::doRead()
{
	S32 local_size = 0;
	std::string local_filename;
	
	if (mState == INIT)
	{
		std::string filename = mCache->getLocalFileName(mID);	
		local_filename = filename + ".j2c";
		local_size = LLAPRFile::size(local_filename, mCache->getLocalAPRFilePool());
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
		if (local_size > 0)
		{
			mState = LOCAL;
		}
		else
		{
			mState = CACHE;
		}
	}

	if (mState == LOCAL)
	{
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
// 					llwarns << "Error reading file from local cache: " << local_filename
// 							<< " Bytes: " << mDataSize << " Offset: " << mOffset
// 							<< " / " << mDataSize << llendl;
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
		S32 bytes_read = LLAPRFile::readEx(local_filename, 
											 mReadData, mOffset, mDataSize, mCache->getLocalAPRFilePool());
		if (bytes_read != mDataSize)
		{
// 			llwarns << "Error reading file from local cache: " << local_filename
// 					<< " Bytes: " << mDataSize << " Offset: " << mOffset
// 					<< " / " << mDataSize << llendl;
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

	S32 idx = -1;
	
	if (mState == CACHE)
	{
		llassert_always(mImageSize == 0);
		idx = mCache->getHeaderCacheEntry(mID, false, &mImageSize);
		if (idx >= 0 && mImageSize > mOffset)
		{
			llassert_always(mImageSize > 0);
			if (!mDataSize || mDataSize > mImageSize)
			{
				mDataSize = mImageSize;
			}
			mState = mOffset < TEXTURE_CACHE_ENTRY_SIZE ? HEADER : BODY;
		}
		else
		{
			mDataSize = 0; // no data
			return true;
		}
	}

	if (mState == HEADER)
	{
#if USE_LFS_READ
		if (mFileHandle == LLLFSThread::nullHandle())
		{
			llassert_always(idx >= 0);
			llassert_always(mOffset < TEXTURE_CACHE_ENTRY_SIZE);
			S32 offset = idx * TEXTURE_CACHE_ENTRY_SIZE + mOffset;
			S32 size = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
			llassert_always(mReadData == NULL);
			mReadData = new U8[size];
			mBytesRead = -1;
			mBytesToRead = size;
			setPriority(LLWorkerThread::PRIORITY_LOW | mPriority);
			mFileHandle = LLLFSThread::sLocal->read(mCache->mHeaderDataFileName,
													mReadData, offset, mBytesToRead,
													new ReadResponder(mCache, mRequestHandle));
			return false;
		}
		else
		{
			if (mBytesRead >= 0)
			{
				if (mBytesRead != mBytesToRead)
				{
// 					llwarns << "LLTextureCacheWorker: "  << mID
// 							<< " incorrect number of bytes read from header: " << mBytesRead
// 							<< " != " << mBytesToRead << llendl;
					mDataSize = -1; // failed
					return true;
				}
				if (mDataSize <= TEXTURE_CACHE_ENTRY_SIZE)
				{
					return true; // done
				}
				else
				{
					mFileHandle = LLLFSThread::nullHandle();
					mState = BODY;
				}
			}
			else
			{
				return false;
			}
		}
#else
		llassert_always(idx >= 0);
		llassert_always(mOffset < TEXTURE_CACHE_ENTRY_SIZE);
		S32 offset = idx * TEXTURE_CACHE_ENTRY_SIZE + mOffset;
		S32 size = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
		mReadData = new U8[size];
		S32 bytes_read = LLAPRFile::readEx(mCache->mHeaderDataFileName, 
											 mReadData, offset, size, mCache->getLocalAPRFilePool());
		if (bytes_read != size)
		{
// 			llwarns << "LLTextureCacheWorker: "  << mID
// 					<< " incorrect number of bytes read from header: " << bytes_read
// 					<< " / " << size << llendl;
			mDataSize = -1; // failed
			return true;
		}
		if (mDataSize <= TEXTURE_CACHE_ENTRY_SIZE)
		{
			return true; // done
		}
		else
		{
			mState = BODY;
		}
#endif
	}

	if (mState == BODY)
	{
#if USE_LFS_READ
		if (mFileHandle == LLLFSThread::nullHandle())
		{
			std::string filename = mCache->getTextureFileName(mID);
			S32 filesize = LLAPRFile::size(filename, mCache->getLocalAPRFilePool());
			if (filesize > mOffset)
			{
				S32 datasize = TEXTURE_CACHE_ENTRY_SIZE + filesize;
				mDataSize = llmin(datasize, mDataSize);
				S32 data_offset = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
				data_offset = llmax(data_offset, 0);
				S32 file_size = mDataSize - data_offset;
				S32 file_offset = mOffset - TEXTURE_CACHE_ENTRY_SIZE;
				file_offset = llmax(file_offset, 0);

				llassert_always(mDataSize > 0);
				U8* data = new U8[mDataSize];
				if (data_offset > 0)
				{
					llassert_always(mReadData);
					llassert_always(data_offset <= mDataSize);
					memcpy(data, mReadData, data_offset);
					delete[] mReadData;
					mReadData = NULL;
				}
				llassert_always(mReadData == NULL);
				mReadData = data;

				mBytesRead = -1;
				mBytesToRead = file_size;
				setPriority(LLWorkerThread::PRIORITY_LOW | mPriority);
				llassert_always(data_offset + mBytesToRead <= mDataSize);
				mFileHandle = LLLFSThread::sLocal->read(filename,
														mReadData + data_offset, file_offset, mBytesToRead,
														new ReadResponder(mCache, mRequestHandle));
				return false;
			}
			else
			{
				mDataSize = TEXTURE_CACHE_ENTRY_SIZE;
				return true; // done
			}
		}
		else
		{
			if (mBytesRead >= 0)
			{
				if (mBytesRead != mBytesToRead)
				{
// 					llwarns << "LLTextureCacheWorker: "  << mID
// 							<< " incorrect number of bytes read from body: " << mBytesRead
// 							<< " != " << mBytesToRead << llendl;
					mDataSize = -1; // failed
				}
				return true;
			}
			else
			{
				return false;
			}
		}
#else
		std::string filename = mCache->getTextureFileName(mID);
		S32 filesize = LLAPRFile::size(filename, mCache->getLocalAPRFilePool());
		S32 bytes_read = 0;
		if (filesize > mOffset)
		{
			S32 datasize = TEXTURE_CACHE_ENTRY_SIZE + filesize;
			mDataSize = llmin(datasize, mDataSize);
			S32 data_offset = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
			data_offset = llmax(data_offset, 0);
			S32 file_size = mDataSize - data_offset;
			S32 file_offset = mOffset - TEXTURE_CACHE_ENTRY_SIZE;
			file_offset = llmax(file_offset, 0);
			
			U8* data = new U8[mDataSize];
			if (data_offset > 0)
			{
				llassert_always(mReadData);
				memcpy(data, mReadData, data_offset);
				delete[] mReadData;
			}
			mReadData = data;
			bytes_read = LLAPRFile::readEx(filename, 
											 mReadData + data_offset,
											 file_offset, file_size,
											 mCache->getLocalAPRFilePool());
			if (bytes_read != file_size)
			{
// 				llwarns << "LLTextureCacheWorker: "  << mID
// 						<< " incorrect number of bytes read from body: " << bytes_read
// 						<< " / " << file_size << llendl;
				mDataSize = -1; // failed
				return true;
			}
		}
		else
		{
			mDataSize = TEXTURE_CACHE_ENTRY_SIZE;
		}
		
		return true;
#endif
	}
	
	return false;
}

bool LLTextureCacheRemoteWorker::doWrite()
{
	S32 idx = -1;

	// No LOCAL state for write()
	
	if (mState == INIT)
	{
		S32 cur_imagesize = 0;
		S32 offset = mOffset;
		idx = mCache->getHeaderCacheEntry(mID, false, &cur_imagesize);
		if (idx >= 0 && cur_imagesize > 0)
		{
			offset = TEXTURE_CACHE_ENTRY_SIZE; // don't re-write header
		}
		idx = mCache->getHeaderCacheEntry(mID, true, &mImageSize); // touch entry
		if (idx >= 0)
		{
			if(cur_imagesize > 0 && mImageSize != cur_imagesize)
			{
// 				llwarns << "Header cache entry size: " << cur_imagesize << " != mImageSize: " << mImageSize << llendl;
				offset = 0; // re-write header
			}
			mState = offset < TEXTURE_CACHE_ENTRY_SIZE ? HEADER : BODY;
		}
		else
		{
			mDataSize = -1; // failed
			return true;
		}
	}
	
	if (mState == HEADER)
	{
#if USE_LFS_WRITE
		if (mFileHandle == LLLFSThread::nullHandle())
		{
			llassert_always(idx >= 0);
			llassert_always(mOffset < TEXTURE_CACHE_ENTRY_SIZE);
			S32 offset = idx * TEXTURE_CACHE_ENTRY_SIZE + mOffset;
			S32 size = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
			mBytesRead = -1;
			mBytesToRead = size;
			setPriority(LLWorkerThread::PRIORITY_LOW | mPriority);
			mFileHandle = LLLFSThread::sLocal->write(mCache->mHeaderDataFileName,
													 mWriteData, offset, mBytesToRead,
													 new WriteResponder(mCache, mRequestHandle));
			return false;
		}
		else
		{
			if (mBytesRead >= 0)
			{
				if (mBytesRead != mBytesToRead)
				{
// 					llwarns << "LLTextureCacheWorker: "  << mID
// 							<< " incorrect number of bytes written to header: " << mBytesRead
// 							<< " != " << mBytesToRead << llendl;
					mDataSize = -1; // failed
					return true;
				}
				if (mDataSize <=  mBytesToRead)
				{
					return true; // done
				}
				else
				{
					mFileHandle = LLLFSThread::nullHandle();
					mState = BODY;
				}
			}
			else
			{
				return false;
			}
		}
#else
		llassert_always(idx >= 0);
		llassert_always(mOffset < TEXTURE_CACHE_ENTRY_SIZE);
		S32 offset = idx * TEXTURE_CACHE_ENTRY_SIZE + mOffset;
		S32 size = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
		S32 bytes_written = LLAPRFile::writeEx(mCache->mHeaderDataFileName, mWriteData, offset, size, mCache->getLocalAPRFilePool());		

		if (bytes_written <= 0)
		{
// 			llwarns << "LLTextureCacheWorker: missing entry: " << mID << llendl;
			mDataSize = -1; // failed
			return true;
		}

		if (mDataSize <= size)
		{
			return true; // done
		}
		else
		{
			mState = BODY;
		}
#endif
	}
	
	if (mState == BODY)
	{
#if USE_LFS_WRITE
		if (mFileHandle == LLLFSThread::nullHandle())
		{
			S32 data_offset = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
			data_offset = llmax(data_offset, 0);
			S32 file_size = mDataSize - data_offset;
			S32 file_offset = mOffset - TEXTURE_CACHE_ENTRY_SIZE;
			file_offset = llmax(file_offset, 0);
			if (file_size > 0 && mCache->appendToTextureEntryList(mID, file_size))
			{
				std::string filename = mCache->getTextureFileName(mID);
				mBytesRead = -1;
				mBytesToRead = file_size;
				setPriority(LLWorkerThread::PRIORITY_LOW | mPriority);
				mFileHandle = LLLFSThread::sLocal->write(filename,
														 mWriteData + data_offset, file_offset, mBytesToRead,
														 new WriteResponder(mCache, mRequestHandle));
				return false;
			}
			else
			{
				mDataSize = 0; // no data written
				return true; // done
			}
		}
		else
		{
			if (mBytesRead >= 0)
			{
				if (mBytesRead != mBytesToRead)
				{
// 					llwarns << "LLTextureCacheWorker: "  << mID
// 							<< " incorrect number of bytes written to body: " << mBytesRead
// 							<< " != " << mBytesToRead << llendl;
					mDataSize = -1; // failed
				}
				return true;
			}
			else
			{
				return false;
			}
		}
#else
		S32 data_offset = TEXTURE_CACHE_ENTRY_SIZE - mOffset;
		data_offset = llmax(data_offset, 0);
		S32 file_size = mDataSize - data_offset;
		S32 file_offset = mOffset - TEXTURE_CACHE_ENTRY_SIZE;
		file_offset = llmax(file_offset, 0);
		S32 bytes_written = 0;
		if (file_size > 0 && mCache->appendToTextureEntryList(mID, file_size))
		{
			std::string filename = mCache->getTextureFileName(mID);
			
			bytes_written = LLAPRFile::writeEx(filename, 
												 mWriteData + data_offset,
												 file_offset, file_size,
												 mCache->getLocalAPRFilePool());
			if (bytes_written <= 0)
			{
				mDataSize = -1; // failed
			}
		}
		else
		{
			mDataSize = 0; // no data written
		}

		return true;
#endif
	}
	
	return false;
}

//virtual
bool LLTextureCacheWorker::doWork(S32 param)
{
// *TODO reenable disabled apr_pool usage disabled due to maint-render-9 merge breakage -brad
	//allocate a new local apr_pool
//	LLAPRPool pool ;

	//save the current mFileAPRPool to avoid breaking anything.
//	apr_pool_t* old_pool = mCache->getFileAPRPool() ;
	//make mFileAPRPool to point to the local one
//	mCache->setFileAPRPool(pool.getAPRPool()) ;

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

	//set mFileAPRPool back, the local one will be released automatically.
//	mCache->setFileAPRPool(old_pool) ;

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
	  mReadOnly(FALSE),
	  mTexturesSizeTotal(0),
	  mDoPurge(FALSE)
{
}

LLTextureCache::~LLTextureCache()
{
}

//////////////////////////////////////////////////////////////////////////////

//virtual
S32 LLTextureCache::update(U32 max_time_ms)
{
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

	for (responder_list_t::iterator iter1 = completed_list.begin();
		 iter1 != completed_list.end(); ++iter1)
	{
		Responder *responder = iter1->first;
		bool success = iter1->second;
		responder->completed(success);
	}
	
	unlockWorkers();
	
	return res;
}

//////////////////////////////////////////////////////////////////////////////
// search for local copy of UUID-based image file
std::string LLTextureCache::getLocalFileName(const LLUUID& id)
{
	// Does not include extension
	std::string idstr = id.asString();
	// TODO: should we be storing cached textures in skin directory?
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_SKINS, "default", "textures", idstr);
	return filename;
}

std::string LLTextureCache::getTextureFileName(const LLUUID& id)
{
	std::string idstr = id.asString();
	std::string delem = gDirUtilp->getDirDelimiter();
	std::string filename = mTexturesDirName + delem + idstr[0] + delem + idstr;
	return filename;
}

bool LLTextureCache::appendToTextureEntryList(const LLUUID& id, S32 bodysize)
{
	bool res = false;
	bool purge = false;
	// Append UUID to end of texture entries
	{
		LLMutexLock lock(&mHeaderMutex);
		size_map_t::iterator iter = mTexturesSizeMap.find(id);
		if (iter == mTexturesSizeMap.end() || iter->second < bodysize)
		{
			llassert_always(bodysize > 0);
			Entry* entry = new Entry(id, bodysize, time(NULL));

			LLAPRFile::writeEx(mTexturesDirEntriesFileName,
								 (U8*)entry, -1, 1*sizeof(Entry),
								 getLocalAPRFilePool());			
			delete entry;
			if (iter != mTexturesSizeMap.end())
			{
				mTexturesSizeTotal -= iter->second;
			}
			mTexturesSizeTotal += bodysize;
			mTexturesSizeMap[id] = bodysize;
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

//////////////////////////////////////////////////////////////////////////////

//static
const S32 MAX_REASONABLE_FILE_SIZE = 512*1024*1024; // 512 MB
F32 LLTextureCache::sHeaderCacheVersion = 1.0f;
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
	mTexturesDirEntriesFileName = mTexturesDirName + delem + entries_filename;
}

void LLTextureCache::purgeCache(ELLPath location)
{
	if (!mReadOnly)
	{
		setDirNames(location);
	
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

struct lru_data
{
	lru_data(U32 t, S32 i, const LLUUID& id) { time=t; index=i; uuid=id; }
	U32 time;
	S32 index;
	LLUUID uuid;
	struct Compare
	{
		// lhs < rhs
		typedef const lru_data* lru_data_ptr;
		bool operator()(const lru_data_ptr& a, const lru_data_ptr& b) const
		{
			if(a->time == b->time)
				return (a->index < b->index);
			else
				return (a->time >= b->time);
		}
	};				
};

// Called from either the main thread or the worker thread
void LLTextureCache::readHeaderCache()
{
	LLMutexLock lock(&mHeaderMutex);
	mHeaderEntriesInfo.mVersion = 0.f;
	mHeaderEntriesInfo.mEntries = 0;
	if (LLAPRFile::isExist(mHeaderEntriesFileName, getLocalAPRFilePool()))
	{
		LLAPRFile::readEx(mHeaderEntriesFileName,
							(U8*)&mHeaderEntriesInfo, 0, sizeof(EntriesInfo),
							getLocalAPRFilePool());
	}
	if (mHeaderEntriesInfo.mVersion != sHeaderCacheVersion)
	{
		if (!mReadOnly)
		{
			// Info with 0 entries
			mHeaderEntriesInfo.mVersion = sHeaderCacheVersion;

			LLAPRFile::writeEx(mHeaderEntriesFileName, 
								 (U8*)&mHeaderEntriesInfo, 0, sizeof(EntriesInfo),
								 getLocalAPRFilePool());
		}
	}
	else
	{
		S32 num_entries = mHeaderEntriesInfo.mEntries;
		if (num_entries)
		{
			Entry* entries = new Entry[num_entries];
			{
				LLAPRFile::readEx(mHeaderEntriesFileName, 
								(U8*)entries, sizeof(EntriesInfo), num_entries*sizeof(Entry),
								getLocalAPRFilePool());
			}
			typedef std::set<lru_data*, lru_data::Compare> lru_set_t;
			lru_set_t lru;
			for (S32 i=0; i<num_entries; i++)
			{
				if (entries[i].mSize >= 0) // -1 indicates erased entry, skip
				{
					const LLUUID& id = entries[i].mID;
					lru.insert(new lru_data(entries[i].mTime, i, id));
					mHeaderIDMap[id] = i;
				}
			}
			mLRU.clear();
			S32 lru_entries = sCacheMaxEntries / 10;
			for (lru_set_t::iterator iter = lru.begin(); iter != lru.end(); ++iter)
			{
				lru_data* data = *iter;
				mLRU[data->index] = data->uuid;
				if (--lru_entries <= 0)
					break;
			}
			for_each(lru.begin(), lru.end(), DeletePointer());
			delete[] entries;
		}
	}
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
			gDirUtilp->deleteFilesInDir(dirname,mask);
			if (purge_directories)
			{
				LLFile::rmdir(dirname);
			}
		}
		LLAPRFile::remove(mTexturesDirEntriesFileName, getLocalAPRFilePool());
		if (purge_directories)
		{
			LLFile::rmdir(mTexturesDirName);
		}
	}
	mTexturesSizeMap.clear();
}

void LLTextureCache::purgeTextures(bool validate)
{
	if (mReadOnly)
	{
		return;
	}

	// *FIX:Mani - watchdog off.
	LLAppViewer::instance()->pauseMainloopTimeout();

	LLMutexLock lock(&mHeaderMutex);
	
	S32 filesize = LLAPRFile::size(mTexturesDirEntriesFileName, getLocalAPRFilePool());
	S32 num_entries = filesize / sizeof(Entry);
	if (num_entries * (S32)sizeof(Entry) != filesize)
	{
		LL_WARNS("TextureCache") << "Bad cache file: " << mTexturesDirEntriesFileName << " Purging." << LL_ENDL;
		purgeAllTextures(false);
		return;
	}
	if (num_entries == 0)
	{
		return; // nothing to do
	}
	
	Entry* entries = new Entry[num_entries];
	S32 bytes_read = LLAPRFile::readEx(mTexturesDirEntriesFileName, 
										 (U8*)entries, 0, num_entries*sizeof(Entry),
										 getLocalAPRFilePool());	
	if (bytes_read != filesize)
	{
		LL_WARNS("TextureCache") << "Bad cache file (2): " << mTexturesDirEntriesFileName << " Purging." << LL_ENDL;
		purgeAllTextures(false);
		return;
	}
	
	LL_DEBUGS("TextureCache") << "TEXTURE CACHE: Reading Entries..." << LL_ENDL;
	
	std::map<LLUUID, S32> entry_idx_map;
	S64 total_size = 0;
	for (S32 idx=0; idx<num_entries; idx++)
	{
		const LLUUID& id = entries[idx].mID;
 		LL_DEBUGS("TextureCache") << "Entry: " << id << " Size: " << entries[idx].mSize << " Time: " << entries[idx].mTime << LL_ENDL;
		std::map<LLUUID, S32>::iterator iter = entry_idx_map.find(id);
		if (iter != entry_idx_map.end())
		{
			// Newer entry replacing older entry
			S32 pidx = iter->second;
			total_size -= entries[pidx].mSize;
			entries[pidx].mSize = 0; // flag: skip older entry
		}
		entry_idx_map[id] = idx;
		total_size += entries[idx].mSize;
	}

	U32 validate_idx = 0;
	if (validate)
	{
		validate_idx = gSavedSettings.getU32("CacheValidateCounter");
		U32 next_idx = (++validate_idx) % 256;
		gSavedSettings.setU32("CacheValidateCounter", next_idx);
		LL_DEBUGS("TextureCache") << "TEXTURE CACHE: Validating: " << validate_idx << LL_ENDL;
	}
	
	S64 min_cache_size = (sCacheMaxTexturesSize * 9) / 10;
	S32 purge_count = 0;
	S32 next_idx = 0;
	for (S32 idx=0; idx<num_entries; idx++)
	{
		if (entries[idx].mSize == 0)
		{
			continue;
		}
		bool purge_entry = false;
		std::string filename = getTextureFileName(entries[idx].mID);
		if (total_size >= min_cache_size)
		{
			purge_entry = true;
		}
		else if (validate)
		{
			// make sure file exists and is the correct size
			U32 uuididx = entries[idx].mID.mData[0];
			if (uuididx == validate_idx)
			{
 				LL_DEBUGS("TextureCache") << "Validating: " << filename << "Size: " << entries[idx].mSize << LL_ENDL;
				S32 bodysize = LLAPRFile::size(filename, getLocalAPRFilePool());
				if (bodysize != entries[idx].mSize)
				{
					LL_WARNS("TextureCache") << "TEXTURE CACHE BODY HAS BAD SIZE: " << bodysize << " != " << entries[idx].mSize
							<< filename << LL_ENDL;
					purge_entry = true;
				}
			}
		}
		if (purge_entry)
		{
			purge_count++;
	 		LL_DEBUGS("TextureCache") << "PURGING: " << filename << LL_ENDL;
			LLAPRFile::remove(filename, getLocalAPRFilePool());
			total_size -= entries[idx].mSize;
			entries[idx].mSize = 0;
		}
		else
		{
			if (next_idx != idx)
			{
				entries[next_idx] = entries[idx];
			}
			++next_idx;
		}
	}
	num_entries = next_idx;

	LL_DEBUGS("TextureCache") << "TEXTURE CACHE: Writing Entries: " << num_entries << LL_ENDL;
	
	LLAPRFile::remove(mTexturesDirEntriesFileName, getLocalAPRFilePool());
	LLAPRFile::writeEx(mTexturesDirEntriesFileName, 
						 (U8*)&entries[0], 0, num_entries*sizeof(Entry),
						 getLocalAPRFilePool());
	
	mTexturesSizeTotal = 0;
	mTexturesSizeMap.clear();
	for (S32 idx=0; idx<num_entries; idx++)
	{
		mTexturesSizeMap[entries[idx].mID] = entries[idx].mSize;
		mTexturesSizeTotal += entries[idx].mSize;
	}
	llassert(mTexturesSizeTotal == total_size);
	
	delete[] entries;

	// *FIX:Mani - watchdog back on.
	LLAppViewer::instance()->resumeMainloopTimeout();
	
	LL_INFOS("TextureCache") << "TEXTURE CACHE:"
			<< " PURGED: " << purge_count
			<< " ENTRIES: " << num_entries
			<< " CACHE SIZE: " << total_size / 1024*1024 << " MB"
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
S32 LLTextureCache::getHeaderCacheEntry(const LLUUID& id, bool touch, S32* imagesize)
{
	bool retry = false;
	S32 idx = -1;

	{
		LLMutexLock lock(&mHeaderMutex);
		id_map_t::iterator iter = mHeaderIDMap.find(id);
		if (iter != mHeaderIDMap.end())
		{
			idx = iter->second;
		}
		else if (touch && !mReadOnly)
		{
			if (mHeaderEntriesInfo.mEntries < sCacheMaxEntries)
			{
				// Add an entry
				idx = mHeaderEntriesInfo.mEntries++;
				mHeaderIDMap[id] = idx;
				// Update Info
				LLAPRFile::writeEx(mHeaderEntriesFileName, 
									(U8*)&mHeaderEntriesInfo, 0, sizeof(EntriesInfo),
									getLocalAPRFilePool());
			}
			else if (!mLRU.empty())
			{
				idx = mLRU.begin()->first; // will be erased below
				const LLUUID& oldid = mLRU.begin()->second;
				mHeaderIDMap.erase(oldid);
				mTexturesSizeMap.erase(oldid);
				mHeaderIDMap[id] = idx;
			}
			else
			{
				idx = -1;
				retry = true;
			}
		}
		if (idx >= 0)
		{
			if (touch && !mReadOnly)
			{
				// Update the lru entry
				mLRU.erase(idx);
				llassert_always(imagesize && *imagesize > 0);
				Entry* entry = new Entry(id, *imagesize, time(NULL));
				S32 offset = sizeof(EntriesInfo) + idx * sizeof(Entry);
				LLAPRFile::writeEx(mHeaderEntriesFileName, 
									 (U8*)entry, offset, sizeof(Entry),
									 getLocalAPRFilePool());
				delete entry;
			}
			else if (imagesize)
			{
				// Get the image size
				Entry entry;
				S32 offset = sizeof(EntriesInfo) + idx * sizeof(Entry);

				LLAPRFile::readEx(mHeaderEntriesFileName, 
									(U8*)&entry, offset, sizeof(Entry),
									getLocalAPRFilePool());
				*imagesize = entry.mSize;
			}
		}
	}
	if (retry)
	{
		readHeaderCache(); // updates the lru
		llassert_always(!mLRU.empty() || mHeaderEntriesInfo.mEntries < sCacheMaxEntries);
		idx = getHeaderCacheEntry(id, touch, imagesize); // assert above ensures no inf. recursion
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
															NULL, size, offset, 0,
															responder);
	handle_t handle = worker->read();
	mReaders[handle] = worker;
	return handle;
}


bool LLTextureCache::readComplete(handle_t handle, bool abort)
{
	lockWorkers();
	handle_map_t::iterator iter = mReaders.find(handle);
	llassert_always(iter != mReaders.end());
	LLTextureCacheWorker* worker = iter->second;
	bool res = worker->complete();
	if (res || abort)
	{
		mReaders.erase(handle);
		unlockWorkers();
		worker->scheduleDelete();
		return true;
	}
	else
	{
		unlockWorkers();
		return false;
	}
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
	if (datasize >= TEXTURE_CACHE_ENTRY_SIZE)
	{
		LLMutexLock lock(&mWorkersMutex);
		llassert_always(imagesize > 0);
		LLTextureCacheWorker* worker = new LLTextureCacheRemoteWorker(this, priority, id,
																data, datasize, 0,
																imagesize, responder);
		handle_t handle = worker->write();
		mWriters[handle] = worker;
		return handle;
	}
	delete responder;
	return LLWorkerThread::nullHandle();
}

bool LLTextureCache::writeComplete(handle_t handle, bool abort)
{
	lockWorkers();
	handle_map_t::iterator iter = mWriters.find(handle);
	llassert_always(iter != mWriters.end());
	LLTextureCacheWorker* worker = iter->second;
	if (worker->complete() || abort)
	{
		mWriters.erase(handle);
		unlockWorkers();
		worker->scheduleDelete();
		return true;
	}
	else
	{
		unlockWorkers();
		return false;
	}
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

bool LLTextureCache::removeHeaderCacheEntry(const LLUUID& id)
{
	if (mReadOnly)
	{
		return false;
	}
	LLMutexLock lock(&mHeaderMutex);
	id_map_t::iterator iter = mHeaderIDMap.find(id);
	if (iter != mHeaderIDMap.end())
	{
		S32 idx = iter->second;
		if (idx >= 0)
		{
			Entry* entry = new Entry(id, -1, time(NULL));
			S32 offset = sizeof(EntriesInfo) + idx * sizeof(Entry);

			LLAPRFile::writeEx(mHeaderEntriesFileName,
								 (U8*)entry, offset, sizeof(Entry),
								 getLocalAPRFilePool());			
			delete entry;
			mLRU[idx] = id;
			mHeaderIDMap.erase(id);
			mTexturesSizeMap.erase(id);
			return true;
		}
	}
	return false;
}

void LLTextureCache::removeFromCache(const LLUUID& id)
{
	//llwarns << "Removing texture from cache: " << id << llendl;
	if (!mReadOnly)
	{
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
