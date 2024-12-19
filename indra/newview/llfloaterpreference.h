/**
 * @file llfloaterpreference.h
 * @brief LLPreferenceCore class definition
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

#ifndef LL_LLFLOATERPREFERENCE_H
#define LL_LLFLOATERPREFERENCE_H

#include "llfloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llconversationlog.h"
#include "llsearcheditor.h"
#include "llsetkeybinddialog.h"
#include "llkeyconflict.h"

class LLConversationLogObserver;
class LLPanelPreference;
class LLPanelLCD;
class LLPanelDebug;
class LLMessageSystem;
class LLComboBox;
class LLScrollListCtrl;
class LLScrollListCell;
class LLSliderCtrl;
class LLSD;
class LLTextBox;

namespace ll
{
    namespace prefs
    {
        struct SearchData;
    }
}

typedef std::map<std::string, std::string> notifications_map;

typedef enum
    {
        GS_LOW_GRAPHICS,
        GS_MID_GRAPHICS,
        GS_HIGH_GRAPHICS,
        GS_ULTRA_GRAPHICS

    } EGraphicsSettings;

// Floater to control preferences (display, audio, bandwidth, general.
class LLFloaterPreference : public LLFloater, public LLAvatarPropertiesObserver, public LLConversationLogObserver
{
public:
    LLFloaterPreference(const LLSD& key);
    ~LLFloaterPreference();

    void apply();
    void cancel(const std::vector<std::string> settings_to_skip = {});
    /*virtual*/ void draw();
    /*virtual*/ bool postBuild();
    /*virtual*/ void onOpen(const LLSD& key);
    /*virtual*/ void onClose(bool app_quitting);
    /*virtual*/ void changed();
    /*virtual*/ void changed(const LLUUID& session_id, U32 mask) {};

    // static data update, called from message handler
    static void updateUserInfo(const std::string& visibility);

    // refresh all the graphics preferences menus
    static void refreshEnabledGraphics();

    // translate user's do not disturb response message according to current locale if message is default, otherwise do nothing
    static void initDoNotDisturbResponse();

    // update Show Favorites checkbox
    static void updateShowFavoritesCheckbox(bool val);

    void processProperties( void* pData, EAvatarProcessorType type );
    void saveAvatarProperties( void );
    static void saveAvatarPropertiesCoro(const std::string url, bool allow_publish);
    void selectPrivacyPanel();
    void selectChatPanel();
    void getControlNames(std::vector<std::string>& names);
    // updates click/double-click action controls depending on values from settings.xml
    void updateClickActionViews();
    void updateSearchableItems();

    void        onBtnOK(const LLSD& userdata);
    void        onBtnCancel(const LLSD& userdata);

protected:

    void        onClickClearCache();            // Clear viewer texture cache, file cache on next startup
    void        onClickBrowserClearCache();     // Clear web history and caches as well as viewer caches above
    void        onLanguageChange();
    void        onNotificationsChange(const std::string& OptionName);
    void        onNameTagOpacityChange(const LLSD& newvalue);

    // set value of "DoNotDisturbResponseChanged" in account settings depending on whether do not disturb response
    // string differs from default after user changes.
    void onDoNotDisturbResponseChanged();
    // if the custom settings box is clicked
    void onChangeCustom();
    void updateMeterText(LLUICtrl* ctrl);
    // callback for defaults
    void setHardwareDefaults();
    void setRecommended();
    // callback for when client modifies a render option
    void onRenderOptionEnable();
    // callback for when client turns on impostors
    void onAvatarImpostorsEnable();

    // callback for commit in the "Single click on land" and "Double click on land" comboboxes.
    void onClickActionChange();
    // updates click/double-click action keybindngs depending on view values
    void updateClickActionControls();

    void onAtmosShaderChange();

