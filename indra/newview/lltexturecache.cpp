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
#include "llimage.h"
#include "llimagej2c.h"
#include "llsdserialize.h"
#include "llviewercontrol.h"
#include "llcoros.h"
#include "llsdutil.h"

//=====================================================================
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_ID("id");
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_CODEC("codec");
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_ENCODED_SIZE("size");
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_DISCARD_LEVEL("discard");
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_CACHED_WIDTH("cached_width");
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_CACHED_HEIGHT("cached_height");
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_FULL_WIDTH("full_width");
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_FULL_HEIGHT("full_height");
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_LAST_ACCESS("last_access");
const std::string LLTextureCache::CachedTextureInfo::CACHE_ENTRY_DISCARD_BYTES("discard_bytes");

//=====================================================================
namespace
{
    // This controls how often we update the on-disk data for the cache
    // with contents of the in-memory map of cached texture data.
    const U32 NEW_ENTRIES_PER_FLUSH_TO_DISK = 10;
    const F32 TIME_PER_FLUSH_TO_DISK = 30.0;
    LLTrace::BlockTimerStatHandle FTM_TEXTURE_CACHE_IO("TexCache IO");

    const std::string TEXTURE_CACHE_DIR_NAME("texturecache");
}

//=====================================================================
LLTextureCache::CachedTextureInfo::CachedTextureInfo():
    mID(),
    mCodec(0),
    mImageEncodedSize(0),
    mDiscardLevel(0),
    mCachedWidth(0),
    mCachedHeight(0),
    mFullWidth(0),
    mFullHeight(0),
    mLastAccess(time(nullptr)),
    mDiscardBytes()
{
    mDiscardBytes.fill(0);
}

LLTextureCache::CachedTextureInfo::CachedTextureInfo(LLUUID id, LLImageFormatted* image):
    mID(),
    mCodec(0),
    mImageEncodedSize(0),
    mDiscardLevel(0),
    mCachedWidth(0),
    mCachedHeight(0),
    mFullWidth(0),
    mFullHeight(0),
    mLastAccess(time(nullptr)),
    mDiscardBytes()
{
    mDiscardBytes.fill(0);
    fromImage(id, image);
}


LLSD LLTextureCache::CachedTextureInfo::toLLSD() const
{
    LLSD data(LLSDMap(CACHE_ENTRY_ID, mID)
        (CACHE_ENTRY_CODEC, LLSD::Integer(mCodec))
        (CACHE_ENTRY_ENCODED_SIZE, LLSD::Integer(mImageEncodedSize))
        (CACHE_ENTRY_DISCARD_LEVEL, LLSD::Integer(mDiscardLevel))
        (CACHE_ENTRY_CACHED_WIDTH, LLSD::Integer(mCachedWidth))
        (CACHE_ENTRY_CACHED_HEIGHT, LLSD::Integer(mCachedHeight))
        (CACHE_ENTRY_FULL_WIDTH, LLSD::Integer(mFullWidth))
        (CACHE_ENTRY_FULL_HEIGHT, LLSD::Integer(mFullHeight))
        (CACHE_ENTRY_LAST_ACCESS, LLSD::Integer(mLastAccess)));

    for (U32 index = 0; index < MAX_DISCARD_LEVEL; index++)
    {
        data[CACHE_ENTRY_DISCARD_BYTES][index] = LLSD::Integer(mDiscardBytes[index]);
    }

    return data;
}

void LLTextureCache::CachedTextureInfo::fromLLSD(LLSD data)
{

    mID = data[CACHE_ENTRY_ID].asUUID();
    mImageEncodedSize = data[CACHE_ENTRY_ENCODED_SIZE].asInteger();
    mCodec =            S8(data[CACHE_ENTRY_CODEC].asInteger() & 0xFF);
    mCachedHeight =     data[CACHE_ENTRY_CACHED_HEIGHT].asInteger();
    mCachedWidth =      data[CACHE_ENTRY_CACHED_WIDTH].asInteger();
    mFullHeight =       data[CACHE_ENTRY_FULL_WIDTH].asInteger();
    mFullWidth =        data[CACHE_ENTRY_FULL_HEIGHT].asInteger();
    mDiscardLevel =     data[CACHE_ENTRY_DISCARD_LEVEL].asInteger();
    mLastAccess =       data[CACHE_ENTRY_LAST_ACCESS].asInteger();

    U32 index(0);
    for (LLSD::array_const_iterator iter = data[CachedTextureInfo::CACHE_ENTRY_DISCARD_BYTES].beginArray();
        (index < MAX_DISCARD_LEVEL) && (iter != data[CachedTextureInfo::CACHE_ENTRY_DISCARD_BYTES].endArray());
        ++iter, ++index)
    {
        mDiscardBytes[index] = iter->asInteger();
    }

}

