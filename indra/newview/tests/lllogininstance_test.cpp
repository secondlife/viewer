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

 // Needed for Auth Test
 #include "../llhasheduniqueid.h"

// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lldoctest.h"
#include "llevents.h"
#include "llnotificationsutil.h"
#include "lltrans.h"

#if defined(LL_WINDOWS)
#pragma warning(disable: 4355)      // using 'this' in base-class ctor initializer expr
#pragma warning(disable: 4702)      // disable 'unreachable code' so we can safely use skip().
#endif

// Constants
const std::string VIEWERLOGIN_URI("viewerlogin_uri");
const std::string VIEWERLOGIN_GRIDLABEL("viewerlogin_grid");

const std::string APPVIEWER_SERIALNUMBER("appviewer_serialno");

const std::string VIEWERLOGIN_CHANNEL("invalid_channel");
const std::string VIEWERLOGIN_VERSION("invalid_version");

// Link seams.

//-----------------------------------------------------------------------------
static LLEventStream gTestPump("test_pump");

#include "../llslurl.h"
#include "../llstartup.h"
LLSLURL LLStartUp::sStartSLURL;
LLSLURL& LLStartUp::getStartSLURL() { return sStartSLURL; }
std::string LLStartUp::getUserId() { return ""; };

#include "lllogin.h"

static std::string gLoginURI;
static LLSD gLoginCreds;
static bool gDisconnectCalled = false;

#include "../llviewerwindow.h"
void LLViewerWindow::setShowProgress(bool show) {}
LLProgressView * LLViewerWindow::getProgressView(void) const { return 0; }

LLViewerWindow* gViewerWindow;

std::string LLTrans::getString(std::string_view xml_desc, const LLStringUtil::format_map_t& args, bool def_string)
{
    return std::string("test_trans");
}

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

LLNotificationPtr LLNotificationsUtil::add(const std::string& name,
                                           const LLSD& substitutions,
                                           const LLSD& payload,
                                           boost::function<void (const LLSD&, const LLSD&)> functor)
{
    return LLNotificationPtr((LLNotification*)NULL);
}

