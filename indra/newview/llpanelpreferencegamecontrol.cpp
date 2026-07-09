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

#include "llpanelpreferencegamecontrol.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lltabcontainer.h"
#include "llspinctrl.h"
#include "lltextbox.h"


namespace
{
    // Singleton instance pointer - only one game control panel exists at a time
    static LLPanelPreferenceGameControl* sGameControlPanel { nullptr };

    // Track current UI selection state for input/output channel assignment.
    // When user clicks a cell to change its mapping, these track which cell is being edited.
    static LLScrollListCtrl* sSelectedGrid { nullptr };
    static LLScrollListItem* sSelectedItem { nullptr };
    static LLScrollListCell* sSelectedCell { nullptr };

    // Selector items use the label "None" for the unmapped entry.
    // Table cells should render that as a blank cell rather than the word.
    const std::string NONE_LABEL("None");

    // Kinds of action mapping, used both as the LLGameControl mapping key and as
    // the block discriminator stored in each action-table row's value.
    const std::string KIND_AXES("Axes");
    const std::string KIND_BUTTONS("Buttons");

    LLSD blankIfNone(const LLSD& label)
    {
        return label.asString() == NONE_LABEL ? LLSD() : label;
    }
}

// Static entry point called when device list changes (device connected/disconnected).
// Delegates to the singleton instance if it exists.
void LLPanelPreferenceGameControl::updateDeviceList()
{
    if (sGameControlPanel)
    {
        sGameControlPanel->updateDeviceListInternal();
    }
}

// Constructor - registers this instance as the singleton
LLPanelPreferenceGameControl::LLPanelPreferenceGameControl()
{
    sGameControlPanel = this;
}

// Destructor - clears singleton pointer
LLPanelPreferenceGameControl::~LLPanelPreferenceGameControl()
{
    sGameControlPanel = nullptr;
}

static LLPanelInjector<LLPanelPreferenceGameControl> t_pref_game_control("panel_preference_game_control");

// Snapshot the panel's UI state as the single "GameControl" setting.
// The returned map can be persisted (saveSettings) or applied directly to
// LLGameControl runtime state via LLGameControl::applySettingsFromLLSD().
LLSD LLPanelPreferenceGameControl::getSettingsAsLLSD()
{
    // Fold each device's current options into GameControl/Devices/<guid>/Config.
    // The global per-mode action mappings are already live in LLGameControl (edited
    // via its accessors), so snapshotting the whole GameControl structure captures both.
    for (auto& [guid, device] : mDeviceOptions)
    {
        device.settings = device.options.saveToString(device.name);
        LLGameControl::setDeviceConfig(guid, device.settings);
    }

    LLSD result = LLSD::emptyMap();
    result["GameControl"] = LLGameControl::getGameControlSettings();
    return result;
}

// Saves current UI state to settings by pushing the "GameControl" snapshot
// (global action mappings + per-device Config) into gSavedSettings.
void LLPanelPreferenceGameControl::saveSettings()
{
    LLPanelPreference::saveSettings();

    if (mOrigSettings.isEmpty())
    {
        rememberOriginalSettings();
    }

    // Push every entry of the snapshot into gSavedSettings, also tracking
    // the new value in mSavedValues so the base panel's accounting stays in sync.
    LLSD settings = getSettingsAsLLSD();
    for (auto it = settings.beginMap(); it != settings.endMap(); ++it)
    {
        if (LLControlVariable* control = gSavedSettings.getControl(it->first))
        {
            control->set(it->second);
            mSavedValues[control] = control->getValue();
        }
    }
}

// Handles row selection in the Actions table.  If the user clicked the editable
// Input column of a real (non-spacer) row, shows the appropriate input selector.
void LLPanelPreferenceGameControl::onGridSelect(LLUICtrl* ctrl)
{
    clearSelectionState();

    LLScrollListCtrl* table = dynamic_cast<LLScrollListCtrl*>(ctrl);
    if (!table || !table->getEnabled())
        return;

    if (LLScrollListItem* item = table->getFirstSelected())
    {
        // Try to show combobox for editing; if not applicable, deselect
        if (initCombobox(item, table))
            return;

        table->deselectAllItems();
    }
}

// Initializes and displays the appropriate combobox over the selected cell.
// Chooses the combobox and editable column from the table and (for the Actions
// table) the row's block kind.  Returns true if a combobox was shown, false if the
// click wasn't on an editable cell.
bool LLPanelPreferenceGameControl::initCombobox(LLScrollListItem* item, LLScrollListCtrl* grid)
{
    LLComboBox* combobox = nullptr;
    S32 col = -1;

    if (grid == mActionMappingsAxes || grid == mActionMappingsButtons)
    {
        // Only the Input column (1) of a real action row is editable.
        LLSD row_value = item->getValue();
        if (item->getSelectedCell() != 1 || !row_value.isMap())
            return false;
        combobox = (grid == mActionMappingsAxes) ? mAxisInputSelector : mButtonInputSelector;
        col = 1;
    }
    else if (grid == mAxisChannels)
    {
        // Only the Output column (4) uses a popup selector.
        if (item->getSelectedCell() != 4)
            return false;
        combobox = mAxisInputSelector;
        col = 4;
    }
    else if (grid == mButtonChannels)
    {
        // Only the Output column (1) uses a popup selector.
        if (item->getSelectedCell() != 1)
            return false;
        combobox = mButtonInputSelector;
        col = 1;
    }

    if (!combobox)
        return false;

    LLScrollListText* cell = dynamic_cast<LLScrollListText*>(item->getColumn(col));
    if (!cell)
        return false;

    // compute new rect for combobox
    S32 row_index = grid->getItemIndex(item);
    fitInRect(combobox, grid, row_index, col);

    // Pre-select the dropdown item whose label matches the cell's current text.
    // Match against the selector items directly (rather than parsing the item
    // value) so this works regardless of the value naming scheme.
    std::string value;
    std::string cell_value = cell->getValue();
    std::vector<LLScrollListItem*> items = combobox->getAllData();
    for (const LLScrollListItem* combo_item : items)
    {
        if (combo_item->getColumn(0)->getValue().asString() == cell_value)
        {
            value = combo_item->getValue().asString();
            break;
        }
    }
    if (value.empty())
    {
        // No match (e.g. a blank "None" cell): select the last item, which is "None".
        value = items.back()->getValue().asString();
    }

    combobox->setValue(value);
    combobox->setVisible(true);
    combobox->showList();

    sSelectedGrid = grid;
    sSelectedItem = item;
    sSelectedCell = cell;

    return true;
}