public:
    // This function squirrels away the current values of the controls so that
    // cancel() can restore them.
    void saveSettings();

    void saveIgnoredNotifications();
    void restoreIgnoredNotifications();

    void setCacheLocation(const LLStringExplicit& location);

    void onClickSetCache();
    void changeCachePath(const std::vector<std::string>& filenames, std::string proposed_name);
    void onClickResetCache();
    void onClickSkin(LLUICtrl* ctrl,const LLSD& userdata);
    void onSelectSkin();
    void onClickSetSounds();
    void onClickEnablePopup();
    void onClickDisablePopup();
    void resetAllIgnored();
    void setAllIgnored();
    void onClickLogPath();
    void changeLogPath(const std::vector<std::string>& filenames, std::string proposed_name);
    bool moveTranscriptsAndLog();
    void setPersonalInfo(const std::string& visibility);
    void refreshEnabledState();
    void onCommitWindowedMode();
    void refresh(); // Refresh enable/disable
    // if the quality radio buttons are changed
    void onChangeQuality(const LLSD& data);

    void refreshUI();

    void onChangeMaturity();
    void onChangeComplexityMode(const LLSD& newvalue);
    void onChangeModelFolder();
    void onChangePBRFolder();
    void onChangeTextureFolder();
    void onChangeSoundFolder();
    void onChangeAnimationFolder();
    void onClickBlockList();
    void onClickProxySettings();
    void onClickTranslationSettings();
    void onClickPermsDefault();
    void onClickRememberedUsernames();
    void onClickAutoReplace();
    void onClickSpellChecker();
    void onClickRenderExceptions();
    void onClickAutoAdjustments();
    void onClickAdvanced();
    void applyUIColor(LLUICtrl* ctrl, const LLSD& param);
    void getUIColor(LLUICtrl* ctrl, const LLSD& param);
    void onLogChatHistorySaved();
    void buildPopupLists();
    static void refreshSkin(void* data);
    void selectPanel(const LLSD& name);
    void saveGraphicsPreset(std::string& preset);

    void setRecommendedSettings();
    void resetAutotuneSettings();

private:

    void onDeleteTranscripts();
    void onDeleteTranscriptsResponse(const LLSD& notification, const LLSD& response);
    void updateDeleteTranscriptsButton();
    void updateMaxComplexity();
    void updateComplexityText();
    static bool loadFromFilename(const std::string& filename, std::map<std::string, std::string> &label_map);

    static std::string sSkin;
    notifications_map mNotificationOptions;
    bool mGotPersonalInfo;
    bool mLanguageChanged;
    bool mAvatarDataInitialized;
    std::string mPriorInstantMessageLogPath;

    bool mOriginalHideOnlineStatus;
    std::string mDirectoryVisibility;

    bool mAllowPublish; // Allow showing agent in search
    std::string mSavedCameraPreset;
    std::string mSavedGraphicsPreset;
    LOG_CLASS(LLFloaterPreference);

    LLSearchEditor* mFilterEdit = nullptr;
    LLScrollListCtrl* mEnabledPopups = nullptr;
    LLScrollListCtrl* mDisabledPopups = nullptr;
    LLButton*       mDeleteTranscriptsBtn = nullptr;
    LLButton*       mEnablePopupBtn = nullptr;
    LLButton*       mDisablePopupBtn = nullptr;
    std::unique_ptr< ll::prefs::SearchData > mSearchData;
    bool mSearchDataDirty;

    boost::signals2::connection mComplexityChangedSignal;

    void onUpdateFilterTerm( bool force = false );
    void collectSearchableItems();
    void filterIgnorableNotifications();

    std::map<std::string, bool> mIgnorableNotifs;
};

class LLPanelPreference : public LLPanel
{
public:
    LLPanelPreference();
    /*virtual*/ bool postBuild();

    virtual ~LLPanelPreference();

    virtual void apply();
    virtual void cancel(const std::vector<std::string> settings_to_skip = {});
    void setControlFalse(const LLSD& user_data);
    virtual void setHardwareDefaults();

    // Disables "Allow Media to auto play" check box only when both
    // "Streaming Music" and "Media" are unchecked. Otherwise enables it.
    void updateMediaAutoPlayCheckbox(LLUICtrl* ctrl);

    // This function squirrels away the current values of the controls so that
    // cancel() can restore them.
    virtual void saveSettings();

    void deletePreset(const LLSD& user_data);
    void savePreset(const LLSD& user_data);
    void loadPreset(const LLSD& user_data);

    class Updater;

protected:
    typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
    control_values_map_t mSavedValues;

private:
    //for "Only friends and groups can call or IM me"
    static void showFriendsOnlyWarning(LLUICtrl*, const LLSD&);
    //for  "Allow Multiple Viewers"
    static void showMultipleViewersWarning(LLUICtrl*, const LLSD&);
    //for "Show my Favorite Landmarks at Login"
    static void handleFavoritesOnLoginChanged(LLUICtrl* checkbox, const LLSD& value);

