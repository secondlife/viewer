/** 
 * @file llplacesinventorypanel.h
 * @brief LLPlacesInventoryPanel class declaration
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

#ifndef LL_LLINVENTORYSUBTREEPANEL_H
#define LL_LLINVENTORYSUBTREEPANEL_H

#include "llfloaterinventory.h"
#include "llinventorypanel.h"
#include "llfolderview.h"

class LLLandmarksPanel;

class LLPlacesInventoryPanel : public LLInventoryPanel
{
public:
	struct Params 
		:	public LLInitParam::Block<Params, LLInventoryPanel::Params>
	{
		Params()
		{}
	};

	LLPlacesInventoryPanel(const Params& p);
	~LLPlacesInventoryPanel();

	/*virtual*/ BOOL postBuild();

	void saveFolderState();
	void restoreFolderState();

	virtual S32	notify(const LLSD& info) ;

private:
	LLSaveFolderState*			mSavedFolderState;
};


class LLPlacesFolderView : public LLFolderView
{
public:
	LLPlacesFolderView(const LLFolderView::Params& p);
	/**
	 *	Handles right mouse down
	 *
	 * Contains workaround for EXT-2786: sets current selected list for landmark
	 * panel using @c mParentLandmarksPanel which is set in @c LLLandmarksPanel::initLandmarksPanel
	 */
	/*virtual*/ BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );

	void setupMenuHandle(LLInventoryType::EType asset_type, LLHandle<LLView> menu_handle);

	void setParentLandmarksPanel(LLLandmarksPanel* panel)
	{
		mParentLandmarksPanel = panel;
	}

	S32 getSelectedCount() { return (S32)mSelectedItems.size(); }

private:
	/**
	 * holds pointer to landmark panel. This pointer is used in @c LLPlacesFolderView::handleRightMouseDown
	 */
	LLLandmarksPanel* mParentLandmarksPanel;
	typedef std::map<LLInventoryType::EType, LLHandle<LLView> > inventory_type_menu_handle_t;
	inventory_type_menu_handle_t mMenuHandlesByInventoryType;

};

#endif //LL_LLINVENTORYSUBTREEPANEL_H