LLNotificationPtr LLNotificationsUtil::add(const std::string& name, const LLSD& args)
{
    return LLNotificationPtr((LLNotification*)NULL);
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
                                  const std::string& update_url_base,
                                  const std::string& web_profile_url,
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
std::string LLGridManager::getGridId(const std::string& grid)
{
    return std::string();
}

//-----------------------------------------------------------------------------
#include "../llviewercontrol.h"
LLControlGroup gSavedSettings("Global");

LLControlGroup::LLControlGroup(const std::string& name) :
    LLInstanceTracker<LLControlGroup, std::string>(name){}
LLControlGroup::~LLControlGroup() {}
void LLControlGroup::setBOOL(std::string_view name, bool val) {}
bool LLControlGroup::getBOOL(std::string_view name) { return false; }
F32 LLControlGroup::getF32(std::string_view name) { return 0.0f; }
U32 LLControlGroup::saveToFile(const std::string& filename, bool nondefault_only) { return 1; }
void LLControlGroup::setString(std::string_view name, const std::string& val) {}
std::string LLControlGroup::getString(std::string_view name) { return "test_string"; }
LLControlVariable* LLControlGroup::declareBOOL(const std::string& name, bool initial_val, const std::string& comment, LLControlVariable::ePersist persist) { return NULL; }
LLControlVariable* LLControlGroup::declareString(const std::string& name, const std::string &initial_val, const std::string& comment, LLControlVariable::ePersist persist) { return NULL; }

#include "lluicolortable.h"
void LLUIColorTable::saveUserSettings(void)const {}

//-----------------------------------------------------------------------------
#include "../llversioninfo.h"

bool llHashedUniqueID(unsigned char* id)
{
    memcpy( id, "66666666666666666666666666666666", MD5HEX_STR_SIZE );
    return true;
}

//-----------------------------------------------------------------------------
#include "../llappviewer.h"
void LLAppViewer::forceQuit(void) {}
bool LLAppViewer::isUpdaterMissing() { return true; }
bool LLAppViewer::waitForUpdater() { return false; }
LLAppViewer * LLAppViewer::sInstance = 0;

//-----------------------------------------------------------------------------
#include "llnotifications.h"
#include "llfloaterreg.h"
static std::string gTOSType;
static LLEventPump * gTOSReplyPump = NULL;

LLPointer<LLSecAPIHandler> gSecAPIHandler;

//static
LLFloater* LLFloaterReg::showInstance(std::string_view name, const LLSD& key, bool focus)
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
S32 LLMachineID::getLegacyID(unsigned char *unique_id, size_t len)
{
    return 0;
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
TEST_SUITE("UnknownSuite") {

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


            gSavedSettings.declareBOOL("NoInventoryLibrary", false, "", LLControlVariable::PERSIST_NO);
            gSavedSettings.declareBOOL("ConnectAsGod", false, "", LLControlVariable::PERSIST_NO);
            gSavedSettings.declareBOOL("UseDebugMenus", false, "", LLControlVariable::PERSIST_NO);
            gSavedSettings.declareString("ClientSettingsFile", "test_settings.xml", "", LLControlVariable::PERSIST_NO);
            gSavedSettings.declareString("NextLoginLocation", "", "", LLControlVariable::PERSIST_NO);
            gSavedSettings.declareBOOL("LoginLastLocation", false, "", LLControlVariable::PERSIST_NO);
            gSavedSettings.declareBOOL("CmdLineSkipUpdater", true, "", LLControlVariable::PERSIST_NO);

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
            logininstance->setPlatformInfo("win", "1.3.5", "Windows Bogus Version 100.6.6.6");
        
};

TEST_CASE_FIXTURE(lllogininstance_data, "test_1")
{

        set_test_name("Test Simple Success And Disconnect");

        // Test default connect.
        logininstance->connect(agentCredential);

        CHECK_MESSAGE(gLoginURI == VIEWERLOGIN_URI, "Default connect uri");

        // Dummy success response.
        LLSD response;
        response["state"] = "online";
        response["change"] = "connect";
        response["progress"] = 1.0;
        response["transfer_rate"] = 7;
        response["data"] = "test_data";

        gTestPump.post(response);

        CHECK_MESSAGE(logininstance->authSuccess(, "Success response"));
        ensure_equals("Test Response Data", logininstance->getResponse().asString(), "test_data");

        logininstance->disconnect();

        CHECK_MESSAGE(gDisconnectCalled == true, "Called Login Module Disconnect");

        response.clear();
        response["state"] = "offline";
        response["change"] = "disconnect";
        response["progress"] = 0.0;
        response["transfer_rate"] = 0;
        response["data"] = "test_data";

        gTestPump.post(response);

        CHECK_MESSAGE(!(logininstance->authSuccess(, "Disconnected")));
    
}

TEST_CASE_FIXTURE(lllogininstance_data, "test_2")
{

        set_test_name("Test User TOS/Critical message Interaction");

        const std::string test_uri = "testing-uri";

        // Test default connect.
        logininstance->connect(test_uri, agentCredential);

        // connect should call LLLogin::connect to init gLoginURI and gLoginCreds.
        CHECK_MESSAGE(gLoginURI == "testing-uri", "Default connect uri");
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

        CHECK_MESSAGE(gTOSType == "message_tos", "TOS Dialog type");
        CHECK_MESSAGE(gTOSReplyPump != 0, "TOS callback given");
        gTOSReplyPump->post(false); // Call callback denying TOS.
        CHECK_MESSAGE(logininstance->authFailure(, "No TOS, failed auth"));

        // Start again.
        logininstance->connect(test_uri, agentCredential);
        gTestPump.post(response); // Fail for tos again.
        gTOSReplyPump->post(true); // Accept tos, should reconnect w/ agree_to_tos.
        ensure_equals("Accepted agree to tos", gLoginCreds["params"]["agree_to_tos"].asBoolean(), true);
        CHECK_MESSAGE(!logininstance->authFailure(, "Incomplete login status") && !logininstance->authSuccess());

        // Fail connection, attempt connect again.
        // The new request should have reset agree to tos to default.
        response["data"]["reason"] = "key"; // bad creds.
        gTestPump.post(response);
        CHECK_MESSAGE(logininstance->authFailure(, "TOS auth failure"));

        logininstance->connect(test_uri, agentCredential);
        ensure_equals("Reset to default for agree to tos", gLoginCreds["params"]["agree_to_tos"].asBoolean(), false);

        // Critical Message failure response.
        logininstance->connect(test_uri, agentCredential);
        response["data"]["reason"] = "critical"; // Change response to "critical message"
        gTestPump.post(response);

        CHECK_MESSAGE(gTOSType == "message_critical", "TOS Dialog type");
        CHECK_MESSAGE(gTOSReplyPump != 0, "TOS callback given");
        gTOSReplyPump->post(true);
        ensure_equals("Accepted read critical message", gLoginCreds["params"]["read_critical"].asBoolean(), true);
        CHECK_MESSAGE(!logininstance->authFailure(, "Incomplete login status") && !logininstance->authSuccess());

        // Fail then attempt new connection
        response["data"]["reason"] = "key"; // bad creds.
        gTestPump.post(response);
        CHECK_MESSAGE(logininstance->authFailure(, "TOS auth failure"));
        logininstance->connect(test_uri, agentCredential);
        ensure_equals("Default for agree to tos", gLoginCreds["params"]["read_critical"].asBoolean(), false);
    
}

} // TEST_SUITE

