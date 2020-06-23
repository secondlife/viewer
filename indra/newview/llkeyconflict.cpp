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
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerinput.h"
#include "llviewermenu.h"
#include "llxuiparser.h"

static const std::string saved_settings_key_controls[] = { "placeholder" }; // add settings from gSavedSettings here

static const std::string filename_default = "key_bindings.xml";
static const std::string filename_temporary = "key_bindings_tmp.xml"; // used to apply uncommited changes on the go.

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

std::string string_from_mouse(EMouseClickType click, bool translate)
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

    if (translate && !res.empty())
    {
        res = LLTrans::getString(res);
    }
    return res;
}

// LLKeyConflictHandler

S32 LLKeyConflictHandler::sTemporaryFileUseCount = 0;

LLKeyConflictHandler::LLKeyConflictHandler()
:   mHasUnsavedChanges(false),
    mUsesTemporaryFile(false),
    mLoadMode(MODE_COUNT)
{
}

LLKeyConflictHandler::LLKeyConflictHandler(ESourceMode mode)
:   mHasUnsavedChanges(false),
    mUsesTemporaryFile(false),
    mLoadMode(mode)
{
    loadFromSettings(mode);
}

LLKeyConflictHandler::~LLKeyConflictHandler()
{
    clearUnsavedChanges();
    // Note: does not reset bindings if temporary file was used
}

bool LLKeyConflictHandler::canHandleControl(const std::string &control_name, EMouseClickType mouse_ind, KEY key, MASK mask)
{
    return mControlsMap[control_name].canHandle(mouse_ind, key, mask);
}

bool LLKeyConflictHandler::canHandleKey(const std::string &control_name, KEY key, MASK mask)
{
    return canHandleControl(control_name, CLICK_NONE, key, mask);
}

bool LLKeyConflictHandler::canHandleMouse(const std::string &control_name, EMouseClickType mouse_ind, MASK mask)
{
    return canHandleControl(control_name, mouse_ind, KEY_NONE, mask);
}

bool LLKeyConflictHandler::canHandleMouse(const std::string &control_name, S32 mouse_ind, MASK mask)
{
    return canHandleControl(control_name, (EMouseClickType)mouse_ind, KEY_NONE, mask);
}

bool LLKeyConflictHandler::canAssignControl(const std::string &control_name)
{
    control_map_t::iterator iter = mControlsMap.find(control_name);
    if (iter != mControlsMap.end())
    {
        return iter->second.mAssignable;
    }
    // If we don't know this control, means it wasn't assigned by user yet and thus is editable
    return true;
}

// static
bool LLKeyConflictHandler::isReservedByMenu(const KEY &key, const MASK &mask)
{
    if (key == KEY_NONE)
    {
        return false;
    }
    return (gMenuBarView && gMenuBarView->hasAccelerator(key, mask))
           || (gLoginMenuBarView && gLoginMenuBarView->hasAccelerator(key, mask));
}

// static
bool LLKeyConflictHandler::isReservedByMenu(const LLKeyData &data)
{
    if (data.mMouse != CLICK_NONE || data.mKey == KEY_NONE)
    {
        return false;
    }
    return (gMenuBarView && gMenuBarView->hasAccelerator(data.mKey, data.mMask))
           || (gLoginMenuBarView && gLoginMenuBarView->hasAccelerator(data.mKey, data.mMask));
}

