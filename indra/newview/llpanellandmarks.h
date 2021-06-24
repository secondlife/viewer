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
class LLFolderViewModelItemInventory;

class LLLandmarksPanel : public LLPanelPlacesTab, LLRemoteParcelInfoObserver
{
public:
	LLLandmarksPanel();
	LLLandmarksPanel(bool is_landmark_panel);
	virtual ~LLLandmarksPanel();

	BOOL postBuild() override;
	void onSearchEdit(const std::string& string) override;
	void onShowOnMap() override;
	void onShowProfile() override;
	void onTeleport() override;
	void onRemoveSelected() override;
	void updateVerbs() override;
	bool isSingleItemSelected() override;

    LLToggleableMenu* getSelectionMenu() override;
    LLToggleableMenu* getSortingMenu() override;
    LLToggleableMenu* getCreateMenu() override;

    /**
     * Processes drag-n-drop of the Landmarks and folders into trash button.
     */
    bool handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept) override;

	void setCurrentSelectedList(LLPlacesInventoryPanel* inventory_list)
	{
		mCurrentSelectedList = inventory_list;
	}

	/**
	 * Selects item with "obj_id" in one of accordion tabs.
	 */
	void setItemSelected(const LLUUID& obj_id, BOOL take_keyboard_focus);

	void updateMenuVisibility(LLUICtrl* menu);

	void doCreatePick(LLLandmark* landmark, const LLUUID &item_id );

	void resetSelection();

protected:
	/**
	 * @return true - if current selected panel is not null and selected item is a landmark
	 */
	bool isLandmarkSelected() const;
	bool isFolderSelected() const;
	void doActionOnCurSelectedLandmark(LLLandmarkList::loaded_callback_t cb);
	LLFolderViewItem* getCurSelectedItem() const;
	LLFolderViewModelItemInventory* getCurSelectedViewModelItem() const;

	void updateSortOrder(LLInventoryPanel* panel, bool byDate);

	//LLRemoteParcelInfoObserver interface
	void processParcelInfo(const LLParcelData& parcel_data) override;
	void setParcelID(const LLUUID& parcel_id) override;
	void setErrorStatus(S32 status, const std::string& reason) override;

	// List Commands Handlers
	void initListCommandsHandlers();
	void initLandmarksPanel(LLPlacesInventoryPanel* inventory_list);

	LLPlacesInventoryPanel*		mCurrentSelectedList;
	
private:
	void initLandmarksInventoryPanel();

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
	 * Landmark actions callbacks. Fire when a landmark is loaded from the list.
	 */
	void doShowOnMap(LLLandmark* landmark);
	void doProcessParcelInfo(LLLandmark* landmark,
							 LLInventoryItem* inv_item,
							 const LLParcelData& parcel_data);

private:
	LLPlacesInventoryPanel*		mLandmarksInventoryPanel;
	LLToggleableMenu*			mGearLandmarkMenu;
	LLToggleableMenu*			mGearFolderMenu;
	LLToggleableMenu*			mSortingMenu;
	LLToggleableMenu*			mAddMenu;

	bool						isLandmarksPanel;
	
    LLUUID                      mCreatePickItemId; // item we requested a pick for
};


class LLFavoritesPanel : public LLLandmarksPanel
{
public:
	LLFavoritesPanel();

	BOOL postBuild() override;
	void initFavoritesInventoryPanel();
};

#endif //LL_LLPANELLANDMARKS_H