// Called when the user selects a value from an input/output selector combobox.
// The originating grid determines the meaning: Actions table -> action mapping;
// axis/button channels table -> per-device physical-to-canonical remap.
void LLPanelPreferenceGameControl::onCommitInputChannel(LLUICtrl* ctrl)
{
    if (!sSelectedGrid || !sSelectedItem || !sSelectedCell)
        return;

    LLComboBox* combobox = dynamic_cast<LLComboBox*>(ctrl);
    llassert(combobox);
    if (!combobox)
        return;

    if (sSelectedGrid == mActionMappingsAxes || sSelectedGrid == mActionMappingsButtons)
    {
        // The edited row identifies the action + block; the combobox supplies the input.
        LLSD row_value = sSelectedItem->getValue();
        std::string mode = currentEditMode();
        std::string kind = row_value["kind"].asString();
        std::string action = row_value["action"].asString();
        const LLComboBox* input_selector = (kind == KIND_AXES) ? mAxisInputSelector : mButtonInputSelector;
        std::string input_value = combobox->getValue().asString();
        std::string input_label = combobox->getSelectedItemLabel();

        if (input_label != NONE_LABEL)
        {
            // An input can drive at most one action within a block: clear it elsewhere.
            removeDuplicateActionInput(mode, kind, action, input_value, input_selector);
        }

        // Store the mapping directly in LLGameControl's live GameControl settings.
        // gSavedSettings is updated later via saveSettings() when the user clicks OK.
        LLGameControl::updateModeMapping(mode, kind, action, input_value);
        sSelectedCell->setValue(blankIfNone(input_label));
    }
    else if (sSelectedGrid == mAxisChannels)
    {
        // The row is a physical axis; the selected item's index is the canonical
        // axis it maps to.  "None" (index >= NUM_AXES) is not a valid output.
        S32 axis = mAxisChannels->getItemIndex(sSelectedItem);
        S32 output = combobox->getCurrentIndex();
        LLGameControl::Options& options = getSelectedDeviceOptions();
        if (output >= 0 && output < (S32)LLGameControl::NUM_AXES)
        {
            options.getAxisMap()[axis] = (U8)output;
            LLGameControl::setDeviceOptions(mSelectedDeviceGUID, options);
        }
        // Re-render the cell from the actual map (handles the ignored "None" case).
        sSelectedCell->setValue(selectorLabelAt(mAxisInputSelector, options.getAxisMap()[axis]));
    }
    else if (sSelectedGrid == mButtonChannels)
    {
        // The row is a physical button; the selected item's index is the canonical
        // button it maps to.  "None" (index >= NUM_BUTTONS) is not a valid output.
        S32 button = mButtonChannels->getItemIndex(sSelectedItem);
        S32 output = combobox->getCurrentIndex();
        LLGameControl::Options& options = getSelectedDeviceOptions();
        if (output >= 0 && output < (S32)LLGameControl::NUM_BUTTONS)
        {
            options.getButtonMap()[button] = (U8)output;
            LLGameControl::setDeviceOptions(mSelectedDeviceGUID, options);
        }
        sSelectedCell->setValue(selectorLabelAt(mButtonInputSelector, options.getButtonMap()[button]));
    }

    sSelectedGrid->deselectAllItems();
    clearSelectionState();
}

// Returns true if a cell is currently selected and waiting for input channel assignment.
// Used to determine whether to capture live controller input.
bool LLPanelPreferenceGameControl::isWaitingForInputChannel()
{
    return sSelectedCell != nullptr;
}

// Static method called when controller input is detected while a channel selector is open.
// Automatically assigns the detected input channel to the selected cell.
// NOTE: live-input capture is currently disabled (stub); the selector popups are the
// only way to assign an input.
void LLPanelPreferenceGameControl::applyGameControlInput()
{
}

// Handles selection in the axis-channels table (invert, deadzone, offset, output).
// Syncs the invert flag immediately; shows the numeric editor for deadzone/offset,
// or the axis selector popup for the Output column.
void LLPanelPreferenceGameControl::onAxisChannelsSelect()
{
    clearSelectionState();

    if (LLScrollListItem* row = mAxisChannels->getFirstSelected())
    {
        LLGameControl::Options& deviceOptions = getSelectedDeviceOptions();
        S32 row_index = mAxisChannels->getItemIndex(row);

        {
            // Always sync invert checkbox - clicking the checkbox selects the row
            // but doesn't automatically update the underlying option
            constexpr S32 invert_checkbox_column = 1;
            bool invert = row->getColumn(invert_checkbox_column)->getValue().asBoolean();
            S32 multiplier = invert ? -1 : 1;
            if (multiplier != deviceOptions.getAxisOptions()[row_index].mMultiplier)
            {
                deviceOptions.getAxisOptions()[row_index].mMultiplier = multiplier;

                if (row_index == LLGameControl::AXIS_LEFT_TRIGGER
                        || row_index == LLGameControl::AXIS_RIGHT_TRIGGER)
                {
                    // The two trigger axes act as one bidirectional axis under the hood,
                    // so their inversion must stay in sync -- update the sibling axis's
                    // option and reflect the new state in its checkbox cell.
                    S32 other_row_index = row_index == LLGameControl::AXIS_LEFT_TRIGGER
                            ? LLGameControl::AXIS_RIGHT_TRIGGER
                            : LLGameControl::AXIS_LEFT_TRIGGER;
                    deviceOptions.getAxisOptions()[other_row_index].mMultiplier = multiplier;

                    if (LLScrollListItem* other_row = mAxisChannels->getItemByIndex(other_row_index))
                    {
                        other_row->getColumn(invert_checkbox_column)->setValue(invert);
                    }
                }

                LLGameControl::setDeviceOptions(mSelectedDeviceGUID, deviceOptions);
            }
        }

        S32 column_index = row->getSelectedCell();
        if (column_index == 2 || column_index == 3)
        {
            fitInRect(mNumericValueEditor, mAxisChannels, row_index, column_index);
            if (column_index == 2)
            {
                mNumericValueEditor->setMinValue(0);
                mNumericValueEditor->setMaxValue(LLGameControl::MAX_AXIS_DEAD_ZONE);
                mNumericValueEditor->setValue(deviceOptions.getAxisOptions()[row_index].mDeadZone);
            }
            else // column_index == 3
            {
                mNumericValueEditor->setMinValue(-LLGameControl::MAX_AXIS_OFFSET);
                mNumericValueEditor->setMaxValue(LLGameControl::MAX_AXIS_OFFSET);
                mNumericValueEditor->setValue(deviceOptions.getAxisOptions()[row_index].mOffset);
            }
            mNumericValueEditor->setVisible(true);
        }
        else
        {
            // Output column (4) shows the axis selector popup; other columns are no-ops.
            initCombobox(row, mAxisChannels);
        }
    }
}

