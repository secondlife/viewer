/** 
 * @file llinventoryicon.cpp
 * @brief Implementation of the inventory icon.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "llinventoryicon.h"

#include "lldictionary.h"
#include "llinventorydefines.h"
#include "llui.h"
#include "llwearabletype.h"

struct IconEntry : public LLDictionaryEntry
{
	IconEntry(const std::string &item_name)
		:
		LLDictionaryEntry(item_name)
	{}
};

class LLIconDictionary : public LLSingleton<LLIconDictionary>,
						 public LLDictionary<LLInventoryIcon::EIconName, IconEntry>
{
public:
	LLIconDictionary();
};

LLIconDictionary::LLIconDictionary()
{
	addEntry(LLInventoryIcon::ICONNAME_TEXTURE, 				new IconEntry("Inv_Texture"));
	addEntry(LLInventoryIcon::ICONNAME_SOUND, 					new IconEntry("Inv_Texture"));
	addEntry(LLInventoryIcon::ICONNAME_CALLINGCARD_ONLINE, 		new IconEntry("Inv_CallingCard"));
	addEntry(LLInventoryIcon::ICONNAME_CALLINGCARD_OFFLINE, 	new IconEntry("Inv_CallingCard"));
	addEntry(LLInventoryIcon::ICONNAME_LANDMARK, 				new IconEntry("Inv_Landmark"));
	addEntry(LLInventoryIcon::ICONNAME_LANDMARK_VISITED, 		new IconEntry("Inv_Landmark"));
	addEntry(LLInventoryIcon::ICONNAME_SCRIPT, 					new IconEntry("Inv_Script"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING, 				new IconEntry("Inv_Clothing"));
	addEntry(LLInventoryIcon::ICONNAME_OBJECT, 					new IconEntry("Inv_Object"));
	addEntry(LLInventoryIcon::ICONNAME_OBJECT_MULTI, 			new IconEntry("Inv_Object_Multi"));
	addEntry(LLInventoryIcon::ICONNAME_NOTECARD, 				new IconEntry("Inv_Notecard"));
	addEntry(LLInventoryIcon::ICONNAME_BODYPART, 				new IconEntry("Inv_Skin"));
	addEntry(LLInventoryIcon::ICONNAME_SNAPSHOT, 				new IconEntry("Inv_Snapshot"));

	addEntry(LLInventoryIcon::ICONNAME_BODYPART_SHAPE, 			new IconEntry("Inv_BodyShape"));
	addEntry(LLInventoryIcon::ICONNAME_BODYPART_SKIN, 			new IconEntry("Inv_Skin"));
	addEntry(LLInventoryIcon::ICONNAME_BODYPART_HAIR, 			new IconEntry("Inv_Hair"));
	addEntry(LLInventoryIcon::ICONNAME_BODYPART_EYES, 			new IconEntry("Inv_Eye"));

	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_SHIRT, 			new IconEntry("Inv_Shirt"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_PANTS, 			new IconEntry("Inv_Pants"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_SHOES, 			new IconEntry("Inv_Shoe"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_SOCKS, 			new IconEntry("Inv_Socks"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_JACKET, 		new IconEntry("Inv_Jacket"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_GLOVES, 		new IconEntry("Inv_Gloves"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_UNDERSHIRT, 	new IconEntry("Inv_Undershirt"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_UNDERPANTS, 	new IconEntry("Inv_Underpants"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_SKIRT, 			new IconEntry("Inv_Skirt"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_ALPHA, 			new IconEntry("Inv_Alpha"));
	addEntry(LLInventoryIcon::ICONNAME_CLOTHING_TATTOO, 		new IconEntry("Inv_Tattoo"));
	addEntry(LLInventoryIcon::ICONNAME_ANIMATION, 				new IconEntry("Inv_Animation"));
	addEntry(LLInventoryIcon::ICONNAME_GESTURE, 				new IconEntry("Inv_Gesture"));

	addEntry(LLInventoryIcon::ICONNAME_LINKITEM, 				new IconEntry("Inv_LinkItem"));
	addEntry(LLInventoryIcon::ICONNAME_LINKFOLDER, 				new IconEntry("Inv_LinkItem"));

	addEntry(LLInventoryIcon::ICONNAME_NONE, 					new IconEntry("NONE"));
}

LLUIImagePtr LLInventoryIcon::getIcon(LLAssetType::EType asset_type,
									  LLInventoryType::EType inventory_type,
									  U32 misc_flag,
									  BOOL item_is_multi)
{
	const std::string& icon_name = getIconName(asset_type, inventory_type, misc_flag, item_is_multi);
	return LLUI::getUIImage(icon_name);
}

LLUIImagePtr LLInventoryIcon::getIcon(EIconName idx)
{
	return LLUI::getUIImage(getIconName(idx));
}

const std::string& LLInventoryIcon::getIconName(LLAssetType::EType asset_type,
												LLInventoryType::EType inventory_type,
												U32 misc_flag,
												BOOL item_is_multi)
{
	EIconName idx = ICONNAME_OBJECT;
	if (item_is_multi)
	{
		idx = ICONNAME_OBJECT_MULTI;
		return getIconName(idx);
	}
	
	switch(asset_type)
	{
		case LLAssetType::AT_TEXTURE:
			idx = (inventory_type == LLInventoryType::IT_SNAPSHOT) ? ICONNAME_SNAPSHOT : ICONNAME_TEXTURE;
			break;
		case LLAssetType::AT_SOUND:
			idx = ICONNAME_SOUND;
			break;
		case LLAssetType::AT_CALLINGCARD:
			idx = (misc_flag != 0) ? ICONNAME_CALLINGCARD_ONLINE : ICONNAME_CALLINGCARD_OFFLINE;
			break;
		case LLAssetType::AT_LANDMARK:
			idx = (misc_flag != 0) ? ICONNAME_LANDMARK_VISITED : ICONNAME_LANDMARK;
			break;
		case LLAssetType::AT_SCRIPT:
		case LLAssetType::AT_LSL_TEXT:
		case LLAssetType::AT_LSL_BYTECODE:
			idx = ICONNAME_SCRIPT;
			break;
		case LLAssetType::AT_CLOTHING:
		case LLAssetType::AT_BODYPART:
			idx = assignWearableIcon(misc_flag);
			break;
		case LLAssetType::AT_NOTECARD:
			idx = ICONNAME_NOTECARD;
			break;
		case LLAssetType::AT_ANIMATION:
			idx = ICONNAME_ANIMATION;
			break;
		case LLAssetType::AT_GESTURE:
			idx = ICONNAME_GESTURE;
			break;
		case LLAssetType::AT_LINK:
			idx = ICONNAME_LINKITEM;
			break;
		case LLAssetType::AT_LINK_FOLDER:
			idx = ICONNAME_LINKFOLDER;
			break;
		case LLAssetType::AT_OBJECT:
			idx = ICONNAME_OBJECT;
			break;
		default:
			break;
	}
	
	return getIconName(idx);
}


const std::string& LLInventoryIcon::getIconName(EIconName idx)
{
	const IconEntry *entry = LLIconDictionary::instance().lookup(idx);
	return entry->mName;
}

LLInventoryIcon::EIconName LLInventoryIcon::assignWearableIcon(U32 misc_flag)
{
	const LLWearableType::EType wearable_type = LLWearableType::EType(LLInventoryItemFlags::II_FLAGS_WEARABLES_MASK & misc_flag);
	return LLWearableType::getIconName(wearable_type);
}
