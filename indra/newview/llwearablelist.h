/** 
 * @file llwearablelist.h
 * @brief LLWearableList class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLWEARABLELIST_H
#define LL_LLWEARABLELIST_H

#include "llmemory.h"
#include "llviewerwearable.h"
#include "lluuid.h"
#include "llassetstorage.h"

// Globally constructed; be careful that there's no dependency with gAgent.
/* 
   BUG: mList's system of mapping between assetIDs and wearables is flawed
   since LLWearable* has an associated itemID, and you can have multiple 
   inventory items pointing to the same asset (i.e. more than one ItemID
   per assetID).  EXT-6252
*/
class LLWearableList : public LLSingleton<LLWearableList>
{
public:
	LLWearableList()	{}
	~LLWearableList();
	void cleanup() ;

	S32					getLength() const { return mList.size(); }

	void				getAsset(const LLAssetID& assetID,
								 const std::string& wearable_name,
								 LLAvatarAppearance *avatarp,
								 LLAssetType::EType asset_type,
								 void(*asset_arrived_callback)(LLViewerWearable*, void* userdata),
								 void* userdata);

	LLViewerWearable*			createCopy(const LLViewerWearable* old_wearable, const std::string& new_name = std::string());
	LLViewerWearable*			createNewWearable(LLWearableType::EType type, LLAvatarAppearance *avatarp);
	
	// Callback
	static void	 	    processGetAssetReply(const char* filename, const LLAssetID& assetID, void* user_data, S32 status, LLExtStat ext_status);

protected:
	LLViewerWearable* generateNewWearable(); // used for the create... functions
private:
	std::map<LLUUID, LLViewerWearable*> mList;
};

#endif  // LL_LLWEARABLELIST_H
