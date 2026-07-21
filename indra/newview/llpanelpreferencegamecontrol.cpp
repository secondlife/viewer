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

    // The combobox popup currently overlaying sSelectedCell (one of the four
    // action-mapping/output selectors below).  Stashed so applyGameControlInput()
    // can drive it (and reuse onCommitInputChannel()) without having to re-derive
    // which selector initCombobox() chose.
    static LLComboBox* sSelectedCombobox { nullptr };

    // The selector combobox overlays the edited cell, but its drop-down button
    // texture is semi-transparent, so the cell's own text shows through and
    // "doubles" with the selector's label.  While a selector is open we blank
    // the cell's text and stash its value here, restoring it when the selector
    // closes (see initCombobox / clearSelectionState).
    static LLSD sSelectedCellValue;

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

    // PromptFont glyph(s) for a canonical controller input value.
    // Codepoints are from the PromptFont package's promptfont.h, preferring the
    // generic vendor-neutral "gamepad"/"dpad"/"analog" sets and falling back to
    // the Xbox set for shoulders/triggers (no generic equivalents exist).
    // Rendered with the "PromptFont" family declared in fonts.xml.  Inputs with
    // no entry (e.g. BUTTON_21..31, *_NONE) return empty and keep their text label.
    std::string promptFontGlyph(const std::string& input_value)
    {
        static const std::map<std::string, LLWString> sGlyphs = {
            // Face buttons
            { "BUTTON_SOUTH", { 0x21A7 } },  // PF_GAMEPAD_A (bottom)
            { "BUTTON_EAST",  { 0x21A6 } },  // PF_GAMEPAD_B (right)
            { "BUTTON_WEST",  { 0x21A4 } },  // PF_GAMEPAD_X (left)
            { "BUTTON_NORTH", { 0x21A5 } },  // PF_GAMEPAD_Y (top)
            // D-pad
            { "BUTTON_DPAD_UP",    { 0x219F } },  // PF_DPAD_UP
            { "BUTTON_DPAD_DOWN",  { 0x21A1 } },  // PF_DPAD_DOWN
            { "BUTTON_DPAD_LEFT",  { 0x219E } },  // PF_DPAD_LEFT
            { "BUTTON_DPAD_RIGHT", { 0x21A0 } },  // PF_DPAD_RIGHT
            // Shoulders / stick clicks
            { "BUTTON_LEFT_SHOULDER",  { 0x2198 } },  // PF_XBOX_LEFT_SHOULDER
            { "BUTTON_RIGHT_SHOULDER", { 0x2199 } },  // PF_XBOX_RIGHT_SHOULDER
            { "BUTTON_LEFT_STICK",     { 0x21BA } },  // PF_ANALOG_L_CLICK (L3)
            { "BUTTON_RIGHT_STICK",    { 0x21BB } },  // PF_ANALOG_R_CLICK (R3)
            // Menu cluster
            { "BUTTON_SELECT", { 0x21F7 } },  // PF_GAMEPAD_SELECT (menu left)
            { "BUTTON_HOME",   { 0x21F9 } },  // PF_GAMEPAD_HOME   (menu middle)
            { "BUTTON_START",  { 0x21F8 } },  // PF_GAMEPAD_START  (menu right)
            // Misc / paddles / touchpad
            { "BUTTON_15",       { 0x2212 } },  // PF_GAMEPAD_M1 (Misc1)
            { "BUTTON_PADDLE1",  { 0x2277 } },  // PF_GAMEPAD_R4
            { "BUTTON_PADDLE2",  { 0x2276 } },  // PF_GAMEPAD_L4
            { "BUTTON_PADDLE3",  { 0x2279 } },  // PF_GAMEPAD_R5
            { "BUTTON_PADDLE4",  { 0x2278 } },  // PF_GAMEPAD_L5
            { "BUTTON_TOUCHPAD", { 0x21E7 } },  // PF_SONY_TOUCHPAD
            // Axes
            { "AXIS_LEFTX",  { 0x21C4 } },  // PF_ANALOG_L_LEFT_RIGHT
            { "AXIS_LEFTY",  { 0x21C5 } },  // PF_ANALOG_L_UP_DOWN
            { "AXIS_RIGHTX", { 0x21C6 } },  // PF_ANALOG_R_LEFT_RIGHT
            { "AXIS_RIGHTY", { 0x21F5 } },  // PF_ANALOG_R_UP_DOWN
            { "AXIS_LEFT_TRIGGER",  { 0x2196 } },  // PF_XBOX_LEFT_TRIGGER
            { "AXIS_RIGHT_TRIGGER", { 0x2197 } },  // PF_XBOX_RIGHT_TRIGGER
            // Combined trigger pair: show both trigger glyphs side by side.
            { "AXIS_TRIGGERS", { 0x2196, 0x2197 } },
        };
        auto it = sGlyphs.find(input_value);
        if (it == sGlyphs.end())
        {
            return LLStringUtil::null;
        }
        return wstring_to_utf8str(it->second);
    }

    // The PromptFont glyphs live on real Unicode arrow codepoints, so they must be
    // drawn with the dedicated PromptFont family rather than the default text font.
    // Sized larger than the SansSerif rows so the icons read clearly.
    const LLFontGL* promptFontForCell()
    {
        return LLFontGL::getFont(LLFontDescriptor("PromptFont", "Huge", 0));
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

    // Deselect everything in the other scroll_list
    LLScrollListCtrl* sibling = (table == mActionMappingsAxes) ? mActionMappingsButtons : mActionMappingsAxes;
    sibling->deselectAllItems(true);

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
        // Two editable columns map to the same input: the PromptFont icon column
        // (1) opens the glyph selector, the text description column (2) opens the
        // text selector.  Both commit through onCommitInputChannel, which rebuilds
        // the row so the icon and description update together.
        LLSD row_value = item->getValue();
        if (!row_value.isMap())
            return false;
        bool axes = (grid == mActionMappingsAxes);
        S32 sel = item->getSelectedCell();
        if (sel == 1)
        {
            combobox = axes ? mAxisInputGlyphSelector : mButtonInputGlyphSelector;
        }
        else if (sel == 2)
        {
            combobox = axes ? mAxisInputSelector : mButtonInputSelector;
        }
        else
        {
            return false;
        }
        col = sel;
    }
    else if (grid == mAxisChannels)
    {
        // Two editable columns map to the same output: the PromptFont icon column
        // ("output") opens the glyph selector, the text column ("output_description")
        // opens the text selector.  Both commit through onCommitInputChannel, which
        // refreshes both cells so the icon and description stay in sync.  The Input
        // columns are the fixed physical axis and are not editable.
        S32 sel = item->getSelectedCell();
        S32 desc_col  = grid->getColumn("output_description")->mIndex;
        S32 glyph_col = grid->getColumn("output")->mIndex;
        if (sel == glyph_col)
        {
            combobox = mAxisOutputGlyphSelector;
        }
        else if (sel == desc_col)
        {
            combobox = mAxisOutputSelector;
        }
        else
        {
            return false;
        }
        col = sel;
    }
    else if (grid == mButtonChannels)
    {
        // Two editable columns map to the same output: the PromptFont icon column
        // ("output") opens the glyph selector, the text column ("output_description")
        // opens the text selector.  Both commit through onCommitInputChannel, which
        // refreshes both cells so the icon and description stay in sync.  The Input
        // columns are the fixed physical button and are not editable.
        S32 sel = item->getSelectedCell();
        S32 desc_col  = grid->getColumn("output_description")->mIndex;
        S32 glyph_col = grid->getColumn("output")->mIndex;
        if (sel == glyph_col)
        {
            combobox = mButtonInputGlyphSelector;
        }
        else if (sel == desc_col)
        {
            combobox = mButtonInputSelector;
        }
        else
        {
            return false;
        }
        col = sel;
    }

    if (!combobox)
        return false;

    LLScrollListText* cell = dynamic_cast<LLScrollListText*>(item->getColumn(col));
    if (!cell)
        return false;

    // compute new rect for combobox
    S32 row_index = grid->getItemIndex(item);
    fitInRect(combobox, grid, row_index, col);

    // Pre-select the item matching the cell's current input.
    std::string value;
    if (grid == mActionMappingsAxes || grid == mActionMappingsButtons)
    {
        // Use the action's stored mapping value directly, so pre-selection is
        // correct even when the icon cell is blank (input has no glyph).
        LLSD row_value = item->getValue();
        LLSD mapping = LLGameControl::getModeMapping(currentEditMode(), row_value["kind"].asString());
        std::string action = row_value["action"].asString();
        value = mapping.has(action) ? mapping[action].asString() : LLStringUtil::null;
    }
    else
    {
        // Device tables: match the cell's current text against the selector items
        // (works regardless of the value naming scheme).
        std::string cell_value = cell->getValue();
        for (const LLScrollListItem* combo_item : combobox->getAllData())
        {
            if (combo_item->getColumn(0)->getValue().asString() == cell_value)
            {
                value = combo_item->getValue().asString();
                break;
            }
        }
    }
    if (value.empty())
    {
        // No match (e.g. a blank "None" cell): select the last item, which is "None".
        value = combobox->getAllData().back()->getValue().asString();
    }

    combobox->setValue(value);
    combobox->setVisible(true);
    combobox->showList();

    sSelectedGrid = grid;
    sSelectedItem = item;
    sSelectedCell = cell;
    sSelectedCombobox = combobox;

    // Hide the cell's text under the (translucent) selector so it doesn't
    // double with the selector's label; clearSelectionState() restores it.
    sSelectedCellValue = cell->getValue();
    cell->setValue(LLSD());

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

    // Editing an action mapping can change whether the input cell shows a text
    // label or a PromptFont glyph, which means recreating the cell with the right
    // font; remember to rebuild the action tables once the commit is applied.
    bool action_grid_edited = (sSelectedGrid == mActionMappingsAxes || sSelectedGrid == mActionMappingsButtons);
    // Editing an axis- or button-channels output likewise changes both its text and
    // icon cells, so both are refreshed from the device map once the commit is applied.
    bool axis_channels_edited = (sSelectedGrid == mAxisChannels);
    bool button_channels_edited = (sSelectedGrid == mButtonChannels);

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
        // The row is a physical axis; the selected item's value names the output it
        // maps to: an individual canonical axis, "Triggers left/right" (the fan-out
        // pair), or "None".  Output codes are resolved by LLGameControl so the panel
        // stays agnostic of the axis-map encoding.
        S32 axis = mAxisChannels->getItemIndex(sSelectedItem);
        std::string output_name = combobox->getValue().asString();
        LLGameControl::Options& options = getSelectedDeviceOptions();
        options.getAxisMap()[axis] = LLGameControl::axisOutputFromName(output_name);
        LLGameControl::setDeviceOptions(mSelectedDeviceGUID, options);
        // Both output cells (icon + description) are re-rendered from the actual
        // map by populateAxisChannelsCells() below.
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
        // Both output cells (icon + description) are re-rendered from the actual
        // map by populateButtonChannelsCells() below.
    }

    // The branches above set the cell to its committed value; keep the
    // restore-on-close value in sync so clearSelectionState() doesn't revert
    // the cell back to its pre-edit text.
    sSelectedCellValue = sSelectedCell->getValue();

    sSelectedGrid->deselectAllItems();
    clearSelectionState();

    // Rebuild the action tables so a newly bound (or unbound) button action is
    // re-rendered with the correct cell font -- text label vs PromptFont glyph.
    if (action_grid_edited)
    {
        populateActionMappings();
    }

    // Refresh the axis-/button-channels output cells so the committed output's text
    // label and PromptFont glyph are both re-rendered from the device map.
    if (axis_channels_edited)
    {
        populateAxisChannelsCells();
    }
    if (button_channels_edited)
    {
        populateButtonChannelsCells();
    }
}

