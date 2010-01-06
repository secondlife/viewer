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
#include "lltextbox.h"

#include "llflatlistview.h"

static const LLDefaultChildRegistry::Register<LLFlatListView> flat_list_view("flat_list_view");

const LLSD SELECTED_EVENT	= LLSD().with("selected", true);
const LLSD UNSELECTED_EVENT	= LLSD().with("selected", false);

static const std::string COMMENT_TEXTBOX = "comment_text";

//forward declaration
bool llsds_are_equal(const LLSD& llsd_1, const LLSD& llsd_2);

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

const LLRect& LLFlatListView::getItemsRect() const
{
	return mItemsPanel->getRect(); 
}

bool LLFlatListView::addItem(LLPanel * item, const LLSD& value /*= LLUUID::null*/, EAddPosition pos /*= ADD_BOTTOM*/,bool rearrange /*= true*/)
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
	item->setRightMouseDownCallback(boost::bind(&LLFlatListView::onItemRightMouseClick, this, new_pair, _4));

	// Children don't accept the focus
	item->setTabStop(false);

	if (rearrange)
	{
		rearrangeItems();
		notifyParentItemsRectChanged();
	}
	return true;
}


bool LLFlatListView::insertItemAfter(LLPanel* after_item, LLPanel* item_to_add, const LLSD& value /*= LLUUID::null*/)
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
		for (; it != mItemPairs.end(); ++it)
		{
			if (*it == after_pair)
			{
				// insert new elements before the element at position of passed iterator.
				mItemPairs.insert(++it, new_pair);
				mItemsPanel->addChild(item_to_add);
				break;
			}
		}
	}

	//_4 is for MASK
	item_to_add->setMouseDownCallback(boost::bind(&LLFlatListView::onItemMouseClick, this, new_pair, _4));
	item_to_add->setRightMouseDownCallback(boost::bind(&LLFlatListView::onItemRightMouseClick, this, new_pair, _4));

	rearrangeItems();
	notifyParentItemsRectChanged();
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

bool LLFlatListView::removeItemByUUID(const LLUUID& uuid)
{
	return removeItemByValue(LLSD(uuid));
}

LLPanel* LLFlatListView::getItemByValue(const LLSD& value) const
{
	if (value.isUndefined()) return NULL;

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

bool LLFlatListView::selectItemByUUID(const LLUUID& uuid, bool select /* = true*/)
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

void LLFlatListView::resetSelection(bool no_commit_on_deselection /*= false*/)
{
	if (mSelectedItemPairs.empty()) return;

	for (pairs_iterator_t it= mSelectedItemPairs.begin(); it != mSelectedItemPairs.end(); ++it)
	{
		item_pair_t* pair_to_deselect = *it;
		LLPanel* item = pair_to_deselect->first;
		item->setValue(UNSELECTED_EVENT);
	}

	mSelectedItemPairs.clear();

	if (mCommitOnSelectionChange && !no_commit_on_deselection)
	{
		onCommit();
	}

	// Stretch selected items rect to ensure it won't be clipped
	mSelectedItemsBorder->setRect(getSelectedItemsRect().stretch(-1));
}

void LLFlatListView::setNoItemsCommentText(const std::string& comment_text)
{
	if (NULL == mNoItemsCommentTextbox)
	{
		LLRect comment_rect = getRect();
		comment_rect.setOriginAndSize(0, 0, comment_rect.getWidth(), comment_rect.getHeight());
		comment_rect.stretch(-getBorderWidth());
		LLTextBox::Params text_p;
		text_p.name(COMMENT_TEXTBOX);
		text_p.border_visible(false);
		text_p.rect(comment_rect);
		text_p.follows.flags(FOLLOWS_ALL);
		mNoItemsCommentTextbox = LLUICtrlFactory::create<LLTextBox>(text_p, this);
	}

	mNoItemsCommentTextbox->setValue(comment_text);
}

void LLFlatListView::clear()
{
	// do not use LLView::deleteAllChildren to avoid removing nonvisible items. drag-n-drop for ex.
	for (pairs_iterator_t it = mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		mItemsPanel->removeChild((*it)->first);
		(*it)->first->die();
		delete *it;
	}
	mItemPairs.clear();
	mSelectedItemPairs.clear();

	// also set items panel height to zero. Reshape it to allow reshaping of non-item children
	LLRect rc = mItemsPanel->getRect();
	rc.mBottom = rc.mTop;
	mItemsPanel->reshape(rc.getWidth(), rc.getHeight());
	mItemsPanel->setRect(rc);

	setNoItemsCommentVisible(true);
	notifyParentItemsRectChanged();
}

void LLFlatListView::sort()
{
	if (!mItemComparator)
	{
		llwarns << "No comparator specified for sorting FlatListView items." << llendl;
		return;
	}

	mItemPairs.sort(ComparatorAdaptor(*mItemComparator));
	rearrangeItems();
}

bool LLFlatListView::updateValue(const LLSD& old_value, const LLSD& new_value)
{
	if (old_value.isUndefined() || new_value.isUndefined()) return false;
	if (llsds_are_equal(old_value, new_value)) return false;

	item_pair_t* item_pair = getItemPair(old_value);
	if (!item_pair) return false;

	item_pair->second = new_value;
	return true;
}

//////////////////////////////////////////////////////////////////////////
// PROTECTED STUFF
//////////////////////////////////////////////////////////////////////////


LLFlatListView::LLFlatListView(const LLFlatListView::Params& p)
:	LLScrollContainer(p)
  , mItemComparator(NULL)
  , mItemsPanel(NULL)
  , mItemPad(p.item_pad)
  , mAllowSelection(p.allow_select)
  , mMultipleSelection(p.multi_select)
  , mKeepOneItemSelected(p.keep_one_selected)
  , mCommitOnSelectionChange(false)
  , mPrevNotifyParentRect(LLRect())
  , mNoItemsCommentTextbox(NULL)
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

	LLViewBorder::Params params;
	params.name("scroll border");
	params.rect(getSelectedItemsRect());
	params.visible(false);
	params.bevel_style(LLViewBorder::BEVEL_IN);
	mSelectedItemsBorder = LLUICtrlFactory::create<LLViewBorder> (params);
	mItemsPanel->addChild( mSelectedItemsBorder );
};

