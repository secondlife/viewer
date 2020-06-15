/** 
 * @file llkeyconflict.h
 * @brief 
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#ifndef LL_LLKEYCONFLICT_H
#define LL_LLKEYCONFLICT_H

#include "llkeybind.h"
#include "llviewerinput.h"


class LLKeyConflict
{
public:
    LLKeyConflict() : mAssignable(true), mConflictMask(U32_MAX) {} //temporary assignable, don't forget to change once all keys are recorded
    LLKeyConflict(bool assignable, U32 conflict_mask)
        : mAssignable(assignable), mConflictMask(conflict_mask) {}
    LLKeyConflict(const LLKeyBind &bind, bool assignable, U32 conflict_mask)
        : mAssignable(assignable), mConflictMask(conflict_mask), mKeyBind(bind) {}

    LLKeyData getPrimaryKeyData() { return mKeyBind.getKeyData(0); }
    LLKeyData getKeyData(U32 index) { return mKeyBind.getKeyData(index); }
    void setPrimaryKeyData(const LLKeyData& data) { mKeyBind.replaceKeyData(data, 0); }
    void setKeyData(const LLKeyData& data, U32 index) { mKeyBind.replaceKeyData(data, index); }
    bool canHandle(EMouseClickType mouse, KEY key, MASK mask) { return mKeyBind.canHandle(mouse, key, mask); }

    LLKeyBind mKeyBind;
    bool mAssignable; // whether user can change key or key simply acts as placeholder
    U32 mConflictMask;
};

class LLKeyConflictHandler
{
public:

    enum ESourceMode // partially repeats e_keyboard_mode
    {
        MODE_FIRST_PERSON,
        MODE_THIRD_PERSON,
        MODE_EDIT_AVATAR,
        MODE_SITTING,
        MODE_SAVED_SETTINGS, // for settings from saved settings
        MODE_COUNT
    };

    const U32 CONFLICT_NOTHING = 0;
    // at the moment this just means that key will conflict with everything that is identical
    const U32 CONFLICT_ANY = U32_MAX;

    // Note: missed selection and edition commands (would be really nice to go through selection via MB4/5 or wheel)

    LLKeyConflictHandler();
    LLKeyConflictHandler(ESourceMode mode);
    ~LLKeyConflictHandler();

    bool canHandleControl(const std::string &control_name, EMouseClickType mouse_ind, KEY key, MASK mask);
    bool canHandleKey(const std::string &control_name, KEY key, MASK mask);
    bool canHandleMouse(const std::string &control_name, EMouseClickType mouse_ind, MASK mask);
    bool canHandleMouse(const std::string &control_name, S32 mouse_ind, MASK mask); //Just for convinience
    bool canAssignControl(const std::string &control_name);
    static bool isReservedByMenu(const KEY &key, const MASK &mask);
    static bool isReservedByMenu(const LLKeyData &data);

    // @control_name - see REGISTER_KEYBOARD_ACTION in llviewerinput for avaliable options,
    // usually this is just name of the function
    // @data_index - single control (function) can have multiple key combinations trigering
    // it, this index indicates combination function will change/add; Note that preferences
    // floater can only display up to 3 options, but data_index can be bigger than that
    // @mouse_ind - mouse action (middle click, MB5 etc)
    // @key - keyboard key action
    // @mask - shift/ctrl/alt flags
    // @ignore_mask - Either to expect exact match (ctrl+K will not trigger if ctrl+shift+K
    // is active) or ignore not expected masks as long as expected mask is present
    // (ctrl+K will be triggered if ctrl+shift+K is active)
    bool registerControl(const std::string &control_name, U32 data_index, EMouseClickType mouse_ind, KEY key, MASK mask, bool ignore_mask); //todo: return conflicts?
    bool clearControl(const std::string &control_name, U32 data_index);

    LLKeyData getControl(const std::string &control_name, U32 data_index);

    // localized string
    static std::string getStringFromKeyData(const LLKeyData& keydata);
    std::string getControlString(const std::string &control_name, U32 data_index);

    // Load single control, overrides existing one if names match
    void loadFromControlSettings(const std::string &name);
    // Drops any changes loads controls with ones from 'saved settings' or from xml
    void loadFromSettings(ESourceMode load_mode);

    // Saves settings to 'saved settings' or to xml
    // If 'temporary' is set, function will save settings to temporary
    // file and reload input bindings from temporary file.
    // 'temporary' does not support gSavedSettings, those are handled
    // by preferences, so 'temporary' is such case will simply not
    // reset mHasUnsavedChanges
    //
    // 'temporary' exists to support ability of live-editing settings in
    // preferences: temporary for testing changes 'live' without saving them,
    // then hitting ok/cancel and save/discard values permanently.
    void saveToSettings(bool apply_temporary = false);

    LLKeyData getDefaultControl(const std::string &control_name, U32 data_index);
    // Resets keybinding to default variant from 'saved settings' or xml
    void resetToDefault(const std::string &control_name, U32 index);
    void resetToDefault(const std::string &control_name);
    // resets current mode to defaults
    void resetToDefaults();

    bool empty() { return mControlsMap.empty(); }
    void clear();

    // reloads bindings from last valid user's xml or from default xml
    // to keyboard's handler
    static void resetKeyboardBindings();

    bool hasUnsavedChanges() { return mHasUnsavedChanges; }
    void setLoadMode(ESourceMode mode) { mLoadMode = mode; }
    ESourceMode getLoadMode() { return mLoadMode; }

private:
    void resetToDefaultAndResolve(const std::string &control_name, bool ignore_conflicts);
    void resetToDefaults(ESourceMode mode);

    // at the moment these kind of control is not savable, but takes part in conflict resolution
    void registerTemporaryControl(const std::string &control_name, EMouseClickType mouse_ind, KEY key, MASK mask, U32 conflict_mask);
    void registerTemporaryControl(const std::string &control_name, U32 conflict_mask = 0);

    typedef std::map<std::string, LLKeyConflict> control_map_t;
    void loadFromSettings(const LLViewerInput::KeyMode& keymode, control_map_t *destination);
    bool loadFromSettings(const ESourceMode &load_mode, const std::string &filename, control_map_t *destination);
    void generatePlaceholders(ESourceMode load_mode); //E.x. non-assignable values
    // returns false in case user is trying to reuse control that can't be reassigned
    bool removeConflicts(const LLKeyData &data, const U32 &conlict_mask);

    // removes flags and removes temporary file, returns 'true' if file was removed
    bool clearUnsavedChanges();
    // return true if there was a file to remove
    static bool clearTemporaryFile();

    control_map_t mControlsMap;
    control_map_t mDefaultsMap;
    bool mHasUnsavedChanges;
    ESourceMode mLoadMode;

    // To implement 'apply immediately'+revert on cancel, class applies changes to temporary file
    // but this only works for settings from keybndings files (key_bindings.xml)
    // saved setting rely onto external mechanism of preferences floater
    bool mUsesTemporaryFile;
    static S32 sTemporaryFileUseCount;
};


#endif  // LL_LLKEYCONFLICT_H