// Returns true if a cell is currently selected and waiting for input channel assignment.
// Used to determine whether to capture live controller input.
bool LLPanelPreferenceGameControl::isWaitingForInputChannel()
{
    return sSelectedCell != nullptr;
}

// Static method called in the mainloop when an Action Mappings input selector is open.
// Refreshes the canonical controller state, and if the user is pressing a button or
// tilting an axis that matches the kind of selector open (button vs. axis), assigns
// it to the selected cell as if the user had picked it from the dropdown.
void LLPanelPreferenceGameControl::applyGameControlInput()
{
    if (!sGameControlPanel || !sSelectedCombobox)
        return;

    bool wants_axis = (sSelectedGrid == sGameControlPanel->mActionMappingsAxes);
    bool wants_button = (sSelectedGrid == sGameControlPanel->mActionMappingsButtons);
    if (!wants_axis && !wants_button)
        return;

    // Recompute the canonical controller state from the live device input without
    // triggering the normal "send to server" bookkeeping (see computeFinalStateAndCheckForChanges()),
    // since input used to configure a mapping should not drive the avatar or the network.
    LLGameControl::computeFinalState();

    LLGameControl::InputChannel channel = LLGameControl::getActiveInputChannel();
    if (wants_axis && !channel.isAxis())
        return;
    if (wants_button && !channel.isButton())
        return;

    std::string value = channel.getRemoteName();
    if (!sSelectedCombobox->setSelectedByValue(value, true))
    {
        // The action-mapping axis selector has no entries for the individual
        // trigger axes: it collapses them into one virtual "Triggers" item
        // (see AXIS_TRIGGERS in panel_preferences_game_control.xml).
        if (value != "AXIS_LEFT_TRIGGER" && value != "AXIS_RIGHT_TRIGGER")
            return;
        if (!sSelectedCombobox->setSelectedByValue(std::string("AXIS_TRIGGERS"), true))
            return;
    }

    sGameControlPanel->onCommitInputChannel(sSelectedCombobox);
}

