/** 
 * @file lllandmarklist.cpp
 * @brief Landmark asset list class
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

#include "llviewerprecompiledheaders.h"

#include "lllandmarklist.h"

#include "message.h"
#include "llassetstorage.h"

#include "llappviewer.h"
#include "llagent.h"
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

LLLandmark* LLLandmarkList::getAsset(const LLUUID& asset_uuid, loaded_callback_t cb)
{
	LLLandmark* landmark = get_ptr_in_map(mList, asset_uuid);
	if(landmark)
	{
		if(cb && !landmark->getGlobalPos(LLVector3d()))
		{
			// landmark is not completely loaded yet
			loaded_callback_map_t::value_type vt(asset_uuid, cb);
			mLoadedCallbackMap.insert(vt);
		}
		return landmark;
	}
	else
	{
	    if ( mBadList.find(asset_uuid) != mBadList.end() )
		{
			return NULL;
		}
		
		landmark_requested_list_t::iterator iter = mRequestedList.find(asset_uuid);
		if (iter != mRequestedList.end())
		{
			const F32 rerequest_time = 30.f; // 30 seconds between requests
			if (gFrameTimeSeconds - iter->second < rerequest_time)
			{
				return NULL;
			}
		}
		
		if (cb)
		{
			loaded_callback_map_t::value_type vt(asset_uuid, cb);
			mLoadedCallbackMap.insert(vt);
		}

		gAssetStorage->getAssetData(asset_uuid,
									LLAssetType::AT_LANDMARK,
									LLLandmarkList::processGetAssetReply,
									NULL);
		mRequestedList[asset_uuid] = gFrameTimeSeconds;
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

		std::vector<char> buffer(file_length + 1);
		file.read( (U8*)&buffer[0], file_length);
		buffer[ file_length ] = 0;

		LLLandmark* landmark = LLLandmark::constructFromString(&buffer[0]);
		if (landmark)
		{
			gLandmarkList.mList[ uuid ] = landmark;
			gLandmarkList.mRequestedList.erase(uuid);
			
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
						boost::bind(&LLLandmarkList::onRegionHandle, &gLandmarkList, uuid));
				}

				// the callback will be called when we get the region handle.
			}
			else
			{
				gLandmarkList.makeCallbacks(uuid);
			}
		}
	}
	else
	{
		LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
		// SJB: No use case for a notification here. Use lldebugs instead
		if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status )
		{
			LL_WARNS("Landmarks") << "Missing Landmark" << LL_ENDL;
			//LLNotificationsUtil::add("LandmarkMissing");
		}
		else
		{
			LL_WARNS("Landmarks") << "Unable to load Landmark" << LL_ENDL;
			//LLNotificationsUtil::add("UnableToLoadLandmark");
		}

		gLandmarkList.mBadList.insert(uuid);
	}

}

BOOL LLLandmarkList::assetExists(const LLUUID& asset_uuid)
{
	return mList.count(asset_uuid) != 0 || mBadList.count(asset_uuid) != 0;
}

void LLLandmarkList::onRegionHandle(const LLUUID& landmark_id)
{
	LLLandmark* landmark = getAsset(landmark_id);

	if (!landmark)
	{
		llwarns << "Got region handle but the landmark not found." << llendl;
		return;
	}

	// Calculate landmark global position.
	// This should succeed since the region handle is available.
	LLVector3d pos;
	if (!landmark->getGlobalPos(pos))
	{
		llwarns << "Got region handle but the landmark global position is still unknown." << llendl;
		return;
	}

	makeCallbacks(landmark_id);
}

void LLLandmarkList::makeCallbacks(const LLUUID& landmark_id)
{
	LLLandmark* landmark = getAsset(landmark_id);

	if (!landmark)
	{
		llwarns << "Landmark to make callbacks for not found." << llendl;
	}

	// make all the callbacks here.
	loaded_callback_map_t::iterator it;
	while((it = mLoadedCallbackMap.find(landmark_id)) != mLoadedCallbackMap.end())
	{
		if (landmark)
			(*it).second(landmark);

		mLoadedCallbackMap.erase(it);
	}
}