bool LLKeyConflictHandler::registerControl(const std::string &control_name, U32 index, EMouseClickType mouse, KEY key, MASK mask, bool ignore_mask)
{
    if (control_name.empty())
    {
        return false;
    }
    LLKeyConflict &type_data = mControlsMap[control_name];
    if (!type_data.mAssignable)
    {
        // Example: user tried to assign camera spin to all modes, but first person mode doesn't support it
        return false;
    }
    LLKeyData data(mouse, key, mask, ignore_mask);
    if (type_data.mKeyBind.getKeyData(index) == data)
    {
        return true;
    }
    if (isReservedByMenu(data))
    {
        return false;
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

bool LLKeyConflictHandler::clearControl(const std::string &control_name, U32 data_index)
{
    if (control_name.empty())
    {
        return false;
    }
    LLKeyConflict &type_data = mControlsMap[control_name];
    if (!type_data.mAssignable)
    {
        // Example: user tried to assign camera spin to all modes, but first person mode doesn't support it
        return false;
    }
    type_data.mKeyBind.resetKeyData(data_index);
    mHasUnsavedChanges = true;
    return true;
}

LLKeyData LLKeyConflictHandler::getControl(const std::string &control_name, U32 index)
{
    if (control_name.empty())
    {
        return LLKeyData();
    }
    return mControlsMap[control_name].getKeyData(index);
}

bool LLKeyConflictHandler::isControlEmpty(const std::string &control_name)
{
    if (control_name.empty())
    {
        return true;
    }
    return mControlsMap[control_name].mKeyBind.isEmpty();
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

    result += string_from_mouse(keydata.mMouse, true);

    return result;
}

std::string LLKeyConflictHandler::getControlString(const std::string &control_name, U32 index)
{
    if (control_name.empty())
    {
        return "";
    }
    return getStringFromKeyData(mControlsMap[control_name].getKeyData(index));
}

void LLKeyConflictHandler::loadFromControlSettings(const std::string &name)
{
    LLControlVariablePtr var = gSavedSettings.getControl(name);
    if (var)
    {
        LLKeyBind bind(var->getValue());
        LLKeyConflict key(bind, true, 0);
        mControlsMap[name] = key;
    }
}

void LLKeyConflictHandler::loadFromSettings(const LLViewerInput::KeyMode& keymode, control_map_t *destination)
{
    for (LLInitParam::ParamIterator<LLViewerInput::KeyBinding>::const_iterator it = keymode.bindings.begin(),
        end_it = keymode.bindings.end();
        it != end_it;
    ++it)
    {
        KEY key;
        MASK mask;
        EMouseClickType mouse = CLICK_NONE;
        if (it->mouse.isProvided())
        {
            LLViewerInput::mouseFromString(it->mouse.getValue(), &mouse);
        }
        if (it->key.getValue().empty())
        {
            key = KEY_NONE;
        }
        else
        {
            LLKeyboard::keyFromString(it->key, &key);
        }
        LLKeyboard::maskFromString(it->mask, &mask);
        // Note: it->command is also the name of UI element, howhever xml we are loading from
        // might not know all the commands, so UI will have to know what to fill by its own
        // Assumes U32_MAX conflict mask, and is assignable by default,
        // but assignability might have been overriden by generatePlaceholders.
        LLKeyConflict &type_data = (*destination)[it->command];
        type_data.mKeyBind.addKeyData(mouse, key, mask, true);
    }
}

bool LLKeyConflictHandler::loadFromSettings(const ESourceMode &load_mode, const std::string &filename, control_map_t *destination)
{
    if (filename.empty())
    {
        return false;
    }

    bool res = false;

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
                res = true;
            }
            break;
        case MODE_THIRD_PERSON:
            if (keys.third_person.isProvided())
            {
                loadFromSettings(keys.third_person, destination);
                res = true;
            }
            break;
        case MODE_EDIT_AVATAR:
            if (keys.edit_avatar.isProvided())
            {
                loadFromSettings(keys.edit_avatar, destination);
                res = true;
            }
            break;
        case MODE_SITTING:
            if (keys.sitting.isProvided())
            {
                loadFromSettings(keys.sitting, destination);
                res = true;
            }
            break;
        default:
            LL_ERRS() << "Not implememted mode " << load_mode << LL_ENDL;
            break;
        }
    }
    return res;
}

