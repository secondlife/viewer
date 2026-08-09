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

#include "../llviewerprecompiledheaders.h"
#include "doctest.h"
#include "indra/test/tut_compat_doctest.h"

#include "../llsecapi.h"
#include "../llviewernetwork.h"
#include "../lllogininstance.h"
#include "../llhasheduniqueid.h"

#include "llevents.h"
#include "llnotificationsutil.h"
#include "lltrans.h"

#if defined(LL_WINDOWS)
#pragma warning(disable: 4355)
#pragma warning(disable: 4702)
#endif

const std::string VIEWERLOGIN_URI("viewerlogin_uri");
const std::string VIEWERLOGIN_GRIDLABEL("viewerlogin_grid");

const std::string APPVIEWER_SERIALNUMBER("appviewer_serialno");

const std::string VIEWERLOGIN_CHANNEL("invalid_channel");
const std::string VIEWERLOGIN_VERSION("invalid_version");

static LLEventStream gTestPump("test_pump");

#include "../llslurl.h"
#include "../llstartup.h"
LLSLURL LLStartUp::sStartSLURL;
LLSLURL& LLStartUp::getStartSLURL() { return sStartSLURL; }
std::string LLStartUp::getUserId() { return ""; }

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
    result["passwd"] = "$1$testpasssd";
    result["first"] = "myfirst";
    result["last"] ="mylast";
    return result;
}
void LLCredential::identifierType(std::string& idType)
{
}

void LLCredential::authenticatorType(std::string& idType)
{
}

LLNotificationPtr LLNotificationsUtil::add(const std::string& name,
                                           const LLSD& substitutions,
                                           const LLSD& payload,
                                           std::function<void (const LLSD&, const LLSD&)> functor)
{
    return LLNotificationPtr((LLNotification*)NULL);
}

LLNotificationPtr LLNotificationsUtil::add(const std::string& name, const LLSD& args)
{
    return LLNotificationPtr((LLNotification*)NULL);
}

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

#include "../llversioninfo.h"

bool llHashedUniqueID(unsigned char* id)
{
    memcpy(id, "66666666666666666666666666666666", MD5HEX_STR_SIZE);
    return true;
}

#include "../llappviewer.h"
void LLAppViewer::forceQuit(void) {}
bool LLAppViewer::isUpdaterMissing() { return true; }
bool LLAppViewer::waitForUpdater() { return false; }
LLAppViewer * LLAppViewer::sInstance = 0;

#include "llnotifications.h"
#include "llfloaterreg.h"
static std::string gTOSType;
static LLEventPump * gTOSReplyPump = NULL;

LLPointer<LLSecAPIHandler> gSecAPIHandler;

LLFloater* LLFloaterReg::showInstance(std::string_view name, const LLSD& key, bool focus)
{
    gTOSType = name;
    gTOSReplyPump = &LLEventPumps::instance().obtain(key["reply_pump"]);
    return NULL;
}

#include "../llprogressview.h"
void LLProgressView::setText(std::string const &) {}
void LLProgressView::setPercent(float) {}
void LLProgressView::setMessage(std::string const &) {}

class MockNotifications : public LLNotificationsInterface
{
    std::function<void (const LLSD&, const LLSD&)> mResponder;
    int mAddedCount;

public:
    MockNotifications() :
        mResponder(0),
        mAddedCount(0)
    {
    }

    virtual ~MockNotifications() {}

