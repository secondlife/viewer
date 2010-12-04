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

#include "llviewerprecompiledheaders.h"
#include "llwearabletype.h"
#include "llinventoryfunctions.h"
#include "lltrans.h"

struct WearableEntry : public LLDictionaryEntry
{
	WearableEntry(const std::string &name,
				  const std::string& default_new_name,
				  LLAssetType::EType assetType,
				  LLInventoryIcon::EIconName iconName,
				  BOOL disable_camera_switch = FALSE,
				  BOOL allow_multiwear = TRUE) :
		LLDictionaryEntry(name),
		mAssetType(assetType),
		mDefaultNewName(default_new_name),
		mLabel(LLTrans::getString(name)),
		mIconName(iconName),
		mDisableCameraSwitch(disable_camera_switch),
		mAllowMultiwear(allow_multiwear)
	{
		
	}
	const LLAssetType::EType mAssetType;
	const std::string mLabel;
	const std::string mDefaultNewName; //keep mLabel for backward compatibility
	LLInventoryIcon::EIconName mIconName;
	BOOL mDisableCameraSwitch;
	BOOL mAllowMultiwear;
};

class LLWearableDictionary : public LLSingleton<LLWearableDictionary>,
							 public LLDictionary<LLWearableType::EType, WearableEntry>
{
public:
	LLWearableDictionary();
};

LLWearableDictionary::LLWearableDictionary()
{
	addEntry(LLWearableType::WT_SHAPE,        new WearableEntry("shape",       "New Shape",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_SHAPE, FALSE, FALSE));
	addEntry(LLWearableType::WT_SKIN,         new WearableEntry("skin",        "New Skin",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_SKIN, FALSE, FALSE));
	addEntry(LLWearableType::WT_HAIR,         new WearableEntry("hair",        "New Hair",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_HAIR, FALSE, FALSE));
	addEntry(LLWearableType::WT_EYES,         new WearableEntry("eyes",        "New Eyes",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_EYES, FALSE, FALSE));
	addEntry(LLWearableType::WT_SHIRT,        new WearableEntry("shirt",       "New Shirt",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SHIRT, FALSE, TRUE));
	addEntry(LLWearableType::WT_PANTS,        new WearableEntry("pants",       "New Pants",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_PANTS, FALSE, TRUE));
	addEntry(LLWearableType::WT_SHOES,        new WearableEntry("shoes",       "New Shoes",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SHOES, FALSE, TRUE));
	addEntry(LLWearableType::WT_SOCKS,        new WearableEntry("socks",       "New Socks",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SOCKS, FALSE, TRUE));
	addEntry(LLWearableType::WT_JACKET,       new WearableEntry("jacket",      "New Jacket",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_JACKET, FALSE, TRUE));
	addEntry(LLWearableType::WT_GLOVES,       new WearableEntry("gloves",      "New Gloves",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_GLOVES, FALSE, TRUE));
	addEntry(LLWearableType::WT_UNDERSHIRT,   new WearableEntry("undershirt",  "New Undershirt",	LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_UNDERSHIRT, FALSE, TRUE));
	addEntry(LLWearableType::WT_UNDERPANTS,   new WearableEntry("underpants",  "New Underpants",	LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_UNDERPANTS, FALSE, TRUE));
	addEntry(LLWearableType::WT_SKIRT,        new WearableEntry("skirt",       "New Skirt",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SKIRT, FALSE, TRUE));
	addEntry(LLWearableType::WT_ALPHA,        new WearableEntry("alpha",       "New Alpha",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_ALPHA, FALSE, TRUE));
	addEntry(LLWearableType::WT_TATTOO,       new WearableEntry("tattoo",      "New Tattoo",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_TATTOO, FALSE, TRUE));

	addEntry(LLWearableType::WT_PHYSICS,      new WearableEntry("physics",     "New Physics",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_PHYSICS, TRUE, FALSE));

	addEntry(LLWearableType::WT_INVALID,      new WearableEntry("invalid",     "Invalid Wearable", 	LLAssetType::AT_NONE, 		LLInventoryIcon::ICONNAME_NONE, FALSE, FALSE));
	addEntry(LLWearableType::WT_NONE,      	  new WearableEntry("none",        "Invalid Wearable", 	LLAssetType::AT_NONE, 		LLInventoryIcon::ICONNAME_NONE, FALSE, FALSE));
}

// static
LLWearableType::EType LLWearableType::typeNameToType(const std::string& type_name)
{
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const LLWearableType::EType wearable = dict->lookup(type_name);
	return wearable;
}

// static 
const std::string& LLWearableType::getTypeName(LLWearableType::EType type)
{ 
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	if (!entry) return getTypeName(WT_INVALID);
	return entry->mName;
}

//static 
const std::string& LLWearableType::getTypeDefaultNewName(LLWearableType::EType type)
{ 
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	if (!entry) return getTypeDefaultNewName(WT_INVALID);
	return entry->mDefaultNewName;
}

// static 
const std::string& LLWearableType::getTypeLabel(LLWearableType::EType type)
{ 
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	if (!entry) return getTypeLabel(WT_INVALID);
	return entry->mLabel;
}

// static 
LLAssetType::EType LLWearableType::getAssetType(LLWearableType::EType type)
{
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	if (!entry) return getAssetType(WT_INVALID);
	return entry->mAssetType;
}

// static 
LLInventoryIcon::EIconName LLWearableType::getIconName(LLWearableType::EType type)
{
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	if (!entry) return getIconName(WT_INVALID);
	return entry->mIconName;
} 

// static 
BOOL LLWearableType::getDisableCameraSwitch(LLWearableType::EType type)
{
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	return entry->mDisableCameraSwitch;
}

// static 
BOOL LLWearableType::getAllowMultiwear(LLWearableType::EType type)
{
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	return entry->mAllowMultiwear;
}

