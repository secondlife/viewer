/** 
 * @file llinventorytype.cpp
 * @brief Inventory item type, more specific than an asset type.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

///----------------------------------------------------------------------------
/// Class LLInventoryType
///----------------------------------------------------------------------------

// Unlike asset type names, not limited to 8 characters.
// Need not match asset type names.
static const char* INVENTORY_TYPE_NAMES[LLInventoryType::IT_COUNT] =
{ 
	"texture",      // 0
	"sound",
	"callcard",
	"landmark",
	NULL,
	NULL,           // 5
	"object",
	"notecard",
	"category",
	"root",
	"script",       // 10
	NULL,
	NULL,
	NULL,
	NULL,
	"snapshot",     // 15
	NULL,
	"attach",
	"wearable",
	"animation",
	"gesture",		// 20
};

// This table is meant for decoding to human readable form. Put any
// and as many printable characters you want in each one.
// See also LLAssetType::mAssetTypeHumanNames
static const char* INVENTORY_TYPE_HUMAN_NAMES[LLInventoryType::IT_COUNT] =
{ 
	"texture",      // 0
	"sound",
	"calling card",
	"landmark",
	NULL,
	NULL,           // 5
	"object",
	"note card",
	"folder",
	"root",
	"script",       // 10
	NULL,
	NULL,
	NULL,
	NULL,
	"snapshot",     // 15
	NULL,
	"attachment",
	"wearable",
	"animation",
	"gesture",		// 20
};

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
	LLInventoryType::IT_ROOT_CATEGORY,	// AT_ROOT_CATEGORY
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
};

static const int MAX_POSSIBLE_ASSET_TYPES = 2;
static const LLAssetType::EType
INVENTORY_TO_ASSET_TYPE[LLInventoryType::IT_COUNT][MAX_POSSIBLE_ASSET_TYPES] =
{
	{ LLAssetType::AT_TEXTURE, LLAssetType::AT_NONE },		// IT_TEXTURE
	{ LLAssetType::AT_SOUND, LLAssetType::AT_NONE },		// IT_SOUND
	{ LLAssetType::AT_CALLINGCARD, LLAssetType::AT_NONE },	// IT_CALLINGCARD
	{ LLAssetType::AT_LANDMARK, LLAssetType::AT_NONE },		// IT_LANDMARK
	{ LLAssetType::AT_NONE, LLAssetType::AT_NONE },
	{ LLAssetType::AT_NONE, LLAssetType::AT_NONE },
	{ LLAssetType::AT_OBJECT, LLAssetType::AT_NONE },		// IT_OBJECT
	{ LLAssetType::AT_NOTECARD, LLAssetType::AT_NONE },		// IT_NOTECARD
	{ LLAssetType::AT_NONE, LLAssetType::AT_NONE },			// IT_CATEGORY
	{ LLAssetType::AT_NONE, LLAssetType::AT_NONE },			// IT_ROOT_CATEGORY
	{ LLAssetType::AT_LSL_TEXT, LLAssetType::AT_LSL_BYTECODE }, // IT_LSL
	{ LLAssetType::AT_NONE, LLAssetType::AT_NONE },
	{ LLAssetType::AT_NONE, LLAssetType::AT_NONE },
	{ LLAssetType::AT_NONE, LLAssetType::AT_NONE },
	{ LLAssetType::AT_NONE, LLAssetType::AT_NONE },
	{ LLAssetType::AT_TEXTURE, LLAssetType::AT_NONE },		// IT_SNAPSHOT
	{ LLAssetType::AT_NONE, LLAssetType::AT_NONE },
	{ LLAssetType::AT_OBJECT, LLAssetType::AT_NONE },		// IT_ATTACHMENT
	{ LLAssetType::AT_CLOTHING, LLAssetType::AT_BODYPART },	// IT_WEARABLE
	{ LLAssetType::AT_ANIMATION, LLAssetType::AT_NONE },	// IT_ANIMATION
	{ LLAssetType::AT_GESTURE, LLAssetType::AT_NONE },		// IT_GESTURE
};

// static
const char* LLInventoryType::lookup(EType type)
{
	if((type >= 0) && (type < IT_COUNT))
	{
		return INVENTORY_TYPE_NAMES[S32(type)];
	}
	else
	{
		return NULL;
	}
}

// static
LLInventoryType::EType LLInventoryType::lookup(const char* name)
{
	for(S32 i = 0; i < IT_COUNT; ++i)
	{
		if((INVENTORY_TYPE_NAMES[i])
		   && (0 == strcmp(name, INVENTORY_TYPE_NAMES[i])))
		{
			// match
			return (EType)i;
		}
	}
	return IT_NONE;
}

// XUI:translate
// translation from a type to a human readable form.
// static
const char* LLInventoryType::lookupHumanReadable(EType type)
{
	if((type >= 0) && (type < IT_COUNT))
	{
		return INVENTORY_TYPE_HUMAN_NAMES[S32(type)];
	}
	else
	{
		return NULL;
	}
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

bool inventory_and_asset_types_match(
	LLInventoryType::EType inventory_type,
	LLAssetType::EType asset_type)
{
	bool rv = false;
	if((inventory_type >= 0) && (inventory_type < LLInventoryType::IT_COUNT))
	{
		for(S32 i = 0; i < MAX_POSSIBLE_ASSET_TYPES; ++i)
		{
			if(INVENTORY_TO_ASSET_TYPE[inventory_type][i] == asset_type)
			{
				rv = true;
				break;
			}
		}
	}
	return rv;
}
