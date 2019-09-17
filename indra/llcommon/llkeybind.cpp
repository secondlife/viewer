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

LLKeyData::LLKeyData(EMouseClickType mouse, KEY key, MASK mask)
: mMouse(mouse), mKey(key), mMask(mask)
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

bool LLKeyData::operator==(const LLKeyData& rhs)
{
    if (mMouse != rhs.mMouse) return false;
    if (mKey != rhs.mKey) return false;
    if (mMask != rhs.mMask) return false;
    return true;
}

bool LLKeyData::operator!=(const LLKeyData& rhs)
{
    if (mMouse != rhs.mMouse) return true;
    if (mKey != rhs.mKey) return true;
    if (mMask != rhs.mMask) return true;
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
    LLSD data;
    for (data_vector_t::const_iterator iter = mData.begin(); iter != mData.end(); iter++)
    {
        if (!iter->isEmpty())
        {
            data.append(iter->asLLSD());
        }
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
        if (iter->mKey == key && iter->mMask == mask && iter->mMouse == mouse)
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

bool LLKeyBind::addKeyData(EMouseClickType mouse, KEY key, MASK mask)
{
    if (!canHandle(mouse, key, mask))
    {
        mData.push_back(LLKeyData(mouse, key, mask));
        return true;
    }
    return false;
}

bool LLKeyBind::addKeyData(const LLKeyData& data)
{
    if (!canHandle(data.mMouse, data.mKey, data.mMask))
    {
        mData.push_back(data);
        return true;
    }
    return false;
}

void LLKeyBind::replaceKeyData(EMouseClickType mouse, KEY key, MASK mask, U32 index)
{
    if (mouse != CLICK_NONE && key != KEY_NONE && mask != MASK_NONE)
    {
        for (data_vector_t::const_iterator iter = mData.begin(); iter != mData.end(); iter++)
        {
            if (iter->mKey == key && iter->mMask == mask && iter->mMouse == mouse)
            {
                mData.erase(iter);
                break;
            }
        }
    }
    if (mData.size() > index)
    {
        mData[index] = LLKeyData(mouse, key, mask);
    }
    else
    {
        mData.push_back(LLKeyData(mouse, key, mask));
    }
}

void LLKeyBind::replaceKeyData(const LLKeyData& data, U32 index)
{
    for (data_vector_t::const_iterator iter = mData.begin(); iter != mData.end(); iter++)
    {
        if (iter->mKey == data.mKey && iter->mMask == data.mMask && iter->mMouse == data.mMouse)
        {
            mData.erase(iter);
            break;
        }
    }
    if (mData.size() > index)
    {
        mData[index] = data;
    }
    else
    {
        mData.push_back(data);
    }
}

bool LLKeyBind::hasKeyData(U32 index) const
{
    return mData.size() > index;
}

LLKeyData LLKeyBind::getKeyData(U32 index) const
{
    if (mData.size() > index)
    {
        return mData[index];
    }
    return LLKeyData();
}

U32 LLKeyBind::getDataCount()
{
    return mData.size();
}

