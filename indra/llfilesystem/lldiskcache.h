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

class LLDiskCache
{
    public:
        LLDiskCache(const std::string cache_dir);
        ~LLDiskCache();

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

        /**
         * Manage max size in bytes of cache - use a discrete setter/getter so the value can
         * be changed in the preferences and cache cleared/purged without restarting viewer
         * to instantiate this class again.
         */
        void setMaxSizeBytes(const uintmax_t size_bytes) { mMaxSizeBytes = size_bytes; }
        uintmax_t getMaxSizeBytes() const { return mMaxSizeBytes; }

    private:
        /**
         * Utility function to gather the total size the files in a given
         * directory. Primarily used here to determine the directory size 
         * before and after the cache purge
         */
        uintmax_t dirFileSize(const std::string dir);

    private:
        /**
         * Default of 20MB seems reasonable - it will likely be reset
         * immediately anyway using a value from the Viewer settings
         */
        const uintmax_t mDefaultSizeBytes = 20 * 1024 * 1024;
        uintmax_t mMaxSizeBytes;

        /**
         * The folder that holds the cached files. The consumer of this
         * class must avoid letting the user set this location as a malicious
         * setting could potentially point it at a non-cache directory (for example,
         * the Windows System dir) with disastrous results.
         */
        std::string mCacheDir;

        /**
         * This is set from the CacheDebugInfo global setting and
         * when enabled, displays additional debugging information in
         * various parts of the code
         */
        bool mCacheDebugInfo;
};

#endif // _LLDISKCACHE
