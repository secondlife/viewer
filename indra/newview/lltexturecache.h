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

class LLImageFormatted;
class LLTextureCacheWorker;

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

	/*virtual*/ S32 update(U32 max_time_ms);	
	
	void purgeCache(ELLPath location);
	void setReadOnly(BOOL read_only) ;
	S64 initCache(ELLPath location, S64 maxsize, BOOL disable_texture_cache);

	handle_t readFromCache(const std::string& local_filename, const LLUUID& id, U32 priority, S32 offset, S32 size,
						   ReadResponder* responder);

	handle_t readFromCache(const LLUUID& id, U32 priority, S32 offset, S32 size,
						   ReadResponder* responder);
	bool readComplete(handle_t handle, bool abort);
	handle_t writeToCache(const LLUUID& id, U32 priority, U8* data, S32 datasize, S32 imagesize,
						  WriteResponder* responder);
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
	void purgeAllTextures(bool purge_directories);
	void purgeTextures(bool validate);
	LLAPRFile* openHeaderEntriesFile(bool readonly, S32 offset);
	void closeHeaderEntriesFile();
	void readEntriesHeader();
	void writeEntriesHeader();
	S32 openAndReadEntry(const LLUUID& id, Entry& entry, bool create);
	bool updateEntry(S32 idx, Entry& entry, S32 new_image_size, S32 new_body_size);
	void updateEntryTimeStamp(S32 idx, Entry& entry) ;
	U32 openAndReadEntries(std::vector<Entry>& entries);
	void writeEntriesAndClose(const std::vector<Entry>& entries);
	void readEntryFromHeaderImmediately(S32 idx, Entry& entry) ;
	void writeEntryToHeaderImmediately(S32 idx, Entry& entry, bool write_header = false) ;
	void removeEntry(S32 idx, Entry& entry, std::string& filename);
	void removeCachedTexture(const LLUUID& id) ;
	S32 getHeaderCacheEntry(const LLUUID& id, Entry& entry);
	S32 setHeaderCacheEntry(const LLUUID& id, Entry& entry, S32 imagesize, S32 datasize);
	void writeUpdatedEntries() ;
	void updatedHeaderEntriesFile() ;
	void lockHeaders() { mHeaderMutex.lock(); }
	void unlockHeaders() { mHeaderMutex.unlock(); }
	
private:
	// Internal
	LLMutex mWorkersMutex;
	LLMutex mHeaderMutex;
	LLMutex mListMutex;
	LLAPRFile* mHeaderAPRFile;
	
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
	EntriesInfo mHeaderEntriesInfo;
	std::set<S32> mFreeList; // deleted entries
	std::set<LLUUID> mLRU;
	typedef std::map<LLUUID,S32> id_map_t;
	id_map_t mHeaderIDMap;

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
