/** 
 * @file LLSidepanelInventory.h
 * @brief Side Bar "Inventory" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
	BOOL isMainInventoryPanelActive() const;

protected:
	// Tracks highlighted (selected) item in inventory panel.
	LLInventoryItem *getSelectedItem();
	U32 getSelectedCount();
	void onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	// "wear", "teleport", etc.
	void performActionOnSelection(const std::string &action);
	bool canShare();

	void showItemInfoPanel();
	void showTaskInfoPanel();
	void showInventoryPanel();
	void updateVerbs();

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

};

#endif //LL_LLSIDEPANELINVENTORY_H