// Handles selection in the axis-channels table (invert, deadzone, offset, output).
// Syncs the invert flag immediately; shows the numeric editor for deadzone/offset,
// or the axis selector popup for the Output column.
void LLPanelPreferenceGameControl::onAxisChannelsSelect()
{
    clearSelectionState();

    // Deselect everything in the other scroll_list
    mButtonChannels->deselectAllItems(true);

    if (LLScrollListItem* row = mAxisChannels->getFirstSelected())
    {
        // Only the Output columns (icon + description) are editable; initCombobox
        // ignores the fixed Input columns.  Axis tuning (invert/offset/dead zone)
        // now lives on the Device State tab.
        initCombobox(row, mAxisChannels);
    }
}

// Handles selection in the (now editable) axis-state table on the Device State
// tab.  Syncs the Invert checkbox immediately and shows the numeric editor for the
// Offset/Dead Zone columns.  Edits target the state tab's selected device.
void LLPanelPreferenceGameControl::onAxisStateSelect()
{
    clearSelectionState();

    // Deselect everything in the other scroll_list
    mButtonState->deselectAllItems(true);

    auto options_it = mDeviceOptions.find(mStateSelectedDeviceGUID);
    if (options_it == mDeviceOptions.end())
    {
        return;
    }
    LLGameControl::Options& deviceOptions = options_it->second.options;

    if (LLScrollListItem* row = mAxisState->getFirstSelected())
    {
        S32 row_index = mAxisState->getItemIndex(row);

        {
            // Always sync invert checkbox - clicking the checkbox selects the row
            // but doesn't automatically update the underlying option.
            S32 invert_checkbox_column = axisStateColumn("invert");
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

                    if (LLScrollListItem* other_row = mAxisState->getItemByIndex(other_row_index))
                    {
                        other_row->getColumn(invert_checkbox_column)->setValue(invert);
                    }
                }

                LLGameControl::setDeviceOptions(mStateSelectedDeviceGUID, deviceOptions);
            }
        }

        S32 offset_col    = axisStateColumn("offset");
        S32 dead_zone_col = axisStateColumn("dead_zone");
        S32 column_index = row->getSelectedCell();
        if (column_index == offset_col || column_index == dead_zone_col)
        {
            fitInRect(mNumericValueEditor, mAxisState, row_index, column_index);
            if (column_index == dead_zone_col)  // Dead Zone
            {
                mNumericValueEditor->setMinValue(0);
                mNumericValueEditor->setMaxValue(LLGameControl::MAX_AXIS_DEAD_ZONE);
                mNumericValueEditor->setValue(deviceOptions.getAxisOptions()[row_index].mDeadZone);
            }
            else  // Offset
            {
                mNumericValueEditor->setMinValue(-LLGameControl::MAX_AXIS_OFFSET);
                mNumericValueEditor->setMaxValue(LLGameControl::MAX_AXIS_OFFSET);
                mNumericValueEditor->setValue(deviceOptions.getAxisOptions()[row_index].mOffset);
            }
            mNumericValueEditor->setVisible(true);
        }
    }
}

// Handles selection in the button-channels table.  Only the Output columns (icon +
// description) use a popup selector; the Input columns are fixed (the physical button).
void LLPanelPreferenceGameControl::onButtonChannelsSelect()
{
    clearSelectionState();

    // Deselect everything in the other scroll_list
    mAxisChannels->deselectAllItems(true);

    if (LLScrollListItem* row = mButtonChannels->getFirstSelected())
    {
        initCombobox(row, mButtonChannels);
    }
}

// Called when user commits a numeric value (offset or dead zone) in the spin
// control over the Device State tab's axis table.  Validates and clamps the value,
// then updates both the UI and the state device's options.
void LLPanelPreferenceGameControl::onCommitNumericValue()
{
    auto options_it = mDeviceOptions.find(mStateSelectedDeviceGUID);
    if (options_it == mDeviceOptions.end())
    {
        return;
    }
    LLGameControl::Options& deviceOptions = options_it->second.options;

    if (LLScrollListItem* row = mAxisState->getFirstSelected())
    {
        S32 value = mNumericValueEditor->getValue().asInteger();
        S32 row_index = mAxisState->getItemIndex(row);
        S32 offset_col    = axisStateColumn("offset");
        S32 dead_zone_col = axisStateColumn("dead_zone");
        S32 column_index = row->getSelectedCell();
        llassert(column_index == offset_col || column_index == dead_zone_col);
        if (column_index != offset_col && column_index != dead_zone_col)
            return;

        if (column_index == dead_zone_col)  // Dead Zone
        {
            value = std::clamp<S32>(value, 0, LLGameControl::MAX_AXIS_DEAD_ZONE);
            deviceOptions.getAxisOptions()[row_index].mDeadZone = (U16)value;
        }
        else  // Offset
        {
            value = std::clamp<S32>(value, -LLGameControl::MAX_AXIS_OFFSET, LLGameControl::MAX_AXIS_OFFSET);
            deviceOptions.getAxisOptions()[row_index].mOffset = (S16)value;
        }
        setNumericLabel(row->getColumn(column_index), value);
        LLGameControl::setDeviceOptions(mStateSelectedDeviceGUID, deviceOptions);
    }
}

