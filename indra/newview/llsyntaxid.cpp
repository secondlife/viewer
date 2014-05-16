/**
 * @file LLSyntaxId
 * @author Ima Mechanique
 * @brief Handles downloading, saving, and checking of LSL keyword/syntax files
 *		for each region.
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

#include "llagent.h"
#include "llappviewer.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llsyntaxid.h"

//-----------------------------------------------------------------------------
// fetchKeywordsFileResponder
//-----------------------------------------------------------------------------
fetchKeywordsFileResponder::fetchKeywordsFileResponder(std::string filespec)
{
	mFileSpec = filespec;
	LL_DEBUGS("SyntaxLSL") << "Instantiating with file saving to: '" << filespec << "'" << LL_ENDL;
}

void fetchKeywordsFileResponder::errorWithContent(U32 status,
												  const std::string& reason,
												  const LLSD& content)
{
	LLSyntaxIdLSL::getInstance()->mLoadFailed = true;
	LL_WARNS("SyntaxLSL") << "fetchKeywordsFileResponder error [status:" << status << "]: " << content << LL_ENDL;
}

void fetchKeywordsFileResponder::result(const LLSD& content_ref)
{
	// Continue only if a valid LLSD object was returned.
	if (content_ref.isMap())
	{
		LL_DEBUGS("SyntaxLSL") << "content_ref isMap so assuming valid XML." << LL_ENDL;

		if (LLSyntaxIdLSL::getInstance()->isSupportedVersion(content_ref))
		{
			LL_DEBUGS("SyntaxLSL") << "Supported verson of syntax file." << LL_ENDL;

			LLSyntaxIdLSL::getInstance()->setKeywordsXml(content_ref);
			LLSyntaxIdLSL::getInstance()->mInitialized = true;
			LLSyntaxIdLSL::getInstance()->mLoaded = true;
			LLSyntaxIdLSL::getInstance()->mLoadFailed = false;

			cacheFile(content_ref);
		}
		else
		{
			LLSyntaxIdLSL::getInstance()->mLoaded = false;
			LLSyntaxIdLSL::getInstance()->mLoadFailed = true;
			LL_WARNS("SyntaxLSL") << "Unknown or unsupported version of syntax file." << LL_ENDL;
		}
	}
	else
	{
		LLSyntaxIdLSL::getInstance()->mLoaded = false;
		LLSyntaxIdLSL::getInstance()->mLoadFailed = true;
		LL_WARNS("SyntaxLSL") << "Syntax file '" << mFileSpec << "' contains invalid LLSD." << LL_ENDL;
	}

	LLSyntaxIdLSL::getInstance()->mFileFetchedSignal();
}

void fetchKeywordsFileResponder::cacheFile(const LLSD& content_ref)
{
	std::stringstream str;
	LLSDSerialize::toPrettyXML(content_ref, str);
	const std::string xml = str.str();

	// save the str to disc, usually to the cache.
	llofstream file(mFileSpec, std::ios_base::out);
	file.write(xml.c_str(), str.str().size());
	file.close();

	LL_DEBUGS("SyntaxLSL") << "Syntax file received, saving as: '" << mFileSpec << "'" << LL_ENDL;
}

//-----------------------------------------------------------------------------
// LLSyntaxIdLSL
//-----------------------------------------------------------------------------
const std::string LLSyntaxIdLSL::CAPABILITY_NAME = "LSLSyntax";
const std::string LLSyntaxIdLSL::FILENAME_DEFAULT = "keywords_lsl_default.xml";
const std::string LLSyntaxIdLSL::SIMULATOR_FEATURE = "LSLSyntaxId";

/**
 * @brief LLSyntaxIdLSL constructor
 */
LLSyntaxIdLSL::LLSyntaxIdLSL() :
	mInitialized(false),
	mKeywordsXml(LLSD()),
	mLoaded(false),
	mLoadFailed(false),
	mVersionChanged(false),
	mCapabilityName(CAPABILITY_NAME),
	mCapabilityURL(""),
	mFileNameCurrent(FILENAME_DEFAULT),
	mFileNameDefault(FILENAME_DEFAULT),
	mFileNameNew(""),
	mFilePath(LL_PATH_APP_SETTINGS),
	mSimulatorFeature(SIMULATOR_FEATURE),
	mSyntaxIdCurrent(LLUUID()),
	mSyntaxIdNew(LLUUID())
{
}

