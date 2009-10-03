/** 
 * @file llpanelpick.cpp
 * @brief LLPanelPick class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

// Display of a "Top Pick" used both for the global top picks in the 
// Find directory, and also for each individual user's picks in their
// profile.

#include "llviewerprecompiledheaders.h"
#include "llpanel.h"
#include "message.h"
#include "llagent.h"
#include "llbutton.h"
#include "lllineeditor.h"
#include "llparcel.h"
#include "llviewerparcelmgr.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lluiconstants.h"
#include "llworldmap.h"
#include "llfloaterworldmap.h"
#include "llfloaterreg.h"
#include "llavatarpropertiesprocessor.h"
#include "llpanelpick.h"


#define XML_PANEL_EDIT_PICK "panel_edit_pick.xml"
#define XML_PANEL_PICK_INFO "panel_pick_info.xml"

#define XML_NAME		"pick_name"
#define XML_DESC		"pick_desc"
#define XML_SNAPSHOT	"pick_snapshot"
#define XML_LOCATION	"pick_location"

#define XML_BTN_SAVE "save_changes_btn"

#define SAVE_BTN_LABEL "[WHAT]"
#define LABEL_PICK = "Pick"
#define LABEL_CHANGES = "Changes"

std::string SET_LOCATION_NOTICE("(will update after save)");

LLPanelPick::LLPanelPick(BOOL edit_mode/* = FALSE */)
:	LLPanel(), LLAvatarPropertiesObserver(), LLRemoteParcelInfoObserver(),
	mEditMode(edit_mode),
	mSnapshotCtrl(NULL),
	mPickId(LLUUID::null),
	mCreatorId(LLUUID::null),
	mDataReceived(FALSE),
	mIsPickNew(false),
	mLocationChanged(false)
{
	if (edit_mode)
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, XML_PANEL_EDIT_PICK);
		LLAvatarPropertiesProcessor::instance().addObserver(gAgentID, this);
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, XML_PANEL_PICK_INFO);
	}

}

LLPanelPick::~LLPanelPick()
{
	if (mCreatorId.notNull()) 	LLAvatarPropertiesProcessor::instance().removeObserver(mCreatorId, this);
}

void LLPanelPick::reset()
{
	setEditMode(FALSE);
	
	mPickId.setNull();
	mCreatorId.setNull();
	mParcelId.setNull();
	
	setPickName("");
	setPickDesc("");
	setPickLocation("");
	mSnapshotCtrl->setImageAssetID(LLUUID::null);

	//*HACK just setting asset id to NULL not enough to clear 
	//the texture controls, w/o setValid(FALSE) it continues to 
	//draw the previously set image
	mSnapshotCtrl->setValid(FALSE);

	mDataReceived = FALSE;

	mPosGlobal.clearVec();

	childSetValue("maturity", "");
}

BOOL LLPanelPick::postBuild()
{
	mSnapshotCtrl = getChild<LLTextureCtrl>(XML_SNAPSHOT);

	if (mEditMode)
	{
		enableSaveButton(FALSE);

		mSnapshotCtrl->setOnSelectCallback(boost::bind(&LLPanelPick::onPickChanged, this, _1));

		LLLineEditor* line_edit = getChild<LLLineEditor>("pick_name");
		line_edit->setKeystrokeCallback(boost::bind(&LLPanelPick::onPickChanged, this, _1), NULL);
		
		LLTextEditor* text_edit = getChild<LLTextEditor>("pick_desc");
		text_edit->setKeystrokeCallback(boost::bind(&LLPanelPick::onPickChanged, this, _1));

		childSetAction("cancel_btn", boost::bind(&LLPanelPick::onClickCancel, this));
		childSetAction("set_to_curr_location_btn", boost::bind(&LLPanelPick::onClickSet, this));
		childSetAction(XML_BTN_SAVE, boost::bind(&LLPanelPick::onClickSave, this));

		mSnapshotCtrl->setMouseEnterCallback(boost::bind(&LLPanelPick::childSetVisible, this, "edit_icon", true));
		mSnapshotCtrl->setMouseLeaveCallback(boost::bind(&LLPanelPick::childSetVisible, this, "edit_icon", false));
	}
	else
	{
		childSetAction("edit_btn", boost::bind(&LLPanelPick::onClickEdit, this));
		childSetAction("teleport_btn", boost::bind(&LLPanelPick::onClickTeleport, this));
		childSetAction("show_on_map_btn", boost::bind(&LLPanelPick::onClickMap, this));

	}

	// EXT-822. We have to process "Back" button click in both Edit & View Modes
	if (!mBackCb.empty())
	{
		LLButton* button = findChild<LLButton>("back_btn");
		if (button) button->setClickedCallback(mBackCb);
	}

	return TRUE;
}