void LLKeyConflictHandler::loadFromSettings(ESourceMode load_mode)
{
    mControlsMap.clear();
    mDefaultsMap.clear();

    // E.X. In case we need placeholder keys for conflict resolution.
    generatePlaceholders(load_mode);

    if (load_mode == MODE_SAVED_SETTINGS)
    {
        // load settings clss knows about, but it also possible to load settings by name separately
        const S32 size = std::extent<decltype(saved_settings_key_controls)>::value;
        for (U32 i = 0; i < size; i++)
        {
            loadFromControlSettings(saved_settings_key_controls[i]);
        }
    }
    else
    {
        // load defaults
        std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, filename_default);
        if (!loadFromSettings(load_mode, filename, &mDefaultsMap))
        {
            LL_WARNS() << "Failed to load default settings, aborting" << LL_ENDL;
            return;
        }

        // load user's
        filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename_default);
        if (!gDirUtilp->fileExists(filename) || !loadFromSettings(load_mode, filename, &mControlsMap))
        {
            // mind placeholders
            mControlsMap.insert(mDefaultsMap.begin(), mDefaultsMap.end());
        }
    }
    mLoadMode = load_mode;
}

void LLKeyConflictHandler::saveToSettings(bool temporary)
{
    if (mControlsMap.empty())
    {
        return;
    }

    if (mLoadMode == MODE_SAVED_SETTINGS)
    {
        // Does not support 'temporary', preferences handle that themself
        // so in case of saved settings we just do not clear mHasUnsavedChanges
        control_map_t::iterator iter = mControlsMap.begin();
        control_map_t::iterator end = mControlsMap.end();

        for (; iter != end; ++iter)
        {
            if (iter->first.empty())
            {
                continue;
            }

            LLKeyConflict &key = iter->second;
            key.mKeyBind.trimEmpty();
            if (!key.mAssignable)
            {
                continue;
            }

            if (gSavedSettings.controlExists(iter->first))
            {
                gSavedSettings.setLLSD(iter->first, key.mKeyBind.asLLSD());
            }
            else if (!key.mKeyBind.empty())
            {
                // Note: this is currently not in use, might be better for load mechanics to ask for and retain control group
                // otherwise settings loaded from other control groups will end in gSavedSettings
                LL_INFOS() << "Creating new keybinding " << iter->first << LL_ENDL;
                gSavedSettings.declareLLSD(iter->first, key.mKeyBind.asLLSD(), "comment", LLControlVariable::PERSIST_ALWAYS);
            }
        }
    }
    else
    {
        // Determine what file to load and load full copy of that file
        std::string filename;

        if (temporary)
        {
            filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename_temporary);
            if (!gDirUtilp->fileExists(filename))
            {
                filename.clear();
            }
        }

        if (filename.empty())
        {
            filename = gDirUtilp->findFile(filename_default,
                gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, ""),
                gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));
        }

        LLViewerInput::Keys keys;
        LLSimpleXUIParser parser;

        if (parser.readXUI(filename, keys)
            && keys.validateBlock())
        {
            // replace category we edited

            // mode is a HACK to correctly reset bindings without reparsing whole file and avoid doing
            // own param container (which will face issues with inasseesible members of LLInitParam)
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
                    if (iter->first.empty())
                    {
                        continue;
                    }

                    LLKeyConflict &key = iter->second;
                    key.mKeyBind.trimEmpty();
                    if (key.mKeyBind.empty() || !key.mAssignable)
                    {
                        continue;
                    }

                    LLKeyData data = key.mKeyBind.getKeyData(i);
                    // Still write empty LLKeyData to make sure we will maintain UI position
                    if (data.mKey == KEY_NONE)
                    {
                        // Might be better idea to be consistent and use NONE. LLViewerInput can work with both cases
                        binding.key = "";
                    }
                    else
                    {
                        binding.key = LLKeyboard::stringFromKey(data.mKey, false /*Do not localize*/);
                    }
                    binding.mask = string_from_mask(data.mMask);
                    if (data.mMouse == CLICK_NONE)
                    {
                        binding.mouse.setProvided(false);
                    }
                    else
                    {
                        // set() because 'optional', for compatibility purposes
                        // just copy old keys.xml and rename to key_bindings.xml, it should work
                        binding.mouse.set(string_from_mouse(data.mMouse, false), true);
                    }
                    binding.command = iter->first;
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
                LL_ERRS() << "Not implememted mode " << mLoadMode << LL_ENDL;
                break;
            }

            if (temporary)
            {
                // write to temporary xml and use it for gViewerInput
                filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename_temporary);
                if (!mUsesTemporaryFile)
                {
                    mUsesTemporaryFile = true;
                    sTemporaryFileUseCount++;
                }
            }
            else
            {
                // write back to user's xml and use it for gViewerInput
                filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename_default);
                // Don't reset mUsesTemporaryFile, it will be reset at cleanup stage
            }

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
                // Ideally instead of rebinding immediately we should shedule
                // the rebind since single file can have multiple handlers,
                // one per mode, saving simultaneously.
                // Or whatever uses LLKeyConflictHandler should control the process.
                gViewerInput.loadBindingsXML(filename);
            }
        }
    }