std::string LLSyntaxIdLSL::buildFileNameNew()
{
	mFileNameNew = mSyntaxIdNew.isNull() ? mFileNameDefault : "keywords_lsl_" + mSyntaxIdNew.asString() + ".llsd.xml";
	return mFileNameNew;
}

std::string LLSyntaxIdLSL::buildFullFileSpec()
{
	ELLPath path = mSyntaxIdNew.isNull() ? LL_PATH_APP_SETTINGS : LL_PATH_CACHE;
	buildFileNameNew();
	mFullFileSpec = gDirUtilp->getExpandedFilename(path, mFileNameNew);
	return mFullFileSpec;
}

//-----------------------------------------------------------------------------
// checkSyntaxIdChange()
//-----------------------------------------------------------------------------
bool LLSyntaxIdLSL::checkSyntaxIdChanged()
{
	mVersionChanged = false;
	LLViewerRegion* region = gAgent.getRegion();

	if (region)
	{
		if (!region->capabilitiesReceived())
		{   // Shouldn't be possible, but experience shows that it may be needed.
			mLoadFailed = true;
			LL_INFOS("SyntaxLSL") << "Region '" << region->getName() << "' has not received capabilities yet. Cannot process SyntaxId." << LL_ENDL;
		}
		else
		{
			LLSD simFeatures;
			region->getSimulatorFeatures(simFeatures);

			// Does the sim have the required feature
			if (simFeatures.has(mSimulatorFeature))
			{
				// get and check the hash
				mSyntaxIdNew = simFeatures[mSimulatorFeature].asUUID();
				mCapabilityURL = region->getCapability(mCapabilityName);
				if (mSyntaxIdCurrent != mSyntaxIdNew)
				{
					LL_DEBUGS("SyntaxLSL") << "Region has LSLSyntaxId capability, and the new hash is '" << mSyntaxIdNew.asString() << "'" << LL_ENDL;

					mVersionChanged = true;
				}
				else
				{
					LL_DEBUGS("SyntaxLSL") << "Region has the same LSLSyntaxId! Leaving hash as '" << mSyntaxIdCurrent.asString() << "'" << LL_ENDL;
				}
			}
			else
			{
				if ( mSyntaxIdCurrent.isNull() && isInitialized())
				{
					LL_DEBUGS("SyntaxLSL") << "Region does not have LSLSyntaxId capability, remaining with default keywords." << LL_ENDL;
				}
				else
				{
					// The hash is set to NULL_KEY to indicate use of default keywords file
					mSyntaxIdNew = LLUUID();
					LL_DEBUGS("SyntaxLSL") << "Region does not have LSLSyntaxId capability, using default keywords." << LL_ENDL;
					mVersionChanged = true;
				}
			}
		}
	}
	return mVersionChanged;
}

/**
 * @brief LLSyntaxIdLSL::fetching
 * If the XML has not loaded yet and it hasn't failed, then we're still fetching it.
 * @return bool Whether the file fetch is still in process.
 */
bool LLSyntaxIdLSL::fetching()
{
	return !(mLoaded || mLoadFailed);
}

//-----------------------------------------------------------------------------
// fetchKeywordsFile
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::fetchKeywordsFile()
{
	LLHTTPClient::get(mCapabilityURL,
					  new fetchKeywordsFileResponder(mFullFileSpec),
					  LLSD(), 30.f
					  );
	LL_DEBUGS("SyntaxLSL") << "LSLSyntaxId capability URL is: " << mCapabilityURL << ". Filename to use is: '" << mFullFileSpec << "'." << LL_ENDL;
}


