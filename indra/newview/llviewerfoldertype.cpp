/** 
 * @file llfoldertype.cpp
 * @brief Implementation of LLViewerFolderType functionality.
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

#include "llviewerprecompiledheaders.h"

#include "llviewerfoldertype.h"
#include "lldictionary.h"
#include "llmemory.h"
#include "llvisualparam.h"

static const std::string empty_string;

struct ViewerFolderEntry : public LLDictionaryEntry
{
	// Constructor for non-ensembles
	ViewerFolderEntry(const std::string &new_category_name, // default name when creating a new category of this type
					  const std::string &icon_name 			// name of the folder icon
		) 
		:
		LLDictionaryEntry(empty_string), // no reverse lookup needed on non-ensembles, so just leave this blank
		mIconName(icon_name),
		mNewCategoryName(new_category_name)
	{
		mAllowedNames.clear();
	}

	// Constructor for ensembles
	ViewerFolderEntry(const std::string &xui_name, 			// name of the xui menu item
					  const std::string &new_category_name, // default name when creating a new category of this type
					  const std::string &icon_name, 		// name of the folder icon
					  const std::string allowed_names 		// allowed item typenames for this folder type
		) 
		:
		LLDictionaryEntry(xui_name),
		mIconName(icon_name),
		mNewCategoryName(new_category_name)
	{
		const std::string delims (",");
		LLStringUtilBase<char>::getTokens(allowed_names, mAllowedNames, delims);
	}

	bool getIsAllowedName(const std::string &name) const
	{
		if (mAllowedNames.empty())
			return false;
		for (name_vec_t::const_iterator iter = mAllowedNames.begin();
			 iter != mAllowedNames.end();
			 iter++)
		{
			if (name == (*iter))
				return true;
		}
		return false;
	}
	const std::string mIconName;
	const std::string mNewCategoryName;
	typedef std::vector<std::string> name_vec_t;
	name_vec_t mAllowedNames;
};

class LLViewerFolderDictionary : public LLSingleton<LLViewerFolderDictionary>,
								 public LLDictionary<LLFolderType::EType, ViewerFolderEntry>
{
public:
	LLViewerFolderDictionary();
protected:
	bool initEnsemblesFromFile(); // Reads in ensemble information from foldertypes.xml
};

LLViewerFolderDictionary::LLViewerFolderDictionary()
{
	initEnsemblesFromFile();

	//       													    	  NEW CATEGORY NAME         FOLDER ICON NAME
	//      												  		     |-------------------------|---------------------------|
	addEntry(LLFolderType::FT_TEXTURE, 				new ViewerFolderEntry("Textures",				"inv_folder_texture.tga"));
	addEntry(LLFolderType::FT_SOUND, 				new ViewerFolderEntry("Sounds",					"inv_folder_sound.tga"));
	addEntry(LLFolderType::FT_CALLINGCARD, 			new ViewerFolderEntry("Calling Cards",			"inv_folder_callingcard.tga"));
	addEntry(LLFolderType::FT_LANDMARK, 			new ViewerFolderEntry("Landmarks",				"inv_folder_landmark.tga"));
	addEntry(LLFolderType::FT_CLOTHING, 			new ViewerFolderEntry("Clothing",				"inv_folder_clothing.tga"));
	addEntry(LLFolderType::FT_OBJECT, 				new ViewerFolderEntry("Objects",				"inv_folder_object.tga"));
	addEntry(LLFolderType::FT_NOTECARD, 			new ViewerFolderEntry("Notecards",				"inv_folder_notecard.tga"));
	addEntry(LLFolderType::FT_CATEGORY, 			new ViewerFolderEntry("New Folder",				"inv_folder_plain_closed.tga"));
	addEntry(LLFolderType::FT_ROOT_CATEGORY, 		new ViewerFolderEntry("Inventory",				""));
	addEntry(LLFolderType::FT_LSL_TEXT, 			new ViewerFolderEntry("Scripts",				"inv_folder_script.tga"));
	addEntry(LLFolderType::FT_BODYPART, 			new ViewerFolderEntry("Body Parts",				"inv_folder_bodypart.tga"));
	addEntry(LLFolderType::FT_TRASH, 				new ViewerFolderEntry("Trash",					"inv_folder_trash.tga"));
	addEntry(LLFolderType::FT_SNAPSHOT_CATEGORY, 	new ViewerFolderEntry("Photo Album",			"inv_folder_snapshot.tga"));
	addEntry(LLFolderType::FT_LOST_AND_FOUND, 		new ViewerFolderEntry("Lost And Found",	   		"inv_folder_lostandfound.tga"));
	addEntry(LLFolderType::FT_ANIMATION, 			new ViewerFolderEntry("Animations",				"inv_folder_animation.tga"));
	addEntry(LLFolderType::FT_GESTURE, 				new ViewerFolderEntry("Gestures",				"inv_folder_gesture.tga"));
	addEntry(LLFolderType::FT_FAVORITE, 			new ViewerFolderEntry("Favorite",				"inv_folder_plain_closed.tga"));

	addEntry(LLFolderType::FT_CURRENT_OUTFIT, 		new ViewerFolderEntry("Current Outfit",			"inv_folder_current_outfit.tga"));
	addEntry(LLFolderType::FT_OUTFIT, 				new ViewerFolderEntry("New Outfit",				"inv_folder_outfit.tga"));
	addEntry(LLFolderType::FT_MY_OUTFITS, 			new ViewerFolderEntry("My Outfits",				"inv_folder_my_outfits.tga"));
	addEntry(LLFolderType::FT_INBOX, 				new ViewerFolderEntry("Inbox",					"inv_folder_inbox.tga"));
		 
	addEntry(LLFolderType::FT_NONE, 				new ViewerFolderEntry("New Folder",				"inv_folder_plain_closed.tga"));
}

bool LLViewerFolderDictionary::initEnsemblesFromFile()
{
	std::string xml_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"foldertypes.xml");
	LLXmlTree folder_def;
	if (!folder_def.parseFile(xml_filename))
	{
		llerrs << "Failed to parse folders file " << xml_filename << llendl;
		return false;
	}

	LLXmlTreeNode* rootp = folder_def.getRoot();
	for (LLXmlTreeNode* ensemble = rootp->getFirstChild();
		 ensemble;
		 ensemble = rootp->getNextChild())
	{
		if (!ensemble->hasName("ensemble"))
		{
			llwarns << "Invalid ensemble definition node " << ensemble->getName() << llendl;
			continue;
		}

		S32 ensemble_type;
		static LLStdStringHandle ensemble_num_string = LLXmlTree::addAttributeString("foldertype_num");
		if (!ensemble->getFastAttributeS32(ensemble_num_string, ensemble_type))
		{
			llwarns << "No ensemble type defined" << llendl;
			continue;
		}


		if (ensemble_type < S32(LLFolderType::FT_ENSEMBLE_START) || ensemble_type > S32(LLFolderType::FT_ENSEMBLE_END))
		{
			llwarns << "Exceeded maximum ensemble index" << LLFolderType::FT_ENSEMBLE_END << llendl;
			break;
		}

		std::string xui_name;
		static LLStdStringHandle xui_name_string = LLXmlTree::addAttributeString("xui_name");
		if (!ensemble->getFastAttributeString(xui_name_string, xui_name))
		{
			llwarns << "No xui name defined" << llendl;
			continue;
		}

		std::string icon_name;
		static LLStdStringHandle icon_name_string = LLXmlTree::addAttributeString("icon_name");
		if (!ensemble->getFastAttributeString(icon_name_string, icon_name))
		{
			llwarns << "No ensemble icon name defined" << llendl;
			continue;
		}

		std::string allowed_names;
		static LLStdStringHandle allowed_names_string = LLXmlTree::addAttributeString("allowed");
		if (!ensemble->getFastAttributeString(allowed_names_string, allowed_names))
		{
		}

		// Add the entry and increment the asset number.
		const static std::string new_ensemble_name = "New Ensemble";
		addEntry(LLFolderType::EType(ensemble_type), new ViewerFolderEntry(xui_name, new_ensemble_name, icon_name, allowed_names));
	}

	return true;
}


const std::string &LLViewerFolderType::lookupXUIName(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mName;
	}
	return badLookup();
}

LLFolderType::EType LLViewerFolderType::lookupTypeFromXUIName(const std::string &name)
{
	return LLViewerFolderDictionary::getInstance()->lookup(name);
}

const std::string &LLViewerFolderType::lookupIconName(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mIconName;
	}
	return badLookup();
}

const std::string &LLViewerFolderType::lookupNewCategoryName(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mNewCategoryName;
	}
	return badLookup();
}

LLFolderType::EType LLViewerFolderType::lookupTypeFromNewCategoryName(const std::string& name)
{
	for (LLViewerFolderDictionary::const_iterator iter = LLViewerFolderDictionary::getInstance()->begin();
		 iter != LLViewerFolderDictionary::getInstance()->end();
		 iter++)
	{
		const ViewerFolderEntry *entry = iter->second;
		if (entry->mNewCategoryName == name)
		{
			return iter->first;
		}
	}
	return FT_NONE;
}


U64 LLViewerFolderType::lookupValidFolderTypes(const std::string& item_name)
{
	U64 matching_folders = 0;
	for (LLViewerFolderDictionary::const_iterator iter = LLViewerFolderDictionary::getInstance()->begin();
		 iter != LLViewerFolderDictionary::getInstance()->end();
		 iter++)
	{
		const ViewerFolderEntry *entry = iter->second;
		if (entry->getIsAllowedName(item_name))
		{
			matching_folders |= 1LL << iter->first;
		}
	}
	return matching_folders;
}