#if 1
    // Legacy support
    // Remove #if-#endif section half a year after DRTVWR-501 releases.
    // Update legacy settings in settings.xml
    // We only care for third person view since legacy settings can't store
    // more than one mode.
    // We are saving this even if we are in temporary mode - preferences
    // will restore values on cancel
    if (mLoadMode == MODE_THIRD_PERSON && mHasUnsavedChanges)
    {
        bool value = canHandleMouse("walk_to", CLICK_DOUBLELEFT, MASK_NONE);
        gSavedSettings.setBOOL("DoubleClickAutoPilot", value);

        value = canHandleMouse("walk_to", CLICK_LEFT, MASK_NONE);
        gSavedSettings.setBOOL("ClickToWalk", value);

        // new method can save both toggle and push-to-talk values simultaneously,
        // but legacy one can save only one. It also doesn't support mask.
        LLKeyData data = getControl("toggle_voice", 0);
        bool can_toggle = !data.isEmpty();
        if (!can_toggle)
        {
            data = getControl("voice_follow_key", 0);
        }

        gSavedSettings.setBOOL("PushToTalkToggle", can_toggle);
        if (data.isEmpty())
        {
            // legacy viewer has a bug that might crash it if NONE value is assigned.
            // just reset to default
            gSavedSettings.getControl("PushToTalkButton")->resetToDefault(false);
        }
        else
        {
            if (data.mKey != KEY_NONE)
            {
                gSavedSettings.setString("PushToTalkButton", LLKeyboard::stringFromKey(data.mKey));
            }
            else
            {
                std::string ctrl_value;
                switch (data.mMouse)
                {
                case CLICK_MIDDLE:
                    ctrl_value = "MiddleMouse";
                    break;
                case CLICK_BUTTON4:
                    ctrl_value = "MouseButton4";
                    break;
                case CLICK_BUTTON5:
                    ctrl_value = "MouseButton5";
                    break;
                default:
                    ctrl_value = "MiddleMouse";
                    break;
                }
                gSavedSettings.setString("PushToTalkButton", ctrl_value);
            }
        }
    }
#endif

    if (mLoadMode == MODE_THIRD_PERSON && mHasUnsavedChanges)
    {
        // Map floater should react to doubleclick if doubleclick for teleport is set
        // Todo: Seems conterintuitive for map floater to share inworld controls
        // after these changes release, discuss with UI UX engineer if this should just
        // be set to 1 by default (before release this also doubles as legacy support)
        bool value = canHandleMouse("teleport_to", CLICK_DOUBLELEFT, MASK_NONE);
        gSavedSettings.setBOOL("DoubleClickTeleport", value);
    }

    if (!temporary)
    {
        // will remove any temporary file if there were any
        clearUnsavedChanges();
    }
}

LLKeyData LLKeyConflictHandler::getDefaultControl(const std::string &control_name, U32 index)
{
    if (control_name.empty())
    {
        return LLKeyData();
    }
    if (mLoadMode == MODE_SAVED_SETTINGS)
    {
        LLControlVariablePtr var = gSavedSettings.getControl(control_name);
        if (var)
        {
            return LLKeyBind(var->getDefault()).getKeyData(index);
        }
        return LLKeyData();
    }
    else
    {
        control_map_t::iterator iter = mDefaultsMap.find(control_name);
        if (iter != mDefaultsMap.end())
        {
            return iter->second.mKeyBind.getKeyData(index);
        }
        return LLKeyData();
    }
}

