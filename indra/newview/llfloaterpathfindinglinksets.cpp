/** 
 * @file llfloaterpathfindinglinksets.cpp
 * @author William Todd Stinson
 * @brief "Pathfinding linksets" floater, allowing manipulation of the Havok AI pathfinding settings.
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

#include "llviewerprecompiledheaders.h"
#include "llfloaterpathfindinglinksets.h"
#include "message.h"
#include "lluuid.h"
#include "llsd.h"
#include "llagent.h"
#include "lluictrl.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llscrolllistctrl.h"
#include "llavatarnamecache.h"
#include "llresmgr.h"

//---------------------------------------------------------------------------
// LLFloaterPathfindingLinksets
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingLinksets::postBuild()
{
	childSetAction("refresh_linksets_list", boost::bind(&LLFloaterPathfindingLinksets::onRefreshLinksetsClicked, this));
	childSetAction("select_all_linksets", boost::bind(&LLFloaterPathfindingLinksets::onSelectAllLinksetsClicked, this));
	childSetAction("select_none_linksets", boost::bind(&LLFloaterPathfindingLinksets::onSelectNoneLinksetsClicked, this));

	mLinksetsScrollList = findChild<LLScrollListCtrl>("pathfinding_linksets");
	llassert(mLinksetsScrollList != NULL);
	mLinksetsScrollList->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onLinksetsSelectionChange, this));
	mLinksetsScrollList->setCommitOnSelectionChange(true);

	mLinksetsStatus = findChild<LLUICtrl>("linksets_status");
	llassert(mLinksetsStatus != NULL);

	updateLinksetsStatus();

	return LLFloater::postBuild();
}

void LLFloaterPathfindingLinksets::onOpen(const LLSD& pKey)
{
	sendLandStatRequest();
}

void LLFloaterPathfindingLinksets::handleLandStatReply(LLMessageSystem* pMsg, void** pData)
{
	LLFloaterPathfindingLinksets *instance = LLFloaterReg::findTypedInstance<LLFloaterPathfindingLinksets>("pathfinding_linksets");
	if (instance != NULL)
	{
		instance->handleLandStatReply(pMsg);
	}
}

void LLFloaterPathfindingLinksets::openLinksetsEditor()
{
	LLFloaterReg::toggleInstanceOrBringToFront("pathfinding_linksets");
}

LLFloaterPathfindingLinksets::LLFloaterPathfindingLinksets(const LLSD& pSeed)
	: LLFloater(pSeed),
	mLinksetsScrollList(NULL),
	mLinksetsStatus(NULL)
{
}

LLFloaterPathfindingLinksets::~LLFloaterPathfindingLinksets()
{
}

void LLFloaterPathfindingLinksets::sendLandStatRequest()
{
	clearLinksetsList();
	updateLinksetsStatusForFetch();

	U32 mode = 0;
	U32 flags = 0;
	std::string filter = "";

	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_LandStatRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_RequestData);
	msg->addU32Fast(_PREHASH_ReportType, mode);
	msg->addU32Fast(_PREHASH_RequestFlags, flags);
	msg->addStringFast(_PREHASH_Filter, filter);
	msg->addS32Fast(_PREHASH_ParcelLocalID, 0);

	msg->sendReliable(gAgent.getRegionHost());
}

void LLFloaterPathfindingLinksets::handleLandStatReply(LLMessageSystem *pMsg)
{
	U32 request_flags;
	U32 total_count;
	U32 current_mode;

	pMsg->getU32Fast(_PREHASH_RequestData, _PREHASH_RequestFlags, request_flags);
	pMsg->getU32Fast(_PREHASH_RequestData, _PREHASH_TotalObjectCount, total_count);
	pMsg->getU32Fast(_PREHASH_RequestData, _PREHASH_ReportType, current_mode);

	clearLinksetsList();

	const LLVector3& avatarPosition = gAgent.getPositionAgent();

	S32 block_count = pMsg->getNumberOfBlocks("ReportData");
	for (S32 block = 0; block < block_count; ++block)
	{
		U32 task_local_id;
		U32 time_stamp = 0;
		LLUUID task_id;
		F32 location_x, location_y, location_z;
		F32 score;
		std::string name_buf;
		std::string owner_buf;
		F32 mono_score = 0.f;
		bool have_extended_data = false;
		S32 public_urls = 0;

		pMsg->getU32Fast(_PREHASH_ReportData, _PREHASH_TaskLocalID, task_local_id, block);
		pMsg->getUUIDFast(_PREHASH_ReportData, _PREHASH_TaskID, task_id, block);
		pMsg->getF32Fast(_PREHASH_ReportData, _PREHASH_LocationX, location_x, block);
		pMsg->getF32Fast(_PREHASH_ReportData, _PREHASH_LocationY, location_y, block);
		pMsg->getF32Fast(_PREHASH_ReportData, _PREHASH_LocationZ, location_z, block);
		pMsg->getF32Fast(_PREHASH_ReportData, _PREHASH_Score, score, block);
		pMsg->getStringFast(_PREHASH_ReportData, _PREHASH_TaskName, name_buf, block);
		pMsg->getStringFast(_PREHASH_ReportData, _PREHASH_OwnerName, owner_buf, block);
		if(pMsg->has("DataExtended"))
		{
			have_extended_data = true;
			pMsg->getU32("DataExtended", "TimeStamp", time_stamp, block);
			pMsg->getF32("DataExtended", "MonoScore", mono_score, block);
			pMsg->getS32("DataExtended", "PublicURLs", public_urls, block);
		}

		LLSD element;

		element["id"] = task_id;

		LLSD columns;

		columns[0]["column"] = "name";
		columns[0]["value"] = name_buf;
		columns[0]["font"] = "SANSSERIF";

		// Owner names can have trailing spaces sent from server
		LLStringUtil::trim(owner_buf);

		if (LLAvatarNameCache::useDisplayNames())
		{
			// ...convert hard-coded name from server to a username
			// *TODO: Send owner_id from server and look up display name
			owner_buf = LLCacheName::buildUsername(owner_buf);
		}
		else
		{
			// ...just strip out legacy "Resident" name
			owner_buf = LLCacheName::cleanFullName(owner_buf);
		}
		columns[1]["column"] = "description";
		columns[1]["value"] = owner_buf;
		columns[1]["font"] = "SANSSERIF";

		columns[2]["column"] = "land_impact";
		columns[2]["value"] = llformat("%0.1f", 55.679);
		columns[2]["font"] = "SANSSERIF";

		LLVector3 location(location_x, location_y, location_z);
		F32 distance = dist_vec(avatarPosition, location);

		columns[3]["column"] = "dist_from_you";
		columns[3]["value"] = llformat("%1.0f m", distance);
		columns[3]["font"] = "SANSSERIF";

		element["columns"] = columns;
		mLinksetsScrollList->addElement(element);
	}

	updateLinksetsStatus();
}

void LLFloaterPathfindingLinksets::onLinksetsSelectionChange()
{
	updateLinksetsStatus();
}

void LLFloaterPathfindingLinksets::onRefreshLinksetsClicked()
{
	sendLandStatRequest();
}

void LLFloaterPathfindingLinksets::onSelectAllLinksetsClicked()
{
	selectAllLinksets();
}

void LLFloaterPathfindingLinksets::onSelectNoneLinksetsClicked()
{
	selectNoneLinksets();
}

void LLFloaterPathfindingLinksets::clearLinksetsList()
{
	mLinksetsScrollList->deleteAllItems();
	updateLinksetsStatus();
}

void LLFloaterPathfindingLinksets::selectAllLinksets()
{
	mLinksetsScrollList->selectAll();
}

void LLFloaterPathfindingLinksets::selectNoneLinksets()
{
	mLinksetsScrollList->deselectAllItems();
}

void LLFloaterPathfindingLinksets::updateLinksetsStatus()
{
	std::string statusText("");

	if (mLinksetsScrollList->isEmpty())
	{
		statusText = getString("linksets_none_found");
	}
	else
	{
		S32 numItems = mLinksetsScrollList->getItemCount();
		S32 numSelectedItems = mLinksetsScrollList->getNumSelected();

		LLLocale locale(LLStringUtil::getLocale());
		std::string numItemsString;
		LLResMgr::getInstance()->getIntegerString(numItemsString, numItems);

		std::string numSelectedItemsString;
		LLResMgr::getInstance()->getIntegerString(numSelectedItemsString, numSelectedItems);

		LLStringUtil::format_map_t string_args;
		string_args["[NUM_SELECTED]"] = numSelectedItemsString;
		string_args["[NUM_TOTAL]"] = numItemsString;
		statusText = getString("linksets_available", string_args);
	}

	mLinksetsStatus->setValue(statusText);
}

void LLFloaterPathfindingLinksets::updateLinksetsStatusForFetch()
{
	mLinksetsStatus->setValue(getString("linksets_fetching"));
}
