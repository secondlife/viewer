/**
 * @file llpanelwearing.h
 * @brief List of agent's worn items.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLPANELWEARING_H
#define LL_LLPANELWEARING_H

#include "llpanel.h"

// newview
#include "llpanelappearancetab.h"

class LLInventoryCategoriesObserver;
class LLListContextMenu;
class LLWearableItemsList;
class LLWearingGearMenu;

/**
 * @class LLPanelWearing
 *
 * A list of agents's currently worn items represented by
 * a flat list view.
 * Starts fetching necessary inventory content on first opening.
 */
class LLPanelWearing : public LLPanelAppearanceTab
{
public:
	LLPanelWearing();
	virtual ~LLPanelWearing();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void onOpen(const LLSD& info);

	/*virtual*/ void setFilterSubString(const std::string& string);

	/*virtual*/ bool isActionEnabled(const LLSD& userdata);

	/*virtual*/ void getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const;

	boost::signals2::connection setSelectionChangeCallback(commit_callback_t cb);

	bool hasItemSelected();

private:
	void onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y);

	LLInventoryCategoriesObserver* 	mCategoriesObserver;
	LLWearableItemsList* 			mCOFItemsList;
	LLWearingGearMenu*				mGearMenu;
	LLListContextMenu*				mContextMenu;

	bool							mIsInitialized;
};

#endif //LL_LLPANELWEARING_H