    LLNotificationPtr add(
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

std::string xml_escape_string(const std::string& in)
{
    return in;
}

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::ensure_equals;
    using tut_compat::set_test_name;

    struct lllogininstance_data
    {
        lllogininstance_data() : logininstance(LLLoginInstance::getInstance())
        {
            gLoginURI.clear();
            gLoginCreds.clear();
            gDisconnectCalled = false;

            gTOSType = "";
            gTOSReplyPump = 0;

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
        }

        LLLoginInstance* logininstance;
        LLPointer<LLCredential> agentCredential;
        LLPointer<LLCredential> accountCredential;
        MockNotifications notifications;
    };
}

TUT_SUITE("LLLoginInstance")
{
    TUT_CASE("LLLoginInstance::lllogininstance_object_t_test_1")
    {
        using namespace tut;
        lllogininstance_data data;
        set_test_name("Test Simple Success And Disconnect");

        data.logininstance->connect(data.agentCredential);

        ensure_equals("Default connect uri", gLoginURI, VIEWERLOGIN_URI);

        LLSD response;
        response["state"] = "online";
        response["change"] = "connect";
        response["progress"] = 1.0;
        response["transfer_rate"] = 7;
        response["data"] = "test_data";

        gTestPump.post(response);

        ensure("Success response", data.logininstance->authSuccess());
        ensure_equals("Test Response Data", data.logininstance->getResponse().asString(), "test_data");

        data.logininstance->disconnect();

        ensure_equals("Called Login Module Disconnect", gDisconnectCalled, true);

        response.clear();
        response["state"] = "offline";
        response["change"] = "disconnect";
        response["progress"] = 0.0;
        response["transfer_rate"] = 0;
        response["data"] = "test_data";

        gTestPump.post(response);

        ensure("Disconnected", !(data.logininstance->authSuccess()));
    }

    TUT_CASE("LLLoginInstance::lllogininstance_object_t_test_2")
    {
        using namespace tut;
        lllogininstance_data data;
        set_test_name("Test User TOS/Critical message Interaction");

        const std::string test_uri = "testing-uri";

        data.logininstance->connect(test_uri, data.agentCredential);

        ensure_equals("Default connect uri", gLoginURI, "testing-uri");
        ensure_equals("Default for agree to tos", gLoginCreds["params"]["agree_to_tos"].asBoolean(), false);
        ensure_equals("Default for read critical", gLoginCreds["params"]["read_critical"].asBoolean(), false);

        LLSD response;
        response["state"] = "offline";
        response["change"] = "fail.login";
        response["progress"] = 0.0;
        response["transfer_rate"] = 7;
        response["data"]["reason"] = "tos";
        gTestPump.post(response);

        ensure_equals("TOS Dialog type", gTOSType, "message_tos");
        ensure("TOS callback given", gTOSReplyPump != 0);
        gTOSReplyPump->post(false);
        ensure("No TOS, failed auth", data.logininstance->authFailure());

        data.logininstance->connect(test_uri, data.agentCredential);
        gTestPump.post(response);
        gTOSReplyPump->post(true);
        ensure_equals("Accepted agree to tos", gLoginCreds["params"]["agree_to_tos"].asBoolean(), true);
        ensure("Incomplete login status", !data.logininstance->authFailure() && !data.logininstance->authSuccess());

        response["data"]["reason"] = "key";
        gTestPump.post(response);
        ensure("TOS auth failure", data.logininstance->authFailure());

        data.logininstance->connect(test_uri, data.agentCredential);
        ensure_equals("Reset to default for agree to tos", gLoginCreds["params"]["agree_to_tos"].asBoolean(), false);

        data.logininstance->connect(test_uri, data.agentCredential);
        response["data"]["reason"] = "critical";
        gTestPump.post(response);

        ensure_equals("TOS Dialog type", gTOSType, "message_critical");
        ensure("TOS callback given", gTOSReplyPump != 0);
        gTOSReplyPump->post(true);
        ensure_equals("Accepted read critical message", gLoginCreds["params"]["read_critical"].asBoolean(), true);
        ensure("Incomplete login status", !data.logininstance->authFailure() && !data.logininstance->authSuccess());

        response["data"]["reason"] = "key";
        gTestPump.post(response);
        ensure("TOS auth failure", data.logininstance->authFailure());
        data.logininstance->connect(test_uri, data.agentCredential);
        ensure_equals("Default for agree to tos", gLoginCreds["params"]["read_critical"].asBoolean(), false);
    }
}
