/** 
 * @file llinventoryfunctions.h
 * @brief Miscellaneous inventory-related functions and classes
 * class definition
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

#ifndef LL_LLINVENTORYICON_H
#define LL_LLINVENTORYICON_H

#include "llassettype.h"
#include "llinventorytype.h"
#include "lluiimage.h"

class LLInventoryIcon
{
public:
	enum EIconName
	{
		ICONNAME_TEXTURE,
		ICONNAME_SOUND,
		ICONNAME_CALLINGCARD_ONLINE,
		ICONNAME_CALLINGCARD_OFFLINE,
		ICONNAME_LANDMARK,
		ICONNAME_LANDMARK_VISITED,
		ICONNAME_SCRIPT,
		ICONNAME_CLOTHING,
		ICONNAME_OBJECT,
		ICONNAME_OBJECT_MULTI,
		ICONNAME_NOTECARD,
		ICONNAME_BODYPART,
		ICONNAME_SNAPSHOT,
		
		ICONNAME_BODYPART_SHAPE,
		ICONNAME_BODYPART_SKIN,
		ICONNAME_BODYPART_HAIR,
		ICONNAME_BODYPART_EYES,
		ICONNAME_CLOTHING_SHIRT,
		ICONNAME_CLOTHING_PANTS,
		ICONNAME_CLOTHING_SHOES,
		ICONNAME_CLOTHING_SOCKS,
		ICONNAME_CLOTHING_JACKET,
		ICONNAME_CLOTHING_GLOVES,
		ICONNAME_CLOTHING_UNDERSHIRT,
		ICONNAME_CLOTHING_UNDERPANTS,
		ICONNAME_CLOTHING_SKIRT,
		ICONNAME_CLOTHING_ALPHA,
		ICONNAME_CLOTHING_TATTOO,

		ICONNAME_ANIMATION,
		ICONNAME_GESTURE,

		ICONNAME_CLOTHING_PHYSICS,
		
		ICONNAME_LINKITEM,
		ICONNAME_LINKFOLDER,
		ICONNAME_MESH,

		ICONNAME_INVALID,
		ICONNAME_COUNT,
		ICONNAME_NONE = -1
	};

	static const std::string& getIconName(LLAssetType::EType asset_type,
										  LLInventoryType::EType inventory_type = LLInventoryType::IT_NONE,
										  U32 misc_flag = 0, // different meanings depending on item type
										  BOOL item_is_multi = FALSE);
	static const std::string& getIconName(EIconName idx);

	static LLUIImagePtr getIcon(LLAssetType::EType asset_type,
								LLInventoryType::EType inventory_type = LLInventoryType::IT_NONE,
								U32 misc_flag = 0, // different meanings depending on item type
								BOOL item_is_multi = FALSE);
	static LLUIImagePtr getIcon(EIconName idx);

protected:
	static EIconName assignWearableIcon(U32 misc_flag);
};
#endif // LL_LLINVENTORYICON_H