// Initializes all UI controls and sets up callbacks.
// Called once when the panel is first built from XML.
bool LLPanelPreferenceGameControl::postBuild()
{
    // Master enable checkbox (top-left of the main panel): runtime on/off for all
    // game-control -> action logic and for sending GameControlData to the server.
    mCheckGameControlEnabled = getChild<LLCheckBoxCtrl>("game_control_enabled");
    mCheckGameControlEnabled->setCommitCallback([this](LLUICtrl*, const LLSD&)
        {
            LLGameControl::setGameControlEnabled(mCheckGameControlEnabled->getValue());
        });

    // Send-to-server checkbox (top-right of the main panel)
    mCheckGameControlToServer = getChild<LLCheckBoxCtrl>("game_control_to_server");
    mCheckGameControlToServer->setCommitCallback([this](LLUICtrl*, const LLSD&)
        {
            LLGameControl::setSendToServer(mCheckGameControlToServer->getValue());
        });

    getChild<LLTabContainer>("game_control_tabs")->setCommitCallback([this](LLUICtrl*, const LLSD&) { clearSelectionState(); });

    mTabActions = getChild<LLPanel>("tab_action_mappings");
    mTabDevices = getChild<LLPanel>("tab_device_mappings");

    // Actions tab (global, per-mode)
    mActionMode = getChild<LLScrollListCtrl>("action_mode");
    mActionMode->setCommitCallback([this](LLUICtrl*, const LLSD&) { onActionModeChanged(); });
    populateActionModeList();

    mRestoreActionsDefaults = getChild<LLButton>("restore_actions_defaults");
    mRestoreActionsDefaults->setCommitCallback([this](LLUICtrl*, const LLSD&) { onResetActionsToDefaults(); });

    mActionMappingsAxes = getChild<LLScrollListCtrl>("action_mappings_axes");
    mActionMappingsAxes->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onGridSelect(ctrl); });

    mActionMappingsButtons = getChild<LLScrollListCtrl>("action_mappings_buttons");
    mActionMappingsButtons->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onGridSelect(ctrl); });

    // Inputs tab (per-device)
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

    // Outputs tab (per device)
    mTabDeviceState = getChild<LLPanel>("tab_device_options");
    mStateNoDeviceMessage = getChild<LLTextBox>("state_nodevice_message");
    mStateDevicePrompt = getChild<LLTextBox>("state_device_prompt");
    mStateRemapNote = getChild<LLTextBox>("state_remap_note");
    mStateDeviceList = getChild<LLComboBox>("options_device_list");
    mPanelDeviceState = getChild<LLPanel>("device_state_settings");
    mStateDeviceList->setCommitCallback([this](LLUICtrl*, const LLSD& value)
        {
            mStateSelectedDeviceGUID = value.asString();
            clearSelectionState();
            populateAxisStateOptionCells();  // refresh invert/offset/dead-zone for the new device
        });

    mRestoreDeviceOptionsDefaults = getChild<LLButton>("restore_device_options_defaults");
    mRestoreDeviceOptionsDefaults->setCommitCallback([this](LLUICtrl*, const LLSD&) { onResetDeviceOptionsToDefaults(); });

    mAxisState = getChild<LLScrollListCtrl>("axis_state");
    // The axis-state table is now editable: its Invert/Offset/Dead Zone columns
    // tune the state device's axis options while its Raw/Adjusted columns update live.
    mAxisState->setCommitCallback([this](LLUICtrl*, const LLSD&) { onAxisStateSelect(); });
    mButtonState = getChild<LLScrollListCtrl>("button_state");

    // Data Output tab (live, read-only view of the last outgoing GameControlInput)
    mTabDataOutput = getChild<LLPanel>("tab_server_data");
    mDataOutput = getChild<LLScrollListCtrl>("data_output");

    // Spin control for editing deadzone/offset values inline
    mNumericValueEditor = getChild<LLSpinCtrl>("numeric_value_editor");
    mNumericValueEditor->setCommitCallback([this](LLUICtrl*, const LLSD&) { onCommitNumericValue(); });

    // Action selectors: provide the "Action" column rows, chosen by edit mode.
    mAnalogActionSelector = getChild<LLComboBox>("analog_action_selector");
    mBinaryActionSelector = getChild<LLComboBox>("binary_action_selector");
    mFlycamAnalogActionSelector = getChild<LLComboBox>("flycam_analog_action_selector");
    mFlycamBinaryActionSelector = getChild<LLComboBox>("flycam_binary_action_selector");
    mCaptiveBinaryActionSelector = getChild<LLComboBox>("sit_binary_action_selector");

    // Canonical input selectors, shown inline when editing an Input/Output cell.
    // The Actions tab binds an axis action to a canonical input (sticks or the
    // "Triggers left/right" pair) via mAxisInputSelector; the Devices tab maps a
    // physical axis to an individual canonical axis via mAxisOutputSelector.
    mAxisInputSelector = getChild<LLComboBox>("axis_input_selector");
    mAxisInputSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });

    mAxisOutputSelector = getChild<LLComboBox>("axis_output_selector");
    mAxisOutputSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });

    // Glyph counterpart of mAxisOutputSelector, shown when editing the PromptFont
    // "output" (icon) column of the axis-channels table.  Same values, rendered as
    // PromptFont glyphs, so build it from the text selector's values.
    mAxisOutputGlyphSelector = getChild<LLComboBox>("axis_output_glyph_selector");
    mAxisOutputGlyphSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });
    buildInputGlyphSelector(mAxisOutputSelector, mAxisOutputGlyphSelector);

    mButtonInputSelector = getChild<LLComboBox>("button_input_selector");
    mButtonInputSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });

    // Glyph selectors shown when editing the PromptFont "Axis"/"Button" column.
    // Their items mirror the text selectors' values, so build them from those.
    mAxisInputGlyphSelector = getChild<LLComboBox>("axis_input_glyph_selector");
    mAxisInputGlyphSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });
    buildInputGlyphSelector(mAxisInputSelector, mAxisInputGlyphSelector);

    mButtonInputGlyphSelector = getChild<LLComboBox>("button_input_glyph_selector");
    mButtonInputGlyphSelector->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&) { onCommitInputChannel(ctrl); });
    buildInputGlyphSelector(mButtonInputSelector, mButtonInputGlyphSelector);

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
    fixHeadingTop(mActionMode);
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
    // Sync checkboxes with current LLGameControl state
    mCheckGameControlEnabled->setValue(LLGameControl::getGameControlEnabled());
    mCheckGameControlToServer->setValue(LLGameControl::sendToServer());

    clearSelectionState();

    // Default to editing the Avatar mode's mappings each time the panel opens
    // (Avatar is the first action_mode item).
    mActionMode->selectFirstItem();
    populateActionMappings();

    // Refresh device list and settings
    updateDeviceListInternal();

    mCheckGameControlEnabled->setEnabled(true);
    mCheckGameControlToServer->setEnabled(true);
    mActionMappingsAxes->setEnabled(true);
    mActionMappingsButtons->setEnabled(true);
    mAxisChannels->setEnabled(true);
    mButtonChannels->setEnabled(true);
    mDeviceList->setEnabled(true);
    mStateDeviceList->setEnabled(true);

    // Apply the current mode's enable flag (must run after the blanket enables
    // above): syncs the checkbox and locks the tables if the mode is disabled.
    updateActionModeEnabledUI();

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
    // Avatar and Captive share the same axis actions (see llgamecontrol.cpp);
    // only their button actions differ (Captive swaps in e.g. "Stand" for "Jump").
    if (axis)
        return flycam ? mFlycamAnalogActionSelector : mAnalogActionSelector;
    if (flycam)
        return mFlycamBinaryActionSelector;
    return (mode == "Captive") ? mCaptiveBinaryActionSelector : mBinaryActionSelector;
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

            // Current input bound to this action, and the PromptFont glyph to
            // show instead of the text label, if this input has one.
            std::string input_value = mapping.has(action) ? mapping[action].asString() : LLStringUtil::null;
            std::string glyph = promptFontGlyph(input_value);

            LLScrollListItem::Params row_params;
            for (S32 c = 0; c < grid->getNumColumns(); ++c)
            {
                LLScrollListCell::Params cell_params;
                cell_params.column = grid->getColumn(c)->mName;
                // The icon column (1) renders the controller glyph in PromptFont
                // when this input has one; the action (0) and description (2)
                // columns match the "Controls" preferences panel font.
                cell_params.font = (c == 1 && !glyph.empty())
                    ? promptFontForCell()
                    : LLFontGL::getFontSansSerif();
                row_params.columns.add(cell_params);
            }

            // Row value carries the block kind and action key (used on commit/lookup).
            LLSD row_value;
            row_value["kind"] = kind;
            row_value["action"] = action;
            row_params.value = row_value;

            LLScrollListItem* row = grid->addRow(row_params);
            row->getColumn(0)->setValue(item->getColumn(0)->getValue());  // action label
            // Columns 1 (icon) and 2 (description) both derive from input_value, so
            // they always refer to the same controller input.
            row->getColumn(1)->setValue(glyph.empty() ? LLSD() : LLSD(glyph));
            row->getColumn(2)->setValue(blankIfNone(inputLabel(input_selector, input_value)));
        }
    };

    addBlock(mActionMappingsAxes, KIND_AXES, actionSelectorForMode(true, mode), mAxisInputSelector);
    addBlock(mActionMappingsButtons, KIND_BUTTONS, actionSelectorForMode(false, mode), mButtonInputSelector);
}

