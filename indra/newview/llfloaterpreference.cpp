/**
 * @file llfloaterpreference.cpp
 * @brief Global preferences with and without persistence.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterpreference.h"

#include "message.h"
#include "llfloaterautoreplacesettings.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llcommandhandler.h"
#include "lldirpicker.h"
#include "lleventtimer.h"
#include "llfeaturemanager.h"
#include "llfocusmgr.h"
//#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llfloaterabout.h"
#include "llfavoritesbar.h"
#include "llfloaterpreferencesgraphicsadvanced.h"
#include "llfloaterperformance.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterimsession.h"
#include "llgamecontrol.h"
#include "llkeyboard.h"
#include "llmodaldialog.h"
#include "llnavigationbar.h"
#include "llfloaterimnearbychat.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llnotificationtemplate.h"
#include "llpanellogin.h"
#include "llpanelvoicedevicesettings.h"
#include "llradiogroup.h"
#include "llsearchcombobox.h"
#include "llsky.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsliderctrl.h"
#include "lltabcontainer.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llviewereventrecorder.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llviewerthrottle.h"
#include "llvoavatarself.h"
#include "llvotree.h"
#include "llfloaterpathfindingconsole.h"
// linden library includes
#include "llavatarnamecache.h"
#include "llerror.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llstring.h"

// project includes

#include "llbutton.h"
#include "llflexibleobject.h"
#include "lllineeditor.h"
#include "llresmgr.h"
#include "llspinctrl.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerobjectlist.h"
#include "llvovolume.h"
#include "llwindow.h"
#include "llworld.h"
#include "lluictrlfactory.h"
#include "llviewermedia.h"
#include "llpluginclassmedia.h"
#include "llteleporthistorystorage.h"
#include "llproxy.h"
#include "llweb.h"

#include "lllogininstance.h"        // to check if logged in yet
#include "llsdserialize.h"
#include "llpresetsmanager.h"
#include "llviewercontrol.h"
#include "llpresetsmanager.h"
#include "llinventoryfunctions.h"

#include "llsearchableui.h"
#include "llperfstats.h"

const F32 BANDWIDTH_UPDATER_TIMEOUT = 0.5f;
char const* const VISIBILITY_DEFAULT = "default";
char const* const VISIBILITY_HIDDEN = "hidden";

//control value for middle mouse as talk2push button
const static std::string MIDDLE_MOUSE_CV = "MiddleMouse"; // for voice client and redability
const static std::string MOUSE_BUTTON_4_CV = "MouseButton4";
const static std::string MOUSE_BUTTON_5_CV = "MouseButton5";

/// This must equal the maximum value set for the IndirectMaxComplexity slider in panel_preferences_graphics1.xml
static const U32 INDIRECT_MAX_ARC_OFF = 101; // all the way to the right == disabled
static const U32 MIN_INDIRECT_ARC_LIMIT = 1; // must match minimum of IndirectMaxComplexity in panel_preferences_graphics1.xml
static const U32 MAX_INDIRECT_ARC_LIMIT = INDIRECT_MAX_ARC_OFF-1; // one short of all the way to the right...

/// These are the effective range of values for RenderAvatarMaxComplexity
static const F32 MIN_ARC_LIMIT =  20000.0f;
static const F32 MAX_ARC_LIMIT = 350000.0f;
static const F32 MIN_ARC_LOG = log(MIN_ARC_LIMIT);
static const F32 MAX_ARC_LOG = log(MAX_ARC_LIMIT);
static const F32 ARC_LIMIT_MAP_SCALE = (MAX_ARC_LOG - MIN_ARC_LOG) / (MAX_INDIRECT_ARC_LIMIT - MIN_INDIRECT_ARC_LIMIT);

struct LabelDef : public LLInitParam::Block<LabelDef>
{
    Mandatory<std::string> name;
    Mandatory<std::string> value;

    LabelDef()
        : name("name"),
        value("value")
    {}
};

struct LabelTable : public LLInitParam::Block<LabelTable>
{
    Multiple<LabelDef> labels;
    LabelTable()
        : labels("label")
    {}
};


// global functions

// helper functions for getting/freeing the web browser media
// if creating/destroying these is too slow, we'll need to create
// a static member and update all our static callbacks

void handleNameTagOptionChanged(const LLSD& newvalue);
void handleDisplayNamesOptionChanged(const LLSD& newvalue);
bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response);
bool callback_clear_cache(const LLSD& notification, const LLSD& response);

//bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);
//bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);

void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator);

bool callback_clear_cache(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if ( option == 0 ) // YES
    {
        // flag client texture cache for clearing next time the client runs
        gSavedSettings.setBOOL("PurgeCacheOnNextStartup", true);
        LLNotificationsUtil::add("CacheWillClear");
    }

    return false;
}

bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if ( option == 0 ) // YES
    {
        // clean web
        LLViewerMedia::getInstance()->clearAllCaches();
        LLViewerMedia::getInstance()->clearAllCookies();

        // clean nav bar history
        LLNavigationBar::getInstance()->clearHistoryCache();

        // flag client texture cache for clearing next time the client runs
        gSavedSettings.setBOOL("PurgeCacheOnNextStartup", true);
        LLNotificationsUtil::add("CacheWillClear");

        LLSearchHistory::getInstance()->clearHistory();
        LLSearchHistory::getInstance()->save();
        LLSearchComboBox* search_ctrl = LLNavigationBar::getInstance()->getChild<LLSearchComboBox>("search_combo_box");
        search_ctrl->clearHistory();

        LLTeleportHistoryStorage::getInstance()->purgeItems();
        LLTeleportHistoryStorage::getInstance()->save();
    }

    return false;
}

void handleNameTagOptionChanged(const LLSD& newvalue)
{
    LLAvatarNameCache::getInstance()->setUseUsernames(gSavedSettings.getBOOL("NameTagShowUsernames"));
    LLVOAvatar::invalidateNameTags();
}

void handleDisplayNamesOptionChanged(const LLSD& newvalue)
{
    LLAvatarNameCache::getInstance()->setUseDisplayNames(newvalue.asBoolean());
    LLVOAvatar::invalidateNameTags();
}

void handleAppearanceCameraMovementChanged(const LLSD& newvalue)
{
    if(!newvalue.asBoolean() && gAgentCamera.getCameraMode() == CAMERA_MODE_CUSTOMIZE_AVATAR)
    {
        gAgentCamera.changeCameraToDefault();
        gAgentCamera.resetView();
    }
}

void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator)
{
    numerator = 0;
    denominator = 0;
    for (F32 test_denominator = 1.f; test_denominator < 30.f; test_denominator += 1.f)
    {
        if (fmodf((decimal_val * test_denominator) + 0.01f, 1.f) < 0.02f)
        {
            numerator = ll_round(decimal_val * test_denominator);
            denominator = ll_round(test_denominator);
            break;
        }
    }
}

// handle secondlife:///app/worldmap/{NAME}/{COORDS} URLs
// Also see LLUrlEntryKeybinding, the value of this command type
// is ability to show up to date value in chat
class LLKeybindingHandler: public LLCommandHandler
{
public:
    // requires trusted browser to trigger
    LLKeybindingHandler(): LLCommandHandler("keybinding", UNTRUSTED_CLICK_ONLY)
    {
    }

    bool handle(const LLSD& params, const LLSD& query_map,
                const std::string& grid, LLMediaCtrl* web)
    {
        if (params.size() < 1) return false;

        LLFloaterPreference* prefsfloater = dynamic_cast<LLFloaterPreference*>
            (LLFloaterReg::showInstance("preferences"));

        if (prefsfloater)
        {
            // find 'controls' panel and bring it the front
            LLTabContainer* tabcontainer = prefsfloater->getChild<LLTabContainer>("pref core");
            LLPanel* panel = prefsfloater->getChild<LLPanel>("controls");
            if (tabcontainer && panel)
            {
                tabcontainer->selectTabPanel(panel);
            }
        }

        return true;
    }
};
LLKeybindingHandler gKeybindHandler;


//////////////////////////////////////////////
// LLFloaterPreference

// static
std::string LLFloaterPreference::sSkin = "";

LLFloaterPreference::LLFloaterPreference(const LLSD& key)
    : LLFloater(key),
    mGotPersonalInfo(false),
    mLanguageChanged(false),
    mAvatarDataInitialized(false),
    mSearchDataDirty(true)
{
    LLConversationLog::instance().addObserver(this);

    //Build Floater is now Called from  LLFloaterReg::add("preferences", "floater_preferences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreference>);

    static bool registered_dialog = false;
    if (!registered_dialog)
    {
        LLFloaterReg::add("keybind_dialog", "floater_select_key.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLSetKeyBindDialog>);
        registered_dialog = true;
    }

    mCommitCallbackRegistrar.add("Pref.Cancel",             boost::bind(&LLFloaterPreference::onBtnCancel, this, _2));
    mCommitCallbackRegistrar.add("Pref.OK",                 boost::bind(&LLFloaterPreference::onBtnOK, this, _2));

    mCommitCallbackRegistrar.add("Pref.ClearCache",             boost::bind(&LLFloaterPreference::onClickClearCache, this));
    mCommitCallbackRegistrar.add("Pref.WebClearCache",          boost::bind(&LLFloaterPreference::onClickBrowserClearCache, this));
    mCommitCallbackRegistrar.add("Pref.SetCache",               boost::bind(&LLFloaterPreference::onClickSetCache, this));
    mCommitCallbackRegistrar.add("Pref.ResetCache",             boost::bind(&LLFloaterPreference::onClickResetCache, this));
    mCommitCallbackRegistrar.add("Pref.ClickSkin",              boost::bind(&LLFloaterPreference::onClickSkin, this,_1, _2));
    mCommitCallbackRegistrar.add("Pref.SelectSkin",             boost::bind(&LLFloaterPreference::onSelectSkin, this));
    mCommitCallbackRegistrar.add("Pref.SetSounds",              boost::bind(&LLFloaterPreference::onClickSetSounds, this));
    mCommitCallbackRegistrar.add("Pref.ClickEnablePopup",       boost::bind(&LLFloaterPreference::onClickEnablePopup, this));
    mCommitCallbackRegistrar.add("Pref.ClickDisablePopup",      boost::bind(&LLFloaterPreference::onClickDisablePopup, this));
    mCommitCallbackRegistrar.add("Pref.LogPath",                boost::bind(&LLFloaterPreference::onClickLogPath, this));
    mCommitCallbackRegistrar.add("Pref.RenderExceptions",       boost::bind(&LLFloaterPreference::onClickRenderExceptions, this));
    mCommitCallbackRegistrar.add("Pref.AutoAdjustments",         boost::bind(&LLFloaterPreference::onClickAutoAdjustments, this));
    mCommitCallbackRegistrar.add("Pref.HardwareDefaults",       boost::bind(&LLFloaterPreference::setHardwareDefaults, this));
    mCommitCallbackRegistrar.add("Pref.AvatarImpostorsEnable",  boost::bind(&LLFloaterPreference::onAvatarImpostorsEnable, this));
    mCommitCallbackRegistrar.add("Pref.UpdateIndirectMaxComplexity",    boost::bind(&LLFloaterPreference::updateMaxComplexity, this));
    mCommitCallbackRegistrar.add("Pref.RenderOptionUpdate",     boost::bind(&LLFloaterPreference::onRenderOptionEnable, this));
    mCommitCallbackRegistrar.add("Pref.WindowedMod",            boost::bind(&LLFloaterPreference::onCommitWindowedMode, this));
    mCommitCallbackRegistrar.add("Pref.UpdateSliderText",       boost::bind(&LLFloaterPreference::refreshUI,this));
    mCommitCallbackRegistrar.add("Pref.QualityPerformance",     boost::bind(&LLFloaterPreference::onChangeQuality, this, _2));
    mCommitCallbackRegistrar.add("Pref.applyUIColor",           boost::bind(&LLFloaterPreference::applyUIColor, this ,_1, _2));
    mCommitCallbackRegistrar.add("Pref.getUIColor",             boost::bind(&LLFloaterPreference::getUIColor, this ,_1, _2));
    mCommitCallbackRegistrar.add("Pref.MaturitySettings",       boost::bind(&LLFloaterPreference::onChangeMaturity, this));
    mCommitCallbackRegistrar.add("Pref.BlockList",              boost::bind(&LLFloaterPreference::onClickBlockList, this));
    mCommitCallbackRegistrar.add("Pref.Proxy",                  boost::bind(&LLFloaterPreference::onClickProxySettings, this));
    mCommitCallbackRegistrar.add("Pref.TranslationSettings",    boost::bind(&LLFloaterPreference::onClickTranslationSettings, this));
    mCommitCallbackRegistrar.add("Pref.AutoReplace",            boost::bind(&LLFloaterPreference::onClickAutoReplace, this));
    mCommitCallbackRegistrar.add("Pref.PermsDefault",           boost::bind(&LLFloaterPreference::onClickPermsDefault, this));
    mCommitCallbackRegistrar.add("Pref.RememberedUsernames",    boost::bind(&LLFloaterPreference::onClickRememberedUsernames, this));
    mCommitCallbackRegistrar.add("Pref.SpellChecker",           boost::bind(&LLFloaterPreference::onClickSpellChecker, this));
    mCommitCallbackRegistrar.add("Pref.Advanced",               boost::bind(&LLFloaterPreference::onClickAdvanced, this));

    sSkin = gSavedSettings.getString("SkinCurrent");

    mCommitCallbackRegistrar.add("Pref.ClickActionChange",      boost::bind(&LLFloaterPreference::onClickActionChange, this));

    gSavedSettings.getControl("NameTagShowUsernames")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged,  _2));
    gSavedSettings.getControl("NameTagShowFriends")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged,  _2));
    gSavedSettings.getControl("UseDisplayNames")->getCommitSignal()->connect(boost::bind(&handleDisplayNamesOptionChanged,  _2));

    gSavedSettings.getControl("AppearanceCameraMovement")->getCommitSignal()->connect(boost::bind(&handleAppearanceCameraMovementChanged,  _2));
    gSavedSettings.getControl("WindLightUseAtmosShaders")->getCommitSignal()->connect(boost::bind(&LLFloaterPreference::onAtmosShaderChange, this));

    LLAvatarPropertiesProcessor::getInstance()->addObserver( gAgent.getID(), this );

    mComplexityChangedSignal = gSavedSettings.getControl("RenderAvatarMaxComplexity")->getCommitSignal()->connect(boost::bind(&LLFloaterPreference::updateComplexityText, this));

    mCommitCallbackRegistrar.add("Pref.ClearLog",               boost::bind(&LLConversationLog::onClearLog, &LLConversationLog::instance()));
    mCommitCallbackRegistrar.add("Pref.DeleteTranscripts",      boost::bind(&LLFloaterPreference::onDeleteTranscripts, this));
    mCommitCallbackRegistrar.add("UpdateFilter", boost::bind(&LLFloaterPreference::onUpdateFilterTerm, this, false)); // <FS:ND/> Hook up for filtering
}

void LLFloaterPreference::processProperties( void* pData, EAvatarProcessorType type )
{
    if ( APT_PROPERTIES_LEGACY == type )
    {
        const LLAvatarLegacyData* pAvatarData = static_cast<const LLAvatarLegacyData*>( pData );
        if (pAvatarData && (gAgent.getID() == pAvatarData->avatar_id) && (pAvatarData->avatar_id != LLUUID::null))
        {
            mAllowPublish = (bool)(pAvatarData->flags & AVATAR_ALLOW_PUBLISH);
            mAvatarDataInitialized = true;
            getChild<LLUICtrl>("online_searchresults")->setValue(mAllowPublish);
        }
    }
}

void LLFloaterPreference::saveAvatarProperties( void )
{
    const bool allowPublish = getChild<LLUICtrl>("online_searchresults")->getValue();

    if ((LLStartUp::getStartupState() == STATE_STARTED)
        && mAvatarDataInitialized
        && (allowPublish != mAllowPublish))
    {
        std::string cap_url = gAgent.getRegionCapability("AgentProfile");
        if (!cap_url.empty())
        {
            mAllowPublish = allowPublish;

            LLCoros::instance().launch("requestAgentUserInfoCoro",
                boost::bind(saveAvatarPropertiesCoro, cap_url, allowPublish));
        }
    }
}

void LLFloaterPreference::saveAvatarPropertiesCoro(const std::string cap_url, bool allow_publish)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("put_avatar_properties_coro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    httpOpts->setFollowRedirects(true);

    std::string finalUrl = cap_url + "/" + gAgentID.asString();
    LLSD data;
    data["allow_publish"] = allow_publish;

    LLSD result = httpAdapter->putAndSuspend(httpRequest, finalUrl, data, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("Preferences") << "Failed to put agent information " << data << " for id " << gAgentID << LL_ENDL;
        return;
    }

    LL_DEBUGS("Preferences") << "Agent id: " << gAgentID << " Data: " << data << " Result: " << httpResults << LL_ENDL;
}

bool LLFloaterPreference::postBuild()
{
    gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&LLFloaterIMSessionTab::processChatHistoryStyleUpdate, false));

    gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&LLViewerChat::signalChatFontChanged));

    gSavedSettings.getControl("ChatBubbleOpacity")->getSignal()->connect(boost::bind(&LLFloaterPreference::onNameTagOpacityChange, this, _2));

    gSavedSettings.getControl("PreferredMaturity")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeMaturity, this));

    gSavedSettings.getControl("RenderAvatarComplexityMode")->getSignal()->connect(
        [this](LLControlVariable* control, const LLSD& new_val, const LLSD& old_val)
        {
            onChangeComplexityMode(new_val);
        });

    gSavedPerAccountSettings.getControl("ModelUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeModelFolder, this));
    gSavedPerAccountSettings.getControl("PBRUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangePBRFolder, this));
    gSavedPerAccountSettings.getControl("TextureUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeTextureFolder, this));
    gSavedPerAccountSettings.getControl("SoundUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeSoundFolder, this));
    gSavedPerAccountSettings.getControl("AnimationUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeAnimationFolder, this));

    LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
    if (!tabcontainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
        tabcontainer->selectFirstTab();

    getChild<LLUICtrl>("cache_location")->setEnabled(false); // make it read-only but selectable (STORM-227)
    std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
    setCacheLocation(cache_location);

    getChild<LLUICtrl>("log_path_string")->setEnabled(false); // make it read-only but selectable

    getChild<LLComboBox>("language_combobox")->setCommitCallback(boost::bind(&LLFloaterPreference::onLanguageChange, this));

    getChild<LLComboBox>("FriendIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"FriendIMOptions"));
    getChild<LLComboBox>("NonFriendIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"NonFriendIMOptions"));
    getChild<LLComboBox>("ConferenceIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"ConferenceIMOptions"));
    getChild<LLComboBox>("GroupChatOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"GroupChatOptions"));
    getChild<LLComboBox>("NearbyChatOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"NearbyChatOptions"));
    getChild<LLComboBox>("ObjectIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"ObjectIMOptions"));

    // if floater is opened before login set default localized do not disturb message
    if (LLStartUp::getStartupState() < STATE_STARTED)
    {
        gSavedPerAccountSettings.setString("DoNotDisturbModeResponse", LLTrans::getString("DoNotDisturbModeResponseDefault"));
    }

    // set 'enable' property for 'Clear log...' button
    changed();

    LLLogChat::getInstance()->setSaveHistorySignal(boost::bind(&LLFloaterPreference::onLogChatHistorySaved, this));

    LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
    fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
    fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());

    bool enable_complexity = gSavedSettings.getS32("RenderAvatarComplexityMode") != LLVOAvatar::AV_RENDER_ONLY_SHOW_FRIENDS;
    getChild<LLSliderCtrl>("IndirectMaxComplexity")->setEnabled(enable_complexity);

    // Hook up and init for filtering
    mFilterEdit = getChild<LLSearchEditor>("search_prefs_edit");
    mFilterEdit->setKeystrokeCallback(boost::bind(&LLFloaterPreference::onUpdateFilterTerm, this, false));

    // Load and assign label for 'default language'
    std::string user_filename = gDirUtilp->getExpandedFilename(LL_PATH_DEFAULT_SKIN, "default_languages.xml");
    std::map<std::string, std::string> labels;
    if (loadFromFilename(user_filename, labels))
    {
        std::string system_lang = gSavedSettings.getString("SystemLanguage");
        std::map<std::string, std::string>::iterator iter = labels.find(system_lang);
        if (iter != labels.end())
        {
            getChild<LLComboBox>("language_combobox")->add(iter->second, LLSD("default"), ADD_TOP, true);
        }
        else
        {
            LL_WARNS() << "Language \"" << system_lang << "\" is not in default_languages.xml" << LL_ENDL;
            getChild<LLComboBox>("language_combobox")->add("System default", LLSD("default"), ADD_TOP, true);
        }
    }
    else
    {
        LL_WARNS() << "Failed to load labels from " << user_filename << ". Using default." << LL_ENDL;
        getChild<LLComboBox>("language_combobox")->add("System default", LLSD("default"), ADD_TOP, true);
    }

    return true;
}

void LLFloaterPreference::updateDeleteTranscriptsButton()
{
    getChild<LLButton>("delete_transcripts")->setEnabled(LLLogChat::transcriptFilesExist());
}

void LLFloaterPreference::onDoNotDisturbResponseChanged()
{
    // set "DoNotDisturbResponseChanged" true if user edited message differs from default, false otherwise
    bool response_changed_flag =
            LLTrans::getString("DoNotDisturbModeResponseDefault")
                    != getChild<LLUICtrl>("do_not_disturb_response")->getValue().asString();

    gSavedPerAccountSettings.setBOOL("DoNotDisturbResponseChanged", response_changed_flag );
}

LLFloaterPreference::~LLFloaterPreference()
{
    LLConversationLog::instance().removeObserver(this);
    mComplexityChangedSignal.disconnect();
}

void LLFloaterPreference::draw()
{
    bool has_first_selected = (getChildRef<LLScrollListCtrl>("disabled_popups").getFirstSelected()!=NULL);
    gSavedSettings.setBOOL("FirstSelectedDisabledPopups", has_first_selected);

    has_first_selected = (getChildRef<LLScrollListCtrl>("enabled_popups").getFirstSelected()!=NULL);
    gSavedSettings.setBOOL("FirstSelectedEnabledPopups", has_first_selected);

    LLFloater::draw();
}

void LLFloaterPreference::saveSettings()
{
    LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
    for (LLView* view : *tabcontainer->getChildList())
    {
        if (LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view))
        {
            panel->saveSettings();
        }
    }

    saveIgnoredNotifications();
}

void LLFloaterPreference::apply()
{
    LLAvatarPropertiesProcessor::getInstance()->addObserver(gAgent.getID(), this);

    LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
    if (sSkin != gSavedSettings.getString("SkinCurrent"))
    {
        LLNotificationsUtil::add("ChangeSkin");
        refreshSkin(this);
    }

    // Call apply() on all panels that derive from LLPanelPreference
    for (LLView* view : *tabcontainer->getChildList())
    {
        if (LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view))
        {
            panel->apply();
        }
    }

    gViewerWindow->requestResolutionUpdate(); // for UIScaleFactor

    LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
    fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
    fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());

    std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
    setCacheLocation(cache_location);

    LLViewerMedia::getInstance()->setCookiesEnabled(getChild<LLUICtrl>("cookies_enabled")->getValue());

    if (hasChild("web_proxy_enabled", true) &&hasChild("web_proxy_editor", true) && hasChild("web_proxy_port", true))
    {
        bool proxy_enable = getChild<LLUICtrl>("web_proxy_enabled")->getValue();
        std::string proxy_address = getChild<LLUICtrl>("web_proxy_editor")->getValue();
        int proxy_port = getChild<LLUICtrl>("web_proxy_port")->getValue();
        LLViewerMedia::getInstance()->setProxyConfig(proxy_enable, proxy_address, proxy_port);
    }

    if (mGotPersonalInfo)
    {
        bool new_hide_online = getChild<LLUICtrl>("online_visibility")->getValue().asBoolean();

        if (new_hide_online != mOriginalHideOnlineStatus)
        {
            // This hack is because we are representing several different
            // possible strings with a single checkbox. Since most users
            // can only select between 2 values, we represent it as a
            // checkbox. This breaks down a little bit for liaisons, but
            // works out in the end.
            if (new_hide_online != mOriginalHideOnlineStatus)
            {
                if (new_hide_online) mDirectoryVisibility = VISIBILITY_HIDDEN;
                else mDirectoryVisibility = VISIBILITY_DEFAULT;
             //Update showonline value, otherwise multiple applys won't work
                mOriginalHideOnlineStatus = new_hide_online;
            }
            gAgent.sendAgentUpdateUserInfo(mDirectoryVisibility);
        }
    }

    saveAvatarProperties();
}

void LLFloaterPreference::cancel(const std::vector<std::string> settings_to_skip)
{
    LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
    // Call cancel() on all panels that derive from LLPanelPreference
    for (LLView* view : *tabcontainer->getChildList())
    {
        if (LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view))
        {
            panel->cancel();
        }
    }

    // hide joystick pref floater
    LLFloaterReg::hideInstance("pref_joystick");

    // hide translation settings floater
    LLFloaterReg::hideInstance("prefs_translation");

    // hide autoreplace settings floater
    LLFloaterReg::hideInstance("prefs_autoreplace");

    // hide spellchecker settings folder
    LLFloaterReg::hideInstance("prefs_spellchecker");

    // hide advanced graphics floater
    LLFloaterReg::hideInstance("prefs_graphics_advanced");

    // reverts any changes to current skin
    gSavedSettings.setString("SkinCurrent", sSkin);

    updateClickActionViews();

    LLFloaterPreferenceProxy * advanced_proxy_settings = LLFloaterReg::findTypedInstance<LLFloaterPreferenceProxy>("prefs_proxy");
    if (advanced_proxy_settings)
    {
        advanced_proxy_settings->cancel();
    }
    //Need to reload the navmesh if the pathing console is up
    LLHandle<LLFloaterPathfindingConsole> pathfindingConsoleHandle = LLFloaterPathfindingConsole::getInstanceHandle();
    if ( !pathfindingConsoleHandle.isDead() )
    {
        LLFloaterPathfindingConsole* pPathfindingConsole = pathfindingConsoleHandle.get();
        pPathfindingConsole->onRegionBoundaryCross();
    }

    if (!mSavedGraphicsPreset.empty())
    {
        gSavedSettings.setString("PresetGraphicActive", mSavedGraphicsPreset);
        LLPresetsManager::getInstance()->triggerChangeSignal();
    }

    restoreIgnoredNotifications();
}

void LLFloaterPreference::onOpen(const LLSD& key)
{
    // this variable and if that follows it are used to properly handle do not disturb mode response message
    static bool initialized = false;
    // if user is logged in and we haven't initialized do not disturb mode response yet, do it
    if (!initialized && LLStartUp::getStartupState() == STATE_STARTED)
    {
        // Special approach is used for do not disturb response localization, because "DoNotDisturbModeResponse" is
        // in non-localizable xml, and also because it may be changed by user and in this case it shouldn't be localized.
        // To keep track of whether do not disturb response is default or changed by user additional setting DoNotDisturbResponseChanged
        // was added into per account settings.

        // initialization should happen once,so setting variable to true
        initialized = true;
        // this connection is needed to properly set "DoNotDisturbResponseChanged" setting when user makes changes in
        // do not disturb response message.
        gSavedPerAccountSettings.getControl("DoNotDisturbModeResponse")->getSignal()->connect(boost::bind(&LLFloaterPreference::onDoNotDisturbResponseChanged, this));
    }
    gAgent.sendAgentUserInfoRequest();

    LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
    for (LLView* view : *tabcontainer->getChildList())
    {
        if (LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view))
        {
            panel->onOpen(key);
        }
    }

    /////////////////////////// From LLPanelGeneral //////////////////////////
    // if we have no agent, we can't let them choose anything
    // if we have an agent, then we only let them choose if they have a choice
    bool can_choose_maturity =
        gAgent.getID().notNull() &&
        (gAgent.isMature() || gAgent.isGodlike());

    LLComboBox* maturity_combo = getChild<LLComboBox>("maturity_desired_combobox");
    LLAvatarPropertiesProcessor::getInstance()->sendAvatarLegacyPropertiesRequest( gAgent.getID() );
    if (can_choose_maturity)
    {
        // if they're not adult or a god, they shouldn't see the adult selection, so delete it
        if (!gAgent.isAdult() && !gAgent.isGodlikeWithoutAdminMenuFakery())
        {
            // we're going to remove the adult entry from the combo
            LLScrollListCtrl* maturity_list = maturity_combo->findChild<LLScrollListCtrl>("ComboBox");
            if (maturity_list)
            {
                maturity_list->deleteItems(LLSD(SIM_ACCESS_ADULT));
            }
        }
        getChildView("maturity_desired_combobox")->setEnabled( true);
        getChildView("maturity_desired_textbox")->setVisible( false);
    }
    else
    {
        getChild<LLUICtrl>("maturity_desired_textbox")->setValue(maturity_combo->getSelectedItemLabel());
        getChildView("maturity_desired_combobox")->setEnabled( false);
    }

    // Forget previous language changes.
    mLanguageChanged = false;

    // Display selected maturity icons.
    onChangeMaturity();

    onChangeModelFolder();
    onChangePBRFolder();
    onChangeTextureFolder();
    onChangeSoundFolder();
    onChangeAnimationFolder();

    // Load (double-)click to walk/teleport settings.
    updateClickActionViews();

    // Enabled/disabled popups, might have been changed by user actions
    // while preferences floater was closed.
    buildPopupLists();


    //get the options that were checked
    onNotificationsChange("FriendIMOptions");
    onNotificationsChange("NonFriendIMOptions");
    onNotificationsChange("ConferenceIMOptions");
    onNotificationsChange("GroupChatOptions");
    onNotificationsChange("NearbyChatOptions");
    onNotificationsChange("ObjectIMOptions");

    LLPanelLogin::setAlwaysRefresh(true);
    refresh();

    // Make sure the current state of prefs are saved away when
    // the floater is opened.  That will make cancel do its job
    saveSettings();

    // Make sure there is a default preference file
    LLPresetsManager::getInstance()->createMissingDefault(PRESETS_CAMERA);
    LLPresetsManager::getInstance()->createMissingDefault(PRESETS_GRAPHIC);

    bool started = (LLStartUp::getStartupState() == STATE_STARTED);

    LLButton* load_btn = findChild<LLButton>("PrefLoadButton");
    LLButton* save_btn = findChild<LLButton>("PrefSaveButton");
    LLButton* delete_btn = findChild<LLButton>("PrefDeleteButton");
    LLButton* exceptions_btn = findChild<LLButton>("RenderExceptionsButton");
    LLButton* auto_adjustments_btn = findChild<LLButton>("AutoAdjustmentsButton");

    if (load_btn && save_btn && delete_btn && exceptions_btn && auto_adjustments_btn)
    {
        load_btn->setEnabled(started);
        save_btn->setEnabled(started);
        delete_btn->setEnabled(started);
        exceptions_btn->setEnabled(started);
        auto_adjustments_btn->setEnabled(started);
    }

    collectSearchableItems();
    if (!mFilterEdit->getText().empty())
    {
        mFilterEdit->setText(LLStringExplicit(""));
        onUpdateFilterTerm(true);
    }
}

void LLFloaterPreference::onRenderOptionEnable()
{
    refreshEnabledGraphics();
}

void LLFloaterPreference::onAvatarImpostorsEnable()
{
    refreshEnabledGraphics();
}

//static
void LLFloaterPreference::initDoNotDisturbResponse()
    {
        if (!gSavedPerAccountSettings.getBOOL("DoNotDisturbResponseChanged"))
        {
            //LLTrans::getString("DoNotDisturbModeResponseDefault") is used here for localization (EXT-5885)
            gSavedPerAccountSettings.setString("DoNotDisturbModeResponse", LLTrans::getString("DoNotDisturbModeResponseDefault"));
        }
    }

//static
void LLFloaterPreference::updateShowFavoritesCheckbox(bool val)
{
    LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
    if (instance)
    {
        instance->getChild<LLUICtrl>("favorites_on_login_check")->setValue(val);
    }
}

void LLFloaterPreference::setHardwareDefaults()
{
    std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");
    if (!preset_graphic_active.empty())
    {
        saveGraphicsPreset(preset_graphic_active);
        saveSettings(); // save here to be able to return to the previous preset by Cancel
    }
    setRecommendedSettings();
}

void LLFloaterPreference::setRecommendedSettings()
{
    resetAutotuneSettings();
    gSavedSettings.getControl("RenderVSyncEnable")->resetToDefault(true);

    LLFeatureManager::getInstance()->applyRecommendedSettings();

    // reset indirects before refresh because we may have changed what they control
    LLAvatarComplexityControls::setIndirectControls();

    refreshEnabledGraphics();
    gSavedSettings.setString("PresetGraphicActive", "");
    LLPresetsManager::getInstance()->triggerChangeSignal();

    LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
    child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
    child_list_t::const_iterator end = tabcontainer->getChildList()->end();
    for ( ; iter != end; ++iter)
    {
        LLView* view = *iter;
        LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
        if (panel)
        {
            panel->setHardwareDefaults();
        }
    }
}

void LLFloaterPreference::resetAutotuneSettings()
{
    gSavedSettings.setBOOL("AutoTuneFPS", false);

    const std::string autotune_settings[] = {
        "AutoTuneLock",
        "KeepAutoTuneLock",
        "TargetFPS",
        "TuningFPSStrategy",
        "AutoTuneImpostorByDistEnabled",
        "AutoTuneImpostorFarAwayDistance" ,
        "AutoTuneRenderFarClipMin",
        "AutoTuneRenderFarClipTarget",
        "RenderAvatarMaxART"
    };

    for (auto it : autotune_settings)
    {
        gSavedSettings.getControl(it)->resetToDefault(true);
    }
}

void LLFloaterPreference::getControlNames(std::vector<std::string>& names)
{
    LLView* view = findChild<LLView>("display");
    LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");
    if (view && advanced)
    {
        std::list<LLView*> stack;
        stack.push_back(view);
        stack.push_back(advanced);
        while(!stack.empty())
        {
            // Process view on top of the stack
            LLView* curview = stack.front();
            stack.pop_front();

            LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
            if (ctrl)
            {
                LLControlVariable* control = ctrl->getControlVariable();
                if (control)
                {
                    std::string control_name = control->getName();
                    if (std::find(names.begin(), names.end(), control_name) == names.end())
                    {
                        names.push_back(control_name);
                    }
                }
            }

            for (child_list_t::const_iterator iter = curview->getChildList()->begin();
                iter != curview->getChildList()->end(); ++iter)
            {
                stack.push_back(*iter);
            }
        }
    }
}

//virtual
void LLFloaterPreference::onClose(bool app_quitting)
{
    gSavedSettings.setS32("LastPrefTab", getChild<LLTabContainer>("pref core")->getCurrentPanelIndex());
    LLPanelLogin::setAlwaysRefresh(false);
    if (!app_quitting)
    {
        cancel();
    }
}

// static
void LLFloaterPreference::onBtnOK(const LLSD& userdata)
{
    // commit any outstanding text entry
    if (hasFocus())
    {
        LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
        if (cur_focus && cur_focus->acceptsTextInput())
        {
            cur_focus->onCommit();
        }
    }

    if (canClose())
    {
        saveSettings();
        apply();

        if (userdata.asString() == "closeadvanced")
        {
            LLFloaterReg::hideInstance("prefs_graphics_advanced");
        }
        else
        {
            closeFloater(false);
        }

        //Conversation transcript and log path changed so reload conversations based on new location
        if(mPriorInstantMessageLogPath.length())
        {
            if(moveTranscriptsAndLog())
            {
                //When floaters are empty but have a chat history files, reload chat history into them
                LLFloaterIMSessionTab::reloadEmptyFloaters();
            }
            //Couldn't move files so restore the old path and show a notification
            else
            {
                gSavedPerAccountSettings.setString("InstantMessageLogPath", mPriorInstantMessageLogPath);
                LLNotificationsUtil::add("PreferenceChatPathChanged");
            }
            mPriorInstantMessageLogPath.clear();
        }

        LLUIColorTable::instance().saveUserSettings();
        gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), true);

        LLGameControl::loadFromSettings();

        // Only save once logged in and loaded per account settings
        if (mGotPersonalInfo)
        {
            gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), true);
        }
    }
    else
    {
        // Show beep, pop up dialog, etc.
        LL_INFOS("Preferences") << "Can't close preferences!" << LL_ENDL;
    }

    LLPanelLogin::updateLocationSelectorsVisibility();
    //Need to reload the navmesh if the pathing console is up
    LLHandle<LLFloaterPathfindingConsole> pathfindingConsoleHandle = LLFloaterPathfindingConsole::getInstanceHandle();
    if ( !pathfindingConsoleHandle.isDead() )
    {
        LLFloaterPathfindingConsole* pPathfindingConsole = pathfindingConsoleHandle.get();
        pPathfindingConsole->onRegionBoundaryCross();
    }
}

// static
void LLFloaterPreference::onBtnCancel(const LLSD& userdata)
{
    if (hasFocus())
    {
        LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
        if (cur_focus && cur_focus->acceptsTextInput())
        {
            cur_focus->onCommit();
        }
        refresh();
    }

    if (userdata.asString() == "closeadvanced")
    {
        cancel({"RenderQualityPerformance"});
        LLFloaterReg::hideInstance("prefs_graphics_advanced");
    }
    else
    {
        cancel();
        closeFloater();
    }
}

// static
void LLFloaterPreference::updateUserInfo(const std::string& visibility)
{
    if (LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences"))
    {
        instance->setPersonalInfo(visibility);
    }
}

void LLFloaterPreference::refreshEnabledGraphics()
{
    if (LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences"))
    {
        instance->refresh();
    }

    if (LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced"))
    {
        advanced->refresh();
    }
}

void LLFloaterPreference::onClickClearCache()
{
    LLNotificationsUtil::add("ConfirmClearCache", LLSD(), LLSD(), callback_clear_cache);
}

void LLFloaterPreference::onClickBrowserClearCache()
{
    LLNotificationsUtil::add("ConfirmClearBrowserCache", LLSD(), LLSD(), callback_clear_browser_cache);
}

// Called when user changes language via the combobox.
void LLFloaterPreference::onLanguageChange()
{
    // Let the user know that the change will only take effect after restart.
    // Do it only once so that we're not too irritating.
    if (!mLanguageChanged)
    {
        LLNotificationsUtil::add("ChangeLanguage");
        mLanguageChanged = true;
    }
}

void LLFloaterPreference::onNotificationsChange(const std::string& OptionName)
{
    mNotificationOptions[OptionName] = getChild<LLComboBox>(OptionName)->getSelectedItemLabel();

    bool show_notifications_alert = true;
    for (notifications_map::iterator it_notification = mNotificationOptions.begin(); it_notification != mNotificationOptions.end(); it_notification++)
    {
        if (it_notification->second != "No action")
        {
            show_notifications_alert = false;
            break;
        }
    }

    getChild<LLTextBox>("notifications_alert")->setVisible(show_notifications_alert);
}

void LLFloaterPreference::onNameTagOpacityChange(const LLSD& newvalue)
{
    if (LLColorSwatchCtrl* color_swatch = findChild<LLColorSwatchCtrl>("background"))
    {
        LLColor4 new_color = color_swatch->get();
        color_swatch->set(new_color.setAlpha((F32)newvalue.asReal()));
    }
}

void LLFloaterPreference::onClickSetCache()
{
    std::string cur_name(gSavedSettings.getString("CacheLocation"));
//  std::string cur_top_folder(gDirUtilp->getBaseFileName(cur_name));

    std::string proposed_name(cur_name);

    (new LLDirPickerThread(boost::bind(&LLFloaterPreference::changeCachePath, this, _1, _2), proposed_name))->getFile();
}

void LLFloaterPreference::changeCachePath(const std::vector<std::string>& filenames, std::string proposed_name)
{
    std::string dir_name = filenames[0];
    if (!dir_name.empty() && dir_name != proposed_name)
    {
        std::string new_top_folder(gDirUtilp->getBaseFileName(dir_name));
        LLNotificationsUtil::add("CacheWillBeMoved");
        gSavedSettings.setString("NewCacheLocation", dir_name);
        gSavedSettings.setString("NewCacheLocationTopFolder", new_top_folder);
    }
    else
    {
        std::string cache_location = gDirUtilp->getCacheDir();
        gSavedSettings.setString("CacheLocation", cache_location);
        std::string top_folder(gDirUtilp->getBaseFileName(cache_location));
        gSavedSettings.setString("CacheLocationTopFolder", top_folder);
    }
}

void LLFloaterPreference::onClickResetCache()
{
    if (gDirUtilp->getCacheDir(false) == gDirUtilp->getCacheDir(true))
    {
        // The cache location was already the default.
        return;
    }
    gSavedSettings.setString("NewCacheLocation", "");
    gSavedSettings.setString("NewCacheLocationTopFolder", "");
    LLNotificationsUtil::add("CacheWillBeMoved");
    std::string cache_location = gDirUtilp->getCacheDir(false);
    gSavedSettings.setString("CacheLocation", cache_location);
    std::string top_folder(gDirUtilp->getBaseFileName(cache_location));
    gSavedSettings.setString("CacheLocationTopFolder", top_folder);
}

void LLFloaterPreference::onClickSkin(LLUICtrl* ctrl, const LLSD& userdata)
{
    gSavedSettings.setString("SkinCurrent", userdata.asString());
    ctrl->setValue(userdata.asString());
}

void LLFloaterPreference::onSelectSkin()
{
    std::string skin_selection = getChild<LLRadioGroup>("skin_selection")->getValue().asString();
    gSavedSettings.setString("SkinCurrent", skin_selection);
}

void LLFloaterPreference::refreshSkin(void* data)
{
    LLPanel*self = (LLPanel*)data;
    sSkin = gSavedSettings.getString("SkinCurrent");
    self->getChild<LLRadioGroup>("skin_selection", true)->setValue(sSkin);
}

void LLFloaterPreference::buildPopupLists()
{
    LLScrollListCtrl& disabled_popups =
        getChildRef<LLScrollListCtrl>("disabled_popups");
    LLScrollListCtrl& enabled_popups =
        getChildRef<LLScrollListCtrl>("enabled_popups");

    disabled_popups.deleteAllItems();
    enabled_popups.deleteAllItems();

    for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
         iter != LLNotifications::instance().templatesEnd();
         ++iter)
    {
        LLNotificationTemplatePtr templatep = iter->second;
        LLNotificationFormPtr formp = templatep->mForm;

        LLNotificationForm::EIgnoreType ignore = formp->getIgnoreType();
        if (ignore <= LLNotificationForm::IGNORE_NO)
            continue;

        LLSD row;
        row["columns"][0]["value"] = formp->getIgnoreMessage();
        row["columns"][0]["font"] = "SANSSERIF_SMALL";
        row["columns"][0]["width"] = 400;

        LLScrollListItem* item = NULL;

        bool show_popup = !formp->getIgnored();
        if (!show_popup)
        {
            if (ignore == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
            {
                LLSD last_response = LLUI::getInstance()->mSettingGroups["config"]->getLLSD("Default" + templatep->mName);
                if (!last_response.isUndefined())
                {
                    for (LLSD::map_const_iterator it = last_response.beginMap();
                         it != last_response.endMap();
                         ++it)
                    {
                        if (it->second.asBoolean())
                        {
                            row["columns"][1]["value"] = formp->getElement(it->first)["ignore"].asString();
                            row["columns"][1]["font"] = "SANSSERIF_SMALL";
                            row["columns"][1]["width"] = 360;
                            break;
                        }
                    }
                }
            }
            item = disabled_popups.addElement(row);
        }
        else
        {
            item = enabled_popups.addElement(row);
        }

        if (item)
        {
            item->setUserdata((void*)&iter->first);
        }
    }
}

void LLFloaterPreference::refreshEnabledState()
{
    LLCheckBoxCtrl* ctrl_pbr = getChild<LLCheckBoxCtrl>("UsePBRShaders");

    //PBR
    ctrl_pbr->setEnabled(true);

    // Cannot have floater active until caps have been received
    getChild<LLButton>("default_creation_permissions")->setEnabled(LLStartUp::getStartupState() >= STATE_STARTED);

    getChildView("block_list")->setEnabled(LLLoginInstance::getInstance()->authSuccess());
}

void LLAvatarComplexityControls::setIndirectControls()
{
    /*
     * We have controls that have an indirect relationship between the control
     * values and adjacent text and the underlying setting they influence.
     * In each case, the control and its associated setting are named Indirect<something>
     * This method interrogates the controlled setting and establishes the
     * appropriate value for the indirect control. It must be called whenever the
     * underlying setting may have changed other than through the indirect control,
     * such as when the 'Reset all to recommended settings' button is used...
     */
    setIndirectMaxNonImpostors();
    setIndirectMaxArc();
}

