/** 
 * @file lllogininstance.cpp
 * @brief Viewer's host for a login connection.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
#include "llviewercontrol.h"
#include "llversioninfo.h"
#include "llslurl.h"
#include "llstartup.h"
#include "llfloaterreg.h"
#include "llnotifications.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llprogressview.h"
#if LL_LINUX || LL_SOLARIS
#include "lltrans.h"
#endif
#include "llsecapi.h"
#include "llstartup.h"
#include "llmachineid.h"
#include "llupdaterservice.h"
#include "llevents.h"
#include "llnotificationsutil.h"
#include "llappviewer.h"

#include <boost/scoped_ptr.hpp>
#include <sstream>

class LLLoginInstance::Disposable {
public:
	virtual ~Disposable() {}
};

namespace {
	class MandatoryUpdateMachine:
		public LLLoginInstance::Disposable
	{
	public:
		MandatoryUpdateMachine(LLLoginInstance & loginInstance, LLUpdaterService & updaterService);
		
		void start(void);
		
	private:
		class State;
		class CheckingForUpdate;
		class Error;
		class ReadyToInstall; 
		class StartingUpdaterService;
		class WaitingForDownload;
		
		LLLoginInstance & mLoginInstance;
		boost::scoped_ptr<State> mState;
		LLUpdaterService & mUpdaterService;
		
		void setCurrentState(State * newState);
	};

	
	class MandatoryUpdateMachine::State {
	public:
		virtual ~State() {}
		virtual void enter(void) {}
		virtual void exit(void) {}
	};
	
	
	class MandatoryUpdateMachine::CheckingForUpdate:
	public MandatoryUpdateMachine::State
	{
	public:
		CheckingForUpdate(MandatoryUpdateMachine & machine);
		
		virtual void enter(void);
		virtual void exit(void);
		
	private:
		LLTempBoundListener mConnection;
		MandatoryUpdateMachine & mMachine;
		LLProgressView * mProgressView;
		
		bool onEvent(LLSD const & event);
	};
	
	
	class MandatoryUpdateMachine::Error:
	public MandatoryUpdateMachine::State
	{
	public:
		Error(MandatoryUpdateMachine & machine);
		
		virtual void enter(void);
		virtual void exit(void);
		void onButtonClicked(const LLSD &, const LLSD &);
		
	private:
		MandatoryUpdateMachine & mMachine;
	};
	
	
	class MandatoryUpdateMachine::ReadyToInstall:
	public MandatoryUpdateMachine::State
	{
	public:
		ReadyToInstall(MandatoryUpdateMachine & machine);
		
		virtual void enter(void);
		virtual void exit(void);
		
	private:
		MandatoryUpdateMachine & mMachine;
	};
	
	
	class MandatoryUpdateMachine::StartingUpdaterService:
	public MandatoryUpdateMachine::State
	{
	public:
		StartingUpdaterService(MandatoryUpdateMachine & machine);
		
		virtual void enter(void);
		virtual void exit(void);
		void onButtonClicked(const LLSD & uiform, const LLSD & result);
	private:
		MandatoryUpdateMachine & mMachine;
	};
	
	
	class MandatoryUpdateMachine::WaitingForDownload:
		public MandatoryUpdateMachine::State
	{
	public:
		WaitingForDownload(MandatoryUpdateMachine & machine);
		
		virtual void enter(void);
		virtual void exit(void);
		
	private:
		LLTempBoundListener mConnection;
		MandatoryUpdateMachine & mMachine;
		LLProgressView * mProgressView;
		
		bool onEvent(LLSD const & event);
	};
}

static const char * const TOS_REPLY_PUMP = "lllogininstance_tos_callback";
static const char * const TOS_LISTENER_NAME = "lllogininstance_tos";

std::string construct_start_string();



// MandatoryUpdateMachine
//-----------------------------------------------------------------------------


MandatoryUpdateMachine::MandatoryUpdateMachine(LLLoginInstance & loginInstance, LLUpdaterService & updaterService):
	mLoginInstance(loginInstance),
	mUpdaterService(updaterService)
{
	; // No op.
}


void MandatoryUpdateMachine::start(void)
{
	llinfos << "starting manditory update machine" << llendl;
	
	if(mUpdaterService.isChecking()) {
		switch(mUpdaterService.getState()) {
			case LLUpdaterService::UP_TO_DATE:
				mUpdaterService.stopChecking();
				mUpdaterService.startChecking();
				// Fall through.
			case LLUpdaterService::INITIAL:
			case LLUpdaterService::CHECKING_FOR_UPDATE:
				setCurrentState(new CheckingForUpdate(*this));
				break;
			case LLUpdaterService::TEMPORARY_ERROR:
				setCurrentState(new Error(*this));
				break;
			case LLUpdaterService::DOWNLOADING:
				setCurrentState(new WaitingForDownload(*this));
				break;
			case LLUpdaterService::TERMINAL:
				if(LLUpdaterService::updateReadyToInstall()) {
					setCurrentState(new ReadyToInstall(*this));
				} else {
					setCurrentState(new Error(*this));
				}
				break;
			case LLUpdaterService::FAILURE:
				setCurrentState(new Error(*this));
				break;
			default:
				llassert(!"unpossible case");
				break;
		}
	} else {
		setCurrentState(new StartingUpdaterService(*this));
	}
}


void MandatoryUpdateMachine::setCurrentState(State * newStatePointer)
{
	{
		boost::scoped_ptr<State> newState(newStatePointer);
		if(mState != 0) mState->exit();
		mState.swap(newState);
		
		// Old state will be deleted on exit from this block before the new state
		// is entered.
	}
	if(mState != 0) mState->enter();
}



// MandatoryUpdateMachine::CheckingForUpdate
//-----------------------------------------------------------------------------


MandatoryUpdateMachine::CheckingForUpdate::CheckingForUpdate(MandatoryUpdateMachine & machine):
	mMachine(machine)
{
	; // No op.
}


void MandatoryUpdateMachine::CheckingForUpdate::enter(void)
{
	llinfos << "entering checking for update" << llendl;
	
	mProgressView = gViewerWindow->getProgressView();
	mProgressView->setMessage("Looking for update...");
	mProgressView->setText("There is a required update for your Second Life installation.");
	mProgressView->setPercent(0);
	mProgressView->setVisible(true);
	mConnection = LLEventPumps::instance().obtain(LLUpdaterService::pumpName()).
		listen("MandatoryUpdateMachine::CheckingForUpdate", boost::bind(&MandatoryUpdateMachine::CheckingForUpdate::onEvent, this, _1));
}


void MandatoryUpdateMachine::CheckingForUpdate::exit(void)
{
}


bool MandatoryUpdateMachine::CheckingForUpdate::onEvent(LLSD const & event)
{
	if(event["type"].asInteger() == LLUpdaterService::STATE_CHANGE) {
		switch(event["state"].asInteger()) {
			case LLUpdaterService::DOWNLOADING:
				mMachine.setCurrentState(new WaitingForDownload(mMachine));
				break;
			case LLUpdaterService::TEMPORARY_ERROR:
			case LLUpdaterService::UP_TO_DATE:
			case LLUpdaterService::TERMINAL:
			case LLUpdaterService::FAILURE:
				mProgressView->setVisible(false);
				mMachine.setCurrentState(new Error(mMachine));
				break;
			case LLUpdaterService::INSTALLING:
				llassert(!"can't possibly be installing");
				break;
			default:
				break;
		}
	} else {
		; // Ignore.
	}
	
	return false;
}



// MandatoryUpdateMachine::Error
//-----------------------------------------------------------------------------


MandatoryUpdateMachine::Error::Error(MandatoryUpdateMachine & machine):
	mMachine(machine)
{
	; // No op.
}


void MandatoryUpdateMachine::Error::enter(void)
{
	llinfos << "entering error" << llendl;
	LLNotificationsUtil::add("FailedRequiredUpdateInstall", LLSD(), LLSD(), boost::bind(&MandatoryUpdateMachine::Error::onButtonClicked, this, _1, _2));
}


void MandatoryUpdateMachine::Error::exit(void)
{
	LLAppViewer::instance()->forceQuit();
}


void MandatoryUpdateMachine::Error::onButtonClicked(const LLSD &, const LLSD &)
{
	mMachine.setCurrentState(0);
}



// MandatoryUpdateMachine::ReadyToInstall
//-----------------------------------------------------------------------------


MandatoryUpdateMachine::ReadyToInstall::ReadyToInstall(MandatoryUpdateMachine & machine):
	mMachine(machine)
{
	; // No op.
}


void MandatoryUpdateMachine::ReadyToInstall::enter(void)
{
	llinfos << "entering ready to install" << llendl;
	// Open update ready dialog.
}


void MandatoryUpdateMachine::ReadyToInstall::exit(void)
{
	// Restart viewer.
}



// MandatoryUpdateMachine::StartingUpdaterService
//-----------------------------------------------------------------------------


MandatoryUpdateMachine::StartingUpdaterService::StartingUpdaterService(MandatoryUpdateMachine & machine):
	mMachine(machine)
{
	; // No op.
}


void MandatoryUpdateMachine::StartingUpdaterService::enter(void)
{
	llinfos << "entering start update service" << llendl;
	LLNotificationsUtil::add("UpdaterServiceNotRunning", LLSD(), LLSD(), boost::bind(&MandatoryUpdateMachine::StartingUpdaterService::onButtonClicked, this, _1, _2));
}


void MandatoryUpdateMachine::StartingUpdaterService::exit(void)
{
	; // No op.
}


void MandatoryUpdateMachine::StartingUpdaterService::onButtonClicked(const LLSD & uiform, const LLSD & result)
{
	if(result["OK_okcancelbuttons"].asBoolean()) {
		mMachine.mUpdaterService.startChecking(false);
		mMachine.setCurrentState(new CheckingForUpdate(mMachine));
	} else {
		LLAppViewer::instance()->forceQuit();
	}
}



// MandatoryUpdateMachine::WaitingForDownload
//-----------------------------------------------------------------------------


MandatoryUpdateMachine::WaitingForDownload::WaitingForDownload(MandatoryUpdateMachine & machine):
	mMachine(machine),
	mProgressView(0)
{
	; // No op.
}


void MandatoryUpdateMachine::WaitingForDownload::enter(void)
{
	llinfos << "entering waiting for download" << llendl;
	mProgressView = gViewerWindow->getProgressView();
	mProgressView->setMessage("Downloading update...");
	std::ostringstream stream;
	stream << "There is a required update for your Second Life installation." << std::endl <<
		"Version " << mMachine.mUpdaterService.updatedVersion();
	mProgressView->setText(stream.str());
	mProgressView->setPercent(0);
	mProgressView->setVisible(true);
	mConnection = LLEventPumps::instance().obtain(LLUpdaterService::pumpName()).
		listen("MandatoryUpdateMachine::CheckingForUpdate", boost::bind(&MandatoryUpdateMachine::WaitingForDownload::onEvent, this, _1));
}


void MandatoryUpdateMachine::WaitingForDownload::exit(void)
{
	mProgressView->setVisible(false);
}


bool MandatoryUpdateMachine::WaitingForDownload::onEvent(LLSD const & event)
{
	switch(event["type"].asInteger()) {
		case LLUpdaterService::DOWNLOAD_COMPLETE:
			mMachine.setCurrentState(new ReadyToInstall(mMachine));
			break;
		case LLUpdaterService::DOWNLOAD_ERROR:
			mMachine.setCurrentState(new Error(mMachine));
			break;
		case LLUpdaterService::PROGRESS: {
			double downloadSize = event["download_size"].asReal();
			double bytesDownloaded = event["bytes_downloaded"].asReal();
			mProgressView->setPercent(100. * bytesDownloaded / downloadSize);
			break;
		}
		default:
			break;
	}

	return false;
}



// LLLoginInstance
//-----------------------------------------------------------------------------


LLLoginInstance::LLLoginInstance() :
	mLoginModule(new LLLogin()),
	mNotifications(NULL),
	mLoginState("offline"),
	mSkipOptionalUpdate(false),
	mAttemptComplete(false),
	mTransferRate(0.0f),
	mDispatcher("LLLoginInstance", "change"),
	mUpdaterService(0)
{
	mLoginModule->getEventPump().listen("lllogininstance", 
		boost::bind(&LLLoginInstance::handleLoginEvent, this, _1));
	// This internal use of LLEventDispatcher doesn't really need
	// per-function descriptions.
	mDispatcher.add("fail.login", "", boost::bind(&LLLoginInstance::handleLoginFailure, this, _1));
	mDispatcher.add("connect",    "", boost::bind(&LLLoginInstance::handleLoginSuccess, this, _1));
	mDispatcher.add("disconnect", "", boost::bind(&LLLoginInstance::handleDisconnect, this, _1));
	mDispatcher.add("indeterminate", "", boost::bind(&LLLoginInstance::handleIndeterminate, this, _1));
}

LLLoginInstance::~LLLoginInstance()
{
}

void LLLoginInstance::connect(LLPointer<LLCredential> credentials)
{
	std::vector<std::string> uris;
	LLGridManager::getInstance()->getLoginURIs(uris);
	connect(uris.front(), credentials);
}

void LLLoginInstance::connect(const std::string& uri, LLPointer<LLCredential> credentials)
{
	mAttemptComplete = false; // Reset attempt complete at this point!
	constructAuthParams(credentials);
	mLoginModule->connect(uri, mRequestData);
}

void LLLoginInstance::reconnect()
{
	// Sort of like connect, only using the pre-existing
	// request params.
	std::vector<std::string> uris;
	LLGridManager::getInstance()->getLoginURIs(uris);
	mLoginModule->connect(uris.front(), mRequestData);
	gViewerWindow->setShowProgress(true);
}

void LLLoginInstance::disconnect()
{
	mAttemptComplete = false; // Reset attempt complete at this point!
	mRequestData.clear();
	mLoginModule->disconnect();
}

LLSD LLLoginInstance::getResponse() 
{
	return mResponseData; 
}

void LLLoginInstance::constructAuthParams(LLPointer<LLCredential> user_credential)
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
	requested_options.append("display_names");
	requested_options.append("event_categories");
	requested_options.append("event_notifications");
	requested_options.append("classified_categories");
	requested_options.append("adult_compliant"); 
	//requested_options.append("inventory-targets");
	requested_options.append("buddy-list");
	requested_options.append("newuser-config");
	requested_options.append("ui-config");

	//send this info to login.cgi for stats gathering 
	//since viewerstats isn't reliable enough
	if (gSavedSettings.getString("SessionSettingsFile").empty())
	{
		requested_options.append("advanced-mode");
	}
	else
	{
		requested_options.append("basic-mode");
	}

#endif
	requested_options.append("max-agent-groups");	
	requested_options.append("map-server-url");	
	requested_options.append("voice-config");
	requested_options.append("tutorial_setting");
	requested_options.append("login-flags");
	requested_options.append("global-textures");
	if(gSavedSettings.getBOOL("ConnectAsGod"))
	{
		gSavedSettings.setBOOL("UseDebugMenus", TRUE);
		requested_options.append("god-connect");
	}
	
	// (re)initialize the request params with creds.
	LLSD request_params = user_credential->getLoginParams();

	char hashed_unique_id_string[MD5HEX_STR_SIZE];		/* Flawfinder: ignore */
	LLMD5 hashed_unique_id;
	unsigned char unique_id[MAC_ADDRESS_BYTES];
	if(LLUUID::getNodeID(unique_id) == 0) {
		if(LLMachineID::getUniqueID(unique_id, sizeof(unique_id)) == 0) {
			llerrs << "Failed to get an id; cannot uniquely identify this machine." << llendl;
		}
	}
	hashed_unique_id.update(unique_id, MAC_ADDRESS_BYTES);
	hashed_unique_id.finalize();
	hashed_unique_id.hex_digest(hashed_unique_id_string);
	
	request_params["start"] = construct_start_string();
	request_params["skipoptional"] = mSkipOptionalUpdate;
	request_params["agree_to_tos"] = false; // Always false here. Set true in 
	request_params["read_critical"] = false; // handleTOSResponse
	request_params["last_exec_event"] = mLastExecEvent;
	request_params["mac"] = hashed_unique_id_string;
	request_params["version"] = LLVersionInfo::getChannelAndVersion(); // Includes channel name
	request_params["channel"] = LLVersionInfo::getChannel();
	request_params["id0"] = mSerialNumber;
	request_params["host_id"] = gSavedSettings.getString("HostID");

	mRequestData.clear();
	mRequestData["method"] = "login_to_simulator";
	mRequestData["params"] = request_params;
	mRequestData["options"] = requested_options;

	mRequestData["cfg_srv_timeout"] = gSavedSettings.getF32("LoginSRVTimeout");
	mRequestData["cfg_srv_pump"] = gSavedSettings.getString("LoginSRVPump");
}

