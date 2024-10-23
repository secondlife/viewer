 /**
 * @file llscrolllistctrl.cpp
 * @brief Scroll lists are composed of rows (items), each of which
 * contains columns (cells).
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llscrolllistctrl.h"

#include <algorithm>

#include "llstl.h"
#include "llboost.h"
//#include "indra_constants.h"

#include "llavatarnamecache.h"
#include "llcheckboxctrl.h"
#include "llclipboard.h"
#include "llfocusmgr.h"
#include "llgl.h"               // LLGLSUIDefault()
#include "lllocalcliprect.h"
//#include "llrender.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrolllistcell.h"
#include "llstring.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llcontrol.h"
#include "llkeyboard.h"
#include "llviewborder.h"
#include "lltextbox.h"
#include "llsdparam.h"
#include "llcachename.h"
#include "llmenugl.h"
#include "llurlaction.h"
#include "lltooltip.h"

#include <boost/bind.hpp>

static LLDefaultChildRegistry::Register<LLScrollListCtrl> r("scroll_list");

// local structures & classes.
struct SortScrollListItem
{
    SortScrollListItem(const std::vector<std::pair<S32, bool> >& sort_orders,const LLScrollListCtrl::sort_signal_t* sort_signal, bool alternate_sort)
    :   mSortOrders(sort_orders)
    ,   mSortSignal(sort_signal)
    ,   mAltSort(alternate_sort)
    {}

    bool operator()(const LLScrollListItem* i1, const LLScrollListItem* i2)
    {
        // sort over all columns in order specified by mSortOrders
        S32 sort_result = 0;
        for (sort_order_t::const_reverse_iterator it = mSortOrders.rbegin();
             it != mSortOrders.rend(); ++it)
        {
            S32 col_idx = it->first;
            bool sort_ascending = it->second;

            S32 order = sort_ascending ? 1 : -1; // ascending or descending sort for this column?

            const LLScrollListCell *cell1 = i1->getColumn(col_idx);
            const LLScrollListCell *cell2 = i2->getColumn(col_idx);
            if (cell1 && cell2)
            {
                if(mSortSignal)
                {
                    sort_result = order * (*mSortSignal)(col_idx,i1, i2);
                }
                else
                {
                    if (mAltSort && !cell1->getAltValue().asString().empty() && !cell2->getAltValue().asString().empty())
                    {
                        sort_result = order * LLStringUtil::compareDict(cell1->getAltValue().asString(), cell2->getAltValue().asString());
                    }
                    else
                    {
                        sort_result = order * LLStringUtil::compareDict(cell1->getValue().asString(), cell2->getValue().asString());
                    }
                }
                if (sort_result != 0)
                {
                    break; // we have a sort order!
                }
            }
        }

        return sort_result < 0;
    }


    typedef std::vector<std::pair<S32, bool> > sort_order_t;
    const LLScrollListCtrl::sort_signal_t* mSortSignal;
    const sort_order_t& mSortOrders;
    const bool mAltSort;
};

//---------------------------------------------------------------------------
// LLScrollListCtrl
//---------------------------------------------------------------------------

void LLScrollListCtrl::SelectionTypeNames::declareValues()
{
    declare("row", LLScrollListCtrl::ROW);
    declare("cell", LLScrollListCtrl::CELL);
    declare("header", LLScrollListCtrl::HEADER);
}

LLScrollListCtrl::Contents::Contents()
:   columns("column"),
    rows("row")
{
    addSynonym(columns, "columns");
    addSynonym(rows, "rows");
}

LLScrollListCtrl::Params::Params()
:   multi_select("multi_select", false),
    has_border("draw_border"),
    draw_heading("draw_heading"),
    search_column("search_column", 0),
    selection_type("selection_type", ROW),
    sort_column("sort_column", -1),
    sort_ascending("sort_ascending", true),
    can_sort("can_sort", true),
    mouse_wheel_opaque("mouse_wheel_opaque", false),
    commit_on_keyboard_movement("commit_on_keyboard_movement", true),
    commit_on_selection_change("commit_on_selection_change", false),
    heading_height("heading_height"),
    page_lines("page_lines", 0),
    background_visible("background_visible"),
    draw_stripes("draw_stripes"),
    column_padding("column_padding"),
    row_padding("row_padding", 2),
    fg_unselected_color("fg_unselected_color"),
    fg_selected_color("fg_selected_color"),
    bg_selected_color("bg_selected_color"),
    fg_disable_color("fg_disable_color"),
    bg_writeable_color("bg_writeable_color"),
    bg_readonly_color("bg_readonly_color"),
    bg_stripe_color("bg_stripe_color"),
    hovered_color("hovered_color"),
    highlighted_color("highlighted_color"),
    contents(""),
    scroll_bar_bg_visible("scroll_bar_bg_visible"),
    scroll_bar_bg_color("scroll_bar_bg_color"),
    border("border")
{}

LLScrollListCtrl::LLScrollListCtrl(const LLScrollListCtrl::Params& p)
:   LLUICtrl(p),
    mLineHeight(0),
    mScrollLines(0),
    mMouseWheelOpaque(p.mouse_wheel_opaque),
    mPageLines(p.page_lines),
    mMaxSelectable(0),
    mAllowKeyboardMovement(true),
    mCommitOnKeyboardMovement(p.commit_on_keyboard_movement),
    mCommitOnSelectionChange(p.commit_on_selection_change),
    mSelectionChanged(false),
    mSelectionType(p.selection_type),
    mNeedsScroll(false),
    mCanSelect(true),
    mCanSort(p.can_sort),
    mColumnsDirty(false),
    mMaxItemCount(INT_MAX),
    mBorderThickness( 2 ),
    mOnDoubleClickCallback( NULL ),
    mOnMaximumSelectCallback( NULL ),
    mOnSortChangedCallback( NULL ),
    mHighlightedItem(-1),
    mBorder(NULL),
    mSortCallback(NULL),
    mNumDynamicWidthColumns(0),
    mTotalStaticColumnWidth(0),
    mTotalColumnPadding(0),
    mSorted(false),
    mDirty(false),
    mOriginalSelection(-1),
    mLastSelected(NULL),
    mHeadingHeight(p.heading_height),
    mAllowMultipleSelection(p.multi_select),
    mDisplayColumnHeaders(p.draw_heading),
    mBackgroundVisible(p.background_visible),
    mDrawStripes(p.draw_stripes),
    mBgWriteableColor(p.bg_writeable_color()),
    mBgReadOnlyColor(p.bg_readonly_color()),
    mBgSelectedColor(p.bg_selected_color()),
    mBgStripeColor(p.bg_stripe_color()),
    mFgSelectedColor(p.fg_selected_color()),
    mFgUnselectedColor(p.fg_unselected_color()),
    mFgDisabledColor(p.fg_disable_color()),
    mHighlightedColor(p.highlighted_color()),
    mHoveredColor(p.hovered_color()),
    mSearchColumn(p.search_column),
    mColumnPadding(p.column_padding),
    mRowPadding(p.row_padding),
    mAlternateSort(false),
    mContextMenuType(MENU_NONE),
    mIsFriendSignal(NULL)
{
    mItemListRect.setOriginAndSize(
        mBorderThickness,
        mBorderThickness,
        getRect().getWidth() - 2 * mBorderThickness,
        getRect().getHeight() - 2 * mBorderThickness );

    updateLineHeight();

    // Init the scrollbar
    static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

    LLRect scroll_rect;
    scroll_rect.setOriginAndSize(
        getRect().getWidth() - mBorderThickness - scrollbar_size,
        mItemListRect.mBottom,
        scrollbar_size,
        mItemListRect.getHeight());

    LLScrollbar::Params sbparams;
    sbparams.name("Scrollbar");
    sbparams.rect(scroll_rect);
    sbparams.orientation(LLScrollbar::VERTICAL);
    sbparams.doc_size(getItemCount());
    sbparams.doc_pos(mScrollLines);
    sbparams.page_size( getLinesPerPage() );
    sbparams.change_callback(boost::bind(&LLScrollListCtrl::onScrollChange, this, _1, _2));
    sbparams.follows.flags(FOLLOWS_RIGHT | FOLLOWS_TOP | FOLLOWS_BOTTOM);
    sbparams.visible(false);
    sbparams.bg_visible(p.scroll_bar_bg_visible);
    sbparams.bg_color(p.scroll_bar_bg_color);
    mScrollbar = LLUICtrlFactory::create<LLScrollbar> (sbparams);
    addChild(mScrollbar);

    // Border
    if (p.has_border)
    {
        LLRect border_rect = getLocalRect();
        LLViewBorder::Params params = p.border;
        params.rect(border_rect);
        mBorder = LLUICtrlFactory::create<LLViewBorder> (params);
        addChild(mBorder);
    }

    // set border *after* rect is fully initialized
    if (mBorder)
    {
        mBorder->setRect(getLocalRect());
        mBorder->reshape(getRect().getWidth(), getRect().getHeight());
    }

    if (p.sort_column >= 0)
    {
        sortByColumnIndex(p.sort_column, p.sort_ascending);
    }


    for (LLInitParam::ParamIterator<LLScrollListColumn::Params>::const_iterator row_it = p.contents.columns.begin();
        row_it != p.contents.columns.end();
        ++row_it)
    {
        addColumn(*row_it);
    }

    LLTextBox::Params text_p;
    text_p.name("comment_text");
    text_p.border_visible(false);
    text_p.rect(mItemListRect);
    text_p.follows.flags(FOLLOWS_ALL);
    // word wrap was added accroding to the EXT-6841
    text_p.wrap(true);
    mCommentText = LLUICtrlFactory::create<LLTextBox>(text_p);
    addChild(mCommentText);

    for (LLInitParam::ParamIterator<LLScrollListItem::Params>::const_iterator row_it = p.contents.rows.begin();
        row_it != p.contents.rows.end();
        ++row_it)
    {
        addRow(*row_it);
    }
}

S32 LLScrollListCtrl::getSearchColumn()
{
    // search for proper search column
    if (mSearchColumn < 0)
    {
        LLScrollListItem* itemp = getFirstData();
        if (itemp)
        {
            for(S32 column = 0; column < getNumColumns(); column++)
            {
                LLScrollListCell* cell = itemp->getColumn(column);
                if (cell && cell->isText())
                {
                    mSearchColumn = column;
                    break;
                }
            }
        }
    }
    return llclamp(mSearchColumn, 0, getNumColumns());
}

// virtual
bool LLScrollListCtrl::preProcessChildNode(LLXMLNodePtr child)
{
    return child->hasName("column") || child->hasName("row");
}

LLScrollListCtrl::~LLScrollListCtrl()
{
    delete mSortCallback;

    std::for_each(mItemList.begin(), mItemList.end(), DeletePointer());
    mItemList.clear();
    clearColumns(); //clears columns and deletes headers
    delete mIsFriendSignal;

    auto menu = mPopupMenuHandle.get();
    if (menu)
    {
        menu->die();
        mPopupMenuHandle.markDead();
    }
}

bool LLScrollListCtrl::setMaxItemCount(S32 max_count)
{
    if (max_count >= getItemCount())
    {
        mMaxItemCount = max_count;
    }
    return (max_count == mMaxItemCount);
}

S32 LLScrollListCtrl::isEmpty() const
{
    return mItemList.empty();
}

S32 LLScrollListCtrl::getItemCount() const
{
    return static_cast<S32>(mItemList.size());
}

bool LLScrollListCtrl::hasSelectedItem() const
{
    item_list::iterator iter;
    for (LLScrollListItem* item : mItemList)
    {
        if (item->getSelected())
        {
            return true;
        }
        iter++;
    }
    return false;
}

// virtual LLScrolListInterface function (was deleteAllItems)
void LLScrollListCtrl::clearRows()
{
    std::for_each(mItemList.begin(), mItemList.end(), DeletePointer());
    mItemList.clear();

    // Scroll the bar back up to the top.
    mScrollbar->setDocParams(0, 0);

    mScrollLines = 0;
    mLastSelected = NULL;
    updateLayout();
    mDirty = false;
}

LLScrollListItem* LLScrollListCtrl::getFirstSelected() const
{
    for (LLScrollListItem* item : mItemList)
    {
        if (item->getSelected())
        {
            return item;
        }
    }
    return NULL;
}

std::vector<LLScrollListItem*> LLScrollListCtrl::getAllSelected() const
{
    std::vector<LLScrollListItem*> ret;
    for (LLScrollListItem* item : mItemList)
    {
        if (item->getSelected())
        {
            ret.push_back(item);
        }
    }
    return ret;
}

std::vector<LLSD> LLScrollListCtrl::getAllSelectedValues() const
{
    std::vector<LLSD> ret;
    for (LLScrollListItem* item : mItemList)
    {
        if (item->getSelected())
        {
            ret.push_back(item->getValue());
        }
    }
    return ret;
}

S32 LLScrollListCtrl::getNumSelected() const
{
    S32 numSelected = 0;

    for (LLScrollListItem* item : mItemList)
    {
        if (item->getSelected())
        {
            ++numSelected;
        }
    }
    return numSelected;
}

S32 LLScrollListCtrl::getFirstSelectedIndex() const
{
    S32 CurSelectedIndex = 0;
    // make sure sort is up to date before returning an index
    updateSort();

    for (LLScrollListItem* item : mItemList)
    {
        if (item->getSelected())
        {
            return CurSelectedIndex;
        }
        CurSelectedIndex++;
    }
    return -1;
}

LLScrollListItem* LLScrollListCtrl::getFirstData() const
{
    return mItemList.empty() ? NULL : mItemList.front();
}

LLScrollListItem* LLScrollListCtrl::getLastData() const
{
    return mItemList.empty() ? NULL : mItemList.back();
}

LLScrollListItem* LLScrollListCtrl::getNthData(size_t index) const
{
    if (mItemList.size() <= index)
    {
        return NULL;
    }
    return mItemList[index];
}

std::vector<LLScrollListItem*> LLScrollListCtrl::getAllData() const
{
    std::vector<LLScrollListItem*> ret;
    for (LLScrollListItem* item : mItemList)
    {
        ret.push_back(item);
    }
    return ret;
}

// returns first matching item
LLScrollListItem* LLScrollListCtrl::getItem(const LLSD& sd) const
{
    std::string string_val = sd.asString();

    for (LLScrollListItem* item : mItemList)
    {
        // assumes string representation is good enough for comparison
        if (item->getValue().asString() == string_val)
        {
            return item;
        }
    }

    return NULL;
}

void LLScrollListCtrl::reshape( S32 width, S32 height, bool called_from_parent )
{
    LLUICtrl::reshape( width, height, called_from_parent );

    updateLayout();
}

void LLScrollListCtrl::updateLayout()
{
    static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
    // reserve room for column headers, if needed
    S32 heading_size = (mDisplayColumnHeaders ? mHeadingHeight : 0);
    mItemListRect.setOriginAndSize(
        mBorderThickness,
        mBorderThickness,
        getRect().getWidth() - 2 * mBorderThickness,
        getRect().getHeight() - (2 * mBorderThickness ) - heading_size );

    mCommentText->setShape(mItemListRect);

    // how many lines of content in a single "page"
    S32 page_lines =  getLinesPerPage();

    bool scrollbar_visible = mLineHeight * getItemCount() > mItemListRect.getHeight();
    if (scrollbar_visible)
    {
        // provide space on the right for scrollbar
        mItemListRect.mRight = getRect().getWidth() - mBorderThickness - scrollbar_size;
    }

    mScrollbar->setOrigin(getRect().getWidth() - mBorderThickness - scrollbar_size, mItemListRect.mBottom);
    mScrollbar->reshape(scrollbar_size, mItemListRect.getHeight() + (mDisplayColumnHeaders ? mHeadingHeight : 0));
    mScrollbar->setPageSize(page_lines);
    mScrollbar->setDocSize( getItemCount() );
    mScrollbar->setVisible(scrollbar_visible);

    dirtyColumns();
}

// Attempt to size the control to show all items.
// Do not make larger than width or height.
void LLScrollListCtrl::fitContents(S32 max_width, S32 max_height)
{
    S32 height = llmin( getRequiredRect().getHeight(), max_height );
    if (mPageLines)
    {
        height = llmin(mPageLines * mLineHeight + 2 * mBorderThickness + (mDisplayColumnHeaders ? mHeadingHeight : 0), height);
    }

    S32 width = getRect().getWidth();

    reshape( width, height );
}

LLRect LLScrollListCtrl::getRequiredRect()
{
    S32 heading_size = (mDisplayColumnHeaders ? mHeadingHeight : 0);
    S32 height = (mLineHeight * getItemCount())
                + (2 * mBorderThickness )
                + heading_size;
    S32 width = getRect().getWidth();

    return LLRect(0, height, width, 0);
}


bool LLScrollListCtrl::addItem( LLScrollListItem* item, EAddPosition pos, bool requires_column )
{
    bool not_too_big = getItemCount() < mMaxItemCount;
    if (not_too_big)
    {
        switch( pos )
        {
        case ADD_TOP:
            mItemList.push_front(item);
            setNeedsSort();
            break;

        case ADD_DEFAULT:
        case ADD_BOTTOM:
            mItemList.push_back(item);
            setNeedsSort();
            break;

        default:
            llassert(0);
            mItemList.push_back(item);
            setNeedsSort();
            break;
        }

        // create new column on demand
        if (mColumns.empty() && requires_column)
        {
            LLScrollListColumn::Params col_params;
            col_params.name =  "default_column";
            col_params.header.label = "";
            col_params.width.dynamic_width = true;
            addColumn(col_params);
        }

        S32 num_cols = item->getNumColumns();
        S32 i = 0;
        for (LLScrollListCell* cell = item->getColumn(i); i < num_cols; cell = item->getColumn(++i))
        {
            if (i >= (S32)mColumnsIndexed.size())
                break;

            cell->setWidth(mColumnsIndexed[i]->getWidth());
        }

        updateLineHeightInsert(item);

        updateLayout();
    }

    return not_too_big;
}

// NOTE: This is *very* expensive for large lists, especially when we are dirtying the list every frame
//  while receiving a long list of names.
// *TODO: Use bookkeeping to make this an incramental cost with item additions
S32 LLScrollListCtrl::calcMaxContentWidth()
{
    const S32 HEADING_TEXT_PADDING = 25;
    const S32 COLUMN_TEXT_PADDING = 10;

    S32 max_item_width = 0;

    for (LLScrollListColumn* column : mColumnsIndexed)
    {
        if (!column)
            continue;

        if (mColumnWidthsDirty)
        {
            // update max content width for this column, by looking at all items
            column->mMaxContentWidth = column->mHeader ? LLFontGL::getFontSansSerifSmall()->getWidth(column->mLabel.getWString().c_str()) + mColumnPadding + HEADING_TEXT_PADDING : 0;
            for (LLScrollListItem* item : mItemList)
            {
                if (LLScrollListCell* cellp = item->getColumn(column->mIndex))
                {
                    column->mMaxContentWidth = llmax(LLFontGL::getFontSansSerifSmall()->getWidth(cellp->getValue().asString()) + mColumnPadding + COLUMN_TEXT_PADDING, column->mMaxContentWidth);
                }
            }
        }
        max_item_width += column->mMaxContentWidth;
    }
    mColumnWidthsDirty = false;

    return max_item_width;
}

bool LLScrollListCtrl::updateColumnWidths()
{
    bool width_changed = false;
    ordered_columns_t::iterator column_itor;
    for (column_itor = mColumnsIndexed.begin(); column_itor != mColumnsIndexed.end(); ++column_itor)
    {
        LLScrollListColumn* column = *column_itor;
        if (!column)
            continue;

        // update column width
        S32 new_width = 0;
        if (column->mRelWidth >= 0)
        {
            new_width = (S32)ll_round(column->mRelWidth*mItemListRect.getWidth());
        }
        else if (column->mDynamicWidth)
        {
            new_width = (mItemListRect.getWidth() - mTotalStaticColumnWidth - mTotalColumnPadding) / mNumDynamicWidthColumns;
        }
        else
        {
            new_width = column->getWidth();
        }

        if (column->getWidth() != new_width)
        {
            column->setWidth(new_width);
            width_changed = true;
        }
    }
    return width_changed;
}

// Line height is the max height of all the cells in all the items.
void LLScrollListCtrl::updateLineHeight()
{
    mLineHeight = 0;
    item_list::iterator iter;
    for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
    {
        LLScrollListItem *itemp = *iter;
        S32 num_cols = itemp->getNumColumns();
        S32 i = 0;
        for (const LLScrollListCell* cell = itemp->getColumn(i); i < num_cols; cell = itemp->getColumn(++i))
        {
            mLineHeight = llmax( mLineHeight, cell->getHeight() + mRowPadding );
        }
    }
}

// when the only change to line height is from an insert, we needn't scan the entire list
void LLScrollListCtrl::updateLineHeightInsert(LLScrollListItem* itemp)
{
    S32 num_cols = itemp->getNumColumns();
    S32 i = 0;
    for (const LLScrollListCell* cell = itemp->getColumn(i); i < num_cols; cell = itemp->getColumn(++i))
    {
        mLineHeight = llmax( mLineHeight, cell->getHeight() + mRowPadding );
    }
}

void LLScrollListCtrl::updateColumns(bool force_update)
{
    if (!mColumnsDirty && !force_update)
        return;

    mColumnsDirty = false;

    bool columns_changed_width = updateColumnWidths();

    // update column headers
    std::vector<LLScrollListColumn*>::iterator column_ordered_it;
    S32 left = mItemListRect.mLeft;
    LLScrollColumnHeader* last_header = NULL;
    for (column_ordered_it = mColumnsIndexed.begin(); column_ordered_it != mColumnsIndexed.end(); ++column_ordered_it)
    {
        LLScrollListColumn* column = *column_ordered_it;
        if (!column || column->getWidth() < 0)
        {
            // skip hidden columns
            continue;
        }

        if (column->mHeader)
        {
            column->mHeader->updateResizeBars();

            last_header = column->mHeader;
            S32 top = mItemListRect.mTop;
            S32 right = left + column->getWidth();

            if (column->mIndex != (S32)mColumnsIndexed.size()-1)
            {
                right += mColumnPadding;
            }
            right = llmax(left, llmin(mItemListRect.getWidth(), right));
            S32 header_width = right - left;

            last_header->reshape(header_width, mHeadingHeight);
            last_header->translate(
                left - last_header->getRect().mLeft,
                top - last_header->getRect().mBottom);
            last_header->setVisible(mDisplayColumnHeaders && header_width > 0);
            left = right;
        }
    }

    bool header_changed_width = false;
    // expand last column header we encountered to full list width
    if (last_header)
    {
        S32 old_width = last_header->getColumn()->getWidth();
        S32 new_width = llmax(0, mItemListRect.mRight - last_header->getRect().mLeft);
        last_header->reshape(new_width, last_header->getRect().getHeight());
        last_header->setVisible(mDisplayColumnHeaders && new_width > 0);
        if (old_width != new_width)
        {
            last_header->getColumn()->setWidth(new_width);
            header_changed_width = true;
        }
    }

    // propagate column widths to individual cells
    if (columns_changed_width || force_update)
    {
        item_list::iterator iter;
        for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
        {
            LLScrollListItem *itemp = *iter;
            S32 num_cols = itemp->getNumColumns();
            S32 i = 0;
            for (LLScrollListCell* cell = itemp->getColumn(i); i < num_cols; cell = itemp->getColumn(++i))
            {
                if (i >= (S32)mColumnsIndexed.size())
                    break;

                cell->setWidth(mColumnsIndexed[i]->getWidth());
            }
        }
    }
    else if (header_changed_width)
    {
        item_list::iterator iter;
        S32 index = last_header->getColumn()->mIndex; // Not always identical to last column!
        for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
        {
            LLScrollListItem *itemp = *iter;
            LLScrollListCell* cell = itemp->getColumn(index);
            if (cell)
            {
                cell->setWidth(last_header->getColumn()->getWidth());
            }
        }
    }
}

void LLScrollListCtrl::setHeadingHeight(S32 heading_height)
{
    mHeadingHeight = heading_height;

    updateLayout();
}

void LLScrollListCtrl::setPageLines(S32 new_page_lines)
{
    mPageLines  = new_page_lines;

    updateLayout();
}

bool LLScrollListCtrl::selectFirstItem()
{
    bool success = false;

    // our $%&@#$()^%#$()*^ iterators don't let us check against the first item inside out iteration
    bool first_item = true;

    item_list::iterator iter;
    for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
    {
        LLScrollListItem *itemp = *iter;
        if( first_item && itemp->getEnabled() )
        {
            if (!itemp->getSelected())
            {
                switch (mSelectionType)
                {
                case CELL:
                    selectItem(itemp, 0);
                    break;
                case HEADER:
                case ROW:
                    selectItem(itemp, -1);
                }
            }
            success = true;
            mOriginalSelection = 0;
        }
        else
        {
            deselectItem(itemp);
        }
        first_item = false;
    }

    if (mCommitOnSelectionChange)
    {
        commitIfChanged();
    }
    return success;
}

// Deselects all other items
// virtual
bool LLScrollListCtrl::selectNthItem(S32 target_index)
{
    return selectItemRange(target_index, target_index);
}

// virtual
bool LLScrollListCtrl::selectItemRange(S32 first_index, S32 last_index)
{
    if (mItemList.empty())
    {
        return false;
    }

    // make sure sort is up to date
    updateSort();

    S32 bottom = (S32)mItemList.size() - 1;
    first_index = llclamp(first_index, 0, bottom);
    last_index = last_index < 0 ? bottom : llclamp(last_index, first_index, bottom);

    bool success = false;
    S32 index = 0;
    for (item_list::iterator iter = mItemList.begin(); iter != mItemList.end(); )
    {
        LLScrollListItem *itemp = *iter;
        if (!itemp)
        {
            iter = mItemList.erase(iter);
            continue;
        }

        if (index >= first_index && index <= last_index)
        {
            if (itemp->getEnabled())
            {
                // TODO: support range selection for cells
                selectItem(itemp, -1, false);
                success = true;
            }
        }
        else
        {
            deselectItem(itemp);
        }
        index++;
        iter++ ;
    }

    if (mCommitOnSelectionChange)
    {
        commitIfChanged();
    }

    mSearchString.clear();

    return success;
}


void LLScrollListCtrl::swapWithNext(S32 index)
{
    if (index >= ((S32)mItemList.size() - 1))
    {
        // At end of list, doesn't do anything
        return;
    }
    updateSort();
    LLScrollListItem *cur_itemp = mItemList[index];
    mItemList[index] = mItemList[index + 1];
    mItemList[index + 1] = cur_itemp;
}


void LLScrollListCtrl::swapWithPrevious(S32 index)
{
    if (index <= 0)
    {
        // At beginning of list, don't do anything
    }

    updateSort();
    LLScrollListItem *cur_itemp = mItemList[index];
    mItemList[index] = mItemList[index - 1];
    mItemList[index - 1] = cur_itemp;
}


void LLScrollListCtrl::deleteSingleItem(S32 target_index)
{
    if (target_index < 0 || target_index >= (S32)mItemList.size())
    {
        return;
    }

    updateSort();

    LLScrollListItem *itemp;
    itemp = mItemList[target_index];
    if (itemp == mLastSelected)
    {
        mLastSelected = NULL;
    }
    delete itemp;
    mItemList.erase(mItemList.begin() + target_index);
    dirtyColumns();
}

//FIXME: refactor item deletion
void LLScrollListCtrl::deleteItems(const LLSD& sd)
{
    item_list::iterator iter;
    for (iter = mItemList.begin(); iter < mItemList.end(); )
    {
        LLScrollListItem* itemp = *iter;
        if (itemp->getValue().asString() == sd.asString())
        {
            if (itemp == mLastSelected)
            {
                mLastSelected = NULL;
            }
            delete itemp;
            iter = mItemList.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    dirtyColumns();
}

void LLScrollListCtrl::deleteSelectedItems()
{
    item_list::iterator iter;
    for (iter = mItemList.begin(); iter < mItemList.end(); )
    {
        LLScrollListItem* itemp = *iter;
        if (itemp->getSelected())
        {
            delete itemp;
            iter = mItemList.erase(iter);
        }
        else
        {
            iter++;
        }
    }
    mLastSelected = NULL;
    dirtyColumns();
}

void LLScrollListCtrl::clearHighlightedItems()
{
    for (item_list::iterator iter = mItemList.begin(); iter != mItemList.end(); ++iter)
    {
        (*iter)->setHighlighted(false);
    }
}

void LLScrollListCtrl::mouseOverHighlightNthItem(S32 target_index)
{
    if (mHighlightedItem != target_index)
    {
        if (mHighlightedItem >= 0 && mHighlightedItem < mItemList.size())
        {
            mItemList[mHighlightedItem]->setHoverCell(-1);
        }
        mHighlightedItem = target_index;
    }
}

S32 LLScrollListCtrl::selectMultiple( uuid_vec_t ids )
{
    item_list::iterator iter;
    S32 count = 0;
    for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
    {
        LLScrollListItem* item = *iter;
        uuid_vec_t::iterator iditr;
        for (iditr = ids.begin(); iditr != ids.end(); ++iditr)
        {
            if (item->getEnabled() && (item->getUUID() == (*iditr)))
            {
                // TODO: support multiple selection for cells
                selectItem(item, -1, false);
                ++count;
                break;
            }
        }
        if (ids.end() != iditr)
        {
            ids.erase(iditr);
        }
    }

    if (mCommitOnSelectionChange)
    {
        commitIfChanged();
    }
    return count;
}

S32 LLScrollListCtrl::getItemIndex( LLScrollListItem* target_item ) const
{
    updateSort();

    S32 index = 0;
    for (LLScrollListItem* item : mItemList)
    {
        if (target_item == item)
        {
            return index;
        }
        index++;
    }
    return -1;
}

S32 LLScrollListCtrl::getItemIndex( const LLUUID& target_id ) const
{
    updateSort();

    S32 index = 0;
    for (LLScrollListItem* item : mItemList)
    {
        if (target_id == item->getUUID())
        {
            return index;
        }
        index++;
    }
    return -1;
}

void LLScrollListCtrl::selectPrevItem( bool extend_selection)
{
    updateSort();

    LLScrollListItem* prev_item = NULL;

    for (LLScrollListItem* item : mItemList)
    {
        if (item->getSelected())
        {
            break;
        }

        // don't allow navigation to disabled elements
        if (item->getEnabled())
        {
            prev_item = item;
        }
    }

    if (!prev_item)
    {
        reportInvalidInput();
        return;
    }

    selectItem(prev_item, prev_item->getSelectedCell(), !extend_selection);

    if ((mCommitOnSelectionChange || mCommitOnKeyboardMovement))
    {
        commitIfChanged();
    }

    mSearchString.clear();
}

void LLScrollListCtrl::selectNextItem( bool extend_selection)
{
    updateSort();

    LLScrollListItem* current_item = NULL;
    LLScrollListItem* next_item = NULL;

    for (LLScrollListItem* item : mItemList)
    {
        if (current_item)
        {
            if (item->getEnabled())
            {
                next_item = item;
                break;
            }
        }
        else if (item->getSelected())
        {
            current_item = item;
            next_item = NULL;
            continue;
        }
        else if (!next_item && item->getEnabled())
        {
            next_item = item;
        }
    }

    if (!next_item)
    {
        reportInvalidInput();
        return;
    }

    selectItem(next_item, next_item->getSelectedCell(), !extend_selection);

    if (mCommitOnKeyboardMovement)
    {
        onCommit();
    }

    mSearchString.clear();
}

void LLScrollListCtrl::deselectAllItems(bool no_commit_on_change)
{
    for (LLScrollListItem* item : mItemList)
    {
        deselectItem(item);
    }

    if (mCommitOnSelectionChange && !no_commit_on_change)
    {
        commitIfChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Use this to add comment text such as "Searching", which ignores column settings of list

void LLScrollListCtrl::setCommentText(const std::string& comment_text)
{
    mCommentText->setValue(comment_text);
}

LLScrollListItem* LLScrollListCtrl::addSeparator(EAddPosition pos)
{
    LLScrollListItem::Params separator_params;
    separator_params.enabled(false);
    LLScrollListCell::Params column_params;
    column_params.type = "icon";
    column_params.value = "menu_separator";
    column_params.color = LLColor4(0.f, 0.f, 0.f, 0.7f);
    column_params.font_halign = LLFontGL::HCENTER;
    separator_params.columns.add(column_params);
    return addRow( separator_params, pos );
}

// Selects first enabled item of the given name.
// Returns false if item not found.
// Calls getItemByLabel in order to combine functionality
bool LLScrollListCtrl::selectItemByLabel(const std::string& label, bool case_sensitive, S32 column/* = 0*/)
{
    deselectAllItems(true);     // ensure that no stale items are selected, even if we don't find a match
    LLScrollListItem* item = getItemByLabel(label, case_sensitive, column);

    bool found = NULL != item;
    if (found)
    {
        selectItem(item, -1);
    }

    if (mCommitOnSelectionChange)
    {
        commitIfChanged();
    }

    return found;
}

