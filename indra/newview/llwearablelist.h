/** 
 * @file llwearablelist.h
 * @brief LLWearableList class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLWEARABLELIST_H
#define LL_LLWEARABLELIST_H

#include "llwearable.h"
#include "lluuid.h"
#include "llassetstorage.h"

class LLWearableList
{
public:
	LLWearableList()	{}
	~LLWearableList();

	S32					getLength() { return mList.size(); }

	void				getAsset( 
							const LLAssetID& assetID,
							const std::string& wearable_name,
							LLAssetType::EType asset_type,
							void(*asset_arrived_callback)(LLWearable*, void* userdata),
							void* userdata );

	LLWearable*			createWearableMatchedToInventoryItem( LLWearable* old_wearable, LLViewerInventoryItem* item );
	LLWearable*			createCopyFromAvatar( LLWearable* old_wearable, const std::string& new_name = std::string() );
	LLWearable*			createCopy( LLWearable* old_wearable );
	LLWearable*			createNewWearable( EWearableType type );
	
	// Callback
	static void	 	    processGetAssetReply(const char* filename, const LLAssetID& assetID, void* user_data, S32 status, LLExtStat ext_status);

protected:
	std::map< LLUUID, LLWearable* > mList;
};

extern LLWearableList gWearableList;

#endif  // LL_LLWEARABLELIST_H
