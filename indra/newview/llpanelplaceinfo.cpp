/**
 * @file llpanelplaceinfo.cpp
 * @brief Base class for place information in Side Tray.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llpanelplaceinfo.h"

#include "llavatarname.h"
#include "llsdutil.h"

#include "llsdutil_math.h"

#include "llregionhandle.h"

#include "lliconctrl.h"
#include "lltextbox.h"

#include "lltrans.h"

#include "llagent.h"
#include "llexpandabletextbox.h"
#include "llpanelpick.h"
#include "lltexturectrl.h"
#include "llviewerregion.h"

LLPanelPlaceInfo::LLPanelPlaceInfo()
:	LLPanel(),
	mParcelID(),
	mRequestedID(),
	mPosRegion(),
	mScrollingPanelMinHeight(0),
	mScrollingPanelWidth(0),
	mInfoType(UNKNOWN),
	mScrollingPanel(NULL),
	mScrollContainer(NULL),
	mDescEditor(NULL)
{}

//virtual
LLPanelPlaceInfo::~LLPanelPlaceInfo()
{
	if (mParcelID.notNull())
	{
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelID, this);
	}
}

//virtual
BOOL LLPanelPlaceInfo::postBuild()
{
	mTitle = getChild<LLTextBox>("title");
	mCurrentTitle = mTitle->getText();

	mSnapshotCtrl = getChild<LLTextureCtrl>("logo");
	mRegionName = getChild<LLTextBox>("region_title");
	mParcelName = getChild<LLTextBox>("parcel_title");
	mDescEditor = getChild<LLExpandableTextBox>("description");

	mMaturityRatingIcon = getChild<LLIconCtrl>("maturity_icon");
	mMaturityRatingText = getChild<LLTextBox>("maturity_value");

	mScrollingPanel = getChild<LLPanel>("scrolling_panel");
	mScrollContainer = getChild<LLScrollContainer>("place_scroll");

	mScrollingPanelMinHeight = mScrollContainer->getScrolledViewRect().getHeight();
	mScrollingPanelWidth = mScrollingPanel->getRect().getWidth();

	return TRUE;
}

//virtual
void LLPanelPlaceInfo::resetLocation()
{
	mParcelID.setNull();
	mRequestedID.setNull();
	mPosRegion.clearVec();

	std::string loading = LLTrans::getString("LoadingData");
	mMaturityRatingText->setValue(loading);
	mRegionName->setText(loading);
	mParcelName->setText(loading);
	mDescEditor->setText(loading);
	mMaturityRatingIcon->setValue(LLUUID::null);

	mSnapshotCtrl->setImageAssetID(LLUUID::null);
}

//virtual
void LLPanelPlaceInfo::setParcelID(const LLUUID& parcel_id)
{
	mParcelID = parcel_id;
	sendParcelInfoRequest();
}

//virtual
void LLPanelPlaceInfo::setInfoType(EInfoType type)
{
	mTitle->setText(mCurrentTitle);
	mTitle->setToolTip(mCurrentTitle);

	mInfoType = type;
}

void LLPanelPlaceInfo::sendParcelInfoRequest()
{
	if (mParcelID != mRequestedID)
	{
        //ext-4655, defensive. remove now incase this gets called twice without a remove
        //as panel never closes its ok atm (but wrong :) 
        LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mRequestedID, this);

		LLRemoteParcelInfoProcessor::getInstance()->addObserver(mParcelID, this);
		LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(mParcelID);

		mRequestedID = mParcelID;
	}
}

void LLPanelPlaceInfo::displayParcelInfo(const LLUUID& region_id,
										 const LLVector3d& pos_global)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
		return;

	mPosRegion.setVec((F32)fmod(pos_global.mdV[VX], (F64)REGION_WIDTH_METERS),
					  (F32)fmod(pos_global.mdV[VY], (F64)REGION_WIDTH_METERS),
					  (F32)pos_global.mdV[VZ]);

	LLSD body;
	std::string url = region->getCapability("RemoteParcelRequest");
	if (!url.empty())
	{
		body["location"] = ll_sd_from_vector3(mPosRegion);
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
	else
	{
		error_text = getString("server_error_text");
	}

	mDescEditor->setText(error_text);

	std::string not_available = getString("not_available");
	mMaturityRatingText->setValue(not_available);
	mRegionName->setText(not_available);
	mParcelName->setText(not_available);
	mMaturityRatingIcon->setValue(LLUUID::null);

	// Enable "Back" button that was disabled when parcel request was sent.
	getChild<LLButton>("back_btn")->setEnabled(TRUE);
}

// virtual
void LLPanelPlaceInfo::processParcelInfo(const LLParcelData& parcel_data)
{
	if(parcel_data.snapshot_id.notNull())
	{
		mSnapshotCtrl->setImageAssetID(parcel_data.snapshot_id);
	}

	if(!parcel_data.sim_name.empty())
	{
		mRegionName->setText(parcel_data.sim_name);
	}
	else
	{
		mRegionName->setText(LLStringUtil::null);
	}

	if(!parcel_data.desc.empty())
	{
		mDescEditor->setText(parcel_data.desc);
	}
	else
	{
		mDescEditor->setText(getString("not_available"));
	}

	S32 region_x;
	S32 region_y;
	S32 region_z;

	// If the region position is zero, grab position from the global
	if(mPosRegion.isExactlyZero())
	{
		region_x = llround(parcel_data.global_x) % REGION_WIDTH_UNITS;
		region_y = llround(parcel_data.global_y) % REGION_WIDTH_UNITS;
		region_z = llround(parcel_data.global_z);
	}
	else
	{
		region_x = llround(mPosRegion.mV[VX]);
		region_y = llround(mPosRegion.mV[VY]);
		region_z = llround(mPosRegion.mV[VZ]);
	}

	if (!parcel_data.name.empty())
	{
		mParcelTitle = parcel_data.name;

		mParcelName->setText(llformat("%s (%d, %d, %d)",
							 mParcelTitle.c_str(), region_x, region_y, region_z));
	}
	else
	{
		mParcelName->setText(getString("not_available"));
	}
}

// virtual
void LLPanelPlaceInfo::reshape(S32 width, S32 height, BOOL called_from_parent)
{

	// This if was added to force collapsing description textbox on Windows at the beginning of reshape
	// (the only case when reshape is skipped here is when it's caused by this textbox, so called_from_parent is FALSE)
	// This way it is consistent with Linux where topLost collapses textbox at the beginning of reshape.
	// On windows it collapsed only after reshape which caused EXT-8342.
	if(called_from_parent)
	{
		if(mDescEditor) mDescEditor->onTopLost();
	}

	LLPanel::reshape(width, height, called_from_parent);

	if (!mScrollContainer || !mScrollingPanel)
		return;

	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	S32 scroll_height = mScrollContainer->getRect().getHeight();
	if (mScrollingPanelMinHeight > scroll_height)
	{
		mScrollingPanel->reshape(mScrollingPanelWidth, mScrollingPanelMinHeight);
	}
	else
	{
		mScrollingPanel->reshape(mScrollingPanelWidth + scrollbar_size, scroll_height);
	}
}

void LLPanelPlaceInfo::createPick(const LLVector3d& pos_global, LLPanelPickEdit* pick_panel)
{
	std::string region_name = mRegionName->getText();

	LLPickData data;
	data.pos_global = pos_global;
	data.name = mParcelTitle.empty() ? region_name : mParcelTitle;
	data.sim_name = region_name;
	data.desc = mDescEditor->getText();
	data.snapshot_id = mSnapshotCtrl->getImageAssetID();
	data.parcel_id = mParcelID;
	pick_panel->setPickData(&data);
}

// static
void LLPanelPlaceInfo::onNameCache(LLTextBox* text, const std::string& full_name)
{
	text->setText(full_name);
}

// static
void LLPanelPlaceInfo::onAvatarNameCache(const LLUUID& agent_id,
										 const LLAvatarName& av_name,
										 LLTextBox* text)
{
	text->setText( av_name.getCompleteName() );
}
