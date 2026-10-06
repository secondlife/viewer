/**
 * @file llsyntaxid.h
 * @brief Contains methods to access the LSLSyntaxId feature and LSLSyntax capability
 * to use the appropriate syntax file for the current region's LSL version.
 * @author Ima Mechanique, Cinder Roxley
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
#pragma once

#include "llviewerprecompiledheaders.h"

#include "llsingleton.h"
#include "lleventcoro.h"
#include "llcoros.h"

class LLSyntaxDefCache : public LLSingleton<LLSyntaxDefCache>
{
    LLSINGLETON(LLSyntaxDefCache) = default;

public:
    using syntax_id_changed_signal_t = boost::signals2::signal<void()>;
    using syntax_id_changed_h = boost::signals2::connection;
    class cache_names_t
    {
    public:
        using name_path_map_t = std::unordered_map<std::string, std::string>;
        using iterator       = name_path_map_t::iterator;
        using const_iterator = name_path_map_t::const_iterator;

        cache_names_t() = default;
        ~cache_names_t() = default;

        std::string getPath(const std::string &name) const
        {
            auto it = mNamePathMap.find(name);
            if (it != mNamePathMap.end())
            {
                return it->second;
            }
            return std::string();
        }

        bool hasName(const std::string &name) const
        {
            return mNamePathMap.find(name) != mNamePathMap.end();
        }

        void addNamePath(const std::string &name, const std::string &path)
        {
            mNamePathMap[name] = path;
        }

        iterator        begin() { return mNamePathMap.begin(); }
        const_iterator  begin() const { return mNamePathMap.begin(); }
        iterator        end() { return mNamePathMap.end(); }
        const_iterator  end() const { return mNamePathMap.end(); }

        size_t          size() const { return mNamePathMap.size(); }
        void            clear() { mNamePathMap.clear(); }
        bool            empty() const { return mNamePathMap.empty(); }

        iterator        find(const std::string &name) { return mNamePathMap.find(name); }
        const_iterator  find(const std::string &name) const { return mNamePathMap.find(name); }

    private:
        name_path_map_t mNamePathMap;
    };

    bool                        keywordFetchInProgress() const { return !mInflightFetches.empty(); }
    LLSD                        getLSLKeywords() const { return mLSLKeywords; };
    LLSD                        getLuaKeywords() const { return mLuaKeywords; };
    LLUUID                      getSyntaxID() const { return mSyntaxId; }
    syntax_id_changed_h         addSyntaxIDCallback(const syntax_id_changed_signal_t::slot_type& cb);

    bool                        checkCacheAndLoad(const LLUUID& syntax_id);

    static std::string          buildCacheDirectoryName(const LLUUID& syntax_id);

    using const_iterator = cache_names_t::const_iterator;

    const_iterator  begin() const { return mFileCachePaths.begin(); }
    const_iterator  end() const { return mFileCachePaths.end(); }

    size_t          size() const { return mFileCachePaths.size(); }
    bool            empty() const { return mFileCachePaths.empty(); }

    const_iterator  find(const std::string& name) const { return mFileCachePaths.find(name); }

    std::vector<std::string> getCacheFileNames() const;
    bool                     hasCacheFile(const std::string& name) const { return mFileCachePaths.hasName(name); }
    std::string              loadCacheFile(const std::string& name) const;
    LLSD                     loadCacheFileAsLLSD(const std::string& name) const;

protected:
    void        initSingleton() override;
    void        cleanupSingleton() override;

private:

    bool        updateSyntaxId();
    static bool isSupportedVersion(const LLSD& content);
    void        handleRegionChanged();
    void        handleCapsReceived(const LLUUID& region_uuid);
    void        setKeywords(const LLSD& lsl, const LLSD& lua) { mLSLKeywords = lsl; mLuaKeywords = lua; };

    void        loadKeywordsIntoLLSD();

    void        fetchKeywords();
    void        fetchKeywordsFileCoro(std::string url, LLUUID syntax_id);
    void        fetchKeywordsDefsCoro(std::string url, LLUUID syntax_id);

    bool        checkLocalCache(const LLUUID& syntax_id) const;
    void        buildDefaultCache() { buildCachePaths(LLUUID::null); }
    void        buildCachePaths(const LLUUID &syntax_id);
    bool        writeCacheFile(const std::string &fileSpec, const LLSD& content_ref);

    static LLSD loadDeserializedCacheFile(const std::string& file_path);

    std::set<LLUUID>            mInflightFetches;
    syntax_id_changed_signal_t  mSyntaxIDChangedSignal;
    syntax_id_changed_h         mRegionChangedCallback;

    std::string                 mCapabilityURL;
    cache_names_t               mFileCachePaths;
    LLUUID                      mSyntaxId;
    LLSD                        mLSLKeywords;
    LLSD                        mLuaKeywords;
    bool                        mUseDefsCap{ false };
};
