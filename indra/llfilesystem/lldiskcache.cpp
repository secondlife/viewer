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
#include "llassettype.h"
#include "lldir.h"
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <chrono>

#include "lldiskcache.h"

LLDiskCache::LLDiskCache(const std::string cache_dir,
                         const int max_size_bytes,
                         const bool enable_cache_debug_info) :
    mCacheDir(cache_dir),
    mMaxSizeBytes(max_size_bytes),
    mEnableCacheDebugInfo(enable_cache_debug_info)
{
    mCacheFilenamePrefix = "sl_cache";

    LLFile::mkdir(cache_dir);
}

void LLDiskCache::purge()
{
    if (mEnableCacheDebugInfo)
    {
        LL_INFOS() << "Total dir size before purge is " << dirFileSize(mCacheDir) << LL_ENDL;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    typedef std::pair<std::time_t, std::pair<uintmax_t, std::string>> file_info_t;
    std::vector<file_info_t> file_info;

#if LL_WINDOWS
    std::wstring cache_path(utf8str_to_utf16str(mCacheDir));
#else
    std::string cache_path(mCacheDir);
#endif
    if (boost::filesystem::is_directory(cache_path))
    {
        for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(cache_path), {}))
        {
            if (boost::filesystem::is_regular_file(entry))
            {
                if (entry.path().string().find(mCacheFilenamePrefix) != std::string::npos)
                {
                    uintmax_t file_size = boost::filesystem::file_size(entry);
                    const std::string file_path = entry.path().string();
                    const std::time_t file_time = boost::filesystem::last_write_time(entry);

                    file_info.push_back(file_info_t(file_time, { file_size, file_path }));
                }
            }
        }
    }

    std::sort(file_info.begin(), file_info.end(), [](file_info_t& x, file_info_t& y)
    {
        return x.first > y.first;
    });

    LL_INFOS() << "Purging cache to a maximum of " << mMaxSizeBytes << " bytes" << LL_ENDL;

    uintmax_t file_size_total = 0;
    for (file_info_t& entry : file_info)
    {
        file_size_total += entry.second.first;

        std::string action = "";
        if (file_size_total > mMaxSizeBytes)
        {
            action = "DELETE:";
            boost::filesystem::remove(entry.second.second);
        }
        else
        {
            action = "  KEEP:";
        }

        if (mEnableCacheDebugInfo)
        {
            // have to do this because of LL_INFO/LL_END weirdness
            std::ostringstream line;

            line << action << "  ";
            line << entry.first << "  ";
            line << entry.second.first << "  ";
            line << entry.second.second;
            line << " (" << file_size_total << "/" << mMaxSizeBytes << ")";
            LL_INFOS() << line.str() << LL_ENDL;
        }
    }

    if (mEnableCacheDebugInfo)
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto execute_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        LL_INFOS() << "Total dir size after purge is " << dirFileSize(mCacheDir) << LL_ENDL;
        LL_INFOS() << "Cache purge took " << execute_time << " ms to execute for " << file_info.size() << " files" << LL_ENDL;
    }
}

const std::string LLDiskCache::assetTypeToString(LLAssetType::EType at)
{
    /**
     * Make use of the handy C++17  feature that allows
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

const std::string LLDiskCache::metaDataToFilepath(const std::string id,
        LLAssetType::EType at,
        const std::string extra_info)
{
    std::ostringstream file_path;

    file_path << mCacheDir;
    file_path << gDirUtilp->getDirDelimiter();
    file_path << mCacheFilenamePrefix;
    file_path << "_";
    file_path << id;
    file_path << "_";
    file_path << (extra_info.empty() ? "0" : extra_info);
    //file_path << "_";
    //file_path << assetTypeToString(at); // see  SL-14210 Prune descriptive tag from new cache filenames
                                          // for details of why it was removed. Note that if you put it
                                          // back or change the format of the filename, the cache files
                                          // files will be invalidated (and perhaps, more importantly,
                                          // never deleted unless you delete them manually).
    file_path << ".asset";

    return file_path.str();
}

void LLDiskCache::updateFileAccessTime(const std::string file_path)
{
    /**
     * Threshold in time_t units that is used to decide if the last access time
     * time of the file is updated or not. Added as a precaution for the concern
     * outlined in SL-14582  about frequent writes on older SSDs reducing their
     * lifespan. I think this is the right place for the threshold value - rather
     * than it being a pref - do comment on that Jira if you disagree...
     *
     * Let's start with 1 hour in time_t units and see how that unfolds
     */
    const std::time_t time_threshold = 1 * 60 * 60;

    // current time
    const std::time_t cur_time = std::time(nullptr);

#if LL_WINDOWS
    // file last write time
    const std::time_t last_write_time = boost::filesystem::last_write_time(utf8str_to_utf16str(file_path));

    // delta between cur time and last time the file was written
    const std::time_t delta_time = cur_time - last_write_time;

    // we only write the new value if the time in time_threshold has elapsed
    // before the last one
    if (delta_time > time_threshold)
    {
        boost::filesystem::last_write_time(utf8str_to_utf16str(file_path), cur_time);
    }
#else
    // file last write time
    const std::time_t last_write_time = boost::filesystem::last_write_time(file_path);

    // delta between cur time and last time the file was written
    const std::time_t delta_time = cur_time - last_write_time;

    // we only write the new value if the time in time_threshold has elapsed
    // before the last one
    if (delta_time > time_threshold)
    {
        boost::filesystem::last_write_time(file_path, cur_time);
    }
#endif
}

const std::string LLDiskCache::getCacheInfo()
{
    std::ostringstream cache_info;

    F32 max_in_mb = (F32)mMaxSizeBytes / (1024.0 * 1024.0);
    F32 percent_used = ((F32)dirFileSize(mCacheDir) / (F32)mMaxSizeBytes) * 100.0;

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
#if LL_WINDOWS
    std::wstring cache_path(utf8str_to_utf16str(mCacheDir));
#else
    std::string cache_path(mCacheDir);
#endif
    if (boost::filesystem::is_directory(cache_path))
    {
        for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(cache_path), {}))
        {
            if (boost::filesystem::is_regular_file(entry))
            {
                if (entry.path().string().find(mCacheFilenamePrefix) != std::string::npos)
                {
                    boost::filesystem::remove(entry);
                }
            }
        }
    }
}

uintmax_t LLDiskCache::dirFileSize(const std::string dir)
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
#if LL_WINDOWS
    std::wstring dir_path(utf8str_to_utf16str(dir));
#else
    std::string dir_path(dir);
#endif
    if (boost::filesystem::is_directory(dir_path))
    {
        for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(dir_path), {}))
        {
            if (boost::filesystem::is_regular_file(entry))
            {
                if (entry.path().string().find(mCacheFilenamePrefix) != std::string::npos)
                {
                    total_file_size += boost::filesystem::file_size(entry);
                }
            }
        }
    }

    return total_file_size;
}
