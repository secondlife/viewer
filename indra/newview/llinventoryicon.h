/** 
 * @file llinventoryfunctions.h
 * @brief Miscellaneous inventory-related functions and classes
 * class definition
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
		
		ICONNAME_LINKITEM,
		ICONNAME_LINKFOLDER,

		ICONNAME_COUNT,

		ICONNAME_NONE = -1
	};

	static const std::string& getIconName(LLAssetType::EType asset_type,
										  LLInventoryType::EType inventory_type = LLInventoryType::IT_NONE,
										  BOOL item_is_link = FALSE,
										  U32 misc_flag = 0, // different meanings depending on item type
										  BOOL item_is_multi = FALSE);
	static const std::string& getIconName(EIconName idx, 
										  BOOL item_is_link = FALSE);

	static LLUIImagePtr getIcon(LLAssetType::EType asset_type,
								LLInventoryType::EType inventory_type = LLInventoryType::IT_NONE,
								BOOL item_is_link = FALSE,
								U32 misc_flag = 0, // different meanings depending on item type
								BOOL item_is_multi = FALSE);
	static LLUIImagePtr getIcon(EIconName idx);

protected:
	static EIconName assignWearableIcon(U32 misc_flag);
};
#endif // LL_LLINVENTORYICON_H



