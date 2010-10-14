/** 
 * @file llvfs.cpp
 * @brief Implementation of virtual file system
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

#include <sys/stat.h>
#include <set>
#include <map>
#if LL_WINDOWS
#include <share.h>
#elif LL_SOLARIS
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <sys/file.h>
#endif
    
#include "llvfs.h"

#include "llstl.h"
#include "lltimer.h"
    
const S32 FILE_BLOCK_MASK = 0x000003FF;	 // 1024-byte blocks
const S32 VFS_CLEANUP_SIZE = 5242880;  // how much space we free up in a single stroke
const S32 BLOCK_LENGTH_INVALID = -1;	// mLength for invalid LLVFSFileBlocks

LLVFS *gVFS = NULL;

// internal class definitions
class LLVFSBlock
{
public:
	LLVFSBlock() 
	{
		mLocation = 0;
		mLength = 0;
	}
    
	LLVFSBlock(U32 loc, S32 size)
	{
		mLocation = loc;
		mLength = size;
	}
    
	static bool locationSortPredicate(
		const LLVFSBlock* lhs,
		const LLVFSBlock* rhs)
	{
		return lhs->mLocation < rhs->mLocation;
	}

public:
	U32 mLocation;
	S32	mLength;		// allocated block size
};
    
LLVFSFileSpecifier::LLVFSFileSpecifier()
:	mFileID(),
	mFileType( LLAssetType::AT_NONE )
{
}
    
LLVFSFileSpecifier::LLVFSFileSpecifier(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	mFileID = file_id;
	mFileType = file_type;
}
    
bool LLVFSFileSpecifier::operator<(const LLVFSFileSpecifier &rhs) const
{
	return (mFileID == rhs.mFileID)
		? mFileType < rhs.mFileType
		: mFileID < rhs.mFileID;
}
    
bool LLVFSFileSpecifier::operator==(const LLVFSFileSpecifier &rhs) const
{
	return (mFileID == rhs.mFileID && 
			mFileType == rhs.mFileType);
}
    
    
class LLVFSFileBlock : public LLVFSBlock, public LLVFSFileSpecifier
{
public:
	LLVFSFileBlock() : LLVFSBlock(), LLVFSFileSpecifier()
	{
		init();
	}
    
	LLVFSFileBlock(const LLUUID &file_id, LLAssetType::EType file_type, U32 loc = 0, S32 size = 0)
		: LLVFSBlock(loc, size), LLVFSFileSpecifier( file_id, file_type )
	{
		init();
	}

	void init()
	{
		mSize = 0;
		mIndexLocation = -1;
		mAccessTime = (U32)time(NULL);

		for (S32 i = 0; i < (S32)VFSLOCK_COUNT; i++)
		{
			mLocks[(EVFSLock)i] = 0;
		}
	}

	#ifdef LL_LITTLE_ENDIAN
	inline void swizzleCopy(void *dst, void *src, int size) { memcpy(dst, src, size); /* Flawfinder: ignore */}

	#else
	
	inline U32 swizzle32(U32 x)
	{
		return(((x >> 24) & 0x000000FF) | ((x >> 8)  & 0x0000FF00) | ((x << 8)  & 0x00FF0000) |((x << 24) & 0xFF000000));
	}
	
	inline U16 swizzle16(U16 x)
	{
		return(	((x >> 8)  & 0x000000FF) | ((x << 8)  & 0x0000FF00) );
	}
	
	inline void swizzleCopy(void *dst, void *src, int size) 
	{
		if(size == 4)
		{
			((U32*)dst)[0] = swizzle32(((U32*)src)[0]); 
		}
		else if(size == 2)
		{
			((U16*)dst)[0] = swizzle16(((U16*)src)[0]); 
		}
		else
		{
			// Perhaps this should assert...
			memcpy(dst, src, size);	/* Flawfinder: ignore */
		}
	}
	
	#endif

	void serialize(U8 *buffer)
	{
		swizzleCopy(buffer, &mLocation, 4);
		buffer += 4;
		swizzleCopy(buffer, &mLength, 4);
		buffer +=4;
		swizzleCopy(buffer, &mAccessTime, 4);
		buffer +=4;
		memcpy(buffer, &mFileID.mData, 16); /* Flawfinder: ignore */	
		buffer += 16;
		S16 temp_type = mFileType;
		swizzleCopy(buffer, &temp_type, 2);
		buffer += 2;
		swizzleCopy(buffer, &mSize, 4);
	}
    
	void deserialize(U8 *buffer, const S32 index_loc)
	{
		mIndexLocation = index_loc;
    
		swizzleCopy(&mLocation, buffer, 4);
		buffer += 4;
		swizzleCopy(&mLength, buffer, 4);
		buffer += 4;
		swizzleCopy(&mAccessTime, buffer, 4);
		buffer += 4;
		memcpy(&mFileID.mData, buffer, 16);
		buffer += 16;
		S16 temp_type;
		swizzleCopy(&temp_type, buffer, 2);
		mFileType = (LLAssetType::EType)temp_type;
		buffer += 2;
		swizzleCopy(&mSize, buffer, 4);
	}
    
	static BOOL insertLRU(LLVFSFileBlock* const& first,
						  LLVFSFileBlock* const& second)
	{
		return (first->mAccessTime == second->mAccessTime)
			? *first < *second
			: first->mAccessTime < second->mAccessTime;
	}
    
public:
	S32  mSize;
	S32  mIndexLocation; // location of index entry
	U32  mAccessTime;
	BOOL mLocks[VFSLOCK_COUNT]; // number of outstanding locks of each type
    
	static const S32 SERIAL_SIZE;
};

// Helper structure for doing lru w/ stl... is there a simpler way?
struct LLVFSFileBlock_less
{
	bool operator()(LLVFSFileBlock* const& lhs, LLVFSFileBlock* const& rhs) const
	{
		return (LLVFSFileBlock::insertLRU(lhs, rhs)) ? true : false;
	}
};


const S32 LLVFSFileBlock::SERIAL_SIZE = 34;
     

