/** 
 * @file llcofwearables.h
 * @brief LLCOFWearables displayes wearables from the current outfit split into three lists (attachments, clothing and body parts)
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLCOFWEARABLES_H
#define LL_LLCOFWEARABLES_H

// llui
#include "llflatlistview.h"
#include "llpanel.h"

#include "llappearancemgr.h"
#include "llinventorymodel.h"

class LLAccordionCtrl;
class LLAccordionCtrlTab;
class LLListContextMenu;
class LLPanelClothingListItem;
class LLPanelBodyPartsListItem;
class LLPanelDeletableWearableListItem;

class LLCOFWearables : public LLPanel
{
public:

	/**
	 * Represents a collection of callbacks assigned to inventory panel item's buttons
	 */
	class LLCOFCallbacks
	{
	public:
		LLCOFCallbacks() {};
		virtual ~LLCOFCallbacks() {};
		
		typedef boost::function<void ()> cof_callback_t;

		cof_callback_t mAddWearable;
		cof_callback_t mMoveWearableCloser;
		cof_callback_t mMoveWearableFurther;
		cof_callback_t mEditWearable;
		cof_callback_t mDeleteWearable;
	};



	LLCOFWearables();
	virtual ~LLCOFWearables();

	/*virtual*/ BOOL postBuild();
	
	LLUUID getSelectedUUID();
	bool getSelectedUUIDs(uuid_vec_t& selected_ids);

	LLPanel* getSelectedItem();
	void getSelectedItems(std::vector<LLPanel*>& selected_items) const;

	/* Repopulate the COF wearables list if the COF category has been changed since the last refresh */
	void refresh();
	void clear();

	LLAssetType::EType getExpandedAccordionAssetType();
	LLAssetType::EType getSelectedAccordionAssetType();
	void expandDefaultAccordionTab();

	LLCOFCallbacks& getCOFCallbacks() { return mCOFCallbacks; }

	/**
	 * Selects first clothing item with specified LLWearableType::EType from clothing list
	 */
	void selectClothing(LLWearableType::EType clothing_type);

protected:

	void populateAttachmentsAndBodypartsLists(const LLInventoryModel::item_array_t& cof_items);
	void populateClothingList(LLAppearanceMgr::wearables_by_type_t& clothing_by_type);
	
	void addClothingTypesDummies(const LLAppearanceMgr::wearables_by_type_t& clothing_by_type);
	void onSelectionChange(LLFlatListView* selected_list);
	void onAccordionTabStateChanged(LLUICtrl* ctrl, const LLSD& expanded);

	LLPanelClothingListItem* buildClothingListItem(LLViewerInventoryItem* item, bool first, bool last);
	LLPanelBodyPartsListItem* buildBodypartListItem(LLViewerInventoryItem* item);
	LLPanelDeletableWearableListItem* buildAttachemntListItem(LLViewerInventoryItem* item);

	void onListRightClick(LLUICtrl* ctrl, S32 x, S32 y, LLListContextMenu* menu);

	LLFlatListViewEx* mAttachments;
	LLFlatListView* mClothing;
	LLFlatListView* mBodyParts;

	LLFlatListView* mLastSelectedList;

	LLAccordionCtrlTab* mClothingTab;
	LLAccordionCtrlTab* mAttachmentsTab;
	LLAccordionCtrlTab* mBodyPartsTab;

	LLAccordionCtrlTab* mLastSelectedTab;

	std::map<const LLAccordionCtrlTab*, LLAssetType::EType> mTab2AssetType;

	LLCOFCallbacks mCOFCallbacks;

	LLListContextMenu* mClothingMenu;
	LLListContextMenu* mAttachmentMenu;
	LLListContextMenu* mBodyPartMenu;

	LLAccordionCtrl*	mAccordionCtrl;

	/* COF category version since last refresh */
	S32 mCOFVersion;
};


#endif