// Handles selection in the button-channels table.  Only the Output column uses a
// popup selector; the Input column is fixed (the physical button).
void LLPanelPreferenceGameControl::onButtonChannelsSelect()
{
    clearSelectionState();

    if (LLScrollListItem* row = mButtonChannels->getFirstSelected())
    {
        initCombobox(row, mButtonChannels);
    }
}

// Called when user commits a numeric value (deadzone or offset) in the spin control.
// Validates and clamps the value, then updates both the UI and device options.
void LLPanelPreferenceGameControl::onCommitNumericValue()
{
    if (LLScrollListItem* row = mAxisChannels->getFirstSelected())
    {
        LLGameControl::Options& deviceOptions = getSelectedDeviceOptions();
        S32 value = mNumericValueEditor->getValue().asInteger();
        S32 row_index = mAxisChannels->getItemIndex(row);
        S32 column_index = row->getSelectedCell();
        llassert(column_index == 2 || column_index == 3);  // 2=deadzone, 3=offset
        if (column_index < 2 || column_index > 3)
            return;

        if (column_index == 2)
        {
            value = std::clamp<S32>(value, 0, LLGameControl::MAX_AXIS_DEAD_ZONE);
            deviceOptions.getAxisOptions()[row_index].mDeadZone = (U16)value;
        }
        else  // column_index == 3
        {
            value = std::clamp<S32>(value, -LLGameControl::MAX_AXIS_OFFSET, LLGameControl::MAX_AXIS_OFFSET);
            deviceOptions.getAxisOptions()[row_index].mOffset = (S16)value;
        }
        setNumericLabel(row->getColumn(column_index), value);
        LLGameControl::setDeviceOptions(mSelectedDeviceGUID, deviceOptions);
    }
}

