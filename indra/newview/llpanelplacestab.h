/** 
 * @file llpanelplacestab.h
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

#ifndef LL_LLPANELPLACESTAB_H
#define LL_LLPANELPLACESTAB_H

#include "llpanel.h"

class LLPanelPlaces;

class LLPanelPlacesTab : public LLPanel
{
public:
	LLPanelPlacesTab() : LLPanel() {}
	virtual ~LLPanelPlacesTab() {}

	virtual void onSearchEdit(const std::string& string) = 0;
	virtual void updateVerbs() = 0;		// Updates buttons at the bottom of Places panel
	virtual void onShowOnMap() = 0;
	virtual void onShowProfile() = 0;
	virtual void onTeleport() = 0;
	virtual bool isSingleItemSelected() = 0;

	bool isTabVisible(); // Check if parent TabContainer is visible.

	void setPanelPlacesButtons(LLPanelPlaces* panel);
	void onRegionResponse(const LLVector3d& landmark_global_pos,
										U64 region_handle,
										const std::string& url,
										const LLUUID& snapshot_id,
										bool teleport);

	const std::string& getFilterSubString() { return sFilterSubString; }
	void setFilterSubString(const std::string& string) { sFilterSubString = string; }

protected:
	LLButton*				mTeleportBtn;
	LLButton*				mShowOnMapBtn;
	LLButton*				mShowProfile;

	// Search string for filtering landmarks and teleport history locations
	static std::string		sFilterSubString;
};

#endif //LL_LLPANELPLACESTAB_H
