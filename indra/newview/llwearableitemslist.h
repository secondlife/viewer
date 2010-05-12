/**
 * @file llwearableitemslist.h
 * @brief A flat list of wearable items.
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

#ifndef LL_LLWEARABLEITEMSLIST_H
#define LL_LLWEARABLEITEMSLIST_H

#include "llpanel.h"

// newview
#include "llinventoryitemslist.h"
#include "llinventorymodel.h"
#include "llwearabletype.h"

/**
 * @class LLPanelWearableListItem
 *
 * Extends LLPanelInventoryListItemBase:
 * - makes side widgets show on mouse_enter and hide on 
 *   mouse_leave events.
 * - provides callback for button clicks
 */
class LLPanelWearableListItem : public LLPanelInventoryListItemBase
{
	LOG_CLASS(LLPanelWearableListItem);
public:

	/**
	* Shows buttons when mouse is over
	*/
	/*virtual*/ void onMouseEnter(S32 x, S32 y, MASK mask);

	/**
	* Hides buttons when mouse is out
	*/
	/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);

protected:

	LLPanelWearableListItem(LLViewerInventoryItem* item);
};

/**
 * @class LLPanelClothingListItem
 *
 * Provides buttons for editing, moving, deleting a wearable.
 */
class LLPanelClothingListItem : public LLPanelWearableListItem
{
	LOG_CLASS(LLPanelClothingListItem);
public:

	static LLPanelClothingListItem* create(LLViewerInventoryItem* item);

	virtual ~LLPanelClothingListItem();

	/*virtual*/ BOOL postBuild();

	/**
	 * Make button visible during mouse over event.
	 */
	inline void setShowDeleteButton(bool show) { setShowWidget("btn_delete", show); }
	inline void setShowMoveUpButton(bool show) { setShowWidget("btn_move_up", show); }

	inline void setShowMoveDownButton(bool show) { setShowWidget("btn_move_down", show); }
	inline void setShowLockButton(bool show) { setShowWidget("btn_lock", show); }
	inline void setShowEditButton(bool show) { setShowWidget("btn_edit", show); }


protected:

	LLPanelClothingListItem(LLViewerInventoryItem* item);
	
	/*virtual*/ void init();
};

class LLPanelBodyPartsListItem : public LLPanelWearableListItem
{
	LOG_CLASS(LLPanelBodyPartsListItem);
public:

	static LLPanelBodyPartsListItem* create(LLViewerInventoryItem* item);

	virtual ~LLPanelBodyPartsListItem();

	/*virtual*/ BOOL postBuild();

	/**
	* Make button visible during mouse over event.
	*/
	inline void setShowLockButton(bool show) { setShowWidget("btn_lock", show); }
	inline void setShowEditButton(bool show) { setShowWidget("btn_edit", show); }

protected:
	LLPanelBodyPartsListItem(LLViewerInventoryItem* item);

	/*virtual*/ void init();
};

/**
 * @class LLPanelDummyClothingListItem
 *
 * A dummy item panel - displays grayed clothing icon, grayed title '<clothing> not worn' and 'add' button
 */
class LLPanelDummyClothingListItem : public LLPanelWearableListItem
{
public:
	static LLPanelDummyClothingListItem* create(LLWearableType::EType w_type);

	/*virtual*/ void updateItem();
	/*virtual*/ BOOL postBuild();

protected:
	LLPanelDummyClothingListItem(LLWearableType::EType w_type);

	/*virtual*/ void init();

	static std::string wearableTypeToString(LLWearableType::EType w_type);

private:
	LLWearableType::EType mWearableType;
};

/**
 * @class LLWearableItemsList
 *
 * A flat list of wearable inventory items.
 * Collects all items that can be a part of an outfit from
 * an inventory category specified by UUID and displays them
 * as a flat list.
 */
class LLWearableItemsList : public LLInventoryItemsList
{
public:
	struct Params : public LLInitParam::Block<Params, LLInventoryItemsList::Params>
	{
		Params();
	};

	virtual ~LLWearableItemsList();

	void updateList(const LLUUID& category_id);

protected:
	friend class LLUICtrlFactory;
	LLWearableItemsList(const LLWearableItemsList::Params& p);
};

#endif //LL_LLWEARABLEITEMSLIST_H
