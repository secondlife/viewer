/** 
 * @file llsidepanelappearance.h
 * @brief Side Bar "Appearance" panel
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

#ifndef LL_LLSIDEPANELAPPEARANCE_H
#define LL_LLSIDEPANELAPPEARANCE_H

#include "llpanel.h"
#include "llinventoryobserver.h"

#include "llinventory.h"
#include "llpaneloutfitedit.h"

class LLFilterEditor;
class LLCurrentlyWornFetchObserver;
class LLPanelEditWearable;
class LLViewerWearable;
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

	static void editWearable(LLViewerWearable *wearable, LLView *data, BOOL disable_camera_switch = FALSE);

	void fetchInventory();
	void inventoryFetched();
	void onNewOutfitButtonClicked();

	void showOutfitsInventoryPanel();
	void showOutfitEditPanel();
	void showWearableEditPanel(LLViewerWearable *wearable = NULL, BOOL disable_camera_switch = FALSE);
	void setWearablesLoading(bool val);
	void showDefaultSubpart();
	void updateScrollingPanelList();
	void updateToVisibility( const LLSD& new_visibility );

private:
	void onFilterEdit(const std::string& search_string);
	void onVisibilityChange ( const LLSD& new_visibility );

	void onOpenOutfitButtonClicked();
	void onEditAppearanceButtonClicked();

	void toggleMyOutfitsPanel(BOOL visible);
	void toggleOutfitEditPanel(BOOL visible, BOOL disable_camera_switch = FALSE);
	void toggleWearableEditPanel(BOOL visible, LLViewerWearable* wearable = NULL, BOOL disable_camera_switch = FALSE);

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

	// Search string for filtering landmarks and teleport
	// history locations
	std::string					mFilterSubString;

	// Gets set to true when we're opened for the first time.
	bool mOpened;
};

#endif //LL_LLSIDEPANELAPPEARANCE_H