LLScrollListItem* LLScrollListCtrl::getItemByLabel(const std::string& label, bool case_sensitive, S32 column)
{
    if (label.empty())  //RN: assume no empty items
    {
        return NULL;
    }

    std::string target_text = label;
    if (!case_sensitive)
    {
        LLStringUtil::toLower(target_text);
    }

    for (LLScrollListItem* item : mItemList)
    {
        std::string item_text = item->getColumn(column)->getValue().asString(); // Only select enabled items with matching names
        if (!case_sensitive)
        {
            LLStringUtil::toLower(item_text);
        }
        if (item_text == target_text)
        {
            return item;
        }
    }
    return NULL;
}

LLScrollListItem* LLScrollListCtrl::getItemByIndex(S32 index)
{
    if (index >= 0 && index < (S32)mItemList.size())
    {
        return mItemList[index];
    }

    return NULL;
}

bool LLScrollListCtrl::selectItemByPrefix(const std::string& target, bool case_sensitive, S32 column)
{
    return selectItemByPrefix(utf8str_to_wstring(target), case_sensitive, column);
}

// Selects first enabled item that has a name where the name's first part matched the target string.
// Returns false if item not found.
bool LLScrollListCtrl::selectItemByPrefix(const LLWString& target, bool case_sensitive, S32 column)
{
    bool found = false;

    LLWString target_trimmed( target );
    auto target_len = target_trimmed.size();

    if( 0 == target_len )
    {
        // Is "" a valid choice?
        item_list::iterator iter;
        for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
        {
            LLScrollListItem* item = *iter;
            // Only select enabled items with matching names
            LLScrollListCell* cellp = item->getColumn(column == -1 ? getSearchColumn() : column);
            bool select = cellp ? item->getEnabled() && ('\0' == cellp->getValue().asString()[0]) : false;
            if (select)
            {
                selectItem(item, -1);
                found = true;
                break;
            }
        }
    }
    else
    {
        if (!case_sensitive)
        {
            // do comparisons in lower case
            LLWStringUtil::toLower(target_trimmed);
        }

        for (item_list::iterator iter = mItemList.begin(); iter != mItemList.end(); iter++)
        {
            LLScrollListItem* item = *iter;

            // Only select enabled items with matching names
            LLScrollListCell* cellp = item->getColumn(column == -1 ? getSearchColumn() : column);
            if (!cellp)
            {
                continue;
            }
            LLWString item_label = utf8str_to_wstring(cellp->getValue().asString());
            if (!case_sensitive)
            {
                LLWStringUtil::toLower(item_label);
            }
            // remove extraneous whitespace from searchable label
            LLWString trimmed_label = item_label;
            LLWStringUtil::trim(trimmed_label);

            bool select = item->getEnabled() && trimmed_label.compare(0, target_trimmed.size(), target_trimmed) == 0;

            if (select)
            {
                // find offset of matching text (might have leading whitespace)
                auto offset = item_label.find(target_trimmed);
                cellp->highlightText(static_cast<S32>(offset), static_cast<S32>(target_trimmed.size()));
                selectItem(item, -1);
                found = true;
                break;
            }
        }
    }

    if (mCommitOnSelectionChange)
    {
        commitIfChanged();
    }

    return found;
}