// static
void LLAvatarComplexityControls::setIndirectMaxNonImpostors()
{
    U32 max_non_impostors = gSavedSettings.getU32("RenderAvatarMaxNonImpostors");
    // for this one, we just need to make zero, which means off, the max value of the slider
    U32 indirect_max_non_impostors = (0 == max_non_impostors) ? LLVOAvatar::NON_IMPOSTORS_MAX_SLIDER : max_non_impostors;
    gSavedSettings.setU32("IndirectMaxNonImpostors", indirect_max_non_impostors);
}

void LLAvatarComplexityControls::setIndirectMaxArc()
{
    U32 max_arc = gSavedSettings.getU32("RenderAvatarMaxComplexity");
    U32 indirect_max_arc;
    if (0 == max_arc)
    {
        // the off position is all the way to the right, so set to control max
        indirect_max_arc = INDIRECT_MAX_ARC_OFF;
    }
    else
    {
        // This is the inverse of the calculation in updateMaxComplexity
        indirect_max_arc = (U32)ll_round(((log(F32(max_arc)) - MIN_ARC_LOG) / ARC_LIMIT_MAP_SCALE)) + MIN_INDIRECT_ARC_LIMIT;
    }
    gSavedSettings.setU32("IndirectMaxComplexity", indirect_max_arc);
}