void LLKeyConflictHandler::resetToDefault(const std::string &control_name, U32 index)
{
    if (control_name.empty())
    {
        return;
    }
    LLKeyData data = getDefaultControl(control_name, index);

    if (data != mControlsMap[control_name].getKeyData(index))
    {
        // reset controls that might have been switched to our current control
        removeConflicts(data, mControlsMap[control_name].mConflictMask);
        mControlsMap[control_name].setKeyData(data, index);
    }
}

void LLKeyConflictHandler::resetToDefaultAndResolve(const std::string &control_name, bool ignore_conflicts)
{
    if (control_name.empty())
    {
        return;
    }
    if (mLoadMode == MODE_SAVED_SETTINGS)
    {
        LLControlVariablePtr var = gSavedSettings.getControl(control_name);
        if (var)
        {
            LLKeyBind bind(var->getDefault());
            if (!ignore_conflicts)
            {
                for (S32 i = 0; i < bind.getDataCount(); ++i)
                {
                    removeConflicts(bind.getKeyData(i), mControlsMap[control_name].mConflictMask);
                }
            }
            mControlsMap[control_name].mKeyBind = bind;
        }
        else
        {
            mControlsMap[control_name].mKeyBind.clear();
        }
    }
    else
    {
        control_map_t::iterator iter = mDefaultsMap.find(control_name);
        if (iter != mDefaultsMap.end())
        {
            if (!ignore_conflicts)
            {
                for (S32 i = 0; i < iter->second.mKeyBind.getDataCount(); ++i)
                {
                    removeConflicts(iter->second.mKeyBind.getKeyData(i), mControlsMap[control_name].mConflictMask);
                }
            }
            mControlsMap[control_name].mKeyBind = iter->second.mKeyBind;
        }
        else
        {
            mControlsMap[control_name].mKeyBind.clear();
        }
    }
}

void LLKeyConflictHandler::resetToDefault(const std::string &control_name)
{
    // reset specific binding without ignoring conflicts
    resetToDefaultAndResolve(control_name, false);
}

void LLKeyConflictHandler::resetToDefaults(ESourceMode mode)
{
    if (mode == MODE_SAVED_SETTINGS)
    {
        control_map_t::iterator iter = mControlsMap.begin();
        control_map_t::iterator end = mControlsMap.end();

        for (; iter != end; ++iter)
        {
            resetToDefaultAndResolve(iter->first, true);
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
    if (clearUnsavedChanges())
    {
        // temporary file was removed, this means we were using it and need to reload keyboard's bindings
        resetKeyboardBindings();
    }
    mControlsMap.clear();
    mDefaultsMap.clear();
}

// static
void LLKeyConflictHandler::resetKeyboardBindings()
{
    // Try to load User's bindings first
    std::string key_bindings_file = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename_default);
    if (!gDirUtilp->fileExists(key_bindings_file) || !gViewerInput.loadBindingsXML(key_bindings_file))
    {
        // Failed to load custom bindings, try default ones
        key_bindings_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, filename_default);
        if (!gViewerInput.loadBindingsXML(key_bindings_file))
        {
            LL_ERRS("InitInfo") << "Unable to open default key bindings from " << key_bindings_file << LL_ENDL;
        }
    }
}

