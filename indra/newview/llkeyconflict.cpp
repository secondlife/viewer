/** 
 * @file llkeyconflict.cpp
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

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#include "llviewerprecompiledheaders.h"

#include "llkeyconflict.h"

#include "llinitparam.h"
#include "llkeyboard.h"
#include "llviewercontrol.h"
#include "llviewerinput.h"
#include "llxuiparser.h"
//#include "llstring.h"

static const std::string typetostring[LLKeyConflictHandler::CONTROL_NUM_INDICES] = {
    "control_view_actions",
    "control_about",
    "control_orbit",
    "control_pan",
    "control_world_map",
    "control_zoom",
    "control_interactions",
    "control_build",
    //"control_drag",
    "control_edit",
    //"control_menu",
    "control_open",
    "control_touch",
    "control_wear",
    "control_movements",
    "walk_to",
    "teleport_to",
    "push_forward",
    "push_backward",
    "turn_left",
    "turn_right",
    "slide_left",
    "slide_right",
    "jump",
    "push_down",
    //"control_run",
    "toggle_run",
    "toggle_fly",
    "toggle_sit",
    "stop_moving",
    "control_camera",
    "look_up",
    "look_down",
    "move_forward",
    "move_backward",
    "move_forward_fast",
    "move_backward_fast",
    "move_forward_sitting",
    "move_backward_sitting",
    "spin_over",
    "spin_under",
    "spin_over_sitting",
    "spin_under_sitting",
    "pan_up",
    "pan_down",
    "pan_left",
    "pan_right",
    "pan_in",
    "pan_out",
    "spin_around_ccw",
    "spin_around_cw",
    "spin_around_ccw_sitting",
    "spin_around_cw_sitting",
    "control_edit_title",
    "edit_avatar_spin_ccw",
    "edit_avatar_spin_cw",
    "edit_avatar_spin_over",
    "edit_avatar_spin_under",
    "edit_avatar_move_forward",
    "edit_avatar_move_backward",
    "control_mediacontent",
    "toggle_pause_media",
    "toggle_enable_media",
    "voice_follow_key",
    "toggle_voice",
    "start_chat",
    "start_gesture",
    "control_reserved",
    "control_delete",
    "control_menu",
    "control_reserved_select",
    "control_shift_select",
    "control_cntrl_select"
};

// note, a solution is needed that will keep this up to date with llviewerinput
typedef std::map<std::string, LLKeyConflictHandler::EControlTypes> control_enum_t;
static const control_enum_t command_to_key =
{
    { "jump", LLKeyConflictHandler::CONTROL_JUMP },
    { "push_down", LLKeyConflictHandler::CONTROL_DOWN },
    { "push_forward", LLKeyConflictHandler::CONTROL_FORWARD },
    { "push_backward", LLKeyConflictHandler::CONTROL_BACKWARD },
    { "look_up", LLKeyConflictHandler::CONTROL_LOOK_UP },
    { "look_down", LLKeyConflictHandler::CONTROL_LOOK_DOWN },
    { "toggle_fly", LLKeyConflictHandler::CONTROL_TOGGLE_FLY },
    { "turn_left", LLKeyConflictHandler::CONTROL_LEFT },
    { "turn_right", LLKeyConflictHandler::CONTROL_RIGHT },
    { "slide_left", LLKeyConflictHandler::CONTROL_LSTRAFE },
    { "slide_right", LLKeyConflictHandler::CONTROL_RSTRAFE },
    { "spin_around_ccw", LLKeyConflictHandler::CONTROL_CAMERA_SPIN_CCW }, // todo, no idea what these spins are
    { "spin_around_cw", LLKeyConflictHandler::CONTROL_CAMERA_SPIN_CW },
    { "spin_around_ccw_sitting", LLKeyConflictHandler::CONTROL_CAMERA_SPIN_CCW_SITTING },
    { "spin_around_cw_sitting", LLKeyConflictHandler::CONTROL_CAMERA_SPIN_CCW_SITTING },
    { "spin_over", LLKeyConflictHandler::CONTROL_CAMERA_SOVER },
    { "spin_under", LLKeyConflictHandler::CONTROL_CAMERA_SUNDER },
    { "spin_over_sitting", LLKeyConflictHandler::CONTROL_CAMERA_SOVER_SITTING },
    { "spin_under_sitting", LLKeyConflictHandler::CONTROL_CAMERA_SUNDER_SITTING },
    { "move_forward", LLKeyConflictHandler::CONTROL_CAMERA_FORWARD },
    { "move_backward", LLKeyConflictHandler::CONTROL_CAMERA_BACKWARD },
    { "move_forward_sitting", LLKeyConflictHandler::CONTROL_CAMERA_FSITTING },
    { "move_backward_sitting", LLKeyConflictHandler::CONTROL_CAMERA_BSITTING },
    { "pan_up", LLKeyConflictHandler::CONTROL_CAMERA_PANUP },
    { "pan_down", LLKeyConflictHandler::CONTROL_CAMERA_PANDOWN },
    { "pan_left", LLKeyConflictHandler::CONTROL_CAMERA_PANLEFT },
    { "pan_right", LLKeyConflictHandler::CONTROL_CAMERA_PANRIGHT },
    { "pan_in", LLKeyConflictHandler::CONTROL_CAMERA_PANIN },
    { "pan_out", LLKeyConflictHandler::CONTROL_CAMERA_PANOUT },
    { "move_forward_fast", LLKeyConflictHandler::CONTROL_CAMERA_FFORWARD },
    { "move_backward_fast", LLKeyConflictHandler::CONTROL_CAMERA_FBACKWARD },
    { "edit_avatar_spin_ccw", LLKeyConflictHandler::CONTROL_EDIT_AV_SPIN_CCW },
    { "edit_avatar_spin_cw", LLKeyConflictHandler::CONTROL_EDIT_AV_SPIN_CW },
    { "edit_avatar_spin_over", LLKeyConflictHandler::CONTROL_EDIT_AV_SPIN_OVER },
    { "edit_avatar_spin_under", LLKeyConflictHandler::CONTROL_EDIT_AV_SPIN_UNDER },
    { "edit_avatar_move_forward", LLKeyConflictHandler::CONTROL_EDIT_AV_MV_FORWARD },
    { "edit_avatar_move_backward", LLKeyConflictHandler::CONTROL_EDIT_AV_MV_BACKWARD },
    { "stop_moving", LLKeyConflictHandler::CONTROL_STOP },
    { "start_chat", LLKeyConflictHandler::CONTROL_START_CHAT },
    { "start_gesture", LLKeyConflictHandler::CONTROL_START_GESTURE },
    { "toggle_run", LLKeyConflictHandler::CONTROL_TOGGLE_RUN },
    { "toggle_sit", LLKeyConflictHandler::CONTROL_SIT },
    { "toggle_parcel_media", LLKeyConflictHandler::CONTROL_PAUSE_MEDIA },
    { "toggle_enable_media", LLKeyConflictHandler::CONTROL_ENABLE_MEDIA },
    { "walk_to", LLKeyConflictHandler::CONTROL_MOVETO },
    { "teleport_to", LLKeyConflictHandler::CONTROL_TELEPORTTO },
    { "toggle_voice", LLKeyConflictHandler::CONTROL_TOGGLE_VOICE },
    { "voice_follow_key", LLKeyConflictHandler::CONTROL_VOICE },
};


// LLKeyboard::stringFromMask is meant for UI and is OS dependent,
// so this class uses it's own version
std::string string_from_mask(MASK mask)
{
    std::string res;
    if ((mask & MASK_CONTROL) != 0)
    {
        res = "CTL";
    }
    if ((mask & MASK_ALT) != 0)
    {
        if (!res.empty()) res += "_";
        res += "ALT";
    }
    if ((mask & MASK_SHIFT) != 0)
    {
        if (!res.empty()) res += "_";
        res += "SHIFT";
    }

    if (mask == MASK_NONE)
    {
        res = "NONE";
    }
    return res;
}

std::string string_from_mouse(EMouseClickType click)
{
    std::string res;
    switch (click)
    {
    case CLICK_LEFT:
        res = "LMB";
        break;
    case CLICK_MIDDLE:
        res = "MMB";
        break;
    case CLICK_RIGHT:
        res = "RMB";
        break;
    case CLICK_BUTTON4:
        res = "MB4";
        break;
    case CLICK_BUTTON5:
        res = "MB5";
        break;
    case CLICK_DOUBLELEFT:
        res = "Double LMB";
        break;
    default:
        break;
    }
    return res;
}

EMouseClickType mouse_from_string(const std::string& input)
{
    if (input == "LMB")
    {
        return CLICK_LEFT;
    }
    if (input == "MMB")
    {
        return CLICK_MIDDLE;
    }
    if (input == "RMB")
    {
        return CLICK_RIGHT;
    }
    if (input == "MB4")
    {
        return CLICK_BUTTON4;
    }
    if (input == "MB5")
    {
        return CLICK_BUTTON5;
    }
    return CLICK_NONE;
}

// LLKeyConflictHandler

LLKeyConflictHandler::LLKeyConflictHandler()
    : mHasUnsavedChanges(false)
{
}

LLKeyConflictHandler::LLKeyConflictHandler(ESourceMode mode)
    : mHasUnsavedChanges(false),
    mLoadMode(mode)
{
    loadFromSettings(mode);
}

bool LLKeyConflictHandler::canHandleControl(LLKeyConflictHandler::EControlTypes control_type, EMouseClickType mouse_ind, KEY key, MASK mask)
{
    return mControlsMap[control_type].canHandle(mouse_ind, key, mask);
}

bool LLKeyConflictHandler::canHandleKey(EControlTypes control_type, KEY key, MASK mask)
{
    return canHandleControl(control_type, CLICK_NONE, key, mask);
}

bool LLKeyConflictHandler::canHandleMouse(LLKeyConflictHandler::EControlTypes control_type, EMouseClickType mouse_ind, MASK mask)
{
    return canHandleControl(control_type, mouse_ind, KEY_NONE, mask);
}

bool LLKeyConflictHandler::canHandleMouse(EControlTypes control_type, S32 mouse_ind, MASK mask)
{
    return canHandleControl(control_type, (EMouseClickType)mouse_ind, KEY_NONE, mask);
}

bool LLKeyConflictHandler::canAssignControl(EControlTypes control_type)
{
    std::map<EControlTypes, LLKeyConflict>::iterator iter = mControlsMap.find(control_type);
    if (iter != mControlsMap.end())
    {
        return iter->second.mAssignable;
    }
    return false;
}

bool LLKeyConflictHandler::registerControl(EControlTypes control_type, U32 index, EMouseClickType mouse, KEY key, MASK mask, bool ignore_mask)
{
    LLKeyConflict &type_data = mControlsMap[control_type];
    if (!type_data.mAssignable)
    {
        LL_ERRS() << "Error in code, user or system should not be able to change certain controls" << LL_ENDL;
    }
    LLKeyData data(mouse, key, mask, ignore_mask);
    if (type_data.mKeyBind.getKeyData(index) == data)
    {
        return true;
    }
    if (removeConflicts(data, type_data.mConflictMask))
    {
        type_data.mKeyBind.replaceKeyData(data, index);
        mHasUnsavedChanges = true;
        return true;
    }
    // control already in use/blocked
    return false;
}

LLKeyData LLKeyConflictHandler::getControl(EControlTypes control_type, U32 index)
{
    return mControlsMap[control_type].getKeyData(index);
}

// static
std::string LLKeyConflictHandler::getStringFromKeyData(const LLKeyData& keydata)
{
    std::string result;

    if (keydata.mMask != MASK_NONE && keydata.mKey != KEY_NONE)
    {
        result = LLKeyboard::stringFromAccelerator(keydata.mMask, keydata.mKey);
    }
    else if (keydata.mKey != KEY_NONE)
    {
        result = LLKeyboard::stringFromKey(keydata.mKey);
    }
    else if (keydata.mMask != MASK_NONE)
    {
        result = LLKeyboard::stringFromAccelerator(keydata.mMask);
    }

    result += string_from_mouse(keydata.mMouse);

    return result;
}

// static
std::string LLKeyConflictHandler::getControlName(EControlTypes control_type)
{
    return typetostring[control_type];
}

std::string LLKeyConflictHandler::getControlString(EControlTypes control_type, U32 index)
{
    return getStringFromKeyData(mControlsMap[control_type].getKeyData(index));
}

void  LLKeyConflictHandler::loadFromSettings(const LLViewerInput::KeyMode& keymode, control_map_t *destination)
{
    for (LLInitParam::ParamIterator<LLViewerInput::KeyBinding>::const_iterator it = keymode.bindings.begin(),
        end_it = keymode.bindings.end();
        it != end_it;
    ++it)
    {
        KEY key;
        MASK mask;
        EMouseClickType mouse = it->mouse.isProvided() ? mouse_from_string(it->mouse) : CLICK_NONE;
        bool ignore = it->ignore.isProvided() ? it->ignore.getValue() : false;
        if (it->key.getValue().empty())
        {
            key = KEY_NONE;
        }
        else
        {
            LLKeyboard::keyFromString(it->key, &key);
        }
        LLKeyboard::maskFromString(it->mask, &mask);
        std::string command_name = it->command;
        // it->command
        // It might be better to have <string,bind> map, but at the moment enum is easier to iterate through.
        // Besides keys.xml might not contain all commands
        control_enum_t::const_iterator iter = command_to_key.find(command_name);
        if (iter != command_to_key.end())
        {
            LLKeyConflict &type_data = (*destination)[iter->second];
            type_data.mAssignable = true;
            // Don't care about conflict level, all movement and view commands already account for it
            type_data.mKeyBind.addKeyData(mouse, key, mask, ignore);
        }
    }
}

void LLKeyConflictHandler::loadFromSettings(const ESourceMode &load_mode, const std::string &filename, control_map_t *destination)
{
    if (filename.empty())
    {
        return;
    }

    LLViewerInput::Keys keys;
    LLSimpleXUIParser parser;

    if (parser.readXUI(filename, keys)
        && keys.validateBlock())
    {
        switch (load_mode)
        {
        case MODE_FIRST_PERSON:
            if (keys.first_person.isProvided())
            {
                loadFromSettings(keys.first_person, destination);
            }
            break;
        case MODE_THIRD_PERSON:
            if (keys.third_person.isProvided())
            {
                loadFromSettings(keys.third_person, destination);
            }
            break;
        case MODE_EDIT:
            if (keys.edit.isProvided())
            {
                loadFromSettings(keys.edit, destination);
            }
            break;
        case MODE_EDIT_AVATAR:
            if (keys.edit_avatar.isProvided())
            {
                loadFromSettings(keys.edit_avatar, destination);
            }
            break;
        case MODE_SITTING:
            if (keys.sitting.isProvided())
            {
                loadFromSettings(keys.sitting, destination);
            }
            break;
        default:
            break;
        }
    }
}

void  LLKeyConflictHandler::loadFromSettings(ESourceMode load_mode)
{
    mControlsMap.clear();
    mDefaultsMap.clear();

    // E.X. In case we need placeholder keys for conflict resolution.
    generatePlaceholders(load_mode);

    if (load_mode == MODE_GENERAL)
    {
        for (U32 i = 0; i < CONTROL_NUM_INDICES; i++)
        {
            EControlTypes type = (EControlTypes)i;
            switch (type)
            {
            case LLKeyConflictHandler::CONTROL_VIEW_ACTIONS:
            case LLKeyConflictHandler::CONTROL_INTERACTIONS:
            case LLKeyConflictHandler::CONTROL_MOVEMENTS:
            case LLKeyConflictHandler::CONTROL_MEDIACONTENT:
            case LLKeyConflictHandler::CONTROL_CAMERA:
            case LLKeyConflictHandler::CONTROL_EDIT_TITLE:
            case LLKeyConflictHandler::CONTROL_RESERVED:
                // ignore 'headers', they are for representation and organization purposes
                break;
            default:
                {
                    std::string name = getControlName(type);
                    LLControlVariablePtr var = gSavedSettings.getControl(name);
                    if (var)
                    {
                        LLKeyBind bind(var->getValue());
                        LLKeyConflict key(bind, true, 0);
                        mControlsMap[type] = key;
                    }
                    break;
                }
            }
        }
    }
    else
    {
        // load defaults
        std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "keys.xml");
        loadFromSettings(load_mode, filename, &mDefaultsMap);

        // load user's
        filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "keys.xml");
        if (gDirUtilp->fileExists(filename))
        {
            loadFromSettings(load_mode, filename, &mControlsMap);
        }
        else
        {
            // mind placeholders
            mControlsMap.insert(mDefaultsMap.begin(), mDefaultsMap.end());
        }
    }
    mLoadMode = load_mode;
}

void  LLKeyConflictHandler::saveToSettings()
{
    if (mControlsMap.empty())
    {
        return;
    }

    if (mLoadMode == MODE_GENERAL)
    {
        for (U32 i = 0; i < CONTROL_NUM_INDICES; i++)
        {
            EControlTypes type = (EControlTypes)i;
            switch (type)
            {
            case LLKeyConflictHandler::CONTROL_VIEW_ACTIONS:
            case LLKeyConflictHandler::CONTROL_INTERACTIONS:
            case LLKeyConflictHandler::CONTROL_MOVEMENTS:
            case LLKeyConflictHandler::CONTROL_MEDIACONTENT:
            case LLKeyConflictHandler::CONTROL_CAMERA:
            case LLKeyConflictHandler::CONTROL_EDIT_TITLE:
            case LLKeyConflictHandler::CONTROL_RESERVED:
                // ignore 'headers', they are for representation and organization purposes
                break;
            default:
            {
                if (mControlsMap[type].mAssignable)
                {
                    std::string name = getControlName(type);
                    if (gSavedSettings.controlExists(name))
                    {
                        gSavedSettings.setLLSD(name, mControlsMap[type].mKeyBind.asLLSD());
                    }
                    else if (!mControlsMap[type].mKeyBind.empty())
                    {
                        // shouldn't happen user side since all settings are supposed to be declared already, but handy when creating new ones
                        // (just don't forget to change comment and to copy them from user's folder)
                        LL_INFOS() << "Creating new keybinding " << name << LL_ENDL;
                        gSavedSettings.declareLLSD(name, mControlsMap[type].mKeyBind.asLLSD(), "comment", LLControlVariable::PERSIST_ALWAYS);
                    }
                }
                break;
            }
            }
        }
    }
    else
    {
        // loaded full copy of original file
        std::string filename = gDirUtilp->findFile("keys.xml",
            gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, ""),
            gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));

        LLViewerInput::Keys keys;
        LLSimpleXUIParser parser;

        if (parser.readXUI(filename, keys)
            && keys.validateBlock())
        {
            // replace category we edited

            // todo: fix this
            // workaround to avoid doing own param container 
            LLViewerInput::KeyMode mode;
            LLViewerInput::KeyBinding binding;

            control_map_t::iterator iter = mControlsMap.begin();
            control_map_t::iterator end = mControlsMap.end();
            for (; iter != end; ++iter)
            {
                // By default xml have (had) up to 6 elements per function
                // eventually it will be cleaned up and UI will only shows 3 per function,
                // so make sure to cleanup.
                // Also this helps in keeping file small.
                iter->second.mKeyBind.trimEmpty();
                U32 size = iter->second.mKeyBind.getDataCount();
                for (U32 i = 0; i < size; ++i)
                {
                    // Still write empty keys to make sure we will maintain UI position
                    LLKeyData data = iter->second.mKeyBind.getKeyData(i);
                    if (data.mKey == KEY_NONE)
                    {
                        binding.key = "";
                    }
                    else
                    {
                        // Note: this is UI string, we might want to hardcode our own for 'fixed' use in keys.xml
                        binding.key = LLKeyboard::stringFromKey(data.mKey);
                    }
                    binding.mask = string_from_mask(data.mMask);
                    binding.mouse.set(string_from_mouse(data.mMouse), true); //set() because 'optional', for compatibility purposes
                    binding.ignore.set(data.mIgnoreMasks, true);
                    binding.command = getControlName(iter->first);
                    mode.bindings.add(binding);
                }
            }

            switch (mLoadMode)
            {
            case MODE_FIRST_PERSON:
                if (keys.first_person.isProvided())
                {
                    keys.first_person.bindings.set(mode.bindings, true);
                }
                break;
            case MODE_THIRD_PERSON:
                if (keys.third_person.isProvided())
                {
                    keys.third_person.bindings.set(mode.bindings, true);
                }
                break;
            case MODE_EDIT:
                if (keys.edit.isProvided())
                {
                    keys.edit.bindings.set(mode.bindings, true);
                }
                break;
            case MODE_EDIT_AVATAR:
                if (keys.edit_avatar.isProvided())
                {
                    keys.edit_avatar.bindings.set(mode.bindings, true);
                }
                break;
            case MODE_SITTING:
                if (keys.sitting.isProvided())
                {
                    keys.sitting.bindings.set(mode.bindings, true);
                }
                break;
            default:
                break;
            }

            // write back to user's xml;
            std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "keys.xml");

            LLXMLNodePtr output_node = new LLXMLNode("keys", false);
            LLXUIParser parser;
            parser.writeXUI(output_node, keys);

            // Write the resulting XML to file
            if (!output_node->isNull())
            {
                LLFILE *fp = LLFile::fopen(filename, "w");
                if (fp != NULL)
                {
                    LLXMLNode::writeHeaderToFile(fp);
                    output_node->writeToFile(fp);
                    fclose(fp);
                }
            }
            // Now force a rebind for keyboard
            if (gDirUtilp->fileExists(filename))
            {
                gViewerInput.loadBindingsXML(filename);
            }
        }
    }
    mHasUnsavedChanges = false;
}

LLKeyData LLKeyConflictHandler::getDefaultControl(EControlTypes control_type, U32 index)
{
    if (mLoadMode == MODE_GENERAL)
    {
        std::string name = getControlName(control_type);
        LLControlVariablePtr var = gSavedSettings.getControl(name);
        if (var)
        {
            return LLKeyBind(var->getDefault()).getKeyData(index);
        }
        return LLKeyData();
    }
    else
    {
        control_map_t::iterator iter = mDefaultsMap.find(control_type);
        if (iter != mDefaultsMap.end())
        {
            return iter->second.mKeyBind.getKeyData(index);
        }
        return LLKeyData();
    }
}

void LLKeyConflictHandler::resetToDefault(EControlTypes control_type, U32 index)
{
    LLKeyData data = getDefaultControl(control_type, index);

    if (data != mControlsMap[control_type].getKeyData(index))
    {
        // reset controls that might have been switched to our current control
        removeConflicts(data, mControlsMap[control_type].mConflictMask);
        mControlsMap[control_type].setKeyData(data, index);
    }
}

void LLKeyConflictHandler::resetToDefaultAndResolve(EControlTypes control_type, bool ignore_conflicts)
{
    if (mLoadMode == MODE_GENERAL)
    {
        std::string name = getControlName(control_type);
        LLControlVariablePtr var = gSavedSettings.getControl(name);
        if (var)
        {
            LLKeyBind bind(var->getDefault());
            if (!ignore_conflicts)
            {
                for (S32 i = 0; i < bind.getDataCount(); ++i)
                {
                    removeConflicts(bind.getKeyData(i), mControlsMap[control_type].mConflictMask);
                }
            }
            mControlsMap[control_type].mKeyBind = bind;
        }
        else
        {
            mControlsMap[control_type].mKeyBind.clear();
        }
    }
    else
    {
        control_map_t::iterator iter = mDefaultsMap.find(control_type);
        if (iter != mDefaultsMap.end())
        {
            if (!ignore_conflicts)
            {
                for (S32 i = 0; i < iter->second.mKeyBind.getDataCount(); ++i)
                {
                    removeConflicts(iter->second.mKeyBind.getKeyData(i), mControlsMap[control_type].mConflictMask);
                }
            }
            mControlsMap[control_type].mKeyBind = iter->second.mKeyBind;
        }
        else
        {
            mControlsMap[control_type].mKeyBind.clear();
        }
    }
}

void LLKeyConflictHandler::resetToDefault(EControlTypes control_type)
{
    // reset specific binding without ignoring conflicts
    resetToDefaultAndResolve(control_type, false);
}

void LLKeyConflictHandler::resetToDefaults(ESourceMode mode)
{
    if (mode == MODE_GENERAL)
    {
        for (U32 i = 0; i < CONTROL_NUM_INDICES; i++)
        {
            EControlTypes type = (EControlTypes)i;
            switch (type)
            {
            case LLKeyConflictHandler::CONTROL_VIEW_ACTIONS:
            case LLKeyConflictHandler::CONTROL_INTERACTIONS:
            case LLKeyConflictHandler::CONTROL_MOVEMENTS:
            case LLKeyConflictHandler::CONTROL_MEDIACONTENT:
            case LLKeyConflictHandler::CONTROL_CAMERA:
            case LLKeyConflictHandler::CONTROL_EDIT_TITLE:
            case LLKeyConflictHandler::CONTROL_RESERVED:
                // ignore 'headers', they are for representation and organization purposes
                break;
            default:
            {
                resetToDefaultAndResolve(type, true);
                break;
            }
            }
        }
    }
    else
    {
        mControlsMap.clear();
        generatePlaceholders(mode);
        mControlsMap.insert(mDefaultsMap.begin(), mDefaultsMap.end());
    }

    mHasUnsavedChanges = true;
}

void LLKeyConflictHandler::resetToDefaults()
{
    if (!empty())
    {
        resetToDefaults(mLoadMode);
    }
    else
    {
        // not optimal since:
        // 1. We are not sure that mLoadMode was set
        // 2. We are not sure if there are any changes in comparison to default
        // 3. We are loading 'current' only to replace it
        // but it is reliable and works Todo: consider optimizing.
        loadFromSettings(mLoadMode);
        resetToDefaults(mLoadMode);
    }
}

void LLKeyConflictHandler::clear()
{
    mHasUnsavedChanges = false;
    mControlsMap.clear();
    mDefaultsMap.clear();
}

void LLKeyConflictHandler::resetKeyboardBindings()
{
    std::string filename = gDirUtilp->findFile("keys.xml",
        gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, ""),
        gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));
    
    gViewerInput.loadBindingsXML(filename);
}

void LLKeyConflictHandler::generatePlaceholders(ESourceMode load_mode)
{
    // These controls are meant to cause conflicts when user tries to assign same control somewhere else
    // also this can be used to pre-record controls that should not conflict or to assign conflict groups/masks
    /*registerTemporaryControl(CONTROL_RESERVED_MENU, CLICK_RIGHT, KEY_NONE, MASK_NONE, 0);
    registerTemporaryControl(CONTROL_DELETE, CLICK_NONE, KEY_DELETE, MASK_NONE, 0);*/
}