void LLFloaterPreference::refresh()
{
    LLFloater::refresh();
    LLAvatarComplexityControls::setText(
        gSavedSettings.getU32("RenderAvatarMaxComplexity"),
        getChild<LLTextBox>("IndirectMaxComplexityText", true));
    refreshEnabledState();
    if (LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced"))
    {
        advanced->refresh();
    }
    updateClickActionViews();
}

void LLFloaterPreference::onCommitWindowedMode()
{
    refresh();
}

void LLFloaterPreference::onChangeQuality(const LLSD& data)
{
    U32 level = (U32)data.asReal();
    LLFeatureManager::getInstance()->setGraphicsLevel(level, true);
    refreshEnabledGraphics();
    refresh();
}

void LLFloaterPreference::onClickSetSounds()
{
    // Disable Enable gesture sounds checkbox if the master sound is disabled
    // or if sound effects are disabled.
    getChild<LLCheckBoxCtrl>("gesture_audio_play_btn")->setEnabled(!gSavedSettings.getBOOL("MuteSounds"));
}

void LLFloaterPreference::onClickEnablePopup()
{
    LLScrollListCtrl& disabled_popups = getChildRef<LLScrollListCtrl>("disabled_popups");

    std::vector<LLScrollListItem*> items = disabled_popups.getAllSelected();
    std::vector<LLScrollListItem*>::iterator itor;
    for (itor = items.begin(); itor != items.end(); ++itor)
    {
        LLNotificationTemplatePtr templatep = LLNotifications::instance().getTemplate(*(std::string*)((*itor)->getUserdata()));
        //gSavedSettings.setWarning(templatep->mName, true);
        std::string notification_name = templatep->mName;
        LLUI::getInstance()->mSettingGroups["ignores"]->setBOOL(notification_name, true);
    }

    buildPopupLists();
    if (!mFilterEdit->getText().empty())
    {
        filterIgnorableNotifications();
    }
}

void LLFloaterPreference::onClickDisablePopup()
{
    LLScrollListCtrl& enabled_popups = getChildRef<LLScrollListCtrl>("enabled_popups");

    std::vector<LLScrollListItem*> items = enabled_popups.getAllSelected();
    std::vector<LLScrollListItem*>::iterator itor;
    for (itor = items.begin(); itor != items.end(); ++itor)
    {
        LLNotificationTemplatePtr templatep = LLNotifications::instance().getTemplate(*(std::string*)((*itor)->getUserdata()));
        templatep->mForm->setIgnored(true);
    }

    buildPopupLists();
    if (!mFilterEdit->getText().empty())
    {
        filterIgnorableNotifications();
    }
}

void LLFloaterPreference::resetAllIgnored()
{
    for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
         iter != LLNotifications::instance().templatesEnd();
         ++iter)
    {
        if (iter->second->mForm->getIgnoreType() > LLNotificationForm::IGNORE_NO)
        {
            iter->second->mForm->setIgnored(false);
        }
    }
}

