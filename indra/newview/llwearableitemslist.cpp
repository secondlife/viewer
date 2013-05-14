/**
 * @file llwearableitemslist.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llwearableitemslist.h"

#include "lliconctrl.h"
#include "llmenugl.h" // for LLContextMenu

#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "llinventoryicon.h"
#include "lltransutil.h"
#include "llviewerattachmenu.h"
#include "llvoavatarself.h"

class LLFindOutfitItems : public LLInventoryCollectFunctor
{
public:
	LLFindOutfitItems() {}
	virtual ~LLFindOutfitItems() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
};

bool LLFindOutfitItems::operator()(LLInventoryCategory* cat,
								   LLInventoryItem* item)
{
	if(item)
	{
		if((item->getType() == LLAssetType::AT_CLOTHING)
		   || (item->getType() == LLAssetType::AT_BODYPART)
		   || (item->getType() == LLAssetType::AT_OBJECT))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void LLPanelWearableListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLPanelInventoryListItemBase::onMouseEnter(x, y, mask);
	setWidgetsVisible(true);
	reshapeWidgets();
}

void LLPanelWearableListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLPanelInventoryListItemBase::onMouseLeave(x, y, mask);
	setWidgetsVisible(false);
	reshapeWidgets();
}

LLPanelWearableListItem::LLPanelWearableListItem(LLViewerInventoryItem* item, const LLPanelWearableListItem::Params& params)
: LLPanelInventoryListItemBase(item, params)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// static
LLPanelWearableOutfitItem* LLPanelWearableOutfitItem::create(LLViewerInventoryItem* item,
															 bool worn_indication_enabled)
{
	LLPanelWearableOutfitItem* list_item = NULL;
	if (item)
	{
		const LLPanelInventoryListItemBase::Params& params = LLUICtrlFactory::getDefaultParams<LLPanelInventoryListItemBase>();

		list_item = new LLPanelWearableOutfitItem(item, worn_indication_enabled, params);
		list_item->initFromParams(params);
		list_item->postBuild();
	}
	return list_item;
}

LLPanelWearableOutfitItem::LLPanelWearableOutfitItem(LLViewerInventoryItem* item,
													 bool worn_indication_enabled,
													 const LLPanelWearableOutfitItem::Params& params)
: LLPanelInventoryListItemBase(item, params)
, mWornIndicationEnabled(worn_indication_enabled)
{
}

// virtual
void LLPanelWearableOutfitItem::updateItem(const std::string& name,
										   EItemState item_state)
{
	std::string search_label = name;

	// Updating item's worn status depending on whether it is linked in COF or not.
	// We don't use get_is_item_worn() here because this update is triggered by
	// an inventory observer upon link in COF beind added or removed so actual
	// worn status of a linked item may still remain unchanged.
	if (mWornIndicationEnabled && LLAppearanceMgr::instance().isLinkInCOF(mInventoryItemUUID))
	{
		search_label += LLTrans::getString("worn");
		item_state = IS_WORN;
	}

	LLPanelInventoryListItemBase::updateItem(search_label, item_state);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
static LLWidgetNameRegistry::StaticRegistrar sRegisterPanelClothingListItem(&typeid(LLPanelClothingListItem::Params), "clothing_list_item");


LLPanelClothingListItem::Params::Params()
:	up_btn("up_btn"),
	down_btn("down_btn"),
	edit_btn("edit_btn"),
	lock_panel("lock_panel"),
	edit_panel("edit_panel"),
	lock_icon("lock_icon")
{}

// static
LLPanelClothingListItem* LLPanelClothingListItem::create(LLViewerInventoryItem* item)
{
	LLPanelClothingListItem* list_item = NULL;
	if(item)
	{
		const LLPanelClothingListItem::Params& params = LLUICtrlFactory::getDefaultParams<LLPanelClothingListItem>();
		list_item = new LLPanelClothingListItem(item, params);
		list_item->initFromParams(params);
		list_item->postBuild();
	}
	return list_item;
}

LLPanelClothingListItem::LLPanelClothingListItem(LLViewerInventoryItem* item, const LLPanelClothingListItem::Params& params)
 : LLPanelDeletableWearableListItem(item, params)
{
	LLButton::Params button_params = params.up_btn;
	applyXUILayout(button_params, this);
	addChild(LLUICtrlFactory::create<LLButton>(button_params));

	button_params = params.down_btn;
	applyXUILayout(button_params, this);
	addChild(LLUICtrlFactory::create<LLButton>(button_params));

	LLPanel::Params panel_params = params.lock_panel;
	applyXUILayout(panel_params, this);
	LLPanel* lock_panelp = LLUICtrlFactory::create<LLPanel>(panel_params);
	addChild(lock_panelp);

	panel_params = params.edit_panel;
	applyXUILayout(panel_params, this);
	LLPanel* edit_panelp = LLUICtrlFactory::create<LLPanel>(panel_params);
	addChild(edit_panelp);

	if (lock_panelp)
{
		LLIconCtrl::Params icon_params = params.lock_icon;
		applyXUILayout(icon_params, this);
		lock_panelp->addChild(LLUICtrlFactory::create<LLIconCtrl>(icon_params));
}

	if (edit_panelp)
{
		button_params = params.edit_btn;
		applyXUILayout(button_params, this);
		edit_panelp->addChild(LLUICtrlFactory::create<LLButton>(button_params));
	}

	setSeparatorVisible(false);
}

LLPanelClothingListItem::~LLPanelClothingListItem()
{
}

BOOL LLPanelClothingListItem::postBuild()
{
	LLPanelDeletableWearableListItem::postBuild();

	addWidgetToRightSide("btn_move_up");
	addWidgetToRightSide("btn_move_down");
	addWidgetToRightSide("btn_lock");
	addWidgetToRightSide("btn_edit_panel");

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static LLWidgetNameRegistry::StaticRegistrar sRegisterPanelBodyPartsListItem(&typeid(LLPanelBodyPartsListItem::Params), "bodyparts_list_item");


LLPanelBodyPartsListItem::Params::Params()
:	edit_btn("edit_btn"),
	edit_panel("edit_panel"),
	lock_panel("lock_panel"),
	lock_icon("lock_icon")
{}

// static
LLPanelBodyPartsListItem* LLPanelBodyPartsListItem::create(LLViewerInventoryItem* item)
{
	LLPanelBodyPartsListItem* list_item = NULL;
	if(item)
	{
		const Params& params = LLUICtrlFactory::getDefaultParams<LLPanelBodyPartsListItem>();
		list_item = new LLPanelBodyPartsListItem(item, params);
		list_item->initFromParams(params);
		list_item->postBuild();
	}
	return list_item;
}

LLPanelBodyPartsListItem::LLPanelBodyPartsListItem(LLViewerInventoryItem* item, const LLPanelBodyPartsListItem::Params& params)
: LLPanelWearableListItem(item, params)
{
	LLPanel::Params panel_params = params.edit_panel;
	applyXUILayout(panel_params, this);
	LLPanel* edit_panelp = LLUICtrlFactory::create<LLPanel>(panel_params);
	addChild(edit_panelp);

	panel_params = params.lock_panel;
	applyXUILayout(panel_params, this);
	LLPanel* lock_panelp = LLUICtrlFactory::create<LLPanel>(panel_params);
	addChild(lock_panelp);
	
	if (edit_panelp)
	{
		LLButton::Params btn_params = params.edit_btn;
		applyXUILayout(btn_params, this);
		edit_panelp->addChild(LLUICtrlFactory::create<LLButton>(btn_params));
}

	if (lock_panelp)
{
		LLIconCtrl::Params icon_params = params.lock_icon;
		applyXUILayout(icon_params, this);
		lock_panelp->addChild(LLUICtrlFactory::create<LLIconCtrl>(icon_params));
	}

	setSeparatorVisible(true);
}

LLPanelBodyPartsListItem::~LLPanelBodyPartsListItem()
{
}

BOOL LLPanelBodyPartsListItem::postBuild()
{
	LLPanelInventoryListItemBase::postBuild();

	addWidgetToRightSide("btn_lock");
	addWidgetToRightSide("btn_edit_panel");

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}

static LLWidgetNameRegistry::StaticRegistrar sRegisterPanelDeletableWearableListItem(&typeid(LLPanelDeletableWearableListItem::Params), "deletable_wearable_list_item");

LLPanelDeletableWearableListItem::Params::Params()
:	delete_btn("delete_btn")
{}

// static
LLPanelDeletableWearableListItem* LLPanelDeletableWearableListItem::create(LLViewerInventoryItem* item)
{
	LLPanelDeletableWearableListItem* list_item = NULL;
	if(item)
	{
		const Params& params = LLUICtrlFactory::getDefaultParams<LLPanelDeletableWearableListItem>();
		list_item = new LLPanelDeletableWearableListItem(item, params);
		list_item->initFromParams(params);
		list_item->postBuild();
	}
	return list_item;
}

LLPanelDeletableWearableListItem::LLPanelDeletableWearableListItem(LLViewerInventoryItem* item, const LLPanelDeletableWearableListItem::Params& params)
: LLPanelWearableListItem(item, params)
{
	LLButton::Params button_params = params.delete_btn;
	applyXUILayout(button_params, this);
	addChild(LLUICtrlFactory::create<LLButton>(button_params));

	setSeparatorVisible(true);
}

BOOL LLPanelDeletableWearableListItem::postBuild()
{
	LLPanelWearableListItem::postBuild();

	addWidgetToLeftSide("btn_delete");

	LLButton* delete_btn = getChild<LLButton>("btn_delete");
	// Reserve space for 'delete' button event if it is invisible.
	setLeftWidgetsWidth(delete_btn->getRect().mRight);

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}


// static
LLPanelAttachmentListItem* LLPanelAttachmentListItem::create(LLViewerInventoryItem* item)
{
	LLPanelAttachmentListItem* list_item = NULL;
	if(item)
	{
		const Params& params = LLUICtrlFactory::getDefaultParams<LLPanelDeletableWearableListItem>();

		list_item = new LLPanelAttachmentListItem(item, params);
		list_item->initFromParams(params);
		list_item->postBuild();
	}
	return list_item;
}

void LLPanelAttachmentListItem::updateItem(const std::string& name,
										   EItemState item_state)
{
	std::string title_joint = name;

	LLViewerInventoryItem* inv_item = getItem();
	if (inv_item && isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(inv_item->getLinkedUUID()))
	{
		std::string joint = LLTrans::getString(gAgentAvatarp->getAttachedPointName(inv_item->getLinkedUUID()));
		title_joint =  title_joint + " (" + joint + ")";
	}

	LLPanelInventoryListItemBase::updateItem(title_joint, item_state);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
static LLWidgetNameRegistry::StaticRegistrar sRegisterPanelDummyClothingListItem(&typeid(LLPanelDummyClothingListItem::Params), "dummy_clothing_list_item");

LLPanelDummyClothingListItem::Params::Params()
:	add_panel("add_panel"),
	add_btn("add_btn")
{}

LLPanelDummyClothingListItem* LLPanelDummyClothingListItem::create(LLWearableType::EType w_type)
{
	const Params& params = LLUICtrlFactory::getDefaultParams<LLPanelDummyClothingListItem>();

	LLPanelDummyClothingListItem* list_item = new LLPanelDummyClothingListItem(w_type, params);
	list_item->initFromParams(params);
	list_item->postBuild();
	return list_item;
}

BOOL LLPanelDummyClothingListItem::postBuild()
{
	addWidgetToRightSide("btn_add_panel");

	setIconImage(LLInventoryIcon::getIcon(LLAssetType::AT_CLOTHING, LLInventoryType::IT_NONE, mWearableType, FALSE));
	updateItem(wearableTypeToString(mWearableType));

	// Make it look loke clothing item - reserve space for 'delete' button
	setLeftWidgetsWidth(getChildView("item_icon")->getRect().mLeft);

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}

LLWearableType::EType LLPanelDummyClothingListItem::getWearableType() const
{
	return mWearableType;
}

LLPanelDummyClothingListItem::LLPanelDummyClothingListItem(LLWearableType::EType w_type, const LLPanelDummyClothingListItem::Params& params)
:	LLPanelWearableListItem(NULL, params), 
	mWearableType(w_type)
{
	LLPanel::Params panel_params(params.add_panel);
	applyXUILayout(panel_params, this);
	LLPanel* add_panelp = LLUICtrlFactory::create<LLPanel>(panel_params);
	addChild(add_panelp);

	if (add_panelp)
{
		LLButton::Params button_params(params.add_btn);
		applyXUILayout(button_params, this);
		add_panelp->addChild(LLUICtrlFactory::create<LLButton>(button_params));
}

	setSeparatorVisible(true);
}

typedef std::map<LLWearableType::EType, std::string> clothing_to_string_map_t;

clothing_to_string_map_t init_clothing_string_map()
{
	clothing_to_string_map_t w_map;
	w_map.insert(std::make_pair(LLWearableType::WT_SHIRT, "shirt_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_PANTS, "pants_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_SHOES, "shoes_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_SOCKS, "socks_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_JACKET, "jacket_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_GLOVES, "gloves_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_UNDERSHIRT, "undershirt_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_UNDERPANTS, "underpants_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_SKIRT, "skirt_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_ALPHA, "alpha_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_TATTOO, "tattoo_not_worn"));
	w_map.insert(std::make_pair(LLWearableType::WT_PHYSICS, "physics_not_worn"));
	return w_map;
}

std::string LLPanelDummyClothingListItem::wearableTypeToString(LLWearableType::EType w_type)
{
	static const clothing_to_string_map_t w_map = init_clothing_string_map();
	static const std::string invalid_str = LLTrans::getString("invalid_not_worn");
	
	std::string type_str = invalid_str;
	clothing_to_string_map_t::const_iterator it = w_map.find(w_type);
	if(w_map.end() != it)
	{
		type_str = LLTrans::getString(it->second);
	}
	return type_str;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLWearableItemTypeNameComparator::LLWearableTypeOrder::LLWearableTypeOrder(LLWearableItemTypeNameComparator::ETypeListOrder order_priority, bool sort_asset_by_name, bool sort_wearable_by_name):
		mOrderPriority(order_priority),
		mSortAssetTypeByName(sort_asset_by_name),
		mSortWearableTypeByName(sort_wearable_by_name)
{
}

LLWearableItemTypeNameComparator::LLWearableItemTypeNameComparator()
{
	// By default the sort order conforms the order by spec of MY OUTFITS items list:
	// 1. CLOTHING - sorted by name
	// 2. OBJECT   - sorted by type
	// 3. BODYPART - sorted by name
	mWearableOrder[LLAssetType::AT_CLOTHING] = LLWearableTypeOrder(ORDER_RANK_1, false, false);
	mWearableOrder[LLAssetType::AT_OBJECT]   = LLWearableTypeOrder(ORDER_RANK_2, true, true);
	mWearableOrder[LLAssetType::AT_BODYPART] = LLWearableTypeOrder(ORDER_RANK_3, false, true);
}

void LLWearableItemTypeNameComparator::setOrder(LLAssetType::EType items_of_type,  LLWearableItemTypeNameComparator::ETypeListOrder order_priority, bool sort_asset_items_by_name, bool sort_wearable_items_by_name)
{
	mWearableOrder[items_of_type] = LLWearableTypeOrder(order_priority, sort_asset_items_by_name, sort_wearable_items_by_name);
}

/*virtual*/
bool LLWearableItemNameComparator::doCompare(const LLPanelInventoryListItemBase* wearable_item1, const LLPanelInventoryListItemBase* wearable_item2) const
{
	std::string name1 = wearable_item1->getItemName();
	std::string name2 = wearable_item2->getItemName();

	LLStringUtil::toUpper(name1);
	LLStringUtil::toUpper(name2);

	return name1 < name2;
}

