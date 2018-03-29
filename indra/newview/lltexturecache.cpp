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
#include "llimage.h"
#include "llsdserialize.h"
#include "llviewercontrol.h"

// This controls how often we update the on-disk data for the cache
// with contents of the in-memory map of cached texture data.
static const U32 NEW_ENTRIES_PER_FLUSH_TO_DISK = 4;

const std::string LLTextureCache::CACHE_ENTRY_ID("id");
const std::string LLTextureCache::CACHE_ENTRY_CODEC("codec");
const std::string LLTextureCache::CACHE_ENTRY_ENCODED_SIZE("size");
const std::string LLTextureCache::CACHE_ENTRY_DISCARD_LEVEL("discard");
const std::string LLTextureCache::CACHE_ENTRY_CACHED_WIDTH("cached_width");
const std::string LLTextureCache::CACHE_ENTRY_CACHED_HEIGHT("cached_height");
const std::string LLTextureCache::CACHE_ENTRY_FULL_WIDTH("full_width");
const std::string LLTextureCache::CACHE_ENTRY_FULL_HEIGHT("full_height");

const std::string TEXTURE_CACHE_DIR_NAME("texturecache");

LLTextureCache::LLTextureCache()
{
}

LLTextureCache::~LLTextureCache()
{
	writeCacheContentsFile(true);
}

bool LLTextureCache::initCache(ELLPath loc, bool purgeCache)
{
    mCacheLoc        = loc;
    mTexturesDirName = gDirUtilp->getExpandedFilename(loc, TEXTURE_CACHE_DIR_NAME);
	
    LLFile::mkdir(mTexturesDirName);

    if (purgeCache)
    {
        purge();
        return true;
    }

    // Convert settings in MB to bytes
    U32 cacheSizeMB = gSavedSettings.getU32("CacheSize");

    mUsageMax = U64(cacheSizeMB) << 20;

    return readCacheContentsFile();
}

U32 LLTextureCache::getEntryCount()
{
    return mMap.size();
}

U64 LLTextureCache::getMaxUsage()
{
    return mUsageMax ? mUsageMax : (1 << 28);
}

U64 LLTextureCache::getUsage()
{
    return mUsage;
}

bool LLTextureCache::add(const LLUUID& id, LLImageFormatted* image)
{
    {
	    LLMutexLock lock(&mMutex);

#if LL_DEBUG
        map_t::iterator iter = mMap.find(id);
        // Insure that this is a unique addition or the added image is exactly the same
        llassert((iter == mMap.end())
            || (iter->second.mCachedHeight == image->getHeight()
             && iter->second.mCachedWidth == image->getWidth()
             && iter->second.mDiscardLevel == image->getDiscardLevel()
             && iter->second.mFullHeight == image->getFullHeight()
             && iter->second.mFullWidth == image->getFullWidth()
             && iter->second.mImageEncodedSize == image->getDataSize()
             && iter->second.mCodec == image->getCodec()));
#endif

        CachedTextureInfo info;
        info.mID                = id;
        info.mCachedHeight      = image->getHeight();
        info.mCachedWidth       = image->getWidth();
        info.mDiscardLevel      = image->getDiscardLevel();
        info.mFullHeight        = image->getFullHeight();
        info.mFullWidth         = image->getFullWidth();
        info.mImageEncodedSize  = image->getDataSize();
        info.mCodec             = image->getCodec();

        while (mUsageMax && ((mUsage + info.mImageEncodedSize) > mUsageMax))
        {
            if (!evict(info.mImageEncodedSize))
            {
                LL_WARNS() << "Failed to evict textures from cache. Exceeding max usage." << LL_ENDL;
                break;
            }
        }

        mMap.insert(std::make_pair(id, info));

        mUsage += info.mImageEncodedSize;
    }

    std::string filename = getTextureFileName(id, EImageCodec(image->getCodec()));

    LLFILE* f = LLFile::fopen(filename, "wb");
    if (!f)
    {
        LL_WARNS() << "Failed to write complete file while caching texture " << filename << LL_ENDL;
        return false;
    }

	S32 bytes_written = fwrite((const char*)image->getData(), 1, image->getDataSize(), f);

    fclose(f);

    if (bytes_written != image->getDataSize())
    {
        LL_WARNS() << "Failed to write complete file while caching texture." << LL_ENDL;
        return false;
    }

    ++mAddedEntries;

    return true;
}

