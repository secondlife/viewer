 /** 
* @file llcrashlogger.cpp
* @brief Crash logger implementation
*
* $LicenseInfo:firstyear=2003&license=viewergpl$
* 
* Copyright (c) 2003-2007, Linden Research, Inc.
* 
* Second Life Viewer Source Code
* The source code in this file ("Source Code") is provided by Linden Lab
* to you under the terms of the GNU General Public License, version 2.0
* ("GPL"), unless you have obtained a separate licensing agreement
* ("Other License"), formally executed by you and Linden Lab.  Terms of
* the GPL can be found in doc/GPL-license.txt in this distribution, or
* online at http://secondlife.com/developers/opensource/gplv2
* 
* There are special exceptions to the terms and conditions of the GPL as
* it is applied to this Source Code. View the full text of the exception
* in the file doc/FLOSS-exception.txt in this software distribution, or
* online at http://secondlife.com/developers/opensource/flossexception
* 
* By copying, modifying or distributing this software, you acknowledge
* that you have read and understood your obligations described above,
* and agree to abide by those obligations.
* 
* ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
* WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
* COMPLETENESS OR PERFORMANCE.
* $/LicenseInfo$
*/
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <map>

#include "llcrashlogger.h"
#include "linden_common.h"
#include "llstring.h"
#include "indra_constants.h"	// CRASH_BEHAVIOR_ASK, CRASH_SETTING_NAME
#include "llerror.h"
#include "lltimer.h"
#include "lldir.h"
#include "llsdserialize.h"
#include "lliopipe.h"
#include "llpumpio.h"
#include "llhttpclient.h"
#include "llsdserialize.h"

LLPumpIO* gServicePump;
BOOL gBreak = false;
BOOL gSent = false;

class LLCrashLoggerResponder : public LLHTTPClient::Responder
{
public:
	LLCrashLoggerResponder() 
	{
	}

	virtual void error(U32 status, const std::string& reason)
	{
		gBreak = true;		
	}

	virtual void result(const LLSD& content)
	{	
		gBreak = true;
		gSent = true;
	}
};

bool LLCrashLoggerText::mainLoop()
{
	std::cout << "Entering main loop" << std::endl;
	sendCrashLogs();
	return true;	
}

void LLCrashLoggerText::updateApplication(const std::string& message)
{
	LLCrashLogger::updateApplication(message);
	std::cout << message << std::endl;
}

LLCrashLogger::LLCrashLogger() :
	mCrashBehavior(CRASH_BEHAVIOR_ASK),
	mCrashInPreviousExec(false),
	mSentCrashLogs(false),
	mCrashHost("")
{

}

LLCrashLogger::~LLCrashLogger()
{

}

void LLCrashLogger::gatherFiles()
{

	/*
	//TODO:This function needs to be reimplemented somewhere in here...
	if(!previous_crash && is_crash_log)
	{
		// Make sure the file isn't too old.
		double age = difftime(gLaunchTime, stat_data.st_mtimespec.tv_sec);
		
		//			llinfos << "age is " << age << llendl;
		
		if(age > 60.0)
		{
				// The file was last modified more than 60 seconds before the crash reporter was launched.  Assume it's stale.
			llwarns << "File " << mFilename << " is too old!" << llendl;
			return;
		}
	}
	*/

	updateApplication("Gathering logs...");

	// Figure out the filename of the debug log
	std::string db_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"debug_info.log");
	std::ifstream debug_log_file(db_file_name.c_str());

	// Look for it in the debug_info.log file
	if (debug_log_file.is_open())
	{		
		LLSDSerialize::fromXML(mDebugLog, debug_log_file);
		mFileMap["SecondLifeLog"] = mDebugLog["SLLog"].asString();
		mFileMap["SettingsXml"] = mDebugLog["SettingsFilename"].asString();
		LLCurl::setCAFile(mDebugLog["CAFilename"].asString());
		llinfos << "Using log file from debug log " << mFileMap["SecondLifeLog"] << llendl;
		llinfos << "Using settings file from debug log " << mFileMap["SettingsXml"] << llendl;
	}
	else
	{
		// Figure out the filename of the second life log
		LLCurl::setCAFile(gDirUtilp->getCAFile());
		mFileMap["SecondLifeLog"] = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.log");
		mFileMap["SettingsXml"] = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,"settings.xml");
	}

	gatherPlatformSpecificFiles();

	//Use the debug log to reconstruct the URL to send the crash report to
	if(mDebugLog.has("CurrentSimHost"))
	{
		mCrashHost = "https://";
		mCrashHost += mDebugLog["CurrentSimHost"].asString();
		mCrashHost += ":12043/crash/report";
	}
	// Use login servers as the alternate, since they are already load balanced and have a known name
	mAltCrashHost = "https://login.agni.lindenlab.com:12043/crash/report";

	mCrashInfo["DebugLog"] = mDebugLog;
	mFileMap["StatsLog"] = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stats.log");
	mFileMap["StackTrace"] = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log");
	
	updateApplication("Encoding files...");

	for(std::map<std::string, std::string>::iterator itr = mFileMap.begin(); itr != mFileMap.end(); ++itr)
	{
		std::ifstream f((*itr).second.c_str());
		if(!f.is_open())
		{
			std::cout << "Can't find file " << (*itr).second << std::endl;
			continue;
		}
		std::stringstream s;
		s << f.rdbuf();
		mCrashInfo[(*itr).first] = s.str();
	}
}

