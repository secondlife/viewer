/** 
 * @file llplacesinventorypanel.h
 * @brief LLPlacesInventoryPanel class declaration
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

private:
	LLSaveFolderState*			mSavedFolderState;
};


class LLPlacesFolderView : public LLFolderView
{
public:
	LLPlacesFolderView(const LLFolderView::Params& p) : LLFolderView(p) {};
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
