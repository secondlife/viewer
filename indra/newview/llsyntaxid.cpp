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
	LL_INFOS("SyntaxLSL")
			<< "Instantiating with file saving to: '" << filespec << "'"
			<< LL_ENDL;
}

void fetchKeywordsFileResponder::errorWithContent(U32 status,
												  const std::string& reason,
												  const LLSD& content)
{
	LL_ERRS("SyntaxLSL")
			<< "fetchKeywordsFileResponder error [status:"
			<< status << "]: " << content
			<< LL_ENDL;
}

void fetchKeywordsFileResponder::result(const LLSD& content_ref)
{
	// Continue only if a valid LLSD object was returned.
	if (content_ref.isMap())
	{
		LL_DEBUGS("SyntaxLSL")
				<< "content_ref isMap so assuming valid XML." << LL_ENDL;

		if (LLSyntaxIdLSL::isSupportedVersion(content_ref))
		{
			LL_INFOS("SyntaxLSL")
					<< "Is a supported verson of syntax file." << LL_ENDL;

			LLSyntaxIdLSL::setKeywordsXml(content_ref);
			LLSyntaxIdLSL::sLoaded = true;

			std::stringstream str;
			LLSDSerialize::toPrettyXML(content_ref, str);
			const std::string xml = str.str();

			// save the str to disc, usually to the cache.
			llofstream file(mFileSpec, std::ios_base::out);
			file.write(xml.c_str(), str.str().size());
			file.close();

			LL_INFOS("SyntaxLSL")
					<< "Syntax file received, saving as: '" << mFileSpec << "'" << LL_ENDL;
		}
		else
		{
			LL_WARNS("SyntaxLSL")
					<< "Unknown or unsupported version of syntax file." << LL_ENDL;
		}
	}
	else
	{
		LLSyntaxIdLSL::sLoaded = false;
		LL_ERRS("SyntaxLSL")
				<< "Syntax file '" << mFileSpec << "' contains invalid LLSD!" << LL_ENDL;
	}
}


//-----------------------------------------------------------------------------
// LLSyntaxIdLSL
//-----------------------------------------------------------------------------
/**
 * @brief LLSyntaxIdLSL constructor
 */
LLSyntaxIdLSL::LLSyntaxIdLSL() :
	// Move these to signature?
	mFileNameDefault("keywords_lsl_default.xml"),
	mSimulatorFeature("LSLSyntaxId"),
	mCapabilityName("LSLSyntax"),
	mCapabilityURL(""),
	mFilePath(LL_PATH_APP_SETTINGS)
{
	mSyntaxIdCurrent = LLUUID();
	mFileNameCurrent = mFileNameDefault;
}

LLSD LLSyntaxIdLSL::sKeywordsXml;
bool LLSyntaxIdLSL::sLoaded;

std::string LLSyntaxIdLSL::buildFileNameNew()
{
	mFileNameNew = mSyntaxIdNew.isNull() ? mFileNameDefault : "keywords_lsl_" + gLastVersionChannel + "_" + mSyntaxIdNew.asString() + ".llsd.xml";
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
	bool changed = false;
	LLViewerRegion* region = gAgent.getRegion();

	if (region)
	{
		if (!region->capabilitiesReceived())
		{   // Shouldn't be possible, but experience shows that it may be needed.
			LL_WARNS("SyntaxLSL")
				<< "Region '" << region->getName()
				<< "' has not received capabilities yet! Cannot process SyntaxId."
				<< LL_ENDL;
		}
		else
		{
			LLSD simFeatures;
			std::string message;
			region->getSimulatorFeatures(simFeatures);

			// get and check the hash
			if (simFeatures.has("LSLSyntaxId"))
			{
				mSyntaxIdNew = simFeatures["LSLSyntaxId"].asUUID();
				mCapabilityURL = region->getCapability(mCapabilityName);
				if (mSyntaxIdCurrent != mSyntaxIdNew)
				{
					message = "' it has LSLSyntaxId capability, and the new hash is '"
							+ mSyntaxIdNew.asString() + "'";

					changed = true;
				}
				else
				{
					message = "' it has the same LSLSyntaxId! Leaving hash as '"
							+ mSyntaxIdCurrent.asString() + "'";
				}
			}
			else
			{
				if ( mSyntaxIdCurrent.isNull() )
				{
					message = " it does not have LSLSyntaxId capability, remaining with default keywords file!";
				}
				else
				{
					// The hash is set to NULL_KEY to indicate use of default keywords file
					mSyntaxIdNew = LLUUID();
					message = " it does not have LSLSyntaxId capability, using default keywords file!";
					changed = true;
				}
			}
			LL_INFOS("SyntaxLSL")
				<< "Region is '" << region->getName() << message << LL_ENDL;
		}
	}
	return changed;
}

