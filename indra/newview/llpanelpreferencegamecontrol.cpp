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

#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lltabcontainer.h"
#include "llspinctrl.h"
#include "lltextbox.h"


namespace
{
    // Singleton instance pointer - only one game control panel exists at a time
    static LLPanelPreferenceGameControl* sGameControlPanel { nullptr };

    // Track current UI selection state for input channel assignment.
    // When user clicks a cell to change its mapping, these track which cell is being edited.
    static LLScrollListCtrl* sSelectedGrid { nullptr };
    static LLScrollListItem* sSelectedItem { nullptr };
    static LLScrollListCell* sSelectedCell { nullptr };
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

// Snapshot the panel's UI state into an LLSD map keyed by gSavedSettings names.
// The returned map can be persisted (saveSettings) or applied directly to
// LLGameControl runtime state via LLGameControl::applySettingsFromLLSD().
LLSD LLPanelPreferenceGameControl::getSettingsAsLLSD()
{
    std::vector<LLScrollListItem*> items = mActionTable->getAllData();

    // Lambda to find the channel associated with an action by looking it up in the UI table
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

    LLSD result = LLSD::emptyMap();
    result["AnalogChannelMappings"] = LLGameControl::stringifyAnalogMappings(getChannel);
    result["BinaryChannelMappings"] = LLGameControl::stringifyBinaryMappings(getChannel);
    result["FlycamChannelMappings"] = LLGameControl::stringifyFlycamMappings(getChannel);

    // Serialize per-device options, omitting any with empty settings strings
    LLSD deviceOptions = LLSD::emptyMap();
    for (auto& [guid, device] : mDeviceOptions)
    {
        device.settings = device.options.saveToString(device.name);
        if (!device.settings.empty())
        {
            deviceOptions.insert(guid, device.settings);
        }
    }
    result["KnownGameControllers"] = deviceOptions;

    return result;
}

// Saves current UI state to settings.
// Extracts channel mappings from the action table and converts to string format.
// Also saves per-device options (axis settings, remappings) to KnownGameControllers.
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

// Handles row selection in any of the mapping tables (action, axis, button).
// If user clicked the editable channel column, shows the appropriate combobox for editing.
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
// Determines which combobox to use based on the table and action type.
// Returns true if a combobox was shown, false if the click wasn't on an editable cell.
bool LLPanelPreferenceGameControl::initCombobox(LLScrollListItem* item, LLScrollListCtrl* grid)
{
    // Only column 1 (the channel/mapping column) is editable
    if (item->getSelectedCell() != 1)
        return false;

    LLScrollListText* cell = dynamic_cast<LLScrollListText*>(item->getColumn(1));
    if (!cell)
        return false;

    // Select combobox based on table type and action category
    LLComboBox* combobox = nullptr;
    if (grid == mActionTable)
    {
        std::string action = item->getValue();
        LLGameControl::ActionNameType actionNameType = LLGameControl::getActionNameType(action);
        combobox =
            actionNameType == LLGameControl::ACTION_NAME_ANALOG ? mAnalogChannelSelector :
            actionNameType == LLGameControl::ACTION_NAME_BINARY ? mBinaryChannelSelector :
            actionNameType == LLGameControl::ACTION_NAME_FLYCAM ? mAnalogChannelSelector :
            nullptr;
    }
    else if (grid == mAxisMappings)
    {
        combobox = mAxisSelector;
    }
    else if (grid == mButtonMappings)
    {
        combobox = mBinaryChannelSelector;
    }
    if (!combobox)
        return false;

    // compute new rect for combobox
    S32 row_index = grid->getItemIndex(item);
    fitInRect(combobox, grid, row_index, 1);

    std::string channel_name = "NONE";
    std::string cell_value = cell->getValue();
    std::vector<LLScrollListItem*> items = combobox->getAllData();
    for (const LLScrollListItem* item : items)
    {
        if (item->getColumn(0)->getValue().asString() == cell_value)
        {
            channel_name = item->getValue().asString();
            break;
        }
    }

    std::string value;
    LLGameControl::InputChannel channel = LLGameControl::getChannelByName(channel_name);
    if (!channel.isNone())
    {
        std::string channel_name = channel.getLocalName();
        std::string channel_label = getChannelLabel(channel_name, combobox->getAllData());
        if (combobox->itemExists(channel_label))
        {
            value = channel_name;
        }
    }
    if (value.empty())
    {
        // Assign the last element in the dropdown list which is "NONE"
        value = combobox->getAllData().back()->getValue().asString();
    }

    combobox->setValue(value);
    combobox->setVisible(true);
    combobox->showList();

    sSelectedGrid = grid;
    sSelectedItem = item;
    sSelectedCell = cell;

    return true;
}

// Called when user selects a channel from the dropdown combobox.
// Updates the cell display and, for device mappings, updates the internal options.
void LLPanelPreferenceGameControl::onCommitInputChannel(LLUICtrl* ctrl)
{
    if (!sSelectedGrid || !sSelectedItem || !sSelectedCell)
        return;

    LLComboBox* combobox = dynamic_cast<LLComboBox*>(ctrl);
    llassert(combobox);
    if (!combobox)
        return;

    if (sSelectedGrid == mActionTable)
    {
        std::string value = combobox->getValue();
        std::string label = (value == "NONE") ?
            LLStringUtil::null : combobox->getSelectedItemLabel();

        removeDuplicateActionMapping(label);

        sSelectedCell->setValue(label);
    }
    else
    {
        S32 chosen_index = combobox->getCurrentIndex();
        if (chosen_index >= 0)
        {
            int row_index = sSelectedGrid->getItemIndex(sSelectedItem);
            llassert(row_index >= 0);
            LLGameControl::Options& deviceOptions = getSelectedDeviceOptions();
            std::vector<U8>& map = sSelectedGrid == mAxisMappings ?
                deviceOptions.getAxisMap() : deviceOptions.getButtonMap();
            if (chosen_index >= map.size())
            {
                chosen_index = row_index;
            }
            std::string label = chosen_index == row_index ?
                LLStringUtil::null : combobox->getSelectedItemLabel();
            sSelectedCell->setValue(label);
            map[row_index] = chosen_index;
        }
    }
    sSelectedGrid->deselectAllItems();
    clearSelectionState();

    // Push the panel's pending UI state straight into LLGameControl's runtime so
    // the effect is immediate.  gSavedSettings is updated later via saveSettings()
    // when the user clicks OK.
    LLGameControl::applySettingsFromLLSD(getSettingsAsLLSD());
}

// Returns true if a cell is currently selected and waiting for input channel assignment.
// Used to determine whether to capture live controller input.
bool LLPanelPreferenceGameControl::isWaitingForInputChannel()
{
    return sSelectedCell != nullptr;
}

// Static method called when controller input is detected while a channel selector is open.
// Automatically assigns the detected input channel to the selected cell.
void LLPanelPreferenceGameControl::applyGameControlInput()
{
    if (!sGameControlPanel || !sSelectedGrid || !sSelectedCell)
        return;

    LLComboBox* combobox;
    LLGameControl::InputChannel::Type expectedType;
    if (sGameControlPanel->mAnalogChannelSelector->getVisible())
    {
        combobox = sGameControlPanel->mAnalogChannelSelector;
        expectedType = LLGameControl::InputChannel::TYPE_AXIS;
    }
    else if (sGameControlPanel->mBinaryChannelSelector->getVisible())
    {
        combobox = sGameControlPanel->mBinaryChannelSelector;
        expectedType = LLGameControl::InputChannel::TYPE_BUTTON;
    }
    else
    {
        return;
    }

    LLGameControl::InputChannel channel = LLGameControl::getActiveInputChannel();
    if (channel.mType == expectedType)
    {
        std::string channel_name = channel.getLocalName();
        std::string channel_label = LLPanelPreferenceGameControl::getChannelLabel(channel_name, combobox->getAllData());

        // Device remapping tables allow duplicate targets, so dedup only applies to mActionTable.
        if (sSelectedGrid == sGameControlPanel->mActionTable)
        {
            sGameControlPanel->removeDuplicateActionMapping(channel_label);
        }

        sSelectedCell->setValue(channel_label);
        sSelectedGrid->deselectAllItems();
        sGameControlPanel->clearSelectionState();

        // Push the panel's pending UI state straight into LLGameControl's
        // runtime without touching gSavedSettings -- the eventual OK click
        // (and the framework-driven saveSettings()) will persist them.
        LLGameControl::applySettingsFromLLSD(sGameControlPanel->getSettingsAsLLSD());
    }
}

// Handles selection in the axis options table (invert, deadzone, offset).
// Shows numeric editor for deadzone/offset columns, updates invert flag immediately.
void LLPanelPreferenceGameControl::onAxisOptionsSelect()
{
    clearSelectionState();

    if (LLScrollListItem* row = mAxisOptions->getFirstSelected())
    {
        LLGameControl::Options& deviceOptions = getSelectedDeviceOptions();
        S32 row_index = mAxisOptions->getItemIndex(row);

        {
            // Always sync invert checkbox - clicking the checkbox selects the row
            // but doesn't automatically update the underlying option
            constexpr S32 invert_checkbox_column = 1;
            bool invert = row->getColumn(invert_checkbox_column)->getValue().asBoolean();
            S32 multiplier = invert ? -1 : 1;
            if (multiplier != deviceOptions.getAxisOptions()[row_index].mMultiplier)
            {
                auto& axis_options = deviceOptions.getAxisOptions();
                deviceOptions.getAxisOptions()[row_index].mMultiplier = multiplier;

                if (row_index == LLGameControl::AXIS_TRIGGERLEFT
                        || row_index == LLGameControl::AXIS_TRIGGERRIGHT)
                {
                    // The two trigger axes act as one bidirectional axis under the hood,
                    // so their inversion must stay in sync -- update the sibling axis's
                    // option and reflect the new state in its checkbox cell.
                    S32 other_row_index = row_index == LLGameControl::AXIS_TRIGGERLEFT
                            ? LLGameControl::AXIS_TRIGGERRIGHT
                            : LLGameControl::AXIS_TRIGGERLEFT;
                    deviceOptions.getAxisOptions()[other_row_index].mMultiplier = multiplier;

                    if (LLScrollListItem* other_row = mAxisOptions->getItemByIndex(other_row_index))
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
            fitInRect(mNumericValueEditor, mAxisOptions, row_index, column_index);
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

        initCombobox(row, mAxisOptions);
    }
}

// Called when user commits a numeric value (deadzone or offset) in the spin control.
// Validates and clamps the value, then updates both the UI and device options.
void LLPanelPreferenceGameControl::onCommitNumericValue()
{
    if (LLScrollListItem* row = mAxisOptions->getFirstSelected())
    {
        LLGameControl::Options& deviceOptions = getSelectedDeviceOptions();
        S32 value = mNumericValueEditor->getValue().asInteger();
        S32 row_index = mAxisOptions->getItemIndex(row);
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

void LLPanelPreferenceGameControl::onChangeGameControlEnabled()
{
    bool enabled = gSavedSettings.getBOOL("EnableGameControl");
    LLGameControl::setEnabled(enabled);
    updateEnable();
}

// Initializes all UI controls and sets up callbacks.
// Called once when the panel is first built from XML.
bool LLPanelPreferenceGameControl::postBuild()
{
    gSavedSettings.getControl("EnableGameControl")->getSignal()->connect(boost::bind(&LLPanelPreferenceGameControl::onChangeGameControlEnabled, this));

    // Main checkboxes that control how game input is used
    mCheckGameControlToServer = getChild<LLCheckBoxCtrl>("game_control_to_server");
    mCheckGameControlToAgent = getChild<LLCheckBoxCtrl>("game_control_to_agent");
    mCheckAgentToGameControl = getChild<LLCheckBoxCtrl>("agent_to_game_control");

    mCheckGameControlToServer->setCommitCallback([this](LLUICtrl*, const LLSD&)
        {
            LLGameControl::setSendToServer(mCheckGameControlToServer->getValue());
            updateActionTableState();
        });
    mCheckGameControlToAgent->setCommitCallback([this](LLUICtrl*, const LLSD&)
        {
            LLGameControl::setControlAgent(mCheckGameControlToAgent->getValue());
            updateActionTableState();
        });
    mCheckAgentToGameControl->setCommitCallback([this](LLUICtrl*, const LLSD&)
        {
            LLGameControl::setTranslateAgentActions(mCheckAgentToGameControl->getValue());
            updateActionTableState();
        });

    getChild<LLTabContainer>("game_control_tabs")->setCommitCallback([this](LLUICtrl*, const LLSD&) { clearSelectionState(); });
    getChild<LLTabContainer>("device_settings_tabs")->setCommitCallback([this](LLUICtrl*, const LLSD&) { clearSelectionState(); });

    // 1st tab "Channel mappings"
    mTabChannelMappings = getChild<LLPanel>("tab_channel_mappings");
    mActionTable = getChild<LLScrollListCtrl>("action_table");
    mActionTable->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onGridSelect(ctrl); });

    // 2nd tab "Device settings"
    mTabDeviceSettings = getChild<LLPanel>("tab_device_settings");
    mNoDeviceMessage = getChild<LLTextBox>("nodevice_message");
    mDevicePrompt = getChild<LLTextBox>("device_prompt");
    mSingleDevice = getChild<LLTextBox>("single_device");
    mDeviceList = getChild<LLComboBox>("device_list");
    mCheckShowAllDevices = getChild<LLCheckBoxCtrl>("show_all_known_devices");
    mPanelDeviceSettings = getChild<LLPanel>("device_settings");

    mCheckShowAllDevices->setCommitCallback([this](LLUICtrl*, const LLSD&) { populateDeviceTitle(); });
    mDeviceList->setCommitCallback([this](LLUICtrl*, const LLSD& value) { populateDeviceSettings(value); });

    // Device settings sub-tabs: Axis Options, Axis Mappings, Button Mappings
    mTabAxisOptions = getChild<LLPanel>("tab_axis_options");
    mAxisOptions = getChild<LLScrollListCtrl>("axis_options");
    mAxisOptions->setCommitCallback([this](LLUICtrl*, const LLSD&) { onAxisOptionsSelect(); });

    mTabAxisMappings = getChild<LLPanel>("tab_axis_mappings");
    mAxisMappings = getChild<LLScrollListCtrl>("axis_mappings");
    mAxisMappings->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onGridSelect(ctrl); });

    mTabButtonMappings = getChild<LLPanel>("tab_button_mappings");
    mButtonMappings = getChild<LLScrollListCtrl>("button_mappings");
    mButtonMappings->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onGridSelect(ctrl); });

