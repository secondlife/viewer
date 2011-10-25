/** 
 * @file llinventorytype.cpp
 * @brief Inventory item type, more specific than an asset type.
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
	addEntry(LLInventoryType::IT_MESH,                new InventoryEntry("mesh",      "mesh",          1, LLAssetType::AT_MESH));
	addEntry(LLInventoryType::IT_WIDGET,              new InventoryEntry("widget",    "widget",        1, LLAssetType::AT_WIDGET));
}


// Maps asset types to the default inventory type for that kind of asset.
// Thus, "Lost and Found" is a "Category"
static const LLInventoryType::EType
DEFAULT_ASSET_FOR_INV_TYPE[LLAssetType::AT_COUNT] =
{
	LLInventoryType::IT_TEXTURE,		// 0	AT_TEXTURE
	LLInventoryType::IT_SOUND,			// 1	AT_SOUND
	LLInventoryType::IT_CALLINGCARD,	// 2	AT_CALLINGCARD
	LLInventoryType::IT_LANDMARK,		// 3	AT_LANDMARK
	LLInventoryType::IT_LSL,			// 4	AT_SCRIPT
	LLInventoryType::IT_WEARABLE,		// 5	AT_CLOTHING
	LLInventoryType::IT_OBJECT,			// 6	AT_OBJECT
	LLInventoryType::IT_NOTECARD,		// 7	AT_NOTECARD
	LLInventoryType::IT_CATEGORY,		// 8	AT_CATEGORY
	LLInventoryType::IT_NONE,			// 9	(null entry)
	LLInventoryType::IT_LSL,			// 10	AT_LSL_TEXT
	LLInventoryType::IT_LSL,			// 11	AT_LSL_BYTECODE
	LLInventoryType::IT_TEXTURE,		// 12	AT_TEXTURE_TGA
	LLInventoryType::IT_WEARABLE,		// 13	AT_BODYPART
	LLInventoryType::IT_CATEGORY,		// 14	AT_TRASH
	LLInventoryType::IT_CATEGORY,		// 15	AT_SNAPSHOT_CATEGORY
	LLInventoryType::IT_CATEGORY,		// 16	AT_LOST_AND_FOUND
	LLInventoryType::IT_SOUND,			// 17	AT_SOUND_WAV
	LLInventoryType::IT_NONE,			// 18	AT_IMAGE_TGA
	LLInventoryType::IT_NONE,			// 19	AT_IMAGE_JPEG
	LLInventoryType::IT_ANIMATION,		// 20	AT_ANIMATION
	LLInventoryType::IT_GESTURE,		// 21	AT_GESTURE
	LLInventoryType::IT_NONE,			// 22	AT_SIMSTATE

	LLInventoryType::IT_NONE,			// 23	AT_LINK
	LLInventoryType::IT_NONE,			// 24	AT_LINK_FOLDER

	LLInventoryType::IT_NONE,			// 25	AT_NONE
	LLInventoryType::IT_NONE,			// 26	AT_NONE
	LLInventoryType::IT_NONE,			// 27	AT_NONE
	LLInventoryType::IT_NONE,			// 28	AT_NONE
	LLInventoryType::IT_NONE,			// 29	AT_NONE
	LLInventoryType::IT_NONE,			// 30	AT_NONE
	LLInventoryType::IT_NONE,			// 31	AT_NONE
	LLInventoryType::IT_NONE,			// 32	AT_NONE
	LLInventoryType::IT_NONE,			// 33	AT_NONE
	LLInventoryType::IT_NONE,			// 34	AT_NONE
	LLInventoryType::IT_NONE,			// 35	AT_NONE
	LLInventoryType::IT_NONE,			// 36	AT_NONE
	LLInventoryType::IT_NONE,			// 37	AT_NONE
	LLInventoryType::IT_NONE,			// 38	AT_NONE
	LLInventoryType::IT_NONE,			// 39	AT_NONE
	LLInventoryType::IT_WIDGET,			// 40	AT_WIDGET
	LLInventoryType::IT_NONE,			// 41	AT_NONE
	LLInventoryType::IT_NONE,			// 42	AT_NONE
	LLInventoryType::IT_NONE,			// 43	AT_NONE
	LLInventoryType::IT_NONE,			// 44	AT_NONE
	LLInventoryType::IT_NONE,			// 45	AT_NONE
	LLInventoryType::IT_NONE,			// 46	AT_NONE
	LLInventoryType::IT_NONE,			// 47	AT_NONE
	LLInventoryType::IT_NONE,			// 48	AT_NONE
	LLInventoryType::IT_MESH,			// 49	AT_MESH
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
