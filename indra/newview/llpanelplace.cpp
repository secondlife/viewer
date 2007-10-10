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
#include "llvieweruictrlfactory.h"
//#include "llviewermenu.h"	// create_landmark()
#include "llweb.h"

//static
std::list<LLPanelPlace*> LLPanelPlace::sAllPanels;

LLPanelPlace::LLPanelPlace()
:	LLPanel("Places Panel"),
	mParcelID(),
	mPosGlobal(),
	mAuctionID(0)
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
	mSnapshotCtrl = LLViewerUICtrlFactory::getTexturePickerByName(this, "snapshot_ctrl");
	mSnapshotCtrl->setEnabled(FALSE);

    mNameEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "name_editor");

    mDescEditor = LLUICtrlFactory::getTextEditorByName(this, "desc_editor");

	mInfoEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "info_editor");

    mLocationEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "location_editor");

	mTeleportBtn = LLViewerUICtrlFactory::getButtonByName(this, "teleport_btn");
	mTeleportBtn->setClickedCallback(onClickTeleport);
	mTeleportBtn->setCallbackUserData(this);

	mMapBtn = LLViewerUICtrlFactory::getButtonByName(this, "map_btn");
	mMapBtn->setClickedCallback(onClickMap);
	mMapBtn->setCallbackUserData(this);

	//mLandmarkBtn = LLViewerUICtrlFactory::getButtonByName(this, "landmark_btn");
	//mLandmarkBtn->setClickedCallback(onClickLandmark);
	//mLandmarkBtn->setCallbackUserData(this);

	mAuctionBtn = LLViewerUICtrlFactory::getButtonByName(this, "auction_btn");
	mAuctionBtn->setClickedCallback(onClickAuction);
	mAuctionBtn->setCallbackUserData(this);

	// Default to no auction button.  We'll show it if we get an auction id
	mAuctionBtn->setVisible(FALSE);

	return TRUE;
}



void LLPanelPlace::setParcelID(const LLUUID& parcel_id)
{
	mParcelID = parcel_id;
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


//static 
void LLPanelPlace::processParcelInfoReply(LLMessageSystem *msg, void **)
{
	LLUUID	agent_id;
	LLUUID	parcel_id;
	LLUUID	owner_id;
	char	name[MAX_STRING];		/*Flawfinder: ignore*/
	char	desc[MAX_STRING];		/*Flawfinder: ignore*/
	S32		actual_area;
	S32		billable_area;
	U8		flags;
	F32		global_x;
	F32		global_y;
	F32		global_z;
	char	sim_name[MAX_STRING];		/*Flawfinder: ignore*/
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
		msg->getString	("Data", "Name", MAX_STRING, name);
		msg->getString	("Data", "Desc", MAX_STRING, desc);
		msg->getS32		("Data", "ActualArea", actual_area);
		msg->getS32		("Data", "BillableArea", billable_area);
		msg->getU8		("Data", "Flags", flags);
		msg->getF32		("Data", "GlobalX", global_x);
		msg->getF32		("Data", "GlobalY", global_y);
		msg->getF32		("Data", "GlobalZ", global_z);
		msg->getString	("Data", "SimName", MAX_STRING, sim_name);
		msg->getUUID	("Data", "SnapshotID", snapshot_id);
		msg->getF32		("Data", "Dwell", dwell);
		msg->getS32		("Data", "SalePrice", sale_price);
		msg->getS32		("Data", "AuctionID", auction_id);

		self->mPosGlobal.setVec(global_x, global_y, global_z);

		self->mAuctionID = auction_id;

		self->mSnapshotCtrl->setImageAssetID(snapshot_id);

		self->mNameEditor->setText(LLString(name));

		self->mDescEditor->setText(LLString(desc));

		LLString info_text;
		LLUIString traffic = self->childGetText("traffic_text");
		traffic.setArg("[TRAFFIC]", llformat("%.0f", dwell));
		info_text = traffic;
		LLUIString area = self->childGetText("area_text");
		traffic.setArg("[AREA]", llformat("%d", actual_area));
		info_text += area;
		if (flags & DFQ_FOR_SALE)
		{
			LLUIString forsale = self->childGetText("forsale_text");
			traffic.setArg("[PRICE]", llformat("%d", sale_price));
			info_text += forsale;
		}
		if (auction_id != 0)
		{
			LLUIString auction = self->childGetText("auction_text");
			auction.setArg("[ID]", llformat("%010d", auction_id));
			info_text += auction;
		}
		self->mInfoEditor->setText(info_text);

		S32 region_x = llround(global_x) % REGION_WIDTH_UNITS;
		S32 region_y = llround(global_y) % REGION_WIDTH_UNITS;
		S32 region_z = llround(global_z);

		// HACK: Flag 0x1 == mature region, otherwise assume PG
		const char* rating = LLViewerRegion::accessToString(SIM_ACCESS_PG);
		if (flags & 0x1)
		{
			rating = LLViewerRegion::accessToString(SIM_ACCESS_MATURE);
		}

		LLString location = llformat("%s %d, %d, %d (%s)", 
									 sim_name, region_x, region_y, region_z, rating);
		self->mLocationEditor->setText(location);

		BOOL show_auction = (auction_id > 0);
		self->mAuctionBtn->setVisible(show_auction);
	}
}


// static
void LLPanelPlace::onClickTeleport(void* data)
{
	LLPanelPlace* self = (LLPanelPlace*)data;

	if (!self->mPosGlobal.isExactlyZero())
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
		char url[256];		/*Flawfinder: ignore*/
		snprintf(url, sizeof(url), "%s%010d", AUCTION_URL, self->mAuctionID);			/* Flawfinder: ignore */

		llinfos << "Loading auction page " << url << llendl;

		LLWeb::loadURL(url);
	}
}