    mResetToDefaults = getChild<LLButton>("reset_to_defaults");
    mResetToDefaults->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onResetToDefaults(); });

    // Spin control for editing deadzone/offset values inline
    mNumericValueEditor = getChild<LLSpinCtrl>("numeric_value_editor");
    mNumericValueEditor->setCommitCallback([this](LLUICtrl*, const LLSD&) { onCommitNumericValue(); });

    // Dropdown selectors shown inline when editing channel assignments.
    // These are positioned over table cells dynamically by fitInRect().
    mAnalogChannelSelector = getChild<LLComboBox>("analog_channel_selector");  // For axis actions
    mAnalogChannelSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });

    mBinaryChannelSelector = getChild<LLComboBox>("binary_channel_selector");  // For button actions
    mBinaryChannelSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });

    mAxisSelector = getChild<LLComboBox>("axis_selector");  // For axis remapping
    mAxisSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });

    // Populate action table with rows from XML config files
    populateActionTableRows("game_control_table_rows.xml");        // Avatar movement actions
    addActionTableSeparator();
    populateActionTableRows("game_control_table_camera_rows.xml"); // Flycam movement actions

    // Populate device settings tables with empty rows
    populateAxisOptionsTableRows();
    populateMappingTableRows(mAxisMappings, mAxisSelector, LLGameControl::NUM_AXES);
    populateMappingTableRows(mButtonMappings, mBinaryChannelSelector, LLGameControl::NUM_BUTTONS);

    // Workaround for the common bug:
    // LLScrollListCtrl with draw_heading="true" initially has incorrect mTop (17 px higher)
    LLRect rect = mAxisOptions->getRect();
    rect.mTop = mAxisOptions->getParent()->getRect().getHeight() - 1;
    mAxisOptions->setRect(rect);
    mAxisOptions->updateLayout();

    return true;
}

