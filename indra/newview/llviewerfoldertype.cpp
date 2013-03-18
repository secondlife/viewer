/** 
 * @file llfoldertype.cpp
 * @brief Implementation of LLViewerFolderType functionality.
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
					  const std::string &icon_name_open,	// name of the folder icon
					  const std::string &icon_name_closed,
					  BOOL is_quiet,						// folder doesn't need a UI update when changed
					  bool hide_if_empty,					// folder not shown if empty
					  const std::string &dictionary_name = empty_string // no reverse lookup needed on non-ensembles, so in most cases just leave this blank
		) 
		:
		LLDictionaryEntry(dictionary_name),
		mNewCategoryName(new_category_name),
		mIconNameOpen(icon_name_open),
		mIconNameClosed(icon_name_closed),
		mIsQuiet(is_quiet),
		mHideIfEmpty(hide_if_empty)
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
		/* Just use default icons until we actually support ensembles
		mIconNameOpen(icon_name),
		mIconNameClosed(icon_name),
		*/
		mIconNameOpen("Inv_FolderOpen"), mIconNameClosed("Inv_FolderClosed"),
		mNewCategoryName(new_category_name),
		mIsQuiet(FALSE),
		mHideIfEmpty(false)
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
	const std::string mIconNameOpen;
	const std::string mIconNameClosed;
	const std::string mNewCategoryName;
	typedef std::vector<std::string> name_vec_t;
	name_vec_t mAllowedNames;
	BOOL mIsQuiet;
	bool mHideIfEmpty;
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
	//       													    	  NEW CATEGORY NAME         FOLDER OPEN             FOLDER CLOSED          QUIET?      HIDE IF EMPTY?
	//      												  		     |-------------------------|-----------------------|----------------------|-----------|--------------|
	addEntry(LLFolderType::FT_TEXTURE, 				new ViewerFolderEntry("Textures",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_SOUND, 				new ViewerFolderEntry("Sounds",					"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_CALLINGCARD, 			new ViewerFolderEntry("Calling Cards",			"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_LANDMARK, 			new ViewerFolderEntry("Landmarks",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_CLOTHING, 			new ViewerFolderEntry("Clothing",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_OBJECT, 				new ViewerFolderEntry("Objects",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_NOTECARD, 			new ViewerFolderEntry("Notecards",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_ROOT_INVENTORY, 		new ViewerFolderEntry("My Inventory",			"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     false));
	addEntry(LLFolderType::FT_LSL_TEXT, 			new ViewerFolderEntry("Scripts",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_BODYPART, 			new ViewerFolderEntry("Body Parts",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_TRASH, 				new ViewerFolderEntry("Trash",					"Inv_TrashOpen",		"Inv_TrashClosed",		TRUE,      false));
	addEntry(LLFolderType::FT_SNAPSHOT_CATEGORY, 	new ViewerFolderEntry("Photo Album",			"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_LOST_AND_FOUND, 		new ViewerFolderEntry("Lost And Found",	   		"Inv_LostOpen",			"Inv_LostClosed",		TRUE,      true));
	addEntry(LLFolderType::FT_ANIMATION, 			new ViewerFolderEntry("Animations",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_GESTURE, 				new ViewerFolderEntry("Gestures",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_FAVORITE, 			new ViewerFolderEntry("Favorites",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));

	addEntry(LLFolderType::FT_CURRENT_OUTFIT, 		new ViewerFolderEntry("Current Outfit",			"Inv_SysOpen",			"Inv_SysClosed",		TRUE,      false));
	addEntry(LLFolderType::FT_OUTFIT, 				new ViewerFolderEntry("New Outfit",				"Inv_LookFolderOpen",	"Inv_LookFolderClosed",	TRUE,      true));
	addEntry(LLFolderType::FT_MY_OUTFITS, 			new ViewerFolderEntry("My Outfits",				"Inv_SysOpen",			"Inv_SysClosed",		TRUE,      true));
	addEntry(LLFolderType::FT_MESH, 				new ViewerFolderEntry("Meshes",					"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	
	addEntry(LLFolderType::FT_INBOX, 				new ViewerFolderEntry("Inbox",					"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
	addEntry(LLFolderType::FT_OUTBOX, 				new ViewerFolderEntry("Outbox",					"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));

	addEntry(LLFolderType::FT_BASIC_ROOT, 			new ViewerFolderEntry("Basic Root",				"Inv_SysOpen",			"Inv_SysClosed",		FALSE,     true));
		 
	addEntry(LLFolderType::FT_NONE, 				new ViewerFolderEntry("New Folder",				"Inv_FolderOpen",		"Inv_FolderClosed",		FALSE,     false, "default"));

#if SUPPORT_ENSEMBLES
	initEnsemblesFromFile();
#else
	for (U32 type = (U32)LLFolderType::FT_ENSEMBLE_START; type <= (U32)LLFolderType::FT_ENSEMBLE_END; ++type)
	{
		addEntry((LLFolderType::EType)type, 		new ViewerFolderEntry("New Folder",				"Inv_FolderOpen",		"Inv_FolderClosed",		FALSE,     false));
	}	
#endif
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

const std::string &LLViewerFolderType::lookupIconName(LLFolderType::EType folder_type, BOOL is_open)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		if (is_open)
			return entry->mIconNameOpen;
		else
			return entry->mIconNameClosed;
	}
	
	// Error condition.  Return something so that we don't show a grey box in inventory view.
	const ViewerFolderEntry *default_entry = LLViewerFolderDictionary::getInstance()->lookup(LLFolderType::FT_NONE);
	if (default_entry)
	{
		return default_entry->mIconNameClosed;
	}
	
	// Should not get here unless there's something corrupted with the FT_NONE entry.
	return badLookup();
}

BOOL LLViewerFolderType::lookupIsQuietType(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mIsQuiet;
	}
	return FALSE;
}

bool LLViewerFolderType::lookupIsHiddenIfEmpty(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mHideIfEmpty;
	}
	return false;
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
