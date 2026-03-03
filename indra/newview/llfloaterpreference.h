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
#include "llgamecontrol.h"
#include "llkeyconflict.h"
#include "llscrolllistcell.h"
#include "llscrolllistctrl.h"
#include "llsearcheditor.h"
#include "llsetkeybinddialog.h"
#include "llspinctrl.h"

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
class LLPanelPreferenceGameControl;

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
    virtual void draw() override;
    virtual bool postBuild() override;
    virtual void onOpen(const LLSD& key) override;
    virtual void onClose(bool app_quitting) override;
    virtual void changed() override;
    virtual void changed(const LLUUID& session_id, U32 mask) override {};

    static void refreshInstance();

    // static data update, called from message handler
    static void updateUserInfo(const std::string& visibility);

    // refresh all the graphics preferences menus
    static void refreshEnabledGraphics();

    // translate user's do not disturb response message according to current locale if message is default, otherwise do nothing
    static void initDoNotDisturbResponse();

    // update Show Favorites checkbox
    static void updateShowFavoritesCheckbox(bool val);

    void processProperties( void* pData, EAvatarProcessorType type ) override;
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
    void        onTimeFormatChange();
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
    void refresh() override; // Refresh enable/disable
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
    void onClickScriptingPerfs();
    void applyUIColor(LLUICtrl* ctrl, const LLSD& param);
    void getUIColor(LLUICtrl* ctrl, const LLSD& param);
    void onLogChatHistorySaved();
    void buildPopupLists();
    static void refreshSkin(void* data);
    void selectPanel(const LLSD& name);
    void setPanelVisibility(const LLSD& name, bool visible);
    void saveGraphicsPreset(std::string& preset);

    void setRecommendedSettings();
    void resetAutotuneSettings();

private:

    void onDeleteTranscripts();
    void onDeleteTranscriptsResponse(const LLSD& notification, const LLSD& response);
    void updateDeleteTranscriptsButton();
    void updateMaxNonImpostors();
    void updateIndirectMaxNonImpostors(const LLSD& newvalue);
    void setMaxNonImpostorsText(U32 value, LLTextBox* text_box);
    void updateMaxComplexity();
    void updateComplexityText();
    static bool loadFromFilename(const std::string& filename, std::map<std::string, std::string> &label_map);

    static std::string sSkin;
    notifications_map mNotificationOptions;
    bool mGotPersonalInfo;
    bool mLanguageChanged;
    bool mAvatarDataInitialized;
    U32 mLastQualityLevel = 0;
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
    LLComboBox*     mTimeFormatCombobox = nullptr;
    LLComboBox*     mLanguageCombobox = nullptr;
    std::unique_ptr< ll::prefs::SearchData > mSearchData;
    bool mSearchDataDirty;

    boost::signals2::connection mImpostorsChangedSignal;
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
    virtual bool postBuild() override;

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
    bool postBuild() override;
    void draw() override;
    void cancel(const std::vector<std::string> settings_to_skip = {}) override;
    void saveSettings() override;
    void resetDirtyChilds();
    void setHardwareDefaults() override;
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

    bool postBuild() override;

    void refresh() override;

    void apply() override;
    void cancel(const std::vector<std::string> settings_to_skip = {}) override;
    void saveSettings() override;
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
    bool onSetKeyBind(EMouseClickType click, KEY key, MASK mask, bool all_modes) override;
    void onDefaultKeyBind(bool all_modes) override;
    void onCancelKeyBind() override;

    // Cleans content and then adds content from xml files according to current mEditingMode
    void populateControlTable();

private:
    // reloads settings, discards current changes, updates table
    void regenerateControls();

    // These fuctions do not clean previous content
    bool addControlTableColumns(const std::string &filename);
    bool addControlTableRows(const std::string &filename);
    void addControlTableSeparator();

    // Updates keybindings from storage to table
    void updateTable();

    LLScrollListCtrl* pControlsTable;
    LLComboBox *pKeyModeBox;
    LLKeyConflictHandler mConflictHandler[LLKeyConflictHandler::MODE_COUNT];
    std::string mEditingControl;
    S32 mEditingColumn;
    S32 mEditingMode;
};

// Preference panel for configuring game controller (gamepad) input.
//
// Provides two main tabs:
//   1. Channel Mappings - Maps viewer actions to controller inputs (axes/buttons)
//   2. Device Settings  - Per-device configuration (axis options, remapping)
//
// Supports live input capture: pressing a controller input while editing
// automatically assigns that input to the selected action.
//
class LLPanelPreferenceGameControl : public LLPanelPreference
{
    LOG_CLASS(LLPanelPreferenceGameControl);
public:

    enum InputType
    {
        TYPE_AXIS,
        TYPE_BUTTON,
        TYPE_NONE
    };

    // Called when a device is connected/disconnected
    static void updateDeviceList();

    LLPanelPreferenceGameControl();
    ~LLPanelPreferenceGameControl();

    void onOpen(const LLSD& key) override;

    void apply() override;
    void cancel(const std::vector<std::string> settings_to_skip = {}) override;
    void saveSettings() override;

    void updateDeviceListInternal(); // Refresh device list and settings

    // UI event handlers
    void onGridSelect(LLUICtrl* ctrl);         // Handle table row selection
    void onCommitInputChannel(LLUICtrl* ctrl); // Handle channel combobox selection
    void onAxisOptionsSelect();                // Handle axis options table selection
    void onCommitNumericValue();               // Handle deadzone/offset value changes

