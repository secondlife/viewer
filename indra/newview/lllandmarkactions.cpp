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

#include "llagent.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "lllandmark.h"
#include "lllandmarklist.h"
#include "llnotifications.h"
#include "llparcel.h"
#include "llviewerinventory.h"
#include "llviewerparcelmgr.h"
#include "roles_constants.h"

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

bool LLLandmarkActions::landmarkAlreadyExists()
{
	// Determine whether there are landmarks pointing to the current parcel.
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLIsAgentParcelLandmark is_current_parcel_landmark;
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		is_current_parcel_landmark);

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
