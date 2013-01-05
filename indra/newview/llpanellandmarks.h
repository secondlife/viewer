/**
 * @file llpanellandmarks.h
 * @brief Landmarks tab for Side Bar "Places" panel
 * class definition
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

#ifndef LL_LLPANELLANDMARKS_H
#define LL_LLPANELLANDMARKS_H

#include "lllandmark.h"

// newview
#include "llinventorymodel.h"
#include "lllandmarklist.h"
#include "llpanelplacestab.h"
#include "llpanelpick.h"
#include "llremoteparcelrequest.h"

class LLAccordionCtrlTab;
class LLFolderViewItem;
class LLMenuButton;
class LLMenuGL;
class LLToggleableMenu;
class LLInventoryPanel;
class LLPlacesInventoryPanel;

class LLLandmarksPanel : public LLPanelPlacesTab, LLRemoteParcelInfoObserver
{
public:
	LLLandmarksPanel();
	virtual ~LLLandmarksPanel();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onSearchEdit(const std::string& string);
	/*virtual*/ void onShowOnMap();
	/*virtual*/ void onShowProfile();
	/*virtual*/ void onTeleport();
	/*virtual*/ void updateVerbs();
	/*virtual*/ bool isSingleItemSelected();

	void onSelectionChange(LLPlacesInventoryPanel* inventory_list, const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	void onSelectorButtonClicked();
	void setCurrentSelectedList(LLPlacesInventoryPanel* inventory_list)
	{
		mCurrentSelectedList = inventory_list;
	}

	/**
	 * 	Update filter ShowFolderState setting to show empty folder message
	 *  if Landmarks inventory folder is empty.
	 */
	void updateShowFolderState();

	/**
	 * Selects item with "obj_id" in one of accordion tabs.
	 */
	void setItemSelected(const LLUUID& obj_id, BOOL take_keyboard_focus);

	LLPlacesInventoryPanel* getLibraryInventoryPanel() { return mLibraryInventoryPanel; }

protected:
	/**
	 * @return true - if current selected panel is not null and selected item is a landmark
	 */
	bool isLandmarkSelected() const;
	bool isFolderSelected() const;
	bool isReceivedFolderSelected() const;
	void doActionOnCurSelectedLandmark(LLLandmarkList::loaded_callback_t cb);
	LLFolderViewItem* getCurSelectedItem() const;

	/**
	 * Selects item with "obj_id" in "inventory_list" and scrolls accordion
	 * scrollbar to show the item.
	 * Returns pointer to the item if it is found in "inventory_list", otherwise NULL.
	 */
	LLFolderViewItem* selectItemInAccordionTab(LLPlacesInventoryPanel* inventory_list,
											   const std::string& tab_name,
											   const LLUUID& obj_id,
											   BOOL take_keyboard_focus) const;

	void updateSortOrder(LLInventoryPanel* panel, bool byDate);

	//LLRemoteParcelInfoObserver interface
	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);
	/*virtual*/ void setParcelID(const LLUUID& parcel_id);
	/*virtual*/ void setErrorStatus(U32 status, const std::string& reason);
	
private:
	void initFavoritesInventoryPanel();
	void initLandmarksInventoryPanel();
	void initMyInventoryPanel();
	void initLibraryInventoryPanel();
	void initLandmarksPanel(LLPlacesInventoryPanel* inventory_list);
	LLAccordionCtrlTab* initAccordion(const std::string& accordion_tab_name, LLPlacesInventoryPanel* inventory_list, bool expand_tab);
	void onAccordionExpandedCollapsed(const LLSD& param, LLPlacesInventoryPanel* inventory_list);
	void deselectOtherThan(const LLPlacesInventoryPanel* inventory_list);

	// List Commands Handlers
	void initListCommandsHandlers();
	void updateListCommands();
	void onActionsButtonClick();
	void showActionMenu(LLMenuGL* menu, std::string spawning_view_name);
	void onTrashButtonClick() const;
	void onAddAction(const LLSD& command_name) const;
	void onClipboardAction(const LLSD& command_name) const;
	void onFoldingAction(const LLSD& command_name);
	bool isActionChecked(const LLSD& userdata) const;
	bool isActionEnabled(const LLSD& command_name) const;
	void onCustomAction(const LLSD& command_name);

	/**
	 * Updates context menu depending on the selected items location.
	 *
	 * For items in Trash category the menu includes the "Restore Item"
	 * context menu entry.
	 */
	void onMenuVisibilityChange(LLUICtrl* ctrl, const LLSD& param);

	/**
	 * Determines if an item can be modified via context/gear menu.
	 *
	 * It validates Places Landmarks rules first. And then LLFolderView permissions.
	 * For now it checks cut/rename/delete/paste actions.
	 */
	bool canItemBeModified(const std::string& command_name, LLFolderViewItem* item) const;
	void onPickPanelExit( LLPanelPickEdit* pick_panel, LLView* owner, const LLSD& params);

	/**
	 * Processes drag-n-drop of the Landmarks and folders into trash button.
	 */
	bool handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept);

	/**
	 * Landmark actions callbacks. Fire when a landmark is loaded from the list.
	 */
	void doShowOnMap(LLLandmark* landmark);
	void doProcessParcelInfo(LLLandmark* landmark,
							 LLFolderViewItem* cur_item,
							 LLInventoryItem* inv_item,
							 const LLParcelData& parcel_data);
	void doCreatePick(LLLandmark* landmark);

private:
	LLPlacesInventoryPanel*		mFavoritesInventoryPanel;
	LLPlacesInventoryPanel*		mLandmarksInventoryPanel;
	LLPlacesInventoryPanel*		mMyInventoryPanel;
	LLPlacesInventoryPanel*		mLibraryInventoryPanel;
	LLMenuButton*				mGearButton;
	LLToggleableMenu*			mGearLandmarkMenu;
	LLToggleableMenu*			mGearFolderMenu;
	LLMenuGL*					mMenuAdd;
	LLPlacesInventoryPanel*		mCurrentSelectedList;
	LLInventoryObserver*		mInventoryObserver;

	LLPanel*					mListCommands;
	
	typedef	std::vector<LLAccordionCtrlTab*> accordion_tabs_t;
	accordion_tabs_t			mAccordionTabs;

	LLAccordionCtrlTab*			mMyLandmarksAccordionTab;
};

#endif //LL_LLPANELLANDMARKS_H
