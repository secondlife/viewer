/** 
 * @file llflatlistview.cpp
 * @brief LLFlatListView base class
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

#include "linden_common.h"

#include "llpanel.h"

#include "llflatlistview.h"

static const LLDefaultChildRegistry::Register<LLFlatListView> flat_list_view("flat_list_view");

const LLSD SELECTED_EVENT = LLSD().insert("selected", true);
const LLSD UNSELECTED_EVENT = LLSD().insert("selected", false);

LLFlatListView::Params::Params()
:	item_pad("item_pad"),
	allow_select("allow_select"),
	multi_select("multi_select"),
	keep_one_selected("keep_one_selected")
{};

void LLFlatListView::reshape(S32 width, S32 height, BOOL called_from_parent /* = TRUE */)
{
	LLScrollContainer::reshape(width, height, called_from_parent);
	setItemsNoScrollWidth(width);
	rearrangeItems();
}

bool LLFlatListView::addItem(LLPanel* item, LLSD value /* = LLUUID::null*/, EAddPosition pos /*= ADD_BOTTOM*/)
{
	if (!item) return false;
	if (value.isUndefined()) return false;
	
	//force uniqueness of items, easiest check but unreliable
	if (item->getParent() == mItemsPanel) return false;
	
	item_pair_t* new_pair = new item_pair_t(item, value);
	switch (pos)
	{
	case ADD_TOP:
		mItemPairs.push_front(new_pair);
		//in LLView::draw() children are iterated in backorder
		mItemsPanel->addChildInBack(item);
		break;
	case ADD_BOTTOM:
		mItemPairs.push_back(new_pair);
		mItemsPanel->addChild(item);
		break;
	default:
		break;
	}
	
	//_4 is for MASK
	item->setMouseDownCallback(boost::bind(&LLFlatListView::onItemMouseClick, this, new_pair, _4));
	item->setRightMouseDownCallback(boost::bind(&LLFlatListView::onItemMouseClick, this, new_pair, _4));

	rearrangeItems();
	return true;
}


bool LLFlatListView::insertItemAfter(LLPanel* after_item, LLPanel* item_to_add, LLSD value /*= LLUUID::null*/)
{
	if (!after_item) return false;
	if (!item_to_add) return false;
	if (value.isUndefined()) return false;

	if (mItemPairs.empty()) return false;

	//force uniqueness of items, easiest check but unreliable
	if (item_to_add->getParent() == mItemsPanel) return false;

	item_pair_t* after_pair = getItemPair(after_item);
	if (!after_pair) return false;

	item_pair_t* new_pair = new item_pair_t(item_to_add, value);
	if (after_pair == mItemPairs.back())
	{
		mItemPairs.push_back(new_pair);
		mItemsPanel->addChild(item_to_add);
	}
	else
	{
		pairs_iterator_t it = mItemPairs.begin();
		++it;
		while (it != mItemPairs.end())
		{
			if (*it == after_pair)
			{
				mItemPairs.insert(++it, new_pair);
				mItemsPanel->addChild(item_to_add);
				break;
			}
		}
	}

	//_4 is for MASK
	item_to_add->setMouseDownCallback(boost::bind(&LLFlatListView::onItemMouseClick, this, new_pair, _4));
	item_to_add->setRightMouseDownCallback(boost::bind(&LLFlatListView::onItemMouseClick, this, new_pair, _4));

	rearrangeItems();
	return true;
}


bool LLFlatListView::removeItem(LLPanel* item)
{
	if (!item) return false;
	if (item->getParent() != mItemsPanel) return false;
	
	item_pair_t* item_pair = getItemPair(item);
	if (!item_pair) return false;

	return removeItemPair(item_pair);
}

bool LLFlatListView::removeItemByValue(const LLSD& value)
{
	if (value.isUndefined()) return false;
	
	item_pair_t* item_pair = getItemPair(value);
	if (!item_pair) return false;

	return removeItemPair(item_pair);
}

bool LLFlatListView::removeItemByUUID(LLUUID& uuid)
{
	return removeItemByValue(LLSD(uuid));
}

LLPanel* LLFlatListView::getItemByValue(LLSD& value) const
{
	if (value.isDefined()) return NULL;

	item_pair_t* pair = getItemPair(value);
	if (pair) return pair->first;
	return NULL;
}

bool LLFlatListView::selectItem(LLPanel* item, bool select /*= true*/)
{
	if (!item) return false;
	if (item->getParent() != mItemsPanel) return false;
	
	item_pair_t* item_pair = getItemPair(item);
	if (!item_pair) return false;

	return selectItemPair(item_pair, select);
}

