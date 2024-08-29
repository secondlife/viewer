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

const S32 LLFileSystem::READ        = 0x00000001;
const S32 LLFileSystem::WRITE       = 0x00000002;
const S32 LLFileSystem::READ_WRITE  = 0x00000003;  // LLFileSystem::READ & LLFileSystem::WRITE
const S32 LLFileSystem::APPEND      = 0x00000006;  // 0x00000004 & LLFileSystem::WRITE

static LLTrace::BlockTimerStatHandle FTM_VFILE_WAIT("VFile Wait");

LLFileSystem::LLFileSystem(const LLUUID& file_id, const LLAssetType::EType file_type, S32 mode)
{
    mFileType = file_type;
    mFileID = file_id;
    mPosition = 0;
    mBytesRead = 0;
    mMode = mode;

    // This block of code was originally called in the read() method but after comments here:
    // https://bitbucket.org/lindenlab/viewer/commits/e28c1b46e9944f0215a13cab8ee7dded88d7fc90#comment-10537114
    // we decided to follow Henri's suggestion and move the code to update the last access time here.
    if (mode == LLFileSystem::READ)
    {
        // build the filename (TODO: we do this in a few places - perhaps we should factor into a single function)
        std::string id;
        mFileID.toString(id);
        const std::string extra_info = "";
        const std::string filename = LLDiskCache::getInstance()->metaDataToFilepath(id, mFileType, extra_info);

        // update the last access time for the file if it exists - this is required
        // even though we are reading and not writing because this is the
        // way the cache works - it relies on a valid "last accessed time" for
        // each file so it knows how to remove the oldest, unused files
        bool exists = gDirUtilp->fileExists(filename);
        if (exists)
        {
            LLDiskCache::getInstance()->updateFileAccessTime(filename);
        }
    }
}

LLFileSystem::~LLFileSystem()
{
}

// static
bool LLFileSystem::getExists(const LLUUID& file_id, const LLAssetType::EType file_type)
{
    std::string id_str;
    file_id.toString(id_str);
    const std::string extra_info = "";
    const std::string filename = LLDiskCache::getInstance()->metaDataToFilepath(id_str, file_type, extra_info);

    llifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        return file.tellg() > 0;
    }
    return false;
}

// static
bool LLFileSystem::removeFile(const LLUUID& file_id, const LLAssetType::EType file_type, int suppress_error /*= 0*/)
{
    std::string id_str;
    file_id.toString(id_str);
    const std::string extra_info = "";
    const std::string filename =  LLDiskCache::getInstance()->metaDataToFilepath(id_str, file_type, extra_info);

    LLFile::remove(filename.c_str(), suppress_error);

    return true;
}

// static
bool LLFileSystem::renameFile(const LLUUID& old_file_id, const LLAssetType::EType old_file_type,
                              const LLUUID& new_file_id, const LLAssetType::EType new_file_type)
{
    std::string old_id_str;
    old_file_id.toString(old_id_str);
    const std::string extra_info = "";
    const std::string old_filename =  LLDiskCache::getInstance()->metaDataToFilepath(old_id_str, old_file_type, extra_info);

    std::string new_id_str;
    new_file_id.toString(new_id_str);
    const std::string new_filename =  LLDiskCache::getInstance()->metaDataToFilepath(new_id_str, new_file_type, extra_info);

    // Rename needs the new file to not exist.
    LLFileSystem::removeFile(new_file_id, new_file_type, ENOENT);

    if (LLFile::rename(old_filename, new_filename) != 0)
    {
        // We would like to return false here indicating the operation
        // failed but the original code does not and doing so seems to
        // break a lot of things so we go with the flow...
        //return false;
        LL_WARNS() << "Failed to rename " << old_file_id << " to " << new_id_str << " reason: "  << strerror(errno) << LL_ENDL;
    }

    return true;
}

// static
S32 LLFileSystem::getFileSize(const LLUUID& file_id, const LLAssetType::EType file_type)
{
    std::string id_str;
    file_id.toString(id_str);
    const std::string extra_info = "";
    const std::string filename =  LLDiskCache::getInstance()->metaDataToFilepath(id_str, file_type, extra_info);

    S32 file_size = 0;
    llifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
    }

    return file_size;
}

bool LLFileSystem::read(U8* buffer, S32 bytes)
{
    bool success = false;

    std::string id;
    mFileID.toString(id);
    const std::string extra_info = "";
    const std::string filename =  LLDiskCache::getInstance()->metaDataToFilepath(id, mFileType, extra_info);

    llifstream file(filename, std::ios::binary);
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
        if (mBytesRead)
        {
            success = true;
        }
    }

    return success;
}

S32 LLFileSystem::getLastBytesRead()
{
    return mBytesRead;
}

bool LLFileSystem::eof()
{
    return mPosition >= getSize();
}

bool LLFileSystem::write(const U8* buffer, S32 bytes)
{
    std::string id_str;
    mFileID.toString(id_str);
    const std::string extra_info = "";
    const std::string filename =  LLDiskCache::getInstance()->metaDataToFilepath(id_str, mFileType, extra_info);

    bool success = false;

    if (mMode == APPEND)
    {
        llofstream ofs(filename, std::ios::app | std::ios::binary);
        if (ofs)
        {
            ofs.write((const char*)buffer, bytes);

            mPosition = ofs.tellp(); // <FS:Ansariel> Fix asset caching

            success = true;
        }
    }
    // <FS:Ansariel> Fix asset caching
    else if (mMode == READ_WRITE)
    {
        // Don't truncate if file already exists
        llofstream ofs(filename, std::ios::in | std::ios::binary);
        if (ofs)
        {
            ofs.seekp(mPosition, std::ios::beg);
            ofs.write((const char*)buffer, bytes);
            mPosition += bytes;
            success = true;
        }
        else
        {
            // File doesn't exist - open in write mode
            ofs.open(filename, std::ios::binary);
            if (ofs.is_open())
            {
                ofs.write((const char*)buffer, bytes);
                mPosition += bytes;
                success = true;
            }
        }
    }
    // </FS:Ansariel>
    else
    {
        llofstream ofs(filename, std::ios::binary);
        if (ofs)
        {
            ofs.write((const char*)buffer, bytes);

            mPosition += bytes;

            success = true;
        }
    }

    return success;
}

bool LLFileSystem::seek(S32 offset, S32 origin)
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
        return false;
    }
    else if (new_pos < 0)
    {
        LL_WARNS() << "Attempt to seek past beginning of file" << LL_ENDL;

        mPosition = 0;
        return false;
    }

    mPosition = new_pos;
    return true;
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

bool LLFileSystem::rename(const LLUUID& new_id, const LLAssetType::EType new_type)
{
    LLFileSystem::renameFile(mFileID, mFileType, new_id, new_type);

    mFileID = new_id;
    mFileType = new_type;

    return true;
}

bool LLFileSystem::remove()
{
    LLFileSystem::removeFile(mFileID, mFileType);

    return true;
}
