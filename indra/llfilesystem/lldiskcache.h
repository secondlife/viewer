/**
 * @file lldiskcache.h
 * @brief The disk cache implementation.
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

#ifndef _LLDISKCACHE
#define _LLDISKCACHE

#include "llsingleton.h"

class LLDiskCache :
    public LLParamSingleton<LLDiskCache>
{
    public:
        LLSINGLETON(LLDiskCache,
                    const std::string cache_dir,
                    const int max_size_bytes,
                    const bool enable_cache_debug_info);
        virtual ~LLDiskCache() = default;

    public:
        ///**
        // * The location of the cache dir is set in llappviewer at startup via the
        // * saved settings parameters.  We do not have access to those saved settings
        // * here in LLCommon so we must provide an accessor for other classes to use
        // * when they need it - e.g. LLFilesystem needs the path so it can write files
        // * to it.
        // */
        //const std::string getCacheDirName() { return mCacheDir; }

        ///**
        // * Each cache filename has a prefix inserted (see definition of the
        // * mCacheFilenamePrefix variable below for why) but the LLFileSystem
        // * component needs access to it to in order to create the file so we
        // * expose it by a getter here.
        // */
        //const std::string getCacheFilenamePrefix() { return mCacheFilenamePrefix; }

        /**
         * Construct a filename and path to it based on the file meta data
         * (id, asset type, additional 'extra' info like discard level perhaps)
         * Worth pointing out that this function used to be in LLFileSystem but
         * so many things had to be pushed back there to accomodate it, that I
         * decided to move it here.  Still not sure that's completely right.
         */
        const std::string metaDataToFilepath(const std::string id,
                                             LLAssetType::EType at,
                                             const std::string extra_info);

        /**
         * Update the "last write time" of a file to "now". This must be called whenever a
         * file in the cache is read (not written) so that the last time the file was
         * accessed which is used in the mechanism for purging the cache, is up to date.
         */
        void updateFileAccessTime(const std::string file_path);

        /**
         * Purge the oldest items in the cache so that the combined size of all files
         * is no bigger than mMaxSizeBytes.
         */
        void purge();

        /**
         * Clear the cache by removing the files in the cache directory individually
         */
        void clearCache(const std::string cache_dir);

    private:
        /**
         * Utility function to gather the total size the files in a given
         * directory. Primarily used here to determine the directory size
         * before and after the cache purge
         */
        uintmax_t dirFileSize(const std::string dir);

        /**
         * Utility function to convert an LLAssetType enum into a
         * string that we use as part of the cache file filename
         */
        const std::string assetTypeToString(LLAssetType::EType at);

    private:
        /**
         * The maximum size of the cache in bytes. After purge is called, the
         * total size of the cache files in the cache directory will be
         * less than this value
         */
        uintmax_t mMaxSizeBytes;

        /**
         * The folder that holds the cached files. The consumer of this
         * class must avoid letting the user set this location as a malicious
         * setting could potentially point it at a non-cache directory (for example,
         * the Windows System dir) with disastrous results.
         */
        std::string mCacheDir;

        /**
         * The prefix inserted at the start of a cache file filename to
         * help identify it as a cache file. It's probably not required
         * (just the presence in the cache folder is enough) but I am
         * paranoid about the cache folder being set to something bad
         * like the users' OS system dir by mistake or maliciously and
         * this will help to offset any damage if that happens.
         */
        std::string mCacheFilenamePrefix;

        /**
         * When enabled, displays additional debugging information in
         * various parts of the code
         */
        bool mEnableCacheDebugInfo;
};

#endif // _LLDISKCACHE