// Initializes all UI controls and sets up callbacks.
// Called once when the panel is first built from XML.
bool LLPanelPreferenceGameControl::postBuild()
{
    // Send-to-server checkbox (top-left of the main panel)
    mCheckGameControlToServer = getChild<LLCheckBoxCtrl>("game_control_to_server");
    mCheckGameControlToServer->setCommitCallback([this](LLUICtrl*, const LLSD&)
        {
            LLGameControl::setSendToServer(mCheckGameControlToServer->getValue());
        });

    getChild<LLTabContainer>("game_control_tabs")->setCommitCallback([this](LLUICtrl*, const LLSD&) { clearSelectionState(); });

    mTabActions = getChild<LLPanel>("tab_actions");
    mTabDevices = getChild<LLPanel>("tab_devices");

    // Actions tab (global, per-mode)
    mActionMode = getChild<LLComboBox>("action_mode");
    mActionMode->setCommitCallback([this](LLUICtrl*, const LLSD&) { onActionModeChanged(); });

    mRestoreActionsDefaults = getChild<LLButton>("restore_actions_defaults");
    mRestoreActionsDefaults->setCommitCallback([this](LLUICtrl*, const LLSD&) { onResetActionsToDefaults(); });

    mActionMappingsAxes = getChild<LLScrollListCtrl>("action_mappings_axes");
    mActionMappingsAxes->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onGridSelect(ctrl); });

    mActionMappingsButtons = getChild<LLScrollListCtrl>("action_mappings_buttons");
    mActionMappingsButtons->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onGridSelect(ctrl); });

    // Devices tab (per-device)
    mNoDeviceMessage = getChild<LLTextBox>("nodevice_message");
    mDevicePrompt = getChild<LLTextBox>("device_prompt");
    mDeviceList = getChild<LLComboBox>("device_list");
    mCheckShowAllDevices = getChild<LLCheckBoxCtrl>("show_all_known_devices");
    mPanelDeviceSettings = getChild<LLPanel>("device_settings");

    mCheckShowAllDevices->setCommitCallback([this](LLUICtrl*, const LLSD&) { populateDeviceTitle(); });
    mDeviceList->setCommitCallback([this](LLUICtrl*, const LLSD& value) { populateDeviceSettings(value); });

    mRestoreDeviceDefaults = getChild<LLButton>("restore_device_defaults");
    mRestoreDeviceDefaults->setCommitCallback([this](LLUICtrl*, const LLSD&) { onResetDeviceToDefaults(); });

    mAxisChannels = getChild<LLScrollListCtrl>("axis_channels");
    mAxisChannels->setCommitCallback([this](LLUICtrl*, const LLSD&) { onAxisChannelsSelect(); });

    mButtonChannels = getChild<LLScrollListCtrl>("button_channels");
    mButtonChannels->setCommitCallback([this](LLUICtrl*, const LLSD&) { onButtonChannelsSelect(); });

    // Device State tab (per-device, live read-only)
    mTabDeviceState = getChild<LLPanel>("tab_device_state");
    mStateNoDeviceMessage = getChild<LLTextBox>("state_nodevice_message");
    mStateDevicePrompt = getChild<LLTextBox>("state_device_prompt");
    mStateRemapNote = getChild<LLTextBox>("state_remap_note");
    mStateDeviceList = getChild<LLComboBox>("state_device_list");
    mPanelDeviceState = getChild<LLPanel>("device_state_settings");
    mStateDeviceList->setCommitCallback([this](LLUICtrl*, const LLSD& value)
        { mStateSelectedDeviceGUID = value.asString(); });

    mAxisState = getChild<LLScrollListCtrl>("axis_state");
    mButtonState = getChild<LLScrollListCtrl>("button_state");

    // Data Output tab (live, read-only view of the last outgoing GameControlInput)
    mTabDataOutput = getChild<LLPanel>("tab_data_output");
    mDataOutput = getChild<LLScrollListCtrl>("data_output");

    // Spin control for editing deadzone/offset values inline
    mNumericValueEditor = getChild<LLSpinCtrl>("numeric_value_editor");
    mNumericValueEditor->setCommitCallback([this](LLUICtrl*, const LLSD&) { onCommitNumericValue(); });

    // Action selectors: provide the "Action" column rows, chosen by edit mode.
    mAnalogActionSelector = getChild<LLComboBox>("analog_action_selector");
    mBinaryActionSelector = getChild<LLComboBox>("binary_action_selector");
    mFlycamAnalogActionSelector = getChild<LLComboBox>("flycam_analog_action_selector");
    mFlycamBinaryActionSelector = getChild<LLComboBox>("flycam_binary_action_selector");

    // Canonical input selectors, shown inline when editing an Input/Output cell.
    mAxisInputSelector = getChild<LLComboBox>("axis_input_selector");
    mAxisInputSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });

    mButtonInputSelector = getChild<LLComboBox>("button_input_selector");
    mButtonInputSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });

    // The device tables' rows are static (one per physical axis/button); their
    // cells are filled per-device.  The action table is mode-specific and built
    // in populateActionMappings() on open / mode change.
    populateAxisChannelsRows();
    populateButtonChannelsRows();

    // The Device State tab's rows are static (one per physical axis/button); their
    // Value cells are refreshed every frame in draw() from the selected device.
    populateAxisStateRows();
    populateButtonStateRows();

    // The Data Output tab's rows are static (AXIS_0..5, a blank separator, then
    // BUTTON_0..31); their Value cells are refreshed whenever a fresh
    // GameControlInput message is sent (see updateDataOutput()).
    populateDataOutputRows();

    // Workaround for the common bug:
    // LLScrollListCtrl with draw_heading="true" initially has incorrect mTop (17 px higher).
    // Each scroll list fills a dedicated wrapper panel, so the corrected top is its
    // parent's height - 1.
    auto fixHeadingTop = [](LLScrollListCtrl* grid)
    {
        LLRect rect = grid->getRect();
        rect.mTop = grid->getParent()->getRect().getHeight() - 1;
        grid->setRect(rect);
        grid->updateLayout();
    };
    fixHeadingTop(mActionMappingsAxes);
    fixHeadingTop(mActionMappingsButtons);
    fixHeadingTop(mAxisChannels);
    fixHeadingTop(mButtonChannels);
    fixHeadingTop(mAxisState);
    fixHeadingTop(mButtonState);
    fixHeadingTop(mDataOutput);

    return true;
}

// Called when the preferences floater is opened.
// Loads current LLGameControl state into UI controls and refreshes all tables.
void LLPanelPreferenceGameControl::onOpen(const LLSD& key)
{
    // Sync checkbox with current LLGameControl state
    mCheckGameControlToServer->setValue(LLGameControl::sendToServer());

    clearSelectionState();

    // Default to editing the Avatar mode's mappings each time the panel opens
    // (Avatar is the first action_mode item).
    mActionMode->selectFirstItem();
    populateActionMappings();

    // Refresh device list and settings
    updateDeviceListInternal();

    mCheckGameControlToServer->setEnabled(true);
    mActionMappingsAxes->setEnabled(true);
    mActionMappingsButtons->setEnabled(true);
    mAxisChannels->setEnabled(true);
    mButtonChannels->setEnabled(true);
    mDeviceList->setEnabled(true);
    mStateDeviceList->setEnabled(true);

    // Clear original settings - will be populated on first saveSettings() call
    mOrigSettings = LLSD::emptyMap();
}

// Per-frame refresh of the Device State tab.  Only touches the tables while that
// tab is actually the visible one, so it costs nothing on the other tabs.
void LLPanelPreferenceGameControl::draw()
{
    if (mTabDeviceState && mTabDeviceState->getVisible())
    {
        populateDeviceStateValues();
    }
    LLPanelPreference::draw();
}

// Called when user clicks OK. Clears selections and prepares for onClose()
void LLPanelPreferenceGameControl::apply()
{
    LLPanelPreference::apply();
    clearSelectionState();

    // Clear mOrigSettings to prevent cancel() from reverting changes.
    // Note: cancel() is called in onClose() even after OK, so we must clear this.
    mOrigSettings = LLSD::emptyMap();

    // Note: any settings changes have already been written to global-settings
    // via the LLControlVariables assigned to the UI elements. LLGameControl has
    // already loaded from global-settings in saveSettings().
}

