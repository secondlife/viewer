/** 
 * @file llflatlistview.cpp
 * @brief LLFlatListView base class and extension to support messages for several cases of an empty list.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "linden_common.h"

#include "llpanel.h"
#include "lltextbox.h"

#include "llflatlistview.h"

static const LLDefaultChildRegistry::Register<LLFlatListView> flat_list_view("flat_list_view");

const LLSD SELECTED_EVENT	= LLSD().with("selected", true);
const LLSD UNSELECTED_EVENT	= LLSD().with("selected", false);

//forward declaration
bool llsds_are_equal(const LLSD& llsd_1, const LLSD& llsd_2);

LLFlatListView::Params::Params()
:	item_pad("item_pad"),
	allow_select("allow_select"),
	multi_select("multi_select"),
	keep_one_selected("keep_one_selected"),
	keep_selection_visible_on_reshape("keep_selection_visible_on_reshape",false),
	no_items_text("no_items_text")
{};

void LLFlatListView::reshape(S32 width, S32 height, BOOL called_from_parent /* = TRUE */)
{
	S32 delta = height - getRect().getHeight();
	LLScrollContainer::reshape(width, height, called_from_parent);
	setItemsNoScrollWidth(width);
	rearrangeItems();
	
	if(delta!= 0 && mKeepSelectionVisibleOnReshape)
	{
		ensureSelectedVisible();
	}
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

bool LLFlatListView::addItemPairs(pairs_list_t panel_list, bool rearrange /*= true*/)
{
    if (!mItemComparator)
    {
        LL_WARNS_ONCE() << "No comparator specified for inserting FlatListView items." << LL_ENDL;
        return false;
    }
    if (panel_list.size() == 0)
    {
        return false;
    }

    // presort list so that it will be easier to sort elements into mItemPairs
    panel_list.sort(ComparatorAdaptor(*mItemComparator));

    pairs_const_iterator_t new_pair_it = panel_list.begin();
    item_pair_t* new_pair = *new_pair_it;
    pairs_iterator_t pair_it = mItemPairs.begin();
    item_pair_t* item_pair = *pair_it;

    // sort panel_list into mItemPars
    while (new_pair_it != panel_list.end() && pair_it != mItemPairs.end())
    {
        if (!new_pair->first || new_pair->first->getParent() == mItemsPanel)
        {
            // iterator already used or we are reusing existing panel
            new_pair_it++;
            new_pair = *new_pair_it;
        }
        else if (mItemComparator->compare(new_pair->first, item_pair->first))
        {
            LLPanel* panel = new_pair->first;

            mItemPairs.insert(pair_it, new_pair);
            mItemsPanel->addChild(panel);

            //_4 is for MASK
            panel->setMouseDownCallback(boost::bind(&LLFlatListView::onItemMouseClick, this, new_pair, _4));
            panel->setRightMouseDownCallback(boost::bind(&LLFlatListView::onItemRightMouseClick, this, new_pair, _4));
            // Children don't accept the focus
            panel->setTabStop(false);
        }
        else
        {
            pair_it++;
            item_pair = *pair_it;
        }
    }

    // Add what is left of panel_list into the end of mItemPairs.
    for (; new_pair_it != panel_list.end(); ++new_pair_it)
    {
        item_pair_t* item_pair = *new_pair_it;
        LLPanel *panel = item_pair->first;
        if (panel && panel->getParent() != mItemsPanel)
        {
            mItemPairs.push_back(item_pair);
            mItemsPanel->addChild(panel);

            //_4 is for MASK
            panel->setMouseDownCallback(boost::bind(&LLFlatListView::onItemMouseClick, this, item_pair, _4));
            panel->setRightMouseDownCallback(boost::bind(&LLFlatListView::onItemRightMouseClick, this, item_pair, _4));
            // Children don't accept the focus
            panel->setTabStop(false);
        }
    }

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


bool LLFlatListView::removeItem(LLPanel* item, bool rearrange)
{
	if (!item) return false;
	if (item->getParent() != mItemsPanel) return false;
	
	item_pair_t* item_pair = getItemPair(item);
	if (!item_pair) return false;

	return removeItemPair(item_pair, rearrange);
}

bool LLFlatListView::removeItemByValue(const LLSD& value, bool rearrange)
{
	if (value.isUndefined()) return false;
	
	item_pair_t* item_pair = getItemPair(value);
	if (!item_pair) return false;

	return removeItemPair(item_pair, rearrange);
}

bool LLFlatListView::removeItemByUUID(const LLUUID& uuid, bool rearrange)
{
	return removeItemByValue(LLSD(uuid), rearrange);
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

void LLFlatListView::getSelectedUUIDs(uuid_vec_t& selected_uuids) const
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

	// Stretch selected item rect to ensure it won't be clipped
	mSelectedItemsBorder->setRect(getLastSelectedItemRect().stretch(-1));
}

void LLFlatListView::setNoItemsCommentText(const std::string& comment_text)
{
	mNoItemsCommentTextbox->setValue(comment_text);
}

U32 LLFlatListView::size(const bool only_visible_items) const
{
	if (only_visible_items)
	{
		U32 size = 0;
		for (pairs_const_iterator_t
				 iter = mItemPairs.begin(),
				 iter_end = mItemPairs.end();
			 iter != iter_end; ++iter)
		{
			if ((*iter)->first->getVisible())
				++size;
		}
		return size;
	}
	else
	{
		return mItemPairs.size();
	}
}

void LLFlatListView::clear()
{
	// This will clear mSelectedItemPairs, calling all appropriate callbacks.
	resetSelection();
	
	// do not use LLView::deleteAllChildren to avoid removing nonvisible items. drag-n-drop for ex.
	for (pairs_iterator_t it = mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		mItemsPanel->removeChild((*it)->first);
		(*it)->first->die();
		delete *it;
	}
	mItemPairs.clear();

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
		LL_WARNS() << "No comparator specified for sorting FlatListView items." << LL_ENDL;
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
  , mIsConsecutiveSelection(false)
  , mKeepSelectionVisibleOnReshape(p.keep_selection_visible_on_reshape)
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
	params.rect(getLastSelectedItemRect());
	params.visible(false);
	params.bevel_style(LLViewBorder::BEVEL_IN);
	mSelectedItemsBorder = LLUICtrlFactory::create<LLViewBorder> (params);
	mItemsPanel->addChild( mSelectedItemsBorder );

	{
		// create textbox for "No Items" comment text
		LLTextBox::Params text_p = p.no_items_text;
		if (!text_p.rect.isProvided())
		{
			LLRect comment_rect = getRect();
			comment_rect.setOriginAndSize(0, 0, comment_rect.getWidth(), comment_rect.getHeight());
			comment_rect.stretch(-getBorderWidth());
			text_p.rect(comment_rect);
		}
		text_p.border_visible(false);

		if (!text_p.follows.isProvided())
		{
			text_p.follows.flags(FOLLOWS_ALL);
		}
		mNoItemsCommentTextbox = LLUICtrlFactory::create<LLTextBox>(text_p, this);
	}
};

LLFlatListView::~LLFlatListView()
{
	for (pairs_iterator_t it = mItemPairs.begin(); it != mItemPairs.end(); ++it)
	{
		mItemsPanel->removeChild((*it)->first);
		(*it)->first->die();
		delete *it;
	}
	mItemPairs.clear();
}

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

	setNoItemsCommentVisible(0==size());

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

	// Stretch selected item rect to ensure it won't be clipped
	mSelectedItemsBorder->setRect(getLastSelectedItemRect().stretch(-1));
}

void LLFlatListView::onItemMouseClick(item_pair_t* item_pair, MASK mask)
{
	if (!item_pair) return;

	if (!item_pair->first) 
	{
		LL_WARNS() << "Attempt to selet an item pair containing null panel item" << LL_ENDL;
		return;
	}

	setFocus(TRUE);
	
	bool select_item = !isSelected(item_pair);

	//*TODO find a better place for that enforcing stuff
	if (mKeepOneItemSelected && numSelected() == 1 && !select_item) return;

	if ( (mask & MASK_SHIFT) && !(mask & MASK_CONTROL)
		 && mMultipleSelection && !mSelectedItemPairs.empty() )
	{
		item_pair_t* last_selected_pair = mSelectedItemPairs.back();

		// If item_pair is already selected - do nothing
		if (last_selected_pair == item_pair)
			return;

		bool grab_items = false;
		bool reverse = false;
		pairs_list_t pairs_to_select;

		// Pick out items from list between last selected and current clicked item_pair.
		for (pairs_iterator_t
				 iter = mItemPairs.begin(),
				 iter_end = mItemPairs.end();
			 iter != iter_end; ++iter)
		{
			item_pair_t* cur = *iter;
			if (cur == last_selected_pair || cur == item_pair)
			{
				// We've got reverse selection if last grabed item isn't a new selection.
				reverse = grab_items && (cur != item_pair);
				grab_items = !grab_items;
				// Skip last selected and current clicked item pairs.
				continue;
			}
			if (!cur->first->getVisible())
			{
				// Skip invisible item pairs.
				continue;
			}
			if (grab_items)
			{
				pairs_to_select.push_back(cur);
			}
		}

		if (reverse)
		{
			pairs_to_select.reverse();
		}

		pairs_to_select.push_back(item_pair);

		for (pairs_iterator_t
				 iter = pairs_to_select.begin(),
				 iter_end = pairs_to_select.end();
			 iter != iter_end; ++iter)
		{
			item_pair_t* pair_to_select = *iter;
			if (isSelected(pair_to_select))
			{
				// Item was already selected but there is a need to keep order from last selected pair to new selection.
				// Do it here to prevent extra mCommitOnSelectionChange in selectItemPair().
				mSelectedItemPairs.remove(pair_to_select);
				mSelectedItemPairs.push_back(pair_to_select);
			}
			else
			{
				selectItemPair(pair_to_select, true);
			}
		}

		if (!select_item)
		{
			// Update last selected item border.
			mSelectedItemsBorder->setRect(getLastSelectedItemRect().stretch(-1));
		}
		return;
	}

	//no need to do additional commit on selection reset
	if (!(mask & MASK_CONTROL) || !mMultipleSelection) resetSelection(true);

	//only CTRL usage allows to deselect an item, usual clicking on an item cannot deselect it
	if (mask & MASK_CONTROL)
	selectItemPair(item_pair, select_item);
	else
		selectItemPair(item_pair, true);
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
		case KEY_ESCAPE:
		{
			if (mask == MASK_NONE)
			{
				setFocus(FALSE); // pass focus to the game area (EXT-8357)
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

	// Stretch selected item rect to ensure it won't be clipped
	mSelectedItemsBorder->setRect(getLastSelectedItemRect().stretch(-1));
	// By default mark it as not consecutive selection
	mIsConsecutiveSelection = false;

	return true;
}

void LLFlatListView::scrollToShowFirstSelectedItem()
{
	if (!mSelectedItemPairs.size())	return;

	LLRect selected_rc = mSelectedItemPairs.front()->first->getRect();

	if (selected_rc.isValid())
	{
		scrollToShowRect(selected_rc);
	}
}

LLRect LLFlatListView::getLastSelectedItemRect()
{
	if (!mSelectedItemPairs.size())
	{
		return LLRect::null;
	}

	return mSelectedItemPairs.back()->first->getRect();
}

void LLFlatListView::selectFirstItem	()
{
	// No items - no actions!
	if (0 == size()) return;

	// Select first visible item
	for (pairs_iterator_t
			 iter = mItemPairs.begin(),
			 iter_end = mItemPairs.end();
		 iter != iter_end; ++iter)
	{
		// skip invisible items
		if ( (*iter)->first->getVisible() )
		{
			selectItemPair(*iter, true);
			ensureSelectedVisible();
			break;
		}
	}
}

void LLFlatListView::selectLastItem		()
{
	// No items - no actions!
	if (0 == size()) return;

	// Select last visible item
	for (pairs_list_t::reverse_iterator
			 r_iter = mItemPairs.rbegin(),
			 r_iter_end = mItemPairs.rend();
		 r_iter != r_iter_end; ++r_iter)
	{
		// skip invisible items
		if ( (*r_iter)->first->getVisible() )
		{
			selectItemPair(*r_iter, true);
			ensureSelectedVisible();
			break;
		}
	}
}

void LLFlatListView::ensureSelectedVisible()
{
	LLRect selected_rc = getLastSelectedItemRect();

	if ( selected_rc.isValid() )
	{
		scrollToShowRect(selected_rc);
	}
}


// virtual
bool LLFlatListView::selectNextItemPair(bool is_up_direction, bool reset_selection)
{
	// No items - no actions!
	if ( 0 == size() )
		return false;

	if (!mIsConsecutiveSelection)
	{
		// Leave only one item selected if list has not consecutive selection
		if (mSelectedItemPairs.size() && !reset_selection)
		{
			item_pair_t* cur_sel_pair = mSelectedItemPairs.back();
			resetSelection();
			selectItemPair (cur_sel_pair, true);
		}
	}

	if ( mSelectedItemPairs.size() )
	{
		item_pair_t* to_sel_pair = NULL;
		item_pair_t* cur_sel_pair = NULL;

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
			// Mark it as consecutive selection
			mIsConsecutiveSelection = true;
			return true;
		}
	}
	else
	{
		// If there weren't selected items then choose the first one bases on given direction
		// Force selection to first item
		if (is_up_direction)
			selectLastItem();
		else
			selectFirstItem();
		// Mark it as consecutive selection
		mIsConsecutiveSelection = true;
		return true;
	}

	return false;
}

BOOL LLFlatListView::canSelectAll() const
{
	return 0 != size() && mAllowSelection && mMultipleSelection;
}

void LLFlatListView::selectAll()
{
	if (!mAllowSelection || !mMultipleSelection)
		return;

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

	// Stretch selected item rect to ensure it won't be clipped
	mSelectedItemsBorder->setRect(getLastSelectedItemRect().stretch(-1));
}

bool LLFlatListView::isSelected(item_pair_t* item_pair) const
{
	llassert(item_pair);

	pairs_const_iterator_t it_end = mSelectedItemPairs.end();
	return std::find(mSelectedItemPairs.begin(), it_end, item_pair) != it_end;
}

bool LLFlatListView::removeItemPair(item_pair_t* item_pair, bool rearrange)
{
	llassert(item_pair);

	bool deleted = false;
	bool selection_changed = false;
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
			selection_changed = true;
			break;
		}
	}

	mItemsPanel->removeChild(item_pair->first);
	item_pair->first->die();
	delete item_pair;

	if (rearrange)
	{
	rearrangeItems();
	notifyParentItemsRectChanged();
	}

	if (selection_changed && mCommitOnSelectionChange)
	{
		onCommit();
	}

	return true;
}

