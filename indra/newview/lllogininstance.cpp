/** 
 * @file lllogininstance.cpp
 * @brief Viewer's host for a login connection.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "lllogininstance.h"

// llcommon
#include "llevents.h"
#include "llmd5.h"
#include "stringize.h"

// llmessage (!)
#include "llfiltersd2xmlrpc.h" // for xml_escape_string()

// login
#include "lllogin.h"

// newview
#include "llviewernetwork.h"
#include "llappviewer.h" // Wish I didn't have to, but...
#include "llviewercontrol.h"
#include "llurlsimstring.h"
#include "llfloatertos.h"
#include "llwindow.h"

std::string construct_start_string();

LLLoginInstance::LLLoginInstance() :
	mLoginModule(new LLLogin()),
	mLoginState("offline"),
	mUserInteraction(true),
	mSkipOptionalUpdate(false),
	mAttemptComplete(false),
	mTransferRate(0.0f)
{
	mLoginModule->getEventPump().listen("lllogininstance", 
		boost::bind(&LLLoginInstance::handleLoginEvent, this, _1));
}

LLLoginInstance::~LLLoginInstance()
{
}


void LLLoginInstance::connect(const LLSD& credentials)
{
	std::vector<std::string> uris;
	LLViewerLogin::getInstance()->getLoginURIs(uris);
	connect(uris.front(), credentials);
}

void LLLoginInstance::connect(const std::string& uri, const LLSD& credentials)
{
	constructAuthParams(credentials);
	mLoginModule->connect(uri, mRequestData);
}

void LLLoginInstance::reconnect()
{
	// Sort of like connect, only using the pre-existing
	// request params.
	std::vector<std::string> uris;
	LLViewerLogin::getInstance()->getLoginURIs(uris);
	mLoginModule->connect(uris.front(), mRequestData);
}

void LLLoginInstance::disconnect()
{
	mRequestData.clear();
	mLoginModule->disconnect();
}

LLSD LLLoginInstance::getResponse() 
{
	return mResponseData; 
}

void LLLoginInstance::constructAuthParams(const LLSD& credentials)
{
	// Set up auth request options.
//#define LL_MINIMIAL_REQUESTED_OPTIONS
	LLSD requested_options;
	// *Note: this is where gUserAuth used to be created.
	requested_options.append("inventory-root");
	requested_options.append("inventory-skeleton");
	//requested_options.append("inventory-meat");
	//requested_options.append("inventory-skel-targets");
#if (!defined LL_MINIMIAL_REQUESTED_OPTIONS)
	if(FALSE == gSavedSettings.getBOOL("NoInventoryLibrary"))
	{
		requested_options.append("inventory-lib-root");
		requested_options.append("inventory-lib-owner");
		requested_options.append("inventory-skel-lib");
	//	requested_options.append("inventory-meat-lib");
	}

	requested_options.append("initial-outfit");
	requested_options.append("gestures");
	requested_options.append("event_categories");
	requested_options.append("event_notifications");
	requested_options.append("classified_categories");
	//requested_options.append("inventory-targets");
	requested_options.append("buddy-list");
	requested_options.append("ui-config");
#endif
	requested_options.append("tutorial_setting");
	requested_options.append("login-flags");
	requested_options.append("global-textures");
	if(gSavedSettings.getBOOL("ConnectAsGod"))
	{
		gSavedSettings.setBOOL("UseDebugMenus", TRUE);
		requested_options.append("god-connect");
	}

	char hashed_mac_string[MD5HEX_STR_SIZE];		/* Flawfinder: ignore */
	LLMD5 hashed_mac;
	hashed_mac.update( gMACAddress, MAC_ADDRESS_BYTES );
	hashed_mac.finalize();
	hashed_mac.hex_digest(hashed_mac_string);

	// prepend "$1$" to the password to indicate its the md5'd version.
	std::string dpasswd("$1$");
	dpasswd.append(credentials["passwd"].asString());

	// (re)initialize the request params with creds.
	LLSD request_params(credentials);
	request_params["passwd"] = dpasswd;
	request_params["start"] = construct_start_string();
	request_params["skipoptional"] = mSkipOptionalUpdate;
	request_params["agree_to_tos"] = false; // Always false here. Set true in 
	request_params["read_critical"] = false; // handleTOSResponse
	request_params["last_exec_event"] = gLastExecEvent;
	request_params["mac"] = hashed_mac_string;
	request_params["version"] = gCurrentVersion; // Includes channel name
	request_params["channel"] = gSavedSettings.getString("VersionChannelName");
	request_params["id0"] = LLAppViewer::instance()->getSerialNumber();

	mRequestData["method"] = "login_to_simulator";
	mRequestData["params"] = request_params;
	mRequestData["options"] = requested_options;
}

