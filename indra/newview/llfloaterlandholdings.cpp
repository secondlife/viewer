/** 
 * @file llfloaterlandholdings.cpp
 * @brief "My Land" floater showing all your land parcels.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llfloaterlandholdings.h"

#include "indra_constants.h"
#include "llfontgl.h"
#include "llqueryflags.h"
#include "llparcel.h"
#include "message.h"

#include "llagent.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llproductinforequest.h"
#include "llscrolllistctrl.h"
#include "llstatusbar.h"
#include "lltextbox.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "lltrans.h"
#include "lluiconstants.h"
#include "llviewermessage.h"
#include "lluictrlfactory.h"

#include "llgroupactions.h"

// protected
LLFloaterLandHoldings::LLFloaterLandHoldings(const LLSD& key)
:	LLFloater(key),
	mActualArea(0),
	mBillableArea(0),
	mFirstPacketReceived(FALSE),
	mSortColumn(""),
	mSortAscending(TRUE)
{
}

BOOL LLFloaterLandHoldings::postBuild()
{
	childSetAction("Teleport", onClickTeleport, this);
	childSetAction("Show on Map", onClickMap, this);

	// Grant list
	LLScrollListCtrl* grant_list = getChild<LLScrollListCtrl>("grant list");
	grant_list->sortByColumnIndex(0, TRUE);
	grant_list->setDoubleClickCallback(onGrantList, this);

	S32 count = gAgent.mGroups.count();
	for(S32 i = 0; i < count; ++i)
	{
		LLUUID id(gAgent.mGroups.get(i).mID);

		LLSD element;
		element["id"] = id;
		element["columns"][0]["column"] = "group";
		element["columns"][0]["value"] = gAgent.mGroups.get(i).mName;
		element["columns"][0]["font"] = "SANSSERIF";

		LLUIString areastr = getString("area_string");
		areastr.setArg("[AREA]", llformat("%d", gAgent.mGroups.get(i).mContribution));
		element["columns"][1]["column"] = "area";
		element["columns"][1]["value"] = areastr;
		element["columns"][1]["font"] = "SANSSERIF";

		grant_list->addElement(element);
	}
	
	center();
	
	return TRUE;
}


// protected
LLFloaterLandHoldings::~LLFloaterLandHoldings()
{
}

void LLFloaterLandHoldings::onOpen(const LLSD& key)
{
	// query_id null is known to be us
	const LLUUID& query_id = LLUUID::null;

	// look only for parcels we own
	U32 query_flags = DFQ_AGENT_OWNED;

	send_places_query(query_id,
					  LLUUID::null,
					  "",
					  query_flags,
					  LLParcel::C_ANY,
					  "");
}

void LLFloaterLandHoldings::draw()
{
	refresh();

	LLFloater::draw();
}


// public
void LLFloaterLandHoldings::refresh()
{
	LLCtrlSelectionInterface *list = childGetSelectionInterface("parcel list");
	BOOL enable_btns = FALSE;
	if (list && list->getFirstSelectedIndex()> -1)
	{
		enable_btns = TRUE;
	}

	getChildView("Teleport")->setEnabled(enable_btns);
	getChildView("Show on Map")->setEnabled(enable_btns);

	refreshAggregates();
}


// static
void LLFloaterLandHoldings::processPlacesReply(LLMessageSystem* msg, void**)
{
	LLFloaterLandHoldings* self = LLFloaterReg::findTypedInstance<LLFloaterLandHoldings>("land_holdings");

	// Is this packet from an old, closed window?
	if (!self)
	{
		return;
	}

	LLCtrlListInterface *list = self->childGetListInterface("parcel list");
	if (!list) return;

	// If this is the first packet, clear out the "loading..." indicator
	if (!self->mFirstPacketReceived)
	{
		self->mFirstPacketReceived = TRUE;
		list->operateOnAll(LLCtrlSelectionInterface::OP_DELETE);
	}

	LLUUID	owner_id;
	std::string	name;
	std::string	desc;
	S32		actual_area;
	S32		billable_area;
	U8		flags;
	F32		global_x;
	F32		global_y;
	std::string	sim_name;
	std::string land_sku;
	std::string land_type;
	
	S32 i;
	S32 count = msg->getNumberOfBlocks("QueryData");
	for (i = 0; i < count; i++)
	{
		msg->getUUID("QueryData", "OwnerID", owner_id, i);
		msg->getString("QueryData", "Name", name, i);
		msg->getString("QueryData", "Desc", desc, i);
		msg->getS32("QueryData", "ActualArea", actual_area, i);
		msg->getS32("QueryData", "BillableArea", billable_area, i);
		msg->getU8("QueryData", "Flags", flags, i);
		msg->getF32("QueryData", "GlobalX", global_x, i);
		msg->getF32("QueryData", "GlobalY", global_y, i);
		msg->getString("QueryData", "SimName", sim_name, i);

		if ( msg->getSizeFast(_PREHASH_QueryData, i, _PREHASH_ProductSKU) > 0 )
		{
			msg->getStringFast(	_PREHASH_QueryData, _PREHASH_ProductSKU, land_sku, i);
			llinfos << "Land sku: " << land_sku << llendl;
			land_type = LLProductInfoRequestManager::instance().getDescriptionForSku(land_sku);
		}
		else
		{
			land_sku.clear();
			land_type = LLTrans::getString("land_type_unknown");
		}
		
		if(owner_id.notNull())
		{
			self->mActualArea += actual_area;
			self->mBillableArea += billable_area;

			S32 region_x = llround(global_x) % REGION_WIDTH_UNITS;
			S32 region_y = llround(global_y) % REGION_WIDTH_UNITS;

			std::string location;
			location = llformat("%s (%d, %d)", sim_name.c_str(), region_x, region_y);

			std::string area;
			if(billable_area == actual_area)
			{
				area = llformat("%d", billable_area);
			}
			else
			{
				area = llformat("%d / %d", billable_area, actual_area);
			}
			
			std::string hidden;
			hidden = llformat("%f %f", global_x, global_y);

			LLSD element;
			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = name;
			element["columns"][0]["font"] = "SANSSERIF";
			
			element["columns"][1]["column"] = "location";
			element["columns"][1]["value"] = location;
			element["columns"][1]["font"] = "SANSSERIF";
			
			element["columns"][2]["column"] = "area";
			element["columns"][2]["value"] = area;
			element["columns"][2]["font"] = "SANSSERIF";
			
			element["columns"][3]["column"] = "type";
			element["columns"][3]["value"] = land_type;
			element["columns"][3]["font"] = "SANSSERIF";
			
			// hidden is always last column
			element["columns"][4]["column"] = "hidden";
			element["columns"][4]["value"] = hidden;

			list->addElement(element);
		}
	}
	
	self->refreshAggregates();
}

void LLFloaterLandHoldings::buttonCore(S32 which)
{
	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("parcel list");
	if (!list) return;

	S32 index = list->getFirstSelectedIndex();
	if (index < 0) return;

	// hidden is always last column
	std::string location = list->getSelectedItemLabel(list->getNumColumns()-1);

	F32 global_x = 0.f;
	F32 global_y = 0.f;
	sscanf(location.c_str(), "%f %f", &global_x, &global_y);

	// Hack: Use the agent's z-height
	F64 global_z = gAgent.getPositionGlobal().mdV[VZ];

	LLVector3d pos_global(global_x, global_y, global_z);
	LLFloaterWorldMap* floater_world_map = LLFloaterWorldMap::getInstance();

	switch(which)
	{
	case 0:
		gAgent.teleportViaLocation(pos_global);
		if(floater_world_map) floater_world_map->trackLocation(pos_global);
		break;
	case 1:
		if(floater_world_map) floater_world_map->trackLocation(pos_global);
		LLFloaterReg::showInstance("world_map", "center");
		break;
	default:
		break;
	}
}

// static
void LLFloaterLandHoldings::onClickTeleport(void* data)
{
	LLFloaterLandHoldings* self = (LLFloaterLandHoldings*)data;
	self->buttonCore(0);
	self->closeFloater();
}


// static
void LLFloaterLandHoldings::onClickMap(void* data)
{
	LLFloaterLandHoldings* self = (LLFloaterLandHoldings*)data;
	self->buttonCore(1);
}

// static
void LLFloaterLandHoldings::onGrantList(void* data)
{
	LLFloaterLandHoldings* self = (LLFloaterLandHoldings*)data;
	LLCtrlSelectionInterface *list = self->childGetSelectionInterface("grant list");
	if (!list) return;
	LLUUID group_id = list->getCurrentID();
	if (group_id.notNull())
	{
		LLGroupActions::show(group_id);
	}
}

void LLFloaterLandHoldings::refreshAggregates()
{
	S32 allowed_area = gStatusBar->getSquareMetersCredit();
	S32 current_area = gStatusBar->getSquareMetersCommitted();
	S32 available_area = gStatusBar->getSquareMetersLeft();

	getChild<LLUICtrl>("allowed_text")->setTextArg("[AREA]", llformat("%d",allowed_area));
	getChild<LLUICtrl>("current_text")->setTextArg("[AREA]", llformat("%d",current_area));
	getChild<LLUICtrl>("available_text")->setTextArg("[AREA]", llformat("%d",available_area));
}
