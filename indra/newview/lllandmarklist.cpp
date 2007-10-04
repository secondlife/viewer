/** 
 * @file lllandmarklist.cpp
 * @brief Landmark asset list class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
	S32 status, 
	LLExtStat ext_status )
{
	if( status == 0 )
	{
		LLVFile file(vfs, uuid, type);
		S32 file_length = file.getSize();

		char* buffer = new char[ file_length + 1 ];
		file.read( (U8*)buffer, file_length);		/*Flawfinder: ignore*/
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
