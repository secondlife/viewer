/** 
 * @file llpanelplace.cpp
 * @brief Display of a place in the Find directory.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
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
LLLinkedList<LLPanelPlace> LLPanelPlace::sAllPanels;

LLPanelPlace::LLPanelPlace()
:	LLPanel("Places Panel"),
	mParcelID(),
	mPosGlobal(),
	mAuctionID(0)
{
	sAllPanels.addData(this);
}


LLPanelPlace::~LLPanelPlace()
{
	sAllPanels.removeData(this);
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
	char	buffer[256];		/*Flawfinder: ignore*/

	msg->getUUID("AgentData", "AgentID", agent_id );
	msg->getUUID("Data", "ParcelID", parcel_id);

	// look up all panels which have this avatar
	LLPanelPlace *self = NULL;

	for (self = sAllPanels.getFirstData(); self; self = sAllPanels.getNextData())
	{
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

		self->mNameEditor->setText(name);

		self->mDescEditor->setText(desc);

		LLString info;
		snprintf(buffer, sizeof(buffer), "Traffic: %.0f, Area: %d sq. m.", dwell, actual_area);			/* Flawfinder: ignore */
		info.append(buffer);
		if (flags & DFQ_FOR_SALE)
		{
			snprintf(buffer, sizeof(buffer), ", For Sale for L$%d", sale_price);			/* Flawfinder: ignore */
			info.append(buffer);
		}
		if (auction_id != 0)
		{
			snprintf(buffer, sizeof(buffer), ", Auction ID %010d", auction_id);			/* Flawfinder: ignore */
			info.append(buffer);
		}
		self->mInfoEditor->setText(info);

		S32 region_x = llround(global_x) % REGION_WIDTH_UNITS;
		S32 region_y = llround(global_y) % REGION_WIDTH_UNITS;
		S32 region_z = llround(global_z);
		
		// HACK: Flag 0x1 == mature region, otherwise assume PG
		const char* rating = LLViewerRegion::accessToString(SIM_ACCESS_PG);
		if (flags & 0x1)
		{
			rating = LLViewerRegion::accessToString(SIM_ACCESS_MATURE);
		}

		snprintf(buffer, sizeof(buffer), "%s %d, %d, %d (%s)", 			/* Flawfinder: ignore */
			sim_name, region_x, region_y, region_z, rating);
		self->mLocationEditor->setText(buffer);

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
