
#ifndef LLSYSNTAXIDLSL_H
#define LLSYSNTAXIDLSL_H

#endif // LLSYSNTAXIDLSL_H

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llenvmanager.h"
#include "llhttpclient.h"
#include "llviewerregion.h"


/**
 * @file llsyntaxid.h
 * @brief The LLSyntaxIdLSL class
 */
class LLSyntaxIdLSL
{
public:
	LLSyntaxIdLSL();

	bool			checkSyntaxIdChanged();
	std::string		getFileNameCurrent()	const { return mFileNameCurrent; }
	ELLPath			getFilePath()			const { return mFilePath; }
	LLUUID			getSyntaxId()			const { return mCurrentSyntaxId; }

	void			initialise();

	static void		setKeywordsXml(const LLSD& content) { sKeywordsXml = content; }


protected:
	std::string		buildFileName(LLUUID& SyntaxId);
	bool			fetchKeywordsFile();
	void			openKeywordsFile();
	void			setSyntaxId(LLUUID SyntaxId) { mCurrentSyntaxId = SyntaxId; }
	void			setFileNameCurrent(std::string& name) { mFileNameCurrent = name; }
	void			setFileNameDefault(std::string& name) { mFileNameDefault = name; }
	void			setFileNameNew(std::string& name) { mFileNameNew = name; }
	void			setSimulatorFeatureName(const std::string& name) { mSimulatorFeature = name; }


//public:


protected:
	LLViewerRegion*	region;


private:
	std::string		mCapabilityName;
	LLUUID			mCurrentSyntaxId;
	std::string		mFileNameCurrent;
	std::string		mFileNameDefault;
	std::string		mFileNameNew;
	ELLPath			mFilePath;
	std::string		mFullFileSpec;
	std::string		mSimulatorFeature;

	static LLSD		sKeywordsXml;
};
