/**
 * @file llwearableitemslist.h
 * @brief A flat list of wearable items.
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

#ifndef LL_LLWEARABLEITEMSLIST_H
#define LL_LLWEARABLEITEMSLIST_H

// libs
#include "llpanel.h"
#include "llsingleton.h"

// newview
#include "llinventoryitemslist.h"
#include "llinventorylistitem.h"
#include "lllistcontextmenu.h"
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

	LLPanelWearableListItem(LLViewerInventoryItem* item, const Params& params);
};

/**
 * @class LLPanelWearableOutfitItem
 *
 * Outfit item for "My Outfits" list.
 * Extends LLPanelInventoryListItemBase with handling
 * double click to wear the item.
 */
class LLPanelWearableOutfitItem : public LLPanelInventoryListItemBase
{
	LOG_CLASS(LLPanelWearableOutfitItem);
public:
	static LLPanelWearableOutfitItem* create(LLViewerInventoryItem* item,
											 bool worn_indication_enabled);

	/**
	 * Updates item name and (worn) suffix.
	 */
	/*virtual*/ void updateItem(const std::string& name,
								EItemState item_state = IS_DEFAULT);

protected:
	LLPanelWearableOutfitItem(LLViewerInventoryItem* item,
							  bool worn_indication_enabled, const Params& params);

private:
	bool	mWornIndicationEnabled;
};

class LLPanelDeletableWearableListItem : public LLPanelWearableListItem
{
	LOG_CLASS(LLPanelDeletableWearableListItem);
public:
	struct Params : public LLInitParam::Block<Params, LLPanelWearableListItem::Params>
	{
		Optional<LLButton::Params>		delete_btn;

		Params();
	};


	static LLPanelDeletableWearableListItem* create(LLViewerInventoryItem* item);

	virtual ~LLPanelDeletableWearableListItem() {};

	/*virtual*/ BOOL postBuild();

	/**
	 * Make button visible during mouse over event.
	 */
	inline void setShowDeleteButton(bool show) { setShowWidget("btn_delete", show); }

protected:
	LLPanelDeletableWearableListItem(LLViewerInventoryItem* item, const Params& params);
};

/** Outfit list item for an attachment */
class LLPanelAttachmentListItem : public LLPanelDeletableWearableListItem
{
	LOG_CLASS(LLPanelAttachmentListItem);
public:
	static LLPanelAttachmentListItem* create(LLViewerInventoryItem* item);
	virtual ~LLPanelAttachmentListItem() {};

	/** Set item title. Joint name is added to the title in parenthesis */
	/*virtual*/ void updateItem(const std::string& name,
								EItemState item_state = IS_DEFAULT);

protected:
	LLPanelAttachmentListItem(LLViewerInventoryItem* item, const Params& params) : LLPanelDeletableWearableListItem(item, params) {};
};

/**
 * @class LLPanelClothingListItem
 *
 * Provides buttons for editing, moving, deleting a wearable.
 */
class LLPanelClothingListItem : public LLPanelDeletableWearableListItem
{
	LOG_CLASS(LLPanelClothingListItem);
public:

	struct Params : public LLInitParam::Block<Params, LLPanelDeletableWearableListItem::Params>
	{
		Optional<LLButton::Params>		up_btn,
										down_btn,
										edit_btn;
		Optional<LLPanel::Params>		lock_panel,
										edit_panel;
		Optional<LLIconCtrl::Params>	lock_icon;

		Params();
	};

	static LLPanelClothingListItem* create(LLViewerInventoryItem* item);

	virtual ~LLPanelClothingListItem();

	/*virtual*/ BOOL postBuild();

	/**
	 * Make button visible during mouse over event.
	 */
	inline void setShowMoveUpButton(bool show) { setShowWidget("btn_move_up", show); }

	inline void setShowMoveDownButton(bool show) { setShowWidget("btn_move_down", show); }
	inline void setShowLockButton(bool show) { setShowWidget("btn_lock", show); }
	inline void setShowEditButton(bool show) { setShowWidget("btn_edit_panel", show); }

protected:

	LLPanelClothingListItem(LLViewerInventoryItem* item, const Params& params);

};

class LLPanelBodyPartsListItem : public LLPanelWearableListItem
{
	LOG_CLASS(LLPanelBodyPartsListItem);
public:
	struct Params : public LLInitParam::Block<Params, LLPanelWearableListItem::Params>
	{
		Optional<LLButton::Params>		edit_btn;
		Optional<LLPanel::Params>		lock_panel,
										edit_panel;
		Optional<LLIconCtrl::Params>	lock_icon;

		Params();
	};

	static LLPanelBodyPartsListItem* create(LLViewerInventoryItem* item);

	virtual ~LLPanelBodyPartsListItem();

	/*virtual*/ BOOL postBuild();

