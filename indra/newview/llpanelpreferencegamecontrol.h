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
//     an axis-channels table and a button-channels table, each mapping the physical
//     input (icon + text) to the canonical output (icon + text).  Axis tuning
//     (invert/offset/dead zone) lives on the Device State tab.
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
    void populateAxisStateOptionCells();// Fill the state-tab axis option cells (invert/offset/dead zone) from the state device
    void populateButtonStateRows();     // Create button-state rows (one per physical button)
    void populateDeviceStateValues();   // Fill the Value columns from the selected device's live state

    // Data Output tab (live, read-only view of the last outgoing GameControlInput).
    void populateDataOutputRows();      // Create the AXIS_/BUTTON_ label rows (with a blank separator)
    void populateDataOutputValues();    // Fill the Value column from the last outgoing server state

    // Utility methods
    // Column index of a named axis-state column.  Lets the code address the
    // Device State axis table by column name, so the XML column order is the
    // single source of truth: columns can be reordered in the XML with no code
    // change here.  Returns -1 if no such column exists.
    S32 axisStateColumn(const std::string& name) const;
    static void setNumericLabel(LLScrollListCell* cell, S32 value);  // Format numeric cell
    static void fitInRect(LLUICtrl* ctrl, LLScrollListCtrl* grid, S32 row_index, S32 col_index);  // Position editor over cell

