/**
 * @file llpanelpreferencegamecontrol.h
 * @brief LLPanelPreferenceGameControl class definition
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

#pragma once

#include "llfloaterpreference.h"
#include "llgamecontrol.h"
#include "llscrolllistcell.h"
#include "llscrolllistctrl.h"

class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLPanel;
class LLSpinCtrl;
class LLTextBox;

//------------------------LLPanelPreferenceGameControl--------------------------------
//
// Preference panel for configuring game controller (gamepad) input.
//
// This panel provides two main tabs:
//   1. Channel Mappings - Maps avatar actions (move forward, jump, etc.) to controller inputs
//   2. Device Settings  - Per-device configuration including:
//      - Axis Options:   Invert, deadzone, and offset for each axis
//      - Axis Mappings:  Remap physical axes to different logical axes
//      - Button Mappings: Remap physical buttons to different logical buttons
//
// Settings are stored in:
//   - AnalogChannelMappings, BinaryChannelMappings, FlycamChannelMappings (action mappings)
//   - KnownGameControllers (per-device options keyed by device GUID)
//
// The panel supports live input capture: when a combobox is open, pressing a controller
// button or moving an axis will automatically select that input channel.
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

    // Snapshots the panel's UI state as an LLSD map keyed by the same
    // setting names LLGameControl uses (AnalogChannelMappings, etc.).
    // The result can be fed to LLGameControl::applySettingsFromLLSD()
    // to push pending changes into the runtime without touching gSavedSettings.
    LLSD getSettingsAsLLSD();

    void updateDeviceListInternal(); // Refresh device list and settings

    // UI event handlers
    void onGridSelect(LLUICtrl* ctrl);         // Handle table row selection
    void onCommitInputChannel(LLUICtrl* ctrl); // Handle channel combobox selection
    void onAxisOptionsSelect();                // Handle axis options table selection
    void onCommitNumericValue();               // Handle deadzone/offset value changes
    void onChangeGameControlEnabled();         // Handle EnableGameControl toggle

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
    void populateAxisOptionsTableRows();    // Create axis options rows
    void populateAxisOptionsTableCells();   // Fill axis options with current values
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

    // Clears any other action row in the same category (avatar movement vs. flycam)
    // whose channel cell holds the given label.  Avatar movement and flycam are
    // independent no-duplicate sets; duplicate empty (NONE) mappings are allowed.
    // No-op if label is empty or no row is currently selected.
    void removeDuplicateActionMapping(const std::string& label);

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