void LLTextureCache::CachedTextureInfo::fromImage(LLUUID id, LLImageFormatted* image)
{
    mID = id;
    mCachedHeight = image->getHeight();
    mCachedWidth = image->getWidth();
    mDiscardLevel = image->getDiscardLevel();
    mFullHeight = image->getFullHeight();
    mFullWidth = image->getFullWidth();
    mImageEncodedSize = image->getDataSize();
    mCodec = image->getCodec();

    if (image->getCodec() == IMG_CODEC_J2C)
    {
        S32* dataSizes = ((LLImageJ2C*)image)->getDataSizes();
        for (U32 index = 1; index < MAX_DISCARD_LEVEL; index++)
        {
            mDiscardBytes[index] = dataSizes[index];
        }
        if (image->getDiscardLevel() == 0)
        {
            mDiscardBytes[0] = image->getDataSize();
        }
        else
        {
            mDiscardBytes[0] = dataSizes[0];
        }
    }
    else
    {
        for (U32 index = 1; index < MAX_DISCARD_LEVEL; index++)
        {
            mDiscardBytes[index] = image->getDataSize();
        }
    }

    updateDiscards(image);
}

void LLTextureCache::CachedTextureInfo::updateDiscards(LLImageFormatted *image)
{
    if (image->getCodec() == IMG_CODEC_J2C)
    {
        S32* dataSizes = ((LLImageJ2C*)image)->getDataSizes();
        for (U32 index = 1; index < MAX_DISCARD_LEVEL; index++)
        {
            mDiscardBytes[index] = dataSizes[index];
        }
        if (image->getDiscardLevel() == 0)
        {
            mDiscardBytes[0] = image->getDataSize();
        }
        else
        {
            mDiscardBytes[0] = dataSizes[0];
        }
    }
    else
    {
        for (U32 index = 1; index < MAX_DISCARD_LEVEL; index++)
        {
            mDiscardBytes[index] = image->getDataSize();
        }
    }

    mLastAccess = time(nullptr);
}

//=====================================================================
LLTextureCache::~LLTextureCache()
{
}

void LLTextureCache::cleanupSingleton() 
{
    LL_INFOS("TEXTURECACHE") << "shutting down texture cache." << LL_ENDL;
    writeCacheContentsFile(true);
}


bool LLTextureCache::initCache(ELLPath loc, bool purgeCache)
{
    mCacheLoc        = loc;
    mTexturesDirName = gDirUtilp->getExpandedFilename(loc, TEXTURE_CACHE_DIR_NAME);
	
    LLFile::mkdir(mTexturesDirName);

    // Convert settings in MB to bytes
    U32 cacheSizeMB = gSavedSettings.getU32("CacheSize");

    mUsageMax = U64(cacheSizeMB) << 20;

    if (purgeCache)
    {
        purge();
        return true;
    }

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
    // only write texture to disk if it's better than what we've cached or it's new
    bool write_to_disk = false;

    {
	    LLMutexLock lock(&mMutex);

        CachedTextureInfo::ptr_t info;

        map_t::iterator iter = mMap.find(id);

        if(iter != mMap.end())
        {
            LL_DEBUGS("TEXTURECACHE") << "Updating texture in cache with ID=" << id << LL_ENDL;
            info = iter->second;

            // got a better discard, record its info...
            if (image->getDiscardLevel() < info->mDiscardLevel)
            {
                write_to_disk = true;
                info->fromImage(id, image);
            }
            else
            {
                info->updateDiscards(image);
            }
        }
        else // new addition
        {
            LL_DEBUGS("TEXTURECACHE") << "Adding new texture to cache with ID=" << id << LL_ENDL;
            info = std::make_shared<CachedTextureInfo>(id, image);
            write_to_disk = true;
        }


        LLSD entry = info->toLLSD();

        mEntries[info->mID.asString()] = entry;

        U32 iterations = 0;
        while (mUsageMax && ((mUsage + info->mImageEncodedSize) > mUsageMax) && (iterations++ < 256))
        {
            if (!evict(info->mImageEncodedSize))
            {
                LL_WARNS("TEXTURECACHE") << "Failed to evict textures from cache. Exceeding max usage." << LL_ENDL;
                break;
            }
        }

        mMap.insert(std::make_pair(id, info));

        mUsage += info->mImageEncodedSize;
    }

    if (write_to_disk)
    {
        std::string filename = getTextureFileName(id, EImageCodec(image->getCodec()));

        LLFILE* f = LLFile::fopen(filename, "wb");
        if (!f)
        {
            LL_WARNS("TEXTURECACHE") << "Failed to write complete file while caching texture " << filename << LL_ENDL;
            return false;
        }

	    S32 bytes_written = fwrite((const char*)image->getData(), 1, image->getDataSize(), f);

        fclose(f);

        if (bytes_written != image->getDataSize())
        {
            LL_WARNS("TEXTURECACHE") << "Failed to write complete file while caching texture." << LL_ENDL;
            return false;
        }
    }

    ++mAddedEntries;

    return true;
}