bool LLKeyConflictHandler::removeConflicts(const LLKeyData &data, const U32 &conlict_mask)
{
    if (conlict_mask == CONFLICT_NOTHING)
    {
        // Can't conflict
        return true;
    }
    std::map<EControlTypes, S32> conflict_list;
    control_map_t::iterator cntrl_iter = mControlsMap.begin();
    control_map_t::iterator cntrl_end = mControlsMap.end();
    for (; cntrl_iter != cntrl_end; ++cntrl_iter)
    {
        S32 index = cntrl_iter->second.mKeyBind.findKeyData(data);
        if (index >= 0
            && cntrl_iter->second.mConflictMask != CONFLICT_NOTHING
            && (cntrl_iter->second.mConflictMask & conlict_mask) != 0)
        {
            if (cntrl_iter->second.mAssignable)
            {
                // Potentially we can have multiple conflict flags conflicting
                // including unassignable keys.
                // So record the conflict and find all others before doing any changes.
                // Assume that there is only one conflict per bind
                conflict_list[cntrl_iter->first] = index;
            }
            else
            {
                return false;
            }
        }
    }

    std::map<EControlTypes, S32>::iterator cnflct_iter = conflict_list.begin();
    std::map<EControlTypes, S32>::iterator cnflct_end = conflict_list.end();
    for (; cnflct_iter != cnflct_end; ++cnflct_iter)
    {
        mControlsMap[cnflct_iter->first].mKeyBind.resetKeyData(cnflct_iter->second);
    }
    return true;
}

void LLKeyConflictHandler::registerTemporaryControl(EControlTypes control_type, EMouseClickType mouse, KEY key, MASK mask, U32 conflict_mask)
{
    LLKeyConflict *type_data = &mControlsMap[control_type];
    type_data->mAssignable = false;
    type_data->mConflictMask = conflict_mask;
    type_data->mKeyBind.addKeyData(mouse, key, mask, false);
}

