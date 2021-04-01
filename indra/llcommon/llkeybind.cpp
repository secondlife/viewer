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

LLKeyData::LLKeyData()
    :
    mMouse(CLICK_NONE),
    mKey(KEY_NONE),
    mMask(MASK_NONE),
    mIgnoreMasks(false)
{
}

LLKeyData::LLKeyData(EMouseClickType mouse, KEY key, MASK mask)
    :
    mMouse(mouse),
    mKey(key),
    mMask(mask),
    mIgnoreMasks(false)
{
}

LLKeyData::LLKeyData(EMouseClickType mouse, KEY key, bool ignore_mask)
    :
    mMouse(mouse),
    mKey(key),
    mMask(MASK_NONE),
    mIgnoreMasks(ignore_mask)
{
}

LLKeyData::LLKeyData(EMouseClickType mouse, KEY key, MASK mask, bool ignore_mask)
    :
    mMouse(mouse),
    mKey(key),
    mMask(mask),
    mIgnoreMasks(ignore_mask)
{
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
}

LLKeyData& LLKeyData::operator=(const LLKeyData& rhs)
{
    mMouse = rhs.mMouse;
    mKey = rhs.mKey;
    mMask = rhs.mMask;
    mIgnoreMasks = rhs.mIgnoreMasks;
    return *this;
}

bool LLKeyData::operator==(const LLKeyData& rhs)
{
    if (mMouse != rhs.mMouse) return false;
    if (mKey != rhs.mKey) return false;
    if (mMask != rhs.mMask) return false;
    if (mIgnoreMasks != rhs.mIgnoreMasks) return false;
    return true;
}

bool LLKeyData::operator!=(const LLKeyData& rhs)
{
    if (mMouse != rhs.mMouse) return true;
    if (mKey != rhs.mKey) return true;
    if (mMask != rhs.mMask) return true;
    if (mIgnoreMasks != rhs.mIgnoreMasks) return true;
    return false;
}

bool LLKeyData::canHandle(const LLKeyData& data) const
{
    if (data.mKey == mKey
        && data.mMouse == mMouse
        && ((mIgnoreMasks && (data.mMask & mMask) == mMask) || data.mMask == mMask))
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

bool LLKeyBind::operator==(const LLKeyBind& rhs)
{
    U32 size = mData.size();
    if (size != rhs.mData.size()) return false;

    for (U32 i = 0; i < size; i++)
    {
        if (mData[i] != rhs.mData[i]) return false;
    }

    return true;
}

bool LLKeyBind::operator!=(const LLKeyBind& rhs)
{
    U32 size = mData.size();
    if (size != rhs.mData.size()) return true;

    for (U32 i = 0; i < size; i++)
    {
        if (mData[i] != rhs.mData[i]) return true;
    }

    return false;
}

bool LLKeyBind::isEmpty() const
{
    for (data_vector_t::const_iterator iter = mData.begin(); iter != mData.end(); iter++)
    {
        if (!iter->isEmpty()) return false;
    }
    return true;
}

LLSD LLKeyBind::asLLSD() const
{
    S32 last = mData.size() - 1;
    while (mData[last].empty())
    {
        last--;
    }

    LLSD data;
    for (S32 i = 0; i <= last; ++i)
    {
        // append even if empty to not affect visual representation
        data.append(mData[i].asLLSD());
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

    for (data_vector_t::const_iterator iter = mData.begin(); iter != mData.end(); iter++)
    {
        if (iter->canHandle(mouse, key, mask))
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
        for (data_vector_t::const_iterator iter = mData.begin(); iter != mData.end(); iter++)
        {
            if (iter->mKey == key
                && iter->mMask == mask
                && iter->mMouse == mouse
                && iter->mIgnoreMasks == ignore)
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
        for (data_vector_t::iterator iter = mData.begin(); iter != mData.end(); iter++)
        {
            if (iter->mKey == data.mKey
                && iter->mMouse == data.mMouse
                && iter->mIgnoreMasks == data.mIgnoreMasks
                && iter->mMask == data.mMask)
            {
                // Replacing only fully equal combinations even in case 'ignore' is set
                // Reason: Simplicity and user might decide to do a 'move' command as W and Shift+Ctrl+W, and 'run' as Shift+W
                iter->reset();
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
    S32 last = mData.size() - 1;
    while (last >= 0 && mData[last].empty())
    {
        mData.erase(mData.begin() + last);
        last--;
    }
}

U32 LLKeyBind::getDataCount()
{
    return mData.size();
}