// virtual
void LLFlatListView::draw()
{
	// Highlight border if a child of this container has keyboard focus
	if( mSelectedItemsBorder->getVisible() )
	{
		mSelectedItemsBorder->setKeyboardFocusHighlight( hasFocus() );
	}
	LLScrollContainer::draw();
}

// virtual
BOOL LLFlatListView::postBuild()
{
	setTabStop(true);
	return LLScrollContainer::postBuild();
}

void LLFlatListView::rearrangeItems()
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	setNoItemsCommentVisible(mItemPairs.empty());

	if (mItemPairs.empty()) return;

	//calculating required height - assuming items can be of different height
	//list should accommodate all its items
	S32 height = 0;

	S32 invisible_children_count = 0;
	pairs_iterator_t it = mItemPairs.begin();
	for (; it != mItemPairs.end(); ++it)
	{
		LLPanel* item = (*it)->first;

		// skip invisible child
		if (!item->getVisible())
		{
			++invisible_children_count;
			continue;
		}

		height += item->getRect().getHeight();
	}

	// add paddings between items, excluding invisible ones
	height += mItemPad * (mItemPairs.size() - invisible_children_count - 1);

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

		// skip invisible child
		if (!item->getVisible())
			continue;

		LLRect rc = item->getRect();
		rc.setLeftTopAndSize(rc.mLeft, item_new_top, width, rc.getHeight());
		item->reshape(rc.getWidth(), rc.getHeight());
		item->setRect(rc);

		// move top for next item in list
		item_new_top -= (rc.getHeight() + mItemPad);
	}

	// Stretch selected items rect to ensure it won't be clipped
	mSelectedItemsBorder->setRect(getSelectedItemsRect().stretch(-1));
}

void LLFlatListView::onItemMouseClick(item_pair_t* item_pair, MASK mask)
{
	if (!item_pair) return;

	setFocus(TRUE);
	
	bool select_item = !isSelected(item_pair);

	//*TODO find a better place for that enforcing stuff
	if (mKeepOneItemSelected && numSelected() == 1 && !select_item) return;
	
	if (!(mask & MASK_CONTROL) || !mMultipleSelection) resetSelection();
	selectItemPair(item_pair, select_item);
}