/*virtual*/
bool LLWearableItemTypeNameComparator::doCompare(const LLPanelInventoryListItemBase* wearable_item1, const LLPanelInventoryListItemBase* wearable_item2) const
{
	const LLAssetType::EType item_type1 = wearable_item1->getType();
	const LLAssetType::EType item_type2 = wearable_item2->getType();

	LLWearableItemTypeNameComparator::ETypeListOrder item_type_order1 = getTypeListOrder(item_type1);
	LLWearableItemTypeNameComparator::ETypeListOrder item_type_order2 = getTypeListOrder(item_type2);

	if (item_type_order1 != item_type_order2)
	{
		// If items are of different asset types we can compare them
		// by types order in the list.
		return item_type_order1 < item_type_order2;
	}

	if (sortAssetTypeByName(item_type1))
	{
		// If both items are of the same asset type except AT_CLOTHING and AT_BODYPART
		// we can compare them by name.
		return LLWearableItemNameComparator::doCompare(wearable_item1, wearable_item2);
	}

	const LLWearableType::EType item_wearable_type1 = wearable_item1->getWearableType();
	const LLWearableType::EType item_wearable_type2 = wearable_item2->getWearableType();

	if (item_wearable_type1 != item_wearable_type2)
		// If items are of different LLWearableType::EType types they are compared
		// by LLWearableType::EType. types order determined in LLWearableType::EType.
	{
		// If items are of different LLWearableType::EType types they are compared
		// by LLWearableType::EType. types order determined in LLWearableType::EType.
		return item_wearable_type1 < item_wearable_type2;
	}
	else
	{
		// If both items are of the same clothing type they are compared
		// by description and place in reverse order (i.e. outer layer item
		// on top) OR by name
		if(sortWearableTypeByName(item_type1))
		{
			return LLWearableItemNameComparator::doCompare(wearable_item1, wearable_item2);
		}
		return wearable_item1->getDescription() > wearable_item2->getDescription();
	}
}