U32 LLScrollListCtrl::searchItems(const std::string& substring, bool case_sensitive, bool focus)
{
    return searchItems(utf8str_to_wstring(substring), case_sensitive, focus);
}

U32 LLScrollListCtrl::searchItems(const LLWString& substring, bool case_sensitive, bool focus)
{
    U32 found = 0;

    LLWString substring_trimmed(substring);
    auto len = substring_trimmed.size();

    if (0 == len)
    {
        // at the moment search for empty element is not supported
        return 0;
    }
    else
    {
        deselectAllItems(true);
        if (!case_sensitive)
        {
            // do comparisons in lower case
            LLWStringUtil::toLower(substring_trimmed);
        }

        for (item_list::iterator iter = mItemList.begin(); iter != mItemList.end(); iter++)
        {
            LLScrollListItem* item = *iter;
            // Only select enabled items with matching names
            if (!item->getEnabled())
            {
                continue;
            }
            LLScrollListCell* cellp = item->getColumn(getSearchColumn());
            if (!cellp)
            {
                continue;
            }
            LLWString item_label = utf8str_to_wstring(cellp->getValue().asString());
            if (!case_sensitive)
            {
                LLWStringUtil::toLower(item_label);
            }
            // remove extraneous whitespace from searchable label
            LLWStringUtil::trim(item_label);

            size_t found_iter = item_label.find(substring_trimmed);

            if (found_iter != std::string::npos)
            {
                // find offset of matching text
                cellp->highlightText(static_cast<S32>(found_iter), static_cast<S32>(substring_trimmed.size()));
                selectItem(item, -1, false);

                found++;

                if (!mAllowMultipleSelection)
                {
                    break;
                }
            }
        }
    }

    if (focus && found != 0)
    {
        mNeedsScroll = true;
    }

    if (mCommitOnSelectionChange)
    {
        commitIfChanged();
    }

    return found;
}

