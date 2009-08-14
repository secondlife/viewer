/** 
* @file lllandmarkactions.cpp
* @brief LLLandmarkActions class implementation
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
#include "lllandmarkactions.h"

#include "roles_constants.h"

#include "llinventory.h"
#include "lllandmark.h"
#include "llparcel.h"

#include "llnotifications.h"

#include "llagent.h"
#include "llinventorymodel.h"
#include "lllandmarklist.h"
#include "llslurl.h"
#include "llviewerinventory.h"
#include "llviewerparcelmgr.h"
#include "llworldmap.h"
#include "lllandmark.h"
#include "llinventorymodel.h"

// Returns true if the given inventory item is a landmark pointing to the current parcel.
// Used to filter inventory items.
class LLIsAgentParcelLandmark : public LLInventoryCollectFunctor
{
public:
	/*virtual*/ bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
	{
		if (!item || item->getType() != LLAssetType::AT_LANDMARK)
			return false;

		LLLandmark* landmark = gLandmarkList.getAsset(item->getAssetUUID());
		if (!landmark) // the landmark not been loaded yet
			return false;

		LLVector3d landmark_global_pos;
		if (!landmark->getGlobalPos(landmark_global_pos))
			return false;

		return LLViewerParcelMgr::getInstance()->inAgentParcel(landmark_global_pos);
	}
};

class LLFetchLandmarksByName : public LLInventoryCollectFunctor
{
private:
	std::string name;

public:
LLFetchLandmarksByName(std::string &landmark_name)
:name(landmark_name)
	{
	}

public:
	/*virtual*/ bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
	{
		if (!item || item->getType() != LLAssetType::AT_LANDMARK)
			return false;

		LLLandmark* landmark = gLandmarkList.getAsset(item->getAssetUUID());
		if (!landmark) // the landmark not been loaded yet
			return false;

		if (item->getName() == name)
		{
			return true;
		}

		return false;
	}
};

LLInventoryModel::item_array_t LLLandmarkActions::fetchLandmarksByName(std::string& name)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLFetchLandmarksByName fetchLandmarks(name);
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
			cats,
			items,
			LLInventoryModel::EXCLUDE_TRASH,
			fetchLandmarks);
	return items;
}
bool LLLandmarkActions::landmarkAlreadyExists()
{
	// Determine whether there are landmarks pointing to the current parcel.
	LLInventoryModel::item_array_t items;
	collectParcelLandmark(items);
	return !items.empty();
}

bool LLLandmarkActions::canCreateLandmarkHere()
{
	LLParcel* agent_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if(!agent_parcel)
	{
		llwarns << "No agent region" << llendl;
		return false;
	}
	if (agent_parcel->getAllowLandmark()
		|| LLViewerParcelMgr::isParcelOwnedByAgent(agent_parcel, GP_LAND_ALLOW_LANDMARK))
	{
		return true;
	}

	return false;
}

void LLLandmarkActions::createLandmarkHere(
	const std::string& name, 
	const std::string& desc, 
	const LLUUID& folder_id)
{
	if(!gAgent.getRegion())
	{
		llwarns << "No agent region" << llendl;
		return;
	}
	LLParcel* agent_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (!agent_parcel)
	{
		llwarns << "No agent parcel" << llendl;
		return;
	}
	if (!canCreateLandmarkHere())
	{
		LLNotifications::instance().add("CannotCreateLandmarkNotOwner");
		return;
	}

	create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
		folder_id, LLTransactionID::tnull,
		name, desc,
		LLAssetType::AT_LANDMARK,
		LLInventoryType::IT_LANDMARK,
		NOT_WEARABLE, PERM_ALL, 
		NULL);
}

void LLLandmarkActions::createLandmarkHere()
{
	std::string landmark_name, landmark_desc;

	gAgent.buildLocationString(landmark_name, LLAgent::LOCATION_FORMAT_LANDMARK);
	gAgent.buildLocationString(landmark_desc, LLAgent::LOCATION_FORMAT_FULL);
	LLUUID folder_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_LANDMARK);

	createLandmarkHere(landmark_name, landmark_desc, folder_id);
}

void LLLandmarkActions::getSLURLfromPosGlobal(const LLVector3d& global_pos, slurl_signal_t signal)
{
	std::string sim_name;
	bool gotSimName = LLWorldMap::getInstance()->simNameFromPosGlobal(global_pos, sim_name);
	if (gotSimName)
	{
		std::string slurl = LLSLURL::buildSLURLfromPosGlobal(sim_name, global_pos);

		signal(global_pos, slurl);

		return;
	}
	else
	{
		U64 new_region_handle = to_region_handle(global_pos);

		LLWorldMap::url_callback_t cb = boost::bind(&LLLandmarkActions::onRegionResponse,
													signal,
													global_pos,
													_1, _2, _3, _4);

		LLWorldMap::getInstance()->sendHandleRegionRequest(new_region_handle, cb, std::string("unused"), false);
	}
}

void LLLandmarkActions::onRegionResponse(slurl_signal_t signal,
										 const LLVector3d& global_pos,
										 U64 region_handle,
										 const std::string& url,
										 const LLUUID& snapshot_id,
										 bool teleport)
{
	std::string sim_name;
	std::string slurl;
	bool gotSimName = LLWorldMap::getInstance()->simNameFromPosGlobal(global_pos, sim_name);
	if (gotSimName)
	{
		slurl = LLSLURL::buildSLURLfromPosGlobal(sim_name, global_pos);
	}
	else
	{
		slurl = "";
	}

	signal(global_pos, slurl);
}

bool LLLandmarkActions::getLandmarkGlobalPos(const LLUUID& landmarkInventoryItemID, LLVector3d& posGlobal)
{
	LLViewerInventoryItem* item = gInventory.getItem(landmarkInventoryItemID);
	if (NULL == item)
		return false;

	const LLUUID& asset_id = item->getAssetUUID();
	LLLandmark* landmark = gLandmarkList.getAsset(asset_id, NULL);

	if (NULL == landmark)
		return false;

	return landmark->getGlobalPos(posGlobal);
}

void LLLandmarkActions::collectParcelLandmark(LLInventoryModel::item_array_t& items){
	LLInventoryModel::cat_array_t cats;
	LLIsAgentParcelLandmark is_current_parcel_landmark;
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		is_current_parcel_landmark);
}
