/** 
 * @file llinventorytype.cpp
 * @brief Inventory item type, more specific than an asset type.
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

#include "linden_common.h"

#include "llinventorytype.h"
#include "lldictionary.h"
#include "llmemory.h"
#include "llsingleton.h"

static const std::string empty_string;

///----------------------------------------------------------------------------
/// Class LLInventoryType
///----------------------------------------------------------------------------
struct InventoryEntry : public LLDictionaryEntry
{
	InventoryEntry(const std::string &name, // unlike asset type names, not limited to 8 characters; need not match asset type names
				   const std::string &human_name, // for decoding to human readable form; put any and as many printable characters you want in each one.
				   int num_asset_types = 0, ...)
		:
		LLDictionaryEntry(name),
		mHumanName(human_name)
	{
		va_list argp;
		va_start(argp, num_asset_types);
		// Read in local textures
		for (U8 i=0; i < num_asset_types; i++)
		{
			LLAssetType::EType t = (LLAssetType::EType)va_arg(argp,int);
			mAssetTypes.push_back(t);
		}
	}
		
	const std::string mHumanName;
	typedef std::vector<LLAssetType::EType> asset_vec_t;
	asset_vec_t mAssetTypes;
};

class LLInventoryDictionary : public LLSingleton<LLInventoryDictionary>,
							  public LLDictionary<LLInventoryType::EType, InventoryEntry>
{
public:
	LLInventoryDictionary();
};

LLInventoryDictionary::LLInventoryDictionary()
{
	addEntry(LLInventoryType::IT_TEXTURE,             new InventoryEntry("texture",   "texture",       1, LLAssetType::AT_TEXTURE));
	addEntry(LLInventoryType::IT_SOUND,               new InventoryEntry("sound",     "sound",         1, LLAssetType::AT_SOUND));
	addEntry(LLInventoryType::IT_CALLINGCARD,         new InventoryEntry("callcard",  "calling card",  1, LLAssetType::AT_CALLINGCARD));
	addEntry(LLInventoryType::IT_LANDMARK,            new InventoryEntry("landmark",  "landmark",      1, LLAssetType::AT_LANDMARK));
	addEntry(LLInventoryType::IT_OBJECT,              new InventoryEntry("object",    "object",        1, LLAssetType::AT_OBJECT));
	addEntry(LLInventoryType::IT_NOTECARD,            new InventoryEntry("notecard",  "note card",     1, LLAssetType::AT_NOTECARD));
	addEntry(LLInventoryType::IT_CATEGORY,            new InventoryEntry("category",  "folder"         ));
	addEntry(LLInventoryType::IT_ROOT_CATEGORY,       new InventoryEntry("root",      "root"           ));
	addEntry(LLInventoryType::IT_LSL,                 new InventoryEntry("script",    "script",        2, LLAssetType::AT_LSL_TEXT, LLAssetType::AT_LSL_BYTECODE));
	addEntry(LLInventoryType::IT_SNAPSHOT,            new InventoryEntry("snapshot",  "snapshot",      1, LLAssetType::AT_TEXTURE));
	addEntry(LLInventoryType::IT_ATTACHMENT,          new InventoryEntry("attach",    "attachment",    1, LLAssetType::AT_OBJECT));
	addEntry(LLInventoryType::IT_WEARABLE,            new InventoryEntry("wearable",  "wearable",      2, LLAssetType::AT_CLOTHING, LLAssetType::AT_BODYPART));
	addEntry(LLInventoryType::IT_ANIMATION,           new InventoryEntry("animation", "animation",     1, LLAssetType::AT_ANIMATION));  
	addEntry(LLInventoryType::IT_GESTURE,             new InventoryEntry("gesture",   "gesture",       1, LLAssetType::AT_GESTURE)); 
}


// Maps asset types to the default inventory type for that kind of asset.
// Thus, "Lost and Found" is a "Category"
static const LLInventoryType::EType
DEFAULT_ASSET_FOR_INV_TYPE[LLAssetType::AT_COUNT] =
{
	LLInventoryType::IT_TEXTURE,		// AT_TEXTURE
	LLInventoryType::IT_SOUND,			// AT_SOUND
	LLInventoryType::IT_CALLINGCARD,	// AT_CALLINGCARD
	LLInventoryType::IT_LANDMARK,		// AT_LANDMARK
	LLInventoryType::IT_LSL,			// AT_SCRIPT
	LLInventoryType::IT_WEARABLE,		// AT_CLOTHING
	LLInventoryType::IT_OBJECT,			// AT_OBJECT
	LLInventoryType::IT_NOTECARD,		// AT_NOTECARD
	LLInventoryType::IT_CATEGORY,		// AT_CATEGORY
	LLInventoryType::IT_NONE,			// (null entry)
	LLInventoryType::IT_LSL,			// AT_LSL_TEXT
	LLInventoryType::IT_LSL,			// AT_LSL_BYTECODE
	LLInventoryType::IT_TEXTURE,		// AT_TEXTURE_TGA
	LLInventoryType::IT_WEARABLE,		// AT_BODYPART
	LLInventoryType::IT_CATEGORY,		// AT_TRASH
	LLInventoryType::IT_CATEGORY,		// AT_SNAPSHOT_CATEGORY
	LLInventoryType::IT_CATEGORY,		// AT_LOST_AND_FOUND
	LLInventoryType::IT_SOUND,			// AT_SOUND_WAV
	LLInventoryType::IT_NONE,			// AT_IMAGE_TGA
	LLInventoryType::IT_NONE,			// AT_IMAGE_JPEG
	LLInventoryType::IT_ANIMATION,		// AT_ANIMATION
	LLInventoryType::IT_GESTURE,		// AT_GESTURE
	LLInventoryType::IT_NONE,			// AT_SIMSTATE

	LLInventoryType::IT_NONE,			// AT_LINK
	LLInventoryType::IT_NONE,			// AT_LINK_FOLDER
};

// static
const std::string &LLInventoryType::lookup(EType type)
{
	const InventoryEntry *entry = LLInventoryDictionary::getInstance()->lookup(type);
	if (!entry) return empty_string;
	return entry->mName;
}

// static
LLInventoryType::EType LLInventoryType::lookup(const std::string& name)
{
	return LLInventoryDictionary::getInstance()->lookup(name);
}

// XUI:translate
// translation from a type to a human readable form.
// static
const std::string &LLInventoryType::lookupHumanReadable(EType type)
{
	const InventoryEntry *entry = LLInventoryDictionary::getInstance()->lookup(type);
	if (!entry) return empty_string;
	return entry->mHumanName;
}

// return the default inventory for the given asset type.
// static
LLInventoryType::EType LLInventoryType::defaultForAssetType(LLAssetType::EType asset_type)
{
	if((asset_type >= 0) && (asset_type < LLAssetType::AT_COUNT))
	{
		return DEFAULT_ASSET_FOR_INV_TYPE[S32(asset_type)];
	}
	else
	{
		return IT_NONE;
	}
}


// add any types that we don't want the user to be able to change permissions on.
// static
bool LLInventoryType::cannotRestrictPermissions(LLInventoryType::EType type)
{
	switch(type)
	{
		case IT_CALLINGCARD:
		case IT_LANDMARK:
			return true;
		default:
			return false;
	}
}

bool inventory_and_asset_types_match(LLInventoryType::EType inventory_type,
									 LLAssetType::EType asset_type)
{
	// Links can be of any inventory type.
	if (LLAssetType::lookupIsLinkType(asset_type))
		return true;

	const InventoryEntry *entry = LLInventoryDictionary::getInstance()->lookup(inventory_type);
	if (!entry) return false;

	for (InventoryEntry::asset_vec_t::const_iterator iter = entry->mAssetTypes.begin();
		 iter != entry->mAssetTypes.end();
		 iter++)
	{
		const LLAssetType::EType type = (*iter);
		if(type == asset_type)
		{
			return true;
		}
	}
	return false;
}