// Builds the mode-selector rows: one per action mode, each with an Enabled
// checkbox and the mode's display label.  The row value is the mode ordinal
// (matching LLGameControl::AgentControlMode) used by currentEditMode().
void LLPanelPreferenceGameControl::populateActionModeList()
{
    mActionMode->clearRows();

    struct ModeRow { LLGameControl::AgentControlMode mode; const char* label; };
    static const ModeRow rows[] = {
        { LLGameControl::CONTROL_MODE_AVATAR,  "When moving avatar" },
        { LLGameControl::CONTROL_MODE_FLYCAM,  "When moving flycam" },
        { LLGameControl::CONTROL_MODE_CAPTIVE, "When sitting"       },
    };

    for (const ModeRow& mr : rows)
    {
        std::string mode = LLGameControl::getModeName(mr.mode);

        LLScrollListItem::Params row_params;
        row_params.value = (S32)mr.mode;

        LLScrollListCell::Params check_cell;
        check_cell.column = "enabled";
        check_cell.type = "checkbox";
        check_cell.value = LLGameControl::isModeEnabled(mode);
        row_params.columns.add(check_cell);

        LLScrollListCell::Params label_cell;
        label_cell.column = "mode";
        label_cell.font = LLFontGL::getFontSansSerif();
        label_cell.value = std::string(mr.label);
        row_params.columns.add(label_cell);

        LLScrollListItem* row = mActionMode->addRow(row_params);

        // Wire the per-row checkbox so toggling it enables/disables that mode
        // independently of which row is currently selected for editing.
        if (auto* check = dynamic_cast<LLScrollListCheck*>(row->getColumn(0)))
        {
            S32 ordinal = (S32)mr.mode;
            check->getCheckBox()->setCommitCallback(
                [this, ordinal](LLUICtrl* ctrl, const LLSD&)
                { onModeEnabledToggled(ordinal, ctrl->getValue().asBoolean()); });
        }
    }
}

// Handles a change in the edit-mode selector: rebuild the action table.
void LLPanelPreferenceGameControl::onActionModeChanged()
{
    clearSelectionState();
    populateActionMappings();
    updateActionModeEnabledUI();
}

// Toggle whether game-control input is converted to one mode's actions.
void LLPanelPreferenceGameControl::onModeEnabledToggled(S32 mode_ordinal, bool enabled)
{
    clearSelectionState();
    LLGameControl::setModeEnabled(
        LLGameControl::getModeName((LLGameControl::AgentControlMode)mode_ordinal), enabled);
    // If the toggled mode is the one being edited, lock/unlock its tables now.
    updateActionModeEnabledUI();
}