private:
    // Inline editing support
    bool initCombobox(LLScrollListItem* item, LLScrollListCtrl* grid);  // Show combobox over cell
    void clearSelectionState();  // Hide editors and clear selection tracking

    // Actions-tab helpers
    std::string currentEditMode();   // mode string ("Avatar"/"FlyCam"/"Captive") from mActionMode
    void onActionModeChanged();      // rebuild the action table when the edit-mode selector changes
    void onModeEnabledToggled(bool enabled);  // toggle conversion for the currently-selected mode
    void updateActionModeEnabledUI();    // sync the Enabled checkbox and lock/unlock the tables
    LLComboBox* actionSelectorForMode(bool axis, const std::string& mode) const;  // default vs flycam selector
    void removeDuplicateActionInput(const std::string& mode, const std::string& kind,
        const std::string& keep_action, const std::string& input_value, const LLComboBox* input_selector);
    static std::string inputLabel(const LLComboBox* input_selector, const std::string& input_value);  // value -> label
    static std::string selectorLabelAt(const LLComboBox* selector, S32 index);  // label of item at index
    // Fill glyph_selector with one item per value in text_selector, each labelled
    // with that input's PromptFont glyph (or its text label where no glyph exists).
    static void buildInputGlyphSelector(const LLComboBox* text_selector, LLComboBox* glyph_selector);

    // Handlers for individual table selections/commits
    void onAxisChannelsSelect();     // output popup (Device Options tab)
    void onButtonChannelsSelect();   // output popup
    void onAxisStateSelect();        // invert checkbox / offset-deadzone editor (Device State tab)

    // Reset to defaults handlers
    void onResetActionsToDefaults();    // Reset current mode's action mappings
    void onResetDeviceToDefaults();     // Reset selected device's options (remap/invert/deadzone/offset)
    void onResetDeviceOptionsToDefaults();  // Reset state device's per-axis tuning (invert/offset/dead zone)

    // Auto-calibration: samples the state device's raw axes over several frames and
    // derives Offset/Dead Zone for axes that don't move.  See updateAutoCalibration().
    void onAutoCalibrate();          // Kick off a new calibration pass
    void updateAutoCalibration();    // Advance the in-progress pass; called once per frame from draw()

    void rememberOriginalSettings();  // Capture settings for cancel restoration

    // Sends game_control data to server
    LLCheckBoxCtrl* mCheckGameControlToServer { nullptr };

    // Sub-tab panels
    LLPanel* mTabActions { nullptr };
    LLPanel* mTabDevices { nullptr };

    // Actions tab
    LLComboBox* mActionMode { nullptr };            // selects which mode's mappings are edited
    LLCheckBoxCtrl* mCheckActionModeEnabled { nullptr };  // enables/disables the selected mode
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
    LLScrollListCtrl* mAxisChannels { nullptr };    // input(glyph) | input_description | output_description | output(glyph)
    LLScrollListCtrl* mButtonChannels { nullptr };  // input(glyph) | input_description | output_description | output(glyph)

    // Device State tab (per-device, live read-only)
    LLPanel* mTabDeviceState { nullptr };
    LLTextBox* mStateNoDeviceMessage { nullptr };  // Shown when no devices connected
    LLTextBox* mStateDevicePrompt { nullptr };     // "Device:" label
    LLTextBox* mStateRemapNote { nullptr };        // Note that values are post-remap
    LLComboBox* mStateDeviceList { nullptr };      // Dropdown listing connected devices
    LLButton* mRestoreDeviceOptionsDefaults { nullptr };  // Restore per-axis tuning to defaults
    LLButton* mAutoCalibrate { nullptr };          // Kicks off automatic offset/dead-zone calibration
    LLPanel* mPanelDeviceState { nullptr };        // Wrapper holding the two state tables
    LLScrollListCtrl* mAxisState { nullptr };      // Input(glyph) | Axis | Raw/Adjusted values | invert/offset/dead-zone
    LLScrollListCtrl* mButtonState { nullptr };    // Input(glyph) | Button | Value (live pressed state)
    std::string mStateSelectedDeviceGUID;          // GUID of the device shown in the state tab

    // Auto-calibration: in-progress sample capture kicked off by mAutoCalibrate.
    // updateAutoCalibration() (called from draw()) appends one raw-axis sample per
    // physical axis each frame; once CALIBRATION_SAMPLE_COUNT samples are collected,
    // any axis whose samples were all equal is treated as being at rest, and its
    // Offset/Dead Zone are derived from that reading.
    static constexpr S32 CALIBRATION_SAMPLE_COUNT = 5;
    bool mCalibrating { false };
    S32 mCalibrationSamplesCollected { 0 };
    std::vector<std::vector<S16>> mCalibrationSamples;  // per physical axis, up to CALIBRATION_SAMPLE_COUNT raw samples

    // Data Output tab (live, read-only view of the last outgoing GameControlInput)
    LLPanel* mTabDataOutput { nullptr };
    LLScrollListCtrl* mDataOutput { nullptr };     // Output | Value (last sent axis/button values)

    // Inline editors - positioned over table cells when editing
    LLSpinCtrl* mNumericValueEditor { nullptr };    // For deadzone/offset values
    LLComboBox* mAxisInputSelector { nullptr };     // Axis-action Input popup (sticks + "Triggers left/right" pair)
    LLComboBox* mAxisOutputSelector { nullptr };    // Device axis Output popup (individual canonical axes, index-based)
    LLComboBox* mButtonInputSelector { nullptr };   // Canonical buttons (action Input + button Output popups)

    // Glyph counterparts of the two action-Input selectors above, shown when the
    // user edits the PromptFont icon ("Axis"/"Button") column instead of the text
    // "input_description" column.  Same values, but each item is rendered as its
    // PromptFont glyph.  Built in postBuild() from the text selectors' values.
    LLComboBox* mAxisInputGlyphSelector { nullptr };
    LLComboBox* mButtonInputGlyphSelector { nullptr };

    // Glyph counterpart of mAxisOutputSelector, shown when the user edits the
    // PromptFont icon ("output") column of the Devices-tab axis-channels table
    // instead of the text "output_description" column.  Built in postBuild().
    LLComboBox* mAxisOutputGlyphSelector { nullptr };

    // Action selectors: source of the "Action" column rows, chosen by edit mode.
    LLComboBox* mAnalogActionSelector { nullptr };        // Avatar axis actions
    LLComboBox* mBinaryActionSelector { nullptr };        // Avatar button actions
    LLComboBox* mFlycamAnalogActionSelector { nullptr };  // FlyCam axis actions
    LLComboBox* mFlycamBinaryActionSelector { nullptr };  // FlyCam button actions
    LLComboBox* mCaptiveBinaryActionSelector { nullptr }; // Captive (sitting) button actions

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