LLVFS::LLVFS(const std::string& index_filename, const std::string& data_filename, const BOOL read_only, const U32 presize, const BOOL remove_after_crash)
:	mRemoveAfterCrash(remove_after_crash),
	mDataFP(NULL),
	mIndexFP(NULL)
{
	mDataMutex = new LLMutex(0);

	S32 i;
	for (i = 0; i < VFSLOCK_COUNT; i++)
	{
		mLockCounts[i] = 0;
	}
	mValid = VFSVALID_OK;
	mReadOnly = read_only;
	mIndexFilename = index_filename;
	mDataFilename = data_filename;
    
	const char *file_mode = mReadOnly ? "rb" : "r+b";
    
	LL_INFOS("VFS") << "Attempting to open VFS index file " << mIndexFilename << LL_ENDL;
	LL_INFOS("VFS") << "Attempting to open VFS data file " << mDataFilename << LL_ENDL;

	mDataFP = openAndLock(mDataFilename, file_mode, mReadOnly);
	if (!mDataFP)
	{
		if (mReadOnly)
		{
			LL_WARNS("VFS") << "Can't find " << mDataFilename << " to open read-only VFS" << LL_ENDL;
			mValid = VFSVALID_BAD_CANNOT_OPEN_READONLY;
			return;
		}

		mDataFP = openAndLock(mDataFilename, "w+b", FALSE);
		if (mDataFP)
		{
			// Since we're creating this data file, assume any index file is bogus
			// remove the index, since this vfs is now blank
			LLFile::remove(mIndexFilename);
		}
		else
		{
			LL_WARNS("VFS") << "Couldn't open vfs data file " 
				<< mDataFilename << LL_ENDL;
			mValid = VFSVALID_BAD_CANNOT_CREATE;
			return;
		}

		if (presize)
		{
			presizeDataFile(presize);
		}
	}

	// Did we leave this file open for writing last time?
	// If so, close it and start over.
	if (!mReadOnly && mRemoveAfterCrash)
	{
		llstat marker_info;
		std::string marker = mDataFilename + ".open";
		if (!LLFile::stat(marker, &marker_info))
		{
			// marker exists, kill the lock and the VFS files
			unlockAndClose(mDataFP);
			mDataFP = NULL;

			LL_WARNS("VFS") << "VFS: File left open on last run, removing old VFS file " << mDataFilename << LL_ENDL;
			LLFile::remove(mIndexFilename);
			LLFile::remove(mDataFilename);
			LLFile::remove(marker);

			mDataFP = openAndLock(mDataFilename, "w+b", FALSE);
			if (!mDataFP)
			{
				LL_WARNS("VFS") << "Can't open VFS data file in crash recovery" << LL_ENDL;
				mValid = VFSVALID_BAD_CANNOT_CREATE;
				return;
			}

			if (presize)
			{
				presizeDataFile(presize);
			}
		}
	}

	// determine the real file size
	fseek(mDataFP, 0, SEEK_END);
	U32 data_size = ftell(mDataFP);

	// read the index file
	// make sure there's at least one file in it too
	// if not, we'll treat this as a new vfs
	llstat fbuf;
	if (! LLFile::stat(mIndexFilename, &fbuf) &&
		fbuf.st_size >= LLVFSFileBlock::SERIAL_SIZE &&
		(mIndexFP = openAndLock(mIndexFilename, file_mode, mReadOnly))	// Yes, this is an assignment and not '=='
		)
	{	
		std::vector<U8> buffer(fbuf.st_size);
    		size_t buf_offset = 0;
		size_t nread = fread(&buffer[0], 1, fbuf.st_size, mIndexFP);
 
		std::vector<LLVFSFileBlock*> files_by_loc;
		
		while (buf_offset < nread)
		{
			LLVFSFileBlock *block = new LLVFSFileBlock();
    
			block->deserialize(&buffer[buf_offset], (S32)buf_offset);
    
			// Do sanity check on this block.
			// Note that this skips zero size blocks, which helps VFS
			// to heal after some errors. JC
			if (block->mLength > 0 &&
				(U32)block->mLength <= data_size &&
				block->mLocation < data_size &&
				block->mSize > 0 &&
				block->mSize <= block->mLength &&
				block->mFileType >= LLAssetType::AT_NONE &&
				block->mFileType < LLAssetType::AT_COUNT)
			{
				mFileBlocks.insert(fileblock_map::value_type(*block, block));
				files_by_loc.push_back(block);
			}
			else
			if (block->mLength && block->mSize > 0)
			{
				// this is corrupt, not empty
				LL_WARNS("VFS") << "VFS corruption: " << block->mFileID << " (" << block->mFileType << ") at index " << block->mIndexLocation << " DS: " << data_size << LL_ENDL;
				LL_WARNS("VFS") << "Length: " << block->mLength << "\tLocation: " << block->mLocation << "\tSize: " << block->mSize << LL_ENDL;
				LL_WARNS("VFS") << "File has bad data - VFS removed" << LL_ENDL;

				delete block;

				unlockAndClose( mIndexFP );
				mIndexFP = NULL;
				LLFile::remove( mIndexFilename );

				unlockAndClose( mDataFP );
				mDataFP = NULL;
				LLFile::remove( mDataFilename );

				LL_WARNS("VFS") << "Deleted corrupt VFS files " 
					<< mDataFilename 
					<< " and "
					<< mIndexFilename
					<< LL_ENDL;

				mValid = VFSVALID_BAD_CORRUPT;
				return;
			}
			else
			{
				// this is a null or bad entry, skip it
				mIndexHoles.push_back(buf_offset);
    
				delete block;
			}
    
			buf_offset += LLVFSFileBlock::SERIAL_SIZE;
		}

		std::sort(
			files_by_loc.begin(),
			files_by_loc.end(),
			LLVFSFileBlock::locationSortPredicate);

		// There are 3 cases that have to be considered.
		// 1. No blocks
		// 2. One block.
		// 3. Two or more blocks.
		if (!files_by_loc.empty())
		{
			// cur walks through the list.
			std::vector<LLVFSFileBlock*>::iterator cur = files_by_loc.begin();
			std::vector<LLVFSFileBlock*>::iterator end = files_by_loc.end();
			LLVFSFileBlock* last_file_block = *cur;
			
			// Check to see if there is an empty space before the first file.
			if (last_file_block->mLocation > 0)
			{
				// If so, create a free block.
				addFreeBlock(new LLVFSBlock(0, last_file_block->mLocation));
			}

			// Walk through the 2nd+ block.  If there is a free space
			// between cur_file_block and last_file_block, add it to
			// the free space collection.  This block will not need to
			// run in the case there is only one entry in the VFS.
			++cur;
			while( cur != end )
			{
				LLVFSFileBlock* cur_file_block = *cur;

				// Dupe check on the block
				if (cur_file_block->mLocation == last_file_block->mLocation
					&& cur_file_block->mLength == last_file_block->mLength)
				{
					LL_WARNS("VFS") << "VFS: removing duplicate entry"
						<< " at " << cur_file_block->mLocation 
						<< " length " << cur_file_block->mLength 
						<< " size " << cur_file_block->mSize
						<< " ID " << cur_file_block->mFileID 
						<< " type " << cur_file_block->mFileType 
						<< LL_ENDL;

					// Duplicate entries.  Nuke them both for safety.
					mFileBlocks.erase(*cur_file_block);	// remove ID/type entry
					if (cur_file_block->mLength > 0)
					{
						// convert to hole
						addFreeBlock(
							new LLVFSBlock(
								cur_file_block->mLocation,
								cur_file_block->mLength));
					}
					lockData();						// needed for sync()
					sync(cur_file_block, TRUE);		// remove first on disk
					sync(last_file_block, TRUE);	// remove last on disk
					unlockData();					// needed for sync()
					last_file_block = cur_file_block;
					++cur;
					continue;
				}

				// Figure out where the last block ended.
				S32 loc = last_file_block->mLocation+last_file_block->mLength;

				// Figure out how much space there is between where
				// the last block ended and this block begins.
				S32 length = cur_file_block->mLocation - loc;
    
				// Check for more errors...  Seeing if the current
				// entry and the last entry make sense together.
				if (length < 0 || loc < 0 || (U32)loc > data_size)
				{
					// Invalid VFS
					unlockAndClose( mIndexFP );
					mIndexFP = NULL;
					LLFile::remove( mIndexFilename );

					unlockAndClose( mDataFP );
					mDataFP = NULL;
					LLFile::remove( mDataFilename );

					LL_WARNS("VFS") << "VFS: overlapping entries"
						<< " at " << cur_file_block->mLocation 
						<< " length " << cur_file_block->mLength 
						<< " ID " << cur_file_block->mFileID 
						<< " type " << cur_file_block->mFileType 
						<< LL_ENDL;

					LL_WARNS("VFS") << "Deleted corrupt VFS files " 
						<< mDataFilename 
						<< " and "
						<< mIndexFilename
						<< LL_ENDL;

					mValid = VFSVALID_BAD_CORRUPT;
					return;
				}

				// we don't want to add empty blocks to the list...
				if (length > 0)
				{
					addFreeBlock(new LLVFSBlock(loc, length));
				}
				last_file_block = cur_file_block;
				++cur;
			}
    
			// also note any empty space at the end
			U32 loc = last_file_block->mLocation + last_file_block->mLength;
			if (loc < data_size)
			{
				addFreeBlock(new LLVFSBlock(loc, data_size - loc));
			}
		}
		else // There where no blocks in the file.
		{
			addFreeBlock(new LLVFSBlock(0, data_size));
		}
	}
	else	// Pre-existing index file wasn't opened
	{
		if (mReadOnly)
		{
			LL_WARNS("VFS") << "Can't find " << mIndexFilename << " to open read-only VFS" << LL_ENDL;
			mValid = VFSVALID_BAD_CANNOT_OPEN_READONLY;
			return;
		}
    
	
		mIndexFP = openAndLock(mIndexFilename, "w+b", FALSE);
		if (!mIndexFP)
		{
			LL_WARNS("VFS") << "Couldn't open an index file for the VFS, probably a sharing violation!" << LL_ENDL;

			unlockAndClose( mDataFP );
			mDataFP = NULL;
			LLFile::remove( mDataFilename );
			
			mValid = VFSVALID_BAD_CANNOT_CREATE;
			return;
		}
	
		// no index file, start from scratch w/ 1GB allocation
		LLVFSBlock *first_block = new LLVFSBlock(0, data_size ? data_size : 0x40000000);
		addFreeBlock(first_block);
	}

	// Open marker file to look for bad shutdowns
	if (!mReadOnly && mRemoveAfterCrash)
	{
		std::string marker = mDataFilename + ".open";
		LLFILE* marker_fp = LLFile::fopen(marker, "w");	/* Flawfinder: ignore */
		if (marker_fp)
		{
			fclose(marker_fp);
			marker_fp = NULL;
		}
	}

	LL_INFOS("VFS") << "Using VFS index file " << mIndexFilename << LL_ENDL;
	LL_INFOS("VFS") << "Using VFS data file " << mDataFilename << LL_ENDL;

	mValid = VFSVALID_OK;
}
    