void LLPanelPick::init(LLUUID creator_id, LLUUID pick_id)
{
	mCreatorId = creator_id;
	mPickId = pick_id;

	//*TODO consider removing this, already called by setEditMode()
	updateButtons();

	requestData();
}

void LLPanelPick::requestData()
{
	mDataReceived = FALSE;
	LLAvatarPropertiesProcessor::instance().addObserver(mCreatorId, this);
	LLAvatarPropertiesProcessor::instance().sendPickInfoRequest(mCreatorId, mPickId);
}

void LLPanelPick::init(LLPickData *pick_data)
{
	mPickId = pick_data->pick_id;
	mCreatorId = pick_data->creator_id;

	setPickName(pick_data->name);
	setPickDesc(pick_data->desc);
	
	mSnapshotCtrl->setImageAssetID(pick_data->snapshot_id);

	//*HACK see reset() where the texture control was set to FALSE
	mSnapshotCtrl->setValid(TRUE);

	mPosGlobal = pick_data->pos_global;
	mSimName = pick_data->sim_name;
	mParcelId = pick_data->parcel_id;

	setPickLocation(createLocationText(pick_data->user_name, pick_data->original_name,
		pick_data->sim_name, pick_data->pos_global));
}

void LLPanelPick::prepareNewPick(const LLVector3d pos_global,
								const std::string& name,
								const std::string& desc,
								const LLUUID& snapshot_id,
								const LLUUID& parcel_id)
{
	mPickId.generate();
	mCreatorId = gAgent.getID();
	mPosGlobal = pos_global;
	setPickName(name);
	setPickDesc(desc);
	mSnapshotCtrl->setImageAssetID(snapshot_id);
	mParcelId = parcel_id;

	setPickLocation(createLocationText(std::string(""), SET_LOCATION_NOTICE, name, pos_global));

	childSetLabelArg(XML_BTN_SAVE, SAVE_BTN_LABEL, std::string("Pick"));

	mIsPickNew = true;
}

// Fill in some reasonable defaults for a new pick.
void LLPanelPick::prepareNewPick()
{
	// Try to fill in the current parcel
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (parcel)
	{
		prepareNewPick(gAgent.getPositionGlobal(),
					  parcel->getName(),
					  parcel->getDesc(),
					  parcel->getSnapshotID(),
					  parcel->getID());
	}
}

/*virtual*/ void LLPanelPick::processProperties(void* data, EAvatarProcessorType type)
{
	if (APT_PICK_INFO != type) return;
	if (!data) return;

	LLPickData* pick_data = static_cast<LLPickData *>(data);
	if (!pick_data) return;
	if (mPickId != pick_data->pick_id) return;

	init(pick_data);
	mDataReceived = TRUE;
	LLAvatarPropertiesProcessor::instance().removeObserver(mCreatorId, this);

	if (!mEditMode) 
	{
		LLRemoteParcelInfoProcessor::getInstance()->addObserver(pick_data->parcel_id, this);
		LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(pick_data->parcel_id);
	}
}


