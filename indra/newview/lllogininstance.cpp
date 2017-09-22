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
#include "stringize.h"
#include "llsdserialize.h"

// llmessage (!)
#include "llfiltersd2xmlrpc.h" // for xml_escape_string()

// login
#include "lllogin.h"

// newview
#include "llhasheduniqueid.h"
#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llversioninfo.h"
#include "llslurl.h"
#include "llstartup.h"
#include "llfloaterreg.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llprogressview.h"
#include "llsecapi.h"
#include "llstartup.h"
#include "llmachineid.h"
#include "llevents.h"
#include "llappviewer.h"

#include <boost/scoped_ptr.hpp>
#include <sstream>

class LLLoginInstance::Disposable {
public:
	virtual ~Disposable() {}
};

static const char * const TOS_REPLY_PUMP = "lllogininstance_tos_callback";
static const char * const TOS_LISTENER_NAME = "lllogininstance_tos";

std::string construct_start_string();

// LLLoginInstance
//-----------------------------------------------------------------------------


LLLoginInstance::LLLoginInstance() :
	mLoginModule(new LLLogin()),
	mNotifications(NULL),
	mLoginState("offline"),
	mAttemptComplete(false),
	mTransferRate(0.0f),
	mDispatcher("LLLoginInstance", "change")
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

void LLLoginInstance::setPlatformInfo(const std::string platform,
									  const std::string platform_version,
                                      const std::string platform_name)
{
	mPlatform = platform;
	mPlatformVersion = platform_version;
    mPlatformVersionName = platform_name;
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
	requested_options.append("buddy-list");
	requested_options.append("newuser-config");
	requested_options.append("ui-config");

	//send this info to login.cgi for stats gathering 
	//since viewerstats isn't reliable enough
	requested_options.append("advanced-mode");

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
	
	LLSD request_params;

    unsigned char hashed_unique_id_string[MD5HEX_STR_SIZE];
    if ( ! llHashedUniqueID(hashed_unique_id_string) )
    {

		LL_WARNS("LLLogin") << "Not providing a unique id in request params" << LL_ENDL;

	}
	request_params["start"] = construct_start_string();
	request_params["agree_to_tos"] = false; // Always false here. Set true in 
	request_params["read_critical"] = false; // handleTOSResponse
	request_params["last_exec_event"] = mLastExecEvent;
	request_params["last_exec_duration"] = mLastExecDuration;
	request_params["mac"] = (char*)hashed_unique_id_string;
	request_params["version"] = LLVersionInfo::getVersion();
	request_params["channel"] = LLVersionInfo::getChannel();
	request_params["platform"] = mPlatform;
	request_params["platform_version"] = mPlatformVersion;
	request_params["address_size"] = ADDRESS_SIZE;
	request_params["platform_string"] = mPlatformVersionName;
	request_params["id0"] = mSerialNumber;
	request_params["host_id"] = gSavedSettings.getString("HostID");
	request_params["extended_errors"] = true; // request message_id and message_args

    // log request_params _before_ adding the credentials   
    LL_DEBUGS("LLLogin") << "Login parameters: " << LLSDOStreamer<LLSDNotationFormatter>(request_params) << LL_ENDL;

    // Copy the credentials into the request after logging the rest
    LLSD credentials(user_credential->getLoginParams());
    for (LLSD::map_const_iterator it = credentials.beginMap();
         it != credentials.endMap();
         it++
         )
    {
        request_params[it->first] = it->second;
    }

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
		LL_ERRS("LLLogin") << "Unknown message from LLLogin: " << event << LL_ENDL;
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
    LL_DEBUGS("LLLogin") << "reason " << reason_response
                         << " message " << message_response
                         << LL_ENDL;
    // For the cases of critical message or TOS agreement,
    // start the TOS dialog. The dialog response will be handled
    // by the LLLoginInstance::handleTOSResponse() callback.
    // The callback intiates the login attempt next step, either 
    // to reconnect or to end the attempt in failure.
    if(reason_response == "tos")
    {
        LL_INFOS("LLLogin") << " ToS" << LL_ENDL;

        LLSD data(LLSD::emptyMap());
        data["message"] = message_response;
        data["reply_pump"] = TOS_REPLY_PUMP;
        if (gViewerWindow)
            gViewerWindow->setShowProgress(FALSE);
        LLFloaterReg::showInstance("message_tos", data);
        LLEventPumps::instance().obtain(TOS_REPLY_PUMP)
            .listen(TOS_LISTENER_NAME,
                    boost::bind(&LLLoginInstance::handleTOSResponse, 
                                this, _1, "agree_to_tos"));
    }
    else if(reason_response == "critical")
    {
        LL_INFOS("LLLogin") << "LLLoginInstance::handleLoginFailure Crit" << LL_ENDL;

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
        
        if (gViewerWindow)
            gViewerWindow->setShowProgress(FALSE);

        LLFloaterReg::showInstance("message_critical", data);
        LLEventPumps::instance().obtain(TOS_REPLY_PUMP)
            .listen(TOS_LISTENER_NAME,
                    boost::bind(&LLLoginInstance::handleTOSResponse, 
                                this, _1, "read_critical"));
    }
    else if(reason_response == "update")
    {
        // This shouldn't happen - the viewer manager should have forced an update; 
        // possibly the user ran the viewer directly and bypassed the update check
        std::string required_version = response["message_args"]["VERSION"];
        LL_WARNS("LLLogin") << "Login failed because an update to version " << required_version << " is required." << LL_ENDL;

        if (gViewerWindow)
            gViewerWindow->setShowProgress(FALSE);

        LLSD data(LLSD::emptyMap());
        data["VERSION"] = required_version;
        LLNotificationsUtil::add("RequiredUpdate", data, LLSD::emptyMap(), boost::bind(&LLLoginInstance::handleLoginDisallowed, this, _1, _2));
    }
    else if(   reason_response == "key"
            || reason_response == "presence"
            || reason_response == "connect"
            )
    {
        // these are events that have already been communicated elsewhere
        attemptComplete();
    }
    else
    {   
        LL_WARNS("LLLogin") << "Login failed for an unknown reason: " << LLSDOStreamer<LLSDNotationFormatter>(response) << LL_ENDL;

        if (gViewerWindow)
            gViewerWindow->setShowProgress(FALSE);

        LLNotificationsUtil::add("LoginFailedUnknown", LLSD::emptyMap(), LLSD::emptyMap(), boost::bind(&LLLoginInstance::handleLoginDisallowed, this, _1, _2));
    }   
}