void LLFlatListView::onItemRightMouseClick(item_pair_t* item_pair, MASK mask)
{
	if (!item_pair)
		return;

	// Forbid deselecting of items on right mouse button click if mMultipleSelection flag is set on,
	// because some of derived classes may have context menu and selected items must be kept.
	if ( !(mask & MASK_CONTROL) && mMultipleSelection && isSelected(item_pair) )
		return;

	// else got same behavior as at onItemMouseClick
	onItemMouseClick(item_pair, mask);
}

BOOL LLFlatListView::handleKeyHere(KEY key, MASK mask)
{
	BOOL reset_selection = (mask != MASK_SHIFT);
	BOOL handled = FALSE;
	switch (key)
	{
		case KEY_RETURN:
		{
			if (mSelectedItemPairs.size() && mask == MASK_NONE)
			{
				mOnReturnSignal(this, getValue());
				handled = TRUE;
			}
			break;
		}
		case KEY_UP:
		{
			if ( !selectNextItemPair(true, reset_selection) && reset_selection)
			{
				// If case we are in accordion tab notify parent to go to the previous accordion
				if(notifyParent(LLSD().with("action","select_prev")) > 0 )//message was processed
					resetSelection();
			}
			break;
		}
		case KEY_DOWN:
		{
			if ( !selectNextItemPair(false, reset_selection) && reset_selection)
			{
				// If case we are in accordion tab notify parent to go to the next accordion
				if( notifyParent(LLSD().with("action","select_next")) > 0 ) //message was processed
					resetSelection();
			}
			break;
		}
		case 'A':
		{
			if(MASK_CONTROL & mask)
			{
				selectAll();
				handled = TRUE;
			}
			break;
		}
		default:
			break;
	}

	if ( ( key == KEY_UP || key == KEY_DOWN ) && mSelectedItemPairs.size() )
	{
		ensureSelectedVisible();
		/*
		LLRect visible_rc = getVisibleContentRect();
		LLRect selected_rc = getLastSelectedItemRect();

		if ( !visible_rc.contains (selected_rc) )
		{
			// But scroll in Items panel coordinates
			scrollToShowRect(selected_rc);
		}

		// In case we are in accordion tab notify parent to show selected rectangle
		LLRect screen_rc;
		localRectToScreen(selected_rc, &screen_rc);
		notifyParent(LLSD().with("scrollToShowRect",screen_rc.getValue()));*/

		handled = TRUE;
	}

	return handled ? handled : LLScrollContainer::handleKeyHere(key, mask);
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

	if (mCommitOnSelectionChange)
	{
		onCommit();
	}

	// Stretch selected items rect to ensure it won't be clipped
	mSelectedItemsBorder->setRect(getSelectedItemsRect().stretch(-1));

	return true;
}

LLRect LLFlatListView::getLastSelectedItemRect()
{
	if (!mSelectedItemPairs.size())
	{
		return LLRect::null;
	}

	return mSelectedItemPairs.back()->first->getRect();
}

LLRect LLFlatListView::getSelectedItemsRect()
{
	if (!mSelectedItemPairs.size())
	{
		return LLRect::null;
	}
	LLRect rc = getLastSelectedItemRect();
	for ( pairs_const_iterator_t
			  it = mSelectedItemPairs.begin(),
			  it_end = mSelectedItemPairs.end();
		  it != it_end; ++it )
	{
		rc.unionWith((*it)->first->getRect());
	}
	return rc;
}

void LLFlatListView::selectFirstItem	()
{
	selectItemPair(mItemPairs.front(), true);
	ensureSelectedVisible();
}

void LLFlatListView::selectLastItem		()
{
	selectItemPair(mItemPairs.back(), true);
	ensureSelectedVisible();
}

void LLFlatListView::ensureSelectedVisible()
{
	LLRect visible_rc = getVisibleContentRect();
	LLRect selected_rc = getLastSelectedItemRect();

	if ( !visible_rc.contains (selected_rc) )
	{
		// But scroll in Items panel coordinates
		scrollToShowRect(selected_rc);
	}

	// In case we are in accordion tab notify parent to show selected rectangle
	LLRect screen_rc;
	localRectToScreen(selected_rc, &screen_rc);
	notifyParent(LLSD().with("scrollToShowRect",screen_rc.getValue()));
}