	/**
	* Make button visible during mouse over event.
	*/
	inline void setShowLockButton(bool show) { setShowWidget("btn_lock", show); }
	inline void setShowEditButton(bool show) { setShowWidget("btn_edit_panel", show); }

protected:
	LLPanelBodyPartsListItem(LLViewerInventoryItem* item, const Params& params);
};


/**
 * @class LLPanelDummyClothingListItem
 *
 * A dummy item panel - displays grayed clothing icon, grayed title '<clothing> not worn' and 'add' button
 */
class LLPanelDummyClothingListItem : public LLPanelWearableListItem
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanelWearableListItem::Params>
	{
		Optional<LLPanel::Params> add_panel;
		Optional<LLButton::Params> add_btn;
		Params();
	};
	static LLPanelDummyClothingListItem* create(LLWearableType::EType w_type);

	/*virtual*/ BOOL postBuild();
	LLWearableType::EType getWearableType() const;

protected:
	LLPanelDummyClothingListItem(LLWearableType::EType w_type, const Params& params);

	static std::string wearableTypeToString(LLWearableType::EType w_type);

private:
	LLWearableType::EType mWearableType;
};

/**
 * @class LLWearableListItemComparator
 *
 * Abstract comparator of wearable list items.
 */
class LLWearableListItemComparator : public LLFlatListView::ItemComparator
{
	LOG_CLASS(LLWearableListItemComparator);

public:
	LLWearableListItemComparator() {};
	virtual ~LLWearableListItemComparator() {};

	virtual bool compare(const LLPanel* item1, const LLPanel* item2) const
	{
		const LLPanelInventoryListItemBase* wearable_item1 = dynamic_cast<const LLPanelInventoryListItemBase*>(item1);
		const LLPanelInventoryListItemBase* wearable_item2 = dynamic_cast<const LLPanelInventoryListItemBase*>(item2);

		if (!wearable_item1 || !wearable_item2)
		{
			llwarning("item1 and item2 cannot be null", 0);
			return true;
		}

		return doCompare(wearable_item1, wearable_item2);
	}

protected:

	/**
	 * Returns true if wearable_item1 < wearable_item2, false otherwise
	 * Implement this method in your particular comparator.
	 */
	virtual bool doCompare(const LLPanelInventoryListItemBase* wearable_item1, const LLPanelInventoryListItemBase* wearable_item2) const = 0;
};

/**
 * @class LLWearableItemNameComparator
 *
 * Comparator for sorting wearable list items by name.
 */
class LLWearableItemNameComparator : public LLWearableListItemComparator
{
	LOG_CLASS(LLWearableItemNameComparator);

public:
	LLWearableItemNameComparator() {};
	virtual ~LLWearableItemNameComparator() {};

protected:
	/*virtual*/ bool doCompare(const LLPanelInventoryListItemBase* wearable_item1, const LLPanelInventoryListItemBase* wearable_item2) const;
};

/**
 * @class LLWearableItemTypeNameComparator
 *
 * Comparator for sorting wearable list items by type and name.
 */
class LLWearableItemTypeNameComparator : public LLWearableItemNameComparator
{
	LOG_CLASS(LLWearableItemTypeNameComparator);

public:

	LLWearableItemTypeNameComparator();
	virtual ~LLWearableItemTypeNameComparator() {};

	enum ETypeListOrder
	{
		ORDER_RANK_1 = 1,
		ORDER_RANK_2,
		ORDER_RANK_3,
		ORDER_RANK_UNKNOWN
	};

	void setOrder(LLAssetType::EType items_of_type, ETypeListOrder order_priority, bool sort_items_by_name, bool sort_wearable_items_by_name);

protected:
	/**
	 * All information about sort order is stored in mWearableOrder map
	 *
	 * mWearableOrder :      KEYS              VALUES
	 *                  [LLAssetType] [struct LLWearableTypeOrder]
	 *
	 *---------------------------------------------------------------------------------------------
	 * I. Determines order (ORDER_RANK) in which items of LLAssetType should be displayed in list.
	 *     For example by spec in MY OUTFITS the order is:
	 *     1. AT_CLOTHING (ORDER_RANK_1)
	 *     2. AT_OBJECT   (ORDER_RANK_2)
	 *     3. AT_BODYPART (ORDER_RANK_3)
	 *
	 * II.Items of each type(LLAssetType) are sorted by name or type(LLWearableType)
	 *     For example by spec in MY OUTFITS the order within each items type(LLAssetType) is:
	 *     1. AT_OBJECTS (abc order)
	 *     2. AT_CLOTHINGS
	 *         - by type (types order determined in LLWearableType::EType)
	 *         - outer layer on top
	 *     3. AT_BODYPARTS  (abc order)
	 *---------------------------------------------------------------------------------------------
	 *
	 * For each LLAssetType (KEYS in mWearableOrder) the information about:
	 *
	 *                                             I.  ORDER_RANK (the flag is LLWearableTypeOrder::mOrderPriority)
	 *
	 *                                             II. whether items of this LLAssetType type should be ordered
	 *                                                 by name or by LLWearableType::EType (the flag is LLWearableTypeOrder::mSortAssetTypeByName)
	 *
	 *                                             III.whether items of LLWearableType type within this LLAssetType
	 *                                                 should be ordered by name (the flag is LLWearableTypeOrder::mSortWearableTypeByName)
	 *
	 *  holds in mWearableOrder map as VALUES (struct LLWearableTypeOrder).
	 */
	/*virtual*/ bool doCompare(const LLPanelInventoryListItemBase* wearable_item1, const LLPanelInventoryListItemBase* wearable_item2) const;

private:

