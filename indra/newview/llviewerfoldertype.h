/** 
 * @file llviewerfoldertype.h
 * @brief Declaration of LLAssetType.
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

#ifndef LL_LLVIEWERFOLDERTYPE_H
#define LL_LLVIEWERFOLDERTYPE_H

#include <string>
#include "llfoldertype.h"

// This class is similar to llfoldertype, but contains methods
// only used by the viewer.  This also handles ensembles.
class LLViewerFolderType : public LLFolderType
{
public:
	static const std::string&   lookupXUIName(EType folder_type); // name used by the UI
	static LLFolderType::EType 	lookupTypeFromXUIName(const std::string& name);

	static const std::string&   lookupIconName(EType folder_type, BOOL is_open = FALSE); // folder icon name
	static BOOL					lookupIsQuietType(EType folder_type); // folder doesn't require UI update when changes have occured
	static bool					lookupIsHiddenIfEmpty(EType folder_type); // folder is not displayed if empty
	static const std::string&	lookupNewCategoryName(EType folder_type); // default name when creating new category
	static LLFolderType::EType	lookupTypeFromNewCategoryName(const std::string& name); // default name when creating new category

	static U64					lookupValidFolderTypes(const std::string& item_name); // which folders allow an item of this type?

protected:
	LLViewerFolderType() {}
	~LLViewerFolderType() {}
};

#endif // LL_LLVIEWERFOLDERTYPE_H
