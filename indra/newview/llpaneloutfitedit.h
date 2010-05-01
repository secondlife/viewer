/** 
 * @file llpaneloutfitedit.h
 * @brief Displays outfit edit information in Side Tray.
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

#ifndef LL_LLPANELOUTFITEDIT_H
#define LL_LLPANELOUTFITEDIT_H

#include "llpanel.h"

#include "v3dmath.h"
#include "lluuid.h"

#include "lliconctrl.h"

#include "llremoteparcelrequest.h"
#include "llinventory.h"
#include "llinventorymodel.h"

class LLButton;
class LLCOFWearables;
class LLTextBox;
class LLInventoryCategory;
class LLInventoryLookObserver;
class LLInventoryPanel;
class LLSaveFolderState;
class LLFolderViewItem;
class LLScrollListCtrl;
class LLToggleableMenu;
class LLLookFetchObserver;
class LLFilterEditor;
class LLFilteredWearableListManager;

class LLPanelOutfitEdit : public LLPanel
{
public:
	
	// NOTE: initialize mLookItemTypes at the index of any new enum you add in the LLPanelOutfitEdit() constructor
	typedef enum e_look_item_type
	{
		LIT_ALL = 0,
		LIT_WEARABLE, // clothing or shape
		LIT_ATTACHMENT,
		NUM_LOOK_ITEM_TYPES
	} ELookItemType; 
	
	struct LLLookItemType {
		std::string displayName;
		U64 inventoryMask;
		LLLookItemType() : displayName("NONE"), inventoryMask(0) {}
		LLLookItemType(std::string name, U64 mask) : displayName(name), inventoryMask(mask) {}
	};
	
	LLPanelOutfitEdit();
	/*virtual*/ ~LLPanelOutfitEdit();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void changed(U32 mask);

	/*virtual*/ void setParcelID(const LLUUID& parcel_id);
		// Sends a request for data about the given parcel, which will
		// only update the location if there is none already available.

	void moveWearable(bool closer_to_body);

	void toggleAddWearablesPanel();
	void showWearablesFilter();
	void showFilteredWearablesPanel();
	void saveOutfit(bool as_new = false);
	void showSaveMenu();

	void onTypeFilterChanged(LLUICtrl* ctrl);
	void onSearchEdit(const std::string& string);
	void onInventorySelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	void onAddToOutfitClicked(void);
	void onOutfitItemSelectionChange(void);
	void onRemoveFromOutfitClicked(void);
	void onEditWearableClicked(void);

	void displayCurrentOutfit();
	
	void lookFetched(void);
	
	void updateLookInfo(void);

private:

	void updateVerbs();

	//*TODO got rid of mCurrentOutfitID
	LLUUID				mCurrentOutfitID;

	LLTextBox*			mCurrentOutfitName;
	LLInventoryPanel*	mInventoryItemsPanel;
	LLFilterEditor*		mSearchFilter;
	LLSaveFolderState*	mSavedFolderState;
	std::string			mSearchString;
	LLButton*			mEditWearableBtn;
	LLToggleableMenu*	mSaveMenu;

	LLFilteredWearableListManager* mWearableListManager;

	LLLookFetchObserver*		mFetchLook;
	LLInventoryLookObserver*	mLookObserver;
	std::vector<LLLookItemType> mLookItemTypes;

	LLCOFWearables*		mCOFWearables;
};

#endif // LL_LLPANELOUTFITEDIT_H