void LLLoginInstance::handleLoginDisallowed(const LLSD& notification, const LLSD& response)
{
    attemptComplete();
}

void LLLoginInstance::handleLoginSuccess(const LLSD& event)
{
	LL_INFOS("LLLogin") << "LLLoginInstance::handleLoginSuccess" << LL_ENDL;

	attemptComplete();
}

void LLLoginInstance::handleDisconnect(const LLSD& event)
{
    // placeholder

	LL_INFOS("LLLogin") << "LLLoginInstance::handleDisconnect placeholder " << LL_ENDL;
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
		LL_INFOS("LLLogin") << "LLLoginInstance::handleIndeterminate " << message.asString() << LL_ENDL;

		LLSD progress_update;
		progress_update["desc"] = message;
		LLEventPumps::getInstance()->obtain("LLProgressView").post(progress_update);
	}
}

bool LLLoginInstance::handleTOSResponse(bool accepted, const std::string& key)
{
	if(accepted)
	{	
		LL_INFOS("LLLogin") << "LLLoginInstance::handleTOSResponse: accepted" << LL_ENDL;

		// Set the request data to true and retry login.
		mRequestData["params"][key] = true; 
		reconnect();
	}
	else
	{
		LL_INFOS("LLLogin") << "LLLoginInstance::handleTOSResponse: attemptComplete" << LL_ENDL;

		attemptComplete();
	}

	LLEventPumps::instance().obtain(TOS_REPLY_PUMP).stopListening(TOS_LISTENER_NAME);
	return true;
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

