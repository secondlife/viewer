/**
 * @file LLSyntaxId
 * @brief Handles downloading, saving, and checking of LSL keyword/syntax files
 *      for each region.
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

#include "llviewerprecompiledheaders.h"

#include "llsyntaxid.h"
#include "llagent.h"
#include "llappviewer.h"
#include "llsdserialize.h"
#include "llviewerregion.h"
#include "llcorehttputil.h"


//-----------------------------------------------------------------------------
// LLSyntaxIdLSL
//-----------------------------------------------------------------------------
namespace
{
    const std::string SYNTAX_ID_CAPABILITY_NAME   = "LSLSyntax";
    const std::string SYNTAX_DEF_CAPABILITY_NAME  = "ScriptDefinitions";
    const std::string SYNTAX_ID_SIMULATOR_FEATURE = "LSLSyntaxId";
    const std::string FILENAME_INTERNAL_LSL        = "lsl_keywords.xml";
    const std::string FILENAME_INTERNAL_LUA        = "lua_keywords.xml";
    const std::string FILENAME_DEFAULT_LSL         = "keywords_lsl_default.xml";
    const std::string FILENAME_DEFAULT_LUA         = "keywords_lua_default.xml";

    constexpr U32     LLSD_SYNTAX_LSL_VERSION_EXPECTED = 2;
    const std::string LLSD_SYNTAX_LSL_VERSION_KEY("llsd-lsl-syntax-version");

    const std::unordered_set<std::string> MEMCACHED_LLSD = {
        FILENAME_INTERNAL_LSL,
        FILENAME_INTERNAL_LUA
    };
} // namespace

//========================================================================
void LLSyntaxDefCache::initSingleton()
{
    buildDefaultCache();
    loadKeywordsIntoLLSD();
    mRegionChangedCallback = gAgent.addRegionChangedCallback(boost::bind(&LLSyntaxDefCache::handleRegionChanged, this));
    handleRegionChanged(); // Kick off an initial caps query and fetch
}

void LLSyntaxDefCache::cleanupSingleton()
{
    gAgent.removeRegionChangedCallback(mRegionChangedCallback);
    mLSLKeywords   = LLSD();
    mLuaKeywords = LLSD();
    mCapabilityURL = std::string();
    mSyntaxId = LLUUID();
    mFileCachePaths.clear();
}

boost::signals2::connection LLSyntaxDefCache::addSyntaxIDCallback(const syntax_id_changed_signal_t::slot_type& cb)
{
    return mSyntaxIDChangedSignal.connect(cb);
}

//========================================================================
// checkSyntaxId()
// Checks the current region for the LSLSyntaxId feature and capability, and
// if found checks the syntax ID against the one we have. If they differ,
// updates the syntax ID and returns true. Otherwise returns false.
bool LLSyntaxDefCache::updateSyntaxId()
{
    LLViewerRegion* region = gAgent.getRegion();

    if (region)
    {
        if (region->capabilitiesReceived())
        {
            LLSD sim_features;
            region->getSimulatorFeatures(sim_features);

            if (sim_features.has(SYNTAX_ID_SIMULATOR_FEATURE))
            {
                // get and check the hash
                LLUUID new_syntax_id = sim_features[SYNTAX_ID_SIMULATOR_FEATURE].asUUID();

                // *Note* the syntax ID may not have changed, but the region almost certainly has.
                // update the cap URL
                mCapabilityURL = region->getCapability(SYNTAX_DEF_CAPABILITY_NAME);
                mUseDefsCap    = !mCapabilityURL.empty();
                if (!mUseDefsCap)
                {
                    mCapabilityURL = region->getCapability(SYNTAX_ID_CAPABILITY_NAME);
                }
                LL_DEBUGS("SyntaxLSL") << SYNTAX_ID_SIMULATOR_FEATURE << " capability URL: " << mCapabilityURL << LL_ENDL;

                if (new_syntax_id != mSyntaxId)
                {
                    LL_DEBUGS("SyntaxLSL") << "New SyntaxID '" << new_syntax_id << "' found." << LL_ENDL;
                    mSyntaxId = new_syntax_id;
                    return true;
                }

                LL_DEBUGS("SyntaxLSL") << "SyntaxID has not changed. Still " << mSyntaxId << LL_ENDL;
            }
        }
        else
        {
            region->setCapabilitiesReceivedCallback(boost::bind(&LLSyntaxDefCache::handleCapsReceived, this, _1));
            LL_DEBUGS("SyntaxLSL") << "Region has not received capabilities. Waiting for caps..." << LL_ENDL;
        }
    }
    return false;
}

void LLSyntaxDefCache::handleRegionChanged()
{
    if (updateSyntaxId())
    {
        if (!checkCacheAndLoad(mSyntaxId))
        {
            fetchKeywords();
        }
    }
}

void LLSyntaxDefCache::handleCapsReceived(const LLUUID& region_uuid)
{
    LLViewerRegion* current_region = gAgent.getRegion();

    if (region_uuid.notNull() && current_region->getRegionID() == region_uuid)
    {
        updateSyntaxId();
        if (!checkCacheAndLoad(mSyntaxId))
        {
            fetchKeywords();
        }
    }
}

//-----------------------------------------------------------------------------
// fetchKeywords
// Initiates a fetch of the current language definitions from the region caps.
void LLSyntaxDefCache::fetchKeywords()
{
    if (mCapabilityURL.empty())
    {
        LL_WARNS("SyntaxLSL") << "No capability URL for fetching syntax definitions." << LL_ENDL;
        return;
    }
    if (mUseDefsCap)
    {
        LLCoros::instance().launch("LLSyntaxDefCache::fetchKeywordsDefsCoro",
            boost::bind(&LLSyntaxDefCache::fetchKeywordsDefsCoro, this, mCapabilityURL, mSyntaxId));
    }
    else
    {
        LLCoros::instance().launch("LLSyntaxDefCache::fetchKeywordsFileCoro",
            boost::bind(&LLSyntaxDefCache::fetchKeywordsFileCoro, this, mCapabilityURL, mSyntaxId));
    }
}

//-----------------------------------------------------------------------------
// fetchKeywordsFileCoro
// This uses the legacy language cap which only sends the LSL keywords file.
void LLSyntaxDefCache::fetchKeywordsFileCoro(std::string url, LLUUID syntax_id)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("fetchKeywordsFileCoro", httpPolicy);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();

    auto insrt = mInflightFetches.insert(syntax_id);
    if (!insrt.second)
    {
        LL_WARNS("SyntaxLSL") << "Already downloading keyword file for syntax ID \"" << syntax_id << "\"." << LL_ENDL;
        return;
    }

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    mInflightFetches.erase(syntax_id);

    if (!status)
    {
        LL_WARNS("SyntaxLSL") << "Failed to fetch syntax file for syntax ID \"" << syntax_id << "\"" << LL_ENDL;
        return;
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);

    if (isSupportedVersion(result))
    {
        std::string path = buildCacheDirectoryName(syntax_id);

        if (!LLFile::exists(path))
        {
            LL_DEBUGS("SyntaxLSL") << "Cache directory '" << path << "' does not exist. Attempting to create." << LL_ENDL;
            if (LLFile::mkdir(path))
            {
                LL_WARNS("SyntaxLSL") << "Failed to create cache directory '" << path << "'. Cannot cache syntax defs file." << LL_ENDL;
                return;
            }
        }

        // Note that after this call the file cache will have all well known files pointing
        // to the default versions, so below, where we get the path to the lua keywords
        // well be loading the default version.
        buildDefaultCache();

        // The LSL keywords we just received
        std::string full_path = gDirUtilp->add(path, FILENAME_INTERNAL_LSL);
        if (writeCacheFile(full_path, result))
        {
            mFileCachePaths.addNamePath(FILENAME_INTERNAL_LSL, full_path);
        }
        // We need to manually load the Lua keywords
        full_path     = mFileCachePaths.getPath(FILENAME_INTERNAL_LUA);
        LLSD lua_defs = loadDeserializedCacheFile(full_path);

        setKeywords(result, lua_defs);

        // Shuttle this task to the main coro/worker.
        LLAppViewer::instance()->postToMainCoro([this]() {
                mSyntaxIDChangedSignal();
            });
    }
    else
    {
        LL_WARNS("SyntaxLSL") << "Unknown or unsupported version of syntax file." << LL_ENDL;
    }

}

void LLSyntaxDefCache::fetchKeywordsDefsCoro(std::string url, LLUUID syntax_id)
{
    LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter =
        std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("fetchKeywordsDefsCoro", httpPolicy);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();

    static std::set<LLUUID> inflightDefsFetches;
    auto insrt = inflightDefsFetches.insert(syntax_id);
    //auto insrt = mInflightFetches.insert(syntax_id);
    if (!insrt.second)
    {
        LL_WARNS("SyntaxLSL") << "Already downloading keyword defs for \"" << syntax_id << "\"." << LL_ENDL;
        return;
    }

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD               httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status      = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    inflightDefsFetches.erase(syntax_id);

    if (!status)
    {
        LL_WARNS("SyntaxLSL") << "Failed to fetch syntax file \"" << syntax_id << "\"" << LL_ENDL;
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        return;
    }

    // Now we need to walk through the returned LLSD.  It consists of a map keyed on the file name containing a binary
    // blob of the actual file contents.
    LLSD files = result["files"];
    // A region may omit definitions for languages it does not support. Keep
    // the defaults/current definitions for any omitted file.
    LLSD lsl_keywords = mLSLKeywords;
    LLSD lua_keywords = mLuaKeywords;
    if (files.isMap())
    {
        std::string path = buildCacheDirectoryName(syntax_id);

        if (!LLFile::exists(path))
        {
            LL_DEBUGS("SyntaxLSL") << "Cache directory '" << path << "' does not exist. Attempting to create." << LL_ENDL;
            if (LLFile::mkdir(path))
            {
                LL_WARNS("SyntaxLSL") << "Failed to create cache directory '" << path << "'. Cannot cache syntax defs file." << LL_ENDL;
                return;
            }
        }

        buildDefaultCache();

        for (const auto &[filename, contents] : llsd::inMap(files))
        {
            std::string full_path = gDirUtilp->add(path, filename);

            if (filename == FILENAME_INTERNAL_LSL)
            {
                lsl_keywords = contents;
            }
            else if (filename == FILENAME_INTERNAL_LUA)
            {
                lua_keywords = contents;
            }

            if (writeCacheFile(full_path, contents))
            {
                mFileCachePaths.addNamePath(filename, full_path);
            }
        }
    }
    else
    {
        LL_WARNS("SyntaxLSL") << "Malformed syntax defs response, missing 'files' map." << LL_ENDL;
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);

    setKeywords(lsl_keywords, lua_keywords);
    LLAppViewer::instance()->postToMainCoro(
        [this]()
        {
            mSyntaxIDChangedSignal();
        });
}

//-----------------------------------------------------------------------------
// buildCache
// Constructs the cache file paths for the given syntax ID. If the syntax ID is null,
// constructs the default cache file paths.
void LLSyntaxDefCache::buildCachePaths(const LLUUID &syntax_id)
{
    if (syntax_id.notNull())
    {   // Initialize the cache files to point to the default files
        buildCachePaths(LLUUID::null);
    }
    else
    {
        mFileCachePaths.clear();
        mFileCachePaths.addNamePath(
            FILENAME_INTERNAL_LSL,
            gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, FILENAME_DEFAULT_LSL));
        mFileCachePaths.addNamePath(
            FILENAME_INTERNAL_LUA,
            gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, FILENAME_DEFAULT_LUA));
        return;
    }

    std::string cache_dir = buildCacheDirectoryName(syntax_id);
    if (!LLFile::exists(cache_dir))
    {
        LL_DEBUGS("SyntaxLSL") << "Cache directory '" << cache_dir << "' does not exist." << LL_ENDL;
        return;
    }

    auto files =  gDirUtilp->getFilesInDir(cache_dir);
    for (const auto &file : files)
    {
        mFileCachePaths.addNamePath(file, gDirUtilp->add(cache_dir, file));
    }
}

bool LLSyntaxDefCache::writeCacheFile(const std::string &fileSpec, const LLSD& content_ref)
{
    bool                    binary(content_ref.isBinary());
    std::ios_base::openmode mode(binary ? (std::ios_base::out | std::ios_base::binary)
                                        : std::ios_base::out);
    std::ofstream           file(fileSpec.c_str(), mode);

    if (!file.is_open())
    {
        LL_WARNS("SyntaxLSL") << "Failed to open file for writing: '" << fileSpec << "'" << LL_ENDL;
        return false;
    }

    if (binary)
    {
        LL_DEBUGS("SyntaxLSL") << "Caching raw content to '" << fileSpec << "'" << LL_ENDL;
        file.write((const char*)content_ref.asBinary().data(), content_ref.asBinary().size());
    }
    else
    {
        LL_DEBUGS("SyntaxLSL") << "Caching XML content to '" << fileSpec << "'" << LL_ENDL;
        LLSDSerialize::serialize(content_ref, file, LLSDSerialize::LLSD_XML, LLSDFormatter::OPTIONS_PRETTY);
    }
    file.close();

    if (!file.good())
    {
        LL_WARNS("SyntaxLSL") << "Failed to write content to file: '" << fileSpec << "'" << LL_ENDL;
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// checkCacheAndLoad
// Tests the local cache for the given syntax ID. If found it loads the keywords
// from the cache into LLSD and returns true. Otherwise returns false.
bool LLSyntaxDefCache::checkCacheAndLoad(const LLUUID& syntax_id)
{
    if (checkLocalCache(syntax_id))
    {
        buildCachePaths(syntax_id);
        loadKeywordsIntoLLSD();
        return true;
    }
    return false;
}

std::string LLSyntaxDefCache::buildCacheDirectoryName(const LLUUID& syntax_id)
{
    if (syntax_id.isNull())
    {
        LL_DEBUGS("SyntaxLSL") << "No SyntaxID, using app settings directory." << LL_ENDL;
        return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "syntax_default");
    }
    else
    {
        LL_DEBUGS("SyntaxLSL") << "Using cache directory for SyntaxID '" << syntax_id << "'." << LL_ENDL;
        std::string cache_dir_name = "syntax_" + syntax_id.asString();

        return gDirUtilp->getExpandedFilename(LL_PATH_CACHE, cache_dir_name);
    }
}

bool LLSyntaxDefCache::checkLocalCache(const LLUUID& syntax_id) const
{
    if (syntax_id.isNull())
    { // Cache check will always fail if we don't have a valid SyntaxID, so skip it in that case.
        LL_DEBUGS("SyntaxLSL") << "No SyntaxID, skipping local cache check." << LL_ENDL;
        return false;
    }

    // Check for the existence of the cache directory for this syntax ID. If it doesn't exist, then we don't have a cached file.
    std::string cache_dir = buildCacheDirectoryName(syntax_id);
    return gDirUtilp->fileExists(cache_dir);
}


//-----------------------------------------------------------------------------
// isSupportedVersion
//-----------------------------------------------------------------------------

bool LLSyntaxDefCache::isSupportedVersion(const LLSD& content)
{
    bool is_valid = false;
    /*
     * If the schema used to store LSL keywords and hints changes, this value is incremented
     * Note that it should _not_ be changed if the keywords and hints _content_ changes.
     */

    if (content.has(LLSD_SYNTAX_LSL_VERSION_KEY))
    {
        LL_DEBUGS("SyntaxLSL") << "LSL syntax version: " << content[LLSD_SYNTAX_LSL_VERSION_KEY].asString() << LL_ENDL;

        if (content[LLSD_SYNTAX_LSL_VERSION_KEY].asInteger() == LLSD_SYNTAX_LSL_VERSION_EXPECTED)
        {
            is_valid = true;
        }
    }
    else
    {
        LL_DEBUGS("SyntaxLSL") << "Missing LSL syntax version key." << LL_ENDL;
    }

    return is_valid;
}

