/** 
 * @file llsyntaxid.h
 * @brief Contains methods to access the LSLSyntaxId feature and LSLSyntax capability
 * to use the appropriate syntax file for the current region's LSL version.
 * @author Ima Mechanique
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
#ifndef LLSYSNTAXIDLSL_H
#define LLSYSNTAXIDLSL_H

#endif // LLSYSNTAXIDLSL_H

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llenvmanager.h"
#include "llhttpclient.h"
#include "llsingleton.h"
#include "llviewerregion.h"


/**
 * @file llsyntaxid.h
 * @brief Tracks the file needed to decorate the current sim's version of LSL.
 */
class LLSyntaxIdLSL: public LLSingleton<LLSyntaxIdLSL>
{
friend class fetchKeywordsFileResponder;
public:
	typedef boost::signals2::signal<void()> file_fetched_signal_t;

	static const std::string	CAPABILITY_NAME;
	static const std::string	FILENAME_DEFAULT;
	static const std::string	SIMULATOR_FEATURE;

protected:
	//LLViewerRegion*	region;

	static bool		sInitialised;
	static LLSD		sKeywordsXml;
	static bool		sLoaded;
	static bool		sLoadFailed;
	static bool		sVersionChanged;
	static file_fetched_signal_t	sFileFetchedSignal;

private:
	std::string		mCapabilityName;
	std::string		mCapabilityURL;
	std::string		mFileNameCurrent;
	std::string		mFileNameDefault;
	std::string		mFileNameNew;
	ELLPath			mFilePath;
	std::string		mFullFileSpec;
	std::string		mSimulatorFeature;
	LLUUID			mSyntaxIdCurrent;
	LLUUID			mSyntaxIdNew;



public:
	LLSyntaxIdLSL();
	LLSyntaxIdLSL(std::string filenameDefault, std::string simFeatureName, std::string capabilityName);

	bool			checkSyntaxIdChanged();
	bool			fetching();
	std::string		getFileNameCurrent()	const { return mFileNameCurrent; }
	ELLPath			getFilePath()			const { return mFilePath; }
	std::string		getFileSpec()			const { return mFullFileSpec; }
	LLSD			getKeywordsXML()		const { return sKeywordsXml; }
	LLUUID			getSyntaxId()			const { return mSyntaxIdCurrent; }
	bool			isDifferentVersion()	const { return sVersionChanged; }
	bool			isInitialised()			const { return sInitialised; }

	void			initialise();
	bool			isLoaded() { return sLoaded; }

	static bool		isSupportedVersion(const LLSD& content);
	static void		setKeywordsXml(const LLSD& content) { sKeywordsXml = content; }

	boost::signals2::connection		addFileFetchedCallback(const file_fetched_signal_t::slot_type& cb);
	void							removeFileFetchedCallback(boost::signals2::connection callback);


protected:
	std::string		buildFileNameNew();
	std::string		buildFullFileSpec();
	void			fetchKeywordsFile();
	void			loadDefaultKeywordsIntoLLSD();
	void			loadKeywordsIntoLLSD();
	void			setSyntaxId(LLUUID SyntaxId) { mSyntaxIdCurrent = SyntaxId; }
	void			setFileNameCurrent(std::string& name) { mFileNameCurrent = name; }
	void			setFileNameDefault(std::string& name) { mFileNameDefault = name; }
	void			setFileNameNew(std::string name) { mFileNameNew = name; }
//	void			setSimulatorFeatureName(const std::string& name) { mSimulatorFeature = name; }
};


/**
 * @file  llsyntaxid.h
 * @brief Handles responses for the LSLSyntax capability's get call. Is a friend of LLSyntaxIdLSL
 */
class fetchKeywordsFileResponder : public LLHTTPClient::Responder
{
public:
	std::string	mFileSpec;

	/**
	 * @brief fetchKeywordsFileResponder
	 * @param filespec	File path and name of where to save the returned data
	 */
	fetchKeywordsFileResponder(std::string filespec);

	void errorWithContent(U32 status,
						const std::string& reason,
						const LLSD& content);

	/**
	 * @brief Checks the returned LLSD for version and stores it in the LLSyntaxIdLSL object.
	 * @param content_ref The returned LLSD.
	 */
	void result(const LLSD& content_ref);

	/**
	 * @brief Saves the returned file to the location provided at instantiation.
	 *			Could be extended to manage cached entries.
	 * @param content_ref	The LSL syntax file for the sim.
	 */
	void cacheFile(const LLSD& content_ref);
};