std::string LLScrollListCtrl::getSelectedItemLabel(S32 column) const
{
    LLScrollListItem* item;

    item = getFirstSelected();
    if (item)
    {
        if (LLScrollListCell* cell = item->getColumn(column))
        {
            return cell->getValue().asString();
        }
    }

    return LLStringUtil::null;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// "StringUUID" interface: use this when you're creating a list that contains non-unique strings each of which
// has an associated, unique UUID, and only one of which can be selected at a time.

LLScrollListItem* LLScrollListCtrl::addStringUUIDItem(const std::string& item_text, const LLUUID& id, EAddPosition pos, bool enabled)
{
    if (getItemCount() < mMaxItemCount)
    {
        LLScrollListItem::Params item_p;
        item_p.enabled(enabled);
        item_p.value(id);
        item_p.columns.add().value(item_text).type("text");

        return addRow( item_p, pos );
    }
    return NULL;
}

// Select the line or lines that match this UUID
bool LLScrollListCtrl::selectByID( const LLUUID& id )
{
    return selectByValue( LLSD(id) );
}

bool LLScrollListCtrl::setSelectedByValue(const LLSD& value, bool selected)
{
    bool found = false;

    if (selected && !mAllowMultipleSelection)
    {
        deselectAllItems(true);
    }

    item_list::iterator iter;
    for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
    {
        LLScrollListItem* item = *iter;
        if (item->getEnabled())
        {
            if (value.isBinary())
            {
                if (item->getValue().isBinary())
                {
                    LLSD::Binary data1 = value.asBinary();
                    LLSD::Binary data2 = item->getValue().asBinary();
                    found = std::equal(data1.begin(), data1.end(), data2.begin());
                }
            }
            else
            {
                found = item->getValue().asString() == value.asString();
            }

            if (found)
            {
                if (selected)
                {
                    selectItem(item, -1, !mAllowMultipleSelection);
                }
                else
                {
                    deselectItem(item);
                }
                break;
            }
        }
    }

    if (mCommitOnSelectionChange)
    {
        commitIfChanged();
    }

    return found;
}

bool LLScrollListCtrl::isSelected(const LLSD& value) const
{
    item_list::const_iterator iter;
    for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
    {
        LLScrollListItem* item = *iter;
        if (item->getValue().asString() == value.asString())
        {
            return item->getSelected();
        }
    }
    return false;
}

LLUUID LLScrollListCtrl::getStringUUIDSelectedItem() const
{
    LLScrollListItem* item = getFirstSelected();

    if (item)
    {
        return item->getUUID();
    }

    return LLUUID::null;
}

LLSD LLScrollListCtrl::getSelectedValue()
{
    LLScrollListItem* item = getFirstSelected();

    if (item)
    {
        return item->getValue();
    }
    else
    {
        return LLSD();
    }
}

void LLScrollListCtrl::drawItems()
{
    S32 x = mItemListRect.mLeft;
    S32 y = mItemListRect.mTop - mLineHeight;

    // allow for partial line at bottom
    S32 num_page_lines = getLinesPerPage();

    LLRect item_rect;

    LLGLSUIDefault gls_ui;

    F32 alpha = getDrawContext().mAlpha;

    {
        LLLocalClipRect clip(mItemListRect);

        S32 cur_y = y;

        S32 max_columns = 0;

        LLColor4 highlight_color = LLColor4::white; // ex: text inside cells
        static LLUICachedControl<F32> type_ahead_timeout ("TypeAheadTimeout", 0);
        highlight_color.mV[VALPHA] = clamp_rescale(mSearchTimer.getElapsedTimeF32(), type_ahead_timeout * 0.7f, type_ahead_timeout(), 0.4f, 0.f);

        S32 first_line = mScrollLines;
        S32 last_line = llmin((S32)mItemList.size() - 1, mScrollLines + getLinesPerPage());

        if (first_line >= mItemList.size())
        {
            return;
        }
        item_list::iterator iter;
        for (S32 line = first_line; line <= last_line; line++)
        {
            LLScrollListItem* item = mItemList[line];

            item_rect.setOriginAndSize(
                x,
                cur_y,
                mItemListRect.getWidth(),
                mLineHeight );
            item->setRect(item_rect);

            max_columns = llmax(max_columns, item->getNumColumns());

            LLColor4 fg_color;
            LLColor4 hover_color(LLColor4::transparent);
            LLColor4 select_color(LLColor4::transparent);

            if( mScrollLines <= line && line < mScrollLines + num_page_lines )
            {
                fg_color = (item->getEnabled() ? mFgUnselectedColor.get() : mFgDisabledColor.get());
                if( item->getSelected() && mCanSelect)
                {
                    if(item->getHighlighted())  // if it's highlighted, average the colors
                    {
                        select_color = lerp(mBgSelectedColor.get(), mHighlightedColor.get(), 0.5f);
                    }
                    else                        // otherwise just select-highlight it
                    {
                        select_color = mBgSelectedColor.get();
                    }

                    fg_color = (item->getEnabled() ? mFgSelectedColor.get() : mFgDisabledColor.get());
                }
                if (mHighlightedItem == line && mCanSelect)
                {
                    if(item->getHighlighted())  // if it's highlighted, average the colors
                    {
                        hover_color = lerp(mHoveredColor.get(), mHighlightedColor.get(), 0.5f);
                    }
                    else                        // otherwise just hover-highlight it
                    {
                        hover_color = mHoveredColor.get();
                    }
                }
                else if (item->getHighlighted())
                {
                    hover_color = mHighlightedColor.get();
                }
                else
                {
                    if (mDrawStripes && (line % 2 == 0) && (max_columns > 1))
                    {
                        hover_color = mBgStripeColor.get();
                    }
                }

                if (!item->getEnabled())
                {
                    hover_color = mBgReadOnlyColor.get();
                }

                item->draw(item_rect, fg_color % alpha, hover_color% alpha, select_color% alpha, highlight_color % alpha, mColumnPadding);

                cur_y -= mLineHeight;
            }
        }
    }
}


void LLScrollListCtrl::draw()
{
    LLLocalClipRect clip(getLocalRect());

    // if user specifies sort, make sure it is maintained
    updateSort();

    if (mNeedsScroll)
    {
        scrollToShowSelected();
        mNeedsScroll = false;
    }
    LLRect background(0, getRect().getHeight(), getRect().getWidth(), 0);
    // Draw background
    if (mBackgroundVisible)
    {
        F32 alpha = getCurrentTransparency();
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gl_rect_2d(background, getEnabled() ? mBgWriteableColor.get() % alpha : mBgReadOnlyColor.get() % alpha );
    }

    updateColumns();

    mCommentText->setVisible(mItemList.empty());

    drawItems();

    if (mBorder)
    {
        mBorder->setKeyboardFocusHighlight(hasFocus());
    }

    LLUICtrl::draw();
}

void LLScrollListCtrl::setEnabled(bool enabled)
{
    mCanSelect = enabled;
    setTabStop(enabled);
    mScrollbar->setTabStop(!enabled && mScrollbar->getPageSize() < mScrollbar->getDocSize());

    // when the table is disabled also disable its items
    for (LLScrollListItem* item : mItemList)
    {
        item->setEnabled(enabled);
        if (!enabled)
        {
            item->setSelected(false);
        }
    }
}

bool LLScrollListCtrl::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    bool handled = false;
    // Pretend the mouse is over the scrollbar
    handled = mScrollbar->handleScrollWheel( 0, 0, clicks );

    if (mMouseWheelOpaque)
    {
        return true;
    }

    return handled;
}

