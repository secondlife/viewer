/**
 * @file lldiskcache.h
 * @brief The disk cache implementation declarations.
 *
 * @Description:
 * This code implements a disk cache using the following ideas:
 * 1/ The metadata for a file can be encapsulated in the filename.
      The filenames will be composed of the following fields:
        Prefix:     Used to identify the file as a part of the cache.
                    An additional reason for using a prefix is that it
                    might be possible, either accidentally or maliciously
                    to end up with the cache dir set to a non-cache
                    location such as your OS system dir or a work folder.
                    Purging files from that would obviously be a disaster
                    so this is an extra step to help avoid that scenario.
        ID:         Typically the asset ID (UUID) of the asset being
                    saved but can be anything valid for a filename
        Extra Info: A field for use in the future that can be used
                    to store extra identifiers - e.g. the discard
                    level of a JPEG2000 file
        Asset Type: A text string created from the LLAssetType enum
                    that identifies the type of asset being stored.
        .asset      A file extension of .asset is used to help
                    identify this as a Viewer asset file
 * 2/ The time of last access for a file can be updated instantly
 *    for file reads and automatically as part of the file writes.
 * 3/ The purge algorithm collects a list of all files in the
 *    directory, sorts them by date of last access (write) and then
 *    deletes any files based on age until the total size of all
 *    the files is less than the maximum size specified.
 * 4/ An LLSingleton idiom is used since there will only ever be
 *    a single cache and we want to access it from numerous places.
 * 5/ Performance on my modest system seems very acceptable. For
 *    example, in testing, I was able to purge a directory of
 *    10,000 files, deleting about half of them in ~ 1700ms. For
 *    the same sized directory of files, writing the last updated
 *    time to each took less than 600ms indicating that this
 *    important part of the mechanism has almost no overhead.
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
        /**
         * Since this is using the LLSingleton pattern but we
         * want to allow the constructor to be called first
         * with various parameters, we also invoke the
         * LLParamSingleton idiom and use it to initialize
         * the class via a call in LLAppViewer.
         */
        LLSINGLETON(LLDiskCache,
                    /**
                     * The full name of the cache folder - typically a
                     * a child of the main Viewer cache directory. Defined
                     * by the setting at 'DiskCacheDirName'
                     */
                    const std::string& cache_dir,
                    /**
                     * The maximum size of the cache in bytes - Based on the
                     * setting at 'CacheSize' and 'DiskCachePercentOfTotal'
                     */
                    const uintmax_t max_size_bytes,
                    /**
                     * A flag that enables extra cache debugging so that
                     * if there are bugs, we can ask uses to enable this
                     * setting and send us their logs
                     */
                    const bool enable_cache_debug_info);

        virtual ~LLDiskCache() = default;

    public:
        /**
         * Construct a filename and path to it based on the file meta data
         * (id, asset type, additional 'extra' info like discard level perhaps)
         * Worth pointing out that this function used to be in LLFileSystem but
         * so many things had to be pushed back there to accomodate it, that I
         * decided to move it here.  Still not sure that's completely right.
         */
        static const std::string metaDataToFilepath(const LLUUID& id, LLAssetType::EType at);

        /**
         * Purge the oldest items in the cache so that the combined size of all files
         * is no bigger than mMaxSizeBytes.
         *
         * WARNING: purge() is called by LLPurgeDiskCacheThread. As such it must
         * NOT touch any LLDiskCache data without introducing and locking a mutex!
         *
         * Purging the disk cache involves nontrivial work on the viewer's
         * filesystem. If called on the main thread, this causes a noticeable
         * freeze.
         */
        void purge();

        /**
         * Clear the cache by removing all the files in the specified cache
         * directory individually. Only the files that contain a prefix defined
         * by mCacheFilenamePrefix will be removed.
         */
        void clearCache();

        /**
         * Return some information about the cache for use in About Box etc.
         */
        const std::string getCacheInfo();

        void removeOldVFSFiles();

    private:
        /**
         * Utility function to gather the total size the files in a given
         * directory. Primarily used here to determine the directory size
         * before and after the cache purge
         */
        uintmax_t dirFileSize(const std::string& dir);

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
        static std::string sCacheDir;

        /**
         * When enabled, displays additional debugging information in
         * various parts of the code
         */
        bool mEnableCacheDebugInfo;
};

class LLPurgeDiskCacheThread : public LLThread
{
public:
    LLPurgeDiskCacheThread();

protected:
    void run() override;
};
#endif // _LLDISKCACHE
