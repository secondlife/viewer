/**
 * @file llpanelplaceinfo.cpp
 * @brief Displays place information in Side Tray.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
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

#include "llviewerprecompiledheaders.h"

#include "llpanelplaceinfo.h"

// *TODO: reorder includes to match the coding standard
#include "llinventory.h"
#include "llviewercontrol.h"
#include "llqueryflags.h"
#include "llui.h"
#include "llsecondlifeurls.h"
#include "llfloater.h"
#include "llfloaterreg.h"

#include "llagent.h"
#include "llviewerwindow.h"
#include "llviewerinventory.h"
#include "llbutton.h"
#include "llfloaterworldmap.h"
#include "lllineeditor.h"
#include "llinventorymodel.h"
#include "lluiconstants.h"
#include "roles_constants.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llworldmap.h"
#include "llviewerregion.h"
#include "lluictrlfactory.h"
#include "llweb.h"
#include "llsdutil.h"

static LLRegisterPanelClassWrapper<LLPanelPlaceInfo> t_places("panel_landmark_info");

LLPanelPlaceInfo::LLPanelPlaceInfo()
:	LLPanel(),
	mParcelID(),
	mRequestedID(),
	mPosRegion(),
	mLandmarkID()
{}

LLPanelPlaceInfo::~LLPanelPlaceInfo()
{
	if (mParcelID.notNull())
	{
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelID, this);
	}
}

BOOL LLPanelPlaceInfo::postBuild()
{
	// Since this is only used in the directory browser, always
	// disable the snapshot control. Otherwise clicking on it will
	// open a texture picker.
	mSnapshotCtrl = getChild<LLTextureCtrl>("logo");
	mSnapshotCtrl->setEnabled(FALSE);

	mRegionName = getChild<LLTextBox>("region_name");
	mParcelName = getChild<LLTextBox>("parcel_name");
	mDescEditor = getChild<LLTextEditor>("description");
	mRating = getChild<LLIconCtrl>("maturity");

	mOwner = getChild<LLTextBox>("owner");
	mCreator = getChild<LLTextBox>("creator");
	mCreated = getChild<LLTextBox>("created");

	mTitleEditor = getChild<LLLineEditor>("title_editor");
	mTitleEditor->setCommitCallback(boost::bind(&LLPanelPlaceInfo::onCommitTitleOrNote, this, TITLE));

	mNotesEditor = getChild<LLTextEditor>("notes_editor");
	mNotesEditor->setCommitCallback(boost::bind(&LLPanelPlaceInfo::onCommitTitleOrNote, this, NOTE));
	mNotesEditor->setCommitOnFocusLost(true);

	mInfoPanel = getChild<LLPanel>("info_panel");

	return TRUE;
}

void LLPanelPlaceInfo::displayItemInfo(const LLInventoryItem* pItem)
{
	if (!pItem)
		return;

	mLandmarkID = pItem->getUUID();

	if(!gCacheName)
		return;

	const LLPermissions& perm = pItem->getPermissions();

	//////////////////
	// CREATOR NAME //
	//////////////////
	if (pItem->getCreatorUUID().notNull())
	{
		std::string name;
		LLUUID creator_id = pItem->getCreatorUUID();
		if (!gCacheName->getFullName(creator_id, name))
		{
			gCacheName->get(creator_id, FALSE,
							boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mCreator, _2, _3));
		}
		mCreator->setText(name);
	}
	else
	{
		mCreator->setText(getString("unknown"));
	}

	////////////////
	// OWNER NAME //
	////////////////
	if(perm.isOwned())
	{
		std::string name;
		if (perm.isGroupOwned())
		{
			LLUUID group_id = perm.getGroup();
			if (!gCacheName->getGroupName(group_id, name))
			{
				gCacheName->get(group_id, TRUE,
								boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mOwner, _2, _3));
			}
		}
		else
		{
			LLUUID owner_id = perm.getOwner();
			if (!gCacheName->getFullName(owner_id, name))
			{
				gCacheName->get(owner_id, FALSE,
								boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mOwner, _2, _3));
			}
		}
		mOwner->setText(name);
	}
	else
	{
		mOwner->setText(getString("public"));
	}
	
	//////////////////
	// ACQUIRE DATE //
	//////////////////
	time_t time_utc = pItem->getCreationDate();
	if (0 == time_utc)
	{
		mCreated->setText(getString("unknown"));
	}
	else
	{
		std::string timeStr = getString("acquired_date");
		LLSD substitution;
		substitution["datetime"] = (S32) time_utc;
		LLStringUtil::format (timeStr, substitution);
		mCreated->setText(timeStr);
	}

	mTitleEditor->setText(pItem->getName());
	mNotesEditor->setText(pItem->getDescription());
}

void LLPanelPlaceInfo::nameUpdatedCallback(
	LLTextBox* text,
	const std::string& first,
	const std::string& last)
{
	text->setText(first + " " + last);
}

void LLPanelPlaceInfo::resetLocation()
{
	mParcelID.setNull();
	mRequestedID.setNull();
	mLandmarkID.setNull();
	mPosRegion.clearVec();
	std::string not_available = getString("not_available");
	mRating->setValue(not_available);
	mRegionName->setText(not_available);
	mParcelName->setText(not_available);
	mDescEditor->setText(not_available);
	mCreator->setText(not_available);
	mOwner->setText(not_available);
	mCreated->setText(not_available);
	mTitleEditor->setText(LLStringUtil::null);
	mNotesEditor->setText(LLStringUtil::null);
}

//virtual
void LLPanelPlaceInfo::setParcelID(const LLUUID& parcel_id)
{
	mParcelID = parcel_id;
	sendParcelInfoRequest();
}

void LLPanelPlaceInfo::sendParcelInfoRequest()
{
	if (mParcelID != mRequestedID)
	{
		LLRemoteParcelInfoProcessor::getInstance()->addObserver(mParcelID, this);
		LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(mParcelID);

		mRequestedID = mParcelID;
	}
}

// virtual
void LLPanelPlaceInfo::setErrorStatus(U32 status, const std::string& reason)
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

// virtual
void LLPanelPlaceInfo::processParcelInfo(const LLParcelData& parcel_data)
{
	if(parcel_data.snapshot_id.notNull())
	{
		mSnapshotCtrl->setImageAssetID(parcel_data.snapshot_id);
	}

	if( !parcel_data.name.empty())
	{
		mParcelName->setText(parcel_data.name);
	}

	if( !parcel_data.desc.empty())
	{
		mDescEditor->setText(parcel_data.desc);
	}

	// HACK: Flag 0x2 == adult region,
	// Flag 0x1 == mature region, otherwise assume PG
	std::string rating = LLViewerRegion::accessToString(SIM_ACCESS_PG);
	std::string rating_icon = "icon_event.tga";
	if (parcel_data.flags & 0x2)
	{
		rating = LLViewerRegion::accessToString(SIM_ACCESS_ADULT);
		rating_icon = "icon_event_adult.tga";
	}
	else if (parcel_data.flags & 0x1)
	{
		rating = LLViewerRegion::accessToString(SIM_ACCESS_MATURE);
		rating_icon = "icon_event_mature.tga";
	}
	mRating->setValue(rating_icon);

	// Just use given region position for display
	S32 region_x = llround(mPosRegion.mV[0]);
	S32 region_y = llround(mPosRegion.mV[1]);
	S32 region_z = llround(mPosRegion.mV[2]);

	// If the region position is zero, grab position from the global
	if(mPosRegion.isExactlyZero())
	{
		region_x = llround(parcel_data.global_x) % REGION_WIDTH_UNITS;
		region_y = llround(parcel_data.global_y) % REGION_WIDTH_UNITS;
		region_z = llround(parcel_data.global_z);
	}

	if (!parcel_data.sim_name.empty())
	{
		std::string name = llformat("%s (%d, %d, %d)",
									parcel_data.sim_name.c_str(), region_x, region_y, region_z);
		mRegionName->setText(name);
	}
}

void LLPanelPlaceInfo::displayParcelInfo(const LLVector3& pos_region,
									 const LLUUID& region_id,
									 const LLVector3d& pos_global)
{
	LLSD body;
	mPosRegion = pos_region;
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
		LLHTTPClient::post(url, body, new LLRemoteParcelRequestResponder(getObserverHandle()));
	}
	else
	{
		mDescEditor->setText(getString("server_update_text"));
	}
	mSnapshotCtrl->setImageAssetID(LLUUID::null);
	mSnapshotCtrl->setFallbackImageName("default_land_picture.j2c");
}

void LLPanelPlaceInfo::onCommitTitleOrNote(LANDMARK_INFO_TYPE type)
{
	LLInventoryItem* item = gInventory.getItem(mLandmarkID);
	if (!item)
		return;

	std::string current_value;
	std::string item_value;
	if (type == TITLE)
	{
		if (mTitleEditor)
		{
			current_value = mTitleEditor->getText();
			item_value = item->getName();
		}
	}
	else
	{
		if (mNotesEditor)
		{
			current_value = mNotesEditor->getText();
			item_value = item->getDescription();
		}
	}

	if (item_value != current_value &&
	    gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);

		if (type == TITLE)
		{
			new_item->rename(current_value);
		}
		else
		{
			new_item->setDescription(current_value);
		}

		new_item->updateServer(FALSE);
		gInventory.updateItem(new_item);
		gInventory.notifyObservers();
	}
}