LLWearableItemTypeNameComparator::ETypeListOrder LLWearableItemTypeNameComparator::getTypeListOrder(LLAssetType::EType item_type) const
{
	wearable_type_order_map_t::const_iterator const_it = mWearableOrder.find(item_type);


	if(const_it == mWearableOrder.end())
	{
		llwarns<<"Absent information about order rang of items of "<<LLAssetType::getDesc(item_type)<<" type"<<llendl;
		return ORDER_RANK_UNKNOWN;
	}

	return const_it->second.mOrderPriority;
}

bool LLWearableItemTypeNameComparator::sortAssetTypeByName(LLAssetType::EType item_type) const
{
	wearable_type_order_map_t::const_iterator const_it = mWearableOrder.find(item_type);


	if(const_it == mWearableOrder.end())
	{
		llwarns<<"Absent information about sorting items of "<<LLAssetType::getDesc(item_type)<<" type"<<llendl;
		return true;
	}


	return const_it->second.mSortAssetTypeByName;
	}


bool LLWearableItemTypeNameComparator::sortWearableTypeByName(LLAssetType::EType item_type) const
{
	wearable_type_order_map_t::const_iterator const_it = mWearableOrder.find(item_type);


	if(const_it == mWearableOrder.end())
	{
		llwarns<<"Absent information about sorting items of "<<LLAssetType::getDesc(item_type)<<" type"<<llendl;
		return true;
}


	return const_it->second.mSortWearableTypeByName;
}

