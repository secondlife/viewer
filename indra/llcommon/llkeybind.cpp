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
    : mMouse(CLICK_NONE), mKey(KEY_NONE), mMask(MASK_NONE)
{
}

LLKeyData::LLKeyData(const LLSD &key_data)
{
    mMouse = (EMouseClickType)key_data["mouse"].asInteger();
    mKey = key_data["key"].asInteger();
    mMask = key_data["mask"].asInteger();
}

LLSD LLKeyData::asLLSD() const
{
    LLSD data;
    data["mouse"] = (LLSD::Integer)mMouse;
    data["key"] = (LLSD::Integer)mKey;
    data["mask"] = (LLSD::Integer)mMask;
    return data;
}

bool LLKeyData::isEmpty() const
{
    return mMouse == CLICK_NONE && mKey == KEY_NONE &&  mMask == MASK_NONE;
}

void LLKeyData::reset()
{
    mMouse = CLICK_NONE;
    mKey = KEY_NONE;
    mMask = MASK_NONE;
}

LLKeyData& LLKeyData::operator=(const LLKeyData& rhs)
{
    mMouse = rhs.mMouse;
    mKey = rhs.mKey;
    mMask = rhs.mMask;
    return *this;
}

// LLKeyBind

LLKeyBind::LLKeyBind(const LLSD &key_bind)
{
    if (key_bind.has("DataPrimary"))
    {
        mDataPrimary = LLKeyData(key_bind["DataPrimary"]);
    }
    if (key_bind.has("DataSecondary"))
    {
        mDataSecondary = LLKeyData(key_bind["DataSecondary"]);
    }
}

bool LLKeyBind::operator==(const LLKeyBind& rhs)
{
    if (mDataPrimary.mMouse != rhs.mDataPrimary.mMouse) return false;
    if (mDataPrimary.mKey != rhs.mDataPrimary.mKey) return false;
    if (mDataPrimary.mMask != rhs.mDataPrimary.mMask) return false;
    if (mDataSecondary.mMouse != rhs.mDataSecondary.mMouse) return false;
    if (mDataSecondary.mKey != rhs.mDataSecondary.mKey) return false;
    if (mDataSecondary.mMask != rhs.mDataSecondary.mMask) return false;
    return true;
}

bool LLKeyBind::empty()
{
    if (mDataPrimary.mMouse != CLICK_NONE) return false;
    if (mDataPrimary.mKey != KEY_NONE) return false;
    if (mDataPrimary.mMask != MASK_NONE) return false;
    if (mDataSecondary.mMouse != CLICK_NONE) return false;
    if (mDataSecondary.mKey != KEY_NONE) return false;
    if (mDataSecondary.mMask != MASK_NONE) return false;
    return false;
}

LLSD LLKeyBind::asLLSD() const
{
    LLSD data;
    if (!mDataPrimary.isEmpty())
    {
        data["DataPrimary"] = mDataPrimary.asLLSD();
    }
    if (!mDataSecondary.isEmpty())
    {
        data["DataSecondary"] = mDataSecondary.asLLSD();
    }
    return data;
}

bool LLKeyBind::canHandle(EMouseClickType mouse, KEY key, MASK mask) const
{
    if (mDataPrimary.mKey == key && mDataPrimary.mMask == mask && mDataPrimary.mMouse == mouse)
    {
        return true;
    }
    if (mDataSecondary.mKey == key && mDataSecondary.mMask == mask && mDataSecondary.mMouse == mouse)
    {
        return true;
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

