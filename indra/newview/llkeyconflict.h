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
        MODE_EDIT,
        MODE_EDIT_AVATAR,
        MODE_SITTING,
        MODE_GENERAL, // for settings from saved settings
        MODE_COUNT
    };

    const U32 CONFLICT_NOTHING = 0;
    // at the moment this just means that key will conflict with everything that is identical
    const U32 CONFLICT_ANY = U32_MAX;

    // todo, unfortunately will have to remove this and use map/array of strings
    enum EControlTypes
    {
        CONTROL_VIEW_ACTIONS = 0, // Group control, for visual representation in view, not for use
        CONTROL_ABOUT,
        CONTROL_ORBIT,
        CONTROL_PAN,
        CONTROL_WORLD_MAP,
        CONTROL_ZOOM,
        CONTROL_INTERACTIONS, // Group control, for visual representation
        CONTROL_BUILD,
        //CONTROL_DRAG,
        CONTROL_EDIT,
        //CONTROL_MENU,
        CONTROL_OPEN,
        CONTROL_TOUCH,
        CONTROL_WEAR,
        CONTROL_MOVEMENTS, // Group control, for visual representation
        CONTROL_MOVETO,
        CONTROL_TELEPORTTO,
        CONTROL_FORWARD,
        CONTROL_BACKWARD,
        CONTROL_LEFT, // Check and sinc name with real movement names
        CONTROL_RIGHT,
        CONTROL_LSTRAFE,
        CONTROL_RSTRAFE,
        CONTROL_JUMP,
        CONTROL_DOWN,
        //CONTROL_RUN,
        CONTROL_TOGGLE_RUN,
        CONTROL_TOGGLE_FLY,
        CONTROL_SIT,
        CONTROL_STOP,
        CONTROL_CAMERA, // Group control, for visual representation
        CONTROL_LOOK_UP,
        CONTROL_LOOK_DOWN,
        CONTROL_CAMERA_FORWARD,
        CONTROL_CAMERA_BACKWARD,
        CONTROL_CAMERA_FFORWARD,
        CONTROL_CAMERA_FBACKWARD,
        CONTROL_CAMERA_FSITTING,
        CONTROL_CAMERA_BSITTING,
        CONTROL_CAMERA_SOVER,
        CONTROL_CAMERA_SUNDER,
        CONTROL_CAMERA_SOVER_SITTING,
        CONTROL_CAMERA_SUNDER_SITTING,
        CONTROL_CAMERA_PANUP,
        CONTROL_CAMERA_PANDOWN,
        CONTROL_CAMERA_PANLEFT,
        CONTROL_CAMERA_PANRIGHT,
        CONTROL_CAMERA_PANIN,
        CONTROL_CAMERA_PANOUT,
        CONTROL_CAMERA_SPIN_CCW,
        CONTROL_CAMERA_SPIN_CW,
        CONTROL_CAMERA_SPIN_CCW_SITTING,
        CONTROL_CAMERA_SPIN_CW_SITTING,
        CONTROL_EDIT_TITLE, // Group control, for visual representation
        CONTROL_EDIT_AV_SPIN_CCW,
        CONTROL_EDIT_AV_SPIN_CW,
        CONTROL_EDIT_AV_SPIN_OVER,
        CONTROL_EDIT_AV_SPIN_UNDER,
        CONTROL_EDIT_AV_MV_FORWARD,
        CONTROL_EDIT_AV_MV_BACKWARD,
        CONTROL_MEDIACONTENT, // Group control, for visual representation
        CONTROL_PAUSE_MEDIA, // Play pause
        CONTROL_ENABLE_MEDIA, // Play stop
        CONTROL_VOICE, // Keep pressing for voice to be ON
        CONTROL_TOGGLE_VOICE, // Press once to ON/OFF
        CONTROL_START_CHAT, // Press once to ON/OFF
        CONTROL_START_GESTURE, // Press once to ON/OFF
        CONTROL_RESERVED, // Special group control, controls that are disabled by default and not meant to be changed
        CONTROL_DELETE,
        CONTROL_RESERVED_MENU,
        CONTROL_RESERVED_SELECT,
        CONTROL_SHIFT_SELECT,
        CONTROL_CNTRL_SELECT,
        CONTROL_NUM_INDICES // Size, always should be last
    };

    // Note: missed selection and edition commands (would be really nice to go through selection via MB4/5 or wheel)

    LLKeyConflictHandler();
    LLKeyConflictHandler(ESourceMode mode);

    bool canHandleControl(EControlTypes control_type, EMouseClickType mouse_ind, KEY key, MASK mask);
    bool canHandleKey(EControlTypes control_type, KEY key, MASK mask);
    bool canHandleMouse(EControlTypes control_type, EMouseClickType mouse_ind, MASK mask);
    bool canHandleMouse(EControlTypes control_type, S32 mouse_ind, MASK mask); //Just for convinience
    bool canAssignControl(EControlTypes control_type);
    bool registerControl(EControlTypes control_type, U32 data_index, EMouseClickType mouse_ind, KEY key, MASK mask, bool ignore_mask); //todo: return conflicts?

    LLKeyData getControl(EControlTypes control_type, U32 data_index);

    static std::string LLKeyConflictHandler::getStringFromKeyData(const LLKeyData& keydata);
    static std::string getControlName(EControlTypes control_type);
    std::string getControlString(EControlTypes control_type, U32 data_index);


    // Drops any changes loads controls with ones from 'saved settings' or from xml
    void loadFromSettings(ESourceMode load_mode);
    // Saves settings to 'saved settings' or to xml
    void saveToSettings();

    LLKeyData getDefaultControl(EControlTypes control_type, U32 data_index);
    // Resets keybinding to default variant from 'saved settings' or xml
    void resetToDefault(EControlTypes control_type, U32 index);
    void resetToDefault(EControlTypes control_type);
    // resets current mode to defaults, 
    void resetToDefaults();

    bool empty() { return mControlsMap.empty(); }
    void clear();

    bool hasUnsavedChanges() { return mHasUnsavedChanges; }
    void setLoadMode(ESourceMode mode) { mLoadMode = mode; }
    ESourceMode getLoadMode() { return mLoadMode; }

private:
    void resetToDefaultAndResolve(EControlTypes control_type, bool ignore_conflicts);
    void resetToDefaults(ESourceMode mode);

    // at the moment these kind of control is not savable, but takes part will take part in conflict resolution
    void registerTemporaryControl(EControlTypes control_type, EMouseClickType mouse_ind, KEY key, MASK mask, U32 conflict_mask);

    typedef std::map<EControlTypes, LLKeyConflict> control_map_t;
    void loadFromSettings(const LLViewerInput::KeyMode& keymode, control_map_t *destination);
    void loadFromSettings(const ESourceMode &load_mode, const std::string &filename, control_map_t *destination);
    void resetKeyboardBindings();
    void generatePlaceholders(ESourceMode load_mode); //E.x. non-assignable values
    // returns false in case user is trying to reuse control that can't be reassigned
    bool removeConflicts(const LLKeyData &data, const U32 &conlict_mask);

    control_map_t mControlsMap;
    control_map_t mDefaultsMap;
    bool mHasUnsavedChanges;
    ESourceMode mLoadMode;
};


#endif  // LL_LLKEYCONFLICT_H
