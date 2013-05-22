/** 
 * @file lltexturecache.h
 * @brief Object for managing texture cachees.
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

#ifndef LL_LLTEXTURECACHE_
#define LL_LLTEXTURECACHE_H

#include "lldir.h"
#include "llstl.h"
#include "llstring.h"
#include "lluuid.h"

#include "llworkerthread.h"

class LLImageFormatted;
class LLTextureCacheWorker;
class LLImageRaw;

class LLTextureCache : public LLWorkerThread
{
	friend class LLTextureCacheWorker;
	friend class LLTextureCacheRemoteWorker;
	friend class LLTextureCacheLocalFileWorker;

private:
	// Entries
	struct EntriesInfo
	{
		EntriesInfo() : mVersion(0.f), mEntries(0) {}
		F32 mVersion;
		U32 mEntries;
	};
	struct Entry
	{
        	Entry() :
		        mBodySize(0),
			mImageSize(0),
			mTime(0)
		{
		}
		Entry(const LLUUID& id, S32 imagesize, S32 bodysize, U32 time) :
			mID(id), mImageSize(imagesize), mBodySize(bodysize), mTime(time) {}
		void init(const LLUUID& id, U32 time) { mID = id, mImageSize = 0; mBodySize = 0; mTime = time; }
		Entry& operator=(const Entry& entry) {mID = entry.mID, mImageSize = entry.mImageSize; mBodySize = entry.mBodySize; mTime = entry.mTime; return *this;}
		LLUUID mID; // 16 bytes
		S32 mImageSize; // total size of image if known
		S32 mBodySize; // size of body file in body cache
		U32 mTime; // seconds since 1/1/1970
	};

	
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

	/*virtual*/ S32 update(F32 max_time_ms);	
	
	void purgeCache(ELLPath location);
	void setReadOnly(BOOL read_only) ;
	S64 initCache(ELLPath location, S64 maxsize, BOOL texture_cache_mismatch);

	handle_t readFromCache(const std::string& local_filename, const LLUUID& id, U32 priority, S32 offset, S32 size,
						   ReadResponder* responder);

	handle_t readFromCache(const LLUUID& id, U32 priority, S32 offset, S32 size,
						   ReadResponder* responder);
	bool readComplete(handle_t handle, bool abort);
	handle_t writeToCache(const LLUUID& id, U32 priority, U8* data, S32 datasize, S32 imagesize, LLPointer<LLImageRaw> rawimage, S32 discardlevel,
						  WriteResponder* responder);
	LLPointer<LLImageRaw> readFromFastCache(const LLUUID& id, S32& discardlevel);
	bool writeComplete(handle_t handle, bool abort = false);
	void prioritizeWrite(handle_t handle);

	bool removeFromCache(const LLUUID& id);

	// For LLTextureCacheWorker::Responder
	LLTextureCacheWorker* getReader(handle_t handle);
	LLTextureCacheWorker* getWriter(handle_t handle);
	void lockWorkers() { mWorkersMutex.lock(); }
	void unlockWorkers() { mWorkersMutex.unlock(); }

	// debug
	S32 getNumReads() { return mReaders.size(); }
	S32 getNumWrites() { return mWriters.size(); }
	S64 getUsage() { return mTexturesSizeTotal; }
	S64 getMaxUsage() { return sCacheMaxTexturesSize; }
	U32 getEntries() { return mHeaderEntriesInfo.mEntries; }
	U32 getMaxEntries() { return sCacheMaxEntries; };
	BOOL isInCache(const LLUUID& id) ;
	BOOL isInLocal(const LLUUID& id) ;

protected:
	// Accessed by LLTextureCacheWorker
	std::string getLocalFileName(const LLUUID& id);
	std::string getTextureFileName(const LLUUID& id);
	void addCompleted(Responder* responder, bool success);
	
protected:
	//void setFileAPRPool(apr_pool_t* pool) { mFileAPRPool = pool ; }

private:
	void setDirNames(ELLPath location);
	void readHeaderCache();
	void clearCorruptedCache();
	void purgeAllTextures(bool purge_directories);
	void purgeTextures(bool validate);
	LLAPRFile* openHeaderEntriesFile(bool readonly, S32 offset);
	void closeHeaderEntriesFile();
	void readEntriesHeader();
	void writeEntriesHeader();
	S32 openAndReadEntry(const LLUUID& id, Entry& entry, bool create);
	bool updateEntry(S32& idx, Entry& entry, S32 new_image_size, S32 new_body_size);
	void updateEntryTimeStamp(S32 idx, Entry& entry) ;
	U32 openAndReadEntries(std::vector<Entry>& entries);
	void writeEntriesAndClose(const std::vector<Entry>& entries);
	void readEntryFromHeaderImmediately(S32& idx, Entry& entry) ;
	void writeEntryToHeaderImmediately(S32& idx, Entry& entry, bool write_header = false) ;
	void removeEntry(S32 idx, Entry& entry, std::string& filename);
	void removeCachedTexture(const LLUUID& id) ;
	S32 getHeaderCacheEntry(const LLUUID& id, Entry& entry);
	S32 setHeaderCacheEntry(const LLUUID& id, Entry& entry, S32 imagesize, S32 datasize);
	void writeUpdatedEntries() ;
	void updatedHeaderEntriesFile() ;
	void lockHeaders() { mHeaderMutex.lock(); }
	void unlockHeaders() { mHeaderMutex.unlock(); }
	
	void openFastCache(bool first_time = false);
	void closeFastCache(bool forced = false);
	bool writeToFastCache(S32 id, LLPointer<LLImageRaw> raw, S32 discardlevel);	

private:
	// Internal
	LLMutex mWorkersMutex;
	LLMutex mHeaderMutex;
	LLMutex mListMutex;
	LLMutex mFastCacheMutex;
	LLAPRFile* mHeaderAPRFile;
	LLVolatileAPRPool* mFastCachePoolp;
	
	typedef std::map<handle_t, LLTextureCacheWorker*> handle_map_t;
	handle_map_t mReaders;
	handle_map_t mWriters;

	typedef std::vector<handle_t> handle_list_t;
	handle_list_t mPrioritizeWriteList;

	typedef std::vector<std::pair<LLPointer<Responder>, bool> > responder_list_t;
	responder_list_t mCompletedList;
	
	BOOL mReadOnly;
	
	// HEADERS (Include first mip)
	std::string mHeaderEntriesFileName;
	std::string mHeaderDataFileName;
	std::string mFastCacheFileName;
	EntriesInfo mHeaderEntriesInfo;
	std::set<S32> mFreeList; // deleted entries
	std::set<LLUUID> mLRU;
	typedef std::map<LLUUID, S32> id_map_t;
	id_map_t mHeaderIDMap;

	LLAPRFile*   mFastCachep;
	LLFrameTimer mFastCacheTimer;
	U8*          mFastCachePadBuffer;

	// BODIES (TEXTURES minus headers)
	std::string mTexturesDirName;
	typedef std::map<LLUUID,S32> size_map_t;
	size_map_t mTexturesSizeMap;
	S64 mTexturesSizeTotal;
	LLAtomic32<BOOL> mDoPurge;

	typedef std::map<S32, Entry> idx_entry_map_t;
	idx_entry_map_t mUpdatedEntryMap;

	// Statics
	static F32 sHeaderCacheVersion;
	static U32 sCacheMaxEntries;
	static S64 sCacheMaxTexturesSize;
};

extern const S32 TEXTURE_CACHE_ENTRY_SIZE;

#endif // LL_LLTEXTURECACHE_H