// Called when user clicks Cancel. Restores all settings to their original values.
void LLPanelPreferenceGameControl::cancel(const std::vector<std::string> settings_to_skip)
{
    LLPanelPreference::cancel(settings_to_skip);
    clearSelectionState();

    // Skip restore if original settings were cleared in apply().
    if (mOrigSettings.isEmpty())
    {
        return;
    }

    // Restore in-memory state from the snapshot, then push back to gSavedSettings
    // via the bulk save path so persistence stays consistent.
    LLGameControl::applySettingsFromLLSD(mOrigSettings);
    LLGameControl::saveToSettings();
}

// Rebuilds the internal device options map from LLGameControl state.
// Merges saved options with currently connected devices.
void LLPanelPreferenceGameControl::updateDeviceListInternal()
{
    mDeviceOptions.clear();

    // Load saved device options from settings
    for (const auto& [guid, options] : LLGameControl::getDeviceOptions())
    {
        DeviceOptions deviceOptions = { LLStringUtil::null, options, LLGameControl::Options() };
        deviceOptions.options.loadFromString(deviceOptions.name, deviceOptions.settings);
        mDeviceOptions.emplace(guid, deviceOptions);
    }

    // Add currently connected devices that don't have saved settings yet
    for (const auto& device : LLGameControl::getDevices())
    {
        if (mDeviceOptions.find(device.getGUID()) == mDeviceOptions.end())
        {
            mDeviceOptions[device.getGUID()] = { device.getName(), device.saveOptionsToString(true), device.getOptions() };
        }
    }

    mCheckShowAllDevices->setValue(false);
    populateDeviceTitle();

    // The Device State tab tracks the live set of connected devices too.
    updateDeviceStateList();
}

// Updates the device selection UI based on available devices.
// Shows single device name, dropdown list, or "no device" message as appropriate.
void LLPanelPreferenceGameControl::populateDeviceTitle()
{
    mSelectedDeviceGUID.clear();

    // "Show all devices" includes saved settings for disconnected devices
    bool showAllDevices = mCheckShowAllDevices->getValue().asBoolean();
    std::size_t deviceCount = showAllDevices ? mDeviceOptions.size() : LLGameControl::getDevices().size();

    mNoDeviceMessage->setVisible(!deviceCount);
    mDevicePrompt->setVisible(deviceCount);
    mDeviceList->setVisible(deviceCount);   // always use the combo box, even for one device
    mPanelDeviceSettings->setVisible(deviceCount);
    mRestoreDeviceDefaults->setVisible(deviceCount);

    if (!deviceCount)
    {
        return;
    }

    auto makeTitle = [](const std::string& guid, const std::string& name) -> std::string
    {
        return guid + ", " + name;
    };

    auto makeListItem = [](const std::string& guid, const std::string& title)
    {
        return LLSD().with("value", guid).with("columns", LLSD().with("label", title));
    };

    mDeviceList->clear();
    mDeviceList->clearRows();

    if (showAllDevices)
    {
        for (const auto& [guid, device] : mDeviceOptions)
        {
            mDeviceList->addElement(makeListItem(guid, makeTitle(guid, device.name)));
        }
    }
    else
    {
        for (const LLGameControl::Device& device : LLGameControl::getDevices())
        {
            mDeviceList->addElement(makeListItem(device.getGUID(), makeTitle(device.getGUID(), device.getName())));
        }
    }

    mDeviceList->selectNthItem(0);
    populateDeviceSettings(mDeviceList->getValue());
}

// Loads settings for the specified device into the device-channel tables.
void LLPanelPreferenceGameControl::populateDeviceSettings(const std::string& guid)
{
    mSelectedDeviceGUID = guid;
    llassert_always(mDeviceOptions.find(guid) != mDeviceOptions.end());

    // Device tables are per-device (mode-independent).
    populateAxisChannelsCells();
    populateButtonChannelsCells();
}

// The mode string ("When moving Avatar"/"When moving FlyCam"/"When sitting") currently selected for editing.
std::string LLPanelPreferenceGameControl::currentEditMode()
{
    S32 ordinal = mActionMode->getValue().asInteger();
    return LLGameControl::getModeName((LLGameControl::AgentControlMode)ordinal);
}

// Picks the action selector supplying the "Action" rows for a block + mode.
LLComboBox* LLPanelPreferenceGameControl::actionSelectorForMode(bool axis, const std::string& mode) const
{
    bool flycam = (mode == "FlyCam");
    if (axis)
        return flycam ? mFlycamAnalogActionSelector : mAnalogActionSelector;
    return flycam ? mFlycamBinaryActionSelector : mBinaryActionSelector;
}

// Rebuilds the two Actions tables for the current mode: axis actions in the top
// table, button actions in the bottom table.  Each row stores {kind, action} as its
// value and shows the action label + its currently-mapped input.
void LLPanelPreferenceGameControl::populateActionMappings()
{
    mActionMappingsAxes->clearRows();
    mActionMappingsButtons->clearRows();

    std::string mode = currentEditMode();

    auto addBlock = [&](LLScrollListCtrl* grid, const std::string& kind,
        const LLComboBox* action_selector, const LLComboBox* input_selector)
    {
        LLSD mapping = LLGameControl::getModeMapping(mode, kind);

        for (LLScrollListItem* item : action_selector->getAllData())
        {
            std::string action = item->getValue().asString();
            if (action == NONE_LABEL)   // skip the "None" sentinel; not an action row
                continue;

            LLScrollListItem::Params row_params;
            LLScrollListCell::Params cell_params;
            // Match the font size used by the "Controls" preferences panel
            cell_params.font = LLFontGL::getFontSansSerif();
            for (S32 c = 0; c < grid->getNumColumns(); ++c)
            {
                cell_params.column = grid->getColumn(c)->mName;
                row_params.columns.add(cell_params);
            }

            // Row value carries the block kind and action key (used on commit/lookup).
            LLSD row_value;
            row_value["kind"] = kind;
            row_value["action"] = action;
            row_params.value = row_value;

            LLScrollListItem* row = grid->addRow(row_params);
            row->getColumn(0)->setValue(item->getColumn(0)->getValue());  // action label
            std::string input_value = mapping.has(action) ? mapping[action].asString() : LLStringUtil::null;
            row->getColumn(1)->setValue(blankIfNone(inputLabel(input_selector, input_value)));
        }
    };

    addBlock(mActionMappingsAxes, KIND_AXES, actionSelectorForMode(true, mode), mAxisInputSelector);
    addBlock(mActionMappingsButtons, KIND_BUTTONS, actionSelectorForMode(false, mode), mButtonInputSelector);
}