/*virtual*/
bool LLWearableItemCreationDateComparator::doCompare(const LLPanelInventoryListItemBase* item1, const LLPanelInventoryListItemBase* item2) const
{
	time_t date1 = item1->getCreationDate();
	time_t date2 = item2->getCreationDate();

	if (date1 == date2)
	{
		return LLWearableItemNameComparator::doCompare(item1, item2);
	}

	return date1 > date2;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static LLWearableItemTypeNameComparator WEARABLE_TYPE_NAME_COMPARATOR;
static const LLWearableItemTypeNameComparator WEARABLE_TYPE_LAYER_COMPARATOR;
static const LLWearableItemNameComparator WEARABLE_NAME_COMPARATOR;
static const LLWearableItemCreationDateComparator WEARABLE_CREATION_DATE_COMPARATOR;

static const LLDefaultChildRegistry::Register<LLWearableItemsList> r("wearable_items_list");

LLWearableItemsList::Params::Params()
:	standalone("standalone", true)
,	worn_indication_enabled("worn_indication_enabled", true)
{}

LLWearableItemsList::LLWearableItemsList(const LLWearableItemsList::Params& p)
:	LLInventoryItemsList(p)
{
	setSortOrder(E_SORT_BY_TYPE_LAYER, false);
	mIsStandalone = p.standalone;
	if (mIsStandalone)
	{
		// Use built-in context menu.
		setRightMouseDownCallback(boost::bind(&LLWearableItemsList::onRightClick, this, _2, _3));
	}
	mWornIndicationEnabled = p.worn_indication_enabled;
	setNoItemsCommentText(LLTrans::getString("LoadingData"));
}

// virtual
LLWearableItemsList::~LLWearableItemsList()
{}

// virtual
void LLWearableItemsList::addNewItem(LLViewerInventoryItem* item, bool rearrange /*= true*/)
{
	if (!item)
	{
		llwarns << "No inventory item. Couldn't create flat list item." << llendl;
		llassert(item != NULL);
	}

	LLPanelWearableOutfitItem *list_item = LLPanelWearableOutfitItem::create(item, mWornIndicationEnabled);
	if (!list_item)
		return;

	bool is_item_added = addItem(list_item, item->getUUID(), ADD_BOTTOM, rearrange);
	if (!is_item_added)
	{
		llwarns << "Couldn't add flat list item." << llendl;
		llassert(is_item_added);
	}
}

void LLWearableItemsList::updateList(const LLUUID& category_id)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;

	LLFindOutfitItems collector = LLFindOutfitItems();
	// collectDescendentsIf takes non-const reference:
	gInventory.collectDescendentsIf(
		category_id,
		cat_array,
		item_array,
		LLInventoryModel::EXCLUDE_TRASH,
		collector);

	if(item_array.empty() && gInventory.isCategoryComplete(category_id))
	{
		setNoItemsCommentText(LLTrans::getString("EmptyOutfitText"));
	}

	refreshList(item_array);
}

