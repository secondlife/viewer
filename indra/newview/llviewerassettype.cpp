/** 
 * @file llassettype.cpp
 * @brief Implementatino of LLViewerAssetType functionality.
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
	
	addEntry(LLViewerAssetType::AT_WIDGET, 				new ViewerAssetEntry(DAD_WIDGET));
	
	addEntry(LLViewerAssetType::AT_PERSON, 				new ViewerAssetEntry(DAD_PERSON));

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