bool LLTextureCache::evict(U64 spaceRequired)
{
    std::vector<LLUUID> evictees;
    U64 spaceReclaimed = 0;
    for (map_t::const_reverse_iterator iter = mMap.rbegin(); iter != mMap.rend(); iter++)
	{
        const CachedTextureInfo& info = iter->second;
        evictees.push_back(iter->first);
        spaceReclaimed += info.mImageEncodedSize;
        if (spaceReclaimed >= spaceRequired)
        {
            break;
        }
    }

    for (LLUUID& id : evictees)
    {
        map_t::iterator iter = mMap.find(id);
        if (iter != mMap.end())
        {
            mUsage -= (mUsage > iter->second.mImageEncodedSize) ? (mUsage - iter->second.mImageEncodedSize) : 0;
        }
        mMap.erase(id);
    }

    return ((mUsage + spaceRequired) < mUsageMax);
}

bool LLTextureCache::remove(const LLUUID& id)
{
    LLMutexLock lock(&mMutex);

    map_t::iterator iter = mMap.find(id);
    if (iter != mMap.end())
    {
        mUsage -= (mUsage > iter->second.mImageEncodedSize) ? (mUsage - iter->second.mImageEncodedSize) : 0;
        mMap.erase(id);
    }
    
    return true;
}

LLPointer<LLImageFormatted> LLTextureCache::find(const LLUUID& id)
{
    LLMutexLock lock(&mMutex);
    map_t::iterator iter = mMap.find(id);    
    if (iter == mMap.end())
    {
        // no texture here by that name...
        return LLPointer<LLImageFormatted>();
    }

    CachedTextureInfo& info = iter->second;

    char* data = ALLOCATE_MEM(LLImageBase::getPrivatePool(), info.mImageEncodedSize);

    std::string filename = getTextureFileName(id, EImageCodec(info.mCodec));
    
    LLFILE* f = LLFile::fopen(filename, "rb");
    if (!f)
    {
        LL_WARNS() << "Failed to open cached texture " << id << " removing from cache." << LL_ENDL;
        remove(id);
        mAddedEntries++;
        return LLPointer<LLImageFormatted>();
    }

    size_t bytes_read = fread(data, 1, info.mImageEncodedSize, f);

    fclose(f);

    if (bytes_read != info.mImageEncodedSize)
    {
        LL_WARNS() << "Failed to read cached texture " << id << " removing from cache." << LL_ENDL;
        FREE_MEM(LLImageBase::getPrivatePool(), data);
        remove(id);
        mAddedEntries++;
        return LLPointer<LLImageFormatted>();
    }

	LLPointer<LLImageFormatted> image = LLImageFormatted::createFromType(info.mCodec);
    image->setData((U8*)data, bytes_read);
    if (!image->updateData())
    {
        LL_WARNS() << "Failed to parse header data from cached texture " << id << " removing from cache." << LL_ENDL;
        remove(id);
        mAddedEntries++;
        return LLPointer<LLImageFormatted>();
    }

    return image;
}

void LLTextureCache::purge()
{
    LLMutexLock lock(&mMutex);

    gDirUtilp->deleteDirAndContents(mTexturesDirName);
    LLFile::rmdir(mTexturesDirName);
	mMap.clear();
	LL_INFOS() << "The entire texture cache is cleared." << LL_ENDL;
}

std::string LLTextureCache::getTextureFileName(const LLUUID& id, EImageCodec codec)
{
	std::string idstr = id.asString();
	std::string delem = gDirUtilp->getDirDelimiter();
    std::string ext   = LLImageBase::getExtensionFromCodec(codec);
	std::string filename = mTexturesDirName + delem + idstr + ext;
	return filename;
}

