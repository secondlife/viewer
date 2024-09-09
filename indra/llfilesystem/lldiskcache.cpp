/**
 * @file lldiskcache.cpp
 * @brief The disk cache implementation.
 *
 * Note: Rather than keep the top level function comments up
 * to date in both the source and header files, I elected to
 * only have explicit comments about each function and variable
 * in the header - look there for details. The same is true for
 * description of how this code is supposed to work.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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
#include "llapp.h"
#include "llassettype.h"
#include "lldir.h"
#include <boost/filesystem.hpp>
#include <chrono>

#include "lldiskcache.h"

 /**
  * The prefix inserted at the start of a cache file filename to
  * help identify it as a cache file. It's probably not required
  * (just the presence in the cache folder is enough) but I am
  * paranoid about the cache folder being set to something bad
  * like the users' OS system dir by mistake or maliciously and
  * this will help to offset any damage if that happens.
  */
static const std::string CACHE_FILENAME_PREFIX("sl_cache");

std::string LLDiskCache::sCacheDir;

LLDiskCache::LLDiskCache(const std::string& cache_dir,
                         const uintmax_t max_size_bytes,
                         const bool enable_cache_debug_info) :
    mMaxSizeBytes(max_size_bytes),
    mEnableCacheDebugInfo(enable_cache_debug_info)
{
    sCacheDir = cache_dir;
    LLFile::mkdir(cache_dir);
}

// WARNING: purge() is called by LLPurgeDiskCacheThread. As such it must
// NOT touch any LLDiskCache data without introducing and locking a mutex!

// Interaction through the filesystem itself should be safe. Let’s say thread
// A is accessing the cache file for reading/writing and thread B is trimming
// the cache. Let’s also assume using llifstream to open a file and
// boost::filesystem::remove are not atomic (which will be pretty much the
// case).

// Now, A is trying to open the file using llifstream ctor. It does some
// checks if the file exists and whatever else it might be doing, but has not
// issued the call to the OS to actually open the file yet. Now B tries to
// delete the file: If the file has been already marked as in use by the OS,
// deleting the file will fail and B will continue with the next file. A can
// safely continue opening the file. If the file has not yet been marked as in
// use, B will delete the file. Now A actually wants to open it, operation
// will fail, subsequent check via llifstream.is_open will fail, asset will
// have to be re-requested. (Assuming here the viewer will actually handle
// this situation properly, that can also happen if there is a file containing
// garbage.)

// Other situation: B is trimming the cache and A wants to read a file that is
// about to get deleted. boost::filesystem::remove does whatever it is doing
// before actually deleting the file. If A opens the file before the file is
// actually gone, the OS call from B to delete the file will fail since the OS
// will prevent this. B continues with the next file. If the file is already
// gone before A finally gets to open it, this operation will fail and the
// asset will have to be re-requested.
void LLDiskCache::purge()
{
    if (mEnableCacheDebugInfo)
    {
        LL_INFOS() << "Total dir size before purge is " << dirFileSize(sCacheDir) << LL_ENDL;
    }

    boost::system::error_code ec;
    auto start_time = std::chrono::high_resolution_clock::now();

    typedef std::pair<std::time_t, std::pair<uintmax_t, std::string>> file_info_t;
    std::vector<file_info_t> file_info;

#if LL_WINDOWS
    std::wstring cache_path(utf8str_to_utf16str(sCacheDir));
#else
    std::string cache_path(sCacheDir);
#endif
    if (boost::filesystem::is_directory(cache_path, ec) && !ec.failed())
    {
        boost::filesystem::directory_iterator iter(cache_path, ec);
        while (iter != boost::filesystem::directory_iterator() && !ec.failed())
        {
            if (boost::filesystem::is_regular_file(*iter, ec) && !ec.failed())
            {
                if ((*iter).path().string().find(CACHE_FILENAME_PREFIX) != std::string::npos)
                {
                    uintmax_t file_size = boost::filesystem::file_size(*iter, ec);
                    if (ec.failed())
                    {
                        continue;
                    }
                    const std::string file_path = (*iter).path().string();
                    const std::time_t file_time = boost::filesystem::last_write_time(*iter, ec);
                    if (ec.failed())
                    {
                        continue;
                    }

                    file_info.push_back(file_info_t(file_time, { file_size, file_path }));
                }
            }
            iter.increment(ec);
        }
    }

    std::sort(file_info.begin(), file_info.end(), [](file_info_t& x, file_info_t& y)
    {
        return x.first > y.first;
    });

    LL_INFOS() << "Purging cache to a maximum of " << mMaxSizeBytes << " bytes" << LL_ENDL;

    std::vector<bool> file_removed;
    if (mEnableCacheDebugInfo)
    {
        file_removed.reserve(file_info.size());
    }
    uintmax_t file_size_total = 0;
    for (file_info_t& entry : file_info)
    {
        file_size_total += entry.second.first;

        bool should_remove = file_size_total > mMaxSizeBytes;
        if (mEnableCacheDebugInfo)
        {
            file_removed.push_back(should_remove);
        }
        if (should_remove)
        {
            boost::filesystem::remove(entry.second.second, ec);
            if (ec.failed())
            {
                LL_WARNS() << "Failed to delete cache file " << entry.second.second << ": " << ec.message() << LL_ENDL;
            }
        }
    }

    if (mEnableCacheDebugInfo)
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto execute_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        // Log afterward so it doesn't affect the time measurement
        // Logging thousands of file results can take hundreds of milliseconds
        for (size_t i = 0; i < file_info.size(); ++i)
        {
            const file_info_t& entry = file_info[i];
            const bool removed = file_removed[i];
            const std::string action = removed ? "DELETE:" : "KEEP:";

            // have to do this because of LL_INFO/LL_END weirdness
            std::ostringstream line;

            line << action << "  ";
            line << entry.first << "  ";
            line << entry.second.first << "  ";
            line << entry.second.second;
            line << " (" << file_size_total << "/" << mMaxSizeBytes << ")";
            LL_INFOS() << line.str() << LL_ENDL;
        }

        LL_INFOS() << "Total dir size after purge is " << dirFileSize(sCacheDir) << LL_ENDL;
        LL_INFOS() << "Cache purge took " << execute_time << " ms to execute for " << file_info.size() << " files" << LL_ENDL;
    }
}