void LLPanelPick::setEditMode( BOOL edit_mode )
{
	if (mEditMode == edit_mode) return;
	mEditMode = edit_mode;

	// preserve data before killing controls
	LLUUID snapshot_id = mSnapshotCtrl->getImageAssetID();
	LLRect old_rect = getRect();

	deleteAllChildren();

	// *WORKAROUND: for EXT-931. Children are created for both XML_PANEL_EDIT_PICK & XML_PANEL_PICK_INFO files
	// The reason is in LLPanel::initPanelXML called from the LLUICtrlFactory::buildPanel().
	// It creates children from the xml file stored while previous initializing in the "mXMLFilename" member
	// and then in creates children from the parameters passed from the LLUICtrlFactory::buildPanel().
	// Xml filename is stored after LLPanel::initPanelXML is called (added with export-from-ll/viewer-2-0, r1594 into LLUICtrlFactory::buildPanel & LLUICtrlFactory::buildFloater)
	// In case panel creates children from the different xml files they appear from both files.
	// So, let clear xml filename related to this instance.
	setXMLFilename("");

	if (edit_mode)
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, XML_PANEL_EDIT_PICK);
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, XML_PANEL_PICK_INFO);
	}

	//*NOTE this code is from LLPanelMeProfile.togglePanel()... doubt this is a right way to do things
	reshape(old_rect.getWidth(), old_rect.getHeight());
	old_rect.setLeftTopAndSize(0, old_rect.getHeight(), old_rect.getWidth(), old_rect.getHeight());
	setRect(old_rect);

	// time to restore data
	setPickName(mName);
	setPickDesc(mDesc);
	setPickLocation(mLocation);
	mSnapshotCtrl->setImageAssetID(snapshot_id);

	updateButtons();
}

void LLPanelPick::onPickChanged(LLUICtrl* ctrl)
{
	if(mLocationChanged)
	{
		// Pick was enabled in onClickSet
		return;
	}

	if( mSnapshotCtrl->isDirty()
		|| getChild<LLLineEditor>("pick_name")->isDirty()
		|| getChild<LLTextEditor>("pick_desc")->isDirty() )
	{
		enableSaveButton(TRUE);
	}
	else
	{
		enableSaveButton(FALSE);
	}
}

//////////////////////////////////////////////////////////////////////////
// PROTECTED AREA
//////////////////////////////////////////////////////////////////////////