bool LLFlatListView::selectItemByValue(const LLSD& value, bool select /*= true*/)
{
	if (value.isUndefined()) return false;

	item_pair_t* item_pair = getItemPair(value);
	if (!item_pair) return false;

	return selectItemPair(item_pair, select);
}

bool LLFlatListView::selectItemByUUID(LLUUID& uuid, bool select /* = true*/)
{
	return selectItemByValue(LLSD(uuid), select);
}


LLSD LLFlatListView::getSelectedValue() const
{
	if (mSelectedItemPairs.empty()) return LLSD();

	item_pair_t* first_selected_pair = mSelectedItemPairs.front();
	return first_selected_pair->second;
}

void LLFlatListView::getSelectedValues(std::vector<LLSD>& selected_values) const
{
	if (mSelectedItemPairs.empty()) return;

	for (pairs_const_iterator_t it = mSelectedItemPairs.begin(); it != mSelectedItemPairs.end(); ++it)
	{
		selected_values.push_back((*it)->second);
	}
}

LLUUID LLFlatListView::getSelectedUUID() const
{
	const LLSD& value = getSelectedValue();
	if (value.isDefined() && value.isUUID())
	{
		return value.asUUID();
	}
	else 
	{
		return LLUUID::null;
	}
}

void LLFlatListView::getSelectedUUIDs(std::vector<LLUUID>& selected_uuids) const
{
	if (mSelectedItemPairs.empty()) return;

	for (pairs_const_iterator_t it = mSelectedItemPairs.begin(); it != mSelectedItemPairs.end(); ++it)
	{
		selected_uuids.push_back((*it)->second.asUUID());
	}
}

LLPanel* LLFlatListView::getSelectedItem() const
{
	if (mSelectedItemPairs.empty()) return NULL;

	return mSelectedItemPairs.front()->first;
}

void LLFlatListView::getSelectedItems(std::vector<LLPanel*>& selected_items) const
{
	if (mSelectedItemPairs.empty()) return;

	for (pairs_const_iterator_t it = mSelectedItemPairs.begin(); it != mSelectedItemPairs.end(); ++it)
	{
		selected_items.push_back((*it)->first);
	}
}

void LLFlatListView::resetSelection()
{
	if (mSelectedItemPairs.empty()) return;

	for (pairs_iterator_t it= mSelectedItemPairs.begin(); it != mSelectedItemPairs.end(); ++it)
	{
		item_pair_t* pair_to_deselect = *it;
		LLPanel* item = pair_to_deselect->first;
		item->setValue(UNSELECTED_EVENT);
	}

	mSelectedItemPairs.clear();
}

void LLFlatListView::clear()
{
	// do not use LLView::deleteAllChildren to avoid removing nonvisible items. drag-n-drop for ex.
	for (pairs_iterator_t it = mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		mItemsPanel->removeChild((*it)->first);
		delete (*it)->first;
		delete *it;
	}
	mItemPairs.clear();
	mSelectedItemPairs.clear();
}


//////////////////////////////////////////////////////////////////////////
// PROTECTED STUFF
//////////////////////////////////////////////////////////////////////////


LLFlatListView::LLFlatListView(const LLFlatListView::Params& p)
:	LLScrollContainer(p),
	mItemsPanel(NULL),
	mItemPad(p.item_pad),
	mAllowSelection(p.allow_select),
	mMultipleSelection(p.multi_select),
	mKeepOneItemSelected(p.keep_one_selected)
{
	mBorderThickness = getBorderWidth();

	LLRect scroll_rect = getRect();
	LLRect items_rect;

	setItemsNoScrollWidth(scroll_rect.getWidth());
	items_rect.setLeftTopAndSize(mBorderThickness, scroll_rect.getHeight() - mBorderThickness, mItemsNoScrollWidth, 0);

	LLPanel::Params pp;
	pp.rect(items_rect);
	mItemsPanel = LLUICtrlFactory::create<LLPanel> (pp);
	addChild(mItemsPanel);

	//we don't need to stretch in vertical direction on reshaping by a parent
	//no bottom following!
	mItemsPanel->setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_TOP);
};