bool LLTextureCache::evict(U64 spaceRequired)
{
    // put the cache into a a vector.
    std::vector<CachedTextureInfo::ptr_t> potential_evictees; 
    std::vector<LLUUID> evictees;
    U64 spaceReclaimed( (mUsage < mUsageMax ) ? (mUsageMax - mUsage) : 0 ); // count existing free space towards reclaimed.

    potential_evictees.reserve(mMap.size());

    for (const auto &value : mMap)
    {   // C++17: In C++17 I could do something cool like const auto &[key, entry]... 
        potential_evictees.push_back(value.second);
    }

    // sort potential_evictees to get least recently used items with largest size at front
    std::sort(potential_evictees.begin(), potential_evictees.end(), 
            [](const CachedTextureInfo::ptr_t &i1, const CachedTextureInfo::ptr_t &i2)
            {
                if (i1->mLastAccess < i2->mLastAccess)
                {
                    return true;
                }
                else if (i1->mLastAccess == i2->mLastAccess)
                {
                    if (i1->mImageEncodedSize > i2->mImageEncodedSize)
                    {
                        return true;
                    }
                }
                return false;
            });

    for (const CachedTextureInfo::ptr_t &evictee : potential_evictees) 
	{
        if (spaceReclaimed >= spaceRequired)
            break;
        map_t::iterator it = mMap.find(evictee->mID);
        if (it != mMap.end())
        {
            mUsage = (mUsage > evictee->mImageEncodedSize) ? (mUsage - evictee->mImageEncodedSize) : 0;
            spaceReclaimed += evictee->mImageEncodedSize;

            mMap.erase(it);
            mEntries.erase(evictee->mID.asString());
            ++mAddedEntries;
        }
    }
    potential_evictees.clear();

    LL_WARNS("TEXTURECACHE") << "Reclaim from texture cache.  Desired:" << spaceRequired << " reclaimed:" << spaceReclaimed << LL_ENDL;

    return ((mUsage + spaceRequired) < mUsageMax);
}

bool LLTextureCache::remove(const LLUUID& id)
{
    LLMutexLock lock(&mMutex);

    map_t::iterator iter = mMap.find(id);
    if (iter != mMap.end())
    {
        LL_DEBUGS("TEXTURECACHE") << "remove ID=" << id << " from texture cache." << LL_ENDL;
        mUsage = (mUsage > iter->second->mImageEncodedSize) ? (mUsage - iter->second->mImageEncodedSize) : 0;
        mMap.erase(id);
        mEntries.erase(id.asString());
        mAddedEntries++; // signal requirement to update on-disk rep to capture removed entry
    }
    
    return true;
}

LLPointer<LLImageFormatted> LLTextureCache::find(const LLUUID& id, S32 discard_level)
{
    LLMutexLock lock(&mMutex);
    map_t::iterator iter = mMap.find(id);    
    if (iter == mMap.end())
    {
        // no texture here by that name...
        return LLPointer<LLImageFormatted>();
    }

    CachedTextureInfo::ptr_t info = iter->second;

    U8* data = (U8*)ll_aligned_malloc_16(info->mImageEncodedSize);

    std::string filename = getTextureFileName(id, EImageCodec(info->mCodec));
    
    LLFILE* f = LLFile::fopen(filename, "rb");
    if (!f)
    {
        LL_WARNS("TEXTURECACHE") << "Failed to open cached texture " << id << " removing from cache." << LL_ENDL;
        remove(id);        
        return LLPointer<LLImageFormatted>();
    }

    size_t bytes_cached = info->mDiscardBytes[discard_level];
    size_t bytes_read   = fread(data, 1, bytes_cached, f);

    fclose(f);

    LLPointer<LLImageFormatted> image;

    if (info->mCodec == IMG_CODEC_J2C)
    {
        image = LLImageFormatted::createFromTypeWithImpl(info->mCodec, gSavedSettings.getS32("JpegDecoderType"));
    }
    else
    {
		image = LLImageFormatted::createFromType(info->mCodec);
    }

    if (bytes_read)
    {	
        image->setData((U8*)data, bytes_read);
        if (!image->updateData())
        {
            LL_WARNS("TEXTURECACHE") << "Failed to parse header data from cached texture " << id << " removing from cache." << LL_ENDL;
            remove(id);
            return LLPointer<LLImageFormatted>();
        }

        info->mLastAccess = time(nullptr);
    }
    else
    {
        LL_WARNS("TEXTURECACHE") << "Failed to read cached texture " << id << " removing from cache." << LL_ENDL;
        ll_aligned_free_16(data);
        remove(id);
    }

    return image;
}

