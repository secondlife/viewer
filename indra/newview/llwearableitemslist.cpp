/**
 * @file llwearableitemslist.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llwearableitemslist.h"

#include "lliconctrl.h"

#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "lltransutil.h"

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

LLPanelWearableListItem::LLPanelWearableListItem(LLViewerInventoryItem* item)
: LLPanelInventoryListItemBase(item)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// static
LLPanelClothingListItem* LLPanelClothingListItem::create(LLViewerInventoryItem* item)
{
	LLPanelClothingListItem* list_item = NULL;
	if(item)
	{
		list_item = new LLPanelClothingListItem(item);
		list_item->init();
	}
	return list_item;
}

LLPanelClothingListItem::LLPanelClothingListItem(LLViewerInventoryItem* item)
 : LLPanelDeletableWearableListItem(item)
{
}

LLPanelClothingListItem::~LLPanelClothingListItem()
{
}

void LLPanelClothingListItem::init()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_clothing_list_item.xml");
}

BOOL LLPanelClothingListItem::postBuild()
{
	LLPanelDeletableWearableListItem::postBuild();

	addWidgetToRightSide("btn_move_up");
	addWidgetToRightSide("btn_move_down");
	addWidgetToRightSide("btn_lock");
	addWidgetToRightSide("btn_edit");

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// static
LLPanelBodyPartsListItem* LLPanelBodyPartsListItem::create(LLViewerInventoryItem* item)
{
	LLPanelBodyPartsListItem* list_item = NULL;
	if(item)
	{
		list_item = new LLPanelBodyPartsListItem(item);
		list_item->init();
	}
	return list_item;
}

LLPanelBodyPartsListItem::LLPanelBodyPartsListItem(LLViewerInventoryItem* item)
: LLPanelWearableListItem(item)
{
}

LLPanelBodyPartsListItem::~LLPanelBodyPartsListItem()
{
}

void LLPanelBodyPartsListItem::init()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_body_parts_list_item.xml");
}

BOOL LLPanelBodyPartsListItem::postBuild()
{
	LLPanelInventoryListItemBase::postBuild();

	addWidgetToRightSide("btn_lock");
	addWidgetToRightSide("btn_edit");

	return TRUE;
}


// static
LLPanelDeletableWearableListItem* LLPanelDeletableWearableListItem::create(LLViewerInventoryItem* item)
{
	LLPanelDeletableWearableListItem* list_item = NULL;
	if(item)
	{
		list_item = new LLPanelDeletableWearableListItem(item);
		list_item->init();
	}
	return list_item;
}

LLPanelDeletableWearableListItem::LLPanelDeletableWearableListItem(LLViewerInventoryItem* item)
: LLPanelWearableListItem(item)
{
}

void LLPanelDeletableWearableListItem::init()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_deletable_wearable_list_item.xml");
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


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelDummyClothingListItem* LLPanelDummyClothingListItem::create(LLWearableType::EType w_type)
{
	LLPanelDummyClothingListItem* list_item = new LLPanelDummyClothingListItem(w_type);
	list_item->init();
	return list_item;
}

void LLPanelDummyClothingListItem::updateItem()
{
	std::string title = wearableTypeToString(mWearableType);
	setTitle(title, LLStringUtil::null);
}

BOOL LLPanelDummyClothingListItem::postBuild()
{
	LLIconCtrl* icon = getChild<LLIconCtrl>("item_icon");
	setIconCtrl(icon);
	setTitleCtrl(getChild<LLTextBox>("item_name"));

	addWidgetToRightSide("btn_add");

	setIconImage(LLInventoryIcon::getIcon(LLAssetType::AT_CLOTHING, LLInventoryType::IT_NONE, FALSE, mWearableType, FALSE));
	updateItem();

	// Make it look loke clothing item - reserve space for 'delete' button
	setLeftWidgetsWidth(icon->getRect().mLeft);

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}

LLPanelDummyClothingListItem::LLPanelDummyClothingListItem(LLWearableType::EType w_type)
 : LLPanelWearableListItem(NULL)
 , mWearableType(w_type)
{
}

void LLPanelDummyClothingListItem::init()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_dummy_clothing_list_item.xml");
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

	if (item_type_order1 & TLO_NOT_CLOTHING)
	{
		// If both items are of the same asset type except AT_CLOTHING
		// we can compare them by name.
		return LLWearableItemNameComparator::doCompare(wearable_item1, wearable_item2);
	}

	const LLWearableType::EType item_wearable_type1 = wearable_item1->getWearableType();
	const LLWearableType::EType item_wearable_type2 = wearable_item2->getWearableType();

	if (item_wearable_type1 != item_wearable_type2)
	{
		// If items are of different clothing types they are compared
		// by clothing types order determined in LLWearableType::EType.
		return item_wearable_type1 < item_wearable_type2;
	}
	else
	{
		// If both items are of the same clothing type they are compared
		// by description and place in reverse order i.e. outer layer item
		// on top.
		return wearable_item1->getDescription() > wearable_item2->getDescription();
	}
}

// static
LLWearableItemTypeNameComparator::ETypeListOrder LLWearableItemTypeNameComparator::getTypeListOrder(LLAssetType::EType item_type)
{
	switch (item_type)
	{
	case LLAssetType::AT_OBJECT:
		return TLO_ATTACHMENT;

	case LLAssetType::AT_CLOTHING:
		return TLO_CLOTHING;

	case LLAssetType::AT_BODYPART:
		return TLO_BODYPART;

	default:
		return TLO_UNKNOWN;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static const LLWearableItemTypeNameComparator WEARABLE_TYPE_NAME_COMPARATOR;

static const LLDefaultChildRegistry::Register<LLWearableItemsList> r("wearable_items_list");

LLWearableItemsList::Params::Params()
{}

LLWearableItemsList::LLWearableItemsList(const LLWearableItemsList::Params& p)
:	LLInventoryItemsList(p)
{
	setComparator(&WEARABLE_TYPE_NAME_COMPARATOR);
}

// virtual
LLWearableItemsList::~LLWearableItemsList()
{}

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

	refreshList(item_array);
}

// EOF
