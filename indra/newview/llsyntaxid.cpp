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
const std::string SYNTAX_ID_CAPABILITY_NAME = "LSLSyntax";
const std::string SYNTAX_ID_SIMULATOR_FEATURE = "LSLSyntaxId";
const std::string FILENAME_DEFAULT = "keywords_lsl_default.xml";

/**
 * @brief LLSyntaxIdLSL constructor
 */
LLSyntaxIdLSL::LLSyntaxIdLSL()
:   mKeywordsXml(LLSD())
,   mCapabilityURL(std::string())
,   mFilePath(LL_PATH_APP_SETTINGS)
,   mSyntaxId(LLUUID())
,   mInitialized(false)
{
    loadDefaultKeywordsIntoLLSD();
    mRegionChangedCallback = gAgent.addRegionChangedCallback(boost::bind(&LLSyntaxIdLSL::handleRegionChanged, this));
    handleRegionChanged(); // Kick off an initial caps query and fetch
}

void LLSyntaxIdLSL::buildFullFileSpec()
{
    ELLPath path = mSyntaxId.isNull() ? LL_PATH_APP_SETTINGS : LL_PATH_CACHE;
    const std::string filename = mSyntaxId.isNull() ? FILENAME_DEFAULT : "keywords_lsl_" + mSyntaxId.asString() + ".llsd.xml";
    mFullFileSpec = gDirUtilp->getExpandedFilename(path, filename);
}

//-----------------------------------------------------------------------------
// syntaxIdChange()
//-----------------------------------------------------------------------------
bool LLSyntaxIdLSL::syntaxIdChanged()
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
                mCapabilityURL = region->getCapability(SYNTAX_ID_CAPABILITY_NAME);
                LL_DEBUGS("SyntaxLSL") << SYNTAX_ID_SIMULATOR_FEATURE << " capability URL: " << mCapabilityURL << LL_ENDL;
                if (new_syntax_id != mSyntaxId)
                {
                    LL_DEBUGS("SyntaxLSL") << "New SyntaxID '" << new_syntax_id << "' found." << LL_ENDL;
                    mSyntaxId = new_syntax_id;
                    return true;
                }
                else
                    LL_DEBUGS("SyntaxLSL") << "SyntaxID matches what we have." << LL_ENDL;
            }
        }
        else
        {
            region->setCapabilitiesReceivedCallback(boost::bind(&LLSyntaxIdLSL::handleCapsReceived, this, _1));
            LL_DEBUGS("SyntaxLSL") << "Region has not received capabilities. Waiting for caps..." << LL_ENDL;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// fetchKeywordsFile
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::fetchKeywordsFile(const std::string& filespec)
{
    LLCoros::instance().launch("LLSyntaxIdLSL::fetchKeywordsFileCoro",
        boost::bind(&LLSyntaxIdLSL::fetchKeywordsFileCoro, this, mCapabilityURL, filespec));
    LL_DEBUGS("SyntaxLSL") << "LSLSyntaxId capability URL is: " << mCapabilityURL << ". Filename to use is: '" << filespec << "'." << LL_ENDL;
}

//-----------------------------------------------------------------------------
// fetchKeywordsFileCoro
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::fetchKeywordsFileCoro(std::string url, std::string fileSpec)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::pair<std::set<std::string>::iterator, bool> insrt = mInflightFetches.insert(fileSpec);
    if (!insrt.second)
    {
        LL_WARNS("SyntaxLSL") << "Already downloading keyword file called \"" << fileSpec << "\"." << LL_ENDL;
        return;
    }

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    mInflightFetches.erase(fileSpec);

    if (!status)
    {
        LL_WARNS("SyntaxLSL") << "Failed to fetch syntax file \"" << fileSpec << "\"" << LL_ENDL;
        return;
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);

    if (isSupportedVersion(result))
    {
        setKeywordsXml(result);
        cacheFile(fileSpec, result);
        loadKeywordsIntoLLSD();
    }
    else
    {
        LL_WARNS("SyntaxLSL") << "Unknown or unsupported version of syntax file." << LL_ENDL;
    }

}

//-----------------------------------------------------------------------------
// cacheFile
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::cacheFile(const std::string &fileSpec, const LLSD& content_ref)
{
    std::stringstream str;
    LLSDSerialize::toXML(content_ref, str);
    const std::string xml = str.str();

    // save the str to disk, usually to the cache.
    llofstream file(fileSpec.c_str(), std::ios_base::out);
    file.write(xml.c_str(), str.str().size());
    file.close();

    LL_DEBUGS("SyntaxLSL") << "Syntax file received, saving as: '" << fileSpec << "'" << LL_ENDL;
}

