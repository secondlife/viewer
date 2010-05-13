/** 
 * @file llwearabletype.cpp
 * @brief LLWearableType class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
				  LLInventoryIcon::EIconName iconName);
	const LLAssetType::EType mAssetType;
	const std::string mLabel;
	const std::string mDefaultNewName; //keep mLabel for backward compatibility
	LLInventoryIcon::EIconName mIconName;
};

WearableEntry::WearableEntry(const std::string &name,
							 const std::string& default_new_name,
							 LLAssetType::EType assetType,
							 LLInventoryIcon::EIconName iconName) :
	LLDictionaryEntry(name),
	mAssetType(assetType),
	mDefaultNewName(default_new_name),
	mLabel(LLTrans::getString(name)),
	mIconName(iconName)
{
}

class LLWearableDictionary : public LLSingleton<LLWearableDictionary>,
							 public LLDictionary<LLWearableType::EType, WearableEntry>
{
public:
	LLWearableDictionary();
};

LLWearableDictionary::LLWearableDictionary()
{
	addEntry(LLWearableType::WT_SHAPE,        new WearableEntry("shape",       "New Shape",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_SHAPE));
	addEntry(LLWearableType::WT_SKIN,         new WearableEntry("skin",        "New Skin",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_SKIN));
	addEntry(LLWearableType::WT_HAIR,         new WearableEntry("hair",        "New Hair",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_HAIR));
	addEntry(LLWearableType::WT_EYES,         new WearableEntry("eyes",        "New Eyes",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_EYES));
	addEntry(LLWearableType::WT_SHIRT,        new WearableEntry("shirt",       "New Shirt",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SHIRT));
	addEntry(LLWearableType::WT_PANTS,        new WearableEntry("pants",       "New Pants",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_PANTS));
	addEntry(LLWearableType::WT_SHOES,        new WearableEntry("shoes",       "New Shoes",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SHOES));
	addEntry(LLWearableType::WT_SOCKS,        new WearableEntry("socks",       "New Socks",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SOCKS));
	addEntry(LLWearableType::WT_JACKET,       new WearableEntry("jacket",      "New Jacket",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_JACKET));
	addEntry(LLWearableType::WT_GLOVES,       new WearableEntry("gloves",      "New Gloves",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_GLOVES));
	addEntry(LLWearableType::WT_UNDERSHIRT,   new WearableEntry("undershirt",  "New Undershirt",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_UNDERSHIRT));
	addEntry(LLWearableType::WT_UNDERPANTS,   new WearableEntry("underpants",  "New Underpants",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_UNDERPANTS));
	addEntry(LLWearableType::WT_SKIRT,        new WearableEntry("skirt",       "New Skirt",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SKIRT));
	addEntry(LLWearableType::WT_ALPHA,        new WearableEntry("alpha",       "New Alpha",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_ALPHA));
	addEntry(LLWearableType::WT_TATTOO,       new WearableEntry("tattoo",      "New Tattoo",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_TATTOO));
	addEntry(LLWearableType::WT_NONE,      	 new WearableEntry("invalid",     "Invalid Wearable", 	LLAssetType::AT_NONE, 		LLInventoryIcon::ICONNAME_NONE));
	addEntry(LLWearableType::WT_COUNT,        NULL);
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
	return entry->mName;
}

//static 
const std::string& LLWearableType::getTypeDefaultNewName(LLWearableType::EType type)
{ 
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	return entry->mDefaultNewName;
}

// static 
const std::string& LLWearableType::getTypeLabel(LLWearableType::EType type)
{ 
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	return entry->mLabel;
}

// static 
LLAssetType::EType LLWearableType::getAssetType(LLWearableType::EType type)
{
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	return entry->mAssetType;
}

// static 
LLInventoryIcon::EIconName LLWearableType::getIconName(LLWearableType::EType type)
{
	const LLWearableDictionary *dict = LLWearableDictionary::getInstance();
	const WearableEntry *entry = dict->lookup(type);
	return entry->mIconName;
}

