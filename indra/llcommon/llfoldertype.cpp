/** 
 * @file llfoldertype.cpp
 * @brief Implementatino of LLFolderType functionality.
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

#include "linden_common.h"

#include "llfoldertype.h"
#include "lldictionary.h"
#include "llmemory.h"
#include "llsingleton.h"

///----------------------------------------------------------------------------
/// Class LLFolderType
///----------------------------------------------------------------------------
struct FolderEntry : public LLDictionaryEntry
{
	FolderEntry(const std::string &type_name, // 8 character limit!
				bool is_protected) // can the viewer change categories of this type?
		:
	LLDictionaryEntry(type_name),
	mIsProtected(is_protected)
	{
		llassert(type_name.length() <= 8);
	}

	const bool mIsProtected;
};

class LLFolderDictionary : public LLSingleton<LLFolderDictionary>,
						   public LLDictionary<LLFolderType::EType, FolderEntry>
{
public:
	LLFolderDictionary();
protected:
	virtual LLFolderType::EType notFound() const
	{
		return LLFolderType::FT_NONE;
	}
};

LLFolderDictionary::LLFolderDictionary()
{
	//       													    TYPE NAME	PROTECTED
	//      													   |-----------|---------|
	addEntry(LLFolderType::FT_TEXTURE, 				new FolderEntry("texture",	TRUE));
	addEntry(LLFolderType::FT_SOUND, 				new FolderEntry("sound",	TRUE));
	addEntry(LLFolderType::FT_CALLINGCARD, 			new FolderEntry("callcard",	TRUE));
	addEntry(LLFolderType::FT_LANDMARK, 			new FolderEntry("landmark",	TRUE));
	addEntry(LLFolderType::FT_CLOTHING, 			new FolderEntry("clothing",	TRUE));
	addEntry(LLFolderType::FT_OBJECT, 				new FolderEntry("object",	TRUE));
	addEntry(LLFolderType::FT_NOTECARD, 			new FolderEntry("notecard",	TRUE));
	addEntry(LLFolderType::FT_ROOT_INVENTORY, 		new FolderEntry("root_inv",	TRUE));
	addEntry(LLFolderType::FT_LSL_TEXT, 			new FolderEntry("lsltext",	TRUE));
	addEntry(LLFolderType::FT_BODYPART, 			new FolderEntry("bodypart",	TRUE));
	addEntry(LLFolderType::FT_TRASH, 				new FolderEntry("trash",	TRUE));
	addEntry(LLFolderType::FT_SNAPSHOT_CATEGORY, 	new FolderEntry("snapshot", TRUE));
	addEntry(LLFolderType::FT_LOST_AND_FOUND, 		new FolderEntry("lstndfnd",	TRUE));
	addEntry(LLFolderType::FT_ANIMATION, 			new FolderEntry("animatn",	TRUE));
	addEntry(LLFolderType::FT_GESTURE, 				new FolderEntry("gesture",	TRUE));
	addEntry(LLFolderType::FT_FAVORITE, 			new FolderEntry("favorite",	TRUE));
	
	for (S32 ensemble_num = S32(LLFolderType::FT_ENSEMBLE_START); ensemble_num <= S32(LLFolderType::FT_ENSEMBLE_END); ensemble_num++)
	{
		addEntry(LLFolderType::EType(ensemble_num), new FolderEntry("ensemble", FALSE)); 
	}

	addEntry(LLFolderType::FT_CURRENT_OUTFIT, 		new FolderEntry("current",	TRUE));
	addEntry(LLFolderType::FT_OUTFIT, 				new FolderEntry("outfit",	FALSE));
	addEntry(LLFolderType::FT_MY_OUTFITS, 			new FolderEntry("my_otfts",	TRUE));
	addEntry(LLFolderType::FT_INBOX, 				new FolderEntry("inbox",	TRUE));
		 
	addEntry(LLFolderType::FT_NONE, 				new FolderEntry("-1",		FALSE));
};

// static
LLFolderType::EType LLFolderType::lookup(const std::string& name)
{
	return LLFolderDictionary::getInstance()->lookup(name);
}

// static
const std::string &LLFolderType::lookup(LLFolderType::EType folder_type)
{
	const FolderEntry *entry = LLFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mName;
	}
	else
	{
		return badLookup();
	}
}

// static
// Only ensembles and plain folders aren't protected.  "Protected" means
// you can't change certain properties such as their type.
bool LLFolderType::lookupIsProtectedType(EType folder_type)
{
	const LLFolderDictionary *dict = LLFolderDictionary::getInstance();
	const FolderEntry *entry = dict->lookup(folder_type);
	if (entry)
	{
		return entry->mIsProtected;
	}
	return true;
}

// static
bool LLFolderType::lookupIsEnsembleType(EType folder_type)
{
	return (folder_type >= FT_ENSEMBLE_START &&
			folder_type <= FT_ENSEMBLE_END);
}

// static
LLAssetType::EType LLFolderType::folderTypeToAssetType(LLFolderType::EType folder_type)
{
	if (LLAssetType::lookup(LLAssetType::EType(folder_type)) == LLAssetType::badLookup())
	{
		llwarns << "Converting to unknown asset type " << folder_type << llendl;
	}
	return (LLAssetType::EType)folder_type;
}

// static
LLFolderType::EType LLFolderType::assetTypeToFolderType(LLAssetType::EType asset_type)
{
	if (LLFolderType::lookup(LLFolderType::EType(asset_type)) == LLFolderType::badLookup())
	{
		llwarns << "Converting to unknown folder type " << asset_type << llendl;
	}
	return (LLFolderType::EType)asset_type;
}

// static
const std::string &LLFolderType::badLookup()
{
	static const std::string sBadLookup = "llfoldertype_bad_lookup";
	return sBadLookup;
}
