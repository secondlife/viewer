/** 
 * @file filesystem.h
 * @brief Simulate local file system operations.
 * @Note The initial implementation does actually use standard C++
 *       file operations but eventually, there will be another
 *       layer that caches and manages file meta data too.
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

#include "lldir.h"
#include "llfilesystem.h"
#include "llfasttimer.h"
#include "lldiskcache.h"

#include <fstream>

const S32 LLFileSystem::READ		= 0x00000001;
const S32 LLFileSystem::WRITE		= 0x00000002;
const S32 LLFileSystem::READ_WRITE	= 0x00000003;  // LLFileSystem::READ & LLFileSystem::WRITE
const S32 LLFileSystem::APPEND		= 0x00000006;  // 0x00000004 & LLFileSystem::WRITE

static LLTrace::BlockTimerStatHandle FTM_VFILE_WAIT("VFile Wait");
LLDiskCache* LLFileSystem::mDiskCache = 0;
std::string LLFileSystem::mCacheDirName = "cache";

LLFileSystem::LLFileSystem(const LLUUID &file_id, const LLAssetType::EType file_type, S32 mode)
{
	mFileType =	file_type;
	mFileID = file_id;
	mPosition = 0;
    mBytesRead = 0;
    mReadComplete = FALSE;
	mMode = mode;
}

LLFileSystem::~LLFileSystem()
{
}

const std::string assetTypeToString(LLAssetType::EType at)
{
    /**
     * Make use of the C++17 (or is it 14) feature that allows
     * for inline initialization of an std::map<>
     */
    typedef std::map<LLAssetType::EType, std::string> asset_type_to_name_t;
    asset_type_to_name_t asset_type_to_name =
    {
        { LLAssetType::AT_TEXTURE, "TEXTURE" },
        { LLAssetType::AT_SOUND, "SOUND" },
        { LLAssetType::AT_CALLINGCARD, "CALLINGCARD" },
        { LLAssetType::AT_LANDMARK, "LANDMARK" },
        { LLAssetType::AT_SCRIPT, "SCRIPT" },
        { LLAssetType::AT_CLOTHING, "CLOTHING" },
        { LLAssetType::AT_OBJECT, "OBJECT" },
        { LLAssetType::AT_NOTECARD, "NOTECARD" },
        { LLAssetType::AT_CATEGORY, "CATEGORY" },
        { LLAssetType::AT_LSL_TEXT, "LSL_TEXT" },
        { LLAssetType::AT_LSL_BYTECODE, "LSL_BYTECODE" },
        { LLAssetType::AT_TEXTURE_TGA, "TEXTURE_TGA" },
        { LLAssetType::AT_BODYPART, "BODYPART" },
        { LLAssetType::AT_SOUND_WAV, "SOUND_WAV" },
        { LLAssetType::AT_IMAGE_TGA, "IMAGE_TGA" },
        { LLAssetType::AT_IMAGE_JPEG, "IMAGE_JPEG" },
        { LLAssetType::AT_ANIMATION, "ANIMATION" },
        { LLAssetType::AT_GESTURE, "GESTURE" },
        { LLAssetType::AT_SIMSTATE, "SIMSTATE" },
        { LLAssetType::AT_LINK, "LINK" },
        { LLAssetType::AT_LINK_FOLDER, "LINK_FOLDER" },
        { LLAssetType::AT_MARKETPLACE_FOLDER, "MARKETPLACE_FOLDER" },
        { LLAssetType::AT_WIDGET, "WIDGET" },
        { LLAssetType::AT_PERSON, "PERSON" },
        { LLAssetType::AT_MESH, "MESH" },
        { LLAssetType::AT_SETTINGS, "SETTINGS" },
        { LLAssetType::AT_UNKNOWN, "UNKNOWN" }
    };

    asset_type_to_name_t::iterator iter = asset_type_to_name.find(at);
    if (iter != asset_type_to_name.end())
    {
        return iter->second;
    }

    return std::string("UNKNOWN");
}

const std::string LLFileSystem::idToFilepath(const std::string id, LLAssetType::EType at)
{
    /**
     * For the moment this is just {UUID}_{ASSET_TYPE}.txt but of
     * course,  will be greatly expanded upon
     */
    std::ostringstream ss;
    ss << "00cache_";
    ss << id;
    ss << "_";
    ss << assetTypeToString(at);
    ss << ".txt";

    const std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, mCacheDirName, ss.str());

    return filepath;
}

// static
bool LLFileSystem::getExists(const LLUUID &file_id, const LLAssetType::EType file_type)
{
	std::string id_str;
	file_id.toString(id_str);
	const std::string filename = idToFilepath(id_str, file_type);

	std::ifstream file(filename, std::ios::binary);
	if (file.is_open())
	{
		file.seekg(0, std::ios::end);
		return file.tellg() > 0;
	}
	return false;
}