void LLFlatListView::rearrangeItems()
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	if (mItemPairs.empty()) return;

	//calculating required height - assuming items can be of different height
	//list should accommodate all its items
	S32 height = 0;

	pairs_iterator_t it = mItemPairs.begin();
	for (; it != mItemPairs.end(); ++it)
	{
		LLPanel* item = (*it)->first;
		height += item->getRect().getHeight();
	}
	height += mItemPad * (mItemPairs.size() - 1);

	LLRect rc = mItemsPanel->getRect();
	S32 width = mItemsNoScrollWidth;

	// update width to avoid horizontal scrollbar
	if (height > getRect().getHeight() - 2 * mBorderThickness)
		width -= scrollbar_size;

	//changes the bottom, end of the list goes down in the scroll container
	rc.setLeftTopAndSize(rc.mLeft, rc.mTop, width, height);
	mItemsPanel->setRect(rc);

	//reshaping items
	S32 item_new_top = height;
	pairs_iterator_t it2, first_it = mItemPairs.begin();
	for (it2 = first_it; it2 != mItemPairs.end(); ++it2)
	{
		LLPanel* item = (*it2)->first;
		LLRect rc = item->getRect();
		if(it2 != first_it)
		{
			item_new_top -= (rc.getHeight() + mItemPad);
		}
		rc.setLeftTopAndSize(rc.mLeft, item_new_top, width, rc.getHeight());
		item->reshape(rc.getWidth(), rc.getHeight());
		item->setRect(rc);
	}
}

void LLFlatListView::onItemMouseClick(item_pair_t* item_pair, MASK mask)
{
	if (!item_pair) return;
	
	bool select_item = !isSelected(item_pair);

	//*TODO find a better place for that enforcing stuff
	if (mKeepOneItemSelected && numSelected() == 1 && !select_item) return;
	
	if (!(mask & MASK_CONTROL) || !mMultipleSelection) resetSelection();
	selectItemPair(item_pair, select_item);
}

LLFlatListView::item_pair_t* LLFlatListView::getItemPair(LLPanel* item) const
{
	llassert(item);

	for (pairs_const_iterator_t it= mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		item_pair_t* item_pair = *it;
		if (item_pair->first == item) return item_pair;
	}
	return NULL;
}

//compares two LLSD's
bool llsds_are_equal(const LLSD& llsd_1, const LLSD& llsd_2)
{
	llassert(llsd_1.isDefined());
	llassert(llsd_2.isDefined());
	
	if (llsd_1.type() != llsd_2.type()) return false;

	if (!llsd_1.isMap())
	{
		if (llsd_1.isUUID()) return llsd_1.asUUID() == llsd_2.asUUID();

		//assumptions that string representaion is enough for other types
		return llsd_1.asString() == llsd_2.asString();
	}

	if (llsd_1.size() != llsd_2.size()) return false;

	LLSD::map_const_iterator llsd_1_it = llsd_1.beginMap();
	LLSD::map_const_iterator llsd_2_it = llsd_2.beginMap();
	for (S32 i = 0; i < llsd_1.size(); ++i)
	{
		if ((*llsd_1_it).first != (*llsd_2_it).first) return false;
		if (!llsds_are_equal((*llsd_1_it).second, (*llsd_2_it).second)) return false;
		++llsd_1_it;
		++llsd_2_it;
	}
	return true;
}

LLFlatListView::item_pair_t* LLFlatListView::getItemPair(const LLSD& value) const
{
	llassert(value.isDefined());
	
	for (pairs_const_iterator_t it= mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		item_pair_t* item_pair = *it;
		if (llsds_are_equal(item_pair->second, value)) return item_pair;
	}
	return NULL;
}

bool LLFlatListView::selectItemPair(item_pair_t* item_pair, bool select)
{
	llassert(item_pair);

	if (!mAllowSelection && select) return false;

	if (isSelected(item_pair) == select) return true; //already in specified selection state
	if (select)
	{
		mSelectedItemPairs.push_back(item_pair);
	}
	else
	{
		mSelectedItemPairs.remove(item_pair);
	}

	//a way of notifying panel of selection state changes
	LLPanel* item = item_pair->first;
	item->setValue(select ? SELECTED_EVENT : UNSELECTED_EVENT);
	return true;
}

bool LLFlatListView::isSelected(item_pair_t* item_pair) const
{
	llassert(item_pair);

	pairs_const_iterator_t it_end = mSelectedItemPairs.end();
	return std::find(mSelectedItemPairs.begin(), it_end, item_pair) != it_end;
}

bool LLFlatListView::removeItemPair(item_pair_t* item_pair)
{
	llassert(item_pair);

	bool deleted = false;
	for (pairs_iterator_t it = mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		item_pair_t* _item_pair = *it;
		if (_item_pair == item_pair)
		{
			mItemPairs.erase(it);
			deleted = true;
			break;
		}
	}

	if (!deleted) return false;

	for (pairs_iterator_t it = mSelectedItemPairs.begin(); it != mSelectedItemPairs.end(); ++it)
	{
		item_pair_t* selected_item_pair = *it;
		if (selected_item_pair == item_pair)
		{
			it = mSelectedItemPairs.erase(it);
			break;
		}
	}

	mItemsPanel->removeChild(item_pair->first);
	delete item_pair->first;
	delete item_pair;

	rearrangeItems();

	return true;
}