bool LLLoginInstance::handleLoginEvent(const LLSD& event)
{
	LL_DEBUGS("LLLogin") << "LoginListener called!: \n" << event << LL_ENDL;

	if(!(event.has("state") && event.has("change") && event.has("progress")))
	{
		llerrs << "Unknown message from LLLogin: " << event << llendl;
	}

	mLoginState = event["state"].asString();
	mResponseData = event["data"];
	
	if(event.has("transfer_rate"))
	{
		mTransferRate = event["transfer_rate"].asReal();
	}

	

	// Call the method registered in constructor, if any, for more specific
	// handling
	mDispatcher.try_call(event);
	return false;
}

void LLLoginInstance::handleLoginFailure(const LLSD& event)
{
	// Login has failed. 
	// Figure out why and respond...
	LLSD response = event["data"];
	std::string reason_response = response["reason"].asString();
	std::string message_response = response["message"].asString();
	// For the cases of critical message or TOS agreement,
	// start the TOS dialog. The dialog response will be handled
	// by the LLLoginInstance::handleTOSResponse() callback.
	// The callback intiates the login attempt next step, either 
	// to reconnect or to end the attempt in failure.
	if(reason_response == "tos")
	{
		LLSD data(LLSD::emptyMap());
		data["message"] = message_response;
		data["reply_pump"] = TOS_REPLY_PUMP;
		gViewerWindow->setShowProgress(FALSE);
		LLFloaterReg::showInstance("message_tos", data);
		LLEventPumps::instance().obtain(TOS_REPLY_PUMP)
			.listen(TOS_LISTENER_NAME,
					boost::bind(&LLLoginInstance::handleTOSResponse, 
								this, _1, "agree_to_tos"));
	}
	else if(reason_response == "critical")
	{
		LLSD data(LLSD::emptyMap());
		data["message"] = message_response;
		data["reply_pump"] = TOS_REPLY_PUMP;
		if(response.has("error_code"))
		{
			data["error_code"] = response["error_code"];
		}
		if(response.has("certificate"))
		{
			data["certificate"] = response["certificate"];
		}
		
		gViewerWindow->setShowProgress(FALSE);
		LLFloaterReg::showInstance("message_critical", data);
		LLEventPumps::instance().obtain(TOS_REPLY_PUMP)
			.listen(TOS_LISTENER_NAME,
					boost::bind(&LLLoginInstance::handleTOSResponse, 
								this, _1, "read_critical"));
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

void LLLoginInstance::handleLoginSuccess(const LLSD& event)
{
	if(gSavedSettings.getBOOL("ForceMandatoryUpdate"))
	{
		LLSD response = event["data"];
		std::string message_response = response["message"].asString();

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
}

void LLLoginInstance::handleDisconnect(const LLSD& event)
{
    // placeholder
}

void LLLoginInstance::handleIndeterminate(const LLSD& event)
{
	// The indeterminate response means that the server
	// gave the viewer a new url and params to try.
	// The login module handles the retry, but it gives us the
	// server response so that we may show
	// the user some status.
	LLSD message = event.get("data").get("message");
	if(message.isDefined())
	{
		LLSD progress_update;
		progress_update["desc"] = message;
		LLEventPumps::getInstance()->obtain("LLProgressView").post(progress_update);
	}
}

bool LLLoginInstance::handleTOSResponse(bool accepted, const std::string& key)
{
	if(accepted)
	{	
		// Set the request data to true and retry login.
		mRequestData["params"][key] = true; 
		reconnect();
	}
	else
	{
		attemptComplete();
	}

	LLEventPumps::instance().obtain(TOS_REPLY_PUMP).stopListening(TOS_LISTENER_NAME);
	return true;
}


void LLLoginInstance::updateApp(bool mandatory, const std::string& auth_msg)
{
	if(mandatory)
	{
		gViewerWindow->setShowProgress(false);
		MandatoryUpdateMachine * machine = new MandatoryUpdateMachine(*this, *mUpdaterService);
		mUpdateStateMachine.reset(machine);
		machine->start();
		return;
	}
	
	// store off config state, as we might quit soon
	gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);	
	LLUIColorTable::instance().saveUserSettings();

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
 We're constructing one of the following 9 strings here:
	 "DownloadWindowsMandatory"
	 "DownloadWindowsReleaseForDownload"
	 "DownloadWindows"
	 "DownloadMacMandatory"
	 "DownloadMacReleaseForDownload"
	 "DownloadMac"
	 "DownloadLinuxMandatory"
	 "DownloadLinuxReleaseForDownload"
	 "DownloadLinux"
 
 I've called them out explicitly in this comment so that they can be grepped for.
 */
	std::string notification_name = "Download";
	
#if LL_WINDOWS
	notification_name += "Windows";
#elif LL_DARWIN
	notification_name += "Mac";
#else
	notification_name += "Linux";
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

	if(mNotifications)
	{
		mNotifications->add(notification_name, args, payload, 
			boost::bind(&LLLoginInstance::updateDialogCallback, this, _1, _2));

		gViewerWindow->setShowProgress(false);
	}
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
	
 	if(mUpdaterLauncher)
  	{
 		mUpdaterLauncher();
  	}
  
 	attemptComplete();

	return false;
}

std::string construct_start_string()
{
	std::string start;
	LLSLURL start_slurl = LLStartUp::getStartSLURL();
	switch(start_slurl.getType())
	{
		case LLSLURL::LOCATION:
		{
			// a startup URL was specified
			LLVector3 position = start_slurl.getPosition();
			std::string unescaped_start = 
			STRINGIZE(  "uri:" 
					  << start_slurl.getRegion() << "&" 
						<< position[VX] << "&" 
						<< position[VY] << "&" 
						<< position[VZ]);
			start = xml_escape_string(unescaped_start);
			break;
		}
		case LLSLURL::HOME_LOCATION:
		{
			start = "home";
			break;
		}
		default:
		{
			start = "last";
		}
	}
	return start;
}

