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
	LL_WARNS("LSLSyntax")
			<< "Instantiating with file saving to: '" << filespec << "'"
			<< LL_ENDL;
}

void fetchKeywordsFileResponder::errorWithContent(U32 status,
												  const std::string& reason,
												  const LLSD& content)
{
	LL_WARNS("LSLSyntax")
			<< "fetchKeywordsFileResponder error [status:"
			<< status
			<< "]: "
			<< content
			<< LL_ENDL;
}

void fetchKeywordsFileResponder::result(const LLSD& content_ref)
{
	LLSyntaxIdLSL::setKeywordsXml(content_ref);

	std::stringstream str;
	LLSDSerialize::toPrettyXML(content_ref, str);
	const std::string xml = str.str();

	// save the str to disc, usually to the cache.
	llofstream file(mFileSpec, std::ios_base::out);
	file.write(xml.c_str(), str.str().size());
	file.close();

	LL_WARNS("LSLSyntax")
		<< "Syntax file received, saving as: '" << mFileSpec << "'"
		<< LL_ENDL;
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

std::string LLSyntaxIdLSL::buildFileNameNew()
{
	std::string filename = "keywords_lsl_";
	if (!mSyntaxIdNew.isNull())
	{
		filename += gLastVersionChannel + "_" + mSyntaxIdNew.asString();
	}
	else
	{
		filename += mFileNameDefault;
	}
	mFileNameNew = filename + ".llsd.xml";
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
		{   // Shouldn't be possible, but experience shows that it's needed
			//region->setCapabilitiesReceivedCallback(boost::bind(&LLSyntaxIdLSL::checkSyntaxIdChange, this));
			LL_WARNS("LSLSyntax")
				<< "region '" << region->getName()
				<< "' has not received capabilities yet! Setting a callback for when they arrive."
				<< LL_ENDL;
		}
		else
		{
			// get and check the hash
			LLSD simFeatures;
			region->getSimulatorFeatures(simFeatures);
			if (simFeatures.has("LSLSyntaxId"))
			{
				mSyntaxIdNew = simFeatures["LSLSyntaxId"].asUUID();
				mCapabilityURL = region->getCapability(mCapabilityName);
				if (mSyntaxIdCurrent != mSyntaxIdNew)
				{
					LL_WARNS("LSLSyntax")
						<< "Region is '" << region->getName()
						<< "' it has LSLSyntaxId capability, and the new hash is '"
						<< mSyntaxIdNew << "'"
						<< LL_ENDL;

					changed = true;
				}
				else
				{
					LL_WARNS("LSLSyntax")
						<< "Region is '" << region->getName()
						<< "' it has the same LSLSyntaxId! Leaving hash as '"
						<< mSyntaxIdCurrent << "'"
						<< LL_ENDL;
				}
			}
			else
			{
				// Set the hash to NULL_KEY to indicate use of default keywords file
				if ( mSyntaxIdCurrent.isNull() )
				{
					LL_WARNS("LSLSyntax")
						<< "Region is '" << region->getName()
						<< " it does not have LSLSyntaxId capability, remaining with default keywords file!"
						<< LL_ENDL;
				}
				else
				{
					mSyntaxIdNew = LLUUID();

					LL_WARNS("LSLSyntax")
						<< "Region is '" << region->getName()
						<< " it does not have LSLSyntaxId capability, using default keywords file!"
						<< LL_ENDL;

					changed = true;
				}
			}
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
		LLHTTPClient::get(mCapabilityURL,
						  new fetchKeywordsFileResponder(mFullFileSpec),
						  LLSD(), 30.f
						  );
		LL_WARNS("LSLSyntax")
				<< "LSLSyntaxId capability URL is: " << mCapabilityURL
				<< ". Filename to use is: '" << mFullFileSpec << "'."
				<< LL_ENDL;
	}
	else
	{
		LL_WARNS("LSLSyntax")
				<< "LSLSyntaxId capability URL is empty using capability: '"
				<< mCapabilityName << "'"
				<< LL_ENDL;
	}
}

void LLSyntaxIdLSL::initialise()
{
	if (checkSyntaxIdChanged())
	{
		LL_WARNS("LSLSyntax")
				<< "Change to LSL version, getting appropriate file."
				<< LL_ENDL;

		// Need a full spec regardless of file source, so build it now.
		buildFullFileSpec();
		if ( !mSyntaxIdNew.isNull() )
		{
			LL_WARNS("LSLSyntax")
					<< "We have an ID for the version, so we will process it!"
					<< LL_ENDL;

			if ( !gDirUtilp->fileExists(mFullFileSpec) )
			{ // Does not exist, so fetch it from the capability
				fetchKeywordsFile();
				LL_WARNS("LSLSyntax")
						<< "File is not cached, we will try to download it!"
						<< LL_ENDL;
			}
			else
			{
				LL_WARNS("LSLSyntax")
						<< "File is cached, no need to download!"
						<< LL_ENDL;
				loadKeywordsIntoLLSD();
			}
		}
		else
		{ // Need to open the default
			LL_WARNS("LSLSyntax")
					<< "ID is null so we will use the default file!"
					<< LL_ENDL;
			loadKeywordsIntoLLSD();
		}
		mFileNameCurrent = mFileNameNew;
		mSyntaxIdCurrent = mSyntaxIdNew;
	}
	else
	{
		LL_INFOS("LSLSyntax")
				<< "No change to Syntax! Nothing to see. Move along now!"
				<< LL_ENDL;
	}
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
	LL_WARNS("LSLSyntax")
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
			LL_WARNS("LSLSyntax") << "Unable to deserialise file: " << mFullFileSpec << LL_ENDL;

			// Is this the right thing to do, or should we leave the old content
			// even if it isn't entirely accurate anymore?
			sKeywordsXml = LLSD().emptyMap();
		}
		else
		{
			sKeywordsXml = content;
			LL_INFOS("LSLSyntax") << "Deserialised file: " << mFullFileSpec << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS("LSLSyntax") << "Unable to open file: " << mFullFileSpec << LL_ENDL;
	}
	return loaded;
}
