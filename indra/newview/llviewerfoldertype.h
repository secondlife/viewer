/** 
 * @file llviewerfoldertype.h
 * @brief Declaration of LLAssetType.
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

	static const std::string&   lookupIconName(EType asset_type); // folder icon name
	static const std::string&	lookupNewCategoryName(EType folder_type); // default name when creating new category
	static LLFolderType::EType	lookupTypeFromNewCategoryName(const std::string& name); // default name when creating new category

	static U64					lookupValidFolderTypes(const std::string& item_name); // which folders allow an item of this type?
protected:
	LLViewerFolderType() {}
	~LLViewerFolderType() {}
};

#endif // LL_LLVIEWERFOLDERTYPE_H
