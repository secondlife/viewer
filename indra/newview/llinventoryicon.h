/** 
 * @file llinventoryicon.h
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

class LLInventoryIcon
{
public:
	static const std::string& getIconName(LLAssetType::EType asset_type,
										  LLInventoryType::EType inventory_type = LLInventoryType::IT_NONE,
										  U32 misc_flag = 0, // different meanings depending on item type
										  bool item_is_multi = false);
	static const std::string& getIconName(LLInventoryType::EIconName idx);

	static LLPointer<class LLUIImage> getIcon(LLAssetType::EType asset_type,
								LLInventoryType::EType inventory_type = LLInventoryType::IT_NONE,
								U32 misc_flag = 0, // different meanings depending on item type
								bool item_is_multi = false);
	static LLPointer<class LLUIImage> getIcon(LLInventoryType::EIconName idx);

protected:
	static LLInventoryType::EIconName assignWearableIcon(U32 misc_flag);
    static LLInventoryType::EIconName assignSettingsIcon(U32 misc_flag);
};
#endif // LL_LLINVENTORYICON_H