void LLWearableItemsList::updateChangedItems(const uuid_vec_t& changed_items_uuids)
{
	// nothing to update
	if (changed_items_uuids.empty()) return;

	typedef std::vector<LLPanel*> item_panel_list_t;

	item_panel_list_t items;
	getItems(items);

	for (item_panel_list_t::iterator items_iter = items.begin();
			items_iter != items.end();
			++items_iter)
	{
		LLPanelInventoryListItemBase* item = dynamic_cast<LLPanelInventoryListItemBase*>(*items_iter);
		if (!item) continue;

		LLViewerInventoryItem* inv_item = item->getItem();
		if (!inv_item) continue;

		LLUUID linked_uuid = inv_item->getLinkedUUID();

		for (uuid_vec_t::const_iterator iter = changed_items_uuids.begin();
				iter != changed_items_uuids.end();
				++iter)
		{
			if (linked_uuid == *iter)
			{
				item->setNeedsRefresh(true);
				break;
			}
		}
	}
}

void LLWearableItemsList::onRightClick(S32 x, S32 y)
{
	uuid_vec_t selected_uuids;

	getSelectedUUIDs(selected_uuids);
	if (selected_uuids.empty())
	{
		return;
	}

	ContextMenu::instance().show(this, selected_uuids, x, y);
}