bool LLScrollListCtrl::handleScrollHWheel(S32 x, S32 y, S32 clicks)
{
    bool handled = false;
    // Pretend the mouse is over the scrollbar
    handled = mScrollbar->handleScrollHWheel( 0, 0, clicks );

    if (mMouseWheelOpaque)
    {
        return true;
    }

    return handled;
}

// *NOTE: Requires a valid row_index and column_index
LLRect LLScrollListCtrl::getCellRect(S32 row_index, S32 column_index)
{
    LLRect cell_rect;
    S32 rect_left = getColumnOffsetFromIndex(column_index) + mItemListRect.mLeft;
    S32 rect_bottom = getRowOffsetFromIndex(row_index);
    LLScrollListColumn* columnp = getColumn(column_index);
    cell_rect.setOriginAndSize(rect_left, rect_bottom,
        /*rect_left + */columnp->getWidth(), mLineHeight);
    return cell_rect;
}

bool LLScrollListCtrl::handleToolTip(S32 x, S32 y, MASK mask)
{
    S32 column_index = getColumnIndexFromOffset(x);
    LLScrollListColumn* columnp = getColumn(column_index);

    if (columnp == NULL) return false;

    bool handled = false;
    // show tooltip for full name of hovered item if it has been truncated
    LLScrollListItem* hit_item = hitItem(x, y);
    if (hit_item)
    {
        LLScrollListCell* hit_cell = hit_item->getColumn(column_index);
        if (!hit_cell) return false;
        if (hit_cell
            && hit_cell->isText()
            && hit_cell->needsToolTip())
        {
            S32 row_index = getItemIndex(hit_item);
            LLRect cell_rect = getCellRect(row_index, column_index);
            // Convert rect local to screen coordinates
            LLRect sticky_rect;
            localRectToScreen(cell_rect, &sticky_rect);

            // display tooltip exactly over original cell, in same font
            LLToolTipMgr::instance().show(LLToolTip::Params()
                                        .message(hit_cell->getToolTip())
                                        .font(LLFontGL::getFontEmojiSmall())
                                        .pos(LLCoordGL(sticky_rect.mLeft - 5, sticky_rect.mTop + 6))
                                        .delay_time(0.2f)
                                        .sticky_rect(sticky_rect));
        }
        handled = true;
    }

    // otherwise, look for a tooltip associated with this column
    LLScrollColumnHeader* headerp = columnp->mHeader;
    if (headerp && !handled)
    {
        handled = headerp->handleToolTip(x, y, mask);
    }

    return handled;
}

bool LLScrollListCtrl::selectItemAt(S32 x, S32 y, MASK mask)
{
    if (!mCanSelect) return false;

    bool selection_changed = false;

    LLScrollListItem* hit_item = hitItem(x, y);

    if( hit_item )
    {
        if( mAllowMultipleSelection )
        {
            if (mask & MASK_SHIFT)
            {
                if (mLastSelected == NULL)
                {
                    selectItem(hit_item, getColumnIndexFromOffset(x));
                }
                else
                {
                    // Select everthing between mLastSelected and hit_item
                    bool selecting = false;
                    item_list::iterator itor;
                    // If we multiselect backwards, we'll stomp on mLastSelected,
                    // meaning that we never stop selecting until hitting max or
                    // the end of the list.
                    LLScrollListItem* lastSelected = mLastSelected;
                    for (itor = mItemList.begin(); itor != mItemList.end(); ++itor)
                    {
                        if(mMaxSelectable > 0 && getAllSelected().size() >= mMaxSelectable)
                        {
                            if(mOnMaximumSelectCallback)
                            {
                                mOnMaximumSelectCallback();
                            }
                            break;
                        }
                        LLScrollListItem *item = *itor;
                        if (item == hit_item || item == lastSelected)
                        {
                            selectItem(item, getColumnIndexFromOffset(x), false);
                            selecting = !selecting;
                            if (hit_item == lastSelected)
                            {
                                // stop selecting now, since we just clicked on our last selected item
                                selecting = false;
                            }
                        }
                        if (selecting)
                        {
                            selectItem(item, getColumnIndexFromOffset(x), false);
                        }
                    }
                }
            }
            else if (mask & MASK_CONTROL)
            {
                if (hit_item->getSelected())
                {
                    deselectItem(hit_item);
                }
                else
                {
                    if(!(mMaxSelectable > 0 && getAllSelected().size() >= mMaxSelectable))
                    {
                        selectItem(hit_item, getColumnIndexFromOffset(x), false);
                    }
                    else
                    {
                        if(mOnMaximumSelectCallback)
                        {
                            mOnMaximumSelectCallback();
                        }
                    }
                }
            }
            else
            {
                deselectAllItems(true);
                selectItem(hit_item, getColumnIndexFromOffset(x));
            }
        }
        else
        {
            selectItem(hit_item, getColumnIndexFromOffset(x));
        }

        selection_changed = mSelectionChanged;
        if (mCommitOnSelectionChange)
        {
            commitIfChanged();
        }

        // clear search string on mouse operations
        mSearchString.clear();
    }
    else
    {
        //mLastSelected = NULL;
        //deselectAllItems(true);
    }

    return selection_changed;
}


bool LLScrollListCtrl::handleMouseDown(S32 x, S32 y, MASK mask)
{
    bool handled = childrenHandleMouseDown(x, y, mask) != NULL;

    if( !handled )
    {
        // set keyboard focus first, in case click action wants to move focus elsewhere
        setFocus(true);

        // clear selection changed flag because user is starting a selection operation
        mSelectionChanged = false;

        handleClick(x, y, mask);
    }

    return true;
}

bool LLScrollListCtrl::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (hasMouseCapture())
    {
        // release mouse capture immediately so
        // scroll to show selected logic will work
        gFocusMgr.setMouseCapture(NULL);
        if(mask == MASK_NONE)
        {
            selectItemAt(x, y, mask);
            mNeedsScroll = true;
        }
    }

    // always commit when mouse operation is completed inside list
    if (mItemListRect.pointInRect(x,y))
    {
        mDirty = mDirty || mSelectionChanged;
        mSelectionChanged = false;
        onCommit();
    }

    return LLUICtrl::handleMouseUp(x, y, mask);
}

// virtual
bool LLScrollListCtrl::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    LLScrollListItem *item = hitItem(x, y);
    if (item)
    {
        // check to see if we have a UUID for this row
        std::string id = item->getValue().asString();
        LLUUID uuid(id);
        if (! uuid.isNull() && mContextMenuType != MENU_NONE)
        {
            // set up the callbacks for all of the avatar/group menu items
            // (N.B. callbacks don't take const refs as id is local scope)
            bool is_group = (mContextMenuType == MENU_GROUP);
            ScopedRegistrarHelper registrar;
            registrar.add("Url.ShowProfile", boost::bind(&LLScrollListCtrl::showProfile, id, is_group));
            registrar.add("Url.SendIM", boost::bind(&LLScrollListCtrl::sendIM, id));
            registrar.add("Url.AddFriend", boost::bind(&LLScrollListCtrl::addFriend, id));
            registrar.add("Url.RemoveFriend", boost::bind(&LLScrollListCtrl::removeFriend, id));
            registrar.add("Url.ReportAbuse", boost::bind(&LLScrollListCtrl::reportAbuse, id, is_group));
            registrar.add("Url.Execute", boost::bind(&LLScrollListCtrl::showNameDetails, id, is_group));
            registrar.add("Url.CopyLabel", boost::bind(&LLScrollListCtrl::copyNameToClipboard, id, is_group));
            registrar.add("Url.CopyUrl", boost::bind(&LLScrollListCtrl::copySLURLToClipboard, id, is_group));

            // create the context menu from the XUI file and display it
            std::string menu_name = is_group ? "menu_url_group.xml" : "menu_url_agent.xml";
            auto menu = mPopupMenuHandle.get();
            if (menu)
            {
                menu->die();
                mPopupMenuHandle.markDead();
            }
            llassert(LLMenuGL::sMenuContainer != NULL);
            menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
                menu_name, LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance());
            if (menu)
            {
                mPopupMenuHandle = menu->getHandle();
                if (mIsFriendSignal)
                {
                    bool isFriend = *(*mIsFriendSignal)(uuid);
                    LLView* addFriendButton = menu->getChild<LLView>("add_friend");
                    LLView* removeFriendButton = menu->getChild<LLView>("remove_friend");

                    if (addFriendButton && removeFriendButton)
                    {
                        addFriendButton->setEnabled(!isFriend);
                        removeFriendButton->setEnabled(isFriend);
                    }
                }

                menu->show(x, y);
                LLMenuGL::showPopup(this, menu, x, y);
                return true;
            }
        }
        return LLUICtrl::handleRightMouseDown(x, y, mask);
    }
    return false;
}

void LLScrollListCtrl::showProfile(std::string id, bool is_group)
{
    // show the resident's profile or the group profile
    std::string sltype = is_group ? "group" : "agent";
    std::string slurl = "secondlife:///app/" + sltype + "/" + id + "/about";
    LLUrlAction::showProfile(slurl);
}

void LLScrollListCtrl::sendIM(std::string id)
{
    // send im to the resident
    std::string slurl = "secondlife:///app/agent/" + id + "/about";
    LLUrlAction::sendIM(slurl);
}

void LLScrollListCtrl::addFriend(std::string id)
{
    // add resident to friends list
    std::string slurl = "secondlife:///app/agent/" + id + "/about";
    LLUrlAction::addFriend(slurl);
}

void LLScrollListCtrl::removeFriend(std::string id)
{
    std::string slurl = "secondlife:///app/agent/" + id + "/about";
    LLUrlAction::removeFriend(slurl);
}

void LLScrollListCtrl::reportAbuse(std::string id, bool is_group)
{
    if (!is_group)
    {
        std::string slurl = "secondlife:///app/agent/" + id + "/about";
        LLUrlAction::reportAbuse(slurl);
    }
}

void LLScrollListCtrl::showNameDetails(std::string id, bool is_group)
{
    // open the resident's details or the group details
    std::string sltype = is_group ? "group" : "agent";
    std::string slurl = "secondlife:///app/" + sltype + "/" + id + "/about";
    LLUrlAction::clickAction(slurl, true);
}

void LLScrollListCtrl::copyNameToClipboard(std::string id, bool is_group)
{
    // copy the name of the avatar or group to the clipboard
    std::string name;
    if (is_group)
    {
        gCacheName->getGroupName(LLUUID(id), name);
    }
    else
    {
        LLAvatarName av_name;
        LLAvatarNameCache::get(LLUUID(id), &av_name);
        name = av_name.getAccountName();
    }
    LLUrlAction::copyURLToClipboard(name);
}

void LLScrollListCtrl::copySLURLToClipboard(std::string id, bool is_group)
{
    // copy a SLURL for the avatar or group to the clipboard
    std::string sltype = is_group ? "group" : "agent";
    std::string slurl = "secondlife:///app/" + sltype + "/" + id + "/about";
    LLUrlAction::copyURLToClipboard(slurl);
}

