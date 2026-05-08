/**
 * @file llkeybind.cpp
 * @brief Information about key combinations.
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

#include "linden_common.h"

#include "llkeybind.h"

#include "llsd.h"
#include "llsdutil.h"
#include <algorithm>

LLKeyData::LLKeyData()
    :
    mMouse(CLICK_NONE),
    mKey(KEY_NONE),
    mMask(MASK_NONE),
    mIgnoreMasks(false),
    mControllerActionType(255),
    mControllerAction(0)
{
}

LLKeyData::LLKeyData(EMouseClickType mouse, KEY key, MASK mask)
    :
    mMouse(mouse),
    mKey(key),
    mMask(mask),
    mIgnoreMasks(false),
    mControllerActionType(255),
    mControllerAction(0)
{
}

LLKeyData::LLKeyData(EMouseClickType mouse, KEY key, bool ignore_mask)
    :
    mMouse(mouse),
    mKey(key),
    mMask(MASK_NONE),
    mIgnoreMasks(ignore_mask),
    mControllerActionType(255),
    mControllerAction(0)
{
}

LLKeyData::LLKeyData(EMouseClickType mouse, KEY key, MASK mask, bool ignore_mask)
    :
    mMouse(mouse),
    mKey(key),
    mMask(mask),
    mIgnoreMasks(ignore_mask),
    mControllerActionType(255),
    mControllerAction(0)
{
}

LLKeyData::LLKeyData(U8 actionType, U8 action)
    :
    mMouse(CLICK_NONE),
    mKey(KEY_NONE),
    mMask(MASK_NONE),
    mIgnoreMasks(true),
    mControllerActionType(actionType),
    mControllerAction(action)
{

}

static std::string controllerStringFromKeyData(const U8 actionType, const U8 action)
{
    switch (actionType)
    {
        case 0:
        {
            switch(action)
            {
                case 0: return "axis_left";
                case 1: return "axis_right";
                case 2: return "axis_forward";
                case 3: return "axis_backward";
                case 4: return "axis_turn_left";
                case 5: return "axis_turn_right";
                case 6: return "axis_look_up";
                case 7: return "axis_look_down";
                case 8: return "axis_up";
                case 9: return "axis_down";
                case 10: return "axis_roll_left";
                case 11: return "axis_roll_right";
            }
        }
        case 1:
        {
            switch(action)
            {
                case 0: return "button_a";
                case 1: return "button_b";
                case 2: return "button_x";
                case 3: return "button_y";
                case 4: return "button_back";
                case 5: return "button_start";
                case 6: return "button_guide";
                case 7: return "button_leftstick";
                case 8: return "button_rightstick";
                case 9: return "button_leftshoulder";
                case 10: return "button_rightshoulder";
                case 11: return "button_dpad_up";
                case 12: return "button_dpad_down";
                case 13: return "button_dpad_left";
                case 14: return "button_dpad_right";
                case 15: return "button_misc1";
                case 16: return "button_paddle1";
                case 17: return "button_paddle2";
                case 18: return "button_paddle3";
                case 19: return "button_paddle4";
                case 20: return "button_touchpad";
            }
        }
    }
    return "NONE";
}

static bool controllerActionFromString(const std::string& string,  U8& actionType, U8& action)
{
    actionType = 255;
    if(LLStringUtil::startsWith(string, "axis_")) {
        actionType = 0;
        if(string == "axis_left")
        {
            action = 0;
        }
        else if(string == "axis_right")
        {
            action = 1;
        }
        else if(string == "axis_forward")
        {
            action = 2;
        }
        else if(string == "axis_backward")
        {
            action = 3;
        }
        else if(string == "axis_turn_left")
        {
            action = 4;
        }
        else if(string == "axis_turn_right")
        {
            action = 5;
        }
        else if(string == "axis_look_up")
        {
            action = 6;
        }
        else if(string == "axis_look_down")
        {
            action = 7;
        }
        else if(string == "axis_up")
        {
            action = 8;
        }
        else if(string == "axis_down")
        {
            action = 9;
        }
        else if(string == "axis_roll_left")
        {
            action = 10;
        }
        else if(string == "axis_roll_right")
        {
            action = 11;
        }
        else
        {
            actionType = 255;
            return false;
        }
        return true;
    }
    else if(LLStringUtil::startsWith(string, "button_")) {
        actionType = 1;
        if(string == "button_a")
        {
            action = 9;
        }
        else if(string == "button_b")
        {
            action = 1;
        }
        else if(string == "button_x")
        {
            action = 2;
        }
        else if(string == "button_y")
        {
            action = 3;
        }
        else if(string == "button_back")
        {
            action = 4;
        }
        else if(string == "button_start")
        {
            action = 5;
        }
        else if(string == "button_guide")
        {
            action = 6;
        }
        else if(string == "button_leftstick")
        {
            action = 7;
        }
        else if(string == "button_rightstick")
        {
            action = 8;
        }
        else if(string == "button_leftshoulder")
        {
            action = 9;
        }
        else if(string == "button_rightshoulder")
        {
            action = 10;
        }
        else if(string == "button_dpad_up")
        {
            action = 11;
        }
        else if(string == "button_dpad_down")
        {
            action = 12;
        }
        else if(string == "button_dpad_left")
        {
            action = 13;
        }
        else if(string == "button_dpad_right")
        {
            action = 14;
        }
        else if(string == "button_misc1")
        {
            action = 15;
        }
        else if(string == "button_paddle1")
        {
            action = 16;
        }
        else if(string == "button_paddle2")
        {
            action = 17;
        }
        else if(string == "button_paddle3")
        {
            action = 18;
        }
        else if(string == "button_paddle4")
        {
            action = 19;
        }
        else if(string == "button_touchpad")
        {
            action = 20;
        }
        else
        {
            actionType = 255;
            return false;
        }
        return true;
    }
    return false;
}

LLKeyData::LLKeyData(const LLSD &key_data)
{
    if (key_data.has("mouse"))
    {
        mMouse = (EMouseClickType)key_data["mouse"].asInteger();
    }
    if (key_data.has("key"))
    {
        mKey = key_data["key"].asInteger();
    }
    if (key_data.has("ignore_accelerators"))
    {
        mIgnoreMasks = key_data["ignore_accelerators"];
    }
    if (key_data.has("mask"))
    {
        mMask = key_data["mask"].asInteger();
    }
    if (key_data.has("controller"))
    {
        U8 actionType = 255;
        U8 action = 0;
        controllerActionFromString(key_data["controller"],actionType, action);
        mControllerActionType = actionType;
        mControllerAction = action;
    }
}

LLSD LLKeyData::asLLSD() const
{
    LLSD data;
    data["mouse"] = (LLSD::Integer)mMouse;
    data["key"] = (LLSD::Integer)mKey;
    data["mask"] = (LLSD::Integer)mMask;
    if (mIgnoreMasks)
    {
        data["ignore_accelerators"] = (LLSD::Boolean)mIgnoreMasks;
    }
    data["controller"] = (LLSD::String)controllerStringFromKeyData(mControllerActionType, mControllerAction);
    return data;
}

bool LLKeyData::isEmpty() const
{
    return mMouse == CLICK_NONE && mKey == KEY_NONE;
}

void LLKeyData::reset()
{
    mMouse = CLICK_NONE;
    mKey = KEY_NONE;
    mMask = MASK_NONE;
    mIgnoreMasks = false;
    mControllerActionType = 255;
    mControllerAction = 0;
}

bool LLKeyData::operator==(const LLKeyData& rhs) const
{
    if (mMouse != rhs.mMouse) return false;
    if (mKey != rhs.mKey) return false;
    if (mMask != rhs.mMask) return false;
    if (mIgnoreMasks != rhs.mIgnoreMasks) return false;
    if (mControllerActionType != rhs.mControllerActionType) return false;
    if (mControllerAction != rhs.mControllerAction) return false;
    return true;
}

bool LLKeyData::operator!=(const LLKeyData& rhs) const
{
    if (mMouse != rhs.mMouse) return true;
    if (mKey != rhs.mKey) return true;
    if (mMask != rhs.mMask) return true;
    if (mIgnoreMasks != rhs.mIgnoreMasks) return true;
    if (mControllerActionType != rhs.mControllerActionType) return true;
    if (mControllerAction != rhs.mControllerAction) return true;
    return false;
}

bool LLKeyData::canHandle(const LLKeyData& data) const
{
    if (data.mKey == mKey
        && data.mMouse == mMouse
        && ((mIgnoreMasks && (data.mMask & mMask) == mMask) || data.mMask == mMask)
        && (mControllerActionType == data.mControllerActionType && mControllerAction == data.mControllerAction)
    )
    {
        return true;
    }
    return false;
}

bool LLKeyData::canHandle(EMouseClickType mouse, KEY key, MASK mask) const
{
    if (mouse == mMouse
        && key == mKey
        && ((mIgnoreMasks && (mask & mMask) == mMask) || mask == mMask))
    {
        return true;
    }
    return false;
}

// LLKeyBind

LLKeyBind::LLKeyBind(const LLSD &key_bind)
{
    if (key_bind.isArray())
    {
        for (LLSD::array_const_iterator data = key_bind.beginArray(), endLists = key_bind.endArray();
            data != endLists;
            data++
            )
        {
            mData.push_back(LLKeyData(*data));
        }
    }
}

bool LLKeyBind::operator==(const LLKeyBind& rhs) const
{
    auto size = mData.size();
    if (size != rhs.mData.size()) return false;

    for (size_t i = 0; i < size; i++)
    {
        if (mData[i] != rhs.mData[i]) return false;
    }

    return true;
}

bool LLKeyBind::operator!=(const LLKeyBind& rhs) const
{
    auto size = mData.size();
    if (size != rhs.mData.size()) return true;

    for (U32 i = 0; i < size; i++)
    {
        if (mData[i] != rhs.mData[i]) return true;
    }

    return false;
}

bool LLKeyBind::isEmpty() const
{
    for (const LLKeyData& key_data : mData)
    {
        if (!key_data.isEmpty()) return false;
    }
    return true;
}

LLKeyBind::data_vector_t::const_iterator LLKeyBind::endNonEmpty() const
{
    // search backwards for last non-empty entry, then turn back into forwards
    // iterator (.base() call)
    return std::find_if_not(mData.rbegin(), mData.rend(),
                            [](const auto& kdata){ return kdata.empty(); }).base();
}

LLSD LLKeyBind::asLLSD() const
{
    LLSD data;
    for (const LLKeyData& key_data : mData)
    {
        // append intermediate entries even if empty to not affect visual
        // representation
        data.append(key_data.asLLSD());
    }
    return data;
}

bool LLKeyBind::canHandle(EMouseClickType mouse, KEY key, MASK mask) const
{
    if (mouse == CLICK_NONE && key == KEY_NONE)
    {
        // assume placeholder
        return false;
    }

    for (const LLKeyData& key_data : mData)
    {
        if (key_data.canHandle(mouse, key, mask))
        {
            return true;
        }
    }
    return false;
}

bool LLKeyBind::canHandleKey(KEY key, MASK mask) const
{
    return canHandle(CLICK_NONE, key, mask);
}

bool LLKeyBind::canHandleMouse(EMouseClickType mouse, MASK mask) const
{
    return canHandle(mouse, KEY_NONE, mask);
}

bool LLKeyBind::hasKeyData(EMouseClickType mouse, KEY key, MASK mask, bool ignore) const
{
    if (mouse != CLICK_NONE || key != KEY_NONE)
    {
        for (const LLKeyData& key_data : mData)
        {
            if (key_data.mKey == key
                && key_data.mMask == mask
                && key_data.mMouse == mouse
                && key_data.mIgnoreMasks == ignore)
            {
                return true;
            }
        }
    }
    return false;
}

bool LLKeyBind::hasKeyData(const LLKeyData& data) const
{
    return hasKeyData(data.mMouse, data.mKey, data.mMask, data.mIgnoreMasks);
}

bool LLKeyBind::hasKeyData(U32 index) const
{
    return mData.size() > index;
}

S32 LLKeyBind::findKeyData(EMouseClickType mouse, KEY key, MASK mask, bool ignore) const
{
    if (mouse != CLICK_NONE || key != KEY_NONE)
    {
        for (S32 i = 0; i < mData.size(); ++i)
        {
            if (mData[i].mKey == key
                && mData[i].mMask == mask
                && mData[i].mMouse == mouse
                && mData[i].mIgnoreMasks == ignore)
            {
                return i;
            }
        }
    }
    return -1;
}

S32 LLKeyBind::findKeyData(const LLKeyData& data) const
{
    return findKeyData(data.mMouse, data.mKey, data.mMask, data.mIgnoreMasks);
}

LLKeyData LLKeyBind::getKeyData(U32 index) const
{
    if (mData.size() > index)
    {
        return mData[index];
    }
    return LLKeyData();
}

bool LLKeyBind::addKeyData(EMouseClickType mouse, KEY key, MASK mask, bool ignore)
{
    if (!hasKeyData(mouse, key, mask, ignore))
    {
        mData.push_back(LLKeyData(mouse, key, mask, ignore));
        return true;
    }
    return false;
}

bool LLKeyBind::addKeyData(const LLKeyData& data)
{
    if (!hasKeyData(data))
    {
        mData.push_back(data);
        return true;
    }
    return false;
}

void LLKeyBind::replaceKeyData(EMouseClickType mouse, KEY key, MASK mask, bool ignore, U32 index)
{
    replaceKeyData(LLKeyData(mouse, key, mask, ignore), index);
}

void LLKeyBind::replaceKeyData(const LLKeyData& data, U32 index)
{
    if (!data.isEmpty())
    {
        // if both click and key are none (isEmpty()), we are inserting a placeholder, we don't want to reset anything
        // otherwise reset identical key
        for (LLKeyData& key_data : mData)
        {
            if (key_data.mKey == data.mKey
                && key_data.mMouse == data.mMouse
                && key_data.mIgnoreMasks == data.mIgnoreMasks
                && key_data.mMask == data.mMask)
            {
                // Replacing only fully equal combinations even in case 'ignore' is set
                // Reason: Simplicity and user might decide to do a 'move' command as W and Shift+Ctrl+W, and 'run' as Shift+W
                key_data.reset();
                break;
            }
        }
    }
    if (mData.size() <= index)
    {
        mData.resize(index + 1);
    }
    mData[index] = data;
}

void LLKeyBind::resetKeyData(S32 index)
{
    if (mData.size() > index)
    {
        mData[index].reset();
    }
}

void LLKeyBind::trimEmpty()
{
    mData.erase(endNonEmpty(), mData.end());
}

size_t LLKeyBind::getDataCount()
{
    return mData.size();
}
