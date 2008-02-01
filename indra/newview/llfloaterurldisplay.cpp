/** 
 * @file llfloaterurldisplay.h
 * @brief Probably should be called LLFloaterTeleport, or LLFloaterLandmark
 * as it gives you a preview of a potential teleport location.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2008, Linden Research, Inc.
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

#include "llfloaterurldisplay.h"

#include "llpanelplace.h"
#include "llvieweruictrlfactory.h"

#include "v3dmath.h"

////////////////////////////////////////////////////////////////////////////
// LLFloaterURLDisplay


LLFloaterURLDisplay::LLFloaterURLDisplay(const LLSD& sd)
{	
	mFactoryMap["place_details_panel"] = LLCallbackMap(LLFloaterURLDisplay::createPlaceDetail, this);
	gUICtrlFactory->buildFloater(this, "floater_preview_url.xml", &getFactoryMap());
	this->setVisible(false);

	// If positioned at 0,0 the teleport button is behind the toolbar.
	LLRect r = getRect();
	if (r.mBottom == 0 && r.mLeft == 0)
	{
		// first use, center it
		center();
	}
	else
	{
		gFloaterView->adjustToFitScreen(this, FALSE);
	}
}

LLFloaterURLDisplay::~LLFloaterURLDisplay()
{
}

void LLFloaterURLDisplay::displayParcelInfo(U64 region_handle, const LLVector3& pos_local)
{
	mRegionHandle = region_handle;
	mRegionPosition = pos_local;
	LLVector3d pos_global = from_region_handle(region_handle);
	pos_global += (LLVector3d)pos_local;

	LLUUID region_id;			// don't know this
	LLUUID landmark_asset_id;	// don't know this either
	mPlacePanel->displayParcelInfo(pos_local, landmark_asset_id, region_id, pos_global);

	this->setVisible(true);
	this->setFrontmost(true);
}

void LLFloaterURLDisplay::setSnapshotDisplay(const LLUUID& snapshot_id)
{
	mPlacePanel->setSnapshot(snapshot_id);
}

void LLFloaterURLDisplay::setName(const std::string& name)
{
	mPlacePanel->setName(name);
}

void LLFloaterURLDisplay::setLocationString(const std::string& name)
{
	mPlacePanel->setLocationString(name);
}

// static
void* LLFloaterURLDisplay::createPlaceDetail(void* userdata)
{
	LLFloaterURLDisplay *self = (LLFloaterURLDisplay*)userdata;
	self->mPlacePanel = new LLPanelPlace();
	gUICtrlFactory->buildPanel(self->mPlacePanel, "panel_place.xml");

	return self->mPlacePanel;
}