LLSD LLCrashLogger::constructPostData()
{
	LLSD ret;

	if(mCrashInPreviousExec)
	{
		mCrashInfo["CrashInPreviousExecution"] = "Y";
	}

	return mCrashInfo;
}

S32 LLCrashLogger::loadCrashBehaviorSetting()
{
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);

	mCrashSettings.loadFromFile(filename);
		
	S32 value = mCrashSettings.getS32(CRASH_BEHAVIOR_SETTING);
	
	if (value < CRASH_BEHAVIOR_ASK || CRASH_BEHAVIOR_NEVER_SEND < value) return CRASH_BEHAVIOR_ASK;

	return value;
}

bool LLCrashLogger::saveCrashBehaviorSetting(S32 crash_behavior)
{
	if (crash_behavior != CRASH_BEHAVIOR_ASK && crash_behavior != CRASH_BEHAVIOR_ALWAYS_SEND) return false;

	mCrashSettings.setS32(CRASH_BEHAVIOR_SETTING, crash_behavior);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);

	mCrashSettings.saveToFile(filename, FALSE);

	return true;
}

bool LLCrashLogger::runCrashLogPost(std::string host, LLSD data, std::string msg, int retries, int timeout)
{
	gBreak = false;
	std::string status_message;
	for(int i = 0; i < retries; ++i)
	{
		status_message = llformat("%s, try %d...", msg.c_str(), i+1);
		LLHTTPClient::post(host, data, new LLCrashLoggerResponder(), timeout);
		while(!gBreak)
		{
			updateApplication(status_message);
		}
		if(gSent)
		{
			return gSent;
		}
	}
	return gSent;
}

bool LLCrashLogger::sendCrashLogs()
{
	gatherFiles();

	LLSD post_data;
	post_data = constructPostData();

	updateApplication("Sending reports...");

	std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
															   "SecondLifeCrashReport");
	std::string report_file = dump_path + ".log";

	std::ofstream out_file(report_file.c_str());
	LLSDSerialize::toPrettyXML(post_data, out_file);
	out_file.close();

	bool sent = false;

	//*TODO: Translate
	if(mCrashHost != "")
	{
		sent = runCrashLogPost(mCrashHost, post_data, std::string("Sending to server"), 3, 5);
	}

	if(!sent)
	{
		sent = runCrashLogPost(mAltCrashHost, post_data, std::string("Sending to alternate server"), 3, 5);
	}
	
	mSentCrashLogs = sent;

	return true;
}

void LLCrashLogger::updateApplication(const std::string& message)
{
	gServicePump->pump();
    gServicePump->callback();
}

bool LLCrashLogger::init()
{
	// We assume that all the logs we're looking for reside on the current drive
	gDirUtilp->initAppDirs("SecondLife");

	// Default to the product name "Second Life" (this is overridden by the -name argument)
	mProductName = "Second Life";
	
	mCrashSettings.declareS32(CRASH_BEHAVIOR_SETTING, CRASH_BEHAVIOR_ASK, "Controls behavior when viewer crashes "
		"(0 = ask before sending crash report, 1 = always send crash report, 2 = never send crash report)");

	llinfos << "Loading crash behavior setting" << llendl;
	mCrashBehavior = loadCrashBehaviorSetting();

	//Run through command line options
	if(getOption("previous").isDefined())
	{
		llinfos << "Previous execution did not remove SecondLife.exec_marker" << llendl;
		mCrashInPreviousExec = TRUE;
	}

	if(getOption("dialog").isDefined())
	{
		llinfos << "Show the user dialog" << llendl;
		mCrashBehavior = CRASH_BEHAVIOR_ASK;
	}
	
	LLSD name = getOption("name");
	if(name.isDefined())
	{	
		mProductName = name.asString();
	}

	// If user doesn't want to send, bail out
	if (mCrashBehavior == CRASH_BEHAVIOR_NEVER_SEND)
	{
		llinfos << "Crash behavior is never_send, quitting" << llendl;
		return false;
	}

	gServicePump = new LLPumpIO(gAPRPoolp);
	gServicePump->prime(gAPRPoolp);
	LLHTTPClient::setPump(*gServicePump);

	//If we've opened the crash logger, assume we can delete the marker file if it exists	
	if( gDirUtilp )
	{
		std::string marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.exec_marker");
		ll_apr_file_remove( marker_file );
	}
	
	return true;
}