LLVFS::~LLVFS()
{
	if (mDataMutex->isLocked())
	{
		LL_ERRS("VFS") << "LLVFS destroyed with mutex locked" << LL_ENDL;
	}
	
	unlockAndClose(mIndexFP);
	mIndexFP = NULL;

	fileblock_map::const_iterator it;
	for (it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		delete (*it).second;
	}
	mFileBlocks.clear();
	
	mFreeBlocksByLength.clear();

	for_each(mFreeBlocksByLocation.begin(), mFreeBlocksByLocation.end(), DeletePairedPointer());
    
	unlockAndClose(mDataFP);
	mDataFP = NULL;
    
	// Remove marker file
	if (!mReadOnly && mRemoveAfterCrash)
	{
		std::string marker = mDataFilename + ".open";
		LLFile::remove(marker);
	}

	delete mDataMutex;
}


// Use this function normally to create LLVFS files.  
// Will append digits to the end of the filename with multiple re-trys
// static 
LLVFS * LLVFS::createLLVFS(const std::string& index_filename, 
		const std::string& data_filename, 
		const BOOL read_only, 
		const U32 presize, 
		const BOOL remove_after_crash)
{
	LLVFS * new_vfs = new LLVFS(index_filename, data_filename, read_only, presize, remove_after_crash);

	if( !new_vfs->isValid() )
	{	// First name failed, retry with new names
		std::string retry_vfs_index_name;
		std::string retry_vfs_data_name;
		S32 count = 0;
		while (!new_vfs->isValid() &&
				count < 256)
		{	// Append '.<number>' to end of filenames
			retry_vfs_index_name = index_filename + llformat(".%u",count);
			retry_vfs_data_name = data_filename + llformat(".%u", count);

			delete new_vfs;	// Delete bad VFS and try again
			new_vfs = new LLVFS(retry_vfs_index_name, retry_vfs_data_name, read_only, presize, remove_after_crash);

			count++;
		}
	}

	if( !new_vfs->isValid() )
	{
		delete new_vfs;		// Delete bad VFS
		new_vfs = NULL;		// Total failure
	}

	return new_vfs;
}



void LLVFS::presizeDataFile(const U32 size)
{
	if (!mDataFP)
	{
		llerrs << "LLVFS::presizeDataFile() with no data file open" << llendl;
		return;
	}

	// we're creating this file for the first time, size it
	fseek(mDataFP, size-1, SEEK_SET);
	S32 tmp = 0;
	tmp = (S32)fwrite(&tmp, 1, 1, mDataFP);
	// fflush(mDataFP);

	// also remove any index, since this vfs is now blank
	LLFile::remove(mIndexFilename);

	if (tmp)
	{
		llinfos << "Pre-sized VFS data file to " << ftell(mDataFP) << " bytes" << llendl;
	}
	else
	{
		llwarns << "Failed to pre-size VFS data file" << llendl;
	}
}

BOOL LLVFS::getExists(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	LLVFSFileBlock *block = NULL;
		
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}

	lockData();
	
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		block = (*it).second;
		block->mAccessTime = (U32)time(NULL);
	}

	BOOL res = (block && block->mLength > 0) ? TRUE : FALSE;
	
	unlockData();
	
	return res;
}
    
S32	 LLVFS::getSize(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	S32 size = 0;
	
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;

	}

	lockData();
	
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;

		block->mAccessTime = (U32)time(NULL);
		size = block->mSize;
	}

	unlockData();
	
	return size;
}
    
S32  LLVFS::getMaxSize(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	S32 size = 0;
	
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}

	lockData();
	
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;

		block->mAccessTime = (U32)time(NULL);
		size = block->mLength;
	}

	unlockData();

	return size;
}

BOOL LLVFS::checkAvailable(S32 max_size)
{
	lockData();
	
	blocks_length_map_t::iterator iter = mFreeBlocksByLength.lower_bound(max_size); // first entry >= size
	const BOOL res(iter == mFreeBlocksByLength.end() ? FALSE : TRUE);

	unlockData();
	
	return res;
}

BOOL LLVFS::setMaxSize(const LLUUID &file_id, const LLAssetType::EType file_type, S32 max_size)
{
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}
	if (mReadOnly)
	{
		llerrs << "Attempt to write to read-only VFS" << llendl;
	}
	if (max_size <= 0)
	{
		llwarns << "VFS: Attempt to assign size " << max_size << " to vfile " << file_id << llendl;
		return FALSE;
	}

	lockData();
	
	LLVFSFileSpecifier spec(file_id, file_type);
	LLVFSFileBlock *block = NULL;
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		block = (*it).second;
	}
    
	// round all sizes upward to KB increments
	// SJB: Need to not round for the new texture-pipeline code so we know the correct
	//      max file size. Need to investigate the potential problems with this...
	if (file_type != LLAssetType::AT_TEXTURE)
	{
		if (max_size & FILE_BLOCK_MASK)
		{
			max_size += FILE_BLOCK_MASK;
			max_size &= ~FILE_BLOCK_MASK;
		}
    }
	
	if (block && block->mLength > 0)
	{    
		block->mAccessTime = (U32)time(NULL);
    
		if (max_size == block->mLength)
		{
			unlockData();
			return TRUE;
		}
		else if (max_size < block->mLength)
		{
			// this file is shrinking
			LLVFSBlock *free_block = new LLVFSBlock(block->mLocation + max_size, block->mLength - max_size);

			addFreeBlock(free_block);
    
			block->mLength = max_size;
    
			if (block->mLength < block->mSize)
			{
				// JC: Was a warning, but Ian says it's bad.
				llerrs << "Truncating virtual file " << file_id << " to " << block->mLength << " bytes" << llendl;
				block->mSize = block->mLength;
			}
    
			sync(block);
			//mergeFreeBlocks();

			unlockData();
			return TRUE;
		}
		else if (max_size > block->mLength)
		{
			// this file is growing
			// first check for an adjacent free block to grow into
			S32 size_increase = max_size - block->mLength;

			// Find the first free block with and addres > block->mLocation
			LLVFSBlock *free_block;
			blocks_location_map_t::iterator iter = mFreeBlocksByLocation.upper_bound(block->mLocation);
			if (iter != mFreeBlocksByLocation.end())
			{
				free_block = iter->second;
			
				if (free_block->mLocation == block->mLocation + block->mLength &&
					free_block->mLength >= size_increase)
				{
					// this free block is at the end of the file and is large enough

					// Must call useFreeSpace before sync(), as sync()
					// unlocks data structures.
					useFreeSpace(free_block, size_increase);
					block->mLength += size_increase;
					sync(block);

					unlockData();
					return TRUE;
				}
			}
			
			// no adjecent free block, find one in the list
			free_block = findFreeBlock(max_size, block);
    
			if (free_block)
			{
				// Save location where data is going, useFreeSpace will move free_block->mLocation;
				U32 new_data_location = free_block->mLocation;

				//mark the free block as used so it does not
				//interfere with other operations such as addFreeBlock
				useFreeSpace(free_block, max_size);		// useFreeSpace takes ownership (and may delete) free_block

				if (block->mLength > 0)
				{
					// create a new free block where this file used to be
					LLVFSBlock *new_free_block = new LLVFSBlock(block->mLocation, block->mLength);

					addFreeBlock(new_free_block);
					
					if (block->mSize > 0)
					{
						// move the file into the new block
						std::vector<U8> buffer(block->mSize);
						fseek(mDataFP, block->mLocation, SEEK_SET);
						if (fread(&buffer[0], block->mSize, 1, mDataFP) == 1)
						{
							fseek(mDataFP, new_data_location, SEEK_SET);
							if (fwrite(&buffer[0], block->mSize, 1, mDataFP) != 1)
							{
								llwarns << "Short write" << llendl;
							}
						} else {
							llwarns << "Short read" << llendl;
						}
					}
				}
    
				block->mLocation = new_data_location;
    
				block->mLength = max_size;


				sync(block);

				unlockData();
				return TRUE;
			}
			else
			{
				llwarns << "VFS: No space (" << max_size << ") to resize existing vfile " << file_id << llendl;
				//dumpMap();
				unlockData();
				dumpStatistics();
				return FALSE;
			}
		}
	}
	else
	{
		// find a free block in the list
		LLVFSBlock *free_block = findFreeBlock(max_size);
    
		if (free_block)
		{        
			if (block)
			{
				block->mLocation = free_block->mLocation;
				block->mLength = max_size;
			}
			else
			{
				// this file doesn't exist, create it
				block = new LLVFSFileBlock(file_id, file_type, free_block->mLocation, max_size);
				mFileBlocks.insert(fileblock_map::value_type(spec, block));
			}

			// Must call useFreeSpace before sync(), as sync()
			// unlocks data structures.
			useFreeSpace(free_block, max_size);
			block->mAccessTime = (U32)time(NULL);

			sync(block);
		}
		else
		{
			llwarns << "VFS: No space (" << max_size << ") for new virtual file " << file_id << llendl;
			//dumpMap();
			unlockData();
			dumpStatistics();
			return FALSE;
		}
	}
	unlockData();
	return TRUE;
}


