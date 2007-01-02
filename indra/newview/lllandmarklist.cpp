/** 
 * @file lllandmarklist.cpp
 * @brief Landmark asset list class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lllandmarklist.h"

#include "message.h"
#include "llassetstorage.h"

#include "llagent.h"
#include "llnotify.h"
#include "llvfile.h"
#include "llviewerstats.h"

// Globals
LLLandmarkList gLandmarkList;


////////////////////////////////////////////////////////////////////////////
// LLLandmarkList

LLLandmarkList::~LLLandmarkList()
{
	std::for_each(mList.begin(), mList.end(), DeletePairedPointer());
}

LLLandmark* LLLandmarkList::getAsset( const LLUUID& asset_uuid )
{
	LLLandmark* landmark = get_ptr_in_map(mList, asset_uuid);
	if(landmark)
	{
		return landmark;
	}
	else
	{
	    if ( gLandmarkList.mBadList.find(asset_uuid) == gLandmarkList.mBadList.end() )
		{
			gAssetStorage->getAssetData(
				asset_uuid,
				LLAssetType::AT_LANDMARK,
				LLLandmarkList::processGetAssetReply,
				NULL);
		}
	}
	return NULL;
}

// static
void LLLandmarkList::processGetAssetReply(
	LLVFS *vfs,
	const LLUUID& uuid,
	LLAssetType::EType type,
	void* user_data,
	S32 status)
{
	if( status == 0 )
	{
		LLVFile file(vfs, uuid, type);
		S32 file_length = file.getSize();

		char* buffer = new char[ file_length + 1 ];
		file.read( (U8*)buffer, file_length);
		buffer[ file_length ] = 0;

		LLLandmark* landmark = LLLandmark::constructFromString(buffer);
		if (landmark)
		{
			LLVector3d pos;
			if(!landmark->getGlobalPos(pos))
			{
				LLUUID region_id;
				if(landmark->getRegionID(region_id))
				{
					LLLandmark::requestRegionHandle(
						gMessageSystem,
						gAgent.getRegionHost(),
						region_id,
						NULL);
				}
			}
			gLandmarkList.mList[ uuid ] = landmark;
		}

		delete[] buffer;
	}
	else
	{
		if( gViewerStats )
		{
			gViewerStats->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
		}

		if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status )
		{
			LLNotifyBox::showXml("LandmarkMissing");
		}
		else
		{
			LLNotifyBox::showXml("UnableToLoadLandmark");
		}

		gLandmarkList.mBadList.insert(uuid);
	}

}

BOOL LLLandmarkList::assetExists(const LLUUID& asset_uuid)
{
	return mList.count(asset_uuid) != 0 || mBadList.count(asset_uuid) != 0;
}