//-----------------------------------------------------------------------------
// fetchKeywordsFile
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::fetchKeywordsFile()
{
	if ( !mCapabilityURL.empty() )
	{
		//LLSyntaxIdLSL::sLoaded = false;
		LLHTTPClient::get(mCapabilityURL,
						  new fetchKeywordsFileResponder(mFullFileSpec),
						  LLSD(), 30.f
						  );
		LL_INFOS("SyntaxLSL")
				<< "LSLSyntaxId capability URL is: " << mCapabilityURL
				<< ". Filename to use is: '" << mFullFileSpec << "'."
				<< LL_ENDL;
	}
	else
	{
		LL_ERRS("SyntaxLSL")
				<< "LSLSyntaxId capability URL is empty!!" << LL_ENDL;
	}
}

//-----------------------------------------------------------------------------
// initialise
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::initialise()
{
	mFileNameNew = mFileNameCurrent;
	mSyntaxIdNew = mSyntaxIdCurrent;
	if (checkSyntaxIdChanged())
	{
		LL_INFOS("SyntaxLSL")
				<< "LSL version has changed, getting appropriate file."
				<< LL_ENDL;

		// Need a full spec regardless of file source, so build it now.
		buildFullFileSpec();
		if ( !mSyntaxIdNew.isNull() )
		{
			if ( !gDirUtilp->fileExists(mFullFileSpec) )
			{ // Does not exist, so fetch it from the capability
				LL_INFOS("SyntaxLSL")
						<< "File is not cached, we will try to download it!"
						<< LL_ENDL;
				fetchKeywordsFile();
			}
			else
			{
				LL_INFOS("SyntaxLSL")
						<< "File is cached, no need to download!"
						<< LL_ENDL;
				sLoaded = loadKeywordsIntoLLSD();
			}
		}
		else
		{ // Need to open the default
			loadDefaultKeywordsIntoLLSD();
		}
	}
	else if (sKeywordsXml.isDefined())
	{
		LL_INFOS("SyntaxLSL")
				<< "No change to Syntax! Nothing to see. Move along now!"
				<< LL_ENDL;
	}
	else
	{ // Need to open the default
		loadDefaultKeywordsIntoLLSD();
	}

	mFileNameCurrent = mFileNameNew;
	mSyntaxIdCurrent = mSyntaxIdNew;
}

//-----------------------------------------------------------------------------
// isSupportedVersion
//-----------------------------------------------------------------------------
bool LLSyntaxIdLSL::isSupportedVersion(const LLSD& content)
{
	bool isValid = false;
	/*
	 * If the schema used to store lsl keywords and hints changes, this value is incremented
	 * Note that it should _not_ be changed if the keywords and hints _content_ changes.
	 */
	const U32         LLSD_SYNTAX_LSL_VERSION_EXPECTED = 2;
	const std::string LLSD_SYNTAX_LSL_VERSION_KEY("llsd-lsl-syntax-version");

	if (content.has(LLSD_SYNTAX_LSL_VERSION_KEY))
	{
		LL_INFOS("SyntaxLSL")
				<< "Syntax file version: " << content[LLSD_SYNTAX_LSL_VERSION_KEY].asString() << LL_ENDL;

		if (content[LLSD_SYNTAX_LSL_VERSION_KEY].asInteger() == LLSD_SYNTAX_LSL_VERSION_EXPECTED)
		{
			isValid = true;
		}
	}
	else
	{
		LL_WARNS("SyntaxLSL") << "No version key available!" << LL_ENDL;
	}

	return isValid;
}

//-----------------------------------------------------------------------------
// loadDefaultKeywordsIntoLLSD()
//-----------------------------------------------------------------------------
void LLSyntaxIdLSL::loadDefaultKeywordsIntoLLSD()
{
	LL_INFOS("SyntaxLSL")
			<< "LSLSyntaxId is null so we will use the default file!" << LL_ENDL;
	mSyntaxIdNew = LLUUID();
	buildFullFileSpec();
	sLoaded = loadKeywordsIntoLLSD();
}

//-----------------------------------------------------------------------------
// loadKeywordsFileIntoLLSD
//-----------------------------------------------------------------------------
/**
 * @brief	Load xml serialised LLSD
 * @desc	Opens the specified filespec and attempts to deserialise the
 *			contained data to the specified LLSD object.
 * @return	Returns boolean true/false indicating success or failure.
 */
bool LLSyntaxIdLSL::loadKeywordsIntoLLSD()
{
	LL_INFOS("SyntaxLSL")
			<< "Trying to open cached or default keyword file ;-)"
			<< LL_ENDL;

	bool loaded = false;
	LLSD content;
	llifstream file;
	file.open(mFullFileSpec);
	if (file.is_open())
	{
		loaded = (bool)LLSDSerialize::fromXML(content, file);
		if (!loaded)
		{
			LL_ERRS("SyntaxLSL") << "Unable to deserialise file: " << mFullFileSpec << LL_ENDL;

			// Is this the right thing to do, or should we leave the old content
			// even if it isn't entirely accurate anymore?
			sKeywordsXml = LLSD().emptyMap();
		}
		else
		{
			sKeywordsXml = content;
			LL_INFOS("SyntaxLSL") << "Deserialised file: " << mFullFileSpec << LL_ENDL;
		}
	}
	else
	{
		LL_ERRS("SyntaxLSL") << "Unable to open file: " << mFullFileSpec << LL_ENDL;
	}
	return loaded;
}