// WARNING: HERE BE DRAGONS!
// rename is the weirdest VFS op, because the file moves but the locks don't!
void LLVFS::renameFile(const LLUUID &file_id, const LLAssetType::EType file_type,
					   const LLUUID &new_id, const LLAssetType::EType &new_type)
{
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}
	if (mReadOnly)
	{
		llerrs << "Attempt to write to read-only VFS" << llendl;
	}

	lockData();
	
	LLVFSFileSpecifier new_spec(new_id, new_type);
	LLVFSFileSpecifier old_spec(file_id, file_type);
	
	fileblock_map::iterator it = mFileBlocks.find(old_spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *src_block = (*it).second;

		// this will purge the data but leave the file block in place, w/ locks, if any
		// WAS: removeFile(new_id, new_type); NOW uses removeFileBlock() to avoid mutex lock recursion
		fileblock_map::iterator new_it = mFileBlocks.find(new_spec);
		if (new_it != mFileBlocks.end())
		{
			LLVFSFileBlock *new_block = (*new_it).second;
			removeFileBlock(new_block);
		}
		
		// if there's something in the target location, remove it but inherit its locks
		it = mFileBlocks.find(new_spec);
		if (it != mFileBlocks.end())
		{
			LLVFSFileBlock *dest_block = (*it).second;

			for (S32 i = 0; i < (S32)VFSLOCK_COUNT; i++)
			{
				if(dest_block->mLocks[i])
				{
					llerrs << "Renaming VFS block to a locked file." << llendl;
				}
				dest_block->mLocks[i] = src_block->mLocks[i];
			}
			
			mFileBlocks.erase(new_spec);
			delete dest_block;
		}

		src_block->mFileID = new_id;
		src_block->mFileType = new_type;
		src_block->mAccessTime = (U32)time(NULL);
   
		mFileBlocks.erase(old_spec);
		mFileBlocks.insert(fileblock_map::value_type(new_spec, src_block));

		sync(src_block);
	}
	else
	{
		llwarns << "VFS: Attempt to rename nonexistent vfile " << file_id << ":" << file_type << llendl;
	}
	unlockData();
}

// mDataMutex must be LOCKED before calling this
void LLVFS::removeFileBlock(LLVFSFileBlock *fileblock)
{
	// convert this into an unsaved, dummy fileblock to preserve locks
	// a more rubust solution would store the locks in a seperate data structure
	sync(fileblock, TRUE);
	
	if (fileblock->mLength > 0)
	{
		// turn this file into an empty block
		LLVFSBlock *free_block = new LLVFSBlock(fileblock->mLocation, fileblock->mLength);
		
		addFreeBlock(free_block);
	}
	
	fileblock->mLocation = 0;
	fileblock->mSize = 0;
	fileblock->mLength = BLOCK_LENGTH_INVALID;
	fileblock->mIndexLocation = -1;

	//mergeFreeBlocks();
}

void LLVFS::removeFile(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}
	if (mReadOnly)
	{
		llerrs << "Attempt to write to read-only VFS" << llendl;
	}

    lockData();
	
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;
		removeFileBlock(block);
	}
	else
	{
		llwarns << "VFS: attempting to remove nonexistent file " << file_id << " type " << file_type << llendl;
	}

	unlockData();
}
    
    
S32 LLVFS::getData(const LLUUID &file_id, const LLAssetType::EType file_type, U8 *buffer, S32 location, S32 length)
{
	S32 bytesread = 0;
	
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}
	llassert(location >= 0);
	llassert(length >= 0);

	BOOL do_read = FALSE;
	
    lockData();
	
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;

		block->mAccessTime = (U32)time(NULL);
    
		if (location > block->mSize)
		{
			llwarns << "VFS: Attempt to read location " << location << " in file " << file_id << " of length " << block->mSize << llendl;
		}
		else
		{
			if (length > block->mSize - location)
			{
				length = block->mSize - location;
			}
			location += block->mLocation;
			do_read = TRUE;
		}
	}

	if (do_read)
	{
		fseek(mDataFP, location, SEEK_SET);
		bytesread = (S32)fread(buffer, 1, length, mDataFP);
	}
	
	unlockData();

	return bytesread;
}
    
S32 LLVFS::storeData(const LLUUID &file_id, const LLAssetType::EType file_type, const U8 *buffer, S32 location, S32 length)
{
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}
	if (mReadOnly)
	{
		llerrs << "Attempt to write to read-only VFS" << llendl;
	}
    
	llassert(length > 0);

    lockData();
    
	LLVFSFileSpecifier spec(file_id, file_type);
	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;

		S32 in_loc = location;
		if (location == -1)
		{
			location = block->mSize;
		}
		llassert(location >= 0);
		
		block->mAccessTime = (U32)time(NULL);
    
		if (block->mLength == BLOCK_LENGTH_INVALID)
		{
			// Block was removed, ignore write
			llwarns << "VFS: Attempt to write to invalid block"
					<< " in file " << file_id 
					<< " location: " << in_loc
					<< " bytes: " << length
					<< llendl;
			unlockData();
			return length;
		}
		else if (location > block->mLength)
		{
			llwarns << "VFS: Attempt to write to location " << location 
					<< " in file " << file_id 
					<< " type " << S32(file_type)
					<< " of size " << block->mSize
					<< " block length " << block->mLength
					<< llendl;
			unlockData();
			return length;
		}
		else
		{
			if (length > block->mLength - location )
			{
				llwarns << "VFS: Truncating write to virtual file " << file_id << " type " << S32(file_type) << llendl;
				length = block->mLength - location;
			}
			U32 file_location = location + block->mLocation;
			
			fseek(mDataFP, file_location, SEEK_SET);
			S32 write_len = (S32)fwrite(buffer, 1, length, mDataFP);
			if (write_len != length)
			{
				llwarns << llformat("VFS Write Error: %d != %d",write_len,length) << llendl;
			}
			// fflush(mDataFP);
			
			if (location + length > block->mSize)
			{
				block->mSize = location + write_len;
				sync(block);
			}
			unlockData();
			
			return write_len;
		}
	}
	else
	{
		unlockData();
		return 0;
	}
}
 