	struct LLWearableTypeOrder
	{
		ETypeListOrder mOrderPriority;
		bool mSortAssetTypeByName;
		bool mSortWearableTypeByName;

		LLWearableTypeOrder(ETypeListOrder order_priority, bool sort_asset_by_name, bool sort_wearable_by_name);
	LLWearableTypeOrder() : mOrderPriority(ORDER_RANK_UNKNOWN), mSortAssetTypeByName(false), mSortWearableTypeByName(false) {};
	};

	ETypeListOrder getTypeListOrder(LLAssetType::EType item_type) const;

	bool sortAssetTypeByName(LLAssetType::EType item_type) const;
	bool sortWearableTypeByName(LLAssetType::EType item_type) const;

	typedef std::map<LLAssetType::EType,LLWearableTypeOrder> wearable_type_order_map_t;
	wearable_type_order_map_t mWearableOrder;
};

/**
 * @class LLWearableItemCreationDateComparator
 *
 * Comparator for sorting wearable list items by creation date (newest go first).
 */
class LLWearableItemCreationDateComparator : public LLWearableItemNameComparator
{
	LOG_CLASS(LLWearableItemCreationDateComparator);

protected:
	/*virtual*/ bool doCompare(const LLPanelInventoryListItemBase* item1, const LLPanelInventoryListItemBase* item2) const;
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
	LOG_CLASS(LLWearableItemsList);
public:
	/**
	 * Context menu.
	 * 
	 * This menu is likely to be used from outside
	 * (e.g. for items selected across multiple wearable lists),
	 * so making it a singleton.
	 */
	class ContextMenu : public LLListContextMenu, public LLSingleton<ContextMenu>
	{
	public:
		ContextMenu();
		/*virtual*/ void show(LLView* spawning_view, const uuid_vec_t& uuids, S32 x, S32 y);

	protected:
		enum {
			MASK_CLOTHING		= 0x01,
			MASK_BODYPART		= 0x02,
			MASK_ATTACHMENT		= 0x04,
			MASK_UNKNOWN		= 0x08,
		};

		/* virtual */ LLContextMenu* createMenu();
		void updateItemsVisibility(LLContextMenu* menu);
		void updateItemsLabels(LLContextMenu* menu);
		static void setMenuItemVisible(LLContextMenu* menu, const std::string& name, bool val);
		static void setMenuItemEnabled(LLContextMenu* menu, const std::string& name, bool val);
		static void updateMask(U32& mask, LLAssetType::EType at);
		static void createNewWearable(const LLUUID& item_id);
		static bool canAddWearables(const uuid_vec_t& item_ids);

		LLWearableItemsList*	mParent;
	};

	struct Params : public LLInitParam::Block<Params, LLInventoryItemsList::Params>
	{
		Optional<bool> standalone;
		Optional<bool> worn_indication_enabled;

		Params();
	};

	typedef enum e_sort_order {
		// Values should be compatible with InventorySortOrder setting.
		E_SORT_BY_NAME			= 0,
		E_SORT_BY_MOST_RECENT	= 1,
		E_SORT_BY_TYPE_LAYER	= 2,
		E_SORT_BY_TYPE_NAME 	= 3,
	} ESortOrder;

	virtual ~LLWearableItemsList();

	/*virtual*/ void addNewItem(LLViewerInventoryItem* item, bool rearrange = true);

	void updateList(const LLUUID& category_id);

	/**
	 * Update items that match UUIDs from changed_items_uuids
	 * or links that point at such items.
	 */
	void updateChangedItems(const uuid_vec_t& changed_items_uuids);

	bool isStandalone() const { return mIsStandalone; }

	ESortOrder getSortOrder() const { return mSortOrder; }

	void setSortOrder(ESortOrder sort_order, bool sort_now = true);

protected:
	friend class LLUICtrlFactory;
	LLWearableItemsList(const LLWearableItemsList::Params& p);

	void onRightClick(S32 x, S32 y);

	bool mIsStandalone;
	bool mWornIndicationEnabled;

	ESortOrder		mSortOrder;
};

#endif //LL_LLWEARABLEITEMSLIST_H