bool LLScrollListCtrl::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    //bool handled = false;
    bool handled = handleClick(x, y, mask);

    if (!handled)
    {
        // Offer the click to the children, even if we aren't enabled
        // so the scroll bars will work.
        if (NULL == LLView::childrenHandleDoubleClick(x, y, mask))
        {
            // Run the callback only if an item is being double-clicked.
            if( mCanSelect && hitItem(x, y) && mOnDoubleClickCallback )
            {
                mOnDoubleClickCallback();
            }
        }
    }

    return true;
}

bool LLScrollListCtrl::handleClick(S32 x, S32 y, MASK mask)
{
    // which row was clicked on?
    LLScrollListItem* hit_item = hitItem(x, y);
    if (!hit_item) return false;

    // get appropriate cell from that row
    S32 column_index = getColumnIndexFromOffset(x);
    LLScrollListCell* hit_cell = hit_item->getColumn(column_index);
    if (!hit_cell) return false;

    // if cell handled click directly (i.e. clicked on an embedded checkbox)
    if (hit_cell->handleClick())
    {
        // if item not currently selected, select it
        if (!hit_item->getSelected())
        {
            selectItemAt(x, y, mask);
            gFocusMgr.setMouseCapture(this);
            mNeedsScroll = true;
        }

        // propagate state of cell to rest of selected column
        {
            // propagate value of this cell to other selected items
            // and commit the respective widgets
            LLSD item_value = hit_cell->getValue();
            for (item_list::iterator iter = mItemList.begin(); iter != mItemList.end(); iter++)
            {
                LLScrollListItem* item = *iter;
                if (item->getSelected())
                {
                    LLScrollListCell* cellp = item->getColumn(column_index);
                    cellp->setValue(item_value);
                    cellp->onCommit();
                    if (mLastSelected == NULL)
                    {
                        break;
                    }
                }
            }
            //FIXME: find a better way to signal cell changes
            onCommit();
        }
        // eat click (e.g. do not trigger double click callback)
        return true;
    }
    else
    {
        // treat this as a normal single item selection
        selectItemAt(x, y, mask);
        gFocusMgr.setMouseCapture(this);
        mNeedsScroll = true;
        // do not eat click (allow double click callback)
        return false;
    }
}

LLScrollListItem* LLScrollListCtrl::hitItem( S32 x, S32 y )
{
    // Excludes disabled items.
    LLScrollListItem* hit_item = NULL;

    updateSort();

    LLRect item_rect;
    item_rect.setLeftTopAndSize(
        mItemListRect.mLeft,
        mItemListRect.mTop,
        mItemListRect.getWidth(),
        mLineHeight );

    // allow for partial line at bottom
    S32 num_page_lines = getLinesPerPage();

    S32 line = 0;
    item_list::iterator iter;
    for(iter = mItemList.begin(); iter != mItemList.end(); iter++)
    {
        LLScrollListItem* item  = *iter;
        if( mScrollLines <= line && line < mScrollLines + num_page_lines )
        {
            if( item->getEnabled() && item_rect.pointInRect( x, y ) )
            {
                hit_item = item;
                break;
            }

            item_rect.translate(0, -mLineHeight);
        }
        line++;
    }

    return hit_item;
}

S32 LLScrollListCtrl::getColumnIndexFromOffset(S32 x)
{
    // which column did we hit?
    S32 left = 0;
    S32 right = 0;
    S32 width = 0;
    S32 column_index = 0;

    ordered_columns_t::const_iterator iter = mColumnsIndexed.begin();
    ordered_columns_t::const_iterator end = mColumnsIndexed.end();
    for ( ; iter != end; ++iter)
    {
        width = (*iter)->getWidth() + mColumnPadding;
        right += width;
        if (left <= x && x < right )
        {
            break;
        }

        // set left for next column as right of current column
        left = right;
        column_index++;
    }

    return llclamp(column_index, 0, getNumColumns() - 1);
}


S32 LLScrollListCtrl::getColumnOffsetFromIndex(S32 index)
{
    S32 column_offset = 0;
    ordered_columns_t::const_iterator iter = mColumnsIndexed.begin();
    ordered_columns_t::const_iterator end = mColumnsIndexed.end();
    for ( ; iter != end; ++iter)
    {
        if (index-- <= 0)
        {
            return column_offset;
        }
        column_offset += (*iter)->getWidth() + mColumnPadding;
    }

    // when running off the end, return the rightmost pixel
    return mItemListRect.mRight;
}

S32 LLScrollListCtrl::getRowOffsetFromIndex(S32 index)
{
    S32 row_bottom = (mItemListRect.mTop - ((index - mScrollLines + 1) * mLineHeight) );
    return row_bottom;
}


bool LLScrollListCtrl::handleHover(S32 x,S32 y,MASK mask)
{
    bool    handled = false;

    if (hasMouseCapture())
    {
        if(mask == MASK_NONE)
        {
            selectItemAt(x, y, mask);
            mNeedsScroll = true;
        }
    }
    else
    if (mCanSelect)
    {
        LLScrollListItem* item = hitItem(x, y);
        if (item)
        {
            mouseOverHighlightNthItem(getItemIndex(item));
            switch (mSelectionType)
            {
            case CELL:
                item->setHoverCell(getColumnIndexFromOffset(x));
                break;
            case HEADER:
                {
                    S32 cell = getColumnIndexFromOffset(x);
                    if (cell > 0)
                    {
                        item->setHoverCell(cell);
                    }
                    else
                    {
                        item->setHoverCell(-1);
                    }
                    break;
                }
            case ROW:
                break;
            }
        }
        else
        {
            mouseOverHighlightNthItem(-1);
        }
    }

    handled = LLUICtrl::handleHover( x, y, mask );

    return handled;
}

void LLScrollListCtrl::onMouseLeave(S32 x, S32 y, MASK mask)
{
    // clear mouse highlight
    mouseOverHighlightNthItem(-1);
}

bool LLScrollListCtrl::handleKeyHere(KEY key,MASK mask )
{
    bool handled = false;

    // not called from parent means we have keyboard focus or a child does
    if (mCanSelect)
    {
        if (mask == MASK_NONE)
        {
            switch(key)
            {
            case KEY_UP:
                if (mAllowKeyboardMovement || hasFocus())
                {
                    // commit implicit in call
                    selectPrevItem(false);
                    mNeedsScroll = true;
                    handled = true;
                }
                break;
            case KEY_DOWN:
                if (mAllowKeyboardMovement || hasFocus())
                {
                    // commit implicit in call
                    selectNextItem(false);
                    mNeedsScroll = true;
                    handled = true;
                }
                break;
            case KEY_LEFT:
                if (mAllowKeyboardMovement || hasFocus())
                {
                    // TODO: support multi-select
                    LLScrollListItem *item = getFirstSelected();
                    if (item)
                    {
                        S32 cell = item->getSelectedCell();
                        switch (mSelectionType)
                        {
                        case CELL:
                            if (cell < mColumns.size()) cell++;
                            break;
                        case HEADER:
                            if (cell == -1) cell = 1;
                            else if (cell > 1 && cell < mColumns.size()) cell++; // skip header
                            break;
                        case ROW:
                            cell = -1;
                            break;
                        }
                        item->setSelectedCell(cell);
                        handled = true;
                    }
                }
                break;
            case KEY_RIGHT:
                if (mAllowKeyboardMovement || hasFocus())
                {
                    // TODO: support multi-select
                    LLScrollListItem *item = getFirstSelected();
                    if (item)
                    {
                        S32 cell = item->getSelectedCell();
                        switch (mSelectionType)
                        {
                        case CELL:
                            if (cell >= 0) cell--;
                            break;
                        case HEADER:
                            if (cell > 1) cell--;
                            else if (cell == 1) cell = -1; // skip header
                            break;
                        case ROW:
                            cell = -1;
                            break;
                        }
                        item->setSelectedCell(cell);
                        handled = true;
                    }
                }
                break;
            case KEY_PAGE_UP:
                if (mAllowKeyboardMovement || hasFocus())
                {
                    selectNthItem(getFirstSelectedIndex() - (mScrollbar->getPageSize() - 1));
                    mNeedsScroll = true;
                    if (mCommitOnKeyboardMovement
                        && !mCommitOnSelectionChange)
                    {
                        onCommit();
                    }
                    handled = true;
                }
                break;
            case KEY_PAGE_DOWN:
                if (mAllowKeyboardMovement || hasFocus())
                {
                    selectNthItem(getFirstSelectedIndex() + (mScrollbar->getPageSize() - 1));
                    mNeedsScroll = true;
                    if (mCommitOnKeyboardMovement
                        && !mCommitOnSelectionChange)
                    {
                        onCommit();
                    }
                    handled = true;
                }
                break;
            case KEY_HOME:
                if (mAllowKeyboardMovement || hasFocus())
                {
                    selectFirstItem();
                    mNeedsScroll = true;
                    if (mCommitOnKeyboardMovement
                        && !mCommitOnSelectionChange)
                    {
                        onCommit();
                    }
                    handled = true;
                }
                break;
            case KEY_END:
                if (mAllowKeyboardMovement || hasFocus())
                {
                    selectNthItem(getItemCount() - 1);
                    mNeedsScroll = true;
                    if (mCommitOnKeyboardMovement
                        && !mCommitOnSelectionChange)
                    {
                        onCommit();
                    }
                    handled = true;
                }
                break;
            case KEY_RETURN:
                // JC - Special case: Only claim to have handled it
                // if we're the special non-commit-on-move
                // type. AND we are visible
                if (!mCommitOnKeyboardMovement && mask == MASK_NONE)
                {
                    onCommit();
                    mSearchString.clear();
                    handled = true;
                }
                break;
            case KEY_BACKSPACE:
                mSearchTimer.reset();
                if (mSearchString.size())
                {
                    mSearchString.erase(mSearchString.size() - 1, 1);
                }
                if (mSearchString.empty())
                {
                    if (getFirstSelected())
                    {
                        LLScrollListCell* cellp = getFirstSelected()->getColumn(getSearchColumn());
                        if (cellp)
                        {
                            cellp->highlightText(0, 0);
                        }
                    }
                }
                else if (selectItemByPrefix(wstring_to_utf8str(mSearchString), false))
                {
                    mNeedsScroll = true;
                    // update search string only on successful match
                    mSearchTimer.reset();

                    if (mCommitOnKeyboardMovement
                        && !mCommitOnSelectionChange)
                    {
                        onCommit();
                    }
                }
                break;
            default:
                break;
            }
        }
        // TODO: multiple: shift-up, shift-down, shift-home, shift-end, select all
    }

    return handled;
}