    static void toggleMuteWhenMinimized();
    typedef std::map<std::string, LLColor4> string_color_map_t;
    string_color_map_t mSavedColors;

    Updater* mBandWidthUpdater;
    LOG_CLASS(LLPanelPreference);
};

class LLPanelPreferenceGraphics : public LLPanelPreference
{
public:
    bool postBuild();
    void draw();
    void cancel(const std::vector<std::string> settings_to_skip = {});
    void saveSettings();
    void resetDirtyChilds();
    void setHardwareDefaults();
    void setPresetText();

protected:
    bool hasDirtyChilds();

private:
    void onPresetsListChange();
    LOG_CLASS(LLPanelPreferenceGraphics);
};

class LLPanelPreferenceControls : public LLPanelPreference, public LLKeyBindResponderInterface
{
    LOG_CLASS(LLPanelPreferenceControls);
public:
    LLPanelPreferenceControls();
    virtual ~LLPanelPreferenceControls();

    bool postBuild();

    void apply();
    void cancel(const std::vector<std::string> settings_to_skip = {});
    void saveSettings();
    void resetDirtyChilds();

    void onListCommit();
    void onModeCommit();
    void onRestoreDefaultsBtn();
    void onRestoreDefaultsResponse(const LLSD& notification, const LLSD& response);

    // Bypass to let Move & view read values without need to create own key binding handler
    // Todo: consider a better way to share access to keybindings
    bool canKeyBindHandle(const std::string &control, EMouseClickType click, KEY key, MASK mask);
    // Bypasses to let Move & view modify values without need to create own key binding handler
    void setKeyBind(const std::string &control, EMouseClickType click, KEY key, MASK mask, bool set /*set or reset*/ );
    void updateAndApply();

    // from interface
    /*virtual*/ bool onSetKeyBind(EMouseClickType click, KEY key, MASK mask, bool all_modes);
    /*virtual*/ void onDefaultKeyBind(bool all_modes);
    /*virtual*/ void onCancelKeyBind();

private:
    // reloads settings, discards current changes, updates table
    void regenerateControls();

    // These fuctions do not clean previous content
    bool addControlTableColumns(const std::string &filename);
    bool addControlTableRows(const std::string &filename);
    void addControlTableSeparator();

    // Cleans content and then adds content from xml files according to current mEditingMode
    void populateControlTable();

    // Updates keybindings from storage to table
    void updateTable();

    LLScrollListCtrl* pControlsTable;
    LLComboBox *pKeyModeBox;
    LLKeyConflictHandler mConflictHandler[LLKeyConflictHandler::MODE_COUNT];
    std::string mEditingControl;
    S32 mEditingColumn;
    S32 mEditingMode;
};

class LLAvatarComplexityControls
{
  public:
    static void updateMax(LLSliderCtrl* slider, LLTextBox* value_label, bool short_val = false);
    static void setText(U32 value, LLTextBox* text_box, bool short_val = false);
    static void updateMaxRenderTime(LLSliderCtrl* slider, LLTextBox* value_label, bool short_val = false);
    static void setRenderTimeText(F32 value, LLTextBox* text_box, bool short_val = false);
    static void setIndirectControls();
    static void setIndirectMaxNonImpostors();
    static void setIndirectMaxArc();
    LOG_CLASS(LLAvatarComplexityControls);
};

class LLFloaterPreferenceProxy : public LLFloater
{
public:
    LLFloaterPreferenceProxy(const LLSD& key);
    ~LLFloaterPreferenceProxy();

    /// show off our menu
    static void show();
    void cancel();

protected:
    bool postBuild();
    void onOpen(const LLSD& key);
    void onClose(bool app_quitting);
    void saveSettings();
    void onBtnOk();
    void onBtnCancel();
    void onClickCloseBtn(bool app_quitting = false);

    void onChangeSocksSettings();

private:

    bool mSocksSettingsDirty;
    typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
    control_values_map_t mSavedValues;
    LOG_CLASS(LLFloaterPreferenceProxy);
};


#endif  // LL_LLPREFERENCEFLOATER_H