// Handles a change in the edit-mode selector: rebuild the action table.
void LLPanelPreferenceGameControl::onActionModeChanged()
{
    clearSelectionState();
    populateActionMappings();
}

// Enforces that an input drives at most one action within a mode's block.  When a
// new (non-None) input is assigned to an action, any *other* action in the same
// mode+kind mapping that currently uses that same input is reset to "None"
// (its Input cell blanked and its mapping set to the selector's None value).
void LLPanelPreferenceGameControl::removeDuplicateActionInput(const std::string& mode,
    const std::string& kind, const std::string& keep_action, const std::string& input_value,
    const LLComboBox* input_selector)
{
    // The selector's last item is the "None" value (e.g. AXIS_NONE / BUTTON_NONE).
    std::string none_value = input_selector->getAllData().back()->getValue().asString();
    LLSD mapping = LLGameControl::getModeMapping(mode, kind);

    LLScrollListCtrl* grid = (kind == KIND_AXES) ? mActionMappingsAxes : mActionMappingsButtons;
    for (LLScrollListItem* row : grid->getAllData())
    {
        LLSD row_value = row->getValue();
        if (!row_value.isMap())
            continue;

        std::string action = row_value["action"].asString();
        if (action == keep_action)
            continue;

        if (mapping.has(action) && mapping[action].asString() == input_value)
        {
            LLGameControl::updateModeMapping(mode, kind, action, none_value);
            row->getColumn(1)->setValue(LLSD());  // blank the Input cell
        }
    }
}

// Creates one row per physical axis in the axis-channels table.
// Columns: Input (physical axis) | Invert | Dead Zone | Offset | Output.
void LLPanelPreferenceGameControl::populateAxisChannelsRows()
{
    mAxisChannels->clearRows();

    // Column 0 shows the physical controller axis inputs (not actions)
    std::vector<LLScrollListItem*> items = mAxisInputSelector->getAllData();

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    // Match the font size used by the "Controls" preferences panel
    cell_params.font = LLFontGL::getFontSansSerif();
    for (S32 i = 0; i < (S32)(mAxisChannels->getNumColumns()); ++i)
    {
        cell_params.column = mAxisChannels->getColumn(i)->mName;
        row_params.columns.add(cell_params);
    }

    // Configure column types and alignment
    row_params.columns(1).type = "checkbox";     // Invert
    row_params.columns(2).font_halign = "right";  // Dead Zone
    row_params.columns(3).font_halign = "right";  // Offset

    for (size_t i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        LLScrollListItem* row = mAxisChannels->addRow(row_params);
        row->getColumn(0)->setValue(items[i]->getColumn(0)->getValue());  // physical axis label
    }
}

// Fills axis-channel cells (invert / deadzone / offset / output) from the current device.
void LLPanelPreferenceGameControl::populateAxisChannelsCells()
{
    std::vector<LLScrollListItem*> rows = mAxisChannels->getAllData();
    const LLGameControl::Options& options = getSelectedDeviceOptions();
    const auto& all_axis_options = options.getAxisOptions();
    const auto& axis_map = options.getAxisMap();
    llassert(rows.size() == all_axis_options.size());

    for (size_t i = 0; i < rows.size(); ++i)
    {
        LLScrollListItem* row = rows[i];
        const LLGameControl::Options::AxisOptions& axis_options = all_axis_options[i];
        row->getColumn(1)->setValue(axis_options.mMultiplier == -1);  // Invert checkbox
        setNumericLabel(row->getColumn(2), axis_options.mDeadZone);
        setNumericLabel(row->getColumn(3), axis_options.mOffset);
        row->getColumn(4)->setValue(selectorLabelAt(mAxisInputSelector, axis_map[i]));  // Output
    }
}

// Creates one row per physical button in the button-channels table.
// Columns: Input (physical button) | Output.
void LLPanelPreferenceGameControl::populateButtonChannelsRows()
{
    mButtonChannels->clearRows();

    std::vector<LLScrollListItem*> items = mButtonInputSelector->getAllData();

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    cell_params.font = LLFontGL::getFontSansSerif();
    for (S32 i = 0; i < (S32)(mButtonChannels->getNumColumns()); ++i)
    {
        cell_params.column = mButtonChannels->getColumn(i)->mName;
        row_params.columns.add(cell_params);
    }

    for (size_t i = 0; i < LLGameControl::NUM_BUTTONS; ++i)
    {
        LLScrollListItem* row = mButtonChannels->addRow(row_params);
        row->getColumn(0)->setValue(items[i]->getColumn(0)->getValue());  // physical button label
    }
}

// Fills the button-channel Output column from the current device's button map.
void LLPanelPreferenceGameControl::populateButtonChannelsCells()
{
    std::vector<LLScrollListItem*> rows = mButtonChannels->getAllData();
    const auto& button_map = getSelectedDeviceOptions().getButtonMap();
    llassert(rows.size() == button_map.size());

    for (size_t i = 0; i < rows.size(); ++i)
    {
        rows[i]->getColumn(1)->setValue(selectorLabelAt(mButtonInputSelector, button_map[i]));
    }
}