void LLVFS::incLock(const LLUUID &file_id, const LLAssetType::EType file_type, EVFSLock lock)
{
	lockData();

	LLVFSFileSpecifier spec(file_id, file_type);
	LLVFSFileBlock *block;
	
 	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		block = (*it).second;
	}
	else
	{
		// Create a dummy block which isn't saved
		block = new LLVFSFileBlock(file_id, file_type, 0, BLOCK_LENGTH_INVALID);
    	block->mAccessTime = (U32)time(NULL);
		mFileBlocks.insert(fileblock_map::value_type(spec, block));
	}

	block->mLocks[lock]++;
	mLockCounts[lock]++;
	
	unlockData();
}

void LLVFS::decLock(const LLUUID &file_id, const LLAssetType::EType file_type, EVFSLock lock)
{
	lockData();

	LLVFSFileSpecifier spec(file_id, file_type);
 	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;

		if (block->mLocks[lock] > 0)
		{
			block->mLocks[lock]--;
		}
		else
		{
			llwarns << "VFS: Decrementing zero-value lock " << lock << llendl;
		}
		mLockCounts[lock]--;
	}

	unlockData();
}

BOOL LLVFS::isLocked(const LLUUID &file_id, const LLAssetType::EType file_type, EVFSLock lock)
{
	lockData();
	
	BOOL res = FALSE;
	
	LLVFSFileSpecifier spec(file_id, file_type);
 	fileblock_map::iterator it = mFileBlocks.find(spec);
	if (it != mFileBlocks.end())
	{
		LLVFSFileBlock *block = (*it).second;
		res = (block->mLocks[lock] > 0);
	}

	unlockData();

	return res;
}

//============================================================================
// protected
//============================================================================

void LLVFS::eraseBlockLength(LLVFSBlock *block)
{
	// find the corresponding map entry in the length map and erase it
	S32 length = block->mLength;
	blocks_length_map_t::iterator iter = mFreeBlocksByLength.lower_bound(length);
	blocks_length_map_t::iterator end = mFreeBlocksByLength.end();
	bool found_block = false;
	while(iter != end)
	{
		LLVFSBlock *tblock = iter->second;
		llassert(tblock->mLength == length); // there had -better- be an entry with our length!
		if (tblock == block)
		{
			mFreeBlocksByLength.erase(iter);
			found_block = true;
			break;
		}
		++iter;
	}
	if(!found_block)
	{
		llerrs << "eraseBlock could not find block" << llendl;
	}
}


// Remove block from both free lists (by location and by length).
void LLVFS::eraseBlock(LLVFSBlock *block)
{
	eraseBlockLength(block);
	// find the corresponding map entry in the location map and erase it	
	U32 location = block->mLocation;
	llverify(mFreeBlocksByLocation.erase(location) == 1); // we should only have one entry per location.
}


// Add the region specified by block location and length to the free lists.
// Also incrementally defragment by merging with previous and next free blocks.
void LLVFS::addFreeBlock(LLVFSBlock *block)
{
#if LL_DEBUG
	size_t dbgcount = mFreeBlocksByLocation.count(block->mLocation);
	if(dbgcount > 0)
	{
		llerrs << "addFreeBlock called with block already in list" << llendl;
	}
#endif

	// Get a pointer to the next free block (by location).
	blocks_location_map_t::iterator next_free_it = mFreeBlocksByLocation.lower_bound(block->mLocation);

	// We can merge with previous if it ends at our requested location.
	LLVFSBlock* prev_block = NULL;
	bool merge_prev = false;
	if (next_free_it != mFreeBlocksByLocation.begin())
	{
		blocks_location_map_t::iterator prev_free_it = next_free_it;
		--prev_free_it;
		prev_block = prev_free_it->second;
		merge_prev = (prev_block->mLocation + prev_block->mLength == block->mLocation);
	}

	// We can merge with next if our block ends at the next block's location.
	LLVFSBlock* next_block = NULL;
	bool merge_next = false;
	if (next_free_it != mFreeBlocksByLocation.end())
	{
		next_block = next_free_it->second;
		merge_next = (block->mLocation + block->mLength == next_block->mLocation);
	}

	if (merge_prev && merge_next)
	{
		// llinfos << "VFS merge BOTH" << llendl;
		// Previous block is changing length (a lot), so only need to update length map.
		// Next block is going away completely. JC
		eraseBlockLength(prev_block);
		eraseBlock(next_block);
		prev_block->mLength += block->mLength + next_block->mLength;
		mFreeBlocksByLength.insert(blocks_length_map_t::value_type(prev_block->mLength, prev_block));
		delete block;
		block = NULL;
		delete next_block;
		next_block = NULL;
	}
	else if (merge_prev)
	{
		// llinfos << "VFS merge previous" << llendl;
		// Previous block is maintaining location, only changing length,
		// therefore only need to update the length map. JC
		eraseBlockLength(prev_block);
		prev_block->mLength += block->mLength;
		mFreeBlocksByLength.insert(blocks_length_map_t::value_type(prev_block->mLength, prev_block)); // multimap insert
		delete block;
		block = NULL;
	}
	else if (merge_next)
	{
		// llinfos << "VFS merge next" << llendl;
		// Next block is changing both location and length,
		// so both free lists must update. JC
		eraseBlock(next_block);
		next_block->mLocation = block->mLocation;
		next_block->mLength += block->mLength;
		// Don't hint here, next_free_it iterator may be invalid.
		mFreeBlocksByLocation.insert(blocks_location_map_t::value_type(next_block->mLocation, next_block)); // multimap insert
		mFreeBlocksByLength.insert(blocks_length_map_t::value_type(next_block->mLength, next_block)); // multimap insert			
		delete block;
		block = NULL;
	}
	else
	{
		// Can't merge with other free blocks.
		// Hint that insert should go near next_free_it.
 		mFreeBlocksByLocation.insert(next_free_it, blocks_location_map_t::value_type(block->mLocation, block)); // multimap insert
 		mFreeBlocksByLength.insert(blocks_length_map_t::value_type(block->mLength, block)); // multimap insert
	}
}

// Superceeded by new addFreeBlock which does incremental free space merging.
// Incremental is faster overall.
//void LLVFS::mergeFreeBlocks()
//{
// 	if (!isValid())
// 	{
// 		llerrs << "Attempting to use invalid VFS!" << llendl;
// 	}
// 	// TODO: could we optimize this with hints from the calling code?
// 	blocks_location_map_t::iterator iter = mFreeBlocksByLocation.begin();	
// 	blocks_location_map_t::iterator end = mFreeBlocksByLocation.end();	
// 	LLVFSBlock *first_block = iter->second;
// 	while(iter != end)
// 	{
// 		blocks_location_map_t::iterator first_iter = iter; // save for if we do a merge
// 		if (++iter == end)
// 			break;
// 		LLVFSBlock *second_block = iter->second;
// 		if (first_block->mLocation + first_block->mLength == second_block->mLocation)
// 		{
// 			// remove the first block from the length map
// 			eraseBlockLength(first_block);
// 			// merge first_block with second_block, since they're adjacent
// 			first_block->mLength += second_block->mLength;
// 			// add the first block to the length map (with the new size)
// 			mFreeBlocksByLength.insert(blocks_length_map_t::value_type(first_block->mLength, first_block)); // multimap insert
//
// 			// erase and delete the second block
// 			eraseBlock(second_block);
// 			delete second_block;
//
// 			// reset iterator
// 			iter = first_iter; // haven't changed first_block, so corresponding iterator is still valid
// 			end = mFreeBlocksByLocation.end();
// 		}
// 		first_block = second_block;
// 	}
//}
	