bool LLLoginInstance::handleLoginEvent(const LLSD& event)
{
	std::cout << "LoginListener called!: \n";
	std::cout << event << "\n";

	if(!(event.has("state") && event.has("progress")))
	{
		llerrs << "Unknown message from LLLogin!" << llendl;
	}

	mLoginState = event["state"].asString();
	mResponseData = event["data"];
	
	if(event.has("transfer_rate"))
	{
		mTransferRate = event["transfer_rate"].asReal();
	}

	if(mLoginState == "offline")
	{
		handleLoginFailure(event);
	}
	else if(mLoginState == "online")
	{
		handleLoginSuccess(event);
	}

	return false;
}

bool LLLoginInstance::handleLoginFailure(const LLSD& event)
{
	// Login has failed. 
	// Figure out why and respond...
	LLSD response = event["data"];
	std::string reason_response = response["reason"].asString();
	std::string message_response = response["message"].asString();
	if(mUserInteraction)
	{
		// For the cases of critical message or TOS agreement,
		// start the TOS dialog. The dialog response will be handled
		// by the LLLoginInstance::handleTOSResponse() callback.
		// The callback intiates the login attempt next step, either 
		// to reconnect or to end the attempt in failure.
		if(reason_response == "tos")
		{
			LLFloaterTOS* tos_dialog = LLFloaterTOS::show(LLFloaterTOS::TOS_TOS,
											message_response,
											boost::bind(&LLLoginInstance::handleTOSResponse, 
														this, _1, "agree_to_tos")
											);
			tos_dialog->startModal();
		}
		else if(reason_response == "critical")
		{
			LLFloaterTOS* tos_dialog = LLFloaterTOS::show(LLFloaterTOS::TOS_CRITICAL_MESSAGE,
											message_response,
											boost::bind(&LLLoginInstance::handleTOSResponse, 
														this, _1, "read_critical")
											);
			tos_dialog->startModal();
		}
		else if(reason_response == "update" || gSavedSettings.getBOOL("ForceMandatoryUpdate"))
		{
			gSavedSettings.setBOOL("ForceMandatoryUpdate", FALSE);
			updateApp(true, message_response);
		}
		else if(reason_response == "optional")
		{
			updateApp(false, message_response);
		}
		else
		{	
			attemptComplete();
		}	
	}
	else // no user interaction
	{
		attemptComplete();
	}

	return false;
}

bool LLLoginInstance::handleLoginSuccess(const LLSD& event)
{
	LLSD response = event["data"];
	std::string message_response = response["message"].asString();
	if(gSavedSettings.getBOOL("ForceMandatoryUpdate"))
	{
		// Testing update...
		gSavedSettings.setBOOL("ForceMandatoryUpdate", FALSE);
		// Don't confuse startup by leaving login "online".
		mLoginModule->disconnect(); 
		updateApp(true, message_response);
	}
	else
	{
		attemptComplete();
	}
	return false;
}

void LLLoginInstance::handleTOSResponse(bool accepted, const std::string& key)
{
	if(accepted)
	{	
		// Set the request data to true and retry login.
		mRequestData[key] = true; 
		reconnect();
	}
	else
	{
		attemptComplete();
	}
}


void LLLoginInstance::updateApp(bool mandatory, const std::string& auth_msg)
{
	// store off config state, as we might quit soon
	gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);	
	gSavedSkinSettings.saveToFile(gSavedSettings.getString("SkinningSettingsFile"), TRUE);

	std::ostringstream message;
	std::string msg;
	if (!auth_msg.empty())
	{
		msg = "(" + auth_msg + ") \n";
	}

	LLSD args;
	args["MESSAGE"] = msg;
	
	LLSD payload;
	payload["mandatory"] = mandatory;