// Rebuilds the Device State selector from the currently connected devices.
// Only connected devices are listed here, since the tab shows live input and a
// disconnected device has no live state.  Shows the "No device" message and hides
// the tables when nothing is connected.
void LLPanelPreferenceGameControl::updateDeviceStateList()
{
    mStateDeviceList->clear();
    mStateDeviceList->clearRows();

    const std::list<LLGameControl::Device>& devices = LLGameControl::getDevices();
    bool hasDevice = !devices.empty();

    mStateNoDeviceMessage->setVisible(!hasDevice);
    mStateDevicePrompt->setVisible(hasDevice);
    mStateRemapNote->setVisible(hasDevice);
    mStateDeviceList->setVisible(hasDevice);
    mPanelDeviceState->setVisible(hasDevice);

    if (!hasDevice)
    {
        mStateSelectedDeviceGUID.clear();
        return;
    }

    for (const LLGameControl::Device& device : devices)
    {
        std::string title = device.getGUID() + ", " + device.getName();
        mStateDeviceList->addElement(LLSD().with("value", device.getGUID())
            .with("columns", LLSD().with("label", title)));
    }

    // Keep the current selection if that device is still connected; otherwise
    // fall back to the first device.
    if (mStateSelectedDeviceGUID.empty() || !mStateDeviceList->setSelectedByValue(mStateSelectedDeviceGUID, true))
    {
        mStateDeviceList->selectNthItem(0);
    }
    mStateSelectedDeviceGUID = mStateDeviceList->getValue().asString();
}

// Creates one row per physical axis in the axis-state table.
// Columns: Axis (physical axis label) | Value (filled live in draw()).
void LLPanelPreferenceGameControl::populateAxisStateRows()
{
    mAxisState->clearRows();

    std::vector<LLScrollListItem*> items = mAxisInputSelector->getAllData();

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    cell_params.font = LLFontGL::getFontSansSerif();
    for (S32 i = 0; i < (S32)(mAxisState->getNumColumns()); ++i)
    {
        cell_params.column = mAxisState->getColumn(i)->mName;
        row_params.columns.add(cell_params);
    }
    row_params.columns(1).font_halign = "right";  // Raw Value
    row_params.columns(2).font_halign = "right";  // Value

    for (size_t i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        LLScrollListItem* row = mAxisState->addRow(row_params);
        row->getColumn(0)->setValue(items[i]->getColumn(0)->getValue());  // physical axis label
    }
}

// Creates one row per physical button in the button-state table.
// Columns: Button (physical button label) | Value (filled live in draw()).
void LLPanelPreferenceGameControl::populateButtonStateRows()
{
    mButtonState->clearRows();

    std::vector<LLScrollListItem*> items = mButtonInputSelector->getAllData();

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    cell_params.font = LLFontGL::getFontSansSerif();
    for (S32 i = 0; i < (S32)(mButtonState->getNumColumns()); ++i)
    {
        cell_params.column = mButtonState->getColumn(i)->mName;
        row_params.columns.add(cell_params);
    }
    row_params.columns(1).font_halign = "right";  // Value

    for (size_t i = 0; i < LLGameControl::NUM_BUTTONS; ++i)
    {
        LLScrollListItem* row = mButtonState->addRow(row_params);
        row->getColumn(0)->setValue(items[i]->getColumn(0)->getValue());  // physical button label
    }
}

// Fills the Value columns of both state tables from the selected device's live
// input state.  Axis values are the raw [0, 32767] readings; button values are
// 1 (pressed) or 0 (released).  Called each frame while the tab is visible.
void LLPanelPreferenceGameControl::populateDeviceStateValues()
{
    if (mStateSelectedDeviceGUID.empty())
    {
        return;
    }

    // Locate the live state for the selected device by GUID.
    const LLGameControl::State* state = nullptr;
    for (const LLGameControl::Device& device : LLGameControl::getDevices())
    {
        if (device.getGUID() == mStateSelectedDeviceGUID)
        {
            state = &device.getState();
            break;
        }
    }
    if (!state)
    {
        return;
    }

    // State stores each canonical axis as a +/- half-axis pair, so axis a's
    // signed value is (positive half) - (negative half).  mRawAxes is pre-fix,
    // mAxes is post-fix; both share the same layout and sign convention.
    std::vector<LLScrollListItem*> axisRows = mAxisState->getAllData();
    for (size_t i = 0; i < axisRows.size(); ++i)
    {
        size_t base = i * 2;
        if (base + 1 >= state->mAxes.size())
        {
            break;
        }
        S32 raw   = (S32)state->mRawAxes[base] - (S32)state->mRawAxes[base + 1];
        S32 fixed = (S32)state->mAxes[base]    - (S32)state->mAxes[base + 1];
        axisRows[i]->getColumn(1)->setValue(llformat("%d ", raw));    // Raw Value (pre-fix)
        axisRows[i]->getColumn(2)->setValue(llformat("%d ", fixed));  // Value (post-fix)
    }

    std::vector<LLScrollListItem*> buttonRows = mButtonState->getAllData();
    for (size_t i = 0; i < buttonRows.size(); ++i)
    {
        S32 value = (state->mButtons >> i) & 0x1;
        buttonRows[i]->getColumn(1)->setValue(llformat("%d ", value));
    }
}

// Static hook, called from the send path each time a fresh GameControlInput
// message goes out.  Refreshes the Data Output tab's Value column from the
// values just packed into that message (the current server state).
void LLPanelPreferenceGameControl::updateDataOutput()
{
    if (sGameControlPanel)
    {
        sGameControlPanel->populateDataOutputValues();
    }
}

// Creates the Data Output rows: one per canonical axis (AXIS_0..AXIS_5), a single
// empty separator row, then one per canonical button (BUTTON_0..BUTTON_31).
// The Value column is filled later in populateDataOutputValues().
void LLPanelPreferenceGameControl::populateDataOutputRows()
{
    mDataOutput->clearRows();

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    cell_params.font = LLFontGL::getFontSansSerif();
    for (S32 i = 0; i < (S32)(mDataOutput->getNumColumns()); ++i)
    {
        cell_params.column = mDataOutput->getColumn(i)->mName;
        row_params.columns.add(cell_params);
    }
    row_params.columns(1).font_halign = "right";  // Value

    for (size_t i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        LLScrollListItem* row = mDataOutput->addRow(row_params);
        row->getColumn(0)->setValue(llformat("AXIS_%d", (S32)i));
    }

    // One empty row separates the axis block from the button block.
    mDataOutput->addRow(row_params);

    for (size_t i = 0; i < LLGameControl::NUM_BUTTONS; ++i)
    {
        LLScrollListItem* row = mDataOutput->addRow(row_params);
        row->getColumn(0)->setValue(llformat("BUTTON_%d", (S32)i));
    }
}