//-----------------------------------------------------------------------------
// initialize
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::initialize()
{
    if(mInitialized) return;
    if (mSyntaxId.isNull())
    {
        loadDefaultKeywordsIntoLLSD();
    }
    else if (!mCapabilityURL.empty())
    {
        LL_DEBUGS("SyntaxLSL") << "LSL version has changed, getting appropriate file." << LL_ENDL;

        // Need a full spec regardless of file source, so build it now.
        buildFullFileSpec();
        if (mSyntaxId.notNull())
        {
            if (!gDirUtilp->fileExists(mFullFileSpec))
            { // Does not exist, so fetch it from the capability
                LL_DEBUGS("SyntaxLSL") << "LSL syntax not cached, attempting download." << LL_ENDL;
                fetchKeywordsFile(mFullFileSpec);
            }
            else
            {
                LL_DEBUGS("SyntaxLSL") << "Found cached Syntax file: " << mFullFileSpec << " Loading keywords." << LL_ENDL;
                loadKeywordsIntoLLSD();
            }
        }
        else
        {
            LL_DEBUGS("SyntaxLSL") << "LSLSyntaxId is null. Loading default values" << LL_ENDL;
            loadDefaultKeywordsIntoLLSD();
        }
    }
    else
    {
        LL_DEBUGS("SyntaxLSL") << "LSLSyntaxId capability URL is empty." << LL_ENDL;
        loadDefaultKeywordsIntoLLSD();
    }
    mInitialized = true;
}

//-----------------------------------------------------------------------------
// isSupportedVersion
//-----------------------------------------------------------------------------
const U32         LLSD_SYNTAX_LSL_VERSION_EXPECTED = 2;
const std::string LLSD_SYNTAX_LSL_VERSION_KEY("llsd-lsl-syntax-version");

bool LLSyntaxIdLSL::isSupportedVersion(const LLSD& content)
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
// loadDefaultKeywordsIntoLLSD()
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::loadDefaultKeywordsIntoLLSD()
{
    mSyntaxId.setNull();
    buildFullFileSpec();
    loadKeywordsIntoLLSD();
}

//-----------------------------------------------------------------------------
// loadKeywordsFileIntoLLSD
//-----------------------------------------------------------------------------
/**
 * @brief   Load xml serialized LLSD
 * @desc    Opens the specified filespec and attempts to deserializes the
 *          contained data to the specified LLSD object. indicate success/failure with
 *          sLoaded/sLoadFailed members.
 */
void LLSyntaxIdLSL::loadKeywordsIntoLLSD()
{
    LLSD content;
    llifstream file;
    file.open(mFullFileSpec.c_str());
    if (file.is_open())
    {
        if (LLSDSerialize::fromXML(content, file) != LLSDParser::PARSE_FAILURE)
        {
            if (isSupportedVersion(content))
            {
                LL_DEBUGS("SyntaxLSL") << "Deserialized: " << mFullFileSpec << LL_ENDL;
            }
            else
            {
                LL_WARNS("SyntaxLSL") << "Unknown or unsupported version of syntax file." << LL_ENDL;
            }
        }
    }
    else
    {
        LL_WARNS("SyntaxLSL") << "Failed to open: " << mFullFileSpec << LL_ENDL;
    }
    mKeywordsXml = content;
    mSyntaxIDChangedSignal();
}

bool LLSyntaxIdLSL::keywordFetchInProgress()
{
    return !mInflightFetches.empty();
}

void LLSyntaxIdLSL::handleRegionChanged()
{
    if (syntaxIdChanged())
    {
        buildFullFileSpec();
        fetchKeywordsFile(mFullFileSpec);
        mInitialized = false;
    }
}

void LLSyntaxIdLSL::handleCapsReceived(const LLUUID& region_uuid)
{
    LLViewerRegion* current_region = gAgent.getRegion();
    
    if (region_uuid.notNull()
        && current_region->getRegionID() == region_uuid)
    {
        syntaxIdChanged();
    }
}

boost::signals2::connection LLSyntaxIdLSL::addSyntaxIDCallback(const syntax_id_changed_signal_t::slot_type& cb)
{
    return mSyntaxIDChangedSignal.connect(cb);
}