// Sync each mode row's checkbox with its stored flag (keeps the list correct
// after apply/cancel/reset) and lock/unlock the action tables (and Restore
// Defaults) for the mode being edited so a disabled mode's mappings can't change.
void LLPanelPreferenceGameControl::updateActionModeEnabledUI()
{
    for (LLScrollListItem* item : mActionMode->getAllData())
    {
        std::string mode = LLGameControl::getModeName(
            (LLGameControl::AgentControlMode)item->getValue().asInteger());
        if (auto* check = dynamic_cast<LLScrollListCheck*>(item->getColumn(0)))
        {
            check->getCheckBox()->set(LLGameControl::isModeEnabled(mode));
        }
    }

    bool enabled = LLGameControl::isModeEnabled(currentEditMode());
    // A disabled mode's tables grey out; onGridSelect() also refuses edits on a
    // disabled table, so this both signals and enforces the lock.
    mActionMappingsAxes->setEnabled(enabled);
    mActionMappingsButtons->setEnabled(enabled);
    mRestoreActionsDefaults->setEnabled(enabled);
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

// Creates one row per physical axis in the axis-channels table.  Columns:
// input(glyph) | input_description | output_description | output(glyph).  The two
// Input columns are the fixed physical axis (icon + text); the two Output columns
// are the canonical axis it maps to (icon + text), edited via popup selectors.
// Axis tuning (invert/offset/dead zone) lives on the Device State tab.
void LLPanelPreferenceGameControl::populateAxisChannelsRows()
{
    mAxisChannels->clearRows();

    // The physical controller axes are the output selector's first NUM_AXES items,
    // in order (item N == canonical axis N).
    std::vector<LLScrollListItem*> items = mAxisOutputSelector->getAllData();

    S32 input_glyph_col = mAxisChannels->getColumn("input")->mIndex;
    S32 input_desc_col  = mAxisChannels->getColumn("input_description")->mIndex;

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    for (S32 i = 0; i < (S32)(mAxisChannels->getNumColumns()); ++i)
    {
        const std::string& name = mAxisChannels->getColumn(i)->mName;
        // The icon columns ("input"/"output") render the controller glyph in
        // PromptFont; the description columns match the "Controls" panel font.
        cell_params.font = (name == "input" || name == "output")
            ? promptFontForCell()
            : LLFontGL::getFontSansSerif();
        cell_params.column = name;
        row_params.columns.add(cell_params);
    }

    for (size_t i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        LLScrollListItem* row = mAxisChannels->addRow(row_params);
        // The physical axis is fixed: fill both its text label and its glyph.
        std::string glyph = promptFontGlyph(items[i]->getValue().asString());
        row->getColumn(input_desc_col)->setValue(items[i]->getColumn(0)->getValue());
        row->getColumn(input_glyph_col)->setValue(glyph.empty() ? LLSD() : LLSD(glyph));
    }
}

// Fills the axis-channel Output cells (text description + PromptFont glyph) from
// the current device's axis map.
void LLPanelPreferenceGameControl::populateAxisChannelsCells()
{
    std::vector<LLScrollListItem*> rows = mAxisChannels->getAllData();
    const LLGameControl::Options& options = getSelectedDeviceOptions();
    const auto& axis_map = options.getAxisMap();

    S32 output_desc_col  = mAxisChannels->getColumn("output_description")->mIndex;
    S32 output_glyph_col = mAxisChannels->getColumn("output")->mIndex;

    for (size_t i = 0; i < rows.size() && i < axis_map.size(); ++i)
    {
        std::string output_name = LLGameControl::axisOutputName(axis_map[i]);
        std::string glyph = promptFontGlyph(output_name);
        rows[i]->getColumn(output_desc_col)->setValue(inputLabel(mAxisOutputSelector, output_name));
        rows[i]->getColumn(output_glyph_col)->setValue(glyph.empty() ? LLSD() : LLSD(glyph));
    }
}

// Creates one row per physical button in the button-channels table.  Columns:
// input(glyph) | input_description | output_description | output(glyph).  The two
// Input columns are the fixed physical button (icon + text); the two Output columns
// are the canonical button it maps to (icon + text), edited via popup selectors.
void LLPanelPreferenceGameControl::populateButtonChannelsRows()
{
    mButtonChannels->clearRows();

    std::vector<LLScrollListItem*> items = mButtonInputSelector->getAllData();

    S32 input_glyph_col = mButtonChannels->getColumn("input")->mIndex;
    S32 input_desc_col  = mButtonChannels->getColumn("input_description")->mIndex;

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    for (S32 i = 0; i < (S32)(mButtonChannels->getNumColumns()); ++i)
    {
        const std::string& name = mButtonChannels->getColumn(i)->mName;
        // The icon columns ("input"/"output") render the controller glyph in
        // PromptFont; the description columns match the "Controls" panel font.
        cell_params.font = (name == "input" || name == "output")
            ? promptFontForCell()
            : LLFontGL::getFontSansSerif();
        cell_params.column = name;
        row_params.columns.add(cell_params);
    }

    for (size_t i = 0; i < LLGameControl::NUM_BUTTONS; ++i)
    {
        LLScrollListItem* row = mButtonChannels->addRow(row_params);
        // The physical button is fixed: fill both its text label and its glyph
        // (buttons without a glyph, e.g. spares, keep an empty icon cell).
        std::string glyph = promptFontGlyph(items[i]->getValue().asString());
        row->getColumn(input_desc_col)->setValue(items[i]->getColumn(0)->getValue());
        row->getColumn(input_glyph_col)->setValue(glyph.empty() ? LLSD() : LLSD(glyph));
    }
}

// Fills the button-channel Output cells (text description + PromptFont glyph) from
// the current device's button map.
void LLPanelPreferenceGameControl::populateButtonChannelsCells()
{
    std::vector<LLScrollListItem*> rows = mButtonChannels->getAllData();
    const auto& button_map = getSelectedDeviceOptions().getButtonMap();
    std::vector<LLScrollListItem*> items = mButtonInputSelector->getAllData();
    llassert(rows.size() == button_map.size());

    S32 output_desc_col  = mButtonChannels->getColumn("output_description")->mIndex;
    S32 output_glyph_col = mButtonChannels->getColumn("output")->mIndex;

    for (size_t i = 0; i < rows.size(); ++i)
    {
        U8 output = button_map[i];
        rows[i]->getColumn(output_desc_col)->setValue(selectorLabelAt(mButtonInputSelector, output));
        std::string value = (output < items.size()) ? items[output]->getValue().asString() : LLStringUtil::null;
        std::string glyph = promptFontGlyph(value);
        rows[i]->getColumn(output_glyph_col)->setValue(glyph.empty() ? LLSD() : LLSD(glyph));
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
    mRestoreDeviceOptionsDefaults->setVisible(hasDevice);
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

    // Refresh the editable axis option cells for the (re)selected state device.
    populateAxisStateOptionCells();
}

// Creates one row per physical axis in the axis-state table.  Columns:
// Axis-state table columns (input, raw_value, invert, offset, dead_zone,
// fixed_value); their display order is defined entirely by the XML.  Raw/Adjusted
// are filled live in draw(); Invert/Offset/Dead Zone are editable axis options.
// All cells are addressed by column name (see axisStateColumn), so reordering the
// columns in the XML requires no change here.
void LLPanelPreferenceGameControl::populateAxisStateRows()
{
    mAxisState->clearRows();

    // One row per physical axis, labelled from the output selector's canonical-axis items.
    std::vector<LLScrollListItem*> items = mAxisOutputSelector->getAllData();

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    for (S32 i = 0; i < (S32)(mAxisState->getNumColumns()); ++i)
    {
        const std::string& name = mAxisState->getColumn(i)->mName;
        // The icon column ("input_glyph") renders the controller glyph in PromptFont;
        // every other column uses the default text font.
        cell_params.font = (name == "input_glyph")
            ? promptFontForCell()
            : LLFontGL::getFontSansSerif();
        cell_params.column = name;
        row_params.columns.add(cell_params);
    }
    // Configure each cell by column name so the XML column order stays the single
    // source of truth (reordering columns in the XML needs no change here).
    row_params.columns(axisStateColumn("raw_value")).font_halign = "right";
    row_params.columns(axisStateColumn("invert")).type = "checkbox";
    row_params.columns(axisStateColumn("invert")).font_halign = "right"; // right-align the checkbox within the cell
    row_params.columns(axisStateColumn("offset")).font_halign = "right";
    row_params.columns(axisStateColumn("dead_zone")).font_halign = "right";
    row_params.columns(axisStateColumn("fixed_value")).font_halign = "right";

    S32 input_glyph_col = axisStateColumn("input_glyph");
    S32 input_col       = axisStateColumn("input");
    for (size_t i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        LLScrollListItem* row = mAxisState->addRow(row_params);
        // The physical axis is fixed: fill both its PromptFont glyph and its text label.
        std::string glyph = promptFontGlyph(items[i]->getValue().asString());
        row->getColumn(input_glyph_col)->setValue(glyph.empty() ? LLSD() : LLSD(glyph));
        row->getColumn(input_col)->setValue(items[i]->getColumn(0)->getValue());  // physical axis label
    }

    // Fill the editable option columns for the currently selected state device.
    populateAxisStateOptionCells();
}

// Fills the Device State axis table's editable option cells (Invert / Offset /
// Dead Zone) from the state tab's selected device.  Called when rows are built and
// whenever the selected state device changes; the per-frame draw() only refreshes
// the Raw/Adjusted columns, so it never clobbers these while the user edits.
void LLPanelPreferenceGameControl::populateAxisStateOptionCells()
{
    auto options_it = mDeviceOptions.find(mStateSelectedDeviceGUID);
    if (options_it == mDeviceOptions.end())
    {
        return;
    }
    const auto& all_axis_options = options_it->second.options.getAxisOptions();
    std::vector<LLScrollListItem*> rows = mAxisState->getAllData();
    S32 invert_col    = axisStateColumn("invert");
    S32 offset_col    = axisStateColumn("offset");
    S32 dead_zone_col = axisStateColumn("dead_zone");
    for (size_t i = 0; i < rows.size() && i < all_axis_options.size(); ++i)
    {
        const LLGameControl::Options::AxisOptions& axis_options = all_axis_options[i];
        rows[i]->getColumn(invert_col)->setValue(axis_options.mMultiplier == -1);  // Invert checkbox
        setNumericLabel(rows[i]->getColumn(offset_col), axis_options.mOffset);      // Offset
        setNumericLabel(rows[i]->getColumn(dead_zone_col), axis_options.mDeadZone); // Dead Zone
    }
}

// Creates one row per physical button in the button-state table.
// Columns: Button (physical button label) | Value (filled live in draw()).
void LLPanelPreferenceGameControl::populateButtonStateRows()
{
    mButtonState->clearRows();

    std::vector<LLScrollListItem*> items = mButtonInputSelector->getAllData();

    S32 input_glyph_col = mButtonState->getColumn("input_glyph")->mIndex;
    S32 input_col       = mButtonState->getColumn("input")->mIndex;

    LLScrollListItem::Params row_params;
    LLScrollListCell::Params cell_params;
    for (S32 i = 0; i < (S32)(mButtonState->getNumColumns()); ++i)
    {
        const std::string& name = mButtonState->getColumn(i)->mName;
        // The icon column ("input_glyph") renders the controller glyph in PromptFont;
        // every other column uses the default text font.
        cell_params.font = (name == "input_glyph")
            ? promptFontForCell()
            : LLFontGL::getFontSansSerif();
        cell_params.column = name;
        row_params.columns.add(cell_params);
    }
    row_params.columns(mButtonState->getColumn("value")->mIndex).font_halign = "right";  // Value

    for (size_t i = 0; i < LLGameControl::NUM_BUTTONS; ++i)
    {
        LLScrollListItem* row = mButtonState->addRow(row_params);
        // The physical button is fixed: fill both its PromptFont glyph and its text label.
        std::string glyph = promptFontGlyph(items[i]->getValue().asString());
        row->getColumn(input_glyph_col)->setValue(glyph.empty() ? LLSD() : LLSD(glyph));
        row->getColumn(input_col)->setValue(items[i]->getColumn(0)->getValue());  // physical button label
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

    // Each row is a physical axis, and its Invert/Offset/Dead-Zone options are keyed by
    // physical axis, so Raw/Adjusted must be too.  State::mAxes/mRawAxes are keyed by
    // canonical output axis (post-mapping), which would land on the wrong row; instead
    // read mPhysicalRawAxes (pre-fix) / mPhysicalFixedAxes (post-fix), the pre-map values
    // onAxis() preserves by physical index specifically for this table.
    std::vector<LLScrollListItem*> axisRows = mAxisState->getAllData();
    S32 raw_col   = axisStateColumn("raw_value");
    S32 fixed_col = axisStateColumn("fixed_value");
    for (size_t i = 0; i < axisRows.size(); ++i)
    {
        if (i >= state->mPhysicalRawAxes.size())
        {
            break;
        }
        S32 raw   = (S32)state->mPhysicalRawAxes[i];    // Raw Value (pre-fix)
        S32 fixed = (S32)state->mPhysicalFixedAxes[i];  // Adjusted Value (post-fix)
        axisRows[i]->getColumn(raw_col)->setValue(llformat("%d ", raw));
        axisRows[i]->getColumn(fixed_col)->setValue(llformat("%d ", fixed));
    }

    // Each row is a physical button, so read the pre-map mPhysicalButtons; mButtons is
    // keyed by canonical button (post-mapping) and would show state on the wrong row.
    std::vector<LLScrollListItem*> buttonRows = mButtonState->getAllData();
    S32 button_value_col = mButtonState->getColumn("value")->mIndex;
    for (size_t i = 0; i < buttonRows.size(); ++i)
    {
        S32 value = (state->mPhysicalButtons >> i) & 0x1;
        buttonRows[i]->getColumn(button_value_col)->setValue(llformat("%d ", value));
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
    row_params.columns(2).font_halign = "right";  // Value

    // Column 0 is a per-block 0-based index; it restarts at 0 for the button
    // block below.  Column 1 is the canonical output name, column 2 the Value.
    for (size_t i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        LLScrollListItem* row = mDataOutput->addRow(row_params);
        row->getColumn(0)->setValue(llformat("%d", (S32)i));
        row->getColumn(1)->setValue(LLGameControl::axisOutputName((U8)i));
    }

    // One empty row separates the axis block from the button block.
    mDataOutput->addRow(row_params);

    for (size_t i = 0; i < LLGameControl::NUM_BUTTONS; ++i)
    {
        LLScrollListItem* row = mDataOutput->addRow(row_params);
        row->getColumn(0)->setValue(llformat("%d", (S32)i));
        // getRemoteName() gives the Button enum's symbolic name for the named
        // buttons (0..BUTTON_TOUCHPAD) but blanks the unnamed tail; fall back to
        // the indexed BUTTON_N form there, which is exactly the enum name for those.
        std::string name = LLGameControl::InputChannel(LLGameControl::InputChannel::TYPE_BUTTON, (U8)i).getRemoteName();
        LLStringUtil::trim(name);
        if (name.empty())
        {
            name = llformat("BUTTON_%d", (S32)i);
        }
        row->getColumn(1)->setValue(name);
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
        rows[row]->getColumn(2)->setValue(llformat("%d ", (S32)state.mAxes[i]));
    }

    ++row;  // skip the empty separator row

    for (size_t i = 0; i < LLGameControl::NUM_BUTTONS && row < rows.size(); ++i, ++row)
    {
        S32 value = (state.mButtons >> i) & 0x1;
        rows[row]->getColumn(2)->setValue(llformat("%d ", value));
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

// Populates glyph_selector with one item per value in text_selector.  Each item
// keeps the canonical value but is displayed as its PromptFont glyph, so the
// dropdown shown over the icon column matches the icons in the table.  Inputs
// with no glyph (e.g. "None", spare buttons) keep their text label in the normal
// font so they remain readable and selectable.
void LLPanelPreferenceGameControl::buildInputGlyphSelector(const LLComboBox* text_selector, LLComboBox* glyph_selector)
{
    glyph_selector->clearRows();
    for (const LLScrollListItem* item : text_selector->getAllData())
    {
        std::string value = item->getValue().asString();
        std::string glyph = promptFontGlyph(value);

        LLSD element;
        element["value"] = value;
        LLSD& column = element["columns"][0];
        column["column"] = "input";
        if (glyph.empty())
        {
            column["value"] = item->getColumn(0)->getValue().asString();  // text label
            column["font"]["name"] = "SansSerif";
            column["font"]["size"] = "Small";
        }
        else
        {
            column["value"] = glyph;
            column["font"]["name"] = "PromptFont";
            column["font"]["size"] = "Huge";
        }
        glyph_selector->addElement(element);
    }
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
S32 LLPanelPreferenceGameControl::axisStateColumn(const std::string& name) const
{
    LLScrollListColumn* column = mAxisState->getColumn(name);
    return column ? column->mIndex : -1;
}

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
    // Restore the text we blanked when the selector opened (initCombobox).
    // On commit this is the freshly committed value; otherwise it's the
    // original, so a dismissed selector leaves the cell unchanged.
    if (sSelectedCell)
    {
        sSelectedCell->setValue(sSelectedCellValue);
    }
    sSelectedCellValue = LLSD();

    sSelectedGrid = nullptr;
    sSelectedItem = nullptr;
    sSelectedCell = nullptr;
    sSelectedCombobox = nullptr;
    mNumericValueEditor->setVisible(false);
    mAxisInputSelector->setVisible(false);
    mAxisOutputSelector->setVisible(false);
    mButtonInputSelector->setVisible(false);
    mAxisInputGlyphSelector->setVisible(false);
    mButtonInputGlyphSelector->setVisible(false);
    mAxisOutputGlyphSelector->setVisible(false);
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
    // Invert/offset/dead-zone now live on the Device State tab; refresh them too
    // (a no-op unless the reset device is the one shown there).
    populateAxisStateOptionCells();

    LLGameControl::applySettingsFromLLSD(getSettingsAsLLSD());
}

// Resets the state-tab device's per-axis tuning (Invert / Offset / Dead Zone) to
// defaults for every axis, leaving the axis/button remaps untouched.  This backs
// the Device Options tab's "Restore Defaults" button.
void LLPanelPreferenceGameControl::onResetDeviceOptionsToDefaults()
{
    clearSelectionState();
    auto options_it = mDeviceOptions.find(mStateSelectedDeviceGUID);
    if (options_it == mDeviceOptions.end())
    {
        return;
    }

    LLGameControl::Options& options = options_it->second.options;
    for (LLGameControl::Options::AxisOptions& axis_options : options.getAxisOptions())
    {
        axis_options.resetToDefaults();
    }
    LLGameControl::setDeviceOptions(mStateSelectedDeviceGUID, options);

    populateAxisStateOptionCells();

    LLGameControl::applySettingsFromLLSD(getSettingsAsLLSD());
}

// Captures current settings values into mOrigSettings for later restoration upon cancel().
void LLPanelPreferenceGameControl::rememberOriginalSettings()
{
    mOrigSettings = LLGameControl::getSettingsAsLLSD();
}
