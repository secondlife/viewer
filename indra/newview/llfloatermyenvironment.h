/** 
 * @file llfloatermyenvironment.h
 * @brief Read-only list of gestures from your inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

/**
 * (Also has legacy gesture editor for testing.)
 */

#ifndef LL_LLFLOATERMYENVIRONMENT_H
#define LL_LLFLOATERMYENVIRONMENT_H
#include <vector> 

#include "llfloater.h"
#include "llinventoryobserver.h"
#include "llinventoryfilter.h"

class LLView;
class LLButton;
class LLLineEditor;
class LLComboBox;
class LLViewerGesture;
class LLGestureOptions;
class LLMenuGL;
class LLInventoryPanel;

class LLFloaterMyEnvironment
:	public LLFloater, LLInventoryFetchDescendentsObserver
{
    LOG_CLASS(LLFloaterMyEnvironment);
public:
                                    LLFloaterMyEnvironment(const LLSD& key);
    virtual                         ~LLFloaterMyEnvironment();

    virtual BOOL                    postBuild() override;
    virtual void                    refresh() override;

    virtual void                    onOpen(const LLSD& key) override;

private:
    LLInventoryPanel *              mInventoryList;
    U64                             mTypeFilter;
    LLInventoryFilter::EFolderShow  mShowFolders;
    LLUUID                          mSelectedAsset;

    void                            onShowFoldersChange();
    void                            onFilterCheckChange();
    void                            onSelectionChange();
    void                            onDeleteSelected();
    void                            onDoCreate(const LLSD &data);
    void                            onDoApply(const std::string &context);
    bool                            canAction(const std::string &context);
    bool                            canApply(const std::string &context);

    void                            getSelectedIds(uuid_vec_t& ids) const;
    void                            refreshButtonStates();

    bool                            isSettingSelected(LLUUID item_id);

    static LLUUID                   findItemByAssetId(LLUUID asset_id, bool copyable_only, bool ignore_library);

#if 0
	virtual void done ();
	void        refreshAll();
	/**
	 * @brief Add new scrolllistitem into gesture_list.
	 * @param  item_id inventory id of gesture
	 * @param  gesture can be NULL , if item was not loaded yet
	 */
	void addGesture(const LLUUID& item_id, LLMultiGesture* gesture, LLCtrlListInterface * list);

protected:
	// Reads from the gesture manager's list of active gestures
	// and puts them in this list.
	void buildGestureList();
	void playGesture(LLUUID item_id);
private:
	void addToCurrentOutFit();
	/**
	 * @brief  This method is using to collect selected items. 
	 * In some places gesture_list can be rebuilt by gestureObservers during  iterating data from LLScrollListCtrl::getAllSelected().
	 * Therefore we have to copy these items to avoid viewer crash.
	 * @see LLFloaterGesture::onActivateBtnClick
	 */
	void getSelectedIds(uuid_vec_t& ids);
	bool isActionEnabled(const LLSD& command);
	/**
	 * @brief Activation rules:
	 *  According to Gesture Spec:
	 *  1. If all selected gestures are active: set to inactive
	 *  2. If all selected gestures are inactive: set to active
	 *  3. If selected gestures are in a mixed state: set all to active
	 */
	void onActivateBtnClick();
	void onClickEdit();
	void onClickPlay();
	void onClickNew();
	void onCommitList();
	void onCopyPasteAction(const LLSD& command);
	void onDeleteSelected();

	LLUUID mSelectedID;
	LLUUID mGestureFolderID;
	LLScrollListCtrl* mGestureList;

	LLFloaterGestureObserver* mObserver;
#endif
};


#endif