bool LLTextureCache::writeCacheContentsFile(bool force_immediate_write)
{
    if (!force_immediate_write && (mAddedEntries < NEW_ENTRIES_PER_FLUSH_TO_DISK))
    {
        return false;
    }

    LLSD contents;
    S32 entries = 0;

    {
        LLMutexLock lock(&mMutex);        
        for (map_t::const_iterator iter = mMap.begin(); iter != mMap.end(); iter++)
	    {
            const CachedTextureInfo& info = iter->second;
            LLSD entry;
            entry[CACHE_ENTRY_ID]            = info.mID;
            entry[CACHE_ENTRY_CODEC]         = LLSD::Integer(info.mCodec);
            entry[CACHE_ENTRY_ENCODED_SIZE]  = LLSD::Integer(info.mImageEncodedSize);
            entry[CACHE_ENTRY_DISCARD_LEVEL] = LLSD::Integer(info.mDiscardLevel);
            entry[CACHE_ENTRY_CACHED_WIDTH]  = LLSD::Integer(info.mCachedWidth);
            entry[CACHE_ENTRY_CACHED_HEIGHT] = LLSD::Integer(info.mCachedHeight);
            entry[CACHE_ENTRY_FULL_WIDTH]    = LLSD::Integer(info.mFullWidth);
            entry[CACHE_ENTRY_FULL_HEIGHT]   = LLSD::Integer(info.mFullHeight);
            contents[info.mID.asString()] = entry;
            entries++;
        }
    }

    if (!contents.size())
    {
        LL_WARNS() << "No texture cache contents file to write." << LL_ENDL;
        return false;
    }

    std::string filename = mTexturesDirName + ".contents";
    llofstream out;
    out.open(filename);
    if (!out.is_open())
    {
        LL_WARNS() << "Could not open texture cache contents file for write." << LL_ENDL;
        return false;
    }

    S32 entriesSerialized = LLSDSerialize::toPrettyNotation(contents, out);

    out.close();

    if (!entriesSerialized)
    {
        LL_WARNS() << "Failed to serialize texture cache content entries." << LL_ENDL;
        return false;
    }

    mAddedEntries = 0;

    return true;
}

// Called from either the main thread or the worker thread
bool LLTextureCache::readCacheContentsFile()
{
	LLMutexLock lock(&mMutex);

    std::string filename = mTexturesDirName + ".contents";
    
	llifstream file;
	file.open(filename.c_str());

	if (!file.is_open())
	{
        LL_WARNS() << "Could not open texture cache contents file." << LL_ENDL;
		return false;
	}

    LLSD llsd;
	LLSDSerialize::fromNotation(llsd, file, (1 << 29));

    if (!llsd.size())
    {
        LL_WARNS() << "No data in texture cache contents file." << LL_ENDL;
		return false;
	}

    for (LLSD::map_const_iterator iter = llsd.beginMap(); iter != llsd.endMap(); ++iter)
    {
        CachedTextureInfo info;
        info.mID                = iter->second[CACHE_ENTRY_ID].asUUID();
        info.mImageEncodedSize  = iter->second[CACHE_ENTRY_ENCODED_SIZE].asInteger();
        info.mCodec             = S8(iter->second[CACHE_ENTRY_CODEC].asInteger() & 0xFF);
        info.mCachedHeight      = iter->second[CACHE_ENTRY_CACHED_HEIGHT].asInteger();
        info.mCachedWidth       = iter->second[CACHE_ENTRY_CACHED_WIDTH].asInteger();
        info.mFullHeight        = iter->second[CACHE_ENTRY_FULL_WIDTH].asInteger();
        info.mFullWidth         = iter->second[CACHE_ENTRY_FULL_HEIGHT].asInteger();
        info.mDiscardLevel      = iter->second[CACHE_ENTRY_DISCARD_LEVEL].asInteger();

        if (info.mID.isNull())
        {
            continue;
        }

        bool validCodec = false;
        switch(info.mCodec)
        {
            default:
            case IMG_CODEC_INVALID:
            case IMG_CODEC_EOF:
                break;

	        case IMG_CODEC_RGB:
	        case IMG_CODEC_J2C:
	        case IMG_CODEC_BMP:
	        case IMG_CODEC_TGA:
	        case IMG_CODEC_JPEG:
	        case IMG_CODEC_DXT:
	        case IMG_CODEC_PNG:
                validCodec = true;
            break;
        }

        if ((info.mCachedHeight > 2048)
         || (info.mCachedWidth > 2048)
         || (info.mFullHeight > 2048)
         || (info.mFullWidth > 2048))
        {
            continue;
        }

        if (!validCodec)
        {
            continue;
        }

        if (info.mDiscardLevel > MAX_DISCARD_LEVEL)
        {
            continue;
        }

        if (info.mImageEncodedSize > (1 << 28))
        {
            continue;
        }

        mMap.insert(std::make_pair(info.mID, info));
    }

    return mMap.size() > 0;
}
