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
#include "llinventoryitemslist.h"
#include "llinventorymodel.h"

class LLButton;
class LLCOFWearables;
class LLTextBox;
class LLInventoryCategory;
class LLCOFObserver;
class LLCOFDragAndDropObserver;
class LLInventoryPanel;
class LLSaveFolderState;
class LLFolderViewItem;
class LLScrollListCtrl;
class LLToggleableMenu;
class LLFilterEditor;
class LLFilteredWearableListManager;
class LLMenuGL;
class LLFindNonLinksByMask;
class LLFindWearablesOfType;
class LLSaveOutfitComboBtn;

class LLPanelOutfitEdit : public LLPanel
{
	LOG_CLASS(LLPanelOutfitEdit);
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
	/*virtual*/ void onOpen(const LLSD& key);

	void moveWearable(bool closer_to_body);

	void toggleAddWearablesPanel();
	void showAddWearablesPanel(bool show__add_wearables);
	void showWearablesFilter();
	void showFilteredWearablesPanel();
	void showFilteredFolderWearablesPanel();

	void onTypeFilterChanged(LLUICtrl* ctrl);
	void onSearchEdit(const std::string& string);
	void onInventorySelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	void onAddToOutfitClicked(void);
	void onOutfitItemSelectionChange(void);
	void onRemoveFromOutfitClicked(void);
	void onEditWearableClicked(void);
	void onAddWearableClicked(void);
	void onReplaceBodyPartMenuItemClicked(LLUUID selected_item_id);

	void displayCurrentOutfit();
	void updateCurrentOutfitName();

	void update();

	void updateVerbs();
	/**
	 * @brief Helper function. Shows one panel instead of another.
	 *		  If panels already switched does nothing and returns false.
	 * @param  switch_from_panel panel to hide
	 * @param  switch_to_panel panel to show
	 * @retun  returns true if switching happened, false if not.
	 */
	bool switchPanels(LLPanel* switch_from_panel, LLPanel* switch_to_panel);

	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									  EDragAndDropType cargo_type,
									  void* cargo_data,
									  EAcceptance* accept,
									  std::string& tooltip_msg);

private:

	void onGearButtonClick(LLUICtrl* clicked_button);
	void showFilteredWearableItemsList(LLWearableType::EType type);


	LLTextBox*			mCurrentOutfitName;
	LLTextBox*			mStatus;
	LLInventoryPanel*	mInventoryItemsPanel;
	LLFilterEditor*		mSearchFilter;
	LLSaveFolderState*	mSavedFolderState;
	std::string			mSearchString;
	LLButton*			mEditWearableBtn;
	LLButton*			mFolderViewBtn;
	LLButton*			mListViewBtn;
	LLPanel*			mAddWearablesPanel;

	LLFindNonLinksByMask*  mWearableListMaskCollector;
	LLFindWearablesOfType* mWearableListTypeCollector;

	LLFilteredWearableListManager* 	mWearableListManager;
	LLInventoryItemsList* 			mWearableItemsList;
	LLPanel*						mWearableItemsPanel;

	LLCOFObserver*	mCOFObserver;
	LLCOFDragAndDropObserver* mCOFDragAndDropObserver;

	std::vector<LLLookItemType> mLookItemTypes;

	LLCOFWearables*		mCOFWearables;
	LLMenuGL*			mGearMenu;
	bool				mInitialized;
	std::auto_ptr<LLSaveOutfitComboBtn> mSaveComboBtn;

};

#endif // LL_LLPANELOUTFITEDIT_H
