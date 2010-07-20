/** 
 * @file llcofwearables.h
 * @brief LLCOFWearables displayes wearables from the current outfit split into three lists (attachments, clothing and body parts)
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#ifndef LL_LLCOFWEARABLES_H
#define LL_LLCOFWEARABLES_H

// llui
#include "llflatlistview.h"
#include "llpanel.h"

#include "llappearancemgr.h"
#include "llinventorymodel.h"

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
		
		typedef boost::function<void (void*)> cof_callback_t;

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

	LLCOFCallbacks& getCOFCallbacks() { return mCOFCallbacks; }

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

	LLFlatListView* mAttachments;
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

	/* COF category version since last refresh */
	S32 mCOFVersion;
};


#endif
