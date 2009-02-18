/** 
 * @file lltexturecache.h
 * @brief Object for managing texture cachees.
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

#ifndef LL_LLTEXTURECACHE_
#define LL_LLTEXTURECACHE_H

#include "lldir.h"
#include "llstl.h"
#include "llstring.h"
#include "lluuid.h"

#include "llworkerthread.h"

class LLTextureCacheWorker;

class LLTextureCache : public LLWorkerThread
{
	friend class LLTextureCacheWorker;
	friend class LLTextureCacheRemoteWorker;
	friend class LLTextureCacheLocalFileWorker;

public:

	class Responder : public LLResponder
	{
	public:
		virtual void setData(U8* data, S32 datasize, S32 imagesize, S32 imageformat, BOOL imagelocal) = 0;
	};
	
	class ReadResponder : public Responder
	{
	public:
		ReadResponder();
		void setData(U8* data, S32 datasize, S32 imagesize, S32 imageformat, BOOL imagelocal);
		void setImage(LLImageFormatted* image) { mFormattedImage = image; }
	protected:
		LLPointer<LLImageFormatted> mFormattedImage;
		S32 mImageSize;
		BOOL mImageLocal;
	};

	class WriteResponder : public Responder
	{
		void setData(U8* data, S32 datasize, S32 imagesize, S32 imageformat, BOOL imagelocal)
		{
			// not used
		}
	};
	
	LLTextureCache(bool threaded);
	~LLTextureCache();

	/*virtual*/ S32 update(U32 max_time_ms);	
	
	void purgeCache(ELLPath location);
	S64 initCache(ELLPath location, S64 maxsize, BOOL read_only);

	handle_t readFromCache(const std::string& local_filename, const LLUUID& id, U32 priority, S32 offset, S32 size,
						   ReadResponder* responder);

	handle_t readFromCache(const LLUUID& id, U32 priority, S32 offset, S32 size,
						   ReadResponder* responder);
	bool readComplete(handle_t handle, bool abort);
	handle_t writeToCache(const LLUUID& id, U32 priority, U8* data, S32 datasize, S32 imagesize,
						  WriteResponder* responder);
	bool writeComplete(handle_t handle, bool abort = false);
	void prioritizeWrite(handle_t handle);

	void removeFromCache(const LLUUID& id);

	// For LLTextureCacheWorker::Responder
	LLTextureCacheWorker* getReader(handle_t handle);
	LLTextureCacheWorker* getWriter(handle_t handle);
	void lockWorkers() { mWorkersMutex.lock(); }
	void unlockWorkers() { mWorkersMutex.unlock(); }

	// debug
	S32 getNumReads() { return mReaders.size(); }
	S32 getNumWrites() { return mWriters.size(); }

protected:
	// Accessed by LLTextureCacheWorker
	bool appendToTextureEntryList(const LLUUID& id, S32 size);
	std::string getLocalFileName(const LLUUID& id);
	std::string getTextureFileName(const LLUUID& id);
	void addCompleted(Responder* responder, bool success);
	
protected:
	//void setFileAPRPool(apr_pool_t* pool) { mFileAPRPool = pool ; }

private:
	void setDirNames(ELLPath location);
	void readHeaderCache();
	void purgeAllTextures(bool purge_directories);
	void purgeTextures(bool validate);
	S32 getHeaderCacheEntry(const LLUUID& id, bool touch, S32* imagesize = NULL);
	bool removeHeaderCacheEntry(const LLUUID& id);
	void lockHeaders() { mHeaderMutex.lock(); }
	void unlockHeaders() { mHeaderMutex.unlock(); }
	
private:
	// Internal
	LLMutex mWorkersMutex;
	LLMutex mHeaderMutex;
	LLMutex mListMutex;
	
	typedef std::map<handle_t, LLTextureCacheWorker*> handle_map_t;
	handle_map_t mReaders;
	handle_map_t mWriters;

	typedef std::vector<handle_t> handle_list_t;
	handle_list_t mPrioritizeWriteList;

	typedef std::vector<std::pair<LLPointer<Responder>, bool> > responder_list_t;
	responder_list_t mCompletedList;
	
	BOOL mReadOnly;
	
	// Entries
	struct EntriesInfo
	{
		F32 mVersion;
		U32 mEntries;
	};
	struct Entry
	{
		Entry() {}
		Entry(const LLUUID& id, S32 size, U32 time) : mID(id), mSize(size), mTime(time) {}
		LLUUID mID; // 128 bits
		S32 mSize; // total size of image if known (NOT size cached)
		U32 mTime; // seconds since 1/1/1970
	};

	// HEADERS (Include first mip)
	std::string mHeaderEntriesFileName;
	std::string mHeaderDataFileName;
	EntriesInfo mHeaderEntriesInfo;
	typedef std::map<S32,LLUUID> index_map_t;
	index_map_t mLRU; // index, id; stored as a map for fast removal
	typedef std::map<LLUUID,S32> id_map_t;
	id_map_t mHeaderIDMap;

	// BODIES (TEXTURES minus headers)
	std::string mTexturesDirName;
	std::string mTexturesDirEntriesFileName;
	typedef std::map<LLUUID,S32> size_map_t;
	size_map_t mTexturesSizeMap;
	S64 mTexturesSizeTotal;
	LLAtomic32<BOOL> mDoPurge;
	
	// Statics
	static F32 sHeaderCacheVersion;
	static U32 sCacheMaxEntries;
	static S64 sCacheMaxTexturesSize;
};

#endif // LL_LLTEXTURECACHE_H
