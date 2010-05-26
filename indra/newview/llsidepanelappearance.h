/** 
 * @file llsidepanelappearance.h
 * @brief Side Bar "Appearance" panel
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

#ifndef LL_LLSIDEPANELAPPEARANCE_H
#define LL_LLSIDEPANELAPPEARANCE_H

#include "llpanel.h"
#include "llinventoryobserver.h"

#include "llinventory.h"
#include "llpaneloutfitedit.h"

class LLFilterEditor;
class LLCurrentlyWornFetchObserver;
class LLWatchForOutfitRenameObserver;
class LLPanelEditWearable;
class LLWearable;
class LLPanelOutfitsInventory;

class LLSidepanelAppearance : public LLPanel
{
	LOG_CLASS(LLSidepanelAppearance);
public:
	LLSidepanelAppearance();
	virtual ~LLSidepanelAppearance();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	void refreshCurrentOutfitName(const std::string& name = "");

	static void editWearable(LLWearable *wearable, LLView *data);

	void fetchInventory();
	void inventoryFetched();
	void onNewOutfitButtonClicked();

	void showOutfitsInventoryPanel();
	void showOutfitEditPanel();
	void showWearableEditPanel(LLWearable *wearable = NULL);
	void setWearablesLoading(bool val);
	void showDefaultSubpart();
	void updateScrollingPanelList();

private:
	void onFilterEdit(const std::string& search_string);

	void onOpenOutfitButtonClicked();
	void onEditAppearanceButtonClicked();

	void togglMyOutfitsPanel(BOOL visible);
	void toggleOutfitEditPanel(BOOL visible, BOOL disable_camera_switch = FALSE);
	void toggleWearableEditPanel(BOOL visible, LLWearable* wearable = NULL, BOOL disable_camera_switch = FALSE);

	LLFilterEditor*			mFilterEditor;
	LLPanelOutfitsInventory* mPanelOutfitsInventory;
	LLPanelOutfitEdit*		mOutfitEdit;
	LLPanelEditWearable*	mEditWearable;

	LLButton*					mOpenOutfitBtn;
	LLButton*					mEditAppearanceBtn;
	LLButton*					mNewOutfitBtn;
	LLPanel*					mCurrOutfitPanel;

	LLTextBox*					mCurrentLookName;
	LLTextBox*					mOutfitStatus;

	// Used to make sure the user's inventory is in memory.
	LLCurrentlyWornFetchObserver* mFetchWorn;

	// Used to update title when currently worn outfit gets renamed.
	LLWatchForOutfitRenameObserver* mOutfitRenameWatcher;

	// Search string for filtering landmarks and teleport
	// history locations
	std::string					mFilterSubString;

	// Gets set to true when we're opened for the first time.
	bool mOpened;
};

#endif //LL_LLSIDEPANELAPPEARANCE_H
