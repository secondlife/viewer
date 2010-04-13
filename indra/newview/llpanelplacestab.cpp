/** 
 * @file llpanelplacestab.cpp
 * @brief Tabs interface for Side Bar "Places" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
		F32 region_x = (F32)fmod( landmark_global_pos.mdV[VX], (F64)REGION_WIDTH_METERS );
		F32 region_y = (F32)fmod( landmark_global_pos.mdV[VY], (F64)REGION_WIDTH_METERS );

		sl_url = LLSLURL::buildSLURL(sim_name, llround(region_x), llround(region_y), llround((F32)landmark_global_pos.mdV[VZ]));
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
