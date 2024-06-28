/**
 * @file llwearabletype.cpp
 * @brief LLWearableType class implementation
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

#include "linden_common.h"
#include "llwearabletype.h"
#include "llinventorytype.h"
#include "llinventorydefines.h"


LLWearableType::LLWearableDictionary::LLWearableDictionary(LLTranslationBridge::ptr_t& trans)
{
    addEntry(LLWearableType::WT_SHAPE,        new WearableEntry(trans, "shape",       "New Shape",          LLAssetType::AT_BODYPART,   LLInventoryType::ICONNAME_BODYPART_SHAPE, false, false));
    addEntry(LLWearableType::WT_SKIN,         new WearableEntry(trans, "skin",        "New Skin",           LLAssetType::AT_BODYPART,   LLInventoryType::ICONNAME_BODYPART_SKIN, false, false));
    addEntry(LLWearableType::WT_HAIR,         new WearableEntry(trans, "hair",        "New Hair",           LLAssetType::AT_BODYPART,   LLInventoryType::ICONNAME_BODYPART_HAIR, false, false));
    addEntry(LLWearableType::WT_EYES,         new WearableEntry(trans, "eyes",        "New Eyes",           LLAssetType::AT_BODYPART,   LLInventoryType::ICONNAME_BODYPART_EYES, false, false));
    addEntry(LLWearableType::WT_SHIRT,        new WearableEntry(trans, "shirt",       "New Shirt",          LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_SHIRT, false, true));
    addEntry(LLWearableType::WT_PANTS,        new WearableEntry(trans, "pants",       "New Pants",          LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_PANTS, false, true));
    addEntry(LLWearableType::WT_SHOES,        new WearableEntry(trans, "shoes",       "New Shoes",          LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_SHOES, false, true));
    addEntry(LLWearableType::WT_SOCKS,        new WearableEntry(trans, "socks",       "New Socks",          LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_SOCKS, false, true));
    addEntry(LLWearableType::WT_JACKET,       new WearableEntry(trans, "jacket",      "New Jacket",     LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_JACKET, false, true));
    addEntry(LLWearableType::WT_GLOVES,       new WearableEntry(trans, "gloves",      "New Gloves",     LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_GLOVES, false, true));
    addEntry(LLWearableType::WT_UNDERSHIRT,   new WearableEntry(trans, "undershirt",  "New Undershirt", LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_UNDERSHIRT, false, true));
    addEntry(LLWearableType::WT_UNDERPANTS,   new WearableEntry(trans, "underpants",  "New Underpants", LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_UNDERPANTS, false, true));
    addEntry(LLWearableType::WT_SKIRT,        new WearableEntry(trans, "skirt",       "New Skirt",          LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_SKIRT, false, true));
    addEntry(LLWearableType::WT_ALPHA,        new WearableEntry(trans, "alpha",       "New Alpha",          LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_ALPHA, false, true));
    addEntry(LLWearableType::WT_TATTOO,       new WearableEntry(trans, "tattoo",      "New Tattoo",     LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_TATTOO, false, true));
    addEntry(LLWearableType::WT_UNIVERSAL,    new WearableEntry(trans, "universal",   "New Universal",     LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_UNIVERSAL, false, true));

    addEntry(LLWearableType::WT_PHYSICS,      new WearableEntry(trans, "physics",     "New Physics",        LLAssetType::AT_CLOTHING,   LLInventoryType::ICONNAME_CLOTHING_PHYSICS, true, true));

    addEntry(LLWearableType::WT_INVALID,      new WearableEntry(trans, "invalid",     "Invalid Wearable",   LLAssetType::AT_NONE,       LLInventoryType::ICONNAME_UNKNOWN, false, false));
    addEntry(LLWearableType::WT_NONE,         new WearableEntry(trans, "none",        "Invalid Wearable",   LLAssetType::AT_NONE,       LLInventoryType::ICONNAME_NONE, false, false));
}


// class LLWearableType

LLWearableType::LLWearableType(LLTranslationBridge::ptr_t &trans)
:   mDictionary(trans)
{
}

LLWearableType::~LLWearableType()
{
}

void LLWearableType::initSingleton()
{
}

LLWearableType::EType LLWearableType::typeNameToType(const std::string& type_name)
{
    const LLWearableType::EType wearable = mDictionary.lookup(type_name);
    return wearable;
}

const std::string& LLWearableType::getTypeName(LLWearableType::EType type)
{
    const WearableEntry *entry = mDictionary.lookup(type);
    if (!entry) return getTypeName(WT_INVALID);
    return entry->mName;
}

const std::string& LLWearableType::getTypeDefaultNewName(LLWearableType::EType type)
{
    const WearableEntry *entry = mDictionary.lookup(type);
    if (!entry) return getTypeDefaultNewName(WT_INVALID);
    return entry->mDefaultNewName;
}

const std::string& LLWearableType::getTypeLabel(LLWearableType::EType type)
{
    const WearableEntry *entry = mDictionary.lookup(type);
    if (!entry) return getTypeLabel(WT_INVALID);
    return entry->mLabel;
}

LLAssetType::EType LLWearableType::getAssetType(LLWearableType::EType type)
{
    const WearableEntry *entry = mDictionary.lookup(type);
    if (!entry) return getAssetType(WT_INVALID);
    return entry->mAssetType;
}

LLInventoryType::EIconName LLWearableType::getIconName(LLWearableType::EType type)
{
    const WearableEntry *entry = mDictionary.lookup(type);
    if (!entry) return getIconName(WT_INVALID);
    return entry->mIconName;
}

bool LLWearableType::getDisableCameraSwitch(LLWearableType::EType type)
{
    const WearableEntry *entry = mDictionary.lookup(type);
    if (!entry) return false;
    return entry->mDisableCameraSwitch;
}

bool LLWearableType::getAllowMultiwear(LLWearableType::EType type)
{
    const WearableEntry *entry = mDictionary.lookup(type);
    if (!entry) return false;
    return entry->mAllowMultiwear;
}

// static
LLWearableType::EType LLWearableType::inventoryFlagsToWearableType(U32 flags)
{
    return  (LLWearableType::EType)(flags & LLInventoryItemFlags::II_FLAGS_SUBTYPE_MASK);
}