//-----------------------------------------------------------------------------
// loadKeywordsIntoLLSD
//-----------------------------------------------------------------------------
/**
 * @brief   Load xml serialized LLSD
 * @desc    Open the internal lsl keywords files and deserialize them into the correct
 *          members.
 */
void LLSyntaxDefCache::loadKeywordsIntoLLSD()
{
    for (auto& filename : MEMCACHED_LLSD)
    {
        // Note, in the case of the legacy language cap (it only delivers the LSL keywords file)
        // The mFileCachePaths will have been initialized in such a way that the Lua keywords
        // point to the default file.
        std::string full_path = mFileCachePaths.getPath(filename);
        if (!full_path.empty())
        {
            LLSD content = loadDeserializedCacheFile(full_path);
            if (!content.isUndefined() && isSupportedVersion(content))
            {
                LL_DEBUGS("SyntaxLSL") << "Deserialized cached file: " << full_path << LL_ENDL;
                if (filename == FILENAME_INTERNAL_LSL)
                {
                    mLSLKeywords = content;
                }
                else if (filename == FILENAME_INTERNAL_LUA)
                {
                    mLuaKeywords = content;
                }
            }
            else
            {
                LL_WARNS("SyntaxLSL") << "Unknown or unsupported version of syntax file " << full_path << "." << LL_ENDL;
            }
        }
    }

    mSyntaxIDChangedSignal();
}