void LLFloaterPreference::setAllIgnored()
{
    for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
         iter != LLNotifications::instance().templatesEnd();
         ++iter)
    {
        if (iter->second->mForm->getIgnoreType() > LLNotificationForm::IGNORE_NO)
        {
            iter->second->mForm->setIgnored(true);
        }
    }
}

void LLFloaterPreference::onClickLogPath()
{
    std::string proposed_name(gSavedPerAccountSettings.getString("InstantMessageLogPath"));
    mPriorInstantMessageLogPath.clear();

    (new LLDirPickerThread(boost::bind(&LLFloaterPreference::changeLogPath, this, _1, _2), proposed_name))->getFile();
}

void LLFloaterPreference::changeLogPath(const std::vector<std::string>& filenames, std::string proposed_name)
{
    //Path changed
    if (proposed_name != filenames[0])
    {
        gSavedPerAccountSettings.setString("InstantMessageLogPath", filenames[0]);
        mPriorInstantMessageLogPath = proposed_name;

        // enable/disable 'Delete transcripts button
        updateDeleteTranscriptsButton();
    }
}

bool LLFloaterPreference::moveTranscriptsAndLog()
{
    std::string instantMessageLogPath(gSavedPerAccountSettings.getString("InstantMessageLogPath"));
    std::string chatLogPath = gDirUtilp->add(instantMessageLogPath, gDirUtilp->getUserName());

    bool madeDirectory = false;

    //Does the directory really exist, if not then make it
    if (!LLFile::isdir(chatLogPath))
    {
        //mkdir success is defined as zero
        if (LLFile::mkdir(chatLogPath) != 0)
        {
            return false;
        }
        madeDirectory = true;
    }

    std::string originalConversationLogDir = LLConversationLog::instance().getFileName();
    std::string targetConversationLogDir = gDirUtilp->add(chatLogPath, "conversation.log");
    //Try to move the conversation log
    if (!LLConversationLog::instance().moveLog(originalConversationLogDir, targetConversationLogDir))
    {
        //Couldn't move the log and created a new directory so remove the new directory
        if (madeDirectory)
        {
            LLFile::rmdir(chatLogPath);
        }
        return false;
    }

    //Attempt to move transcripts
    std::vector<std::string> listOfTranscripts;
    std::vector<std::string> listOfFilesMoved;

    LLLogChat::getListOfTranscriptFiles(listOfTranscripts);

    if (!LLLogChat::moveTranscripts(gDirUtilp->getChatLogsDir(),
                                    instantMessageLogPath,
                                    listOfTranscripts,
                                    listOfFilesMoved))
    {
        //Couldn't move all the transcripts so restore those that moved back to their old location
        LLLogChat::moveTranscripts(instantMessageLogPath,
            gDirUtilp->getChatLogsDir(),
            listOfFilesMoved);

        //Move the conversation log back
        LLConversationLog::instance().moveLog(targetConversationLogDir, originalConversationLogDir);

        if(madeDirectory)
        {
            LLFile::rmdir(chatLogPath);
        }

        return false;
    }

    gDirUtilp->setChatLogsDir(instantMessageLogPath);
    gDirUtilp->updatePerAccountChatLogsDir();

    return true;
}

void LLFloaterPreference::setPersonalInfo(const std::string& visibility)
{
    mGotPersonalInfo = true;
    mDirectoryVisibility = visibility;

    if (visibility == VISIBILITY_DEFAULT)
    {
        mOriginalHideOnlineStatus = false;
        getChildView("online_visibility")->setEnabled(true);
    }
    else if (visibility == VISIBILITY_HIDDEN)
    {
        mOriginalHideOnlineStatus = true;
        getChildView("online_visibility")->setEnabled(true);
    }
    else
    {
        mOriginalHideOnlineStatus = true;
    }

    getChild<LLUICtrl>("online_searchresults")->setEnabled(true);
    getChildView("friends_online_notify_checkbox")->setEnabled(true);
    getChild<LLUICtrl>("online_visibility")->setValue(mOriginalHideOnlineStatus);
    getChild<LLUICtrl>("online_visibility")->setLabelArg("[DIR_VIS]", mDirectoryVisibility);

    getChildView("favorites_on_login_check")->setEnabled(true);
    getChildView("log_path_button")->setEnabled(true);
    getChildView("chat_font_size")->setEnabled(true);
    getChildView("conversation_log_combo")->setEnabled(true);
    getChild<LLUICtrl>("voice_call_friends_only_check")->setEnabled(true);
    getChild<LLUICtrl>("voice_call_friends_only_check")->setValue(gSavedPerAccountSettings.getBOOL("VoiceCallsFriendsOnly"));
}

void LLFloaterPreference::refreshUI()
{
    refresh();
}

void LLAvatarComplexityControls::updateMax(LLSliderCtrl* slider, LLTextBox* value_label, bool short_val)
{
    // Called when the IndirectMaxComplexity control changes
    // Responsible for fixing the slider label (IndirectMaxComplexityText) and setting RenderAvatarMaxComplexity
    U32 indirect_value = slider->getValue().asInteger();
    U32 max_arc;

    if (INDIRECT_MAX_ARC_OFF == indirect_value)
    {
        // The 'off' position is when the slider is all the way to the right,
        // which is a value of INDIRECT_MAX_ARC_OFF,
        // so it is necessary to set max_arc to 0 disable muted avatars.
        max_arc = 0;
    }
    else
    {
        // if this is changed, the inverse calculation in setIndirectMaxArc
        // must be changed to match
        max_arc = (U32)ll_round(exp(MIN_ARC_LOG + (ARC_LIMIT_MAP_SCALE * (indirect_value - MIN_INDIRECT_ARC_LIMIT))));
    }

    gSavedSettings.setU32("RenderAvatarMaxComplexity", (U32)max_arc);
    setText(max_arc, value_label, short_val);
}

void LLAvatarComplexityControls::setText(U32 value, LLTextBox* text_box, bool short_val)
{
    if (0 == value)
    {
        text_box->setText(LLTrans::getString("no_limit"));
    }
    else
    {
        std::string text_value = short_val ? llformat("%d", value / 1000) : llformat("%d", value);
        text_box->setText(text_value);
    }
}

void LLAvatarComplexityControls::updateMaxRenderTime(LLSliderCtrl* slider, LLTextBox* value_label, bool short_val)
{
    setRenderTimeText((F32)(LLPerfStats::renderAvatarMaxART_ns/1000), value_label, short_val);
}

void LLAvatarComplexityControls::setRenderTimeText(F32 value, LLTextBox* text_box, bool short_val)
{
    if (0 == value)
    {
        text_box->setText(LLTrans::getString("no_limit"));
    }
    else
    {
        text_box->setText(llformat("%.0f", value));
    }
}

void LLFloaterPreference::updateMaxComplexity()
{
    // Called when the IndirectMaxComplexity control changes
    LLAvatarComplexityControls::updateMax(
        getChild<LLSliderCtrl>("IndirectMaxComplexity"),
        getChild<LLTextBox>("IndirectMaxComplexityText"));
}

void LLFloaterPreference::updateComplexityText()
{
    LLAvatarComplexityControls::setText(gSavedSettings.getU32("RenderAvatarMaxComplexity"),
        getChild<LLTextBox>("IndirectMaxComplexityText", true));
}

bool LLFloaterPreference::loadFromFilename(const std::string& filename, std::map<std::string, std::string> &label_map)
{
    LLXMLNodePtr root;

    if (!LLXMLNode::parseFile(filename, root, NULL))
    {
        LL_WARNS("Preferences") << "Unable to parse file " << filename << LL_ENDL;
        return false;
    }

    if (!root->hasName("labels"))
    {
        LL_WARNS("Preferences") << filename << " is not a valid definition file" << LL_ENDL;
        return false;
    }

    LabelTable params;
    LLXUIParser parser;
    parser.readXUI(root, params, filename);

    if (params.validateBlock())
    {
        for (LLInitParam::ParamIterator<LabelDef>::const_iterator it = params.labels.begin();
            it != params.labels.end();
            ++it)
        {
            LabelDef label_entry = *it;
            label_map[label_entry.name] = label_entry.value;
        }
    }
    else
    {
        LL_WARNS("Preferences") << filename << " failed to load" << LL_ENDL;
        return false;
    }

    return true;
}

void LLFloaterPreference::onChangeMaturity()
{
    U8 sim_access = gSavedSettings.getU32("PreferredMaturity");

    getChild<LLIconCtrl>("rating_icon_general")->setVisible(sim_access == SIM_ACCESS_PG
                                                            || sim_access == SIM_ACCESS_MATURE
                                                            || sim_access == SIM_ACCESS_ADULT);

    getChild<LLIconCtrl>("rating_icon_moderate")->setVisible(sim_access == SIM_ACCESS_MATURE
                                                            || sim_access == SIM_ACCESS_ADULT);

    getChild<LLIconCtrl>("rating_icon_adult")->setVisible(sim_access == SIM_ACCESS_ADULT);
}

void LLFloaterPreference::onChangeComplexityMode(const LLSD& newvalue)
{
    bool enable_complexity = newvalue.asInteger() != LLVOAvatar::AV_RENDER_ONLY_SHOW_FRIENDS;
    getChild<LLSliderCtrl>("IndirectMaxComplexity")->setEnabled(enable_complexity);
}

std::string get_category_path(LLFolderType::EType cat_type)
{
    LLUUID cat_id = gInventory.findUserDefinedCategoryUUIDForType(cat_type);
    return get_category_path(cat_id);
}

void LLFloaterPreference::onChangeModelFolder()
{
    if (gInventory.isInventoryUsable())
    {
        getChild<LLTextBox>("upload_models")->setText(get_category_path(LLFolderType::FT_OBJECT));
    }
}

void LLFloaterPreference::onChangePBRFolder()
{
    if (gInventory.isInventoryUsable())
    {
        getChild<LLTextBox>("upload_pbr")->setText(get_category_path(LLFolderType::FT_MATERIAL));
    }
}

void LLFloaterPreference::onChangeTextureFolder()
{
    if (gInventory.isInventoryUsable())
    {
        getChild<LLTextBox>("upload_textures")->setText(get_category_path(LLFolderType::FT_TEXTURE));
    }
}

void LLFloaterPreference::onChangeSoundFolder()
{
    if (gInventory.isInventoryUsable())
    {
        getChild<LLTextBox>("upload_sounds")->setText(get_category_path(LLFolderType::FT_SOUND));
    }
}

void LLFloaterPreference::onChangeAnimationFolder()
{
    if (gInventory.isInventoryUsable())
    {
        getChild<LLTextBox>("upload_animation")->setText(get_category_path(LLFolderType::FT_ANIMATION));
    }
}

// FIXME: this will stop you from spawning the sidetray from preferences dialog on login screen
// but the UI for this will still be enabled
void LLFloaterPreference::onClickBlockList()
{
    LLFloaterSidePanelContainer::showPanel("people", "panel_people",
        LLSD().with("people_panel_tab_name", "blocked_panel"));
}

void LLFloaterPreference::onClickProxySettings()
{
    LLFloaterReg::showInstance("prefs_proxy");
}

void LLFloaterPreference::onClickTranslationSettings()
{
    LLFloaterReg::showInstance("prefs_translation");
}

void LLFloaterPreference::onClickAutoReplace()
{
    LLFloaterReg::showInstance("prefs_autoreplace");
}

void LLFloaterPreference::onClickSpellChecker()
{
    LLFloaterReg::showInstance("prefs_spellchecker");
}

void LLFloaterPreference::onClickRenderExceptions()
{
    LLFloaterReg::showInstance("avatar_render_settings");
}

void LLFloaterPreference::onClickAutoAdjustments()
{
    LLFloaterPerformance* performance_floater = LLFloaterReg::showTypedInstance<LLFloaterPerformance>("performance");
    if (performance_floater)
    {
        performance_floater->showAutoadjustmentsPanel();
    }
}

void LLFloaterPreference::onClickAdvanced()
{
    LLFloaterReg::showInstance("prefs_graphics_advanced");

    LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
    for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
         iter != tabcontainer->getChildList()->end(); ++iter)
    {
        LLView* view = *iter;
        LLPanelPreferenceGraphics* panel = dynamic_cast<LLPanelPreferenceGraphics*>(view);
        if (panel)
        {
            panel->resetDirtyChilds();
        }
    }
}

void LLFloaterPreference::onClickActionChange()
{
    updateClickActionControls();
}

void LLFloaterPreference::onAtmosShaderChange()
{
    if (LLCheckBoxCtrl* ctrl_alm = getChild<LLCheckBoxCtrl>("UseLightShaders"))
    {
        //Deferred/SSAO/Shadows
        bool bumpshiny = LLCubeMap::sUseCubeMaps && LLFeatureManager::getInstance()->isFeatureAvailable("RenderObjectBump") && gSavedSettings.getBOOL("RenderObjectBump");
        bool shaders = gSavedSettings.getBOOL("WindLightUseAtmosShaders");
        bool enabled = LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred") &&
                        bumpshiny &&
                        shaders;

        ctrl_alm->setEnabled(enabled);
    }
}

void LLFloaterPreference::onClickPermsDefault()
{
    LLFloaterReg::showInstance("perms_default");
}

void LLFloaterPreference::onClickRememberedUsernames()
{
    LLFloaterReg::showInstance("forget_username");
}

void LLFloaterPreference::onDeleteTranscripts()
{
    LLSD args;
    args["FOLDER"] = gDirUtilp->getUserName();

    LLNotificationsUtil::add("PreferenceChatDeleteTranscripts", args, LLSD(), boost::bind(&LLFloaterPreference::onDeleteTranscriptsResponse, this, _1, _2));
}

void LLFloaterPreference::onDeleteTranscriptsResponse(const LLSD& notification, const LLSD& response)
{
    if (0 == LLNotificationsUtil::getSelectedOption(notification, response))
    {
        LLLogChat::deleteTranscripts();
        updateDeleteTranscriptsButton();
    }
}

void LLFloaterPreference::onLogChatHistorySaved()
{
    LLButton* delete_transcripts_buttonp = getChild<LLButton>("delete_transcripts");

    if (!delete_transcripts_buttonp->getEnabled())
    {
        delete_transcripts_buttonp->setEnabled(true);
    }
}

