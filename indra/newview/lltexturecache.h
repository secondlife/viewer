/** 
 * @file lltexturecache.h
 * @brief Object for managing texture cache.
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

#ifndef LL_LLTEXTURECACHE_H
#define LL_LLTEXTURECACHE_H

#include "lldir.h"
#include "llstl.h"
#include "llstring.h"
#include "lluuid.h"
#include "llmutex.h"
#include "llpointer.h"
#include "llimage.h"

struct CachedTextureInfo
{
    LLUUID  mID;
    S8      mCodec;
    U32     mImageEncodedSize;
    U32     mDiscardLevel;
    U32     mCachedWidth;
    U32     mCachedHeight;
    U32     mFullWidth;
    U32     mFullHeight;
    U32     mLastAccess; // epoch time for LRU
};

class LLTextureCache
{
public:
    static const std::string CACHE_ENTRY_ID;
    static const std::string CACHE_ENTRY_CODEC;
    static const std::string CACHE_ENTRY_ENCODED_SIZE;
    static const std::string CACHE_ENTRY_DISCARD_LEVEL;
    static const std::string CACHE_ENTRY_CACHED_WIDTH;
    static const std::string CACHE_ENTRY_CACHED_HEIGHT;
    static const std::string CACHE_ENTRY_FULL_WIDTH;
    static const std::string CACHE_ENTRY_FULL_HEIGHT;

	LLTextureCache();
	~LLTextureCache();

    bool initCache(ELLPath loc, bool purgeCache);

	bool                        add(const LLUUID& id, LLImageFormatted* rawimage);
    bool                        remove(const LLUUID& id);
	LLPointer<LLImageFormatted> find(const LLUUID& id);

    void purge();

    U32 getEntryCount();
    U64 getMaxUsage();
    U64 getUsage();

    bool writeCacheContentsFile(bool force_immediate_write);
    bool readCacheContentsFile();

private:
    typedef std::map<LLUUID, CachedTextureInfo> map_t;

	std::string getTextureFileName(const LLUUID& id, EImageCodec codec);

    bool evict(U64 spaceRequired);
	
    LLMutex     mMutex;
    ELLPath     mCacheLoc;
    std::string mTexturesDirName;
	map_t       mMap;
    U32         mAddedEntries = 0;
    U64         mUsageMax = 0; // 0 -> unlimited
    U64         mUsage    = 0;
};
#endif // LL_LLTEXTURECACHE_H
