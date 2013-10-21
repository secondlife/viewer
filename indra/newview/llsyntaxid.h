
#ifndef LLSYSNTAXIDLSL_H
#define LLSYSNTAXIDLSL_H

#endif // LLSYSNTAXIDLSL_H

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llenvmanager.h"
#include "llhttpclient.h"
#include "llviewerregion.h"


//class LLKeywords;


/**
 * @file llsyntaxid.h
 * @brief The LLSyntaxIdLSL class
 */
class LLSyntaxIdLSL
{
public:
	/**
	 * @brief LLSyntaxIdLSL constructor
	 */
	LLSyntaxIdLSL();

	LLUUID		getSyntaxId() const { return mCurrentSyntaxId; }

	bool		checkSyntaxIdChange();
	std::string	filenameCurrent() { return mFilenameCurrent; }
	ELLPath		filenamePath() { return mFilenameLocation; }
	void		initialise();
	static void	setKeywordsXml(LLSD& content) { LLSyntaxIdLSL::mKeywordsXml = content; }

protected:
	std::string	buildFilename(LLUUID& SyntaxId);
	bool		fetchKeywordsFile();
	void		openKeywordsFile();
	void		setSyntaxId(LLUUID SyntaxId) { mCurrentSyntaxId = SyntaxId; }
	void		setFilenameCurrent(std::string& name) { mFilenameCurrent = name; }
	void		setFilenameDefault(std::string& name) { mFilenameDefault = name; }
	void		setSimulatorFeatureName(const std::string& name) { mSimulatorFeature = name; }

public:
	static LLHTTPClient::ResponderPtr	mResponder;


protected:
//	LLKeywords&							mKeywords;
	LLViewerRegion*						mRegion;

private:
	std::string							mCapabilityName;
	LLUUID								mCurrentSyntaxId;
	std::string							mFilenameCurrent;
	std::string							mFilenameDefault;
	std::string							mFilenameFull;
	ELLPath								mFilenameLocation;
	std::string							mFilenameSpec;
	static LLSD							mKeywordsXml;
	std::string							mSimulatorFeature;
};
