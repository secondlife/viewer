/**
 * @file llpanellandmarks.h
 * @brief Landmarks tab for Side Bar "Places" panel
 * class definition
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
class LLMenuGL;
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
	/*virtual*/ void onTeleport();
	/*virtual*/ void updateVerbs();

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
	 * Determines if selected item can be modified via context/gear menu.
	 *
	 * It validates Places Landmarks rules first. And then LLFolderView permissions.
	 * For now it checks cut/rename/delete/paste actions.
	 */
	bool canSelectedBeModified(const std::string& command_name) const;
	void onPickPanelExit( LLPanelPickEdit* pick_panel, LLView* owner, const LLSD& params);

	/**
	 * Processes drag-n-drop of the Landmarks and folders into trash button.
	 */
	bool handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, EAcceptance* accept);

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
	LLMenuGL*					mGearLandmarkMenu;
	LLMenuGL*					mGearFolderMenu;
	LLMenuGL*					mMenuAdd;
	LLPlacesInventoryPanel*		mCurrentSelectedList;
	LLInventoryObserver*		mInventoryObserver;

	LLPanel*					mListCommands;
	bool 						mSortByDate;
	
	typedef	std::vector<LLAccordionCtrlTab*> accordion_tabs_t;
	accordion_tabs_t			mAccordionTabs;

	LLAccordionCtrlTab*			mMyLandmarksAccordionTab;
};

#endif //LL_LLPANELLANDMARKS_H
