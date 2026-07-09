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
// Two visible sub-tabs:
//   - Actions (global, per mode Avatar/FlyCam/Captive): one table binding each
//     mode's axis and button actions to a canonical input.  Independent of device.
//   - Devices (per device): normalizes physical hardware to canonical inputs --
//     an axis-channels table (Input | Invert | Dead Zone | Offset | Output) and a
//     button-channels table (Input | Output).
//
// All settings are stored under a single "GameControl" key:
//   GameControl/ModeMappings/<Mode>/{Axes,Buttons} - GLOBAL action -> input
//   GameControl/Devices/<guid>/Config             - serialized per-device options
//
class LLPanelPreferenceGameControl : public LLPanelPreference
{
    LOG_CLASS(LLPanelPreferenceGameControl);
public:

    // Called when a device is connected/disconnected
    static void updateDeviceList();

    LLPanelPreferenceGameControl();
    ~LLPanelPreferenceGameControl();

    void onOpen(const LLSD& key) override;
    void draw() override;

    void apply() override;
    void cancel(const std::vector<std::string> settings_to_skip = {}) override;
    void saveSettings() override;

    // Snapshots the panel's UI state as the single "GameControl" setting map.
    // The result can be fed to LLGameControl::applySettingsFromLLSD()
    // to push pending changes into the runtime without touching gSavedSettings.
    LLSD getSettingsAsLLSD();

    void updateDeviceListInternal(); // Refresh device list and settings

    // UI event handlers
    void onGridSelect(LLUICtrl* ctrl);         // Handle table row selection
    void onCommitInputChannel(LLUICtrl* ctrl); // Handle input/output combobox selection
    void onCommitNumericValue();               // Handle deadzone/offset value changes

    // Live input capture support
    static bool isWaitingForInputChannel();    // True if a cell is waiting for input
    static void applyGameControlInput();       // Assign detected input to selected cell

    // Data Output tab: refresh the Value column with the channel values packed into
    // the most recent outgoing GameControlInput message.  Called from the send path.
    static void updateDataOutput();

protected:
    bool postBuild() override;

    // Actions tab (global, per-mode) population.
    void populateActionMappings();      // Rebuild the action table for the current mode
    LLGameControl::Options& getSelectedDeviceOptions();  // Get options for selected device

    // Devices tab (per-device) population.
    void populateDeviceTitle();         // Update device selector UI
    void populateDeviceSettings(const std::string& guid);  // Load settings for device
    void populateAxisChannelsRows();    // Create axis-channel rows (one per physical axis)
    void populateAxisChannelsCells();   // Fill axis-channel invert/deadzone/offset/output
    void populateButtonChannelsRows();  // Create button-channel rows (one per physical button)
    void populateButtonChannelsCells(); // Fill button-channel output column

    // Device State tab (per-device, live read-only) population.
    void updateDeviceStateList();       // Rebuild the state device selector from connected devices
    void populateAxisStateRows();       // Create axis-state rows (one per physical axis)
    void populateButtonStateRows();     // Create button-state rows (one per physical button)
    void populateDeviceStateValues();   // Fill the Value columns from the selected device's live state

    // Data Output tab (live, read-only view of the last outgoing GameControlInput).
    void populateDataOutputRows();      // Create the AXIS_/BUTTON_ label rows (with a blank separator)
    void populateDataOutputValues();    // Fill the Value column from the last outgoing server state

    // Utility methods
    static void setNumericLabel(LLScrollListCell* cell, S32 value);  // Format numeric cell
    static void fitInRect(LLUICtrl* ctrl, LLScrollListCtrl* grid, S32 row_index, S32 col_index);  // Position editor over cell

private:
    // Inline editing support
    bool initCombobox(LLScrollListItem* item, LLScrollListCtrl* grid);  // Show combobox over cell
    void clearSelectionState();  // Hide editors and clear selection tracking

    // Actions-tab helpers
    std::string currentEditMode();   // mode string ("Avatar"/"FlyCam"/"Captive") from mActionMode
    void populateActionModeList();       // build the per-mode rows (Enabled checkbox + label)
    void onActionModeChanged();      // rebuild the action table when the edit-mode selector changes
    void onModeEnabledToggled(S32 mode_ordinal, bool enabled);  // toggle conversion for one mode
    void updateActionModeEnabledUI();    // sync the per-mode checkboxes and lock/unlock the tables
    LLComboBox* actionSelectorForMode(bool axis, const std::string& mode) const;  // default vs flycam selector
    void removeDuplicateActionInput(const std::string& mode, const std::string& kind,
        const std::string& keep_action, const std::string& input_value, const LLComboBox* input_selector);
    static std::string inputLabel(const LLComboBox* input_selector, const std::string& input_value);  // value -> label
    static std::string selectorLabelAt(const LLComboBox* selector, S32 index);  // label of item at index