// length bytes from free_block are going to be used (so they are no longer free)
void LLVFS::useFreeSpace(LLVFSBlock *free_block, S32 length)
{
	if (free_block->mLength == length)
	{
		eraseBlock(free_block);
		delete free_block;
	}
	else
	{
		eraseBlock(free_block);
  		
		free_block->mLocation += length;
		free_block->mLength -= length;

		addFreeBlock(free_block);
	}
}

// NOTE! mDataMutex must be LOCKED before calling this
// sync this index entry out to the index file
// we need to do this constantly to avoid corruption on viewer crash
void LLVFS::sync(LLVFSFileBlock *block, BOOL remove)
{
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}
	if (mReadOnly)
	{
		llwarns << "Attempt to sync read-only VFS" << llendl;
		return;
	}
	if (block->mLength == BLOCK_LENGTH_INVALID)
	{
		// This is a dummy file, don't save
		return;
	}
	if (block->mLength == 0)
	{
		llerrs << "VFS syncing zero-length block" << llendl;
	}

    BOOL set_index_to_end = FALSE;
	long seek_pos = block->mIndexLocation;
		
	if (-1 == seek_pos)
	{
		if (!mIndexHoles.empty())
		{
			seek_pos = mIndexHoles.front();
			mIndexHoles.pop_front();
		}
		else
		{
			set_index_to_end = TRUE;
		}
	}

    if (set_index_to_end)
	{
		// Need fseek/ftell to update the seek_pos and hence data
		// structures, so can't unlockData() before this.
		fseek(mIndexFP, 0, SEEK_END);
		seek_pos = ftell(mIndexFP);
	}
	    
	block->mIndexLocation = seek_pos;
	if (remove)
	{
		mIndexHoles.push_back(seek_pos);
	}

	U8 buffer[LLVFSFileBlock::SERIAL_SIZE];
	if (remove)
	{
		memset(buffer, 0, LLVFSFileBlock::SERIAL_SIZE);
	}
	else
	{
		block->serialize(buffer);
	}

	// If set_index_to_end, file pointer is already at seek_pos
	// and we don't need to do anything.  Only seek if not at end.
	if (!set_index_to_end)
	{
		fseek(mIndexFP, seek_pos, SEEK_SET);
	}

	if (fwrite(buffer, LLVFSFileBlock::SERIAL_SIZE, 1, mIndexFP) != 1)
	{
		llwarns << "Short write" << llendl;
	}

	// *NOTE:  Why was this commented out?
	// fflush(mIndexFP);
	
	return;
}

// mDataMutex must be LOCKED before calling this
// Can initiate LRU-based file removal to make space.
// The immune file block will not be removed.
LLVFSBlock *LLVFS::findFreeBlock(S32 size, LLVFSFileBlock *immune)
{
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}

	LLVFSBlock *block = NULL;
	BOOL have_lru_list = FALSE;
	
	typedef std::set<LLVFSFileBlock*, LLVFSFileBlock_less> lru_set;
	lru_set lru_list;
    
	LLTimer timer;

	while (! block)
	{
		// look for a suitable free block
		blocks_length_map_t::iterator iter = mFreeBlocksByLength.lower_bound(size); // first entry >= size
		if (iter != mFreeBlocksByLength.end())
			block = iter->second;
    	
		// no large enough free blocks, time to clean out some junk
		if (! block)
		{
			// create a list of files sorted by usage time
			// this is far faster than sorting a linked list
			if (! have_lru_list)
			{
				for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
				{
					LLVFSFileBlock *tmp = (*it).second;

					if (tmp != immune &&
						tmp->mLength > 0 &&
						! tmp->mLocks[VFSLOCK_READ] &&
						! tmp->mLocks[VFSLOCK_APPEND] &&
						! tmp->mLocks[VFSLOCK_OPEN])
					{
						lru_list.insert(tmp);
					}
				}
				
				have_lru_list = TRUE;
			}

			if (lru_list.size() == 0)
			{
				// No more files to delete, and still not enough room!
				llwarns << "VFS: Can't make " << size << " bytes of free space in VFS, giving up" << llendl;
				break;
			}

			// is the oldest file big enough?  (Should be about half the time)
			lru_set::iterator it = lru_list.begin();
			LLVFSFileBlock *file_block = *it;
			if (file_block->mLength >= size && file_block != immune)
			{
				// ditch this file and look again for a free block - should find it
				// TODO: it'll be faster just to assign the free block and break
				llinfos << "LRU: Removing " << file_block->mFileID << ":" << file_block->mFileType << llendl;
				lru_list.erase(it);
				removeFileBlock(file_block);
				file_block = NULL;
				continue;
			}

			
			llinfos << "VFS: LRU: Aggressive: " << (S32)lru_list.size() << " files remain" << llendl;
			dumpLockCounts();
			
			// Now it's time to aggressively make more space
			// Delete the oldest 5MB of the vfs or enough to hold the file, which ever is larger
			// This may yield too much free space, but we'll use it up soon enough
			U32 cleanup_target = (size > VFS_CLEANUP_SIZE) ? size : VFS_CLEANUP_SIZE;
			U32 cleaned_up = 0;
		   	for (it = lru_list.begin();
				 it != lru_list.end() && cleaned_up < cleanup_target;
				 )
			{
				file_block = *it;
				
				// TODO: it would be great to be able to batch all these sync() calls
				// llinfos << "LRU2: Removing " << file_block->mFileID << ":" << file_block->mFileType << " last accessed" << file_block->mAccessTime << llendl;

				cleaned_up += file_block->mLength;
				lru_list.erase(it++);
				removeFileBlock(file_block);
				file_block = NULL;
			}
			//mergeFreeBlocks();
		}
	}
    
	F32 time = timer.getElapsedTimeF32();
	if (time > 0.5f)
	{
		llwarns << "VFS: Spent " << time << " seconds in findFreeBlock!" << llendl;
	}

	return block;
}

//============================================================================
// public
//============================================================================

void LLVFS::pokeFiles()
{
	if (!isValid())
	{
		llerrs << "Attempting to use invalid VFS!" << llendl;
	}
	U32 word;
	
	// only write data if we actually read 4 bytes
	// otherwise we're writing garbage and screwing up the file
	fseek(mDataFP, 0, SEEK_SET);
	if (fread(&word, sizeof(word), 1, mDataFP) == 1)
	{
		fseek(mDataFP, 0, SEEK_SET);
		if (fwrite(&word, sizeof(word), 1, mDataFP) != 1)
		{
			llwarns << "Could not write to data file" << llendl;
		}
		fflush(mDataFP);
	}

	fseek(mIndexFP, 0, SEEK_SET);
	if (fread(&word, sizeof(word), 1, mIndexFP) == 1)
	{
		fseek(mIndexFP, 0, SEEK_SET);
		if (fwrite(&word, sizeof(word), 1, mIndexFP) != 1)
		{
			llwarns << "Could not write to index file" << llendl;
		}
		fflush(mIndexFP);
	}
}

    
void LLVFS::dumpMap()
{
	llinfos << "Files:" << llendl;
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileBlock *file_block = (*it).second;
		llinfos << "Location: " << file_block->mLocation << "\tLength: " << file_block->mLength << "\t" << file_block->mFileID << "\t" << file_block->mFileType << llendl;
	}
    
	llinfos << "Free Blocks:" << llendl;
	for (blocks_location_map_t::iterator iter = mFreeBlocksByLocation.begin(),
			 end = mFreeBlocksByLocation.end();
		 iter != end; iter++)
	{
		LLVFSBlock *free_block = iter->second;
		llinfos << "Location: " << free_block->mLocation << "\tLength: " << free_block->mLength << llendl;
	}
}
    