const std::string LLDiskCache::metaDataToFilepath(const LLUUID& id, LLAssetType::EType at)
{
    return llformat("%s%s%s_%s_0.asset", sCacheDir.c_str(), gDirUtilp->getDirDelimiter().c_str(), CACHE_FILENAME_PREFIX.c_str(), id.asString().c_str());
}

const std::string LLDiskCache::getCacheInfo()
{
    std::ostringstream cache_info;

    F32 max_in_mb = (F32)mMaxSizeBytes / (1024.0f * 1024.0f);
    F32 percent_used = ((F32)dirFileSize(sCacheDir) / (F32)mMaxSizeBytes) * 100.0f;

    cache_info << std::fixed;
    cache_info << std::setprecision(1);
    cache_info << "Max size " << max_in_mb << " MB ";
    cache_info << "(" << percent_used << "% used)";

    return cache_info.str();
}

void LLDiskCache::clearCache()
{
    /**
     * See notes on performance in dirFileSize(..) - there may be
     * a quicker way to do this by operating on the parent dir vs
     * the component files but it's called infrequently so it's
     * likely just fine
     */
    boost::system::error_code ec;
#if LL_WINDOWS
    std::wstring cache_path(utf8str_to_utf16str(sCacheDir));
#else
    std::string cache_path(sCacheDir);
#endif
    if (boost::filesystem::is_directory(cache_path, ec) && !ec.failed())
    {
        boost::filesystem::directory_iterator iter(cache_path, ec);
        while (iter != boost::filesystem::directory_iterator() && !ec.failed())
        {
            if (boost::filesystem::is_regular_file(*iter, ec) && !ec.failed())
            {
                if ((*iter).path().string().find(CACHE_FILENAME_PREFIX) != std::string::npos)
                {
                    boost::filesystem::remove(*iter, ec);
                    if (ec.failed())
                    {
                        LL_WARNS() << "Failed to delete cache file " << *iter << ": " << ec.message() << LL_ENDL;
                    }
                }
            }
            iter.increment(ec);
        }
    }
}

void LLDiskCache::removeOldVFSFiles()
{
    //VFS files won't be created, so consider removing this code later
    static const char CACHE_FORMAT[] = "inv.llsd";
    static const char DB_FORMAT[] = "db2.x";

    boost::system::error_code ec;
#if LL_WINDOWS
    std::wstring cache_path(utf8str_to_utf16str(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "")));
#else
    std::string cache_path(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, ""));
#endif
    if (boost::filesystem::is_directory(cache_path, ec) && !ec.failed())
    {
        boost::filesystem::directory_iterator iter(cache_path, ec);
        while (iter != boost::filesystem::directory_iterator() && !ec.failed())
        {
            if (boost::filesystem::is_regular_file(*iter, ec) && !ec.failed())
            {
                if (((*iter).path().string().find(CACHE_FORMAT) != std::string::npos) ||
                    ((*iter).path().string().find(DB_FORMAT) != std::string::npos))
                {
                    boost::filesystem::remove(*iter, ec);
                    if (ec.failed())
                    {
                        LL_WARNS() << "Failed to delete cache file " << *iter << ": " << ec.message() << LL_ENDL;
                    }
                }
            }
            iter.increment(ec);
        }
    }
}

uintmax_t LLDiskCache::dirFileSize(const std::string& dir)
{
    uintmax_t total_file_size = 0;

    /**
     * There may be a better way that works directly on the folder (similar to
     * right clicking on a folder in the OS and asking for size vs right clicking
     * on all files and adding up manually) but this is very fast - less than 100ms
     * for 10,000 files in my testing so, so long as it's not called frequently,
     * it should be okay. Note that's it's only currently used for logging/debugging
     * so if performance is ever an issue, optimizing this or removing it altogether,
     * is an easy win.
     */
    boost::system::error_code ec;
#if LL_WINDOWS
    std::wstring dir_path(utf8str_to_utf16str(dir));
#else
    std::string dir_path(dir);
#endif
    if (boost::filesystem::is_directory(dir_path, ec) && !ec.failed())
    {
        boost::filesystem::directory_iterator iter(dir_path, ec);
        while (iter != boost::filesystem::directory_iterator() && !ec.failed())
        {
            if (boost::filesystem::is_regular_file(*iter, ec) && !ec.failed())
            {
                if ((*iter).path().string().find(CACHE_FILENAME_PREFIX) != std::string::npos)
                {
                    uintmax_t file_size = boost::filesystem::file_size(*iter, ec);
                    if (!ec.failed())
                    {
                        total_file_size += file_size;
                    }
                }
            }
            iter.increment(ec);
        }
    }

    return total_file_size;
}

LLPurgeDiskCacheThread::LLPurgeDiskCacheThread() :
    LLThread("PurgeDiskCacheThread", nullptr)
{
}

void LLPurgeDiskCacheThread::run()
{
    constexpr std::chrono::seconds CHECK_INTERVAL{60};

    while (LLApp::instance()->sleep(CHECK_INTERVAL))
    {
        LLDiskCache::instance().purge();
    }
}
