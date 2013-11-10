
#ifndef LLSYSNTAXIDLSL_H
#define LLSYSNTAXIDLSL_H

#endif // LLSYSNTAXIDLSL_H

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llenvmanager.h"
#include "llhttpclient.h"
#include "llviewerregion.h"


class fetchKeywordsFileResponder : public LLHTTPClient::Responder
{
public:
	std::string	mFileSpec;

	fetchKeywordsFileResponder(std::string filespec);

	void errorWithContent(U32 status,
						const std::string& reason,
						const LLSD& content);

	void result(const LLSD& content_ref);
};


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
	std::string		getFileSpec()			const { return mFullFileSpec; }
	LLSD			getKeywordsXML()		const { return sKeywordsXml; }
	LLUUID			getSyntaxId()			const { return mSyntaxIdCurrent; }

	void			initialise();

	static void		setKeywordsXml(const LLSD& content) { sKeywordsXml = content; }


protected:
	std::string		buildFileNameNew();
	std::string		buildFullFileSpec();
	void			fetchKeywordsFile();
	bool			loadKeywordsIntoLLSD();
	void			setSyntaxId(LLUUID SyntaxId) { mSyntaxIdCurrent = SyntaxId; }
	void			setFileNameCurrent(std::string& name) { mFileNameCurrent = name; }
	void			setFileNameDefault(std::string& name) { mFileNameDefault = name; }
	void			setFileNameNew(std::string name) { mFileNameNew = name; }
	void			setSimulatorFeatureName(const std::string& name) { mSimulatorFeature = name; }


//public:


protected:
	LLViewerRegion*	region;


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

	static LLSD		sKeywordsXml;

};