// Called when the preferences floater is opened.
// Loads current LLGameControl state into UI controls and refreshes all tables.
void LLPanelPreferenceGameControl::onOpen(const LLSD& key)
{
    // Sync checkboxes with current LLGameControl state
    mCheckGameControlToServer->setValue(LLGameControl::getSendToServer());
    mCheckGameControlToAgent->setValue(LLGameControl::getControlAgent());
    mCheckAgentToGameControl->setValue(LLGameControl::getTranslateAgentActions());

    clearSelectionState();

    // Refresh action mappings table with current channel assignments
    populateActionTableCells();
    updateActionTableState();

    // Refresh device list and settings
    updateDeviceListInternal();
    updateEnable();

    // Clear original settings - will be populated on first saveSettings() call
    mOrigSettings = LLSD::emptyMap();
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
}

// Loads action definitions from an XML file and adds them as rows to the action table.
// Each row gets a second column for the channel assignment (populated later).
void LLPanelPreferenceGameControl::populateActionTableRows(const std::string& filename)
{
    LLScrollListCtrl::Contents contents;
    if (!parseXmlFile(contents, filename, "rows"))
        return;

    // Setup params for the channel column (column 1)
    LLScrollListCell::Params second_cell_params;
    second_cell_params.font = LLFontGL::getFontSansSerif();
    second_cell_params.font_halign = LLFontGL::LEFT;
    second_cell_params.column = mActionTable->getColumn(1)->mName;
    second_cell_params.value = ""; // Actual value is assigned in populateActionTableCells

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

// Fills in the channel column for each action row based on current LLGameControl mappings.
void LLPanelPreferenceGameControl::populateActionTableCells()
{
    std::vector<LLScrollListItem*> rows = mActionTable->getAllData();
    std::vector<LLScrollListItem*> axes = mAnalogChannelSelector->getAllData();
    std::vector<LLScrollListItem*> btns = mBinaryChannelSelector->getAllData();

    for (LLScrollListItem* row : rows)
    {
        if (row->getNumColumns() >= 2)  // Skip title and separator rows
        {
            std::string name = row->getValue().asString();
            if (!name.empty() && name != "menu_separator")
            {
                LLGameControl::InputChannel channel = LLGameControl::getChannelByAction(name);
                std::string channel_name = channel.getLocalName();
                std::string channel_label =
                    channel.isAxis() ? getChannelLabel(channel_name, axes) :
                    channel.isButton() ? getChannelLabel(channel_name, btns) :
                    LLStringUtil::null;
                row->getColumn(1)->setValue(channel_label);
            }
        }
    }
}

// Parses an XML file containing scroll list row definitions.
// Used to load action table rows from external configuration files.
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
    mSingleDevice->setVisible(deviceCount == 1);
    mDeviceList->setVisible(deviceCount > 1);
    mPanelDeviceSettings->setVisible(deviceCount);

    auto makeTitle = [](const std::string& guid, const std::string& name) -> std::string
    {
        return guid + ", " + name;
    };

    if (deviceCount == 1)
    {
        if (showAllDevices)
        {
            const auto& [guid, device] = *mDeviceOptions.begin();
            mSingleDevice->setValue(makeTitle(guid, device.name));
            populateDeviceSettings(guid);
        }
        else
        {
            const LLGameControl::Device& device = LLGameControl::getDevices().front();
            mSingleDevice->setValue(makeTitle(device.getGUID(), device.getName()));
            populateDeviceSettings(device.getGUID());
        }
    }
    else if (deviceCount)
    {
        mDeviceList->clear();
        mDeviceList->clearRows();

        auto makeListItem = [](const std::string& guid, const std::string& title)
        {
            return LLSD().with("value", guid).with("columns", LLSD().with("label", title));
        };

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
}

// Loads settings for the specified device into the device settings sub-tabs.
void LLPanelPreferenceGameControl::populateDeviceSettings(const std::string& guid)
{
    mSelectedDeviceGUID = guid;
    auto options_it = mDeviceOptions.find(guid);
    llassert_always(options_it != mDeviceOptions.end());
    const DeviceOptions& deviceOptions = options_it->second;

    // Populate all three device settings sub-tabs
    populateAxisOptionsTableCells();
    populateMappingTableCells(mAxisMappings, deviceOptions.options.getAxisMap(), mAxisSelector);
    populateMappingTableCells(mButtonMappings, deviceOptions.options.getButtonMap(), mBinaryChannelSelector);
}

// Creates empty rows in the axis options table (one per axis).
// Columns: axis name, invert checkbox, deadzone, offset
void LLPanelPreferenceGameControl::populateAxisOptionsTableRows()
{
    mAxisOptions->clearRows();

    std::vector<LLScrollListItem*> items = mAxisSelector->getAllData();

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    cell_params.font = LLFontGL::getFontMonospace();
    for (S32 i = 0; i < (S32)(mAxisOptions->getNumColumns()); ++i)
    {
        cell_params.column = mAxisOptions->getColumn(i)->mName;
        row_params.columns.add(cell_params);
    }

    // Configure column types and alignment
    row_params.columns(1).type = "checkbox";   // Invert
    row_params.columns(2).font_halign = "right"; // Deadzone
    row_params.columns(3).font_halign = "right"; // Offset

    for (size_t i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        LLScrollListItem* row = mAxisOptions->addRow(row_params);
        row->getColumn(0)->setValue(items[i]->getColumn(0)->getValue());
    }
}

// Fills axis options table cells with current device settings.
void LLPanelPreferenceGameControl::populateAxisOptionsTableCells()
{
    std::vector<LLScrollListItem*> rows = mAxisOptions->getAllData();
    const auto& all_axis_options = getSelectedDeviceOptions().getAxisOptions();
    llassert(rows.size() == all_axis_options.size());

    for (size_t i = 0; i < rows.size(); ++i)
    {
        LLScrollListItem* row = rows[i];
        const LLGameControl::Options::AxisOptions& axis_options = all_axis_options[i];
        row->getColumn(1)->setValue(axis_options.mMultiplier == -1);  // Invert checkbox
        setNumericLabel(row->getColumn(2), axis_options.mDeadZone);
        setNumericLabel(row->getColumn(3), axis_options.mOffset);
    }
}

// Creates empty rows in a mapping table (axis or button remapping).
// Column 0 shows the physical input name, column 1 will show the mapped target.
void LLPanelPreferenceGameControl::populateMappingTableRows(LLScrollListCtrl* target,
    const LLComboBox* source, size_t row_count)
{
    target->clearRows();

    // Use the channel selector combobox as source for input names
    std::vector<LLScrollListItem*> items = source->getAllData();

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    cell_params.font = LLFontGL::getFontMonospace();
    for (S32 i = 0; i < target->getNumColumns(); ++i)
    {
        cell_params.column = target->getColumn(i)->mName;
        row_params.columns.add(cell_params);
    }

    for (size_t i = 0; i < row_count; ++i)
    {
        LLScrollListItem* row = target->addRow(row_params);
        row->getColumn(0)->setValue(items[i]->getColumn(0)->getValue());
    }
}

// Fills mapping table cells with current remapping settings.
// Only shows a value if the input is remapped to something different.
void LLPanelPreferenceGameControl::populateMappingTableCells(LLScrollListCtrl* target,
    const std::vector<U8>& mappings, const LLComboBox* source)
{
    std::vector<LLScrollListItem*> rows = target->getAllData();
    std::vector<LLScrollListItem*> items = source->getAllData();
    llassert(rows.size() == mappings.size());

    for (size_t i = 0; i < rows.size(); ++i)
    {
        U8 mapping = mappings[i];
        llassert(mapping < items.size());
        // Show empty for default (identity) mapping, otherwise show target name
        rows[i]->getColumn(1)->setValue(mapping == i ? LLSD() :
            items[mapping]->getColumn(0)->getValue());
    }
}

// Returns the options struct for the currently selected device.
LLGameControl::Options& LLPanelPreferenceGameControl::getSelectedDeviceOptions()
{
    auto options_it = mDeviceOptions.find(mSelectedDeviceGUID);
    llassert_always(options_it != mDeviceOptions.end());
    return options_it->second.options;
}

// Looks up the display label for a channel name by searching the combobox items.
// Returns empty string if not found or if channel is "NONE".
std::string LLPanelPreferenceGameControl::getChannelLabel(const std::string& channel_name,
    const std::vector<LLScrollListItem*>& items)
{
    if (!channel_name.empty() && channel_name != "NONE")
    {
        for (LLScrollListItem* item : items)
        {
            if (item->getValue().asString() == channel_name)
            {
                if (item->getNumColumns())
                {
                    return item->getColumn(0)->getValue().asString();
                }
                break;
            }
        }
    }
    return LLStringUtil::null;
}

// Formats a numeric value for display in a table cell.
// Shows empty string for zero (default) values.
void LLPanelPreferenceGameControl::setNumericLabel(LLScrollListCell* cell, S32 value)
{
    cell->setValue(value ? llformat("%d ", value) : LLStringUtil::null);
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

// Clears any other action row in the same category (avatar movement vs. flycam) whose
// channel cell holds the given label.  Avatar movement and flycam are independent
// no-duplicate sets, so a channel may appear in one row of each set simultaneously.
// Duplicate empty (NONE) mappings are allowed, so no-op when label is empty.
void LLPanelPreferenceGameControl::removeDuplicateActionMapping(const std::string& label)
{
    if (label.empty() || !sSelectedItem)
        return;

    bool selected_is_flycam =
        LLGameControl::getActionNameType(sSelectedItem->getValue().asString())
        == LLGameControl::ACTION_NAME_FLYCAM;

    for (LLScrollListItem* item : mActionTable->getAllData())
    {
        if (item == sSelectedItem || item->getNumColumns() < 2)
            continue;

        bool item_is_flycam =
            LLGameControl::getActionNameType(item->getValue().asString())
            == LLGameControl::ACTION_NAME_FLYCAM;
        if (item_is_flycam != selected_is_flycam)
            continue;

        LLScrollListCell* cell = item->getColumn(1);
        if (cell && cell->getValue().asString() == label)
        {
            cell->setValue(LLStringUtil::null);
        }
    }
}

// Clears the current cell selection state and hides all popup editors.
void LLPanelPreferenceGameControl::clearSelectionState()
{
    sSelectedGrid = nullptr;
    sSelectedItem = nullptr;
    sSelectedCell = nullptr;
    mNumericValueEditor->setVisible(false);
    mAnalogChannelSelector->setVisible(false);
    mBinaryChannelSelector->setVisible(false);
    mAxisSelector->setVisible(false);
}

// Adds a visual separator row to the action table.
void LLPanelPreferenceGameControl::addActionTableSeparator()
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

// Enables or disables UI controls based on whether game control is enabled globally.
void LLPanelPreferenceGameControl::updateEnable()
{
    bool enabled = LLGameControl::isEnabled();
    LLGameControl::setEnabled(enabled);

    mCheckGameControlToServer->setEnabled(enabled);
    mCheckGameControlToAgent->setEnabled(enabled);
    mCheckAgentToGameControl->setEnabled(enabled);

    mActionTable->setEnabled(enabled);
    mAxisOptions->setEnabled(enabled);
    mAxisMappings->setEnabled(enabled);
    mButtonMappings->setEnabled(enabled);
    mDeviceList->setEnabled(enabled);

    if (!enabled)
    {
        //mActionTable->deselectAllItems();
        mAnalogChannelSelector->setVisible(false);
        mBinaryChannelSelector->setVisible(false);
        mAxisSelector->setVisible(false);
        clearSelectionState();
    }
}

// Updates action table enabled state based on checkbox settings.
// Table is only editable if game control affects agent movement.
void LLPanelPreferenceGameControl::updateActionTableState()
{
    bool enable_table = LLGameControl::isEnabled() &&
        (mCheckGameControlToAgent->get() || mCheckAgentToGameControl->get());

    mActionTable->deselectAllItems();
    mActionTable->setEnabled(enable_table);
    mAnalogChannelSelector->setVisible(false);
    mBinaryChannelSelector->setVisible(false);
}

// Handles "Reset to Defaults" button click.
// Resets the appropriate settings based on which tab/sub-tab is currently visible.
void LLPanelPreferenceGameControl::onResetToDefaults()
{
    clearSelectionState();
    if (mTabChannelMappings->getVisible())
    {
        resetChannelMappingsToDefaults();
    }
    else if (mTabDeviceSettings->getVisible() && !mSelectedDeviceGUID.empty())
    {
        if (mTabAxisOptions->getVisible())
        {
            resetAxisOptionsToDefaults();
        }
        else if (mTabAxisMappings->getVisible())
        {
            resetAxisMappingsToDefaults();
        }
        else if (mTabButtonMappings->getVisible())
        {
            resetButtonMappingsToDefaults();
        }
    }

    // Push the panel's pending UI state straight into LLGameControl's runtime so
    // the effect is immediate.  gSavedSettings is updated later via saveSettings()
    // when the user clicks OK.
    LLGameControl::applySettingsFromLLSD(getSettingsAsLLSD());
}

// Resets action-to-channel mappings to built-in defaults.
void LLPanelPreferenceGameControl::resetChannelMappingsToDefaults()
{
    std::vector<std::pair<std::string, LLGameControl::InputChannel>> mappings;
    LLGameControl::getDefaultMappings(mappings);
    std::vector<LLScrollListItem*> rows = mActionTable->getAllData();
    std::vector<LLScrollListItem*> axes = mAnalogChannelSelector->getAllData();
    std::vector<LLScrollListItem*> btns = mBinaryChannelSelector->getAllData();
    for (LLScrollListItem* row : rows)
    {
        if (row->getNumColumns() >= 2) // Skip separators
        {
            std::string action_name = row->getValue().asString();
            if (!action_name.empty() && action_name != "menu_separator")
            {
                std::string channel_label;
                for (const auto& mapping : mappings)
                {
                    if (mapping.first == action_name)
                    {
                        std::string channel_name = mapping.second.getLocalName();
                        channel_label =
                            mapping.second.isAxis() ? getChannelLabel(channel_name, axes) :
                            mapping.second.isButton() ? getChannelLabel(channel_name, btns) :
                            LLStringUtil::null;
                        break;
                    }
                }
                row->getColumn(1)->setValue(channel_label);
            }
        }
    }
}

// Resets axis options (invert, deadzone, offset) to defaults for selected device.
void LLPanelPreferenceGameControl::resetAxisOptionsToDefaults()
{
    std::vector<LLScrollListItem*> rows = mAxisOptions->getAllData();
    llassert(rows.size() == LLGameControl::NUM_AXES);
    LLGameControl::Options& options = getSelectedDeviceOptions();
    llassert(options.getAxisOptions().size() == LLGameControl::NUM_AXES);
    for (U8 i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        rows[i]->getColumn(1)->setValue(false);          // Invert = false
        rows[i]->getColumn(2)->setValue(LLStringUtil::null); // Deadzone = 0
        rows[i]->getColumn(3)->setValue(LLStringUtil::null); // Offset = 0
        options.getAxisOptions()[i].resetToDefaults();
    }
}

// Resets axis remapping to identity (each axis maps to itself) for selected device.
void LLPanelPreferenceGameControl::resetAxisMappingsToDefaults()
{
    std::vector<LLScrollListItem*> rows = mAxisMappings->getAllData();
    llassert(rows.size() == LLGameControl::NUM_AXES);
    LLGameControl::Options& options = getSelectedDeviceOptions();
    llassert(options.getAxisMap().size() == LLGameControl::NUM_AXES);
    for (U8 i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        rows[i]->getColumn(1)->setValue(LLStringUtil::null);
        options.getAxisMap()[i] = i;  // Identity mapping
    }
}

// Resets button remapping to identity (each button maps to itself) for selected device.
void LLPanelPreferenceGameControl::resetButtonMappingsToDefaults()
{
    std::vector<LLScrollListItem*> rows = mButtonMappings->getAllData();
    llassert(rows.size() == LLGameControl::NUM_BUTTONS);
    LLGameControl::Options& options = getSelectedDeviceOptions();
    llassert(options.getButtonMap().size() == LLGameControl::NUM_BUTTONS);
    for (U8 i = 0; i < LLGameControl::NUM_BUTTONS; ++i)
    {
        rows[i]->getColumn(1)->setValue(LLStringUtil::null);
        options.getButtonMap()[i] = i;  // Identity mapping
    }
}

// Captures current settings values into mOrigSettings for later restoration upon cancel().
void LLPanelPreferenceGameControl::rememberOriginalSettings()
{
    mOrigSettings = LLGameControl::getSettingsAsLLSD();
}