//static
std::string LLPanelPick::createLocationText(const std::string& owner_name, const std::string& original_name,
												 const std::string& sim_name, const LLVector3d& pos_global)
{
	std::string location_text;
	location_text.append(owner_name);
	if (!original_name.empty())
	{
		if (!location_text.empty()) location_text.append(", ");
		location_text.append(original_name);

	}
	if (!sim_name.empty())
	{
		if (!location_text.empty()) location_text.append(", ");
		location_text.append(sim_name);
	}

	if (!location_text.empty()) location_text.append(" ");

	if (!pos_global.isNull())
	{
		S32 region_x = llround((F32)pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
		S32 region_y = llround((F32)pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
		S32 region_z = llround((F32)pos_global.mdV[VZ]);
		location_text.append(llformat(" (%d, %d, %d)", region_x, region_y, region_z));
	}
	return location_text;
}

void LLPanelPick::setPickName(std::string name)
{
	childSetValue(XML_NAME, name);
	
	//preserving non-wrapped text for info/edit modes switching
	mName = name;
}

void LLPanelPick::setPickDesc(std::string desc)
{
	childSetValue(XML_DESC, desc);

	//preserving non-wrapped text for info/edit modes switching
	mDesc = desc;
}

void LLPanelPick::setPickLocation(const std::string& location)
{
	childSetValue(XML_LOCATION, location);

	//preserving non-wrapped text for info/edit modes switching
	mLocation = location;
}

std::string LLPanelPick::getPickName()
{
	return childGetValue(XML_NAME).asString();
}

std::string LLPanelPick::getPickDesc()
{
	return childGetValue(XML_DESC).asString();
}

std::string LLPanelPick::getPickLocation()
{
	return childGetValue(XML_LOCATION).asString();
}

void LLPanelPick::sendUpdate()
{
	LLPickData pick_data;

	// If we don't have a pick id yet, we'll need to generate one,
	// otherwise we'll keep overwriting pick_id 00000 in the database.
	if (mPickId.isNull()) mPickId.generate();

	pick_data.agent_id = gAgent.getID();
	pick_data.session_id = gAgent.getSessionID();
	pick_data.pick_id = mPickId;
	pick_data.creator_id = gAgentID;

	//legacy var  need to be deleted
	pick_data.top_pick = FALSE; 
	pick_data.parcel_id = mParcelId;
	pick_data.name = getPickName();
	pick_data.desc = getPickDesc();
	pick_data.snapshot_id = mSnapshotCtrl->getImageAssetID();
	pick_data.pos_global = mPosGlobal;
	pick_data.sort_order = 0;
	pick_data.enabled = TRUE;

	mDataReceived = FALSE;
	LLAvatarPropertiesProcessor::instance().addObserver(gAgentID, this);

	LLAvatarPropertiesProcessor::instance().sendPickInfoUpdate(&pick_data);
}


//-----------------------------------------
// "PICK INFO" (VIEW MODE) BUTTON HANDLERS
//-----------------------------------------

//static
void LLPanelPick::onClickEdit()
{
	if (mEditMode) return;
	if (!mDataReceived) return;
	setEditMode(TRUE);
}

//static
void LLPanelPick::onClickTeleport()
{
	teleport(mPosGlobal);
}

//static
void LLPanelPick::onClickMap()
{
	showOnMap(mPosGlobal);
}


//-----------------------------------------
// "EDIT PICK" (EDIT MODE) BUTTON HANDLERS
//-----------------------------------------

//static
void LLPanelPick::onClickCancel()
{
	if (!mEditMode) return;
	
	if (mIsPickNew)
	{
		mBackCb(this, LLSD());
		return;
	}

	LLUUID pick_id = mPickId;
	LLUUID creator_id = mCreatorId;
	reset();
	init(creator_id, pick_id);
}

// static
void LLPanelPick::onClickSet()
{
	if (!mEditMode) return;
	if (!mIsPickNew && !mDataReceived) return;

	// Save location for later.
	mPosGlobal = gAgent.getPositionGlobal();

	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (parcel)
	{
		mParcelId = parcel->getID();
		mSimName = parcel->getName();
	}
	setPickLocation(createLocationText(std::string(""), SET_LOCATION_NOTICE, mSimName, mPosGlobal));

	mLocationChanged = true;
	enableSaveButton(TRUE);
}

// static
void LLPanelPick::onClickSave()
{
	if (!mEditMode) return;
	if (!mIsPickNew && !mDataReceived) return;

	sendUpdate();
	
	if (mIsPickNew)
	{
		mBackCb(this, LLSD());
		return;
	}

	setEditMode(FALSE);
}

void LLPanelPick::updateButtons()
{

	// on Pick Info panel (for non-Agent picks) edit_btn should be invisible
	if (mEditMode)
	{
		childSetLabelArg(XML_BTN_SAVE, SAVE_BTN_LABEL, std::string("Changes"));
	}
	else 
	{
		if (mCreatorId != gAgentID)
		{
			childSetEnabled("edit_btn", FALSE);
			childSetVisible("edit_btn", FALSE);
		}
		else 
		{
			childSetEnabled("edit_btn", TRUE);
			childSetVisible("edit_btn", TRUE);
		}
	}
}

void LLPanelPick::setExitCallback(commit_callback_t cb)
{
	mBackCb = cb;
	LLButton* button = findChild<LLButton>("back_btn");
	if (button) button->setClickedCallback(mBackCb);
}

//static
void LLPanelPick::teleport(const LLVector3d& position)
{
	if (!position.isExactlyZero())
	{
		gAgent.teleportViaLocation(position);
		LLFloaterWorldMap::getInstance()->trackLocation(position);
	}
}

//static
void LLPanelPick::showOnMap(const LLVector3d& position)
{
	LLFloaterWorldMap::getInstance()->trackLocation(position);
	LLFloaterReg::showInstance("world_map", "center");
}

void LLPanelPick::processParcelInfo(const LLParcelData& parcel_data)
{
	if (mEditMode) return;

	// HACK: Flag 0x2 == adult region,
	// Flag 0x1 == mature region, otherwise assume PG
	std::string rating_icon = "icon_event.tga";
	if (parcel_data.flags & 0x2)
	{
		rating_icon = "icon_event_adult.tga";
	}
	else if (parcel_data.flags & 0x1)
	{
		rating_icon = "icon_event_mature.tga";
	}
	
	childSetValue("maturity", rating_icon);

	//*NOTE we don't removeObserver(...) ourselves cause LLRemoveParcelProcessor does it for us
}

void LLPanelPick::enableSaveButton(bool enable)
{
	if(!mEditMode)
	{
		return;
	}
	childSetEnabled(XML_BTN_SAVE, enable);
}
