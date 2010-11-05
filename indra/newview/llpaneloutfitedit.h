/** 
 * @file llpaneloutfitedit.h
 * @brief Displays outfit edit information in Side Tray.
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

#ifndef LL_LLPANELOUTFITEDIT_H
#define LL_LLPANELOUTFITEDIT_H

#include "llpanel.h"

#include "v3dmath.h"
#include "lluuid.h"

#include "lliconctrl.h"

#include "llremoteparcelrequest.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llwearableitemslist.h"

class LLButton;
class LLCOFWearables;
class LLComboBox;
class LLTextBox;
class LLInventoryCategory;
class LLOutfitObserver;
class LLCOFDragAndDropObserver;
class LLInventoryPanel;
class LLSaveFolderState;
class LLFolderViewItem;
class LLScrollListCtrl;
class LLToggleableMenu;
class LLFilterEditor;
class LLFilteredWearableListManager;
class LLMenuButton;
class LLMenuGL;
class LLFindNonLinksByMask;
class LLFindWearablesOfType;
class LLSaveOutfitComboBtn;
class LLWearableItemTypeNameComparator;

class LLPanelOutfitEdit : public LLPanel
{
	LOG_CLASS(LLPanelOutfitEdit);
public:
	
	// NOTE: initialize mFolderViewItemTypes at the index of any new enum you add in the LLPanelOutfitEdit() constructor
	typedef enum e_folder_view_item_type
	{
		FVIT_ALL = 0,
		FVIT_WEARABLE, // clothing or shape
		FVIT_ATTACHMENT,
		NUM_FOLDER_VIEW_ITEM_TYPES
	} EFolderViewItemType; 
	
	//should reflect order from LLWearableType::EType
	typedef enum e_list_view_item_type
	{
		LVIT_ALL = 0,
		LVIT_CLOTHING,
		LVIT_BODYPART,
		LVIT_ATTACHMENT,
		LVIT_SHAPE,
		LVIT_SKIN,
		LVIT_HAIR,
		LVIT_EYES,
		LVIT_SHIRT,
		LVIT_PANTS,
		LVIT_SHOES,
		LVIT_SOCKS,
		LVIT_JACKET,
		LVIT_GLOVES,
		LVIT_UNDERSHIRT,
		LVIT_UNDERPANTS,
		LVIT_SKIRT,
		LVIT_ALPHA,
		LVIT_TATTOO,
		NUM_LIST_VIEW_ITEM_TYPES
	} EListViewItemType; 

	struct LLLookItemType {
		std::string displayName;
		U64 inventoryMask;
		LLLookItemType() : displayName("NONE"), inventoryMask(0) {}
		LLLookItemType(std::string name, U64 mask) : displayName(name), inventoryMask(mask) {}
	};

	struct LLFilterItem {
		std::string displayName;
		LLInventoryCollectFunctor* collector;
		LLFilterItem() : displayName("NONE"), collector(NULL) {}
		LLFilterItem(std::string name, LLInventoryCollectFunctor* _collector) : displayName(name), collector(_collector) {}
		~LLFilterItem() { delete collector; }

	//the struct is not supposed to by copied, either way the destructor kills collector
	//LLPointer is not used as it requires LLInventoryCollectFunctor to extend LLRefCount what it doesn't do
	private:
		LLFilterItem(const LLFilterItem& filter_item) {};
	};
	
	LLPanelOutfitEdit();
	/*virtual*/ ~LLPanelOutfitEdit();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	void moveWearable(bool closer_to_body);

	void toggleAddWearablesPanel();
	void showAddWearablesPanel(bool show__add_wearables);

	//following methods operate with "add wearables" panel
	void showWearablesFilter();
	void showWearablesListView();
	void showWearablesFolderView();

	void updateFiltersVisibility();

	void onFolderViewFilterCommitted(LLUICtrl* ctrl);
	void onListViewFilterCommitted(LLUICtrl* ctrl);
	void onSearchEdit(const std::string& string);
	void updatePlusButton();
	void onPlusBtnClicked(void);

	void onVisibilityChange(const LLSD &in_visible_chain);

	void applyFolderViewFilter(EFolderViewItemType type);
	void applyListViewFilter(EListViewItemType type);

	/**
	 * Filter items in views of Add Wearables Panel and show appropriate view depending on currently selected COF item(s)
	 * No COF items selected - shows the folder view, reset filter
	 * 1 COF item selected - shows the list view and filters wearables there by a wearable type of the selected item
	 * More than 1 COF item selected - shows the list view and filters it by a type of the selected item (attachment or clothing)
	 */
	void filterWearablesBySelectedItem(void);

	void onRemoveFromOutfitClicked(void);
	void onEditWearableClicked(void);
	void onAddWearableClicked(void);
	void onReplaceMenuItemClicked(LLUUID selected_item_id);
	void onShopButtonClicked();

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

	void resetAccordionState();

	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									  EDragAndDropType cargo_type,
									  void* cargo_data,
									  EAcceptance* accept,
									  std::string& tooltip_msg);

private:
	void onAddMoreButtonClicked();
	void showFilteredWearablesListView(LLWearableType::EType type);
	void onOutfitChanging(bool started);
	void getSelectedItemsUUID(uuid_vec_t& uuid_list);
	void getCurrentItemUUID(LLUUID& selected_id);
	void onCOFChanged();

	/**
	 * Method preserves selection while switching between folder/list view modes
	*/
	void saveListSelection();

	void updateWearablesPanelVerbButtons();

	typedef std::pair<LLWearableType::EType, size_t> selection_info_t;

	LLWearableType::EType getCOFWearablesSelectionType() const;
	selection_info_t getAddMorePanelSelectionType() const;
	LLWearableType::EType getWearableTypeByItemUUID(const LLUUID& item_uuid) const;

	LLTextBox*			mCurrentOutfitName;
	LLTextBox*			mStatus;
	LLInventoryPanel*	mInventoryItemsPanel;
	LLFilterEditor*		mSearchFilter;
	LLSaveFolderState*	mSavedFolderState;
	std::string			mSearchString;
	LLButton*			mEditWearableBtn;
	LLButton*			mFolderViewBtn;
	LLButton*			mListViewBtn;
	LLButton*			mPlusBtn;
	LLPanel*			mAddWearablesPanel;
	
	LLComboBox*			mFolderViewFilterCmbBox;
	LLComboBox*			mListViewFilterCmbBox;

	LLFilteredWearableListManager* 	mWearableListManager;
	LLWearableItemsList* 			mWearableItemsList;
	LLPanel*						mWearablesListViewPanel;
	LLWearableItemTypeNameComparator* mWearableListViewItemsComparator;

	LLCOFDragAndDropObserver* mCOFDragAndDropObserver;

	std::vector<LLLookItemType> mFolderViewItemTypes;
	std::vector<LLFilterItem*> mListViewItemTypes;

	LLCOFWearables*		mCOFWearables;
	LLToggleableMenu*	mGearMenu;
	LLToggleableMenu*	mAddWearablesGearMenu;
	bool				mInitialized;
	std::auto_ptr<LLSaveOutfitComboBtn> mSaveComboBtn;
	LLMenuButton*		mWearablesGearMenuBtn;
	LLMenuButton*		mGearMenuBtn;

};

#endif // LL_LLPANELOUTFITEDIT_H
