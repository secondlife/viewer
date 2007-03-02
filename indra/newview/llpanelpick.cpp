/** 
 * @file llpanelpick.cpp
 * @brief LLPanelPick class implementation
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Display of a "Top Pick" used both for the global top picks in the 
// Find directory, and also for each individual user's picks in their
// profile.

#include "llviewerprecompiledheaders.h"

#include "llpanelpick.h"

#include "lldir.h"
#include "llparcel.h"
#include "message.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llviewercontrol.h"
#include "lllineeditor.h"
#include "lltabcontainervertical.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "lltexturectrl.h"
#include "lluiconstants.h"
#include "llvieweruictrlfactory.h"
#include "llviewerparcelmgr.h"
#include "llworldmap.h"
#include "llfloaterworldmap.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

//static
LLLinkedList<LLPanelPick> LLPanelPick::sAllPanels;

LLPanelPick::LLPanelPick(BOOL top_pick)
:	LLPanel("Top Picks Panel"),
	mTopPick(top_pick),
	mPickID(),
	mCreatorID(),
	mParcelID(),
	mDataRequested(FALSE),
	mDataReceived(FALSE),
    mPosGlobal(),
    mSnapshotCtrl(NULL),
    mNameEditor(NULL),
    mDescEditor(NULL),
    mLocationEditor(NULL),
    mTeleportBtn(NULL),
    mMapBtn(NULL),
    //mLandmarkBtn(NULL),
    mSortOrderText(NULL),
    mSortOrderEditor(NULL),
    mEnabledCheck(NULL),
    mSetBtn(NULL)
{
    sAllPanels.addData(this);

	std::string pick_def_file;
	if (top_pick)
	{
		gUICtrlFactory->buildPanel(this, "panel_top_pick.xml");
	}
	else
	{
		gUICtrlFactory->buildPanel(this, "panel_avatar_pick.xml");
	}	
}


LLPanelPick::~LLPanelPick()
{
    sAllPanels.removeData(this);
}


void LLPanelPick::reset()
{
	mPickID.setNull();
	mCreatorID.setNull();
	mParcelID.setNull();

	// Don't request data, this isn't valid
	mDataRequested = TRUE;
	mDataReceived = FALSE;

	mPosGlobal.clearVec();

	clearCtrls();
}


BOOL LLPanelPick::postBuild()
{
    mSnapshotCtrl = LLViewerUICtrlFactory::getTexturePickerByName(this, "snapshot_ctrl");
	mSnapshotCtrl->setCommitCallback(onCommitAny);
	mSnapshotCtrl->setCallbackUserData(this);

    mNameEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "given_name_editor");
	mNameEditor->setCommitOnFocusLost(TRUE);
	mNameEditor->setCommitCallback(onCommitAny);
	mNameEditor->setCallbackUserData(this);

    mDescEditor = LLUICtrlFactory::getTextEditorByName(this, "desc_editor");
	mDescEditor->setCommitOnFocusLost(TRUE);
	mDescEditor->setCommitCallback(onCommitAny);
	mDescEditor->setCallbackUserData(this);
	mDescEditor->setTabToNextField(TRUE);

    mLocationEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "location_editor");

    mSetBtn = LLViewerUICtrlFactory::getButtonByName(this, "set_location_btn");
    mSetBtn->setClickedCallback(onClickSet);
    mSetBtn->setCallbackUserData(this);

    mTeleportBtn = LLViewerUICtrlFactory::getButtonByName(this, "pick_teleport_btn");
    mTeleportBtn->setClickedCallback(onClickTeleport);
    mTeleportBtn->setCallbackUserData(this);

    mMapBtn = LLViewerUICtrlFactory::getButtonByName(this, "pick_map_btn");
    mMapBtn->setClickedCallback(onClickMap);
    mMapBtn->setCallbackUserData(this);

	mSortOrderText = LLViewerUICtrlFactory::getTextBoxByName(this, "sort_order_text");

	mSortOrderEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "sort_order_editor");
	mSortOrderEditor->setPrevalidate(LLLineEditor::prevalidateInt);
	mSortOrderEditor->setCommitOnFocusLost(TRUE);
	mSortOrderEditor->setCommitCallback(onCommitAny);
	mSortOrderEditor->setCallbackUserData(this);

	mEnabledCheck = LLViewerUICtrlFactory::getCheckBoxByName(this, "enabled_check");
	mEnabledCheck->setCommitCallback(onCommitAny);
	mEnabledCheck->setCallbackUserData(this);

    return TRUE;
}


// Fill in some reasonable defaults for a new pick.
void LLPanelPick::initNewPick()
{
	mPickID.generate();

	mCreatorID = gAgent.getID();

	mPosGlobal = gAgent.getPositionGlobal();

	// Try to fill in the current parcel
	LLParcel* parcel = gParcelMgr->getAgentParcel();
	if (parcel)
	{
		mNameEditor->setText(parcel->getName());
		mDescEditor->setText(parcel->getDesc());
		mSnapshotCtrl->setImageAssetID(parcel->getSnapshotID());
	}

	// Commit to the database, since we've got "new" values.
	sendPickInfoUpdate();
}


void LLPanelPick::setPickID(const LLUUID& id)
{
	mPickID = id;
}


// Schedules the panel to request data
// from the server next time it is drawn.
void LLPanelPick::markForServerRequest()
{
	mDataRequested = FALSE;
	mDataReceived = FALSE;
}


std::string LLPanelPick::getPickName()
{
	return mNameEditor->getText();
}


void LLPanelPick::sendPickInfoRequest()
{
    LLMessageSystem *msg = gMessageSystem;

    msg->newMessage("PickInfoRequest");
    msg->nextBlock("AgentData");
    msg->addUUID("AgentID", gAgent.getID() );
    msg->addUUID("SessionID", gAgent.getSessionID());
    msg->nextBlock("Data");
	msg->addUUID("PickID", mPickID);
	gAgent.sendReliableMessage();

	mDataRequested = TRUE;
}


void LLPanelPick::sendPickInfoUpdate()
{
	// If we don't have a pick id yet, we'll need to generate one,
	// otherwise we'll keep overwriting pick_id 00000 in the database.
	if (mPickID.isNull())
	{
		mPickID.generate();
	}

	LLMessageSystem* msg = gMessageSystem;

	msg->newMessage("PickInfoUpdate");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addUUID("PickID", mPickID);
	msg->addUUID("CreatorID", mCreatorID);
	msg->addBOOL("TopPick", mTopPick);
	// fills in on simulator if null
	msg->addUUID("ParcelID", mParcelID);
	msg->addString("Name", mNameEditor->getText());
	msg->addString("Desc", mDescEditor->getText());
	msg->addUUID("SnapshotID", mSnapshotCtrl->getImageAssetID());
	msg->addVector3d("PosGlobal", mPosGlobal);
	
	// Only top picks have a sort order
	S32 sort_order;
	if (mTopPick)
	{
		sort_order = atoi(mSortOrderEditor->getText().c_str());
	}
	else
	{
		sort_order = 0;
	}
	msg->addS32("SortOrder", sort_order);
	msg->addBOOL("Enabled", mEnabledCheck->get());
	gAgent.sendReliableMessage();
}


//static
void LLPanelPick::processPickInfoReply(LLMessageSystem *msg, void **)
{
    // Extract the agent id and verify the message is for this
    // client.
    LLUUID agent_id;
    msg->getUUID("AgentData", "AgentID", agent_id );
    if (agent_id != gAgent.getID())
    {
        llwarns << "Agent ID mismatch in processPickInfoReply"
            << llendl;
		return;
    }

    LLUUID pick_id;
    msg->getUUID("Data", "PickID", pick_id);

    LLUUID creator_id;
    msg->getUUID("Data", "CreatorID", creator_id);

	BOOL top_pick;
	msg->getBOOL("Data", "TopPick", top_pick);

    LLUUID parcel_id;
    msg->getUUID("Data", "ParcelID", parcel_id);

	char name[DB_PARCEL_NAME_SIZE];		/*Flawfinder: ignore*/
	msg->getString("Data", "Name", DB_PARCEL_NAME_SIZE, name);

	char desc[DB_PICK_DESC_SIZE];		/*Flawfinder: ignore*/
	msg->getString("Data", "Desc", DB_PICK_DESC_SIZE, desc);

	LLUUID snapshot_id;
	msg->getUUID("Data", "SnapshotID", snapshot_id);

    // "Location text" is actually the owner name, the original
    // name that owner gave the parcel, and the location.
	char buffer[256];		/*Flawfinder: ignore*/
    LLString location_text;

    msg->getString("Data", "User", 256, buffer);
    location_text.assign(buffer);
    location_text.append(", ");

    msg->getString("Data", "OriginalName", 256, buffer);
	if (buffer[0] != '\0')
	{
		location_text.append(buffer);
		location_text.append(", ");
	}

	char sim_name[256];		/*Flawfinder: ignore*/
	msg->getString("Data", "SimName", 256, sim_name);

	LLVector3d pos_global;
	msg->getVector3d("Data", "PosGlobal", pos_global);

    S32 region_x = llround((F32)pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = llround((F32)pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = llround((F32)pos_global.mdV[VZ]);
   
    snprintf(buffer, sizeof(buffer), "%s (%d, %d, %d)", sim_name, region_x, region_y, region_z);		/*Flawfinder: ignore*/
    location_text.append(buffer);

	S32 sort_order;
    msg->getS32("Data", "SortOrder", sort_order);

	BOOL enabled;
	msg->getBOOL("Data", "Enabled", enabled);

    // Look up the panel to fill in
    LLPanelPick *self = NULL;
    for (self = sAllPanels.getFirstData(); self; self = sAllPanels.getNextData())
    {
		// For top picks, must match pick id
		if (self->mPickID != pick_id)
		{
			continue;
		}

		self->mDataReceived = TRUE;

        // Found the panel, now fill in the information
		self->mPickID = pick_id;
		self->mCreatorID = creator_id;
		self->mParcelID = parcel_id;
		self->mSimName.assign(sim_name);
		self->mPosGlobal = pos_global;

		// Update UI controls
        self->mNameEditor->setText(name);
        self->mDescEditor->setText(desc);
        self->mSnapshotCtrl->setImageAssetID(snapshot_id);
        self->mLocationEditor->setText(location_text);
        self->mEnabledCheck->set(enabled);

		snprintf(buffer, sizeof(buffer), "%d", sort_order);		/*Flawfinder: ignore*/
		self->mSortOrderEditor->setText(buffer);
    }
}

void LLPanelPick::draw()
{
	if (getVisible())
	{
		refresh();

		LLPanel::draw();
	}
}


void LLPanelPick::refresh()
{
	if (!mDataRequested)
	{
		sendPickInfoRequest();
	}

    // Check for god mode
    BOOL godlike = gAgent.isGodlike();
	BOOL is_self = (gAgent.getID() == mCreatorID);

    // Set button visibility/enablement appropriately
	if (mTopPick)
	{
		mSnapshotCtrl->setEnabled(godlike);
		mNameEditor->setEnabled(godlike);
		mDescEditor->setEnabled(godlike);

		mSortOrderText->setVisible(godlike);

		mSortOrderEditor->setVisible(godlike);
		mSortOrderEditor->setEnabled(godlike);

		mEnabledCheck->setVisible(godlike);
		mEnabledCheck->setEnabled(godlike);

		mSetBtn->setVisible(godlike);
		mSetBtn->setEnabled(godlike);
	}
	else
	{
		mSnapshotCtrl->setEnabled(is_self);
		mNameEditor->setEnabled(is_self);
		mDescEditor->setEnabled(is_self);

		mSortOrderText->setVisible(FALSE);

		mSortOrderEditor->setVisible(FALSE);
		mSortOrderEditor->setEnabled(FALSE);

		mEnabledCheck->setVisible(FALSE);
		mEnabledCheck->setEnabled(FALSE);

		mSetBtn->setVisible(is_self);
		mSetBtn->setEnabled(is_self);
	}
}


// static
void LLPanelPick::onClickTeleport(void* data)
{
    LLPanelPick* self = (LLPanelPick*)data;

    if (!self->mPosGlobal.isExactlyZero())
    {
        gAgent.teleportViaLocation(self->mPosGlobal);
        gFloaterWorldMap->trackLocation(self->mPosGlobal);
    }
}


// static
void LLPanelPick::onClickMap(void* data)
{
	LLPanelPick* self = (LLPanelPick*)data;
	gFloaterWorldMap->trackLocation(self->mPosGlobal);
	LLFloaterWorldMap::show(NULL, TRUE);
}

// static
/*
void LLPanelPick::onClickLandmark(void* data)
{
    LLPanelPick* self = (LLPanelPick*)data;
	create_landmark(self->mNameEditor->getText().c_str(), "", self->mPosGlobal);
}
*/

// static
void LLPanelPick::onClickSet(void* data)
{
    LLPanelPick* self = (LLPanelPick*)data;

	// Save location for later.
	self->mPosGlobal = gAgent.getPositionGlobal();

	LLString location_text;
	location_text.assign("(will update after save)");
	location_text.append(", ");

    S32 region_x = llround((F32)self->mPosGlobal.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = llround((F32)self->mPosGlobal.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = llround((F32)self->mPosGlobal.mdV[VZ]);

	location_text.append(self->mSimName);
    location_text.append(llformat(" (%d, %d, %d)", region_x, region_y, region_z));

	// if sim name in pick is different from current sim name
	// make sure it's clear that all that's being changed
	// is the location and nothing else
	if ( gAgent.getRegion ()->getName () != self->mSimName )
	{
		gViewerWindow->alertXml("SetPickLocation");
	};

	self->mLocationEditor->setText(location_text);

	onCommitAny(NULL, data);
}


// static
void LLPanelPick::onCommitAny(LLUICtrl* ctrl, void* data)
{
	LLPanelPick* self = (LLPanelPick*)data;

	// have we received up to date data for this pick?
	if (self->mDataReceived)
	{
		self->sendPickInfoUpdate();

		// Big hack - assume that top picks are always in a browser,
		// and non-top-picks are always in a tab container.
		/*if (self->mTopPick)
		{
			LLPanelDirPicks* panel = (LLPanelDirPicks*)self->getParent();
			panel->renamePick(self->mPickID, self->mNameEditor->getText().c_str());
		}
		else
		{*/
		LLTabContainerVertical* tab = (LLTabContainerVertical*)self->getParent();
		if (tab)
		{
			if(tab) tab->setCurrentTabName(self->mNameEditor->getText());
		}
		//}
	}
}