    // Handlers for individual table selections/commits
    void onAxisChannelsSelect();     // invert checkbox / deadzone-offset editor / output popup
    void onButtonChannelsSelect();   // output popup

    // Reset to defaults handlers
    void onResetActionsToDefaults();    // Reset current mode's action mappings
    void onResetDeviceToDefaults();     // Reset selected device's options (remap/invert/deadzone/offset)

    void rememberOriginalSettings();  // Capture settings for cancel restoration

    // Sends game_control data to server
    LLCheckBoxCtrl* mCheckGameControlEnabled { nullptr };   // master on/off for the feature
    LLCheckBoxCtrl* mCheckGameControlToServer { nullptr };

    // Sub-tab panels
    LLPanel* mTabActions { nullptr };
    LLPanel* mTabDevices { nullptr };

    // Actions tab
    LLScrollListCtrl* mActionMode { nullptr };      // Avatar / FlyCam / Captive rows,
                                                    // each with an Enabled checkbox
    LLButton* mRestoreActionsDefaults { nullptr };
    LLScrollListCtrl* mActionMappingsAxes { nullptr };     // Action | Axis Input (axis action block)
    LLScrollListCtrl* mActionMappingsButtons { nullptr };  // Action | Buttons Input (button action block)

    // Devices tab
    LLTextBox* mNoDeviceMessage { nullptr };     // Shown when no devices connected
    LLTextBox* mDevicePrompt { nullptr };        // "Device:" label
    LLComboBox* mDeviceList { nullptr };         // Dropdown listing available devices
    LLCheckBoxCtrl* mCheckShowAllDevices { nullptr };  // Include disconnected devices
    LLPanel* mPanelDeviceSettings { nullptr };
    LLButton* mRestoreDeviceDefaults { nullptr };
    LLScrollListCtrl* mAxisChannels { nullptr };    // Input | Invert | Dead Zone | Offset | Output
    LLScrollListCtrl* mButtonChannels { nullptr };  // Input | Output

    // Device State tab (per-device, live read-only)
    LLPanel* mTabDeviceState { nullptr };
    LLTextBox* mStateNoDeviceMessage { nullptr };  // Shown when no devices connected
    LLTextBox* mStateDevicePrompt { nullptr };     // "Device:" label
    LLTextBox* mStateRemapNote { nullptr };        // Note that values are post-remap
    LLComboBox* mStateDeviceList { nullptr };      // Dropdown listing connected devices
    LLPanel* mPanelDeviceState { nullptr };        // Wrapper holding the two state tables
    LLScrollListCtrl* mAxisState { nullptr };      // Axis | Value (live raw axis values)
    LLScrollListCtrl* mButtonState { nullptr };    // Button | Value (live pressed state)
    std::string mStateSelectedDeviceGUID;          // GUID of the device shown in the state tab

    // Data Output tab (live, read-only view of the last outgoing GameControlInput)
    LLPanel* mTabDataOutput { nullptr };
    LLScrollListCtrl* mDataOutput { nullptr };     // Output | Value (last sent axis/button values)

    // Inline editors - positioned over table cells when editing
    LLSpinCtrl* mNumericValueEditor { nullptr };    // For deadzone/offset values
    LLComboBox* mAxisInputSelector { nullptr };     // Axis-action Input popup (sticks + "Triggers left/right" pair)
    LLComboBox* mAxisOutputSelector { nullptr };    // Device axis Output popup (individual canonical axes, index-based)
    LLComboBox* mButtonInputSelector { nullptr };   // Canonical buttons (action Input + button Output popups)

    // Action selectors: source of the "Action" column rows, chosen by edit mode.
    LLComboBox* mAnalogActionSelector { nullptr };        // Avatar/Captive axis actions
    LLComboBox* mBinaryActionSelector { nullptr };        // Avatar/Captive button actions
    LLComboBox* mFlycamAnalogActionSelector { nullptr };  // FlyCam axis actions
    LLComboBox* mFlycamBinaryActionSelector { nullptr };  // FlyCam button actions

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