void LLFloaterPreference::updateClickActionControls()
{
    const int single_clk_action = getChild<LLComboBox>("single_click_action_combo")->getValue().asInteger();
    const int double_clk_action = getChild<LLComboBox>("double_click_action_combo")->getValue().asInteger();

    // Todo: This is a very ugly way to get access to keybindings.
    // Reconsider possible options.
    // Potential option: make constructor of LLKeyConflictHandler private
    // but add a getter that will return shared pointer for specific
    // mode, pointer should only exist so long as there are external users.
    // In such case we won't need to do this 'dynamic_cast' nightmare.
    // updateTable() can also be avoided
    LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
    for (LLView* view : *tabcontainer->getChildList())
    {
        if (LLPanelPreferenceControls* panel = dynamic_cast<LLPanelPreferenceControls*>(view))
        {
            panel->setKeyBind("walk_to",
                              EMouseClickType::CLICK_LEFT,
                              KEY_NONE,
                              MASK_NONE,
                              single_clk_action == 1);

            panel->setKeyBind("walk_to",
                              EMouseClickType::CLICK_DOUBLELEFT,
                              KEY_NONE,
                              MASK_NONE,
                              double_clk_action == 1);

            panel->setKeyBind("teleport_to",
                              EMouseClickType::CLICK_DOUBLELEFT,
                              KEY_NONE,
                              MASK_NONE,
                              double_clk_action == 2);

            panel->updateAndApply();
        }
    }
}

void LLFloaterPreference::updateClickActionViews()
{
    bool click_to_walk = false;
    bool dbl_click_to_walk = false;
    bool dbl_click_to_teleport = false;

    // Todo: This is a very ugly way to get access to keybindings.
    // Reconsider possible options.
    LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
    for (LLView* view : *tabcontainer->getChildList())
    {
        if (LLPanelPreferenceControls* panel = dynamic_cast<LLPanelPreferenceControls*>(view))
        {
            click_to_walk = panel->canKeyBindHandle("walk_to",
                EMouseClickType::CLICK_LEFT,
                KEY_NONE,
                MASK_NONE);

            dbl_click_to_walk = panel->canKeyBindHandle("walk_to",
                EMouseClickType::CLICK_DOUBLELEFT,
                KEY_NONE,
                MASK_NONE);

            dbl_click_to_teleport = panel->canKeyBindHandle("teleport_to",
                EMouseClickType::CLICK_DOUBLELEFT,
                KEY_NONE,
                MASK_NONE);
        }
    }

    getChild<LLComboBox>("single_click_action_combo")->setValue((int)click_to_walk);
    getChild<LLComboBox>("double_click_action_combo")->setValue(dbl_click_to_teleport ? 2 : (int)dbl_click_to_walk);
}

void LLFloaterPreference::updateSearchableItems()
{
    mSearchDataDirty = true;
}

void LLFloaterPreference::applyUIColor(LLUICtrl* ctrl, const LLSD& param)
{
    LLUIColorTable::instance().setColor(param.asString(), LLColor4(ctrl->getValue()));
}

void LLFloaterPreference::getUIColor(LLUICtrl* ctrl, const LLSD& param)
{
    LLColorSwatchCtrl* color_swatch = (LLColorSwatchCtrl*) ctrl;
    color_swatch->setOriginal(LLUIColorTable::instance().getColor(param.asString()));
}

void LLFloaterPreference::setCacheLocation(const LLStringExplicit& location)
{
    LLUICtrl* cache_location_editor = getChild<LLUICtrl>("cache_location");
    cache_location_editor->setValue(location);
    cache_location_editor->setToolTip(location);
}

void LLFloaterPreference::selectPanel(const LLSD& name)
{
    LLTabContainer * tab_containerp = getChild<LLTabContainer>("pref core");
    LLPanel * panel = tab_containerp->getPanelByName(name.asStringRef());
    if (NULL != panel)
    {
        tab_containerp->selectTabPanel(panel);
    }
}

void LLFloaterPreference::selectPrivacyPanel()
{
    selectPanel("im");
}

void LLFloaterPreference::selectChatPanel()
{
    selectPanel("chat");
}

void LLFloaterPreference::changed()
{
    getChild<LLButton>("clear_log")->setEnabled(LLConversationLog::instance().getConversations().size() > 0);

    // set 'enable' property for 'Delete transcripts...' button
    updateDeleteTranscriptsButton();
}

void LLFloaterPreference::saveGraphicsPreset(std::string& preset)
{
    mSavedGraphicsPreset = preset;
}

//------------------------------Updater---------------------------------------

static bool handleBandwidthChanged(const LLSD& newvalue)
{
    gViewerThrottle.setMaxBandwidth((F32) newvalue.asReal());
    return true;
}

class LLPanelPreference::Updater : public LLEventTimer
{

public:

    typedef boost::function<bool(const LLSD&)> callback_t;

    Updater(callback_t cb, F32 period)
    :LLEventTimer(period),
     mCallback(cb)
    {
        mEventTimer.stop();
    }

    virtual ~Updater(){}

    void update(const LLSD& new_value)
    {
        mNewValue = new_value;
        mEventTimer.start();
    }

protected:

    bool tick()
    {
        mCallback(mNewValue);
        mEventTimer.stop();

        return false;
    }

private:

    LLSD mNewValue;
    callback_t mCallback;
};
//----------------------------------------------------------------------------
static LLPanelInjector<LLPanelPreference> t_places("panel_preference");
LLPanelPreference::LLPanelPreference()
: LLPanel(),
  mBandWidthUpdater(NULL)
{
    mCommitCallbackRegistrar.add("Pref.setControlFalse",    boost::bind(&LLPanelPreference::setControlFalse,this, _2));
    mCommitCallbackRegistrar.add("Pref.updateMediaAutoPlayCheckbox",    boost::bind(&LLPanelPreference::updateMediaAutoPlayCheckbox, this, _1));
    mCommitCallbackRegistrar.add("Pref.PrefDelete", boost::bind(&LLPanelPreference::deletePreset, this, _2));
    mCommitCallbackRegistrar.add("Pref.PrefSave",   boost::bind(&LLPanelPreference::savePreset, this, _2));
    mCommitCallbackRegistrar.add("Pref.PrefLoad",   boost::bind(&LLPanelPreference::loadPreset, this, _2));
}

//virtual
bool LLPanelPreference::postBuild()
{
    ////////////////////// PanelGeneral ///////////////////
    if (hasChild("display_names_check", true))
    {
        bool use_people_api = gSavedSettings.getBOOL("UsePeopleAPI");
        LLCheckBoxCtrl* ctrl_display_name = getChild<LLCheckBoxCtrl>("display_names_check");
        ctrl_display_name->setEnabled(use_people_api);
        if (!use_people_api)
        {
            ctrl_display_name->setValue(false);
        }
    }

    ////////////////////// PanelVoice ///////////////////
    if (hasChild("voice_unavailable", true))
    {
        bool voice_disabled = gSavedSettings.getBOOL("CmdLineDisableVoice");
        getChildView("voice_unavailable")->setVisible( voice_disabled);
        getChildView("enable_voice_check")->setVisible( !voice_disabled);
    }

    //////////////////////PanelSkins ///////////////////

    if (hasChild("skin_selection", true))
    {
        LLFloaterPreference::refreshSkin(this);
        // if skin is set to a skin that no longer exists (silver) set back to default
        if (getChild<LLRadioGroup>("skin_selection")->getSelectedIndex() < 0)
        {
            gSavedSettings.setString("SkinCurrent", "default");
            LLFloaterPreference::refreshSkin(this);
        }
    }

    //////////////////////PanelPrivacy ///////////////////
    if (hasChild("media_enabled", true))
    {
        bool media_enabled = gSavedSettings.getBOOL("AudioStreamingMedia");

        getChild<LLCheckBoxCtrl>("media_enabled")->set(media_enabled);
        getChild<LLCheckBoxCtrl>("autoplay_enabled")->setEnabled(media_enabled);
    }
    if (hasChild("music_enabled", true))
    {
        getChild<LLCheckBoxCtrl>("music_enabled")->set(gSavedSettings.getBOOL("AudioStreamingMusic"));
    }
    if (hasChild("voice_call_friends_only_check", true))
    {
        getChild<LLCheckBoxCtrl>("voice_call_friends_only_check")->setCommitCallback(boost::bind(&showFriendsOnlyWarning, _1, _2));
    }
    if (hasChild("allow_multiple_viewer_check", true))
    {
        getChild<LLCheckBoxCtrl>("allow_multiple_viewer_check")->setCommitCallback(boost::bind(&showMultipleViewersWarning, _1, _2));
    }
    if (hasChild("favorites_on_login_check", true))
    {
        getChild<LLCheckBoxCtrl>("favorites_on_login_check")->setCommitCallback(boost::bind(&handleFavoritesOnLoginChanged, _1, _2));
        bool show_favorites_at_login = LLPanelLogin::getShowFavorites();
        getChild<LLCheckBoxCtrl>("favorites_on_login_check")->setValue(show_favorites_at_login);
    }
    if (hasChild("mute_chb_label", true))
    {
        getChild<LLTextBox>("mute_chb_label")->setShowCursorHand(false);
        getChild<LLTextBox>("mute_chb_label")->setSoundFlags(LLView::MOUSE_UP);
        getChild<LLTextBox>("mute_chb_label")->setClickedCallback(boost::bind(&toggleMuteWhenMinimized));
    }

    //////////////////////PanelSetup ///////////////////
    if (hasChild("max_bandwidth", true))
    {
        mBandWidthUpdater = new LLPanelPreference::Updater(boost::bind(&handleBandwidthChanged, _1), BANDWIDTH_UPDATER_TIMEOUT);
        gSavedSettings.getControl("ThrottleBandwidthKBPS")->getSignal()->connect(boost::bind(&LLPanelPreference::Updater::update, mBandWidthUpdater, _2));
    }

#ifdef EXTERNAL_TOS
    LLRadioGroup* ext_browser_settings = getChild<LLRadioGroup>("preferred_browser_behavior");
    if (ext_browser_settings)
    {
        // turn off ability to set external/internal browser
        ext_browser_settings->setSelectedByValue(LLWeb::BROWSER_EXTERNAL_ONLY, true);
        ext_browser_settings->setEnabled(false);
    }
#endif

    apply();
    return true;
}

LLPanelPreference::~LLPanelPreference()
{
    if (mBandWidthUpdater)
    {
        delete mBandWidthUpdater;
    }
}

// virtual
void LLPanelPreference::apply()
{
    // no-op
}

// virtual
void LLPanelPreference::saveSettings()
{
    LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");

    // Save the value of all controls in the hierarchy
    mSavedValues.clear();
    std::list<LLView*> view_stack;
    view_stack.push_back(this);
    if (advanced)
    {
        view_stack.push_back(advanced);
    }
    while (!view_stack.empty())
    {
        // Process view on top of the stack
        LLView* curview = view_stack.front();
        view_stack.pop_front();

        if (LLColorSwatchCtrl* color_swatch = dynamic_cast<LLColorSwatchCtrl*>(curview))
        {
            mSavedColors[color_swatch->getName()] = color_swatch->get();
        }
        else
        {
            if (LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview))
            {
                if (LLControlVariable* control = ctrl->getControlVariable())
                {
                    mSavedValues[control] = control->getValue();
                }
            }
        }

        // Push children onto the end of the work stack
        for (LLView* view : *curview->getChildList())
        {
            view_stack.push_back(view);
        }
    }

    if (LLStartUp::getStartupState() == STATE_STARTED)
    {
        LLControlVariable* control = gSavedPerAccountSettings.getControl("VoiceCallsFriendsOnly");
        if (control)
        {
            mSavedValues[control] = control->getValue();
        }
    }
}

void LLPanelPreference::showMultipleViewersWarning(LLUICtrl* checkbox, const LLSD& value)
{
    if (checkbox && checkbox->getValue())
    {
        LLNotificationsUtil::add("AllowMultipleViewers");
    }
}

void LLPanelPreference::showFriendsOnlyWarning(LLUICtrl* checkbox, const LLSD& value)
{
    if (checkbox)
    {
        gSavedPerAccountSettings.setBOOL("VoiceCallsFriendsOnly", checkbox->getValue().asBoolean());
        if (checkbox->getValue())
        {
            LLNotificationsUtil::add("FriendsAndGroupsOnly");
        }
    }
}

void LLPanelPreference::handleFavoritesOnLoginChanged(LLUICtrl* checkbox, const LLSD& value)
{
    if (checkbox)
    {
        LLFavoritesOrderStorage::instance().showFavoritesOnLoginChanged(checkbox->getValue().asBoolean());
        if(checkbox->getValue())
        {
            LLNotificationsUtil::add("FavoritesOnLogin");
        }
    }
}

void LLPanelPreference::toggleMuteWhenMinimized()
{
    std::string mute("MuteWhenMinimized");
    gSavedSettings.setBOOL(mute, !gSavedSettings.getBOOL(mute));
    LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
    if (instance)
    {
        instance->getChild<LLCheckBoxCtrl>("mute_when_minimized")->setBtnFocus();
    }
}

// virtual
void LLPanelPreference::cancel()
{
    for (const auto& iter : mSavedValues)
    {
        if ((iter.first->getName() != "InstantMessageLogPath") || !iter.second.asString().empty())
        {
            iter.first->set(iter.second);
        }
    }

    for (const auto& iter : mSavedColors)
    {
        if (LLColorSwatchCtrl* color_swatch = findChild<LLColorSwatchCtrl>(iter.first))
        {
            color_swatch->set(iter.second);
            color_swatch->onCommit();
        }
    }
}

void LLPanelPreference::setControlFalse(const LLSD& user_data)
{
    std::string control_name = user_data.asString();
    LLControlVariable* control = findControl(control_name);

    if (control)
        control->set(LLSD(false));
}

void LLPanelPreference::updateMediaAutoPlayCheckbox(LLUICtrl* ctrl)
{
    std::string name = ctrl->getName();

    // Disable "Allow Media to auto play" only when both
    // "Streaming Music" and "Media" are unchecked. STORM-513.
    if ((name == "enable_music") || (name == "enable_media"))
    {
        bool music_enabled = getChild<LLCheckBoxCtrl>("enable_music")->get();
        bool media_enabled = getChild<LLCheckBoxCtrl>("enable_media")->get();

        getChild<LLCheckBoxCtrl>("media_auto_play_combo")->setEnabled(music_enabled || media_enabled);
    }
}

void LLPanelPreference::deletePreset(const LLSD& user_data)
{
    LLFloaterReg::showInstance("delete_pref_preset", user_data.asString());
}

void LLPanelPreference::savePreset(const LLSD& user_data)
{
    LLFloaterReg::showInstance("save_pref_preset", user_data.asString());
}

void LLPanelPreference::loadPreset(const LLSD& user_data)
{
    LLFloaterReg::showInstance("load_pref_preset", user_data.asString());
}

// virtual
void LLPanelPreference::setHardwareDefaults()
{
}

class LLPanelPreferencePrivacy : public LLPanelPreference
{
public:
    LLPanelPreferencePrivacy()
    {
        mAccountIndependentSettings.push_back("AutoDisengageMic");
    }

