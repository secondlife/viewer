/** 
 * @file llpanelplace.cpp
 * @brief Display of a place in the Find directory.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

#include "llpanelplace.h"

#include "llviewercontrol.h"
#include "llqueryflags.h"
#include "message.h"
#include "llui.h"
#include "llsecondlifeurls.h"
#include "llremoteparcelrequest.h"
#include "llfloater.h"

#include "llagent.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llfloaterworldmap.h"
#include "lllineeditor.h"
#include "lluiconstants.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "lltexturectrl.h"
#include "llworldmap.h"
#include "llviewerregion.h"
#include "lluictrlfactory.h"
//#include "llviewermenu.h"	// create_landmark()
#include "llweb.h"
#include "llsdutil.h"

//static
std::list<LLPanelPlace*> LLPanelPlace::sAllPanels;

LLPanelPlace::LLPanelPlace()
:	LLPanel(std::string("Places Panel")),
	mParcelID(),
	mRequestedID(),
	mRegionID(),
	mPosGlobal(),
	mPosRegion(),
	mAuctionID(0),
	mLandmarkAssetID()
{
	sAllPanels.push_back(this);
}


LLPanelPlace::~LLPanelPlace()
{
	sAllPanels.remove(this);
}


BOOL LLPanelPlace::postBuild()
{
	// Since this is only used in the directory browser, always
	// disable the snapshot control. Otherwise clicking on it will
	// open a texture picker.
	mSnapshotCtrl = getChild<LLTextureCtrl>("snapshot_ctrl");
	mSnapshotCtrl->setEnabled(FALSE);

    mNameEditor = getChild<LLTextBox>("name_editor");
	// Text boxes appear to have a " " in them by default.  This breaks the
	// emptiness test for filling in data from the network.  Slam to empty.
	mNameEditor->setText( LLStringUtil::null );

    mDescEditor = getChild<LLTextEditor>("desc_editor");

	mInfoEditor = getChild<LLTextBox>("info_editor");

    mLocationEditor = getChild<LLTextBox>("location_editor");

	mTeleportBtn = getChild<LLButton>( "teleport_btn");
	mTeleportBtn->setClickedCallback(onClickTeleport);
	mTeleportBtn->setCallbackUserData(this);

	mMapBtn = getChild<LLButton>( "map_btn");
	mMapBtn->setClickedCallback(onClickMap);
	mMapBtn->setCallbackUserData(this);

	//mLandmarkBtn = getChild<LLButton>( "landmark_btn");
	//mLandmarkBtn->setClickedCallback(onClickLandmark);
	//mLandmarkBtn->setCallbackUserData(this);

	mAuctionBtn = getChild<LLButton>( "auction_btn");
	mAuctionBtn->setClickedCallback(onClickAuction);
	mAuctionBtn->setCallbackUserData(this);

	// Default to no auction button.  We'll show it if we get an auction id
	mAuctionBtn->setVisible(FALSE);

	// Temporary text to explain why the description panel is blank.
	// mDescEditor->setText("Parcel information not available without server update");

	return TRUE;
}

void LLPanelPlace::displayItemInfo(const LLInventoryItem* pItem)
{
	mNameEditor->setText(pItem->getName());
	mDescEditor->setText(pItem->getDescription());
}

// Use this for search directory clicks, because we are totally
// recycling the panel and don't need to use what's there.
//
// For SLURL clicks, don't call this, because we need to cache
// the location info from the user.
void LLPanelPlace::resetLocation()
{
	mParcelID.setNull();
	mRequestedID.setNull();
	mRegionID.setNull();
	mLandmarkAssetID.setNull();
	mPosGlobal.clearVec();
	mPosRegion.clearVec();
	mAuctionID = 0;
	mNameEditor->setText( LLStringUtil::null );
	mDescEditor->setText( LLStringUtil::null );
	mInfoEditor->setText( LLStringUtil::null );
	mLocationEditor->setText( LLStringUtil::null );
}

void LLPanelPlace::setParcelID(const LLUUID& parcel_id)
{
	mParcelID = parcel_id;
	sendParcelInfoRequest();
}

void LLPanelPlace::setSnapshot(const LLUUID& snapshot_id)
{
	mSnapshotCtrl->setImageAssetID(snapshot_id);
}


void LLPanelPlace::setLocationString(const std::string& location)
{
	mLocationEditor->setText(location);
}

void LLPanelPlace::sendParcelInfoRequest()
{
	LLMessageSystem *msg = gMessageSystem;

	if (mParcelID != mRequestedID)
	{
		msg->newMessage("ParcelInfoRequest");
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("ParcelID", mParcelID);
		gAgent.sendReliableMessage();
		mRequestedID = mParcelID;
	}
}

void LLPanelPlace::setErrorStatus(U32 status, const std::string& reason)
{
	// We only really handle 404 and 499 errors
	std::string error_text;
	if(status == 404)
	{	
		error_text = getString("server_error_text");
	}
	else if(status == 499)
	{
		error_text = getString("server_forbidden_text");
	}
	mDescEditor->setText(error_text);
}

//static 
void LLPanelPlace::processParcelInfoReply(LLMessageSystem *msg, void **)
{
	LLUUID	agent_id;
	LLUUID	parcel_id;
	LLUUID	owner_id;
	std::string	name;
	std::string	desc;
	S32		actual_area;
	S32		billable_area;
	U8		flags;
	F32		global_x;
	F32		global_y;
	F32		global_z;
	std::string	sim_name;
	LLUUID	snapshot_id;
	F32		dwell;
	S32		sale_price;
	S32		auction_id;

	msg->getUUID("AgentData", "AgentID", agent_id );
	msg->getUUID("Data", "ParcelID", parcel_id);

	// look up all panels which have this avatar
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelPlace* self = *iter;
		if (self->mParcelID != parcel_id)
		{
			continue;
		}

		msg->getUUID	("Data", "OwnerID", owner_id);
		msg->getString	("Data", "Name", name);
		msg->getString	("Data", "Desc", desc);
		msg->getS32		("Data", "ActualArea", actual_area);
		msg->getS32		("Data", "BillableArea", billable_area);
		msg->getU8		("Data", "Flags", flags);
		msg->getF32		("Data", "GlobalX", global_x);
		msg->getF32		("Data", "GlobalY", global_y);
		msg->getF32		("Data", "GlobalZ", global_z);
		msg->getString	("Data", "SimName", sim_name);
		msg->getUUID	("Data", "SnapshotID", snapshot_id);
		msg->getF32		("Data", "Dwell", dwell);
		msg->getS32		("Data", "SalePrice", sale_price);
		msg->getS32		("Data", "AuctionID", auction_id);


		self->mAuctionID = auction_id;

		if(snapshot_id.notNull())
		{
			self->mSnapshotCtrl->setImageAssetID(snapshot_id);
		}

		// Only assign the name and description if they are not empty and there is not a 
		// value present (passed in from a landmark, e.g.)

		if( !name.empty()
		   && self->mNameEditor && self->mNameEditor->getText().empty())
		{
			self->mNameEditor->setText(name);
		}

		if( !desc.empty()
			&& self->mDescEditor && self->mDescEditor->getText().empty())
		{
			self->mDescEditor->setText(desc);
		}

		std::string info_text;
		LLUIString traffic = self->getString("traffic_text");
		traffic.setArg("[TRAFFIC]", llformat("%d ", (int)dwell));
		info_text = traffic;
		LLUIString area = self->getString("area_text");
		area.setArg("[AREA]", llformat("%d", actual_area));
		info_text += area;
		if (flags & DFQ_FOR_SALE)
		{
			LLUIString forsale = self->getString("forsale_text");
			forsale.setArg("[PRICE]", llformat("%d", sale_price));
			info_text += forsale;
		}
		if (auction_id != 0)
		{
			LLUIString auction = self->getString("auction_text");
			auction.setArg("[ID]", llformat("%010d ", auction_id));
			info_text += auction;
		}
		if (self->mInfoEditor)
		{
			self->mInfoEditor->setText(info_text);
		}

		// HACK: Flag 0x1 == mature region, otherwise assume PG
		std::string rating = LLViewerRegion::accessToString(SIM_ACCESS_PG);
		if (flags & 0x1)
		{
			rating = LLViewerRegion::accessToString(SIM_ACCESS_MATURE);
		}

		// Just use given region position for display
		S32 region_x = llround(self->mPosRegion.mV[0]);
		S32 region_y = llround(self->mPosRegion.mV[1]);
		S32 region_z = llround(self->mPosRegion.mV[2]);

		// If the region position is zero, grab position from the global
		if(self->mPosRegion.isExactlyZero())
		{
			region_x = llround(global_x) % REGION_WIDTH_UNITS;
			region_y = llround(global_y) % REGION_WIDTH_UNITS;
			region_z = llround(global_z);
		}

		if(self->mPosGlobal.isExactlyZero())
		{
			self->mPosGlobal.setVec(global_x, global_y, global_z);
		}

		std::string location = llformat("%s %d, %d, %d (%s)",
										sim_name.c_str(), region_x, region_y, region_z, rating.c_str());
		if (self->mLocationEditor)
		{
			self->mLocationEditor->setText(location);
		}

		BOOL show_auction = (auction_id > 0);
		self->mAuctionBtn->setVisible(show_auction);
	}
}


void LLPanelPlace::displayParcelInfo(const LLVector3& pos_region,
									 const LLUUID& landmark_asset_id,
									 const LLUUID& region_id,
									 const LLVector3d& pos_global)
{
	LLSD body;
	mPosRegion = pos_region;
	mPosGlobal = pos_global;
	mLandmarkAssetID = landmark_asset_id;
	std::string url = gAgent.getRegion()->getCapability("RemoteParcelRequest");
	if (!url.empty())
	{
		body["location"] = ll_sd_from_vector3(pos_region);
		if (!region_id.isNull())
		{
			body["region_id"] = region_id;
		}
		if (!pos_global.isExactlyZero())
		{
			U64 region_handle = to_region_handle(pos_global);
			body["region_handle"] = ll_sd_from_U64(region_handle);
		}
		LLHTTPClient::post(url, body, new LLRemoteParcelRequestResponder(this->getHandle()));
	}
	else
	{
		mDescEditor->setText(getString("server_update_text"));
	}
	mSnapshotCtrl->setImageAssetID(LLUUID::null);
	mSnapshotCtrl->setFallbackImageName("default_land_picture.j2c");
}


// static
void LLPanelPlace::onClickTeleport(void* data)
{
	LLPanelPlace* self = (LLPanelPlace*)data;

	LLView* parent_viewp = self->getParent();
	LLFloater* parent_floaterp = dynamic_cast<LLFloater*>(parent_viewp);
	if (parent_floaterp)
	{
		parent_floaterp->close();
	}
	// LLFloater* parent_floaterp = (LLFloater*)self->getParent();
	parent_viewp->setVisible(false);
	if(self->mLandmarkAssetID.notNull())
	{
		gAgent.teleportViaLandmark(self->mLandmarkAssetID);
		gFloaterWorldMap->trackLandmark(self->mLandmarkAssetID);

	}
	else if (!self->mPosGlobal.isExactlyZero())
	{
		gAgent.teleportViaLocation(self->mPosGlobal);
		gFloaterWorldMap->trackLocation(self->mPosGlobal);
	}
}

// static
void LLPanelPlace::onClickMap(void* data)
{
	LLPanelPlace* self = (LLPanelPlace*)data;
	if (!self->mPosGlobal.isExactlyZero())
	{
		gFloaterWorldMap->trackLocation(self->mPosGlobal);
		LLFloaterWorldMap::show(NULL, TRUE);
	}
}

// static
/*
void LLPanelPlace::onClickLandmark(void* data)
{
	LLPanelPlace* self = (LLPanelPlace*)data;

	create_landmark(self->mNameEditor->getText(), "", self->mPosGlobal);
}
*/


// static
void LLPanelPlace::onClickAuction(void* data)
{
	LLPanelPlace* self = (LLPanelPlace*)data;

	gViewerWindow->alertXml("GoToAuctionPage",
		callbackAuctionWebPage, 
		self);
}

// static
void LLPanelPlace::callbackAuctionWebPage(S32 option, void* data)
{
	LLPanelPlace* self = (LLPanelPlace*)data;

	if (0 == option)
	{
		std::string url;
		url = AUCTION_URL + llformat( "%010d", self->mAuctionID);

		llinfos << "Loading auction page " << url << llendl;

		LLWeb::loadURL(url);
	}
}