void LLWearableItemsList::setSortOrder(ESortOrder sort_order, bool sort_now)
{
	switch (sort_order)
	{
	case E_SORT_BY_MOST_RECENT:
		setComparator(&WEARABLE_CREATION_DATE_COMPARATOR);
		break;
	case E_SORT_BY_NAME:
		setComparator(&WEARABLE_NAME_COMPARATOR);
		break;
	case E_SORT_BY_TYPE_LAYER:
		setComparator(&WEARABLE_TYPE_LAYER_COMPARATOR);
		break;
	case E_SORT_BY_TYPE_NAME:
	{
		WEARABLE_TYPE_NAME_COMPARATOR.setOrder(LLAssetType::AT_CLOTHING, LLWearableItemTypeNameComparator::ORDER_RANK_1, false, true);
		setComparator(&WEARABLE_TYPE_NAME_COMPARATOR);
		break;
	}

	// No "default:" to raise compiler warning
	// if we're not handling something
	}

	mSortOrder = sort_order;

	if (sort_now)
	{
		sort();
	}
}

//////////////////////////////////////////////////////////////////////////
/// ContextMenu
//////////////////////////////////////////////////////////////////////////

LLWearableItemsList::ContextMenu::ContextMenu()
:	mParent(NULL)
{
}

void LLWearableItemsList::ContextMenu::show(LLView* spawning_view, const uuid_vec_t& uuids, S32 x, S32 y)
{
	mParent = dynamic_cast<LLWearableItemsList*>(spawning_view);
	LLListContextMenu::show(spawning_view, uuids, x, y);
	mParent = NULL; // to avoid dereferencing an invalid pointer
}

// virtual
LLContextMenu* LLWearableItemsList::ContextMenu::createMenu()
{
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	const uuid_vec_t& ids = mUUIDs;		// selected items IDs
	LLUUID selected_id = ids.front();	// ID of the first selected item

	// Register handlers common for all wearable types.
	registrar.add("Wearable.Wear", boost::bind(wear_multiple, ids, true));
	registrar.add("Wearable.Add", boost::bind(wear_multiple, ids, false));
	registrar.add("Wearable.Edit", boost::bind(handleMultiple, LLAgentWearables::editWearable, ids));
	registrar.add("Wearable.CreateNew", boost::bind(createNewWearable, selected_id));
	registrar.add("Wearable.ShowOriginal", boost::bind(show_item_original, selected_id));
	registrar.add("Wearable.TakeOffDetach", 
				  boost::bind(&LLAppearanceMgr::removeItemsFromAvatar, LLAppearanceMgr::getInstance(), ids));

	// Register handlers for clothing.
	registrar.add("Clothing.TakeOff", 
				  boost::bind(&LLAppearanceMgr::removeItemsFromAvatar, LLAppearanceMgr::getInstance(), ids));

	// Register handlers for body parts.

	// Register handlers for attachments.
	registrar.add("Attachment.Detach", 
				  boost::bind(&LLAppearanceMgr::removeItemsFromAvatar, LLAppearanceMgr::getInstance(), ids));
	registrar.add("Attachment.Profile", boost::bind(show_item_profile, selected_id));
	registrar.add("Object.Attach", boost::bind(LLViewerAttachMenu::attachObjects, ids, _2));

	// Create the menu.
	LLContextMenu* menu = createFromFile("menu_wearable_list_item.xml");

	// Determine which items should be visible/enabled.
	updateItemsVisibility(menu);

	// Update labels for the items requiring that.
	updateItemsLabels(menu);
	return menu;
}