    void saveSettings() override
    {
        LLPanelPreference::saveSettings();

        // Don't save (=erase from the saved values map) per-account privacy settings
        // if we're not logged in, otherwise they will be reset to defaults on log off.
        if (LLStartUp::getStartupState() != STATE_STARTED)
        {
            // Erase only common settings, assuming there are no color settings on Privacy page.
            for (control_values_map_t::iterator it = mSavedValues.begin(); it != mSavedValues.end(); )
            {
                const std::string setting = it->first->getName();
                if (find(mAccountIndependentSettings.begin(),
                    mAccountIndependentSettings.end(), setting) == mAccountIndependentSettings.end())
                {
                    it = mSavedValues.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

private:
    std::list<std::string> mAccountIndependentSettings;
};

static LLPanelInjector<LLPanelPreferenceGraphics> t_pref_graph("panel_preference_graphics");
static LLPanelInjector<LLPanelPreferencePrivacy> t_pref_privacy("panel_preference_privacy");

bool LLPanelPreferenceGraphics::postBuild()
{
    LLFloaterReg::showInstance("prefs_graphics_advanced");
    LLFloaterReg::hideInstance("prefs_graphics_advanced");

    resetDirtyChilds();
    setPresetText();

    LLPresetsManager* presetsMgr = LLPresetsManager::getInstance();
    presetsMgr->setPresetListChangeCallback(boost::bind(&LLPanelPreferenceGraphics::onPresetsListChange, this));
    presetsMgr->createMissingDefault(PRESETS_GRAPHIC); // a no-op after the first time, but that's ok

    return LLPanelPreference::postBuild();
}

void LLPanelPreferenceGraphics::draw()
{
    setPresetText();
    LLPanelPreference::draw();
}

void LLPanelPreferenceGraphics::onPresetsListChange()
{
    resetDirtyChilds();
    setPresetText();

    LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
    if (instance && !gSavedSettings.getString("PresetGraphicActive").empty())
    {
        instance->saveSettings(); //make cancel work correctly after changing the preset
    }
}

void LLPanelPreferenceGraphics::setPresetText()
{
    LLTextBox* preset_text = getChild<LLTextBox>("preset_text");

    std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");

    if (!preset_graphic_active.empty() && preset_graphic_active != preset_text->getText())
    {
        LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
        if (instance)
        {
            instance->saveGraphicsPreset(preset_graphic_active);
        }
    }

    if (hasDirtyChilds() && !preset_graphic_active.empty())
    {
        gSavedSettings.setString("PresetGraphicActive", "");
        preset_graphic_active.clear();
        // This doesn't seem to cause an infinite recursion.  This trigger is needed to cause the pulldown
        // panel to update.
        LLPresetsManager::getInstance()->triggerChangeSignal();
    }

    if (!preset_graphic_active.empty())
    {
        if (preset_graphic_active == PRESETS_DEFAULT)
        {
            preset_graphic_active = LLTrans::getString(PRESETS_DEFAULT);
        }
        preset_text->setText(preset_graphic_active);
    }
    else
    {
        preset_text->setText(LLTrans::getString("none_paren_cap"));
    }

    preset_text->resetDirty();
}

bool LLPanelPreferenceGraphics::hasDirtyChilds()
{
    LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");
    std::list<LLView*> view_stack;
    view_stack.push_back(this);
    if (advanced)
    {
        view_stack.push_back(advanced);
    }
    while(!view_stack.empty())
    {
        // Process view on top of the stack
        LLView* curview = view_stack.front();
        view_stack.pop_front();

        LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
        if (ctrl)
        {
            if (ctrl->isDirty())
            {
                LLControlVariable* control = ctrl->getControlVariable();
                if (control)
                {
                    std::string control_name = control->getName();
                    if (!control_name.empty())
                    {
                        return true;
                    }
                }
            }
        }
        // Push children onto the end of the work stack
        for (child_list_t::const_iterator iter = curview->getChildList()->begin();
             iter != curview->getChildList()->end(); ++iter)
        {
            view_stack.push_back(*iter);
        }
    }

    return false;
}

void LLPanelPreferenceGraphics::resetDirtyChilds()
{
    LLFloater* advanced = LLFloaterReg::findTypedInstance<LLFloater>("prefs_graphics_advanced");
    std::list<LLView*> view_stack;
    view_stack.push_back(this);
    if (advanced)
    {
        view_stack.push_back(advanced);
    }
    while(!view_stack.empty())
    {
        // Process view on top of the stack
        LLView* curview = view_stack.front();
        view_stack.pop_front();

        LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
        if (ctrl)
        {
            ctrl->resetDirty();
        }
        // Push children onto the end of the work stack
        for (child_list_t::const_iterator iter = curview->getChildList()->begin();
             iter != curview->getChildList()->end(); ++iter)
        {
            view_stack.push_back(*iter);
        }
    }
}

void LLPanelPreferenceGraphics::cancel(const std::vector<std::string> settings_to_skip)
{
    LLPanelPreference::cancel(settings_to_skip);
}

void LLPanelPreferenceGraphics::saveSettings()
{
    resetDirtyChilds();

    std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");
    if (preset_graphic_active.empty())
    {
        LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
        if (instance)
        {
            //don't restore previous preset after closing Preferences
            instance->saveGraphicsPreset(preset_graphic_active);
        }
    }

    LLPanelPreference::saveSettings();
}

void LLPanelPreferenceGraphics::setHardwareDefaults()
{
    resetDirtyChilds();
}

//------------------------LLPanelPreferenceControls--------------------------------
static LLPanelInjector<LLPanelPreferenceControls> t_pref_contrls("panel_preference_controls");

LLPanelPreferenceControls::LLPanelPreferenceControls()
    :LLPanelPreference(),
    mEditingColumn(-1),
    mEditingMode(0)
{
    // MODE_COUNT - 1 because there are currently no settings assigned to 'saved settings'.
    for (U32 i = 0; i < LLKeyConflictHandler::MODE_COUNT - 1; ++i)
    {
        mConflictHandler[i].setLoadMode((LLKeyConflictHandler::ESourceMode)i);
    }
}

LLPanelPreferenceControls::~LLPanelPreferenceControls()
{
}

bool LLPanelPreferenceControls::postBuild()
{
    // populate list of controls
    pControlsTable = getChild<LLScrollListCtrl>("controls_list");
    pKeyModeBox = getChild<LLComboBox>("key_mode");

    pControlsTable->setCommitCallback(boost::bind(&LLPanelPreferenceControls::onListCommit, this));
    pKeyModeBox->setCommitCallback(boost::bind(&LLPanelPreferenceControls::onModeCommit, this));
    getChild<LLButton>("restore_defaults")->setCommitCallback(boost::bind(&LLPanelPreferenceControls::onRestoreDefaultsBtn, this));

    return true;
}

void LLPanelPreferenceControls::regenerateControls()
{
    mEditingMode = pKeyModeBox->getValue().asInteger();
    mConflictHandler[mEditingMode].loadFromSettings((LLKeyConflictHandler::ESourceMode)mEditingMode);
    populateControlTable();
}

bool LLPanelPreferenceControls::addControlTableColumns(const std::string &filename)
{
    LLXMLNodePtr xmlNode;
    LLScrollListCtrl::Contents contents;
    if (!LLUICtrlFactory::getLayeredXMLNode(filename, xmlNode))
    {
        LL_WARNS("Preferences") << "Failed to load " << filename << LL_ENDL;
        return false;
    }
    LLXUIParser parser;
    parser.readXUI(xmlNode, contents, filename);

    if (!contents.validateBlock())
    {
        return false;
    }

    for (LLInitParam::ParamIterator<LLScrollListColumn::Params>::const_iterator col_it = contents.columns.begin();
        col_it != contents.columns.end();
        ++col_it)
    {
        pControlsTable->addColumn(*col_it);
    }

    return true;
}

bool LLPanelPreferenceControls::addControlTableRows(const std::string &filename)
{
    LLXMLNodePtr xmlNode;
    LLScrollListCtrl::Contents contents;
    if (!LLUICtrlFactory::getLayeredXMLNode(filename, xmlNode))
    {
        LL_WARNS("Preferences") << "Failed to load " << filename << LL_ENDL;
        return false;
    }
    LLXUIParser parser;
    parser.readXUI(xmlNode, contents, filename);

    if (!contents.validateBlock())
    {
        return false;
    }

    LLScrollListCell::Params cell_params;
    // init basic cell params
    cell_params.font = LLFontGL::getFontSansSerif();
    cell_params.font_halign = LLFontGL::LEFT;
    cell_params.column = "";
    cell_params.value = "";

    for (LLScrollListItem::Params& row_params : contents.rows)
    {
        std::string control = row_params.value.getValue().asString();
        if (!control.empty() && control != "menu_separator")
        {
            bool show = true;
            bool enabled = mConflictHandler[mEditingMode].canAssignControl(control);
            if (!enabled)
            {
                // If empty: this is a placeholder to make sure user won't assign
                // value by accident, don't show it
                // If not empty: predefined control combination user should see
                // to know that combination is reserved
                show = !mConflictHandler[mEditingMode].isControlEmpty(control);
                // example: teleport_to and walk_to in first person view, and
                // sitting related functions, see generatePlaceholders()
            }

            if (show)
            {
                // At the moment viewer is hardcoded to assume that columns are named as lst_ctrl%d
                LLScrollListItem::Params item_params(row_params);
                item_params.enabled.setValue(enabled);

                S32 num_columns = pControlsTable->getNumColumns();
                for (S32 col = 1; col < num_columns; col++)
                {
                    cell_params.column = llformat("lst_ctrl%d", col);
                    cell_params.value = mConflictHandler[mEditingMode].getControlString(control, col - 1);
                    item_params.columns.add(cell_params);
                }
                pControlsTable->addRow(item_params, EAddPosition::ADD_BOTTOM);
            }
        }
        else
        {
            // Separator example:
            // <rows
            //  enabled = "false">
            //  <columns
            //   type = "icon"
            //   color = "0 0 0 0.7"
            //   halign = "center"
            //   value = "menu_separator"
            //   column = "lst_action" / >
            //</rows>
            pControlsTable->addRow(row_params, EAddPosition::ADD_BOTTOM);
        }
    }
    return true;
}

void LLPanelPreferenceControls::addControlTableSeparator()
{
    LLScrollListItem::Params separator_params;
    separator_params.enabled(false);
    LLScrollListCell::Params column_params;
    column_params.type = "icon";
    column_params.value = "menu_separator";
    column_params.column = "lst_action";
    column_params.color = LLColor4(0.f, 0.f, 0.f, 0.7f);
    column_params.font_halign = LLFontGL::HCENTER;
    separator_params.columns.add(column_params);
    pControlsTable->addRow(separator_params, EAddPosition::ADD_BOTTOM);
}

void LLPanelPreferenceControls::populateControlTable()
{
    pControlsTable->clearRows();
    pControlsTable->clearColumns();

    // Add columns
    std::string filename;
    switch ((LLKeyConflictHandler::ESourceMode)mEditingMode)
    {
    case LLKeyConflictHandler::MODE_THIRD_PERSON:
    case LLKeyConflictHandler::MODE_FIRST_PERSON:
    case LLKeyConflictHandler::MODE_EDIT_AVATAR:
    case LLKeyConflictHandler::MODE_SITTING:
        filename = "control_table_contents_columns_basic.xml";
        break;
    default:
        {
            // Either unknown mode or MODE_SAVED_SETTINGS
            // It doesn't have UI or actual settings yet
            LL_WARNS("Preferences") << "Unimplemented mode" << LL_ENDL;

            // Searchable columns were removed, mark searchables for an update
            LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
            if (instance)
            {
                instance->updateSearchableItems();
            }
            return;
        }
    }
    addControlTableColumns(filename);

    // Add rows.
    // Each file represents individual visual group (movement/camera/media...)
    if (mEditingMode == LLKeyConflictHandler::MODE_FIRST_PERSON)
    {
        // Don't display whole camera and editing groups
        addControlTableRows("control_table_contents_movement.xml");
        addControlTableSeparator();
        addControlTableRows("control_table_contents_media.xml");
        addControlTableSeparator();
        addControlTableRows("control_table_contents_game_control.xml");
    }
    // MODE_THIRD_PERSON; MODE_EDIT_AVATAR; MODE_SITTING
    else if (mEditingMode < LLKeyConflictHandler::MODE_SAVED_SETTINGS)
    {
        // In case of 'sitting' mode, movements still apply due to vehicles
        // but walk_to is not supported and will be hidden by addControlTableRows
        addControlTableRows("control_table_contents_movement.xml");
        addControlTableSeparator();

        addControlTableRows("control_table_contents_camera.xml");
        addControlTableSeparator();

        addControlTableRows("control_table_contents_editing.xml");
        addControlTableSeparator();

        addControlTableRows("control_table_contents_media.xml");
        addControlTableSeparator();

        addControlTableRows("control_table_contents_game_control.xml");
    }
    else
    {
        LL_WARNS("Preferences") << "Unimplemented mode" << LL_ENDL;
    }

    // explicit update to make sure table is ready for llsearchableui
    pControlsTable->updateColumns();

    // Searchable columns were removed and readded, mark searchables for an update
    // Note: at the moment tables/lists lack proper llsearchableui support
    LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
    if (instance)
    {
        instance->updateSearchableItems();
    }
}

void LLPanelPreferenceControls::updateTable()
{
    mEditingControl.clear();
    std::vector<LLScrollListItem*> list = pControlsTable->getAllData();
    for (S32 i = 0; i < list.size(); ++i)
    {
        std::string control = list[i]->getValue();
        if (!control.empty())
        {
            LLScrollListCell* cell = NULL;

            S32 num_columns = pControlsTable->getNumColumns();
            for (S32 col = 1; col < num_columns; col++)
            {
                cell = list[i]->getColumn(col);
                cell->setValue(mConflictHandler[mEditingMode].getControlString(control, col - 1));
            }
        }
    }
    pControlsTable->deselectAllItems();
}

void LLPanelPreferenceControls::apply()
{
    for (U32 i = 0; i < LLKeyConflictHandler::MODE_COUNT - 1; ++i)
    {
        if (mConflictHandler[i].hasUnsavedChanges())
        {
            mConflictHandler[i].saveToSettings();
        }
    }
}

void LLPanelPreferenceControls::cancel(const std::vector<std::string> settings_to_skip)
{
    for (U32 i = 0; i < LLKeyConflictHandler::MODE_COUNT - 1; ++i)
    {
        if (mConflictHandler[i].hasUnsavedChanges())
        {
            mConflictHandler[i].clear();
            if (mEditingMode == i)
            {
                // cancel() can be called either when preferences floater closes
                // or when child floater closes (like advanced graphical settings)
                // in which case we need to clear and repopulate table
                regenerateControls();
            }
        }
    }
}

void LLPanelPreferenceControls::saveSettings()
{
    for (U32 i = 0; i < LLKeyConflictHandler::MODE_COUNT - 1; ++i)
    {
        if (mConflictHandler[i].hasUnsavedChanges())
        {
            mConflictHandler[i].saveToSettings();
            mConflictHandler[i].clear();
        }
    }

    S32 mode = pKeyModeBox->getValue().asInteger();
    if (mConflictHandler[mode].empty() || pControlsTable->isEmpty())
    {
        regenerateControls();
    }
}

void LLPanelPreferenceControls::resetDirtyChilds()
{
    regenerateControls();
}

void LLPanelPreferenceControls::onListCommit()
{
    LLScrollListItem* item = pControlsTable->getFirstSelected();
    if (item == NULL)
    {
        return;
    }

    std::string control = item->getValue();

    if (control.empty())
    {
        pControlsTable->deselectAllItems();
        return;
    }

    if (!mConflictHandler[mEditingMode].canAssignControl(control))
    {
        pControlsTable->deselectAllItems();
        return;
    }

    S32 cell_ind = item->getSelectedCell();
    if (cell_ind <= 0)
    {
        pControlsTable->deselectAllItems();
        return;
    }

    // List does not tell us what cell was clicked, so we have to figure it out manually, but
    // fresh mouse coordinates are not yet accessible during onCommit() and there are other issues,
    // so we cheat: remember item user clicked at, trigger 'key dialog' on hover that comes next,
    // use coordinates from hover to calculate cell

    LLScrollListCell* cell = item->getColumn(cell_ind);
    if (cell)
    {
        LLSetKeyBindDialog* dialog = LLFloaterReg::getTypedInstance<LLSetKeyBindDialog>("keybind_dialog", LLSD());
        if (dialog)
        {
            mEditingControl = control;
            mEditingColumn = cell_ind;
            dialog->setParent(this, pControlsTable, DEFAULT_KEY_FILTER);

            LLFloater* root_floater = gFloaterView->getParentFloater(this);
            if (root_floater)
                root_floater->addDependentFloater(dialog);
            dialog->openFloater();
            dialog->setFocus(true);
        }
    }
    else
    {
        pControlsTable->deselectAllItems();
    }
}

void LLPanelPreferenceControls::onModeCommit()
{
    mEditingMode = pKeyModeBox->getValue().asInteger();
    if (mConflictHandler[mEditingMode].empty())
    {
        // opening for first time
        mConflictHandler[mEditingMode].loadFromSettings((LLKeyConflictHandler::ESourceMode)mEditingMode);
    }
    populateControlTable();
}

void LLPanelPreferenceControls::onRestoreDefaultsBtn()
{
    LLNotificationsUtil::add("PreferenceControlsDefaults", LLSD(), LLSD(), boost::bind(&LLPanelPreferenceControls::onRestoreDefaultsResponse, this, _1, _2));
}

void LLPanelPreferenceControls::onRestoreDefaultsResponse(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch(option)
    {
    case 0: // All
        for (U32 i = 0; i < LLKeyConflictHandler::MODE_COUNT - 1; ++i)
        {
            mConflictHandler[i].resetToDefaults();
            // Apply changes to viewer as 'temporary'
            mConflictHandler[i].saveToSettings(true);

            // notify comboboxes in move&view about potential change
            LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
            if (instance)
            {
                instance->updateClickActionViews();
            }
        }

        updateTable();
        break;
    case 1: // Current
        mConflictHandler[mEditingMode].resetToDefaults();
        // Apply changes to viewer as 'temporary'
        mConflictHandler[mEditingMode].saveToSettings(true);

        if (mEditingMode == LLKeyConflictHandler::MODE_THIRD_PERSON)
        {
            // notify comboboxes in move&view about potential change
            LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
            if (instance)
            {
                instance->updateClickActionViews();
            }
        }

        updateTable();
        break;
    case 2: // Cancel
    default:
        //exit;
        break;
    }
}

// Bypass to let Move & view read values without need to create own key binding handler
// Assumes third person view
// Might be better idea to just move whole mConflictHandler into LLFloaterPreference
bool LLPanelPreferenceControls::canKeyBindHandle(const std::string &control, EMouseClickType click, KEY key, MASK mask)
{
    S32 mode = LLKeyConflictHandler::MODE_THIRD_PERSON;
    if (mConflictHandler[mode].empty())
    {
        // opening for first time
        mConflictHandler[mode].loadFromSettings(LLKeyConflictHandler::MODE_THIRD_PERSON);
    }

    return mConflictHandler[mode].canHandleControl(control, click, key, mask);
}

// Bypass to let Move & view modify values without need to create own key binding handler
// Assumes third person view
// Might be better idea to just move whole mConflictHandler into LLFloaterPreference
void LLPanelPreferenceControls::setKeyBind(const std::string &control, EMouseClickType click, KEY key, MASK mask, bool set)
{
    S32 mode = LLKeyConflictHandler::MODE_THIRD_PERSON;
    if (mConflictHandler[mode].empty())
    {
        // opening for first time
        mConflictHandler[mode].loadFromSettings(LLKeyConflictHandler::MODE_THIRD_PERSON);
    }

    if (!mConflictHandler[mode].canAssignControl(mEditingControl))
    {
        return;
    }

    bool already_recorded = mConflictHandler[mode].canHandleControl(control, click, key, mask);
    if (set)
    {
        if (already_recorded)
        {
            // nothing to do
            return;
        }

        // find free spot to add data, if no free spot, assign to first
        S32 index = 0;
        for (S32 i = 0; i < 3; i++)
        {
            if (mConflictHandler[mode].getControl(control, i).isEmpty())
            {
                index = i;
                break;
            }
        }
        // At the moment 'ignore_mask' mask is mostly ignored, a placeholder
        // Todo: implement it since it's preferable for things like teleport to match
        // mask exactly but for things like running to ignore additional masks
        // Ideally this needs representation in keybindings UI
        bool ignore_mask = true;
        mConflictHandler[mode].registerControl(control, index, click, key, mask, ignore_mask);
    }
    else if (!set)
    {
        if (!already_recorded)
        {
            // nothing to do
            return;
        }

        // find specific control and reset it
        for (S32 i = 0; i < 3; i++)
        {
            LLKeyData data = mConflictHandler[mode].getControl(control, i);
            if (data.mMouse == click && data.mKey == key && data.mMask == mask)
            {
                mConflictHandler[mode].clearControl(control, i);
            }
        }
    }
}

void LLPanelPreferenceControls::updateAndApply()
{
    S32 mode = LLKeyConflictHandler::MODE_THIRD_PERSON;
    mConflictHandler[mode].saveToSettings(true);
    updateTable();
}

// from LLSetKeybindDialog's interface
bool LLPanelPreferenceControls::onSetKeyBind(EMouseClickType click, KEY key, MASK mask, bool all_modes)
{
    if (!mConflictHandler[mEditingMode].canAssignControl(mEditingControl))
    {
        return true;
    }

    if ( mEditingColumn > 0)
    {
        if (all_modes)
        {
            for (U32 i = 0; i < LLKeyConflictHandler::MODE_COUNT - 1; ++i)
            {
                if (mConflictHandler[i].empty())
                {
                    mConflictHandler[i].loadFromSettings((LLKeyConflictHandler::ESourceMode)i);
                }
                mConflictHandler[i].registerControl(mEditingControl, mEditingColumn - 1, click, key, mask, true);
                // Apply changes to viewer as 'temporary'
                mConflictHandler[i].saveToSettings(true);
            }
        }
        else
        {
            mConflictHandler[mEditingMode].registerControl(mEditingControl, mEditingColumn - 1, click, key, mask, true);
            // Apply changes to viewer as 'temporary'
            mConflictHandler[mEditingMode].saveToSettings(true);
        }
    }

    updateTable();

    if ((mEditingMode == LLKeyConflictHandler::MODE_THIRD_PERSON || all_modes)
        && (mEditingControl == "walk_to"
            || mEditingControl == "teleport_to"
            || click == CLICK_LEFT
            || click == CLICK_DOUBLELEFT))
    {
        // notify comboboxes in move&view about potential change
        LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
        if (instance)
        {
            instance->updateClickActionViews();
        }
    }

    return true;
}

void LLPanelPreferenceControls::onDefaultKeyBind(bool all_modes)
{
    if (!mConflictHandler[mEditingMode].canAssignControl(mEditingControl))
    {
        return;
    }

    if (mEditingColumn > 0)
    {
        if (all_modes)
        {
            for (U32 i = 0; i < LLKeyConflictHandler::MODE_COUNT - 1; ++i)
            {
                if (mConflictHandler[i].empty())
                {
                    mConflictHandler[i].loadFromSettings((LLKeyConflictHandler::ESourceMode)i);
                }
                mConflictHandler[i].resetToDefault(mEditingControl, mEditingColumn - 1);
                // Apply changes to viewer as 'temporary'
                mConflictHandler[i].saveToSettings(true);
            }
        }
        else
        {
            mConflictHandler[mEditingMode].resetToDefault(mEditingControl, mEditingColumn - 1);
            // Apply changes to viewer as 'temporary'
            mConflictHandler[mEditingMode].saveToSettings(true);
        }
    }

    updateTable();

    if (mEditingMode == LLKeyConflictHandler::MODE_THIRD_PERSON || all_modes)
    {
        // notify comboboxes in move&view about potential change
        LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
        if (instance)
        {
            instance->updateClickActionViews();
        }
    }
}

void LLPanelPreferenceControls::onCancelKeyBind()
{
    pControlsTable->deselectAllItems();
}

//------------------------LLPanelPreferenceGameControl--------------------------------

// LLPanelPreferenceGameControl is effectively a singleton, so we track its instance
static LLPanelPreferenceGameControl* gGameControlPanel { nullptr };
static LLScrollListCell* gSelectedCell { nullptr };

LLPanelPreferenceGameControl::LLPanelPreferenceGameControl()
{
    gGameControlPanel = this;
}

LLPanelPreferenceGameControl::~LLPanelPreferenceGameControl()
{
    gGameControlPanel = nullptr;
}

static LLPanelInjector<LLPanelPreferenceGameControl> t_pref_game_control("panel_preference_game_control");

// Collect all UI control values into mSavedValues
void LLPanelPreferenceGameControl::saveSettings()
{
    LLPanelPreference::saveSettings();

    std::vector<LLScrollListItem*> items = mActionTable->getAllData();

    // Find the channel visually associated with the specified action
    LLGameControl::getChannel_t getChannel =
    [&](const std::string& action) -> LLGameControl::InputChannel
    {
        for (LLScrollListItem* item : items)
        {
            if (action == item->getValue() && (item->getNumColumns() >= 2))
            {
                return LLGameControl::getChannelByName(item->getColumn(1)->getValue());
            }
        }
        return LLGameControl::InputChannel();
    };

    // Use string formatting functions provided by class LLGameControl:
    // stringifyAnalogMappings(), stringifyBinaryMappings(), stringifyFlycamMappings()

    if (LLControlVariable* analogMappings = gSavedSettings.getControl("AnalogChannelMappings"))
    {
        analogMappings->set(LLGameControl::stringifyAnalogMappings(getChannel));
        mSavedValues[analogMappings] = analogMappings->getValue();
    }

    if (LLControlVariable* binaryMappings = gSavedSettings.getControl("BinaryChannelMappings"))
    {
        binaryMappings->set(LLGameControl::stringifyBinaryMappings(getChannel));
        mSavedValues[binaryMappings] = binaryMappings->getValue();
    }

    if (LLControlVariable* flycamMappings = gSavedSettings.getControl("FlycamChannelMappings"))
    {
        flycamMappings->set(LLGameControl::stringifyFlycamMappings(getChannel));
        mSavedValues[flycamMappings] = flycamMappings->getValue();
    }

    if (LLControlVariable* knownControllers = gSavedSettings.getControl("KnownGameControllers"))
    {
        LLSD deviceOptions(mDeviceOptions, true);
        knownControllers->set(deviceOptions);
        mSavedValues[knownControllers] = deviceOptions;
    }
}

void LLPanelPreferenceGameControl::onActionSelect()
{
    if (!mActionTable->getEnabled())
        return;

    clearSelectionState();

    if (LLScrollListItem* item = mActionTable->getFirstSelected())
    {
        if (initChannelSelector(item))
            return;
    }

    mActionTable->deselectAllItems();
}

bool LLPanelPreferenceGameControl::initChannelSelector(LLScrollListItem* item)
{
    gSelectedCell = nullptr;
    mAnalogChannelSelector->setVisible(false);
    mBinaryChannelSelector->setVisible(false);

    if (item->getSelectedCell() != 1)
        return false;

    LLScrollListText* cell = dynamic_cast<LLScrollListText*>(item->getColumn(1));
    if (!cell)
        return false;

    std::string action = item->getValue();
    LLGameControl::ActionNameType actionNameType = LLGameControl::getActionNameType(action);
    LLComboBox* channelSelector =
        actionNameType == LLGameControl::ACTION_NAME_ANALOG ? mAnalogChannelSelector :
        actionNameType == LLGameControl::ACTION_NAME_BINARY ? mBinaryChannelSelector :
        actionNameType == LLGameControl::ACTION_NAME_FLYCAM ? mAnalogChannelSelector :
        nullptr;
    if (!channelSelector)
        return false;

    // compute new rect for mChannelSelector
    S32 row = mActionTable->getItemIndex(item);
    LLRect rect(mActionTable->getCellRect(row, 1));
    channelSelector->setRect(rect);

    std::string value;
    LLGameControl::InputChannel channel = LLGameControl::getChannelByName(cell->getValue());
    if (!channel.isNone())
    {
        std::string channelName = channel.getLocalName();
        if (channelSelector->itemExists(channelName))
        {
            value = channelName;
        }
    }
    if (value.empty())
    {
        // Assign the last element in the dropdown list which is "NONE"
        value = channelSelector->getAllData().back()->getValue().asString();
    }

    channelSelector->setValue(value);
    channelSelector->setVisible(true);
    channelSelector->showList();

    gSelectedCell = cell;

    return true;
}

void LLPanelPreferenceGameControl::onCommitInputChannel(LLUICtrl* ctrl)
{
    if (!gSelectedCell)
        return;

    LLComboBox* channelSelector = dynamic_cast<LLComboBox*>(ctrl);
    llassert(channelSelector);
    if (!channelSelector)
        return;

    std::string value = channelSelector->getSelectedItemLabel();
    gSelectedCell->setValue((value == "NONE") ? " " : value);
    mActionTable->deselectAllItems();
    clearSelectionState();
}

bool LLPanelPreferenceGameControl::isWaitingForInputChannel()
{
    return gSelectedCell != nullptr;
}

// static
void LLPanelPreferenceGameControl::applyGameControlInput()
{
    if (!gGameControlPanel || !gSelectedCell)
        return;

    LLGameControl::InputChannel::Type expectedType =
        gGameControlPanel->mAnalogChannelSelector->getVisible() ? LLGameControl::InputChannel::TYPE_AXIS :
        gGameControlPanel->mBinaryChannelSelector->getVisible() ? LLGameControl::InputChannel::TYPE_BUTTON :
        LLGameControl::InputChannel::TYPE_NONE;
    if (expectedType == LLGameControl::InputChannel::TYPE_NONE)
        return;

    LLGameControl::InputChannel channel = LLGameControl::getActiveInputChannel();
    if (channel.mType == expectedType)
    {
        gSelectedCell->setValue(channel.getLocalName());
        gGameControlPanel->mActionTable->deselectAllItems();
        gGameControlPanel->clearSelectionState();
    }
}

bool LLPanelPreferenceGameControl::postBuild()
{
    mCheckGameControlToServer = getChild<LLCheckBoxCtrl>("game_control_to_server");
    mCheckGameControlToAgent = getChild<LLCheckBoxCtrl>("game_control_to_agent");
    mCheckAgentToGameControl = getChild<LLCheckBoxCtrl>("agent_to_game_control");

    mCheckGameControlToAgent->setCommitCallback([this](LLUICtrl*, const LLSD&) { updateTableState(); });
    mCheckAgentToGameControl->setCommitCallback([this](LLUICtrl*, const LLSD&) { updateTableState(); });

    mActionTable = getChild<LLScrollListCtrl>("action_table");
    mActionTable->setCommitCallback(boost::bind(&LLPanelPreferenceGameControl::onActionSelect, this));

    mAnalogChannelSelector = getChild<LLComboBox>("analog_channel_selector");
    mAnalogChannelSelector->setCommitCallback(boost::bind(&LLPanelPreferenceGameControl::onCommitInputChannel, this, _1));

    mBinaryChannelSelector = getChild<LLComboBox>("binary_channel_selector");
    mBinaryChannelSelector->setCommitCallback(boost::bind(&LLPanelPreferenceGameControl::onCommitInputChannel, this, _1));

    populateActionTable();

    return true;
}

// Update all UI control values from real objects
// This function is called before floater is shown
void LLPanelPreferenceGameControl::onOpen(const LLSD& key)
{
    mCheckGameControlToServer->setValue(LLGameControl::getSendToServer());
    mCheckGameControlToAgent->setValue(LLGameControl::getControlAgent());
    mCheckAgentToGameControl->setValue(LLGameControl::getTranslateAgentActions());

    populateCells();

    updateTableState();

    mDeviceOptions = LLGameControl::getDeviceOptions();
    // Add missing device settings even if they are default
    for (const auto& device : LLGameControl::getDevices())
    {
        if (mDeviceOptions.find(device.getGUID()) == mDeviceOptions.end())
        {
            mDeviceOptions[device.getGUID()] = device.saveOptionsToString(true);
        }
    }
}

void LLPanelPreferenceGameControl::populateActionTable()
{
    if (!populateColumns("game_control_table_columns.xml"))
        return;

    populateRows("game_control_table_rows.xml");
    addTableSeparator();
    populateRows("game_control_table_camera_rows.xml");
}

bool LLPanelPreferenceGameControl::populateColumns(const std::string& filename)
{
    LLScrollListCtrl::Contents contents;
    if (!parseXmlFile(contents, filename, "columns"))
        return false;

    for (const LLScrollListColumn::Params& column_params : contents.columns)
    {
        mActionTable->addColumn(column_params);
    }

    // we expect the mActionTable to have at least three columns
    if (mActionTable->getNumColumns() < 3)
    {
        LL_WARNS("Preferences") << "expected at least three columns in '" << filename << "'" << LL_ENDL;
        return false;
    }

    return true;
}

void LLPanelPreferenceGameControl::populateRows(const std::string& filename)
{
    LLScrollListCtrl::Contents contents;
    if (!parseXmlFile(contents, filename, "rows"))
        return;

    // init basic cell params
    LLScrollListCell::Params second_cell_params;
    second_cell_params.font = LLFontGL::getFontSansSerif();
    second_cell_params.font_halign = LLFontGL::LEFT;
    second_cell_params.column = mActionTable->getColumn(1)->mName;
    second_cell_params.value = ""; // Actual value is assigned in populateCells

    for (const LLScrollListItem::Params& row_params : contents.rows)
    {
        std::string name = row_params.value.getValue().asString();
        if (!name.empty() && name != "menu_separator")
        {
            LLScrollListItem::Params new_params(row_params);
            new_params.enabled.setValue(true);
            // item_params should already have one column that was defined
            // in XUI config file, and now we want to add one more
            if (new_params.columns.size() == 1)
            {
                new_params.columns.add(second_cell_params);
            }
            mActionTable->addRow(new_params, EAddPosition::ADD_BOTTOM);
        }
        else
        {
            mActionTable->addRow(row_params, EAddPosition::ADD_BOTTOM);
        }
    }
}

void LLPanelPreferenceGameControl::populateCells()
{
    std::vector<LLScrollListItem*> items = mActionTable->getAllData();
    for (LLScrollListItem* item : items)
    {
        if (item->getNumColumns() >= 2) // Skip separators
        {
            std::string name = item->getValue().asString();
            if (!name.empty() && name != "menu_separator")
            {
                LLGameControl::InputChannel channel = LLGameControl::getChannelByAction(name);
                item->getColumn(1)->setValue(channel.isNone() ? " " : channel.getLocalName());
            }
        }
    }
}

bool LLPanelPreferenceGameControl::parseXmlFile(LLScrollListCtrl::Contents& contents,
    const std::string& filename, const std::string& what)
{
    LLXMLNodePtr xmlNode;
    if (!LLUICtrlFactory::getLayeredXMLNode(filename, xmlNode))
    {
        LL_WARNS("Preferences") << "Failed to populate " << what << " from '" << filename << "'" << LL_ENDL;
        return false;
    }

    LLXUIParser parser;
    parser.readXUI(xmlNode, contents, filename);
    if (!contents.validateBlock())
    {
        LL_WARNS("Preferences") << "Failed to parse " << what << " from '" << filename << "'" << LL_ENDL;
        return false;
    }

    return true;
}

void LLPanelPreferenceGameControl::clearSelectionState()
{
    gSelectedCell = nullptr;
    mAnalogChannelSelector->setVisible(false);
    mBinaryChannelSelector->setVisible(false);
}

void LLPanelPreferenceGameControl::addTableSeparator()
{
    LLScrollListItem::Params separator_params;
    separator_params.enabled(false);
    LLScrollListCell::Params column_params;
    column_params.type = "icon";
    column_params.value = "menu_separator";
    column_params.column = "action";
    column_params.color = LLColor4(0.f, 0.f, 0.f, 0.7f);
    column_params.font_halign = LLFontGL::HCENTER;
    separator_params.columns.add(column_params);
    mActionTable->addRow(separator_params, EAddPosition::ADD_BOTTOM);
}

void LLPanelPreferenceGameControl::updateTableState()
{
    // Enable the table if at least one of the GameControl<-->Agent options is enabled
    bool enable_table = mCheckGameControlToAgent->get() || mCheckAgentToGameControl->get();

    mActionTable->deselectAllItems();
    mActionTable->setEnabled(enable_table);
    mAnalogChannelSelector->setVisible(false);
    mBinaryChannelSelector->setVisible(false);
}

//------------------------LLFloaterPreferenceProxy--------------------------------

LLFloaterPreferenceProxy::LLFloaterPreferenceProxy(const LLSD& key)
    : LLFloater(key),
      mSocksSettingsDirty(false)
{
    mCommitCallbackRegistrar.add("Proxy.OK",                boost::bind(&LLFloaterPreferenceProxy::onBtnOk, this));
    mCommitCallbackRegistrar.add("Proxy.Cancel",            boost::bind(&LLFloaterPreferenceProxy::onBtnCancel, this));
    mCommitCallbackRegistrar.add("Proxy.Change",            boost::bind(&LLFloaterPreferenceProxy::onChangeSocksSettings, this));
}

LLFloaterPreferenceProxy::~LLFloaterPreferenceProxy()
{
}

bool LLFloaterPreferenceProxy::postBuild()
{
    LLRadioGroup* socksAuth = getChild<LLRadioGroup>("socks5_auth_type");
    if (!socksAuth)
    {
        return false;
    }
    if (socksAuth->getSelectedValue().asString() == "None")
    {
        getChild<LLLineEditor>("socks5_username")->setEnabled(false);
        getChild<LLLineEditor>("socks5_password")->setEnabled(false);
    }
    else
    {
        // Populate the SOCKS 5 credential fields with protected values.
        LLPointer<LLCredential> socks_cred = gSecAPIHandler->loadCredential("SOCKS5");
        getChild<LLLineEditor>("socks5_username")->setValue(socks_cred->getIdentifier()["username"].asString());
        getChild<LLLineEditor>("socks5_password")->setValue(socks_cred->getAuthenticator()["creds"].asString());
    }

    return true;
}

void LLFloaterPreferenceProxy::onOpen(const LLSD& key)
{
    saveSettings();
}

void LLFloaterPreferenceProxy::onClose(bool app_quitting)
{
    if (app_quitting)
    {
        cancel();
    }

    if (mSocksSettingsDirty)
    {

        // If the user plays with the Socks proxy settings after login, it's only fair we let them know
        // it will not be updated until next restart.
        if (LLStartUp::getStartupState()>STATE_LOGIN_WAIT)
        {
            LLNotifications::instance().add("ChangeProxySettings", LLSD(), LLSD());
            mSocksSettingsDirty = false; // we have notified the user now be quiet again
        }
    }
}

void LLFloaterPreferenceProxy::saveSettings()
{
    // Save the value of all controls in the hierarchy
    mSavedValues.clear();
    std::list<LLView*> view_stack;
    view_stack.push_back(this);
    while (!view_stack.empty())
    {
        // Process view on top of the stack
        LLView* curview = view_stack.front();
        view_stack.pop_front();

        LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
        if (ctrl)
        {
            LLControlVariable* control = ctrl->getControlVariable();
            if (control)
            {
                mSavedValues[control] = control->getValue();
            }
        }

        // Push children onto the end of the work stack
        for (child_list_t::const_iterator iter = curview->getChildList()->begin();
                iter != curview->getChildList()->end(); ++iter)
        {
            view_stack.push_back(*iter);
        }
    }
}

void LLFloaterPreferenceProxy::onBtnOk()
{
    // commit any outstanding text entry
    if (hasFocus())
    {
        LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
        if (cur_focus && cur_focus->acceptsTextInput())
        {
            cur_focus->onCommit();
        }
    }

    // Save SOCKS proxy credentials securely if password auth is enabled
    LLRadioGroup* socksAuth = getChild<LLRadioGroup>("socks5_auth_type");
    if (socksAuth->getSelectedValue().asString() == "UserPass")
    {
        LLSD socks_id = LLSD::emptyMap();
        socks_id["type"] = "SOCKS5";
        socks_id["username"] = getChild<LLLineEditor>("socks5_username")->getValue().asString();

        LLSD socks_authenticator = LLSD::emptyMap();
        socks_authenticator["type"] = "SOCKS5";
        socks_authenticator["creds"] = getChild<LLLineEditor>("socks5_password")->getValue().asString();

        // Using "SOCKS5" as the "grid" argument since the same proxy
        // settings will be used for all grids and because there is no
        // way to specify the type of credential.
        LLPointer<LLCredential> socks_cred = gSecAPIHandler->createCredential("SOCKS5", socks_id, socks_authenticator);
        gSecAPIHandler->saveCredential(socks_cred, true);
    }
    else
    {
        // Clear SOCKS5 credentials since they are no longer needed.
        LLPointer<LLCredential> socks_cred = new LLCredential("SOCKS5");
        gSecAPIHandler->deleteCredential(socks_cred);
    }

    closeFloater(false);
}

void LLFloaterPreferenceProxy::onBtnCancel()
{
    if (hasFocus())
    {
        LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
        if (cur_focus && cur_focus->acceptsTextInput())
        {
            cur_focus->onCommit();
        }
        refresh();
    }

    cancel();
}

void LLFloaterPreferenceProxy::onClickCloseBtn(bool app_quitting)
{
    cancel();
}

void LLFloaterPreferenceProxy::cancel()
{
    for (const auto& iter : mSavedValues)
    {
        iter.first->set(iter.second);
    }
    mSocksSettingsDirty = false;
    closeFloater();
}

void LLFloaterPreferenceProxy::onChangeSocksSettings()
{
    mSocksSettingsDirty = true;

    LLRadioGroup* socksAuth = getChild<LLRadioGroup>("socks5_auth_type");
    if (socksAuth->getSelectedValue().asString() == "None")
    {
        getChild<LLLineEditor>("socks5_username")->setEnabled(false);
        getChild<LLLineEditor>("socks5_password")->setEnabled(false);
    }
    else
    {
        getChild<LLLineEditor>("socks5_username")->setEnabled(true);
        getChild<LLLineEditor>("socks5_password")->setEnabled(true);
    }

    // Check for invalid states for the other HTTP proxy radio
    LLRadioGroup* otherHttpProxy = getChild<LLRadioGroup>("other_http_proxy_type");
    if ((otherHttpProxy->getSelectedValue().asString() == "Socks" &&
            getChild<LLCheckBoxCtrl>("socks_proxy_enabled")->get() == false )||(
                    otherHttpProxy->getSelectedValue().asString() == "Web" &&
                    getChild<LLCheckBoxCtrl>("web_proxy_enabled")->get() == false ) )
    {
        otherHttpProxy->selectFirstItem();
    }
}

void LLFloaterPreference::onUpdateFilterTerm(bool force)
{
    LLWString seachValue = utf8str_to_wstring(mFilterEdit->getValue());
    LLWStringUtil::toLower(seachValue);

    if (!mSearchData || (mSearchData->mLastFilter == seachValue && !force))
        return;

    if (mSearchDataDirty)
    {
        // Data exists, but is obsolete, regenerate
        collectSearchableItems();
    }

    mSearchData->mLastFilter = seachValue;

    if (!mSearchData->mRootTab)
        return;

    mSearchData->mRootTab->hightlightAndHide( seachValue );
    filterIgnorableNotifications();

    if (LLTabContainer* pRoot = getChild<LLTabContainer>("pref core"))
        pRoot->selectFirstTab();
}

void LLFloaterPreference::filterIgnorableNotifications()
{
    bool visible = getChildRef<LLScrollListCtrl>("enabled_popups").highlightMatchingItems(mFilterEdit->getValue());
    visible |= getChildRef<LLScrollListCtrl>("disabled_popups").highlightMatchingItems(mFilterEdit->getValue());

    if (visible)
    {
        getChildRef<LLTabContainer>("pref core").setTabVisibility( getChild<LLPanel>("msgs"), true );
    }
}

void collectChildren( LLView const *aView, ll::prefs::PanelDataPtr aParentPanel, ll::prefs::TabContainerDataPtr aParentTabContainer )
{
    if (!aView)
        return;

    llassert_always(aParentPanel || aParentTabContainer);

    for (LLView* pView : *aView->getChildList())
    {
        if (!pView)
            continue;

        ll::prefs::PanelDataPtr pCurPanelData = aParentPanel;
        ll::prefs::TabContainerDataPtr pCurTabContainer = aParentTabContainer;

        LLPanel const *pPanel = dynamic_cast<LLPanel const*>(pView);
        LLTabContainer const *pTabContainer = dynamic_cast<LLTabContainer const*>(pView);
        ll::ui::SearchableControl const *pSCtrl = dynamic_cast<ll::ui::SearchableControl const*>( pView );

        if (pTabContainer)
        {
            pCurPanelData.reset();

            pCurTabContainer = ll::prefs::TabContainerDataPtr(new ll::prefs::TabContainerData);
            pCurTabContainer->mTabContainer = const_cast< LLTabContainer *>(pTabContainer);
            pCurTabContainer->mLabel = pTabContainer->getLabel();
            pCurTabContainer->mPanel = 0;

            if (aParentPanel)
                aParentPanel->mChildPanel.push_back(pCurTabContainer);
            if (aParentTabContainer)
                aParentTabContainer->mChildPanel.push_back(pCurTabContainer);
        }
        else if (pPanel)
        {
            pCurTabContainer.reset();

            pCurPanelData = ll::prefs::PanelDataPtr(new ll::prefs::PanelData);
            pCurPanelData->mPanel = pPanel;
            pCurPanelData->mLabel = pPanel->getLabel();

            llassert_always( aParentPanel || aParentTabContainer );

            if (aParentTabContainer)
                aParentTabContainer->mChildPanel.push_back(pCurPanelData);
            else if (aParentPanel)
                aParentPanel->mChildPanel.push_back(pCurPanelData);
        }
        else if (pSCtrl && pSCtrl->getSearchText().size())
        {
            ll::prefs::SearchableItemPtr item = ll::prefs::SearchableItemPtr(new ll::prefs::SearchableItem());
            item->mView = pView;
            item->mCtrl = pSCtrl;

            item->mLabel = utf8str_to_wstring(pSCtrl->getSearchText());
            LLWStringUtil::toLower(item->mLabel);

            llassert_always(aParentPanel || aParentTabContainer);

            if (aParentPanel)
                aParentPanel->mChildren.push_back(item);
            if (aParentTabContainer)
                aParentTabContainer->mChildren.push_back(item);
        }
        collectChildren(pView, pCurPanelData, pCurTabContainer);
    }
}

void LLFloaterPreference::collectSearchableItems()
{
    mSearchData.reset( nullptr );
    LLTabContainer *pRoot = getChild< LLTabContainer >( "pref core" );
    if( mFilterEdit && pRoot )
    {
        mSearchData.reset(new ll::prefs::SearchData() );

        ll::prefs::TabContainerDataPtr pRootTabcontainer = ll::prefs::TabContainerDataPtr( new ll::prefs::TabContainerData );
        pRootTabcontainer->mTabContainer = pRoot;
        pRootTabcontainer->mLabel = pRoot->getLabel();
        mSearchData->mRootTab = pRootTabcontainer;

        collectChildren( this, ll::prefs::PanelDataPtr(), pRootTabcontainer );
    }
    mSearchDataDirty = false;
}

void LLFloaterPreference::saveIgnoredNotifications()
{
    for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
            iter != LLNotifications::instance().templatesEnd();
            ++iter)
    {
        LLNotificationTemplatePtr templatep = iter->second;
        LLNotificationFormPtr formp = templatep->mForm;

        LLNotificationForm::EIgnoreType ignore = formp->getIgnoreType();
        if (ignore <= LLNotificationForm::IGNORE_NO)
            continue;

        mIgnorableNotifs[templatep->mName] = !formp->getIgnored();
    }
}

void LLFloaterPreference::restoreIgnoredNotifications()
{
    for (std::map<std::string, bool>::iterator it = mIgnorableNotifs.begin(); it != mIgnorableNotifs.end(); ++it)
    {
        LLUI::getInstance()->mSettingGroups["ignores"]->setBOOL(it->first, it->second);
    }
}
