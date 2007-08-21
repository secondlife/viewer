/** 
 * @file llwearablelist.h
 * @brief LLWearableList class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLWEARABLELIST_H
#define LL_LLWEARABLELIST_H

#include "llwearable.h"
#include "llskiplist.h"
#include "lluuid.h"
#include "llassetstorage.h"

class LLWearableList
{
public:
	LLWearableList()	{}
	~LLWearableList();

	S32					getLength() { return mList.getLength(); }
	const LLWearable*	getFirst()	{ return mList.getFirstData(); }
	const LLWearable*	getNext()	{ return mList.getNextData(); }

	void				getAsset( 
							const LLAssetID& assetID,
							const LLString& wearable_name,
							LLAssetType::EType asset_type,
							void(*asset_arrived_callback)(LLWearable*, void* userdata),
							void* userdata );

	LLWearable*			createLegacyWearableFromAvatar( EWearableType type );

	LLWearable*			createWearableMatchedToInventoryItem( LLWearable* old_wearable, LLViewerInventoryItem* item );
	LLWearable*			createCopyFromAvatar( LLWearable* old_wearable, const std::string& new_name = "" );
	LLWearable*			createCopy( LLWearable* old_wearable );
	LLWearable*			createNewWearable( EWearableType type );
	
	// Pseudo-private
	static void	 	    processGetAssetReply(const char* filename, const LLAssetID& assetID, void* user_data, S32 status, LLExtStat ext_status);

protected:
	LLPtrSkipMap< const LLUUID, LLWearable* > mList;
};

extern LLWearableList gWearableList;

#endif  // LL_LLWEARABLELIST_H
