/** 
 * @file llassettype.cpp
 * @brief Implementatino of LLAssetType functionality.
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

#include "llassettype.h"
#include "lldictionary.h"
#include "llmemory.h"
#include "llsingleton.h"

///----------------------------------------------------------------------------
/// Class LLAssetType
///----------------------------------------------------------------------------
struct AssetEntry : public LLDictionaryEntry
{
	AssetEntry(const char *desc_name,
			   const char *type_name, // 8 character limit!
			   const char *human_name,
			   const char *category_name, // used by llinventorymodel when creating new categories
			   EDragAndDropType dad_type);

	// limited to 8 characters
	const char *mTypeName;
	// human readable form.  Put as many printable characters you want in each one.
	// (c.f. llinventory.cpp INVENTORY_TYPE_HUMAN_NAMES).
	const char *mHumanName;
	const char *mCategoryName;
	EDragAndDropType mDadType;
};

class LLAssetDictionary : public LLSingleton<LLAssetDictionary>,
						  public LLDictionary<LLAssetType::EType, AssetEntry>
{
public:
	LLAssetDictionary();
};

LLAssetDictionary::LLAssetDictionary()
{
	addEntry(LLAssetType::AT_TEXTURE, 			new AssetEntry("TEXTURE",			"texture",	"texture",			"Textures", 		DAD_TEXTURE));
	addEntry(LLAssetType::AT_SOUND, 			new AssetEntry("SOUND",				"sound",	"sound",			"Sounds", 			DAD_SOUND));
	addEntry(LLAssetType::AT_CALLINGCARD, 		new AssetEntry("CALLINGCARD",		"callcard",	"calling card",		"Calling Cards", 	DAD_CALLINGCARD));
	addEntry(LLAssetType::AT_LANDMARK, 			new AssetEntry("LANDMARK",			"landmark",	"landmark",			"Landmarks", 		DAD_LANDMARK));
	addEntry(LLAssetType::AT_SCRIPT, 			new AssetEntry("SCRIPT",			"script",	"legacy script",	"Scripts", 			DAD_NONE));
	addEntry(LLAssetType::AT_CLOTHING, 			new AssetEntry("CLOTHING",			"clothing",	"clothing",			"Clothing", 		DAD_CLOTHING));
	addEntry(LLAssetType::AT_OBJECT, 			new AssetEntry("OBJECT",			"object",	"object",			"Objects", 			DAD_OBJECT));
	addEntry(LLAssetType::AT_NOTECARD, 			new AssetEntry("NOTECARD",			"notecard",	"note card",		"Notecards", 		DAD_NOTECARD));
	addEntry(LLAssetType::AT_CATEGORY, 			new AssetEntry("CATEGORY",			"category",	"folder",			"New Folder", 		DAD_CATEGORY));
	addEntry(LLAssetType::AT_ROOT_CATEGORY, 	new AssetEntry("ROOT_CATEGORY",		"root",		"root",				"Inventory", 		DAD_ROOT_CATEGORY));
	addEntry(LLAssetType::AT_LSL_TEXT, 			new AssetEntry("LSL_TEXT",			"lsltext",	"lsl2 script",		"Scripts", 			DAD_SCRIPT));
	addEntry(LLAssetType::AT_LSL_BYTECODE, 		new AssetEntry("LSL_BYTECODE",		"lslbyte",	"lsl bytecode",		"Scripts", 			DAD_NONE));
	addEntry(LLAssetType::AT_TEXTURE_TGA, 		new AssetEntry("TEXTURE_TGA",		"txtr_tga",	"tga texture",		"Uncompressed Images", DAD_NONE));
	addEntry(LLAssetType::AT_BODYPART, 			new AssetEntry("BODYPART",			"bodypart",	"body part",		"Body Parts", 		DAD_BODYPART));
	addEntry(LLAssetType::AT_TRASH, 			new AssetEntry("TRASH",				"trash",	"trash",			"Trash", 			DAD_NONE));
	addEntry(LLAssetType::AT_SNAPSHOT_CATEGORY, new AssetEntry("SNAPSHOT_CATEGORY", "snapshot",	"snapshot",			"Photo Album", 		DAD_NONE));
	addEntry(LLAssetType::AT_LOST_AND_FOUND, 	new AssetEntry("LOST_AND_FOUND", 	"lstndfnd",	"lost and found",	"Lost And Found", 	DAD_NONE));
	addEntry(LLAssetType::AT_SOUND_WAV, 		new AssetEntry("SOUND_WAV",			"snd_wav",	"sound",			"Uncompressed Sounds", DAD_NONE));
	addEntry(LLAssetType::AT_IMAGE_TGA, 		new AssetEntry("IMAGE_TGA",			"img_tga",	"targa image",		"Uncompressed Images", DAD_NONE));
	addEntry(LLAssetType::AT_IMAGE_JPEG, 		new AssetEntry("IMAGE_JPEG",		"jpeg",		"jpeg image",		"Uncompressed Images", DAD_NONE));
	addEntry(LLAssetType::AT_ANIMATION, 		new AssetEntry("ANIMATION",			"animatn",	"animation",		"Animations", 		DAD_ANIMATION));
	addEntry(LLAssetType::AT_GESTURE, 			new AssetEntry("GESTURE",			"gesture",	"gesture",			"Gestures", 		DAD_GESTURE));
	addEntry(LLAssetType::AT_SIMSTATE, 			new AssetEntry("SIMSTATE",			"simstate",	"simstate",			"New Folder", 		DAD_NONE));
	addEntry(LLAssetType::AT_LINK, 				new AssetEntry("LINK",				"link",		"symbolic  link",	"New Folder", 		DAD_NONE));
	addEntry(LLAssetType::AT_FAVORITE, 			new AssetEntry("FAVORITE",			"favorite",	"favorite",			"favorite", 		DAD_NONE));
	addEntry(LLAssetType::AT_NONE, 				new AssetEntry("NONE",				"-1",		NULL,		  		"New Folder", 		DAD_NONE));
};

AssetEntry::AssetEntry(const char *desc_name,
					   const char *type_name,
					   const char *human_name,
					   const char *category_name,
					   EDragAndDropType dad_type) :
	LLDictionaryEntry(desc_name),
	mTypeName(type_name),
	mHumanName(human_name),
	mCategoryName(category_name),
	mDadType(dad_type)
{
	llassert(strlen(mTypeName) <= 8);
}

// static
LLAssetType::EType LLAssetType::getType(const std::string& desc_name)
{
	std::string s = desc_name;
	LLStringUtil::toUpper(s);
	return LLAssetDictionary::getInstance()->lookup(s);
}

// static
const std::string &LLAssetType::getDesc(LLAssetType::EType asset_type)
{
	const AssetEntry *entry = LLAssetDictionary::getInstance()->lookup(asset_type);
	if (entry)
{ 
		return entry->mName;
	}
	else
	{ 
		static const std::string error_string = "BAD TYPE";
		return error_string;
	}
}

// static
const char *LLAssetType::lookup(LLAssetType::EType asset_type)
{
	const LLAssetDictionary *dict = LLAssetDictionary::getInstance();
	const AssetEntry *entry = dict->lookup(asset_type);
	if (entry)
	{
		return entry->mTypeName;
	}
	else
	{
		return "-1";
	}
}

// static
LLAssetType::EType LLAssetType::lookup( const char* name )
{
	return lookup(ll_safe_string(name));
}

LLAssetType::EType LLAssetType::lookup(const std::string& type_name)
{
	const LLAssetDictionary *dict = LLAssetDictionary::getInstance();
	for (LLAssetDictionary::const_iterator iter = dict->begin();
		 iter != dict->end();
		 iter++)
	{
		const AssetEntry *entry = iter->second;
		if (type_name == entry->mTypeName)
		{
			return iter->first;
		}
	}
	return AT_NONE;
}

// static
const char *LLAssetType::lookupHumanReadable(LLAssetType::EType asset_type)
{
	const LLAssetDictionary *dict = LLAssetDictionary::getInstance();
	const AssetEntry *entry = dict->lookup(asset_type);
	if (entry)
	{
		return entry->mHumanName;
	}
	else
	{
		return NULL;
	}
}

// static
LLAssetType::EType LLAssetType::lookupHumanReadable( const char* name )
{
	return lookupHumanReadable(ll_safe_string(name));
}

LLAssetType::EType LLAssetType::lookupHumanReadable(const std::string& readable_name)
{
	const LLAssetDictionary *dict = LLAssetDictionary::getInstance();
	for (LLAssetDictionary::const_iterator iter = dict->begin();
		 iter != dict->end();
		 iter++)
	{
		const AssetEntry *entry = iter->second;
		if (readable_name == entry->mHumanName)
		{
			return iter->first;
		}
	}
	return AT_NONE;
}

// static
const char *LLAssetType::lookupCategoryName(LLAssetType::EType asset_type)
{
	const LLAssetDictionary *dict = LLAssetDictionary::getInstance();
	const AssetEntry *entry = dict->lookup(asset_type);
	if (entry)
	{
		return entry->mCategoryName;
	}
	else
	{
		return "New Folder";
	}
}

// static
EDragAndDropType LLAssetType::lookupDragAndDropType(EType asset_type)
{
	const LLAssetDictionary *dict = LLAssetDictionary::getInstance();
	const AssetEntry *entry = dict->lookup(asset_type);
	if (entry)
		return entry->mDadType;
	else
		return DAD_NONE;
}

// static. Generate a good default description
void LLAssetType::generateDescriptionFor(LLAssetType::EType asset_type,
										 std::string& description)
{
	const S32 BUF_SIZE = 30;
	char time_str[BUF_SIZE];	/* Flawfinder: ignore */
	time_t now;
	time(&now);
	memset(time_str, '\0', BUF_SIZE);
	strftime(time_str, BUF_SIZE - 1, "%Y-%m-%d %H:%M:%S ", localtime(&now));
	description.assign(time_str);
	description.append(LLAssetType::lookupHumanReadable(asset_type));
}
