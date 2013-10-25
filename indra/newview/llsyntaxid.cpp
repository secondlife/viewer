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

#include "llhttpclient.h"
#include "llagent.h"
#include "llappviewer.h"
#include "llcurl.h"
#include "llenvmanager.h"
#include "llsdserialize.h"
#include "llsyntaxid.h"

//-----------------------------------------------------------------------------
// fetchKeywordsFileResponder
//-----------------------------------------------------------------------------
class fetchKeywordsFileResponder : public LLCurl::Responder
{
public:
	std::string	mFileSpec;

	fetchKeywordsFileResponder(std::string filespec)
	{
		mFileSpec = filespec;
	}

	void errorWithContent(U32 status,
						  const std::string& reason,
						  const LLSD& content)
	{
		LL_WARNS("")
				<< "fetchKeywordsFileResponder error [status:"
				<< status
				<< "]: "
				<< content
				<< LL_ENDL;
	}

	void result(const LLSD& content_ref)
	{
		//LLSyntaxIdLSL::setKeywordsXml(content_ref);
		std::stringstream str;
		LLSDSerialize::toPrettyXML(content_ref, str);

		LLSD& xml = LLSD::emptyMap();
		LLSDSerialize::deserialize(xml, str, 10485760);
		//LLSyntaxIdLSL::setKeywordsXml(xml);
		LL_WARNS("")
				<< "fetchKeywordsFileResponder result:" << str.str()
				<< "filename: '" << mFileSpec << "'"
				<< LL_ENDL;

		// TODO save the damn str to disc
		//llofstream file(mFileSpec, std::ios_base::out);
		//file.write(str.str(), str.str().size());
		//file.close();
	}
};

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
	mFilePath(LL_PATH_APP_SETTINGS)
{
	mCurrentSyntaxId = LLUUID();
	mFileNameCurrent = mFileNameDefault;
}

std::string LLSyntaxIdLSL::buildFileName(LLUUID& SyntaxId)
{
	std::string filename = "keywords_lsl_" + SyntaxId.asString() + "_" + gLastVersionChannel + ".llsd.xml";
	return filename;
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
		/*
		LL_WARNS("LSLSyntax")
			<< "REGION is '" << region->getName() << "'"
			<< LL_ENDL;
			*/

		if (!region->capabilitiesReceived())
		{   // Shouldn't be possible, but experience shows that it's needed
//				region->setCapabilitiesReceivedCallback(boost::bind(&LLSyntaxIdLSL::checkSyntaxIdChange, this));
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
				LLUUID SyntaxId = simFeatures["LSLSyntaxId"].asUUID();
				if (mCurrentSyntaxId != SyntaxId)
				{
					// set the properties for the fetcher to use
					mFileNameCurrent = buildFileName(SyntaxId);
					mFilePath = LL_PATH_CACHE;
					mCurrentSyntaxId = SyntaxId;

					LL_WARNS("LSLSyntax")
						<< "Region changed to '" << region->getName()
						<< "' it has LSLSyntaxId capability, and the new hash is '"
						<< SyntaxId << "'"
						<< LL_ENDL;

					changed = true;
				}
				else
				{
					LL_WARNS("LSLSyntax")
						<< "Region changed to '" << region->getName()
						<< "' it has the same LSLSyntaxId! Leaving hash as '"
						<< mCurrentSyntaxId << "'"
						<< LL_ENDL;
				}
			}
			else
			{
				// Set the hash to NULL_KEY to indicate use of default keywords file
				if ( mCurrentSyntaxId.isNull() )
				{
					LL_WARNS("LSLSyntax")
						<< "Region changed to '" << region->getName()
						<< " it does not have LSLSyntaxId capability, remaining with default keywords file!"
						<< LL_ENDL;
				}
				else
				{
					mCurrentSyntaxId = LLUUID();
					mFileNameCurrent = mFileNameDefault;
					mFilePath = LL_PATH_APP_SETTINGS;

					LL_WARNS("LSLSyntax")
						<< "Region changed to '" << region->getName()
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
bool LLSyntaxIdLSL::fetchKeywordsFile()
{
	LLViewerRegion* region = gAgent.getRegion();
	bool fetched = false;

	std::string cap_url = region->getCapability(mCapabilityName);
	if ( !cap_url.empty() )
	{
		LLHTTPClient::get(cap_url, new fetchKeywordsFileResponder(mFullFileSpec));
	}

	return fetched;
}

void LLSyntaxIdLSL::initialise()
{
	if (checkSyntaxIdChanged())
	{
		LL_WARNS("LSLSyntax")
				<< "Change to syntax, setting up new file."
				<< LL_ENDL;

		setFileNameNew(gDirUtilp->getExpandedFilename(
					mFilePath,
					mFileNameCurrent
					));
		if ( !mCurrentSyntaxId.isNull() )
		{
			bool success = false;
			if ( !gDirUtilp->fileExists(mFullFileSpec) )
			{ // Does not exist, so fetch it from the capability
				success = fetchKeywordsFile();
			}
		}
		// TODO add a signal here to tell the editor the hash has changed?
	}
	else
	{
		LL_WARNS("LSLSyntax")
				<< "Apparently there is no change to Syntax!"
				<< LL_ENDL;

	}
	//LLEnvManagerNew::instance().setRegionChangeCallback(boost::bind(&LLSyntaxIdLSL::checkSyntaxIdChange(), this));
}

//-----------------------------------------------------------------------------
// openKeywordsFile
//-----------------------------------------------------------------------------
void openKeywordsFile()
{
	;
}