void LLWearableItemsList::ContextMenu::updateItemsVisibility(LLContextMenu* menu)
{
	if (!menu)
	{
		llwarns << "Invalid menu" << llendl;
		return;
	}

	const uuid_vec_t& ids = mUUIDs;	// selected items IDs
	U32 mask = 0;					// mask of selected items' types
	U32 n_items = ids.size();		// number of selected items
	U32 n_worn = 0;					// number of worn items among the selected ones
	U32 n_already_worn = 0;			// number of items worn of same type as selected items
	U32 n_links = 0;				// number of links among the selected items
	U32 n_editable = 0;				// number of editable items among the selected ones

	bool can_be_worn = true;

	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		LLUUID id = *it;
		LLViewerInventoryItem* item = gInventory.getItem(id);

		if (!item)
		{
			llwarns << "Invalid item" << llendl;
			// *NOTE: the logic below may not work in this case
			continue;
		}

		updateMask(mask, item->getType());

		const LLWearableType::EType wearable_type = item->getWearableType();
		const bool is_link = item->getIsLinkType();
		const bool is_worn = get_is_item_worn(id);
		const bool is_editable = gAgentWearables.isWearableModifiable(id);
		const bool is_already_worn = gAgentWearables.selfHasWearable(wearable_type);
		if (is_worn)
		{
			++n_worn;
		}
		if (is_editable)
		{
			++n_editable;
		}
		if (is_link)
		{
			++n_links;
		}
		if (is_already_worn)
		{
			++n_already_worn;
		}

		if (can_be_worn)
		{
			can_be_worn = get_can_item_be_worn(item->getLinkedUUID());
		}
	} // for

	bool standalone = mParent ? mParent->isStandalone() : false;
	bool wear_add_visible = mask & (MASK_CLOTHING|MASK_ATTACHMENT) && n_worn == 0 && can_be_worn && (n_already_worn != 0 || mask & MASK_ATTACHMENT);

	// *TODO: eliminate multiple traversals over the menu items
	setMenuItemVisible(menu, "wear_wear", 			n_already_worn == 0 && n_worn == 0 && can_be_worn);
	setMenuItemEnabled(menu, "wear_wear", 			n_already_worn == 0 && n_worn == 0);
	setMenuItemVisible(menu, "wear_add",			wear_add_visible);
	setMenuItemEnabled(menu, "wear_add",			canAddWearables(ids));
	setMenuItemVisible(menu, "wear_replace",		n_worn == 0 && n_already_worn != 0 && can_be_worn);
	//visible only when one item selected and this item is worn
	setMenuItemVisible(menu, "edit",				!standalone && mask & (MASK_CLOTHING|MASK_BODYPART) && n_worn == n_items && n_worn == 1);
	setMenuItemEnabled(menu, "edit",				n_editable == 1 && n_worn == 1 && n_items == 1);
	setMenuItemVisible(menu, "create_new",			mask & (MASK_CLOTHING|MASK_BODYPART) && n_items == 1);
	setMenuItemEnabled(menu, "create_new",			canAddWearables(ids));
	setMenuItemVisible(menu, "show_original",		!standalone);
	setMenuItemEnabled(menu, "show_original",		n_items == 1 && n_links == n_items);
	setMenuItemVisible(menu, "take_off",			mask == MASK_CLOTHING && n_worn == n_items);
	setMenuItemVisible(menu, "detach",				mask == MASK_ATTACHMENT && n_worn == n_items);
	setMenuItemVisible(menu, "take_off_or_detach",	mask == (MASK_ATTACHMENT|MASK_CLOTHING));
	setMenuItemEnabled(menu, "take_off_or_detach",	n_worn == n_items);
	setMenuItemVisible(menu, "object_profile",		!standalone);
	setMenuItemEnabled(menu, "object_profile",		n_items == 1);
	setMenuItemVisible(menu, "--no options--", 		FALSE);
	setMenuItemEnabled(menu, "--no options--",		FALSE);

	// Populate or hide the "Attach to..." / "Attach to HUD..." submenus.
	if (mask == MASK_ATTACHMENT && n_worn == 0)
	{
		LLViewerAttachMenu::populateMenus("wearable_attach_to", "wearable_attach_to_hud");
	}
	else
	{
		setMenuItemVisible(menu, "wearable_attach_to",			false);
		setMenuItemVisible(menu, "wearable_attach_to_hud",		false);
	}

	if (mask & MASK_UNKNOWN)
	{
		llwarns << "Non-wearable items passed." << llendl;
	}

	U32 num_visible_items = 0;
	for (U32 menu_item_index = 0; menu_item_index < menu->getItemCount(); ++menu_item_index)
	{
		const LLMenuItemGL* menu_item = menu->getItem(menu_item_index);
		if (menu_item && menu_item->getVisible())
		{
			num_visible_items++;
		}
	}
	if (num_visible_items == 0)
	{
		setMenuItemVisible(menu, "--no options--", TRUE);
	}
}

