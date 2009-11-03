/** 
 * @file llfoldertype.h
 * @brief Declaration of LLFolderType.
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

#ifndef LL_LLFOLDERTYPE_H
#define LL_LLFOLDERTYPE_H

#include <string>
#include "llassettype.h"

// This class handles folder types (similar to assettype, except for folders)
// and operations on those.
class LLFolderType
{
public:
	// ! BACKWARDS COMPATIBILITY ! Folder type enums must match asset type enums.
	enum EType
	{
		FT_TEXTURE = 0,

		FT_SOUND = 1, 

		FT_CALLINGCARD = 2,

		FT_LANDMARK = 3,

		// FT_SCRIPT = 4,

		FT_CLOTHING = 5,

		FT_OBJECT = 6,

		FT_NOTECARD = 7,

		FT_CATEGORY = 8,

		FT_ROOT_CATEGORY = 9,

		FT_LSL_TEXT = 10,

		// FT_LSL_BYTECODE = 11,
		// FT_TEXTURE_TGA = 12,

		FT_BODYPART = 13,

		FT_TRASH = 14,

		FT_SNAPSHOT_CATEGORY = 15,

		FT_LOST_AND_FOUND = 16,

		// FT_SOUND_WAV = 17,
		// FT_IMAGE_TGA = 18,
		// FT_IMAGE_JPEG = 19,

		FT_ANIMATION = 20,

		FT_GESTURE = 21,

		// FT_SIMSTATE = 22,

		FT_FAVORITE = 23,

		FT_ENSEMBLE_START = 26,
		FT_ENSEMBLE_END = 45,
			// This range is reserved for special clothing folder types.

		FT_CURRENT_OUTFIT = 46,
		FT_OUTFIT = 47,
		FT_MY_OUTFITS = 48,
		
		FT_INBOX = 49,

		FT_COUNT = 50,

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