// virtual
bool LLFlatListView::selectNextItemPair(bool is_up_direction, bool reset_selection)
{
	// No items - no actions!
	if ( !mItemPairs.size() )
		return false;

	
	item_pair_t* to_sel_pair = NULL;
	item_pair_t* cur_sel_pair = NULL;
	if ( mSelectedItemPairs.size() )
	{
		// Take the last selected pair
		cur_sel_pair = mSelectedItemPairs.back();
		// Bases on given direction choose next item to select
		if ( is_up_direction )
		{
			// Find current selected item position in mItemPairs list
			pairs_list_t::reverse_iterator sel_it = std::find(mItemPairs.rbegin(), mItemPairs.rend(), cur_sel_pair);

			for (;++sel_it != mItemPairs.rend();)
			{
				// skip invisible items
				if ( (*sel_it)->first->getVisible() )
				{
					to_sel_pair = *sel_it;
					break;
				}
			}
		}
		else
		{
			// Find current selected item position in mItemPairs list
			pairs_list_t::iterator sel_it = std::find(mItemPairs.begin(), mItemPairs.end(), cur_sel_pair);

			for (;++sel_it != mItemPairs.end();)
			{
				// skip invisible items
				if ( (*sel_it)->first->getVisible() )
				{
					to_sel_pair = *sel_it;
					break;
				}
			}
		}
	}
	else
	{
		// If there weren't selected items then choose the first one bases on given direction
		cur_sel_pair = (is_up_direction) ? mItemPairs.back() : mItemPairs.front();
		// Force selection to first item
		to_sel_pair = cur_sel_pair;
	}


	if ( to_sel_pair )
	{
		bool select = true;

		if ( reset_selection )
		{
			// Reset current selection if we were asked about it
			resetSelection();
		}
		else
		{
			// If item already selected and no reset request than we should deselect last selected item.
			select = (mSelectedItemPairs.end() == std::find(mSelectedItemPairs.begin(), mSelectedItemPairs.end(), to_sel_pair));
		}

		// Select/Deselect next item
		selectItemPair(select ? to_sel_pair : cur_sel_pair, select);

		return true;
	}
	return false;
}

bool LLFlatListView::selectAll()
{
	if (!mAllowSelection)
		return false;

	mSelectedItemPairs.clear();

	for (pairs_const_iterator_t it= mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		item_pair_t* item_pair = *it;
		mSelectedItemPairs.push_back(item_pair);
		//a way of notifying panel of selection state changes
		LLPanel* item = item_pair->first;
		item->setValue(SELECTED_EVENT);
	}

	if (mCommitOnSelectionChange)
	{
		onCommit();
	}

	// Stretch selected items rect to ensure it won't be clipped
	mSelectedItemsBorder->setRect(getSelectedItemsRect().stretch(-1));

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
	item_pair->first->die();
	delete item_pair;

	rearrangeItems();
	notifyParentItemsRectChanged();

	return true;
}

void LLFlatListView::notifyParentItemsRectChanged()
{
	S32 comment_height = 0;

	// take into account comment text height if exists
	if (mNoItemsCommentTextbox && mNoItemsCommentTextbox->getVisible())
	{
		comment_height = mNoItemsCommentTextbox->getTextPixelHeight();
	}

	LLRect req_rect =  getItemsRect();

	// get maximum of items total height and comment text height
	req_rect.setOriginAndSize(req_rect.mLeft, req_rect.mBottom, req_rect.getWidth(), llmax(req_rect.getHeight(), comment_height));

	// take into account border size.
	req_rect.stretch(getBorderWidth());

	if (req_rect == mPrevNotifyParentRect)
		return;

	mPrevNotifyParentRect = req_rect;

	LLSD params;
	params["action"] = "size_changes";
	params["width"] = req_rect.getWidth();
	params["height"] = req_rect.getHeight();

	if (getParent()) // dummy widgets don't have a parent
		getParent()->notifyParent(params);
}

