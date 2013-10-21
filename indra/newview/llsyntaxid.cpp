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
	std::string	mFileSpec;
public:
	fetchKeywordsFileResponder(std::string filespec)
	{
		mFileSpec = filespec;
	}

	void errorWithContent(U32 status, const std::string& reason, const LLSD& content)
	{
		LL_WARNS("")
				<< "fetchKeywordsFileResponder error [status:"
				<< status
				<< "]: "
				<< content
				<< LL_ENDL;
	}

	void result(LLSD& content_ref)
	{
		LLSyntaxIdLSL::setKeywordsXml(content_ref);

		std::stringstream str;
		LLSDSerialize::toPrettyXML(content_ref, str);
		LL_WARNS("")
				<< "fetchKeywordsFileResponder result:"
				<< str.str()
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
LLSyntaxIdLSL::LLSyntaxIdLSL() :
	// Move these to signature?
	mFilenameDefault("keywords_lsl_default.xml"),
	mSimulatorFeature("LSLSyntaxId"),
	mCapabilityName("LSLSyntax")
{
	mCurrentSyntaxId = LLUUID();
	mFilenameCurrent = mFilenameDefault;
	mFilenameLocation = LL_PATH_APP_SETTINGS;
	checkSyntaxIdChange();
	//LLEnvManagerNew::instance().setRegionChangeCallback(boost::bind(&LLSyntaxIdLSL::checkSyntaxIdChange(), this));
}

std::string LLSyntaxIdLSL::buildFilename(LLUUID& SyntaxId)
{
	std::string filename = "keywords_lsl_" + SyntaxId.asString() + "_" + gLastVersionChannel + ".llsd.xml";
	return filename;
}

//-----------------------------------------------------------------------------
// checkSyntaxIdChange()
//-----------------------------------------------------------------------------
bool LLSyntaxIdLSL::checkSyntaxIdChange()
{
	bool changed = false;
	if (mRegion)
	{
		LL_WARNS("LSLSyntax")
			<< "REGION is '"
			<< mRegion->getName()
			<< "'"
			<< LL_ENDL;

		if (!mRegion->capabilitiesReceived())
		{   // Shouldn't be possible, but experience shows that it's needed
//				mRegion->setCapabilitiesReceivedCallback(boost::bind(&LLSyntaxIdLSL::checkSyntaxIdChange, this));
			LL_WARNS("LSLSyntax")
				<< "mRegion '"
				<< mRegion->getName()
				<< "' has not received capabilities yet! Setting a callback for when they arrive."
				<< LL_ENDL;
		} else {
			// get and check the hash
			LLSD simFeatures;
			mRegion->getSimulatorFeatures(simFeatures);
			if (simFeatures.has("LSLSyntaxId"))
			{
				LLUUID SyntaxId = simFeatures["LSLSyntaxId"].asUUID();
				if (mCurrentSyntaxId != SyntaxId)
				{
					// set the properties for the fetcher to use
					mFilenameCurrent = buildFilename(SyntaxId);
					mFilenameLocation = LL_PATH_CACHE;
					mCurrentSyntaxId = SyntaxId;

					LL_WARNS("LSLSyntax")
						<< "Region changed to '"
						<< mRegion->getName()
						<< "' it has LSLSyntaxId capability, and the new hash is '"
						<< SyntaxId
						<< "'"
						<< LL_ENDL;

					changed = true;
				} else {
					LL_WARNS("LSLSyntax")
						<< "Region changed to '"
						<< mRegion->getName()
						<< "' it has the same LSLSyntaxId! Leaving hash as '"
						<< mCurrentSyntaxId
						<< "'"
						<< LL_ENDL;
				}
			} else {
				// Set the hash to NULL_KEY to indicate use of default keywords file
				if ( mCurrentSyntaxId.isNull() )
				{
					LL_WARNS("LSLSyntax")
						<< "Region does not have LSLSyntaxId capability, remaining with default keywords file!"
						<< LL_ENDL;
				} else {
					mCurrentSyntaxId = LLUUID();
					mFilenameCurrent = mFilenameDefault;
					mFilenameLocation = LL_PATH_APP_SETTINGS;

					LL_WARNS("LSLSyntax")
						<< "Region does not have LSLSyntaxId capability, using default keywords file!"
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
	bool fetched = false;
	std::string cap_url = mRegion->getCapability(mCapabilityName);
//	mResponder->setFileSpec(mFilenameSpec);
	if ( !cap_url.empty() )
	{
		LLHTTPClient::get(cap_url, mResponder);
	}

	return fetched;
}

void LLSyntaxIdLSL::initialise()
{
	mRegion = gAgent.getRegion();
	if (checkSyntaxIdChange())
	{
		mFilenameFull = gDirUtilp->getExpandedFilename(
					mFilenameLocation,
					mFilenameCurrent
					);
		if ( !mCurrentSyntaxId.isNull() )
		{
			bool success = true;
			if (!gDirUtilp->fileExists(mFilenameSpec))
			{
				mResponder = new fetchKeywordsFileResponder(mFilenameSpec);
				success = fetchKeywordsFile();
			}
		}
		// TODO add a signal here to tell the editor the hash has changed?
	}

	mRegion = NULL;
}

//-----------------------------------------------------------------------------
// openKeywordsFile
//-----------------------------------------------------------------------------
void openKeywordsFile()
{
	;
}