// static
bool LLFileSystem::removeFile(const LLUUID &file_id, const LLAssetType::EType file_type)
{
    std::string id_str;
    file_id.toString(id_str);
    const std::string filename = idToFilepath(id_str, file_type);
    
    std::remove(filename.c_str());

    return true;
}

// static
bool LLFileSystem::renameFile(const LLUUID &old_file_id, const LLAssetType::EType old_file_type,
                         const LLUUID &new_file_id, const LLAssetType::EType new_file_type)
{
    std::string old_id_str;
    old_file_id.toString(old_id_str);
    const std::string old_filename = idToFilepath(old_id_str, old_file_type);

    std::string new_id_str;
    new_file_id.toString(new_id_str);
    const std::string new_filename = idToFilepath(new_id_str, new_file_type);

    if (std::rename(old_filename.c_str(), new_filename.c_str()))
    {
        // We would like to return FALSE here indicating the operation
        // failed but the original code does not and doing so seems to
        // break a lot of things so we go with the flow... 
        //return FALSE;
    }

    return TRUE;
}

// static
S32 LLFileSystem::getFileSize(const LLUUID &file_id, const LLAssetType::EType file_type)
{
    std::string id_str;
    file_id.toString(id_str);
    const std::string filename = idToFilepath(id_str, file_type);

    S32 file_size = 0;
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
    }

    return file_size;
}

BOOL LLFileSystem::read(U8 *buffer, S32 bytes, BOOL async, F32 priority)
{
	BOOL success = TRUE;

    mReadComplete = FALSE;

    std::string id;
    mFileID.toString(id);
    const std::string filename = idToFilepath(id, mFileType);

    std::ifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        file.seekg(mPosition, std::ios::beg);

        file.read((char*)buffer, bytes);

        if (file)
        {
            mBytesRead = bytes;
        }
        else
        {
            mBytesRead = file.gcount();
        }

        file.close();

        mPosition += mBytesRead;
        if (!mBytesRead)
        {
            success = FALSE;
        }

        mReadComplete = TRUE;
    }

    return success;
}

BOOL LLFileSystem::isReadComplete()
{
    if (mReadComplete)
    {
        return TRUE;
    }

    return FALSE;
}

S32 LLFileSystem::getLastBytesRead()
{
	return mBytesRead;
}

BOOL LLFileSystem::eof()
{
	return mPosition >= getSize();
}

BOOL LLFileSystem::write(const U8 *buffer, S32 bytes)
{
    std::string id_str;
    mFileID.toString(id_str);
    const std::string filename = idToFilepath(id_str, mFileType);

    BOOL success = FALSE;

    if (mMode == APPEND)
    {
        std::ofstream ofs(filename, std::ios::app | std::ios::binary);
        if (ofs)
        {
            ofs.write((const char*)buffer, bytes);

            success = TRUE;
        }
    }
    else
    {
        std::ofstream ofs(filename, std::ios::binary);
        if (ofs)
        {
            ofs.write((const char*)buffer, bytes);

            mPosition += bytes;

            success = TRUE;
        }
    }

    return success;
}

//static
BOOL LLFileSystem::writeFile(const U8 *buffer, S32 bytes, const LLUUID &uuid, LLAssetType::EType type)
{
	LLFileSystem file(uuid, type, LLFileSystem::WRITE);
	return file.write(buffer, bytes);
}

BOOL LLFileSystem::seek(S32 offset, S32 origin)
{
	if (-1 == origin)
	{
		origin = mPosition;
	}

	S32 new_pos = origin + offset;

	S32 size = getSize();

	if (new_pos > size)
	{
		LL_WARNS() << "Attempt to seek past end of file" << LL_ENDL;

		mPosition = size;
		return FALSE;
	}
	else if (new_pos < 0)
	{
		LL_WARNS() << "Attempt to seek past beginning of file" << LL_ENDL;

		mPosition = 0;
		return FALSE;
	}

	mPosition = new_pos;
	return TRUE;
}

S32 LLFileSystem::tell() const
{
	return mPosition;
}

S32 LLFileSystem::getSize()
{
    return LLFileSystem::getFileSize(mFileID, mFileType);
}

S32 LLFileSystem::getMaxSize()
{
    // offer up a huge size since we don't care what the max is 
    return INT_MAX;
}

BOOL LLFileSystem::rename(const LLUUID &new_id, const LLAssetType::EType new_type)
{
    LLFileSystem::renameFile(mFileID, mFileType, new_id, new_type);

    mFileID = new_id;
    mFileType = new_type;

    return TRUE;
}

BOOL LLFileSystem::remove()
{
    LLFileSystem::removeFile(mFileID, mFileType);

    return TRUE;
}

// static
void LLFileSystem::initClass()
{
    const std::string cache_dir = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, mCacheDirName);

    LLFileSystem::mDiskCache = new LLDiskCache(cache_dir);

    mDiskCache->purge();
}

// static
void LLFileSystem::cleanupClass()
{
    delete LLFileSystem::mDiskCache;
}
