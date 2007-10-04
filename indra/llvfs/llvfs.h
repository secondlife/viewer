/** 
 * @file llvfs.h
 * @brief Definition of virtual file system
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVFS_H
#define LL_LLVFS_H

#include <deque>
#include "lluuid.h"
#include "linked_lists.h"
#include "llassettype.h"
#include "llthread.h"

enum EVFSValid 
{
	VFSVALID_UNKNOWN = 0, 
	VFSVALID_OK = 1,
	VFSVALID_BAD_CORRUPT = 2,
	VFSVALID_BAD_CANNOT_OPEN_READONLY = 3,
	VFSVALID_BAD_CANNOT_CREATE = 4
};

// Lock types for open vfiles, pending async reads, and pending async appends
// (There are no async normal writes, currently)
enum EVFSLock
{
	VFSLOCK_OPEN = 0,
	VFSLOCK_READ = 1,
	VFSLOCK_APPEND = 2,

	VFSLOCK_COUNT = 3
};

// internal classes
class LLVFSBlock;
class LLVFSFileBlock;
class LLVFSFileSpecifier
{
public:
	LLVFSFileSpecifier();
	LLVFSFileSpecifier(const LLUUID &file_id, const LLAssetType::EType file_type);
	bool operator<(const LLVFSFileSpecifier &rhs) const;
	bool operator==(const LLVFSFileSpecifier &rhs) const;

public:
	LLUUID mFileID;
	LLAssetType::EType mFileType;
};

class LLVFS
{
public:
	// Pass 0 to not presize
	LLVFS(const char *index_filename, const char *data_filename, const BOOL read_only, const U32 presize, const BOOL remove_after_crash);
	~LLVFS();

	BOOL isValid() const			{ return (VFSVALID_OK == mValid); }
	EVFSValid getValidState() const	{ return mValid; }

	// ---------- The following fucntions lock/unlock mDataMutex ----------
	BOOL getExists(const LLUUID &file_id, const LLAssetType::EType file_type);
	S32	 getSize(const LLUUID &file_id, const LLAssetType::EType file_type);

	BOOL checkAvailable(S32 max_size);
	
	S32  getMaxSize(const LLUUID &file_id, const LLAssetType::EType file_type);
	BOOL setMaxSize(const LLUUID &file_id, const LLAssetType::EType file_type, S32 max_size);

	void renameFile(const LLUUID &file_id, const LLAssetType::EType file_type,
		const LLUUID &new_id, const LLAssetType::EType &new_type);
	void removeFile(const LLUUID &file_id, const LLAssetType::EType file_type);

	S32 getData(const LLUUID &file_id, const LLAssetType::EType file_type, U8 *buffer, S32 location, S32 length);
	S32 storeData(const LLUUID &file_id, const LLAssetType::EType file_type, const U8 *buffer, S32 location, S32 length);

	void incLock(const LLUUID &file_id, const LLAssetType::EType file_type, EVFSLock lock);
	void decLock(const LLUUID &file_id, const LLAssetType::EType file_type, EVFSLock lock);
	BOOL isLocked(const LLUUID &file_id, const LLAssetType::EType file_type, EVFSLock lock);
	// ----------------------------------------------------------------

	// Used to trigger evil WinXP behavior of "preloading" entire file into memory.
	void pokeFiles();

	// Verify that the index file contents match the in-memory file structure
	// Very slow, do not call routinely. JC
	void audit();
	// Check for uninitialized blocks.  Slow, do not call in release. JC
	void checkMem();
	// for debugging, prints a map of the vfs
	void dumpMap();
	void dumpLockCounts();
	void dumpStatistics();
	void listFiles();
	void dumpFiles();

protected:
	void removeFileBlock(LLVFSFileBlock *fileblock);
	
	void eraseBlockLength(LLVFSBlock *block);
	void eraseBlock(LLVFSBlock *block);
	void addFreeBlock(LLVFSBlock *block);
	//void mergeFreeBlocks();
	void useFreeSpace(LLVFSBlock *free_block, S32 length);
	void sync(LLVFSFileBlock *block, BOOL remove = FALSE);
	void presizeDataFile(const U32 size);

	static FILE *openAndLock(const char *filename, const char *mode, BOOL read_lock);
	static void unlockAndClose(FILE *fp);
	
	// Can initiate LRU-based file removal to make space.
	// The immune file block will not be removed.
	LLVFSBlock *findFreeBlock(S32 size, LLVFSFileBlock *immune = NULL);

	// lock/unlock data mutex (mDataMutex)
	void lockData() { mDataMutex->lock(); }
	void unlockData() { mDataMutex->unlock(); }	
	
protected:
	LLMutex* mDataMutex;
	
	typedef std::map<LLVFSFileSpecifier, LLVFSFileBlock*> fileblock_map;
	fileblock_map mFileBlocks;

	typedef std::multimap<S32, LLVFSBlock*>	blocks_length_map_t;
	blocks_length_map_t 	mFreeBlocksByLength;
	typedef std::multimap<U32, LLVFSBlock*>	blocks_location_map_t;
	blocks_location_map_t 	mFreeBlocksByLocation;

	FILE *mDataFP;
	FILE *mIndexFP;

	std::deque<S32> mIndexHoles;

	char *mIndexFilename;
	char *mDataFilename;
	BOOL mReadOnly;

	EVFSValid mValid;

	S32 mLockCounts[VFSLOCK_COUNT];
	BOOL mRemoveAfterCrash;
};

extern LLVFS *gVFS;

#endif
