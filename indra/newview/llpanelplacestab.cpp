/** 
 * @file llpanelplacestab.cpp
 * @brief Tabs interface for Side Bar "Places" panel
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

#include "llpanelplacestab.h"

#include "llbutton.h"
#include "llnotificationsutil.h"

#include "llwindow.h"

#include "llpanelplaces.h"
#include "llslurl.h"
#include "llworldmap.h"

std::string LLPanelPlacesTab::sFilterSubString = LLStringUtil::null;

bool LLPanelPlacesTab::isTabVisible()
{
	LLUICtrl* parent = getParentUICtrl();
	if (!parent) return false;
	if (!parent->getVisible()) return false;
	return true;
}

void LLPanelPlacesTab::setPanelPlacesButtons(LLPanelPlaces* panel)
{
	mTeleportBtn = panel->getChild<LLButton>("teleport_btn");
	mShowOnMapBtn = panel->getChild<LLButton>("map_btn");
	mShowProfile = panel->getChild<LLButton>("profile_btn");
}

void LLPanelPlacesTab::onRegionResponse(const LLVector3d& landmark_global_pos,
										U64 region_handle,
										const std::string& url,
										const LLUUID& snapshot_id,
										bool teleport)
{
	std::string sim_name;
	bool gotSimName = LLWorldMap::getInstance()->simNameFromPosGlobal( landmark_global_pos, sim_name );

	std::string sl_url;
	if ( gotSimName )
	{
		sl_url = LLSLURL(sim_name, landmark_global_pos).getSLURLString();
	}
	else
	{
		sl_url = "";
	}

	LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(sl_url));
		
	LLSD args;
	args["SLURL"] = sl_url;

	LLNotificationsUtil::add("CopySLURL", args);
}