void LLFlatListView::setNoItemsCommentVisible(bool visible) const
{
	if (mNoItemsCommentTextbox)
	{
		if (visible)
		{
			// We have to update child rect here because of issues with rect after reshaping while creating LLTextbox
			// It is possible to have invalid LLRect if Flat List is in LLAccordionTab
			LLRect comment_rect = getLocalRect();

			// To see comment correctly (EXT - 3244) in mNoItemsCommentTextbox we must get border width
			// of LLFlatListView (@see getBorderWidth()) and stretch mNoItemsCommentTextbox to this width
			// But getBorderWidth() returns 0 if LLFlatListView not visible. So we have to get border width
			// from 'scroll_border'
			LLViewBorder* scroll_border = getChild<LLViewBorder>("scroll border");
			comment_rect.stretch(-scroll_border->getBorderWidth());
			mNoItemsCommentTextbox->setRect(comment_rect);
		}
		mNoItemsCommentTextbox->setVisible(visible);
	}
}

void LLFlatListView::getItems(std::vector<LLPanel*>& items) const
{
	if (mItemPairs.empty()) return;

	items.clear();
	for (pairs_const_iterator_t it = mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		items.push_back((*it)->first);
	}
}

void LLFlatListView::getValues(std::vector<LLSD>& values) const
{
	if (mItemPairs.empty()) return;

	values.clear();
	for (pairs_const_iterator_t it = mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		values.push_back((*it)->second);
	}
}

// virtual
void LLFlatListView::onFocusReceived()
{
	mSelectedItemsBorder->setVisible(TRUE);
}
// virtual
void LLFlatListView::onFocusLost()
{
	mSelectedItemsBorder->setVisible(FALSE);
}

//virtual 
S32 LLFlatListView::notify(const LLSD& info)
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "select_first")
		{
			setFocus(true);
			selectFirstItem();
			return 1;
		}
		else if(str_action == "select_last")
		{
			setFocus(true);
			selectLastItem();
			return 1;
		}
	}
	else if (info.has("rearrange"))
	{
		rearrangeItems();
		notifyParentItemsRectChanged();
		return 1;
	}
	return 0;
}

void LLFlatListView::detachItems(std::vector<LLPanel*>& detached_items)
{
	LLSD action;
	action.with("detach", LLSD());
	// Clear detached_items list
	detached_items.clear();
	// Go through items and detach valid items, remove them from items panel
	// and add to detached_items.
	for (pairs_iterator_t
			 iter = mItemPairs.begin(),
			 iter_end = mItemPairs.end();
		 iter != iter_end; ++iter)
	{
		LLPanel* pItem = (*iter)->first;
		if (1 == pItem->notify(action))
		{
			selectItemPair((*iter), false);
			mItemsPanel->removeChild(pItem);
			detached_items.push_back(pItem);
		}
	}
	if (!detached_items.empty())
	{
		// Some items were detached, clean ourself from unusable memory
		if (detached_items.size() == mItemPairs.size())
		{
			// This way will be faster if all items were disconnected
			for (pairs_iterator_t
					 iter = mItemPairs.begin(),
					 iter_end = mItemPairs.end();
				 iter != iter_end; ++iter)
			{
				(*iter)->first = NULL;
				delete *iter;
			}
			mItemPairs.clear();
			// Also set items panel height to zero.
			// Reshape it to allow reshaping of non-item children.
			LLRect rc = mItemsPanel->getRect();
			rc.mBottom = rc.mTop;
			mItemsPanel->reshape(rc.getWidth(), rc.getHeight());
			mItemsPanel->setRect(rc);
			setNoItemsCommentVisible(true);
		}
		else
		{
			for (std::vector<LLPanel*>::const_iterator
					 detached_iter = detached_items.begin(),
					 detached_iter_end = detached_items.end();
				 detached_iter != detached_iter_end; ++detached_iter)
			{
				LLPanel* pDetachedItem = *detached_iter;
				for (pairs_iterator_t
						 iter = mItemPairs.begin(),
						 iter_end = mItemPairs.end();
					 iter != iter_end; ++iter)
				{
					item_pair_t* item_pair = *iter;
					if (item_pair->first == pDetachedItem)
					{
						mItemPairs.erase(iter);
						item_pair->first = NULL;
						delete item_pair;
						break;
					}
				}
			}
			rearrangeItems();
		}
		notifyParentItemsRectChanged();
	}
}

//EOF