void LLWearableItemsList::ContextMenu::updateItemsLabels(LLContextMenu* menu)
{
	llassert(menu);
	if (!menu) return;

	// Set proper label for the "Create new <WEARABLE_TYPE>" menu item.
	LLViewerInventoryItem* item = gInventory.getLinkedItem(mUUIDs.back());
	if (!item || !item->isWearableType()) return;

	LLWearableType::EType w_type = item->getWearableType();
	std::string new_label = LLTrans::getString("create_new_" + LLWearableType::getTypeName(w_type));

	LLMenuItemGL* menu_item = menu->getChild<LLMenuItemGL>("create_new");
	menu_item->setLabel(new_label);
}

// We need this method to convert non-zero BOOL values to exactly 1 (TRUE).
// Otherwise code relying on a BOOL value being TRUE may fail
// (I experienced a weird assert in LLView::drawChildren() because of that.
// static
void LLWearableItemsList::ContextMenu::setMenuItemVisible(LLContextMenu* menu, const std::string& name, bool val)
{
	menu->setItemVisible(name, val);
}

// static
void LLWearableItemsList::ContextMenu::setMenuItemEnabled(LLContextMenu* menu, const std::string& name, bool val)
{
	menu->setItemEnabled(name, val);
}

// static
void LLWearableItemsList::ContextMenu::updateMask(U32& mask, LLAssetType::EType at)
{
	if (at == LLAssetType::AT_CLOTHING)
	{
		mask |= MASK_CLOTHING;
	}
	else if (at == LLAssetType::AT_BODYPART)
	{
		mask |= MASK_BODYPART;
	}
	else if (at == LLAssetType::AT_OBJECT)
	{
		mask |= MASK_ATTACHMENT;
	}
	else
	{
		mask |= MASK_UNKNOWN;
	}
}

// static
void LLWearableItemsList::ContextMenu::createNewWearable(const LLUUID& item_id)
{
	LLViewerInventoryItem* item = gInventory.getLinkedItem(item_id);
	if (!item || !item->isWearableType()) return;

	LLAgentWearables::createWearable(item->getWearableType(), true);
}

// Returns true if all the given objects and clothes can be added.
// static
bool LLWearableItemsList::ContextMenu::canAddWearables(const uuid_vec_t& item_ids)
{
	// TODO: investigate wearables may not be loaded at this point EXT-8231

	U32 n_objects = 0;
	boost::unordered_map<LLWearableType::EType, U32> clothes_by_type;

	// Count given clothes (by wearable type) and objects.
	for (uuid_vec_t::const_iterator it = item_ids.begin(); it != item_ids.end(); ++it)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		if (!item)
		{
			return false;
		}

		if (item->getType() == LLAssetType::AT_OBJECT)
		{
			++n_objects;
		}
		else if (item->getType() == LLAssetType::AT_CLOTHING)
		{
			++clothes_by_type[item->getWearableType()];
		}
		else
		{
			llwarns << "Unexpected wearable type" << llendl;
			return false;
		}
	}

	// Check whether we can add all the objects.
	if (!isAgentAvatarValid() || !gAgentAvatarp->canAttachMoreObjects(n_objects))
	{
		return false;
	}

	// Check whether we can add all the clothes.
	boost::unordered_map<LLWearableType::EType, U32>::const_iterator m_it;
	for (m_it = clothes_by_type.begin(); m_it != clothes_by_type.end(); ++m_it)
	{
		LLWearableType::EType w_type	= m_it->first;
		U32 n_clothes					= m_it->second;

		U32 wearable_count = gAgentWearables.getWearableCount(w_type);
		if ((wearable_count > 0) && !LLWearableType::getAllowMultiwear(w_type))
		{
			return false;
		}
		if ((wearable_count + n_clothes) > LLAgentWearables::MAX_CLOTHING_PER_TYPE)
		{
			return false;
		}

	}

	return true;
}

// EOF