//-----------------------------------------------------------------------------
// initialize
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::initialize()
{
	mFileNameNew = mFileNameCurrent;
	mSyntaxIdNew = mSyntaxIdCurrent;

	if (checkSyntaxIdChanged())
	{
		mKeywordsXml = LLSD();
		mLoaded = mLoadFailed = false;

		if (mSyntaxIdNew.isNull())
		{ // Need to open the default
			loadDefaultKeywordsIntoLLSD();
		}
		else if (!mCapabilityURL.empty() )
		{
			LL_DEBUGS("SyntaxLSL") << "LSL version has changed, getting appropriate file." << LL_ENDL;

			// Need a full spec regardless of file source, so build it now.
			buildFullFileSpec();
			if ( !mSyntaxIdNew.isNull())
			{
				if ( !gDirUtilp->fileExists(mFullFileSpec) )
				{ // Does not exist, so fetch it from the capability
					LL_INFOS("SyntaxLSL") << "LSL syntax not cached, attempting download." << LL_ENDL;
					fetchKeywordsFile();
				}
				else
				{
					loadKeywordsIntoLLSD();
				}
			}
			else
			{ // Need to open the default
				loadDefaultKeywordsIntoLLSD();
			}
		}
		else
		{
			mLoadFailed = true;
			LL_WARNS("SyntaxLSL") << "LSLSyntaxId capability URL is empty." << LL_ENDL;
			loadDefaultKeywordsIntoLLSD();
		}
	}
	else if (!isInitialized())
	{
		loadDefaultKeywordsIntoLLSD();
	}

	mFileNameCurrent = mFileNameNew;
	mSyntaxIdCurrent = mSyntaxIdNew;
}

//-----------------------------------------------------------------------------
// isSupportedVersion
//-----------------------------------------------------------------------------
const U32         LLSD_SYNTAX_LSL_VERSION_EXPECTED = 2;
const std::string LLSD_SYNTAX_LSL_VERSION_KEY("llsd-lsl-syntax-version");

bool LLSyntaxIdLSL::isSupportedVersion(const LLSD& content)
{
	bool isValid = false;
	/*
	 * If the schema used to store LSL keywords and hints changes, this value is incremented
	 * Note that it should _not_ be changed if the keywords and hints _content_ changes.
	 */

	if (content.has(LLSD_SYNTAX_LSL_VERSION_KEY))
	{
		LL_DEBUGS("SyntaxLSL") << "LSL syntax version: " << content[LLSD_SYNTAX_LSL_VERSION_KEY].asString() << LL_ENDL;

		if (content[LLSD_SYNTAX_LSL_VERSION_KEY].asInteger() == LLSD_SYNTAX_LSL_VERSION_EXPECTED)
		{
			isValid = true;
		}
	}
	else
	{
		LL_DEBUGS("SyntaxLSL") << "No LSL syntax version key." << LL_ENDL;
	}

	return isValid;
}

//-----------------------------------------------------------------------------
// loadDefaultKeywordsIntoLLSD()
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::loadDefaultKeywordsIntoLLSD()
{
	LL_DEBUGS("SyntaxLSL") << "LSLSyntaxId is null so we will use the default file." << LL_ENDL;
	mSyntaxIdNew = LLUUID();
	buildFullFileSpec();
	loadKeywordsIntoLLSD();
}

//-----------------------------------------------------------------------------
// loadKeywordsFileIntoLLSD
//-----------------------------------------------------------------------------
/**
 * @brief	Load xml serialised LLSD
 * @desc	Opens the specified filespec and attempts to deserialise the
 *			contained data to the specified LLSD object. indicate success/failure with
 *			sLoaded/sLoadFailed members.
 */
void LLSyntaxIdLSL::loadKeywordsIntoLLSD()
{
	LL_DEBUGS("SyntaxLSL") << "Trying to open cached or default keyword file" << LL_ENDL;

	// Is this the right thing to do, or should we leave the old content
	// even if it isn't entirely accurate anymore?
	mKeywordsXml = LLSD().emptyMap();

	LLSD content;
	llifstream file;
	file.open(mFullFileSpec);
	if (file.is_open())
	{
		mLoaded = (bool)LLSDSerialize::fromXML(content, file);
		if (!mLoaded)
		{
			LL_WARNS("SyntaxLSL") << "Unable to deserialise: " << mFullFileSpec << LL_ENDL;
		}
		else
		{
			if (isSupportedVersion(content))
			{
				mKeywordsXml = content;
				mLoaded = true;
				mInitialized = true;
				LL_DEBUGS("SyntaxLSL") << "Deserialised: " << mFullFileSpec << LL_ENDL;
			}
			else
			{
				mLoaded = false;
				LL_WARNS("SyntaxLSL") << "Unknown or unsupported version of syntax file." << LL_ENDL;
			}
		}
	}
	else
	{
		LL_WARNS("SyntaxLSL") << "Unable to open: " << mFullFileSpec << LL_ENDL;
	}
	mLoadFailed = !mLoaded;
}

boost::signals2::connection LLSyntaxIdLSL::addFileFetchedCallback(const file_fetched_signal_t::slot_type& cb)
{
	return mFileFetchedSignal.connect(cb);
}