std::vector<std::string> LLSyntaxDefCache::getCacheFileNames() const
{
    std::vector<std::string> names;
    for (const auto& [name, path] : mFileCachePaths)
    {
        names.push_back(name);
    }
    return names;
}

LLSD LLSyntaxDefCache::loadDeserializedCacheFile(const std::string& file_path)
{
    std::ifstream file(file_path.c_str());
    if (file.good())
    {
        LLSD content;
        if (LLSDSerialize::deserialize(content, file, -1))
        {
            return content;
        }
    }
    else
    {
        LL_WARNS("SyntaxLSL") << "Failed to open cached file: " << file_path << LL_ENDL;
    }
    return LLSD();
}

std::string LLSyntaxDefCache::loadCacheFile(const std::string& name) const
{
    std::string full_path = mFileCachePaths.getPath(name);
    if (!full_path.empty())
    {
        std::ifstream file(full_path.c_str());
        if (file.good())
        {
            std::ostringstream ss;
            ss << file.rdbuf();
            return (file.good()) ? ss.str() : std::string();
        }
        else
        {
            LL_WARNS("SyntaxLSL") << "Failed to open cached file: " << full_path << LL_ENDL;
        }
    }
    return std::string();
}

LLSD LLSyntaxDefCache::loadCacheFileAsLLSD(const std::string& name) const
{
    std::string full_path = mFileCachePaths.getPath(name);
    if (!full_path.empty())
    {
        return loadDeserializedCacheFile(full_path);
    }
    return LLSD();
}