bool LLScrollListCtrl::handleUnicodeCharHere(llwchar uni_char)
{
    if ((uni_char < 0x20) || (uni_char == 0x7F)) // Control character or DEL
    {
        return false;
    }

    // perform incremental search based on keyboard input
    static LLUICachedControl<F32> type_ahead_timeout ("TypeAheadTimeout", 0);
    if (mSearchTimer.getElapsedTimeF32() > type_ahead_timeout)
    {
        mSearchString.clear();
    }

    // type ahead search is case insensitive
    uni_char = LLStringOps::toLower((llwchar)uni_char);

    if (selectItemByPrefix(wstring_to_utf8str(mSearchString + (llwchar)uni_char), false))
    {
        // update search string only on successful match
        mNeedsScroll = true;
        mSearchString += uni_char;
        mSearchTimer.reset();

        if (mCommitOnKeyboardMovement
            && !mCommitOnSelectionChange)
        {
            onCommit();
        }
    }
    // handle iterating over same starting character
    else if (isRepeatedChars(mSearchString + (llwchar)uni_char) && !mItemList.empty())
    {
        // start from last selected item, in case we previously had a successful match against
        // duplicated characters ('AA' matches 'Aaron')
        item_list::iterator start_iter = mItemList.begin();
        S32 first_selected = getFirstSelectedIndex();

        // if we have a selection (> -1) then point iterator at the selected item
        if (first_selected > 0)
        {
            // point iterator to first selected item
            start_iter += first_selected;
        }

        // start search at first item after current selection
        item_list::iterator iter = start_iter;
        ++iter;
        if (iter == mItemList.end())
        {
            iter = mItemList.begin();
        }

        // loop around once, back to previous selection
        while(iter != start_iter)
        {
            LLScrollListItem* item = *iter;

            LLScrollListCell* cellp = item->getColumn(getSearchColumn());
            if (cellp)
            {
                // Only select enabled items with matching first characters
                LLWString item_label = utf8str_to_wstring(cellp->getValue().asString());
                if (item->getEnabled() && LLStringOps::toLower(item_label[0]) == uni_char)
                {
                    selectItem(item, -1);
                    mNeedsScroll = true;
                    cellp->highlightText(0, 1);
                    mSearchTimer.reset();

                    if (mCommitOnKeyboardMovement
                        && !mCommitOnSelectionChange)
                    {
                        onCommit();
                    }

                    break;
                }
            }

            ++iter;
            if (iter == mItemList.end())
            {
                iter = mItemList.begin();
            }
        }
    }

    return true;
}


void LLScrollListCtrl::reportInvalidInput()
{
    make_ui_sound("UISndBadKeystroke");
}

bool LLScrollListCtrl::isRepeatedChars(const LLWString& string) const
{
    if (string.empty())
    {
        return false;
    }

    llwchar first_char = string[0];

    for (U32 i = 0; i < string.size(); i++)
    {
        if (string[i] != first_char)
        {
            return false;
        }
    }

    return true;
}

void LLScrollListCtrl::selectItem(LLScrollListItem* itemp, S32 cell, bool select_single_item)
{
    if (!itemp) return;

    if (!itemp->getSelected() || itemp->getSelectedCell() != cell)
    {
        if (mLastSelected)
        {
            LLScrollListCell* cellp = mLastSelected->getColumn(getSearchColumn());
            if (cellp)
            {
                cellp->highlightText(0, 0);
            }
        }
        if (select_single_item)
        {
            deselectAllItems(true);
        }
        itemp->setSelected(true);
        switch (mSelectionType)
        {
        case CELL:
            itemp->setSelectedCell(cell);
            break;
        case HEADER:
            itemp->setSelectedCell(cell <= 0 ? -1 : cell);
            break;
        case ROW:
            itemp->setSelectedCell(-1);
            break;
        }
        mLastSelected = itemp;
        mSelectionChanged = true;
    }
}

void LLScrollListCtrl::deselectItem(LLScrollListItem* itemp)
{
    if (itemp && itemp->getSelected())
    {
        if (mLastSelected == itemp)
        {
            mLastSelected = NULL;
        }

        itemp->setSelected(false);
        LLScrollListCell* cellp = itemp->getColumn(getSearchColumn());
        if (cellp)
        {
            cellp->highlightText(0, 0);
        }
        mSelectionChanged = true;
    }
}

void LLScrollListCtrl::commitIfChanged()
{
    if (mSelectionChanged)
    {
        mDirty = true;
        mSelectionChanged = false;
        onCommit();
    }
}

struct SameSortColumn
{
    SameSortColumn(S32 column) : mColumn(column) {}
    S32 mColumn;

    bool operator()(std::pair<S32, bool> sort_column) { return sort_column.first == mColumn; }
};

bool LLScrollListCtrl::setSort(S32 column_idx, bool ascending)
{
    LLScrollListColumn* sort_column = getColumn(column_idx);
    if (!sort_column)
        return false;

    sort_column->mSortDirection = ascending ? LLScrollListColumn::ASCENDING : LLScrollListColumn::DESCENDING;

    sort_column_t new_sort_column(column_idx, ascending);

    setNeedsSort();

    if (mSortColumns.empty())
    {
        mSortColumns.push_back(new_sort_column);
        return true;
    }

    // grab current sort column
    sort_column_t cur_sort_column = mSortColumns.back();

    // remove any existing sort criterion referencing this column
    // and add the new one
    mSortColumns.erase(remove_if(mSortColumns.begin(), mSortColumns.end(), SameSortColumn(column_idx)), mSortColumns.end());
    mSortColumns.push_back(new_sort_column);

    // did the sort criteria change?
    return cur_sort_column != new_sort_column;
}

S32 LLScrollListCtrl::getLinesPerPage()
{
    if (mPageLines)
    {
        return mPageLines;
    }

    // If mPageLines is NOT provided then display all items
    return mLineHeight ? mItemListRect.getHeight() / mLineHeight : getItemCount();
}


// Called by scrollbar
void LLScrollListCtrl::onScrollChange( S32 new_pos, LLScrollbar* scrollbar )
{
    mScrollLines = new_pos;
}


void LLScrollListCtrl::sortByColumn(const std::string& name, bool ascending)
{
    column_map_t::iterator itor = mColumns.find(name);
    if (itor != mColumns.end())
    {
        sortByColumnIndex(itor->second->mIndex, ascending);
    }
}

// First column is column 0
void  LLScrollListCtrl::sortByColumnIndex(U32 column, bool ascending)
{
    setSort(column, ascending);
    updateSort();
}

void LLScrollListCtrl::updateSort() const
{
    if (hasSortOrder() && !isSorted())
    {
        // do stable sort to preserve any previous sorts
        std::stable_sort(
            mItemList.begin(),
            mItemList.end(),
            SortScrollListItem(mSortColumns,mSortCallback, mAlternateSort));

        mSorted = true;
    }
}

// for one-shot sorts, does not save sort column/order
void LLScrollListCtrl::sortOnce(S32 column, bool ascending)
{
    std::vector<std::pair<S32, bool> > sort_column;
    sort_column.push_back(std::make_pair(column, ascending));

    // do stable sort to preserve any previous sorts
    std::stable_sort(
        mItemList.begin(),
        mItemList.end(),
        SortScrollListItem(sort_column,mSortCallback,mAlternateSort));
}

void LLScrollListCtrl::dirtyColumns()
{
    mColumnsDirty = true;
    mColumnWidthsDirty = true;

    // need to keep mColumnsIndexed up to date
    // just in case someone indexes into it immediately
    mColumnsIndexed.resize(mColumns.size());

    column_map_t::iterator column_itor;
    for (column_itor = mColumns.begin(); column_itor != mColumns.end(); ++column_itor)
    {
        LLScrollListColumn *column = column_itor->second;
        mColumnsIndexed[column_itor->second->mIndex] = column;
    }
}


S32 LLScrollListCtrl::getScrollPos() const
{
    return mScrollbar->getDocPos();
}


void LLScrollListCtrl::setScrollPos( S32 pos )
{
    mScrollbar->setDocPos( pos );

    onScrollChange(mScrollbar->getDocPos(), mScrollbar);
}


void LLScrollListCtrl::scrollToShowSelected()
{
    // don't scroll automatically when capturing mouse input
    // as that will change what is currently under the mouse cursor
    if (hasMouseCapture())
    {
        return;
    }

    updateSort();

    S32 index = getFirstSelectedIndex();
    if (index < 0)
    {
        return;
    }

    LLScrollListItem* item = mItemList[index];
    if (!item)
    {
        // I don't THINK this should ever happen.
        return;
    }

    S32 lowest = mScrollLines;
    S32 page_lines = getLinesPerPage();
    S32 highest = mScrollLines + page_lines;

    if (index < lowest)
    {
        // need to scroll to show item
        setScrollPos(index);
    }
    else if (highest <= index)
    {
        setScrollPos(index - page_lines + 1);
    }
}

void LLScrollListCtrl::updateStaticColumnWidth(LLScrollListColumn* col, S32 new_width)
{
    mTotalStaticColumnWidth += llmax(0, new_width) - llmax(0, col->getWidth());
}

// LLEditMenuHandler functions

// virtual
void LLScrollListCtrl::copy()
{
    std::string buffer;

    std::vector<LLScrollListItem*> items = getAllSelected();
    std::vector<LLScrollListItem*>::iterator itor;
    for (itor = items.begin(); itor != items.end(); ++itor)
    {
        buffer += (*itor)->getContentsCSV() + "\n";
    }
    LLClipboard::instance().copyToClipboard(utf8str_to_wstring(buffer), 0, static_cast<S32>(buffer.length()));
}

// virtual
bool LLScrollListCtrl::canCopy() const
{
    return (getFirstSelected() != NULL);
}

// virtual
void LLScrollListCtrl::cut()
{
    copy();
    doDelete();
}

// virtual
bool LLScrollListCtrl::canCut() const
{
    return canCopy() && canDoDelete();
}

// virtual
void LLScrollListCtrl::selectAll()
{
    // Deselects all other items
    for (LLScrollListItem* itemp : mItemList)
    {
        if (itemp->getEnabled())
        {
            selectItem(itemp, -1, false);
        }
    }

    if (mCommitOnSelectionChange)
    {
        commitIfChanged();
    }
}

// virtual
bool LLScrollListCtrl::canSelectAll() const
{
    return getCanSelect() && mAllowMultipleSelection && !(mMaxSelectable > 0 && mItemList.size() > mMaxSelectable);
}

// virtual
void LLScrollListCtrl::deselect()
{
    deselectAllItems();
}

// virtual
bool    LLScrollListCtrl::canDeselect() const
{
    return getCanSelect();
}

void LLScrollListCtrl::addColumn(const LLSD& column, EAddPosition pos)
{
    LLScrollListColumn::Params p;
    LLParamSDParser parser;
    parser.readSD(column, p);
    addColumn(p, pos);
}