    // Live input capture support
    static bool isWaitingForInputChannel();    // True if a cell is waiting for input
    static void applyGameControlInput();       // Assign detected input to selected cell

protected:
    bool postBuild() override;

    // Action table population (Tab 1: Channel Mappings)
    void populateActionTableRows(const std::string& filename);  // Load action rows from XML
    void populateActionTableCells();                            // Fill channel column with current mappings
    static bool parseXmlFile(LLScrollListCtrl::Contents& contents,
        const std::string& filename, const std::string& what);

    // Device settings population (Tab 2: Device Settings)
    void populateDeviceTitle();         // Update device selector UI
    void populateDeviceSettings(const std::string& guid);  // Load settings for device
    void populateOptionsTableRows();    // Create axis options rows
    void populateOptionsTableCells();   // Fill axis options with current values
    void populateMappingTableRows(LLScrollListCtrl* target,
        const LLComboBox* source, size_t row_count);  // Create mapping table rows
    void populateMappingTableCells(LLScrollListCtrl* target,
        const std::vector<U8>& mappings, const LLComboBox* source);  // Fill mapping cells
    LLGameControl::Options& getSelectedDeviceOptions();  // Get options for selected device

    // Utility methods
    static std::string getChannelLabel(const std::string& channelName,
        const std::vector<LLScrollListItem*>& items);  // Look up display label
    static void setNumericLabel(LLScrollListCell* cell, S32 value);  // Format numeric cell
    static void fitInRect(LLUICtrl* ctrl, LLScrollListCtrl* grid, S32 row_index, S32 col_index);  // Position editor over cell

private:
    // Inline editing support
    bool initCombobox(LLScrollListItem* item, LLScrollListCtrl* grid);  // Show combobox over cell
    void clearSelectionState();  // Hide editors and clear selection tracking

    // UI helpers
    void addActionTableSeparator();   // Add visual separator to action table
    void updateEnable();              // Enable/disable UI based on global setting
    void updateActionTableState();    // Update action table enabled state

    // Reset to defaults handlers
    void onResetToDefaults();         // Route to appropriate reset method
    void resetChannelMappingsToDefaults();  // Reset action->channel mappings
    void resetAxisOptionsToDefaults();      // Reset axis invert/deadzone/offset
    void resetAxisMappingsToDefaults();     // Reset axis remapping to identity
    void resetButtonMappingsToDefaults();   // Reset button remapping to identity

    void rememberOriginalSettings();  // Capture settings for cancel restoration

    // Main checkboxes controlling how game input is used
    LLCheckBoxCtrl* mCheckGameControlToServer { nullptr };  // Send game_control data to server
    LLCheckBoxCtrl* mCheckGameControlToAgent { nullptr };   // Use game_control data to move avatar
    LLCheckBoxCtrl* mCheckAgentToGameControl { nullptr };   // Translate avatar actions to game_control

    // Tab 1: Channel Mappings - maps actions to controller inputs
    LLPanel* mTabChannelMappings { nullptr };
    LLScrollListCtrl* mActionTable { nullptr };  // Action name | Channel assignment

    // Tab 2: Device Settings - per-device configuration
    LLPanel* mTabDeviceSettings { nullptr };
    LLTextBox* mNoDeviceMessage { nullptr };     // Shown when no devices connected
    LLTextBox* mDevicePrompt { nullptr };        // "Select device:" label
    LLTextBox* mSingleDevice { nullptr };        // Shows single device name
    LLComboBox* mDeviceList { nullptr };         // Dropdown for multiple devices
    LLCheckBoxCtrl* mCheckShowAllDevices { nullptr };  // Include disconnected devices
    LLPanel* mPanelDeviceSettings { nullptr };

    // Device settings sub-tabs
    LLPanel* mTabAxisOptions { nullptr };
    LLScrollListCtrl* mAxisOptions { nullptr };    // Axis | Invert | Deadzone | Offset
    LLPanel* mTabAxisMappings { nullptr };
    LLScrollListCtrl* mAxisMappings { nullptr };   // Physical axis | Mapped to
    LLPanel* mTabButtonMappings { nullptr };
    LLScrollListCtrl* mButtonMappings { nullptr }; // Physical button | Mapped to

    LLButton* mResetToDefaults { nullptr };

    // Inline editors - positioned over table cells when editing
    LLSpinCtrl* mNumericValueEditor { nullptr };    // For deadzone/offset values
    LLComboBox* mAnalogChannelSelector { nullptr }; // For axis channel selection
    LLComboBox* mBinaryChannelSelector { nullptr }; // For button channel selection
    LLComboBox* mAxisSelector { nullptr };          // For axis remapping

    // Per-device options storage
    struct DeviceOptions
    {
        std::string name;      // Device display name
        std::string settings;  // Serialized settings string
        LLGameControl::Options options;  // Parsed options (axis settings, mappings)
    };
    std::map<std::string, DeviceOptions> mDeviceOptions;  // Keyed by device GUID
    std::string mSelectedDeviceGUID;  // Currently selected device

    LLSD mOrigSettings;  // Captured settings for cancel restoration
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
    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void onClose(bool app_quitting) override;
    void saveSettings();
    void onBtnOk();
    void onBtnCancel();
    void onClickCloseBtn(bool app_quitting = false) override;

    void onChangeSocksSettings();

private:

    bool mSocksSettingsDirty;
    typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
    control_values_map_t mSavedValues;
    LOG_CLASS(LLFloaterPreferenceProxy);
};


#endif  // LL_LLPREFERENCEFLOATER_H
