/** 
 * @file llassettype.cpp
 * @brief Implementatino of LLViewerAssetType functionality.
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

#include "llviewerassettype.h"
#include "lldictionary.h"
#include "llmemory.h"
#include "llsingleton.h"

static const std::string empty_string;

struct ViewerAssetEntry : public LLDictionaryEntry
{
	ViewerAssetEntry(EDragAndDropType dad_type // drag and drop type
		)
		:
		LLDictionaryEntry(empty_string), // no reverse lookup needed for now, so just leave this blank
		mDadType(dad_type)
	{
	}
	EDragAndDropType mDadType;
};

class LLViewerAssetDictionary : public LLSingleton<LLViewerAssetDictionary>,
						  public LLDictionary<LLViewerAssetType::EType, ViewerAssetEntry>
{
public:
	LLViewerAssetDictionary();
};

LLViewerAssetDictionary::LLViewerAssetDictionary()
{
	//       												      	   	   	 DRAG&DROP TYPE		    
	//   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	|--------------------|
	addEntry(LLViewerAssetType::AT_TEXTURE, 			new ViewerAssetEntry(DAD_TEXTURE));
	addEntry(LLViewerAssetType::AT_SOUND, 				new ViewerAssetEntry(DAD_SOUND));
	addEntry(LLViewerAssetType::AT_CALLINGCARD, 		new ViewerAssetEntry(DAD_CALLINGCARD));
	addEntry(LLViewerAssetType::AT_LANDMARK, 			new ViewerAssetEntry(DAD_LANDMARK));
	addEntry(LLViewerAssetType::AT_SCRIPT, 				new ViewerAssetEntry(DAD_NONE));
	addEntry(LLViewerAssetType::AT_CLOTHING, 			new ViewerAssetEntry(DAD_CLOTHING));
	addEntry(LLViewerAssetType::AT_OBJECT, 				new ViewerAssetEntry(DAD_OBJECT));
	addEntry(LLViewerAssetType::AT_NOTECARD, 			new ViewerAssetEntry(DAD_NOTECARD));
	addEntry(LLViewerAssetType::AT_CATEGORY, 			new ViewerAssetEntry(DAD_CATEGORY));
	addEntry(LLViewerAssetType::AT_ROOT_CATEGORY, 		new ViewerAssetEntry(DAD_ROOT_CATEGORY));
	addEntry(LLViewerAssetType::AT_LSL_TEXT, 			new ViewerAssetEntry(DAD_SCRIPT));
	addEntry(LLViewerAssetType::AT_LSL_BYTECODE, 		new ViewerAssetEntry(DAD_NONE));
	addEntry(LLViewerAssetType::AT_TEXTURE_TGA, 		new ViewerAssetEntry(DAD_NONE));
	addEntry(LLViewerAssetType::AT_BODYPART, 			new ViewerAssetEntry(DAD_BODYPART));
	addEntry(LLViewerAssetType::AT_SOUND_WAV, 			new ViewerAssetEntry(DAD_NONE));
	addEntry(LLViewerAssetType::AT_IMAGE_TGA, 			new ViewerAssetEntry(DAD_NONE));
	addEntry(LLViewerAssetType::AT_IMAGE_JPEG, 			new ViewerAssetEntry(DAD_NONE));
	addEntry(LLViewerAssetType::AT_ANIMATION, 			new ViewerAssetEntry(DAD_ANIMATION));
	addEntry(LLViewerAssetType::AT_GESTURE, 			new ViewerAssetEntry(DAD_GESTURE));
	addEntry(LLViewerAssetType::AT_SIMSTATE, 			new ViewerAssetEntry(DAD_NONE));

	addEntry(LLViewerAssetType::AT_LINK, 				new ViewerAssetEntry(DAD_LINK));
	addEntry(LLViewerAssetType::AT_LINK_FOLDER, 		new ViewerAssetEntry(DAD_LINK));

	addEntry(LLViewerAssetType::AT_MESH, 				new ViewerAssetEntry(DAD_MESH));

	addEntry(LLViewerAssetType::AT_NONE, 				new ViewerAssetEntry(DAD_NONE));
};

EDragAndDropType LLViewerAssetType::lookupDragAndDropType(EType asset_type)
{
	const LLViewerAssetDictionary *dict = LLViewerAssetDictionary::getInstance();
	const ViewerAssetEntry *entry = dict->lookup(asset_type);
	if (entry)
		return entry->mDadType;
	else
		return DAD_NONE;
}

// Generate a good default description
void LLViewerAssetType::generateDescriptionFor(LLViewerAssetType::EType asset_type,
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