void LLKeyConflictHandler::generatePlaceholders(ESourceMode load_mode)
{
    // These controls are meant to cause conflicts when user tries to assign same control somewhere else
    // also this can be used to pre-record controls that should not conflict or to assign conflict groups/masks

    if (load_mode == MODE_FIRST_PERSON)
    {
        // First person view doesn't support camera controls
        // Note: might be better idea to just load these from control_table_contents_camera.xml
        // or to pass from floaterpreferences when it loads said file
        registerTemporaryControl("look_up");
        registerTemporaryControl("look_down");
        registerTemporaryControl("move_forward");
        registerTemporaryControl("move_backward");
        registerTemporaryControl("move_forward_fast");
        registerTemporaryControl("move_backward_fast");
        registerTemporaryControl("spin_over");
        registerTemporaryControl("spin_under");
        registerTemporaryControl("pan_up");
        registerTemporaryControl("pan_down");
        registerTemporaryControl("pan_left");
        registerTemporaryControl("pan_right");
        registerTemporaryControl("pan_in");
        registerTemporaryControl("pan_out");
        registerTemporaryControl("spin_around_ccw");
        registerTemporaryControl("spin_around_cw");

        // control_table_contents_editing.xml
        registerTemporaryControl("edit_avatar_spin_ccw");
        registerTemporaryControl("edit_avatar_spin_cw");
        registerTemporaryControl("edit_avatar_spin_over");
        registerTemporaryControl("edit_avatar_spin_under");
        registerTemporaryControl("edit_avatar_move_forward");
        registerTemporaryControl("edit_avatar_move_backward");

        // no autopilot or teleport
        registerTemporaryControl("walk_to");
        registerTemporaryControl("teleport_to");
    }

    if (load_mode == MODE_EDIT_AVATAR)
    {
        // no autopilot or teleport
        registerTemporaryControl("walk_to");
        registerTemporaryControl("teleport_to");
    }

    if (load_mode == MODE_SITTING)
    {
        // no autopilot
        registerTemporaryControl("walk_to");
    }
    else 
    {
        // sitting related functions should only be avaliable in sitting mode
        registerTemporaryControl("move_forward_sitting");
        registerTemporaryControl("move_backward_sitting");
        registerTemporaryControl("spin_over_sitting");
        registerTemporaryControl("spin_under_sitting");
        registerTemporaryControl("spin_around_ccw_sitting");
        registerTemporaryControl("spin_around_cw_sitting");
    }
}

bool LLKeyConflictHandler::removeConflicts(const LLKeyData &data, const U32 &conlict_mask)
{
    if (conlict_mask == CONFLICT_NOTHING)
    {
        // Can't conflict
        return true;
    }
    std::map<std::string, S32> conflict_list;
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

    std::map<std::string, S32>::iterator cnflct_iter = conflict_list.begin();
    std::map<std::string, S32>::iterator cnflct_end = conflict_list.end();
    for (; cnflct_iter != cnflct_end; ++cnflct_iter)
    {
        mControlsMap[cnflct_iter->first].mKeyBind.resetKeyData(cnflct_iter->second);
    }
    return true;
}

void LLKeyConflictHandler::registerTemporaryControl(const std::string &control_name, EMouseClickType mouse, KEY key, MASK mask, U32 conflict_mask)
{
    LLKeyConflict *type_data = &mControlsMap[control_name];
    type_data->mAssignable = false;
    type_data->mConflictMask = conflict_mask;
    type_data->mKeyBind.addKeyData(mouse, key, mask, false);
}

void LLKeyConflictHandler::registerTemporaryControl(const std::string &control_name, U32 conflict_mask)
{
    LLKeyConflict *type_data = &mControlsMap[control_name];
    type_data->mAssignable = false;
    type_data->mConflictMask = conflict_mask;
}

bool LLKeyConflictHandler::clearUnsavedChanges()
{
    bool result = false;
    mHasUnsavedChanges = false;

    if (mUsesTemporaryFile)
    {
        mUsesTemporaryFile = false;
        sTemporaryFileUseCount--;
        if (!sTemporaryFileUseCount)
        {
            result = clearTemporaryFile();
        }
        // else: might be usefull to overwrite content of temp file with defaults
        // but at the moment there is no such need
    }
    return result;
}

//static
bool LLKeyConflictHandler::clearTemporaryFile()
{
    // At the moment single file needs five handlers (one per mode), so doing this
    // will remove file for all hadlers
    std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename_temporary);
    if (gDirUtilp->fileExists(filename))
    {
        LLFile::remove(filename);
        return true;
    }
    return false;
}

