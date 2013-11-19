/** 
 * @file llinventoryicon.cpp
 * @brief Implementation of the inventory icon.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"
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
						 public LLDictionary<LLInventoryType::EIconName, IconEntry>
{
public:
	LLIconDictionary();
};

LLIconDictionary::LLIconDictionary()
{
	addEntry(LLInventoryType::ICONNAME_TEXTURE, 				new IconEntry("Inv_Texture"));
	addEntry(LLInventoryType::ICONNAME_SOUND, 					new IconEntry("Inv_Sound"));
	addEntry(LLInventoryType::ICONNAME_CALLINGCARD_ONLINE, 		new IconEntry("Inv_CallingCard"));
	addEntry(LLInventoryType::ICONNAME_CALLINGCARD_OFFLINE, 	new IconEntry("Inv_CallingCard"));
	addEntry(LLInventoryType::ICONNAME_LANDMARK, 				new IconEntry("Inv_Landmark"));
	addEntry(LLInventoryType::ICONNAME_LANDMARK_VISITED, 		new IconEntry("Inv_Landmark"));
	addEntry(LLInventoryType::ICONNAME_SCRIPT, 					new IconEntry("Inv_Script"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING, 				new IconEntry("Inv_Clothing"));
	addEntry(LLInventoryType::ICONNAME_OBJECT, 					new IconEntry("Inv_Object"));
	addEntry(LLInventoryType::ICONNAME_OBJECT_MULTI, 			new IconEntry("Inv_Object_Multi"));
	addEntry(LLInventoryType::ICONNAME_NOTECARD, 				new IconEntry("Inv_Notecard"));
	addEntry(LLInventoryType::ICONNAME_BODYPART, 				new IconEntry("Inv_Skin"));
	addEntry(LLInventoryType::ICONNAME_SNAPSHOT, 				new IconEntry("Inv_Snapshot"));

	addEntry(LLInventoryType::ICONNAME_BODYPART_SHAPE, 			new IconEntry("Inv_BodyShape"));
	addEntry(LLInventoryType::ICONNAME_BODYPART_SKIN, 			new IconEntry("Inv_Skin"));
	addEntry(LLInventoryType::ICONNAME_BODYPART_HAIR, 			new IconEntry("Inv_Hair"));
	addEntry(LLInventoryType::ICONNAME_BODYPART_EYES, 			new IconEntry("Inv_Eye"));

	addEntry(LLInventoryType::ICONNAME_CLOTHING_SHIRT, 			new IconEntry("Inv_Shirt"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_PANTS, 			new IconEntry("Inv_Pants"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_SHOES, 			new IconEntry("Inv_Shoe"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_SOCKS, 			new IconEntry("Inv_Socks"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_JACKET, 		new IconEntry("Inv_Jacket"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_GLOVES, 		new IconEntry("Inv_Gloves"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_UNDERSHIRT, 	new IconEntry("Inv_Undershirt"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_UNDERPANTS, 	new IconEntry("Inv_Underpants"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_SKIRT, 			new IconEntry("Inv_Skirt"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_ALPHA, 			new IconEntry("Inv_Alpha"));
	addEntry(LLInventoryType::ICONNAME_CLOTHING_TATTOO, 		new IconEntry("Inv_Tattoo"));
	addEntry(LLInventoryType::ICONNAME_ANIMATION, 				new IconEntry("Inv_Animation"));
	addEntry(LLInventoryType::ICONNAME_GESTURE, 				new IconEntry("Inv_Gesture"));

	addEntry(LLInventoryType::ICONNAME_CLOTHING_PHYSICS, 		new IconEntry("Inv_Physics"));

	addEntry(LLInventoryType::ICONNAME_LINKITEM, 				new IconEntry("Inv_LinkItem"));
	addEntry(LLInventoryType::ICONNAME_LINKFOLDER, 				new IconEntry("Inv_LinkFolder"));
	addEntry(LLInventoryType::ICONNAME_MESH,	 				new IconEntry("Inv_Mesh"));

	addEntry(LLInventoryType::ICONNAME_INVALID, 				new IconEntry("Inv_Invalid"));

	addEntry(LLInventoryType::ICONNAME_NONE, 					new IconEntry("NONE"));
}

LLUIImagePtr LLInventoryIcon::getIcon(LLAssetType::EType asset_type,
									  LLInventoryType::EType inventory_type,
									  U32 misc_flag,
									  BOOL item_is_multi)
{
	const std::string& icon_name = getIconName(asset_type, inventory_type, misc_flag, item_is_multi);
	return LLUI::getUIImage(icon_name);
}

LLUIImagePtr LLInventoryIcon::getIcon(LLInventoryType::EIconName idx)
{
	return LLUI::getUIImage(getIconName(idx));
}

const std::string& LLInventoryIcon::getIconName(LLAssetType::EType asset_type,
												LLInventoryType::EType inventory_type,
												U32 misc_flag,
												BOOL item_is_multi)
{
	LLInventoryType::EIconName idx = LLInventoryType::ICONNAME_OBJECT;
	if (item_is_multi)
	{
		idx = LLInventoryType::ICONNAME_OBJECT_MULTI;
		return getIconName(idx);
	}
	
	switch(asset_type)
	{
		case LLAssetType::AT_TEXTURE:
			idx = (inventory_type == LLInventoryType::IT_SNAPSHOT) ? LLInventoryType::ICONNAME_SNAPSHOT : LLInventoryType::ICONNAME_TEXTURE;
			break;
		case LLAssetType::AT_SOUND:
			idx = LLInventoryType::ICONNAME_SOUND;
			break;
		case LLAssetType::AT_CALLINGCARD:
			idx = (misc_flag != 0) ? LLInventoryType::ICONNAME_CALLINGCARD_ONLINE : LLInventoryType::ICONNAME_CALLINGCARD_OFFLINE;
			break;
		case LLAssetType::AT_LANDMARK:
			idx = (misc_flag != 0) ? LLInventoryType::ICONNAME_LANDMARK_VISITED : LLInventoryType::ICONNAME_LANDMARK;
			break;
		case LLAssetType::AT_SCRIPT:
		case LLAssetType::AT_LSL_TEXT:
		case LLAssetType::AT_LSL_BYTECODE:
			idx = LLInventoryType::ICONNAME_SCRIPT;
			break;
		case LLAssetType::AT_CLOTHING:
		case LLAssetType::AT_BODYPART:
			idx = assignWearableIcon(misc_flag);
			break;
		case LLAssetType::AT_NOTECARD:
			idx = LLInventoryType::ICONNAME_NOTECARD;
			break;
		case LLAssetType::AT_ANIMATION:
			idx = LLInventoryType::ICONNAME_ANIMATION;
			break;
		case LLAssetType::AT_GESTURE:
			idx = LLInventoryType::ICONNAME_GESTURE;
			break;
		case LLAssetType::AT_LINK:
			idx = LLInventoryType::ICONNAME_LINKITEM;
			break;
		case LLAssetType::AT_LINK_FOLDER:
			idx = LLInventoryType::ICONNAME_LINKFOLDER;
			break;
		case LLAssetType::AT_OBJECT:
			idx = LLInventoryType::ICONNAME_OBJECT;
			break;
		case LLAssetType::AT_MESH:
			idx = LLInventoryType::ICONNAME_MESH;
		default:
			break;
	}
	
	return getIconName(idx);
}


const std::string& LLInventoryIcon::getIconName(LLInventoryType::EIconName idx)
{
	const IconEntry *entry = LLIconDictionary::instance().lookup(idx);
	return entry->mName;
}

LLInventoryType::EIconName LLInventoryIcon::assignWearableIcon(U32 misc_flag)
{
	const LLWearableType::EType wearable_type = LLWearableType::EType(LLInventoryItemFlags::II_FLAGS_WEARABLES_MASK & misc_flag);
	return LLWearableType::getIconName(wearable_type);
}