void LLScrollListCtrl::addColumn(const LLScrollListColumn::Params& column_params, EAddPosition pos)
{
    if (!column_params.validateBlock())
        return;

    std::string name = column_params.name;
    // if no column name provided, just use ordinal as name
    if (name.empty())
    {
        name = llformat("%d", mColumnsIndexed.size());
    }

    if (mColumns.find(name) == mColumns.end())
    {
        // Add column
        mColumns[name] = new LLScrollListColumn(column_params, this);
        LLScrollListColumn* new_column = mColumns[name];
        new_column->mIndex = static_cast<S32>(mColumns.size()) - 1;

        // Add button
        if (new_column->getWidth() > 0 || new_column->mRelWidth > 0 || new_column->mDynamicWidth)
        {
            if (getNumColumns() > 0)
            {
                mTotalColumnPadding += mColumnPadding;
            }
            if (new_column->mRelWidth >= 0)
            {
                new_column->setWidth((S32)ll_round(new_column->mRelWidth*mItemListRect.getWidth()));
            }
            else if (new_column->mDynamicWidth)
            {
                mNumDynamicWidthColumns++;
                new_column->setWidth((mItemListRect.getWidth() - mTotalStaticColumnWidth - mTotalColumnPadding) / mNumDynamicWidthColumns);
            }
            S32 top = mItemListRect.mTop;

            S32 left = mItemListRect.mLeft;
            for (const auto& data_pair : mColumns)
            {
                if (data_pair.second->mIndex < new_column->mIndex &&
                    data_pair.second->getWidth() > 0)
                {
                    left += data_pair.second->getWidth() + mColumnPadding;
                }
            }

            S32 right = left+new_column->getWidth();
            if (new_column->mIndex != (S32)mColumns.size()-1)
            {
                right += mColumnPadding;
            }

            LLRect temp_rect = LLRect(left,top+mHeadingHeight,right,top);

            LLScrollColumnHeader::Params params(LLUICtrlFactory::getDefaultParams<LLScrollColumnHeader>());
            params.name = "btn_" + name;
            params.rect = temp_rect;
            params.column = new_column;
            params.tool_tip = column_params.tool_tip;
            params.tab_stop = false;
            params.visible = mDisplayColumnHeaders;

            if(column_params.header.image.isProvided())
            {
                params.image_selected = column_params.header.image;
                params.image_unselected = column_params.header.image;
            }
            else
            {
                params.label = column_params.header.label;
            }

            new_column->mHeader = LLUICtrlFactory::create<LLScrollColumnHeader>(params);
            addChild(new_column->mHeader);

            sendChildToFront(mScrollbar);
        }
    }

    dirtyColumns();
}

// static
void LLScrollListCtrl::onClickColumn(void *userdata)
{
    LLScrollListColumn *info = (LLScrollListColumn*)userdata;
    if (!info) return;

    LLScrollListCtrl *parent = info->mParentCtrl;
    if (!parent) return;

    if (!parent->mCanSort) return;

    S32 column_index = info->mIndex;

    LLScrollListColumn* column = parent->mColumnsIndexed[info->mIndex];
    bool ascending = column->mSortDirection == LLScrollListColumn::ASCENDING;
    if (column->mSortingColumn != column->mName
        && parent->mColumns.find(column->mSortingColumn) != parent->mColumns.end())
    {
        LLScrollListColumn* info_redir = parent->mColumns[column->mSortingColumn];
        column_index = info_redir->mIndex;
    }

    // if this column is the primary sort key, reverse the direction
    if (!parent->mSortColumns.empty() && parent->mSortColumns.back().first == column_index)
    {
        ascending = !parent->mSortColumns.back().second;
    }

    parent->sortByColumnIndex(column_index, ascending);

    if (parent->mOnSortChangedCallback)
    {
        parent->mOnSortChangedCallback();
    }
}

std::string LLScrollListCtrl::getSortColumnName()
{
    LLScrollListColumn* column = mSortColumns.empty() ? NULL : mColumnsIndexed[mSortColumns.back().first];

    return column ? column->mName : LLStringUtil::null;
}

bool LLScrollListCtrl::hasSortOrder() const
{
    return !mSortColumns.empty();
}

void LLScrollListCtrl::clearSortOrder()
{
    mSortColumns.clear();
}

void LLScrollListCtrl::clearColumns()
{
    column_map_t::iterator itor;
    for (itor = mColumns.begin(); itor != mColumns.end(); ++itor)
    {
        LLScrollColumnHeader *header = itor->second->mHeader;
        if (header)
        {
            removeChild(header);
            delete header;
        }
    }
    std::for_each(mColumns.begin(), mColumns.end(), DeletePairedPointer());
    mColumns.clear();
    mSortColumns.clear();
    mTotalStaticColumnWidth = 0;
    mTotalColumnPadding = 0;

    dirtyColumns(); // Clears mColumnsIndexed
}

void LLScrollListCtrl::setColumnLabel(const std::string& column, const std::string& label)
{
    LLScrollListColumn* columnp = getColumn(column);
    if (columnp)
    {
        columnp->mLabel = label;
        if (columnp->mHeader)
        {
            columnp->mHeader->setLabel(label);
        }
    }
}

LLScrollListColumn* LLScrollListCtrl::getColumn(S32 index)
{
    if (index < 0 || index >= (S32)mColumnsIndexed.size())
    {
        return NULL;
    }
    return mColumnsIndexed[index];
}

LLScrollListColumn* LLScrollListCtrl::getColumn(const std::string& name)
{
    column_map_t::iterator column_itor = mColumns.find(name);
    if (column_itor != mColumns.end())
    {
        return column_itor->second;
    }
    return NULL;
}

LLScrollListItem* LLScrollListCtrl::addElement(const LLSD& element, EAddPosition pos, void* userdata)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    LLScrollListItem::Params item_params;
    LLParamSDParser parser;
    parser.readSD(element, item_params);
    item_params.userdata = userdata;
    return addRow(item_params, pos);
}

LLScrollListItem* LLScrollListCtrl::addRow(const LLScrollListItem::Params& item_p, EAddPosition pos)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    LLScrollListItem *new_item = new LLScrollListItem(item_p);
    return addRow(new_item, item_p, pos);
}

LLScrollListItem* LLScrollListCtrl::addRow(LLScrollListItem *new_item, const LLScrollListItem::Params& item_p, EAddPosition pos)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    if (!item_p.validateBlock() || !new_item)
        return nullptr;

    new_item->setNumColumns((S32)mColumns.size());

    // Add any columns we don't already have
    S32 col_index = 0;

    for (LLScrollListCell::Params cell_p : item_p.columns)
    {
        std::string column = cell_p.column;

        // empty columns strings index by ordinal
        if (column.empty())
        {
            column = llformat("%d", col_index);
        }

        LLScrollListColumn* columnp = getColumn(column);

        // create new column on demand
        if (!columnp)
        {
            LLScrollListColumn::Params new_column;
            new_column.name = column;
            new_column.header.label = column;

            // if width supplied for column, use it, otherwise
            // use adaptive width
            if (cell_p.width.isProvided())
            {
                new_column.width.pixel_width = cell_p.width;
            }
            addColumn(new_column);
            columnp = mColumns[column];
            new_item->setNumColumns(static_cast<S32>(mColumns.size()));
        }

        S32 index = columnp->mIndex;
        if (!cell_p.width.isProvided())
        {
            cell_p.width = columnp->getWidth();
        }

        LLScrollListCell* cell = LLScrollListCell::create(cell_p);

        if (cell)
        {
            new_item->setColumn(index, cell);
            if (columnp->mHeader
                && cell->isText()
                && !cell->getValue().asString().empty())
            {
                columnp->mHeader->setHasResizableElement(true);
            }
        }

        col_index++;
    }

    if (item_p.columns.empty())
    {
        if (mColumns.empty())
        {
            LLScrollListColumn::Params new_column;
            new_column.name = "0";

            addColumn(new_column);
            new_item->setNumColumns(static_cast<S32>(mColumns.size()));
        }

        if (LLScrollListCell* cell = LLScrollListCell::create(LLScrollListCell::Params().value(item_p.value)))
        {
            LLScrollListColumn* columnp = mColumns.begin()->second;

            new_item->setColumn(0, cell);
            if (columnp->mHeader
                && cell->isText()
                && !cell->getValue().asString().empty())
            {
                columnp->mHeader->setHasResizableElement(true);
            }
        }
    }

    // add dummy cells for missing columns
    for (const auto& column_it : mColumns)
    {
        S32 column_idx = column_it.second->mIndex;
        if (new_item->getColumn(column_idx) == NULL)
        {
            LLScrollListCell::Params cell_p;
            cell_p.width = column_it.second->getWidth();
            new_item->setColumn(column_idx, new LLScrollListSpacer(cell_p));
        }
    }

    addItem(new_item, pos);
    return new_item;
}

LLScrollListItem* LLScrollListCtrl::addSimpleElement(const std::string& value, EAddPosition pos, const LLSD& id)
{
    LLSD entry_id = id;

    if (id.isUndefined())
    {
        entry_id = value;
    }

    LLScrollListItem::Params item_params;
    item_params.value(entry_id);
    item_params.columns.add()
        .value(value)
        .font(LLFontGL::getFontEmojiSmall());

    return addRow(item_params, pos);
}

void LLScrollListCtrl::setValue(const LLSD& value )
{
    LLSD::array_const_iterator itor;
    for (itor = value.beginArray(); itor != value.endArray(); ++itor)
    {
        addElement(*itor);
    }
}

LLSD LLScrollListCtrl::getValue() const
{
    LLScrollListItem *item = getFirstSelected();
    if (!item) return LLSD();
    return item->getValue();
}

bool LLScrollListCtrl::operateOnSelection(EOperation op)
{
    if (op == OP_DELETE)
    {
        deleteSelectedItems();
        return true;
    }
    else if (op == OP_DESELECT)
    {
        deselectAllItems();
    }
    return false;
}

bool LLScrollListCtrl::operateOnAll(EOperation op)
{
    if (op == OP_DELETE)
    {
        clearRows();
        return true;
    }
    else if (op == OP_DESELECT)
    {
        deselectAllItems();
    }
    else if (op == OP_SELECT)
    {
        selectAll();
    }
    return false;
}
//virtual
void LLScrollListCtrl::setFocus(bool b)
{
    // for tabbing into pristine scroll lists (Finder)
    if (!getFirstSelected())
    {
        selectFirstItem();
        //onCommit(); // SJB: selectFirstItem() will call onCommit() if appropriate
    }
    LLUICtrl::setFocus(b);
}


// virtual
bool    LLScrollListCtrl::isDirty() const
{
    bool grubby = mDirty;
    if ( !mAllowMultipleSelection )
    {
        grubby = (mOriginalSelection != getFirstSelectedIndex());
    }
    return grubby;
}

// Clear dirty state
void LLScrollListCtrl::resetDirty()
{
    mDirty = false;
    mOriginalSelection = getFirstSelectedIndex();
}


//virtual
void LLScrollListCtrl::onFocusReceived()
{
    // forget latent selection changes when getting focus
    mSelectionChanged = false;
    LLUICtrl::onFocusReceived();
}

//virtual
void LLScrollListCtrl::onFocusLost()
{
    if (hasMouseCapture())
    {
        gFocusMgr.setMouseCapture(NULL);
    }

    mSearchString.clear();

    LLUICtrl::onFocusLost();
}

boost::signals2::connection LLScrollListCtrl::setIsFriendCallback(const is_friend_signal_t::slot_type& cb)
{
    if (!mIsFriendSignal)
    {
        mIsFriendSignal = new is_friend_signal_t();
    }
    return mIsFriendSignal->connect(cb);
}

bool LLScrollListCtrl::highlightMatchingItems(const std::string& filter_str)
{
    if (filter_str == "" || filter_str == " ")
    {
        clearHighlightedItems();
        return false;
    }

    bool res = false;

    setHighlightedColor(LLUIColorTable::instance().getColor("SearchableControlHighlightColor", LLColor4::red));

    std::string filter_str_lc(filter_str);
    LLStringUtil::toLower(filter_str_lc);

    std::vector<LLScrollListItem*> data = getAllData();
    std::vector<LLScrollListItem*>::iterator iter = data.begin();
    while (iter != data.end())
    {
        LLScrollListCell* cell = (*iter)->getColumn(0);
        if (cell)
        {
            std::string value = cell->getValue().asString();
            LLStringUtil::toLower(value);
            if (value.find(filter_str_lc) == std::string::npos)
            {
                (*iter)->setHighlighted(false);
            }
            else
            {
                (*iter)->setHighlighted(true);
                res = true;
            }
        }
        iter++;
    }
    return res;
}
