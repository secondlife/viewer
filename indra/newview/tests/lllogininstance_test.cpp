/**
 * @file   lllogininstance_test.cpp
 * @brief  Test for lllogininstance.cpp.
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

// Precompiled header
#include "../llviewerprecompiledheaders.h"
// Own header
#include "../llsecapi.h"
#include "../llviewernetwork.h"
#include "../lllogininstance.h"

// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"
#include "llevents.h"

#if defined(LL_WINDOWS)
#pragma warning(disable: 4355)      // using 'this' in base-class ctor initializer expr
#pragma warning(disable: 4702)      // disable 'unreachable code' so we can safely use skip().
#endif

// Constants
const std::string VIEWERLOGIN_URI("viewerlogin_uri");
const std::string VIEWERLOGIN_GRIDLABEL("viewerlogin_grid");

const std::string APPVIEWER_SERIALNUMBER("appviewer_serialno");

const std::string VIEWERLOGIN_CHANNEL("invalid_channel");
const std::string VIEWERLOGIN_VERSION_CHANNEL("invalid_version");

// Link seams.

//-----------------------------------------------------------------------------
static LLEventStream gTestPump("test_pump");

#include "../llslurl.h"
#include "../llstartup.h"
LLSLURL LLStartUp::sStartSLURL;

#include "lllogin.h"

static std::string gLoginURI;
static LLSD gLoginCreds;
static bool gDisconnectCalled = false;

#include "../llviewerwindow.h"
void LLViewerWindow::setShowProgress(BOOL show) {}
LLProgressView * LLViewerWindow::getProgressView(void) const { return 0; }

LLViewerWindow* gViewerWindow;

class LLLogin::Impl
{
};
LLLogin::LLLogin() {}
LLLogin::~LLLogin() {}
LLEventPump& LLLogin::getEventPump() { return gTestPump; }
void LLLogin::connect(const std::string& uri, const LLSD& credentials) 
{
	gLoginURI = uri;
	gLoginCreds = credentials;
}

void LLLogin::disconnect() 
{
	gDisconnectCalled = true;
}

LLSD LLCredential::getLoginParams()
{
	LLSD result = LLSD::emptyMap();

	// legacy credential
	result["passwd"] = "$1$testpasssd";
	result["first"] = "myfirst";
	result["last"] ="mylast";
	return result;
}
void LLCredential::identifierType(std::string &idType)
{
}

void LLCredential::authenticatorType(std::string &idType)
{
}

//-----------------------------------------------------------------------------
#include "../llviewernetwork.h"
LLGridManager::~LLGridManager()
{
}

bool LLGridManager::addGrid(LLSD& grid_data)
{
	return true;
}
LLGridManager::LLGridManager()
:
	mIsInProductionGrid(false)
{	
}

void LLGridManager::getLoginURIs(std::vector<std::string>& uris)
{
	uris.push_back(VIEWERLOGIN_URI);
}

void LLGridManager::addSystemGrid(const std::string& label, 
								  const std::string& name, 
								  const std::string& login, 
								  const std::string& helper,
								  const std::string& login_page,
								  const std::string& login_id)
{
}
std::map<std::string, std::string> LLGridManager::getKnownGrids()
{
	std::map<std::string, std::string> result;
	return result;
}

void LLGridManager::setGridChoice(const std::string& grid_name)
{
}

bool LLGridManager::isInProductionGrid()
{
	return false;
}

std::string LLGridManager::getSLURLBase(const std::string& grid_name)
{
	return "myslurl";
}
std::string LLGridManager::getAppSLURLBase(const std::string& grid_name)
{
	return "myappslurl";
}

//-----------------------------------------------------------------------------
#include "../llviewercontrol.h"
LLControlGroup gSavedSettings("Global");

LLControlGroup::LLControlGroup(const std::string& name) :
	LLInstanceTracker<LLControlGroup, std::string>(name){}
LLControlGroup::~LLControlGroup() {}
void LLControlGroup::setBOOL(const std::string& name, BOOL val) {}
BOOL LLControlGroup::getBOOL(const std::string& name) { return FALSE; }
F32 LLControlGroup::getF32(const std::string& name) { return 0.0f; }
U32 LLControlGroup::saveToFile(const std::string& filename, BOOL nondefault_only) { return 1; }
void LLControlGroup::setString(const std::string& name, const std::string& val) {}
std::string LLControlGroup::getString(const std::string& name) { return "test_string"; }
BOOL LLControlGroup::declareBOOL(const std::string& name, BOOL initial_val, const std::string& comment, BOOL persist) { return TRUE; }
BOOL LLControlGroup::declareString(const std::string& name, const std::string &initial_val, const std::string& comment, BOOL persist) { return TRUE; }

#include "lluicolortable.h"
void LLUIColorTable::saveUserSettings(void)const {}

//-----------------------------------------------------------------------------
#include "../llversioninfo.h"
const std::string &LLVersionInfo::getChannelAndVersion() { return VIEWERLOGIN_VERSION_CHANNEL; }
const std::string &LLVersionInfo::getChannel() { return VIEWERLOGIN_CHANNEL; }

//-----------------------------------------------------------------------------
#include "../llappviewer.h"
void LLAppViewer::forceQuit(void) {}
LLAppViewer * LLAppViewer::sInstance = 0;

//-----------------------------------------------------------------------------
#include "llnotificationsutil.h"
LLNotificationPtr LLNotificationsUtil::add(const std::string& name, 
					  const LLSD& substitutions, 
					  const LLSD& payload, 
					  boost::function<void (const LLSD&, const LLSD&)> functor) { return LLNotificationPtr((LLNotification*)0); }


//-----------------------------------------------------------------------------
#include "llupdaterservice.h"

std::string const & LLUpdaterService::pumpName(void)
{
	static std::string wakka = "wakka wakka wakka";
	return wakka;
}
bool LLUpdaterService::updateReadyToInstall(void) { return false; }
void LLUpdaterService::initialize(const std::string& protocol_version,
				const std::string& url, 
				const std::string& path,
				const std::string& channel,
								  const std::string& version) {}

void LLUpdaterService::setCheckPeriod(unsigned int seconds) {}
void LLUpdaterService::startChecking(bool install_if_ready) {}
void LLUpdaterService::stopChecking() {}
bool LLUpdaterService::isChecking() { return false; }
LLUpdaterService::eUpdaterState LLUpdaterService::getState() { return INITIAL; }
std::string LLUpdaterService::updatedVersion() { return ""; }

//-----------------------------------------------------------------------------
#include "llnotifications.h"
#include "llfloaterreg.h"
static std::string gTOSType;
static LLEventPump * gTOSReplyPump = NULL;

//static
LLFloater* LLFloaterReg::showInstance(const std::string& name, const LLSD& key, BOOL focus)
{
	gTOSType = name;
	gTOSReplyPump = &LLEventPumps::instance().obtain(key["reply_pump"]);
	return NULL;
}

//----------------------------------------------------------------------------
#include "../llprogressview.h"
void LLProgressView::setText(std::string const &){}
void LLProgressView::setPercent(float){}
void LLProgressView::setMessage(std::string const &){}

//-----------------------------------------------------------------------------
// LLNotifications
class MockNotifications : public LLNotificationsInterface
{
	boost::function<void (const LLSD&, const LLSD&)> mResponder;
	int mAddedCount;

public: 
	MockNotifications() :
		mResponder(0),
		mAddedCount(0)
	{
	}

	virtual ~MockNotifications() {}

	/* virtual */ LLNotificationPtr add(
					const std::string& name,
					const LLSD& substitutions,
					const LLSD& payload, 
					LLNotificationFunctorRegistry::ResponseFunctor functor)
	{
		mResponder = functor;
		mAddedCount++;
		return LLNotificationPtr((LLNotification*)NULL);
	}

	void sendYesResponse()
	{
		LLSD notification;
		LLSD response;
		response = 1;
		mResponder(notification, response);
	}

	void sendNoResponse()
	{
		LLSD notification;
		LLSD response;
		response = 2;
		mResponder(notification, response);
	}

	void sendBogusResponse()
	{
		LLSD notification;
		LLSD response;
		response = 666;
		mResponder(notification, response);
	}

	int addedCount() { return mAddedCount; }
};

