/** 
 * @file llinventorytype.h
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

#ifndef LLINVENTORYTYPE_H
#define LLINVENTORYTYPE_H

#include "llassettype.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryType
//
// Class used to encapsulate operations around inventory type.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryType
{
public:
	enum EType
	{
		IT_TEXTURE = 0,
		IT_SOUND = 1, 
		IT_CALLINGCARD = 2,
		IT_LANDMARK = 3,
		//IT_SCRIPT = 4,
		//IT_CLOTHING = 5,
		IT_OBJECT = 6,
		IT_NOTECARD = 7,
		IT_CATEGORY = 8,
		IT_ROOT_CATEGORY = 9,
		IT_LSL = 10,
		//IT_LSL_BYTECODE = 11,
		//IT_TEXTURE_TGA = 12,
		//IT_BODYPART = 13,
		//IT_TRASH = 14,
		IT_SNAPSHOT = 15,
		//IT_LOST_AND_FOUND = 16,
		IT_ATTACHMENT = 17,
		IT_WEARABLE = 18,
		IT_ANIMATION = 19,
		IT_GESTURE = 20,
		IT_COUNT = 21,

		IT_NONE = -1
	};

	// machine transation between type and strings
	static EType lookup(const std::string& name);
	static const std::string &lookup(EType type);
	// translation from a type to a human readable form.
	static const std::string &lookupHumanReadable(EType type);

	// return the default inventory for the given asset type.
	static EType defaultForAssetType(LLAssetType::EType asset_type);

	// true if this type cannot have restricted permissions.
	static bool cannotRestrictPermissions(EType type);

private:
	// don't instantiate or derive one of these objects
	LLInventoryType( void );
	~LLInventoryType( void );
};

// helper function that returns true if inventory type and asset type
// are potentially compatible. For example, an attachment must be an
// object, but a wearable can be a bodypart or clothing asset.
bool inventory_and_asset_types_match(LLInventoryType::EType inventory_type,
									 LLAssetType::EType asset_type);

#endif