void LLTextureCache::purge()
{
    LLMutexLock lock(&mMutex);

    gDirUtilp->deleteDirAndContents(mTexturesDirName);
    LLFile::rmdir(mTexturesDirName);
    // recreate this dir so subsequent caching to local disk works...
    LLFile::mkdir(mTexturesDirName);
	mMap.clear();    
    LL_INFOS("TEXTURECACHE") << "The entire texture cache is cleared." << LL_ENDL;
}

std::string LLTextureCache::getTextureFileName(const LLUUID& id, EImageCodec codec)
{
	std::string idstr = id.asString();
	std::string delem = gDirUtilp->getDirDelimiter();
    const std::string& ext   = LLImageBase::getExtensionFromCodec(codec);
	std::string filename = mTexturesDirName + delem + idstr + ext;
	return filename;
}

bool LLTextureCache::writeCacheContentsFile(bool force_immediate_write)
{
    LL_DEBUGS("TEXTURECACHE") << "Writing texture cache." << LL_ENDL;
    try
    {
        //LL_RECORD_BLOCK_TIME(FTM_TEXTURE_CACHE_IO);

        if (!mEntries.size())
        {
            LL_WARNS("TEXTURECACHE") << "No texture cache contents file to write." << LL_ENDL;
            return false;
        }

        std::string filename = mTexturesDirName + ".contents";
        llofstream out;
        out.open(filename);
        if (!out.is_open())
        {
            LL_WARNS("TEXTURECACHE") << "Could not open texture cache contents file for write." << LL_ENDL;
            return false;
        }

        S32 entriesSerialized = LLSDSerialize::toPrettyNotation(mEntries, out);

        out.close();

        if (!entriesSerialized)
        {
            LL_WARNS("TEXTURECACHE") << "Failed to serialize texture cache content entries." << LL_ENDL;
            return false;
        }

        mAddedEntries = 0;
    }
    catch (const std::exception& e)
    {
        LL_WARNS("TEXTURECACHE") << "Caught exception writing cache file: \"" << e.what() << "\"" << LL_ENDL;
        llassert(false);
    }

        return true;
}

bool LLTextureCache::updateCacheContentsFile(bool force_immediate_write)
{
    static LLTimer sFlushTimeout;

    if (!force_immediate_write)
    {
        if ((sFlushTimeout.getElapsedTimeF32() < TIME_PER_FLUSH_TO_DISK) || (mAddedEntries < NEW_ENTRIES_PER_FLUSH_TO_DISK))
            return false;
    }
    sFlushTimeout.reset();

    std::string name = LLCoros::instance().launch("TexCacheWrite", boost::bind(&LLTextureCache::writeCacheContentsFile, this, force_immediate_write));

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

	LLSDSerialize::fromNotation(mEntries, file, (1 << 29));

    if (!mEntries.size())
    {
        LL_WARNS() << "No data in texture cache contents file." << LL_ENDL;
		return false;
	}

    for (LLSD::map_const_iterator iter = mEntries.beginMap(); iter != mEntries.endMap(); ++iter)
    {
        CachedTextureInfo::ptr_t info(std::make_shared<CachedTextureInfo>());
        info->fromLLSD(iter->second);

        if (info->mID.isNull())
        {
            continue;
        }

        bool validCodec = false;
        switch(info->mCodec)
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

        if ((info->mCachedHeight > 2048)
         || (info->mCachedWidth > 2048)
         || (info->mFullHeight > 2048)
         || (info->mFullWidth > 2048))
        {
            continue;
        }

        if (!validCodec)
        {
            continue;
        }

        if (info->mDiscardLevel > MAX_DISCARD_LEVEL)
        {
            continue;
        }

        if (info->mImageEncodedSize > (1 << 28))
        {
            continue;
        }

        mMap.insert(std::make_pair(info->mID, info));
    }

    return mMap.size() > 0;
}
