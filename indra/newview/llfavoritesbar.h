/** 
 * @file llfavoritesbar.h
 * @brief LLFavoritesBarCtrl base class
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

#ifndef LL_LLFAVORITESBARCTRL_H
#define LL_LLFAVORITESBARCTRL_H

#include "llbutton.h"
#include "lluictrl.h"
#include "lltextbox.h"

#include "llinventoryobserver.h"
#include "llinventorymodel.h"

class LLFavoritesBarCtrl : public LLUICtrl, public LLInventoryObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUIImage*> image_drag_indication;
		Optional<LLButton::Params> chevron_button;
		Optional<LLTextBox::Params> label;
		Params();
	};

protected:
	LLFavoritesBarCtrl(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLFavoritesBarCtrl();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);

	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	// LLInventoryObserver observer trigger
	virtual void changed(U32 mask);
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void draw();

	void showDragMarker(BOOL show) { mShowDragMarker = show; }
	void setLandingTab(LLUICtrl* tab) { mLandingTab = tab; }

protected:
	void updateButtons();
	LLButton* createButton(const LLPointer<LLViewerInventoryItem> item, LLXMLNodePtr &root, S32 x_offset );
	LLXMLNodePtr getButtonXMLNode();
	BOOL collectFavoriteItems(LLInventoryModel::item_array_t &items);

	void onButtonClick(LLUUID id);
	void onButtonRightClick(LLUUID id,LLView* button,S32 x,S32 y,MASK mask);
	
	void onButtonMouseDown(LLUUID id, LLUICtrl* button, S32 x, S32 y, MASK mask);
	void onOverflowMenuItemMouseDown(LLUUID id, LLUICtrl* item, S32 x, S32 y, MASK mask);
	void onButtonMouseUp(LLUUID id, LLUICtrl* button, S32 x, S32 y, MASK mask);

	void onEndDrag();

	bool enableSelected(const LLSD& userdata);
	void doToSelected(const LLSD& userdata);
	BOOL isClipboardPasteable() const;
	void pastFromClipboard() const;
	
	void showDropDownMenu();

	LLHandle<LLView> mPopupMenuHandle;
	LLHandle<LLView> mInventoryItemsPopupMenuHandle;

	LLUUID mFavoriteFolderId;
	const LLFontGL *mFont;
	S32 mFirstDropDownItem;
	bool mUpdateDropDownItems;

	LLUUID mSelectedItemID;

	LLUIImage* mImageDragIndication;

private:
	/*
	 * Helper function to make code more readable. It handles all drag and drop
	 * operations of the existing favorites items on the favorites bar.
	 */
	void handleExistingFavoriteDragAndDrop(S32 x, S32 y);

	/*
	 * Helper function to make code more readable. It handles all drag and drop
	 * operations of the new landmark to the favorites bar.
	 */
	void handleNewFavoriteDragAndDrop(LLInventoryItem *item, const LLUUID& favorites_id, S32 x, S32 y);

	// finds a control under the specified LOCAL point
	LLUICtrl* findChildByLocalCoords(S32 x, S32 y);

	// checks if the current order of the favorites items must be saved
	BOOL needToSaveItemsOrder(const LLInventoryModel::item_array_t& items);

	/**
	 * inserts an item identified by insertedItemId BEFORE an item identified by beforeItemId.
	 * this function assumes that an item identified by insertedItemId doesn't exist in items array.
	 */
	void insertBeforeItem(LLInventoryModel::item_array_t& items, const LLUUID& beforeItemId, LLViewerInventoryItem* insertedItem);

	// finds an item by it's UUID in the items array
	LLInventoryModel::item_array_t::iterator findItemByUUID(LLInventoryModel::item_array_t& items, const LLUUID& id);

	BOOL mShowDragMarker;
	LLUICtrl* mLandingTab;
	LLUICtrl* mLastTab;
	LLButton* mChevronButton;
	LLTextBox* mBarLabel;

	LLUUID mDragItemId;
	BOOL mStartDrag;
	LLInventoryModel::item_array_t mItems;

	BOOL mTabsHighlightEnabled;

	boost::signals2::connection mEndDragConnection;
};


#endif // LL_LLFAVORITESBARCTRL_H