// verify that the index file contents match the in-memory file structure
// Very slow, do not call routinely. JC
void LLVFS::audit()
{
	// Lock the mutex through this whole function.
	LLMutexLock lock_data(mDataMutex);
	
	fflush(mIndexFP);

	fseek(mIndexFP, 0, SEEK_END);
	size_t index_size = ftell(mIndexFP);
	fseek(mIndexFP, 0, SEEK_SET);
    
	BOOL vfs_corrupt = FALSE;
	
	std::vector<U8> buffer(index_size);

	if (fread(&buffer[0], 1, index_size, mIndexFP) != index_size)
	{
		llwarns << "Index truncated" << llendl;
		vfs_corrupt = TRUE;
	}
    
	size_t buf_offset = 0;
    
	std::map<LLVFSFileSpecifier, LLVFSFileBlock*>	found_files;
	U32 cur_time = (U32)time(NULL);

	std::vector<LLVFSFileBlock*> audit_blocks;
	while (!vfs_corrupt && buf_offset < index_size)
	{
		LLVFSFileBlock *block = new LLVFSFileBlock();
		audit_blocks.push_back(block);
		
		block->deserialize(&buffer[buf_offset], (S32)buf_offset);
		buf_offset += block->SERIAL_SIZE;
    
		// do sanity check on this block
		if (block->mLength >= 0 &&
			block->mSize >= 0 &&
			block->mSize <= block->mLength &&
			block->mFileType >= LLAssetType::AT_NONE &&
			block->mFileType < LLAssetType::AT_COUNT &&
			block->mAccessTime <= cur_time &&
			block->mFileID != LLUUID::null)
		{
			if (mFileBlocks.find(*block) == mFileBlocks.end())
			{
				llwarns << "VFile " << block->mFileID << ":" << block->mFileType << " on disk, not in memory, loc " << block->mIndexLocation << llendl;
			}
			else if (found_files.find(*block) != found_files.end())
			{
				std::map<LLVFSFileSpecifier, LLVFSFileBlock*>::iterator it;
				it = found_files.find(*block);
				LLVFSFileBlock* dupe = it->second;
				// try to keep data from being lost
				unlockAndClose(mIndexFP);
				mIndexFP = NULL;
				unlockAndClose(mDataFP);
				mDataFP = NULL;
				llwarns << "VFS: Original block index " << block->mIndexLocation
					<< " location " << block->mLocation 
					<< " length " << block->mLength 
					<< " size " << block->mSize 
					<< " id " << block->mFileID
					<< " type " << block->mFileType
					<< llendl;
				llwarns << "VFS: Duplicate block index " << dupe->mIndexLocation
					<< " location " << dupe->mLocation 
					<< " length " << dupe->mLength 
					<< " size " << dupe->mSize 
					<< " id " << dupe->mFileID
					<< " type " << dupe->mFileType
					<< llendl;
				llwarns << "VFS: Index size " << index_size << llendl;
				llwarns << "VFS: INDEX CORRUPT" << llendl;
				vfs_corrupt = TRUE;
				break;
			}
			else
			{
				found_files[*block] = block;
			}
		}
		else
		{
			if (block->mLength)
			{
				llwarns << "VFile " << block->mFileID << ":" << block->mFileType << " corrupt on disk" << llendl;
			}
			// else this is just a hole
		}
	}
    
	if (!vfs_corrupt)
	{
		for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
		{
			LLVFSFileBlock* block = (*it).second;

			if (block->mSize > 0)
			{
				if (! found_files.count(*block))
				{
					llwarns << "VFile " << block->mFileID << ":" << block->mFileType << " in memory, not on disk, loc " << block->mIndexLocation<< llendl;
					fseek(mIndexFP, block->mIndexLocation, SEEK_SET);
					U8 buf[LLVFSFileBlock::SERIAL_SIZE];
					if (fread(buf, LLVFSFileBlock::SERIAL_SIZE, 1, mIndexFP) != 1)
					{
						llwarns << "VFile " << block->mFileID
								<< " gave short read" << llendl;
					}
    			
					LLVFSFileBlock disk_block;
					disk_block.deserialize(buf, block->mIndexLocation);
				
					llwarns << "Instead found " << disk_block.mFileID << ":" << block->mFileType << llendl;
				}
				else
				{
					block = found_files.find(*block)->second;
					found_files.erase(*block);
				}
			}
		}
    
		for (std::map<LLVFSFileSpecifier, LLVFSFileBlock*>::iterator iter = found_files.begin();
			 iter != found_files.end(); iter++)
		{
			LLVFSFileBlock* block = iter->second;
			llwarns << "VFile " << block->mFileID << ":" << block->mFileType << " szie:" << block->mSize << " leftover" << llendl;
		}
    
		llinfos << "VFS: audit OK" << llendl;
		// mutex released by LLMutexLock() destructor.
	}

	for_each(audit_blocks.begin(), audit_blocks.end(), DeletePointer());
}
    
    
// quick check for uninitialized blocks
// Slow, do not call in release.
void LLVFS::checkMem()
{
	lockData();
	
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileBlock *block = (*it).second;
		llassert(block->mFileType >= LLAssetType::AT_NONE &&
				 block->mFileType < LLAssetType::AT_COUNT &&
				 block->mFileID != LLUUID::null);
    
		for (std::deque<S32>::iterator iter = mIndexHoles.begin();
			 iter != mIndexHoles.end(); ++iter)
		{
			S32 index_loc = *iter;
			if (index_loc == block->mIndexLocation)
			{
				llwarns << "VFile block " << block->mFileID << ":" << block->mFileType << " is marked as a hole" << llendl;
			}
		}
	}
    
	llinfos << "VFS: mem check OK" << llendl;

	unlockData();
}

void LLVFS::dumpLockCounts()
{
	S32 i;
	for (i = 0; i < VFSLOCK_COUNT; i++)
	{
		llinfos << "LockType: " << i << ": " << mLockCounts[i] << llendl;
	}
}