/*
 We're constructing one of the following 6 strings here:
	 "DownloadWindowsMandatory"
	 "DownloadWindowsReleaseForDownload"
	 "DownloadWindows"
	 "DownloadMacMandatory"
	 "DownloadMacReleaseForDownload"
	 "DownloadMac"
 
 I've called them out explicitly in this comment so that they can be grepped for.
 
 Also, we assume that if we're not Windows we're Mac. If we ever intend to support 
 Linux with autoupdate, this should be an explicit #elif LL_DARWIN, but 
 we'd rather deliver the wrong message than no message, so until Linux is supported
 we'll leave it alone.
 */
	std::string notification_name = "Download";
	
#if LL_WINDOWS
	notification_name += "Windows";
#else
	notification_name += "Mac";
#endif
	
	if (mandatory)
	{
		notification_name += "Mandatory";
	}
	else
	{
#if LL_RELEASE_FOR_DOWNLOAD
		notification_name += "ReleaseForDownload";
#endif
	}
	
	LLNotifications::instance().add(notification_name, args, payload, 
		boost::bind(&LLLoginInstance::updateDialogCallback, this, _1, _2));
}

bool LLLoginInstance::updateDialogCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	std::string update_exe_path;
	bool mandatory = notification["payload"]["mandatory"].asBoolean();

#if !LL_RELEASE_FOR_DOWNLOAD
	if (option == 2)
	{
		// This condition attempts to skip the 
		// update if using a dev build.
		// The relog probably won't work if the 
		// update is mandatory. :)

	    // *REMOVE:Mani - Saving for reference...
		//LLStartUp::setStartupState( STATE_LOGIN_AUTH_INIT ); 
		mSkipOptionalUpdate = true;
		reconnect();
		return false;
	}
#endif

	if (option == 1)
	{
		// ...user doesn't want to do it
		if (mandatory)
		{
			// Mandatory update, user chose to not to update...
			// The login attemp is complete, startup should 
			// quit when detecting this.
			attemptComplete();

			// *REMOVE:Mani - Saving for reference...
			//LLAppViewer::instance()->forceQuit();
			// // Bump them back to the login screen.
			// //reset_login();
		}
		else
		{
			// Optional update, user chose to skip
			mSkipOptionalUpdate = true;
			reconnect();
		}
		return false;
	}
	
	LLSD query_map = LLSD::emptyMap();
	// *TODO place os string in a global constant
#if LL_WINDOWS  
	query_map["os"] = "win";
#elif LL_DARWIN
	query_map["os"] = "mac";
#elif LL_LINUX
	query_map["os"] = "lnx";
#elif LL_SOLARIS
	query_map["os"] = "sol";
#endif
	// *TODO change userserver to be grid on both viewer and sim, since
	// userserver no longer exists.
	query_map["userserver"] = LLViewerLogin::getInstance()->getGridLabel();
	query_map["channel"] = gSavedSettings.getString("VersionChannelName");
	// *TODO constantize this guy
	// *NOTE: This URL is also used in win_setup/lldownloader.cpp
	LLURI update_url = LLURI::buildHTTP("secondlife.com", 80, "update.php", query_map);
	
	if(LLAppViewer::sUpdaterInfo)
	{
		delete LLAppViewer::sUpdaterInfo;
	}
	LLAppViewer::sUpdaterInfo = new LLAppViewer::LLUpdaterInfo() ;
	