S32 LLNotification::getSelectedOption(const LLSD& notification, const LLSD& response)
{
	return response.asInteger();
}

//-----------------------------------------------------------------------------
#include "../llmachineid.h"
unsigned char gMACAddress[MAC_ADDRESS_BYTES] = {77,21,46,31,89,2};

S32 LLMachineID::getUniqueID(unsigned char *unique_id, size_t len)
{
	memcpy(unique_id, gMACAddress, len);
	return 1;
}
//-----------------------------------------------------------------------------
// misc
std::string xml_escape_string(const std::string& in)
{
	return in;
}

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct lllogininstance_data
    {
		lllogininstance_data() : logininstance(LLLoginInstance::getInstance())
		{
			// Global initialization
			gLoginURI.clear();
			gLoginCreds.clear();
			gDisconnectCalled = false;

			gTOSType = ""; // Set to invalid value.
			gTOSReplyPump = 0; // clear the callback.


			gSavedSettings.declareBOOL("NoInventoryLibrary", FALSE, "", FALSE);
			gSavedSettings.declareBOOL("ConnectAsGod", FALSE, "", FALSE);
			gSavedSettings.declareBOOL("UseDebugMenus", FALSE, "", FALSE);
			gSavedSettings.declareBOOL("ForceMandatoryUpdate", FALSE, "", FALSE);
			gSavedSettings.declareString("ClientSettingsFile", "test_settings.xml", "", FALSE);
			gSavedSettings.declareString("NextLoginLocation", "", "", FALSE);
			gSavedSettings.declareBOOL("LoginLastLocation", FALSE, "", FALSE);

			LLSD authenticator = LLSD::emptyMap();
			LLSD identifier = LLSD::emptyMap();
			identifier["type"] = "agent";
			identifier["first_name"] = "testfirst";
			identifier["last_name"] = "testlast";
			authenticator["passwd"] = "testpass";
			agentCredential = new LLCredential();
			agentCredential->setCredentialData(identifier, authenticator);
			
			authenticator = LLSD::emptyMap();
			identifier = LLSD::emptyMap();
			identifier["type"] = "account";
			identifier["username"] = "testuser";
			authenticator["secret"] = "testsecret";
			accountCredential = new LLCredential();
			accountCredential->setCredentialData(identifier, authenticator);			

			logininstance->setNotificationsInterface(&notifications);
		}

		LLLoginInstance* logininstance;
		LLPointer<LLCredential> agentCredential;
		LLPointer<LLCredential> accountCredential;
		MockNotifications notifications;
    };

    typedef test_group<lllogininstance_data> lllogininstance_group;
    typedef lllogininstance_group::object lllogininstance_object;
    lllogininstance_group llsdmgr("LLLoginInstance");

    template<> template<>
    void lllogininstance_object::test<1>()
    {
		set_test_name("Test Simple Success And Disconnect");

		// Test default connect.
		logininstance->connect(agentCredential);

		ensure_equals("Default connect uri", gLoginURI, VIEWERLOGIN_URI); 

		// Dummy success response.
		LLSD response;
		response["state"] = "online";
		response["change"] = "connect";
		response["progress"] = 1.0;
		response["transfer_rate"] = 7;
		response["data"] = "test_data";

		gTestPump.post(response);

		ensure("Success response", logininstance->authSuccess());
		ensure_equals("Test Response Data", logininstance->getResponse().asString(), "test_data");

		logininstance->disconnect();

		ensure_equals("Called Login Module Disconnect", gDisconnectCalled, true);

		response.clear();
		response["state"] = "offline";
		response["change"] = "disconnect";
		response["progress"] = 0.0;
		response["transfer_rate"] = 0;
		response["data"] = "test_data";

		gTestPump.post(response);

		ensure("Disconnected", !(logininstance->authSuccess()));
    }

    template<> template<>
    void lllogininstance_object::test<2>()
    {
		set_test_name("Test User TOS/Critical message Interaction");

		const std::string test_uri = "testing-uri";

		// Test default connect.
		logininstance->connect(test_uri, agentCredential);

		// connect should call LLLogin::connect to init gLoginURI and gLoginCreds.
		ensure_equals("Default connect uri", gLoginURI, "testing-uri"); 
		ensure_equals("Default for agree to tos", gLoginCreds["params"]["agree_to_tos"].asBoolean(), false);
		ensure_equals("Default for read critical", gLoginCreds["params"]["read_critical"].asBoolean(), false);

		// TOS failure response.
		LLSD response;
		response["state"] = "offline";
		response["change"] = "fail.login";
		response["progress"] = 0.0;
		response["transfer_rate"] = 7;
		response["data"]["reason"] = "tos";
		gTestPump.post(response);

		ensure_equals("TOS Dialog type", gTOSType, "message_tos");
		ensure("TOS callback given", gTOSReplyPump != 0);
		gTOSReplyPump->post(false); // Call callback denying TOS.
		ensure("No TOS, failed auth", logininstance->authFailure());

		// Start again.
		logininstance->connect(test_uri, agentCredential);
		gTestPump.post(response); // Fail for tos again.
		gTOSReplyPump->post(true); // Accept tos, should reconnect w/ agree_to_tos.
		ensure_equals("Accepted agree to tos", gLoginCreds["params"]["agree_to_tos"].asBoolean(), true);
		ensure("Incomplete login status", !logininstance->authFailure() && !logininstance->authSuccess());
	
		// Fail connection, attempt connect again.
		// The new request should have reset agree to tos to default.
		response["data"]["reason"] = "key"; // bad creds.
		gTestPump.post(response);
		ensure("TOS auth failure", logininstance->authFailure());

		logininstance->connect(test_uri, agentCredential);
		ensure_equals("Reset to default for agree to tos", gLoginCreds["params"]["agree_to_tos"].asBoolean(), false);

		// Critical Message failure response.
		logininstance->connect(test_uri, agentCredential);
		response["data"]["reason"] = "critical"; // Change response to "critical message"
		gTestPump.post(response);

		ensure_equals("TOS Dialog type", gTOSType, "message_critical");
		ensure("TOS callback given", gTOSReplyPump != 0);
		gTOSReplyPump->post(true); 
		ensure_equals("Accepted read critical message", gLoginCreds["params"]["read_critical"].asBoolean(), true);
		ensure("Incomplete login status", !logininstance->authFailure() && !logininstance->authSuccess());

		// Fail then attempt new connection
		response["data"]["reason"] = "key"; // bad creds.
		gTestPump.post(response);
		ensure("TOS auth failure", logininstance->authFailure());
		logininstance->connect(test_uri, agentCredential);
		ensure_equals("Default for agree to tos", gLoginCreds["params"]["read_critical"].asBoolean(), false);
	}

    template<> template<>
    void lllogininstance_object::test<3>()
    {
		skip();
		
		set_test_name("Test Mandatory Update User Accepts");

		// Part 1 - Mandatory Update, with User accepts response.
		// Test connect with update needed.
		logininstance->connect(agentCredential);

		ensure_equals("Default connect uri", gLoginURI, VIEWERLOGIN_URI); 

		// Update needed failure response.
		LLSD response;
		response["state"] = "offline";
		response["change"] = "fail.login";
		response["progress"] = 0.0;
		response["transfer_rate"] = 7;
		response["data"]["reason"] = "update";
		gTestPump.post(response);

		ensure_equals("Notification added", notifications.addedCount(), 1);

		notifications.sendYesResponse();

		ensure("Disconnected", !(logininstance->authSuccess()));
	}

	template<> template<>
    void lllogininstance_object::test<4>()
    {
		skip();
		
		set_test_name("Test Mandatory Update User Decline");

		// Test connect with update needed.
		logininstance->connect(agentCredential);

		ensure_equals("Default connect uri", gLoginURI, VIEWERLOGIN_URI); 

		// Update needed failure response.
		LLSD response;
		response["state"] = "offline";
		response["change"] = "fail.login";
		response["progress"] = 0.0;
		response["transfer_rate"] = 7;
		response["data"]["reason"] = "update";
		gTestPump.post(response);

		ensure_equals("Notification added", notifications.addedCount(), 1);
		notifications.sendNoResponse();

		ensure("Disconnected", !(logininstance->authSuccess()));
	}

	template<> template<>
    void lllogininstance_object::test<6>()
    {
		set_test_name("Test Optional Update User Accept");

		// Part 3 - Mandatory Update, with bogus response.
		// Test connect with update needed.
		logininstance->connect(agentCredential);

		ensure_equals("Default connect uri", gLoginURI, VIEWERLOGIN_URI); 

		// Update needed failure response.
		LLSD response;
		response["state"] = "offline";
		response["change"] = "fail.login";
		response["progress"] = 0.0;
		response["transfer_rate"] = 7;
		response["data"]["reason"] = "optional";
		gTestPump.post(response);

		ensure_equals("Notification added", notifications.addedCount(), 1);
		notifications.sendYesResponse();

		ensure("Disconnected", !(logininstance->authSuccess()));
	}

	template<> template<>
    void lllogininstance_object::test<7>()
    {
		set_test_name("Test Optional Update User Denies");

		// Part 3 - Mandatory Update, with bogus response.
		// Test connect with update needed.
		logininstance->connect(agentCredential);

		ensure_equals("Default connect uri", gLoginURI, VIEWERLOGIN_URI); 

		// Update needed failure response.
		LLSD response;
		response["state"] = "offline";
		response["change"] = "fail.login";
		response["progress"] = 0.0;
		response["transfer_rate"] = 7;
		response["data"]["reason"] = "optional";
		gTestPump.post(response);

		ensure_equals("Notification added", notifications.addedCount(), 1);
		notifications.sendNoResponse();

		// User skips, should be reconnecting.
		ensure_equals("reconnect uri", gLoginURI, VIEWERLOGIN_URI); 
		ensure_equals("skipping optional update", gLoginCreds["params"]["skipoptional"].asBoolean(), true); 
	}
}