void LLFlatListView::notifyParentItemsRectChanged()
{
	S32 comment_height = 0;

	// take into account comment text height if exists
	if (mNoItemsCommentTextbox && mNoItemsCommentTextbox->getVisible())
	{
		// top text padding inside the textbox is included into the height
		comment_height = mNoItemsCommentTextbox->getTextPixelHeight();

		// take into account a distance from parent's top border to textbox's top
		comment_height += getRect().getHeight() - mNoItemsCommentTextbox->getRect().mTop;
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
		mSelectedItemsBorder->setVisible(!visible);
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
	if (size())
	{
	mSelectedItemsBorder->setVisible(TRUE);
	}
	gEditMenuHandler = this;
}
// virtual
void LLFlatListView::onFocusLost()
{
	mSelectedItemsBorder->setVisible(FALSE);
	// Route menu back to the default
 	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}
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


/************************************************************************/
/*             LLFlatListViewEx implementation                          */
/************************************************************************/
LLFlatListViewEx::Params::Params()
: no_items_msg("no_items_msg")
, no_filtered_items_msg("no_filtered_items_msg")
{

}

LLFlatListViewEx::LLFlatListViewEx(const Params& p)
:	LLFlatListView(p)
, mNoFilteredItemsMsg(p.no_filtered_items_msg)
, mNoItemsMsg(p.no_items_msg)
, mForceShowingUnmatchedItems(false)
, mHasMatchedItems(false)
{

}

void LLFlatListViewEx::updateNoItemsMessage(const std::string& filter_string)
{
	bool items_filtered = !filter_string.empty();
	if (items_filtered)
	{
		// items were filtered
		LLStringUtil::format_map_t args;
		args["[SEARCH_TERM]"] = LLURI::escape(filter_string);
		std::string text = mNoFilteredItemsMsg;
		LLStringUtil::format(text, args);
		setNoItemsCommentText(text);
	}
	else
	{
		// list does not contain any items at all
		setNoItemsCommentText(mNoItemsMsg);
	}

}

bool LLFlatListViewEx::getForceShowingUnmatchedItems()
{
	return mForceShowingUnmatchedItems;
}

void LLFlatListViewEx::setForceShowingUnmatchedItems(bool show)
{
	mForceShowingUnmatchedItems = show;
}

void LLFlatListViewEx::setFilterSubString(const std::string& filter_str)
{
	if (0 != LLStringUtil::compareInsensitive(filter_str, mFilterSubString))
	{
		mFilterSubString = filter_str;
		updateNoItemsMessage(mFilterSubString);
		filterItems();
	}
}

void LLFlatListViewEx::updateItemVisibility(LLPanel* item, const LLSD &action)
{
	if (!item) return;

	// 0 signifies that filter is matched,
	// i.e. we don't hide items that don't support 'match_filter' action, separators etc.
	if (0 == item->notify(action))
	{
		mHasMatchedItems = true;
		item->setVisible(true);
	}
	else
	{
		// TODO: implement (re)storing of current selection.
		if (!mForceShowingUnmatchedItems)
		{
			selectItem(item, false);
		}
		item->setVisible(mForceShowingUnmatchedItems);
	}
}

void LLFlatListViewEx::filterItems()
{
	typedef std::vector <LLPanel*> item_panel_list_t;

	std::string cur_filter = mFilterSubString;
	LLStringUtil::toUpper(cur_filter);

	LLSD action;
	action.with("match_filter", cur_filter);

	item_panel_list_t items;
	getItems(items);

	mHasMatchedItems = false;
	for (item_panel_list_t::iterator
			 iter = items.begin(),
			 iter_end = items.end();
		 iter != iter_end; ++iter)
	{
		LLPanel* pItem = (*iter);
		updateItemVisibility(pItem, action);
	}

	sort();
	notifyParentItemsRectChanged();
}

bool LLFlatListViewEx::hasMatchedItems()
{
	return mHasMatchedItems;
}

//EOF