#if LL_WINDOWS
	LLAppViewer::sUpdaterInfo->mUpdateExePath = gDirUtilp->getTempFilename();
	if (LLAppViewer::sUpdaterInfo->mUpdateExePath.empty())
	{
		delete LLAppViewer::sUpdaterInfo ;
		LLAppViewer::sUpdaterInfo = NULL ;

		// We're hosed, bail
		LL_WARNS("AppInit") << "LLDir::getTempFilename() failed" << LL_ENDL;

		attemptComplete();
		// *REMOVE:Mani - Saving for reference...
		// LLAppViewer::instance()->forceQuit();
		return false;
	}

	LLAppViewer::sUpdaterInfo->mUpdateExePath += ".exe";

	std::string updater_source = gDirUtilp->getAppRODataDir();
	updater_source += gDirUtilp->getDirDelimiter();
	updater_source += "updater.exe";

	LL_DEBUGS("AppInit") << "Calling CopyFile source: " << updater_source
			<< " dest: " << LLAppViewer::sUpdaterInfo->mUpdateExePath
			<< LL_ENDL;


	if (!CopyFileA(updater_source.c_str(), LLAppViewer::sUpdaterInfo->mUpdateExePath.c_str(), FALSE))
	{
		delete LLAppViewer::sUpdaterInfo ;
		LLAppViewer::sUpdaterInfo = NULL ;

		LL_WARNS("AppInit") << "Unable to copy the updater!" << LL_ENDL;
		attemptComplete();
		// *REMOVE:Mani - Saving for reference...
		// LLAppViewer::instance()->forceQuit();
		return false;
	}

	// if a sim name was passed in via command line parameter (typically through a SLURL)
	if ( LLURLSimString::sInstance.mSimString.length() )
	{
		// record the location to start at next time
		gSavedSettings.setString( "NextLoginLocation", LLURLSimString::sInstance.mSimString ); 
	};

	LLAppViewer::sUpdaterInfo->mParams << "-url \"" << update_url.asString() << "\"";

	LL_DEBUGS("AppInit") << "Calling updater: " << LLAppViewer::sUpdaterInfo->mUpdateExePath << " " << LLAppViewer::sUpdaterInfo->mParams.str() << LL_ENDL;

	//Explicitly remove the marker file, otherwise we pass the lock onto the child process and things get weird.
	LLAppViewer::instance()->removeMarkerFile(); // In case updater fails

	// *NOTE:Mani The updater is spawned as the last thing before the WinMain exit.
	// see LLAppViewerWin32.cpp
	
#elif LL_DARWIN
	// if a sim name was passed in via command line parameter (typically through a SLURL)
	if ( LLURLSimString::sInstance.mSimString.length() )
	{
		// record the location to start at next time
		gSavedSettings.setString( "NextLoginLocation", LLURLSimString::sInstance.mSimString ); 
	};
	
	LLAppViewer::sUpdaterInfo->mUpdateExePath = "'";
	LLAppViewer::sUpdaterInfo->mUpdateExePath += gDirUtilp->getAppRODataDir();
	LLAppViewer::sUpdaterInfo->mUpdateExePath += "/mac-updater.app/Contents/MacOS/mac-updater' -url \"";
	LLAppViewer::sUpdaterInfo->mUpdateExePath += update_url.asString();
	LLAppViewer::sUpdaterInfo->mUpdateExePath += "\" -name \"";
	LLAppViewer::sUpdaterInfo->mUpdateExePath += LLAppViewer::instance()->getSecondLifeTitle();
	LLAppViewer::sUpdaterInfo->mUpdateExePath += "\" &";

	LL_DEBUGS("AppInit") << "Calling updater: " << LLAppViewer::sUpdaterInfo->mUpdateExePath << LL_ENDL;

	// Run the auto-updater.
	system(LLAppViewer::sUpdaterInfo->mUpdateExePath.c_str()); /* Flawfinder: ignore */

#elif LL_LINUX || LL_SOLARIS
	OSMessageBox(LLTrans::getString("MBNoAutoUpdate"), LLStringUtil::null, OSMB_OK);
#endif

	// *REMOVE:Mani - Saving for reference...
	// LLAppViewer::instance()->forceQuit();

	return false;
}

std::string construct_start_string()
{
	std::string start;
	if (LLURLSimString::parse())
	{
		// a startup URL was specified
		std::string unescaped_start = 
			STRINGIZE(  "uri:" 
						<< LLURLSimString::sInstance.mSimName << "&" 
						<< LLURLSimString::sInstance.mX << "&" 
						<< LLURLSimString::sInstance.mY << "&" 
						<< LLURLSimString::sInstance.mZ);
		start = xml_escape_string(unescaped_start);
	}
	else
	{
		start = gSavedSettings.getString("LoginLocation");
	}
	return start;
}