// Fills the Data Output Value column from the last outgoing server state: each
// axis shows its signed [-32768, 32767] value; each button shows 1 (pressed) or
// 0 (released).  The row layout matches populateDataOutputRows() (axes, then a
// blank separator row, then buttons).
void LLPanelPreferenceGameControl::populateDataOutputValues()
{
    const LLGameControl::ServerState& state = LLGameControl::getServerState();

    std::vector<LLScrollListItem*> rows = mDataOutput->getAllData();
    size_t row = 0;

    for (size_t i = 0; i < LLGameControl::NUM_AXES && i < state.mAxes.size() && row < rows.size(); ++i, ++row)
    {
        rows[row]->getColumn(1)->setValue(llformat("%d ", (S32)state.mAxes[i]));
    }

    ++row;  // skip the empty separator row

    for (size_t i = 0; i < LLGameControl::NUM_BUTTONS && row < rows.size(); ++i, ++row)
    {
        S32 value = (state.mButtons >> i) & 0x1;
        rows[row]->getColumn(1)->setValue(llformat("%d ", value));
    }
}

// Returns the display label of the input selector item with the given value,
// or empty if there is no such item.
std::string LLPanelPreferenceGameControl::inputLabel(const LLComboBox* input_selector, const std::string& input_value)
{
    for (const LLScrollListItem* item : input_selector->getAllData())
    {
        if (item->getValue().asString() == input_value)
        {
            return item->getColumn(0)->getValue().asString();
        }
    }
    return LLStringUtil::null;
}

// Returns the display label of the selector item at the given index (the item
// position matches the canonical axis/button index), or empty if out of range.
std::string LLPanelPreferenceGameControl::selectorLabelAt(const LLComboBox* selector, S32 index)
{
    std::vector<LLScrollListItem*> items = selector->getAllData();
    if (index < 0 || index >= (S32)items.size())
    {
        return LLStringUtil::null;
    }
    return items[index]->getColumn(0)->getValue().asString();
}

// Returns the options struct for the currently selected device.
LLGameControl::Options& LLPanelPreferenceGameControl::getSelectedDeviceOptions()
{
    auto options_it = mDeviceOptions.find(mSelectedDeviceGUID);
    llassert_always(options_it != mDeviceOptions.end());
    return options_it->second.options;
}

// Formats a numeric value for display in a table cell.
// Always shows the value, including the zero (default) case.
void LLPanelPreferenceGameControl::setNumericLabel(LLScrollListCell* cell, S32 value)
{
    cell->setValue(llformat("%d ", value));
}

// Positions a UI control (combobox or spin control) to overlay a specific table cell.
// Used to show editors inline within the scroll list tables.
void LLPanelPreferenceGameControl::fitInRect(LLUICtrl* ctrl, LLScrollListCtrl* grid, S32 row_index, S32 col_index)
{
    LLRect rect(grid->getCellRect(row_index, col_index));
    LLView* parent = grid->getParent();
    while (parent && parent != ctrl->getParent())
    {
        rect.translate(parent->getRect().mLeft, parent->getRect().mBottom);
        parent = parent->getParent();
    }

    ctrl->setRect(rect);
    rect.translate(-rect.mLeft, -rect.mBottom);
    for (LLView* child : *ctrl->getChildList())
    {
        LLRect childRect(child->getRect());
        childRect.intersectWith(rect);
        if (childRect.mRight < rect.mRight &&
            childRect.mRight > (rect.mLeft + rect.mRight) / 2)
        {
            childRect.mRight = rect.mRight;
        }
        child->setRect(childRect);
    }
}

// Clears the current cell selection state and hides all popup editors.
void LLPanelPreferenceGameControl::clearSelectionState()
{
    sSelectedGrid = nullptr;
    sSelectedItem = nullptr;
    sSelectedCell = nullptr;
    mNumericValueEditor->setVisible(false);
    mAxisInputSelector->setVisible(false);
    mButtonInputSelector->setVisible(false);
}

// Resets the current mode's axis + button action mappings to the built-in defaults.
void LLPanelPreferenceGameControl::onResetActionsToDefaults()
{
    clearSelectionState();

    std::string mode = currentEditMode();
    LLSD defaults = LLGameControl::getDefaultModeMappings();
    LLGameControl::setModeMapping(mode, KIND_AXES, defaults[mode][KIND_AXES]);
    LLGameControl::setModeMapping(mode, KIND_BUTTONS, defaults[mode][KIND_BUTTONS]);
    populateActionMappings();

    // Push the pending UI state into LLGameControl's runtime so the effect is
    // immediate.  gSavedSettings is updated later via saveSettings() on OK.
    LLGameControl::applySettingsFromLLSD(getSettingsAsLLSD());
}

// Resets the selected device's options (axis/button remap, invert, deadzone, offset)
// to defaults.
void LLPanelPreferenceGameControl::onResetDeviceToDefaults()
{
    clearSelectionState();
    if (mSelectedDeviceGUID.empty())
    {
        return;
    }

    LLGameControl::Options& options = getSelectedDeviceOptions();
    options.resetToDefaults();
    LLGameControl::setDeviceOptions(mSelectedDeviceGUID, options);

    populateAxisChannelsCells();
    populateButtonChannelsCells();

    LLGameControl::applySettingsFromLLSD(getSettingsAsLLSD());
}

// Captures current settings values into mOrigSettings for later restoration upon cancel().
void LLPanelPreferenceGameControl::rememberOriginalSettings()
{
    mOrigSettings = LLGameControl::getSettingsAsLLSD();
}