void LLVFS::dumpStatistics()
{
	lockData();
	
	// Investigate file blocks.
	std::map<S32, S32> size_counts;
	std::map<U32, S32> location_counts;
	std::map<LLAssetType::EType, std::pair<S32,S32> > filetype_counts;

	S32 max_file_size = 0;
	S32 total_file_size = 0;
	S32 invalid_file_count = 0;
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileBlock *file_block = (*it).second;
		if (file_block->mLength == BLOCK_LENGTH_INVALID)
		{
			invalid_file_count++;
		}
		else if (file_block->mLength <= 0)
		{
			llinfos << "Bad file block at: " << file_block->mLocation << "\tLength: " << file_block->mLength << "\t" << file_block->mFileID << "\t" << file_block->mFileType << llendl;
			size_counts[file_block->mLength]++;
			location_counts[file_block->mLocation]++;
		}
		else
		{
			total_file_size += file_block->mLength;
		}

		if (file_block->mLength > max_file_size)
		{
			max_file_size = file_block->mLength;
		}

		filetype_counts[file_block->mFileType].first++;
		filetype_counts[file_block->mFileType].second += file_block->mLength;
	}
    
	for (std::map<S32,S32>::iterator it = size_counts.begin(); it != size_counts.end(); ++it)
	{
		S32 size = it->first;
		S32 size_count = it->second;
		llinfos << "Bad files size " << size << " count " << size_count << llendl;
	}
	for (std::map<U32,S32>::iterator it = location_counts.begin(); it != location_counts.end(); ++it)
	{
		U32 location = it->first;
		S32 location_count = it->second;
		llinfos << "Bad files location " << location << " count " << location_count << llendl;
	}

	// Investigate free list.
	S32 max_free_size = 0;
	S32 total_free_size = 0;
	std::map<S32, S32> free_length_counts;
	for (blocks_location_map_t::iterator iter = mFreeBlocksByLocation.begin(),
			 end = mFreeBlocksByLocation.end();
		 iter != end; iter++)
	{
		LLVFSBlock *free_block = iter->second;
		if (free_block->mLength <= 0)
		{
			llinfos << "Bad free block at: " << free_block->mLocation << "\tLength: " << free_block->mLength << llendl;
		}
		else
		{
			llinfos << "Block: " << free_block->mLocation
					<< "\tLength: " << free_block->mLength
					<< "\tEnd: " << free_block->mLocation + free_block->mLength
					<< llendl;
			total_free_size += free_block->mLength;
		}

		if (free_block->mLength > max_free_size)
		{
			max_free_size = free_block->mLength;
		}

		free_length_counts[free_block->mLength]++;
	}

	// Dump histogram of free block sizes
	for (std::map<S32,S32>::iterator it = free_length_counts.begin(); it != free_length_counts.end(); ++it)
	{
		llinfos << "Free length " << it->first << " count " << it->second << llendl;
	}

	llinfos << "Invalid blocks: " << invalid_file_count << llendl;
	llinfos << "File blocks:    " << mFileBlocks.size() << llendl;

	S32 length_list_count = (S32)mFreeBlocksByLength.size();
	S32 location_list_count = (S32)mFreeBlocksByLocation.size();
	if (length_list_count == location_list_count)
	{
		llinfos << "Free list lengths match, free blocks: " << location_list_count << llendl;
	}
	else
	{
		llwarns << "Free list lengths do not match!" << llendl;
		llwarns << "By length: " << length_list_count << llendl;
		llwarns << "By location: " << location_list_count << llendl;
	}
	llinfos << "Max file: " << max_file_size/1024 << "K" << llendl;
	llinfos << "Max free: " << max_free_size/1024 << "K" << llendl;
	llinfos << "Total file size: " << total_file_size/1024 << "K" << llendl;
	llinfos << "Total free size: " << total_free_size/1024 << "K" << llendl;
	llinfos << "Sum: " << (total_file_size + total_free_size) << " bytes" << llendl;
	llinfos << llformat("%.0f%% full",((F32)(total_file_size)/(F32)(total_file_size+total_free_size))*100.f) << llendl;

	llinfos << " " << llendl;
	for (std::map<LLAssetType::EType, std::pair<S32,S32> >::iterator iter = filetype_counts.begin();
		 iter != filetype_counts.end(); ++iter)
	{
		llinfos << "Type: " << LLAssetType::getDesc(iter->first)
				<< " Count: " << iter->second.first
				<< " Bytes: " << (iter->second.second>>20) << " MB" << llendl;
	}
	
	// Look for potential merges 
	{
 		blocks_location_map_t::iterator iter = mFreeBlocksByLocation.begin();	
 		blocks_location_map_t::iterator end = mFreeBlocksByLocation.end();	
 		LLVFSBlock *first_block = iter->second;
 		while(iter != end)
 		{
 			if (++iter == end)
 				break;
 			LLVFSBlock *second_block = iter->second;
 			if (first_block->mLocation + first_block->mLength == second_block->mLocation)
 			{
				llinfos << "Potential merge at " << first_block->mLocation << llendl;
 			}
 			first_block = second_block;
 		}
	}
	unlockData();
}

// Debug Only!
std::string get_extension(LLAssetType::EType type)
{
	std::string extension;
	switch(type)
	{
	case LLAssetType::AT_TEXTURE:
		extension = ".jp2";	// formerly ".j2c"
		break;
	case LLAssetType::AT_SOUND:
		extension = ".ogg";
		break;
	case LLAssetType::AT_SOUND_WAV:
		extension = ".wav";
		break;
	case LLAssetType::AT_TEXTURE_TGA:
		extension = ".tga";
		break;
	case LLAssetType::AT_ANIMATION:
		extension = ".lla";
		break;
	case LLAssetType::AT_MESH:
		extension = ".slm";
		break;
	default:
		// Just use the asset server filename extension in most cases
		extension += ".";
		extension += LLAssetType::lookup(type);
		break;
	}
	return extension;
}

void LLVFS::listFiles()
{
	lockData();
	
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileSpecifier file_spec = it->first;
		LLVFSFileBlock *file_block = it->second;
		S32 length = file_block->mLength;
		S32 size = file_block->mSize;
		if (length != BLOCK_LENGTH_INVALID && size > 0)
		{
			LLUUID id = file_spec.mFileID;
			std::string extension = get_extension(file_spec.mFileType);
			llinfos << " File: " << id
					<< " Type: " << LLAssetType::getDesc(file_spec.mFileType)
					<< " Size: " << size
					<< llendl;
		}
	}
	
	unlockData();
}

#include "llapr.h"
void LLVFS::dumpFiles()
{
	lockData();
	
	S32 files_extracted = 0;
	for (fileblock_map::iterator it = mFileBlocks.begin(); it != mFileBlocks.end(); ++it)
	{
		LLVFSFileSpecifier file_spec = it->first;
		LLVFSFileBlock *file_block = it->second;
		S32 length = file_block->mLength;
		S32 size = file_block->mSize;
		if (length != BLOCK_LENGTH_INVALID && size > 0)
		{
			LLUUID id = file_spec.mFileID;
			LLAssetType::EType type = file_spec.mFileType;
			std::vector<U8> buffer(size);

			unlockData();
			getData(id, type, &buffer[0], 0, size);
			lockData();
			
			std::string extension = get_extension(type);
			std::string filename = id.asString() + extension;
			llinfos << " Writing " << filename << llendl;
			
			LLAPRFile outfile;
			outfile.open(filename, LL_APR_WB);
			outfile.write(&buffer[0], size);
			outfile.close();

			files_extracted++;
		}
	}
	
	unlockData();

	llinfos << "Extracted " << files_extracted << " files out of " << mFileBlocks.size() << llendl;
}

//============================================================================
// protected
//============================================================================

// static
LLFILE *LLVFS::openAndLock(const std::string& filename, const char* mode, BOOL read_lock)
{
#if LL_WINDOWS
    	
	return LLFile::_fsopen(filename, mode, (read_lock ? _SH_DENYWR : _SH_DENYRW));
    	
#else

	LLFILE *fp;
	int fd;
	
	// first test the lock in a non-destructive way
#if LL_SOLARIS
        struct flock fl;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 1;
#else // !LL_SOLARIS
	if (strchr(mode, 'w') != NULL)
	{
		fp = LLFile::fopen(filename, "rb");	/* Flawfinder: ignore */
		if (fp)
		{
			fd = fileno(fp);
			if (flock(fd, (read_lock ? LOCK_SH : LOCK_EX) | LOCK_NB) == -1)
			{
				fclose(fp);
				return NULL;
			}
		  
			fclose(fp);
		}
	}
#endif // !LL_SOLARIS

	// now actually open the file for use
	fp = LLFile::fopen(filename, mode);	/* Flawfinder: ignore */
	if (fp)
	{
		fd = fileno(fp);
#if LL_SOLARIS
                fl.l_type = read_lock ? F_RDLCK : F_WRLCK;
                if (fcntl(fd, F_SETLK, &fl) == -1)
#else
		if (flock(fd, (read_lock ? LOCK_SH : LOCK_EX) | LOCK_NB) == -1)
#endif
		{
			fclose(fp);
			fp = NULL;
		}
	}

	return fp;
    	
#endif
}
    
// static
void LLVFS::unlockAndClose(LLFILE *fp)
{
	if (fp)
	{
	// IW: we don't actually want to unlock on linux
	// this is because a forked process can kill the parent's lock
	// with an explicit unlock
	// however, fclose() will implicitly remove the lock
	// but only once both parent and child have closed the file
    /*	
	  #if !LL_WINDOWS
	  int fd = fileno(fp);
	  flock(fd, LOCK_UN);
	  #endif
    */
#if LL_SOLARIS
	        struct flock fl;
		fl.l_whence = SEEK_SET;
		fl.l_start = 0;
		fl.l_len = 1;
		fl.l_type = F_UNLCK;
		fcntl(fileno(fp), F_SETLK, &fl);
#endif
		fclose(fp);
	}
}
