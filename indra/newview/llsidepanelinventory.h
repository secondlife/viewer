/** 
 * @file LLSidepanelInventory.h
 * @brief Side Bar "Inventory" panel
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

#ifndef LL_LLSIDEPANELINVENTORY_H
#define LL_LLSIDEPANELINVENTORY_H

#include "llpanel.h"

class LLFolderViewItem;
class LLInventoryItem;
class LLInventoryPanel;
class LLPanelMainInventory;
class LLSidepanelItemInfo;
class LLSidepanelTaskInfo;

class LLSidepanelInventory : public LLPanel
{
public:
	LLSidepanelInventory();
	virtual ~LLSidepanelInventory();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	LLInventoryPanel* getActivePanel(); // Returns an active inventory panel, if any.
	LLPanelMainInventory* getMainInventoryPanel() const { return mPanelMainInventory; }
	BOOL isMainInventoryPanelActive() const;

	void showItemInfoPanel();
	void showTaskInfoPanel();
	void showInventoryPanel();

	// checks can share selected item(s)
	bool canShare();

protected:
	// Tracks highlighted (selected) item in inventory panel.
	LLInventoryItem *getSelectedItem();
	U32 getSelectedCount();
	void onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	// "wear", "teleport", etc.
	void performActionOnSelection(const std::string &action);
	void updateVerbs();

	bool canWearSelected(); // check whether selected items can be worn

	//
	// UI Elements
	//
private:
	LLPanel*					mInventoryPanel; // Main inventory view
	LLSidepanelItemInfo*		mItemPanel; // Individual item view
	LLSidepanelTaskInfo*		mTaskPanel; // Individual in-world object view
	LLPanelMainInventory*		mPanelMainInventory;

protected:
	void 						onInfoButtonClicked();
	void 						onShareButtonClicked();
	void 						onShopButtonClicked();
	void 						onWearButtonClicked();
	void 						onPlayButtonClicked();
	void 						onTeleportButtonClicked();
	void 						onOverflowButtonClicked();
	void 						onBackButtonClicked();
private:
	LLButton*					mInfoBtn;
	LLButton*					mShareBtn;
	LLButton*					mWearBtn;
	LLButton*					mPlayBtn;
	LLButton*					mTeleportBtn;
	LLButton*					mOverflowBtn;
	LLButton*					mShopBtn;

};

#endif //LL_LLSIDEPANELINVENTORY_H
