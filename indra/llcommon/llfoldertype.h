/** 
 * @file llfoldertype.h
 * @brief Declaration of LLFolderType.
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

#ifndef LL_LLFOLDERTYPE_H
#define LL_LLFOLDERTYPE_H

#include <string>
#include "llassettype.h"

// This class handles folder types (similar to assettype, except for folders)
// and operations on those.
class LL_COMMON_API LLFolderType
{
public:
	// ! BACKWARDS COMPATIBILITY ! Folder type enums must match asset type enums.
	enum EType
	{
		FT_TEXTURE = 0,

		FT_SOUND = 1, 

		FT_CALLINGCARD = 2,

		FT_LANDMARK = 3,

		FT_CLOTHING = 5,

		FT_OBJECT = 6,

		FT_NOTECARD = 7,

		FT_ROOT_INVENTORY = 8,
			// We'd really like to change this to 9 since AT_CATEGORY is 8,
			// but "My Inventory" has been type 8 for a long time.

		FT_LSL_TEXT = 10,

		FT_BODYPART = 13,

		FT_TRASH = 14,

		FT_SNAPSHOT_CATEGORY = 15,

		FT_LOST_AND_FOUND = 16,

		FT_ANIMATION = 20,

		FT_GESTURE = 21,

		FT_FAVORITE = 23,

		FT_ENSEMBLE_START = 26,
		FT_ENSEMBLE_END = 45,
			// This range is reserved for special clothing folder types.

		FT_CURRENT_OUTFIT = 46,
		FT_OUTFIT = 47,
		FT_MY_OUTFITS = 48,
		
		FT_MESH = 49,

		FT_INBOX = 50,

		FT_COUNT = 51,

		FT_NONE = -1
	};

	static EType 				lookup(const std::string& type_name);
	static const std::string&	lookup(EType folder_type);

	static bool 				lookupIsProtectedType(EType folder_type);
	static bool 				lookupIsEnsembleType(EType folder_type);

	static LLAssetType::EType	folderTypeToAssetType(LLFolderType::EType folder_type);
	static LLFolderType::EType	assetTypeToFolderType(LLAssetType::EType asset_type);

	static const std::string&	badLookup(); // error string when a lookup fails

protected:
	LLFolderType() {}
	~LLFolderType() {}
};

#endif // LL_LLFOLDERTYPE_H
