/** 
 * @file llfloaterurldisplay.h
 * @brief Probably should be called LLFloaterTeleport, or LLFloaterLandmark
 * as it gives you a preview of a potential teleport location.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llregionhandle.h"
#include "v3dmath.h"

#include "llfloaterurldisplay.h"

#include "llpanelplace.h"
#include "lluictrlfactory.h"

////////////////////////////////////////////////////////////////////////////
// LLFloaterURLDisplay


LLFloaterURLDisplay::LLFloaterURLDisplay(const LLSD& sd)
	: LLFloater(sd)
{	
	mFactoryMap["place_details_panel"] = LLCallbackMap(LLFloaterURLDisplay::createPlaceDetail, this);
//	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_preview_url.xml");

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
{	// Set the name and also clear description
	mPlacePanel->resetName(name);
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
	LLUICtrlFactory::getInstance()->buildPanel(self->mPlacePanel, "panel_place.xml");

	return self->mPlacePanel;
}
