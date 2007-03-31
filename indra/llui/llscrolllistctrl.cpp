/** 
 * @file llscrolllistctrl.cpp
 * @brief LLScrollListCtrl base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include <algorithm>

#include "linden_common.h"
#include "llstl.h"
#include "llboost.h"

#include "llscrolllistctrl.h"

#include "indra_constants.h"

#include "llcheckboxctrl.h"
#include "llclipboard.h"
#include "llfocusmgr.h"
#include "llgl.h"
#include "llglheaders.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llstring.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llcontrol.h"
#include "llkeyboard.h"
#include "llresizebar.h"

const S32 LIST_BORDER_PAD = 2;		// white space inside the border and to the left of the scrollbar
const S32 MIN_COLUMN_WIDTH = 20;
const S32 LIST_SNAP_PADDING = 5;

// local structures & classes.
struct SortScrollListItem
{
	SortScrollListItem(const S32 sort_col, BOOL sort_ascending)
	{
		mSortCol = sort_col;
		mSortAscending = sort_ascending;
	}

	bool operator()(const LLScrollListItem* i1, const LLScrollListItem* i2)
	{
		const LLScrollListCell *cell1;
		const LLScrollListCell *cell2;
		
		cell1 = i1->getColumn(mSortCol);
		cell2 = i2->getColumn(mSortCol);
		
		S32 order = 1;
		if (!mSortAscending)
		{
			order = -1;
		}

		BOOL retval = FALSE;

		if (cell1 && cell2)
		{
			retval = ((order * LLString::compareDict(cell1->getText(),	cell2->getText())) < 0);
		}

		return (retval ? TRUE : FALSE);
	}

protected:
	S32 mSortCol;
	S32 mSortAscending;
};



//
// LLScrollListIcon
//
LLScrollListIcon::LLScrollListIcon(LLImageGL* icon, S32 width, LLUUID image_id) :
mIcon(icon), mImageUUID(image_id.asString())
{
	if (width)
	{
		mWidth = width;
	}
	else
	{
		mWidth = icon->getWidth();
	}
}

LLScrollListIcon::~LLScrollListIcon()
{
}

//
// LLScrollListCheck
//
LLScrollListCheck::LLScrollListCheck(LLCheckBoxCtrl* check_box, S32 width)
{
	mCheckBox = check_box;
	LLRect rect(mCheckBox->getRect());
	if (width)
	{
		
		rect.mRight = rect.mLeft + width;
		mCheckBox->setRect(rect);
		mWidth = width;
	}
	else
	{
		mWidth = rect.getWidth(); //check_box->getWidth();
	}
}

LLScrollListCheck::~LLScrollListCheck()
{
	delete mCheckBox;
}

void LLScrollListCheck::drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const
{
	mCheckBox->draw();

}

BOOL LLScrollListCheck::handleClick()
{ 
	if ( mCheckBox->getEnabled() )
	{
		LLCheckBoxCtrl::onButtonPress(mCheckBox); 
	}
	return TRUE; 
}

//
// LLScrollListText
//
U32 LLScrollListText::sCount = 0;

LLScrollListText::LLScrollListText( const LLString& text, const LLFontGL* font, S32 width, U8 font_style, LLFontGL::HAlign font_alignment, LLColor4& color, BOOL use_color, BOOL visible)
:	mText( text ),
	mFont( font ),
	mFontStyle( font_style ),
	mFontAlignment( font_alignment ),
	mWidth( width ),
	mVisible( visible ),
	mHighlightCount( 0 ),
	mHighlightOffset( 0 )
{
	if (use_color)
	{
		mColor = new LLColor4();
		mColor->setVec(color);
	}
	else
	{
		mColor = NULL;
	}

	sCount++;

	// initialize rounded rect image
	if (!mRoundedRectImage)
	{
		mRoundedRectImage = LLUI::sImageProvider->getUIImageByID(LLUUID(LLUI::sAssetsGroup->getString("rounded_square.tga")));
	}
}

LLScrollListText::~LLScrollListText()
{
	sCount--;
	delete mColor;
}

void LLScrollListText::setText(const LLString& text)
{
	mText = text;
}

void LLScrollListText::drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const
{
	// If the user has specified a small minimum width, use that.
	if (mWidth > 0 && mWidth < width)
	{
		width = mWidth;
	}

	const LLColor4* display_color;
	if (mColor)
	{
		display_color = mColor;
	}
	else
	{
		display_color = &color;
	}

	if (mHighlightCount > 0)
	{
		mRoundedRectImage->bind();
		glColor4fv(highlight_color.mV);
		S32 left = 0;
		switch(mFontAlignment)
		{
		case LLFontGL::LEFT:
			left = mFont->getWidth(mText.getString(), 0, mHighlightOffset);
			break;
		case LLFontGL::RIGHT:
			left = width - mFont->getWidth(mText.getString(), mHighlightOffset, S32_MAX);
			break;
		case LLFontGL::HCENTER:
			left = (width - mFont->getWidth(mText.getString())) / 2;
			break;
		}
		gl_segmented_rect_2d_tex(left - 2, 
				llround(mFont->getLineHeight()) + 1, 
				left + mFont->getWidth(mText.getString(), mHighlightOffset, mHighlightCount) + 1, 
				1, 
				mRoundedRectImage->getWidth(), 
				mRoundedRectImage->getHeight(), 
				16);
	}

	// Try to draw the entire string
	F32 right_x;
	U32 string_chars = mText.length();
	F32 start_x = 0.f;
	switch(mFontAlignment)
	{
	case LLFontGL::LEFT:
		start_x = 0.f;
		break;
	case LLFontGL::RIGHT:
		start_x = (F32)width;
		break;
	case LLFontGL::HCENTER:
		start_x = (F32)width * 0.5f;
		break;
	}
	mFont->render(mText.getWString(), 0, 
						start_x, 2.f,
						*display_color,
						mFontAlignment,
						LLFontGL::BOTTOM, 
						mFontStyle,
						string_chars, 
						width,
						&right_x, FALSE, TRUE);
}


LLScrollListItem::~LLScrollListItem()
{
	std::for_each(mColumns.begin(), mColumns.end(), DeletePointer());
}

BOOL LLScrollListItem::handleClick(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	S32 left = 0;
	S32 right = 0;
	S32 width = 0;

	std::vector<LLScrollListCell *>::iterator iter = mColumns.begin();
	std::vector<LLScrollListCell *>::iterator end = mColumns.end();
	for ( ; iter != end; ++iter)
	{
		width = (*iter)->getWidth();
		right += width;
		if (left <= x && x < right )
		{
			handled = (*iter)->handleClick();
			break;
		}
		
		left += width;
	}
	return handled;
}

void LLScrollListItem::setNumColumns(S32 columns)
{
	S32 prev_columns = mColumns.size();
	if (columns < prev_columns)
	{
		std::for_each(mColumns.begin()+columns, mColumns.end(), DeletePointer());
	}
	
	mColumns.resize(columns);

	for (S32 col = prev_columns; col < columns; ++col)
	{
		mColumns[col] = NULL;
	}
}

void LLScrollListItem::setColumn( S32 column, LLScrollListCell *cell )
{
	if (column < (S32)mColumns.size())
	{
		delete mColumns[column];
		mColumns[column] = cell;
	}
	else
	{
		llerrs << "LLScrollListItem::setColumn: bad column: " << column << llendl;
	}
}

LLString LLScrollListItem::getContentsCSV()
{
	LLString ret;

	S32 count = getNumColumns();
	for (S32 i=0; i<count; ++i)
	{
		ret += getColumn(i)->getText();
		if (i < count-1)
		{
			ret += ", ";
		}
	}

	return ret;
}

void LLScrollListItem::setEnabled(BOOL b)
{
	if (b != mEnabled)
	{
		std::vector<LLScrollListCell *>::iterator iter = mColumns.begin();
		std::vector<LLScrollListCell *>::iterator end = mColumns.end();
		for ( ; iter != end; ++iter)
		{
			(*iter)->setEnabled(b);
		}
		mEnabled = b;
	}
}

//---------------------------------------------------------------------------
// LLScrollListCtrl
//---------------------------------------------------------------------------

LLScrollListCtrl::LLScrollListCtrl(const LLString& name, const LLRect& rect,
	void (*commit_callback)(LLUICtrl* ctrl, void* userdata),
	void* callback_user_data,
	BOOL allow_multiple_selection,
	BOOL show_border
	)
 :	LLUICtrl(name, rect, TRUE, commit_callback, callback_user_data),
	mLineHeight(0),
	mScrollLines(0),
	mPageLines(0),
	mHeadingHeight(20),
	mMaxSelectable(0),
	mAllowMultipleSelection( allow_multiple_selection ),
	mAllowKeyboardMovement(TRUE),
	mCommitOnKeyboardMovement(TRUE),
	mCommitOnSelectionChange(FALSE),
	mSelectionChanged(FALSE),
	mCanSelect(TRUE),
	mDisplayColumnHeaders(FALSE),
	mCollapseEmptyColumns(FALSE),
	mIsPopup(FALSE),
	mMaxItemCount(INT_MAX), 
	//mItemCount(0),
	mBackgroundVisible( TRUE ),
	mDrawStripes(TRUE),
	mBgWriteableColor(	LLUI::sColorsGroup->getColor( "ScrollBgWriteableColor" ) ),
	mBgReadOnlyColor(	LLUI::sColorsGroup->getColor( "ScrollBgReadOnlyColor" ) ),
	mBgSelectedColor( LLUI::sColorsGroup->getColor("ScrollSelectedBGColor") ),
	mBgStripeColor( LLUI::sColorsGroup->getColor("ScrollBGStripeColor") ),
	mFgSelectedColor( LLUI::sColorsGroup->getColor("ScrollSelectedFGColor") ),
	mFgUnselectedColor( LLUI::sColorsGroup->getColor("ScrollUnselectedColor") ),
	mFgDisabledColor( LLUI::sColorsGroup->getColor("ScrollDisabledColor") ),
	mHighlightedColor( LLUI::sColorsGroup->getColor("ScrollHighlightedColor") ),
	mHighlightedItem(-1),
	mBorderThickness( 2 ),
	mOnDoubleClickCallback( NULL ),
	mOnMaximumSelectCallback( NULL ),
	mOnSortChangedCallback( NULL ),
	mDrewSelected(FALSE),
	mBorder(NULL),
	mSearchColumn(0),
	mDefaultColumn("SIMPLE"),

	mNumDynamicWidthColumns(0),
	mTotalStaticColumnWidth(0),
	mSortColumn(-1),
	mSortAscending(TRUE)
{
	mItemListRect.setOriginAndSize(
		mBorderThickness + LIST_BORDER_PAD,
		mBorderThickness + LIST_BORDER_PAD,
		mRect.getWidth() - 2*( mBorderThickness + LIST_BORDER_PAD ) - SCROLLBAR_SIZE,
		mRect.getHeight() - 2*( mBorderThickness + LIST_BORDER_PAD ) );

	updateLineHeight();

	mPageLines = mLineHeight? (mItemListRect.getHeight()) / mLineHeight : 0;

	// Init the scrollbar
	LLRect scroll_rect;
	scroll_rect.setOriginAndSize( 
		mRect.getWidth() - mBorderThickness - SCROLLBAR_SIZE,
		mItemListRect.mBottom,
		SCROLLBAR_SIZE,
		mItemListRect.getHeight());
	mScrollbar = new LLScrollbar( "Scrollbar", scroll_rect,
		LLScrollbar::VERTICAL,
		getItemCount(),
		mScrollLines,
		mPageLines,
		&LLScrollListCtrl::onScrollChange, this );
	mScrollbar->setFollowsRight();
	mScrollbar->setFollowsTop();
	mScrollbar->setFollowsBottom();
	mScrollbar->setEnabled( TRUE );
	mScrollbar->setVisible( TRUE );
	addChild(mScrollbar);

	// Border
	if (show_border)
	{
		LLRect border_rect( 0, mRect.getHeight(), mRect.getWidth(), 0 );
		mBorder = new LLViewBorder( "dlg border", border_rect, LLViewBorder::BEVEL_IN, LLViewBorder::STYLE_LINE, 1 );
		addChild(mBorder);
	}

	mColumnPadding = 5;

	mLastSelected = NULL;
}

LLScrollListCtrl::~LLScrollListCtrl()
{
	std::for_each(mItemList.begin(), mItemList.end(), DeletePointer());

	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}
}


BOOL LLScrollListCtrl::setMaxItemCount(S32 max_count)
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
	return mItemList.size();
}

// virtual LLScrolListInterface function (was deleteAllItems)
void LLScrollListCtrl::clearRows()
{
	std::for_each(mItemList.begin(), mItemList.end(), DeletePointer());
	mItemList.clear();
	//mItemCount = 0;

	// Scroll the bar back up to the top.
	mScrollbar->setDocParams(0, 0);

	mScrollLines = 0;
	mLastSelected = NULL;
	updateMaxContentWidth(NULL);
}


LLScrollListItem* LLScrollListCtrl::getFirstSelected() const
{
	item_list::const_iterator iter;
	for(iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item  = *iter;
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
	item_list::const_iterator iter;
	for(iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item  = *iter;
		if (item->getSelected())
		{
			ret.push_back(item);
		}
	}
	return ret;
}

S32 LLScrollListCtrl::getFirstSelectedIndex()
{
	S32 CurSelectedIndex = 0;
	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item  = *iter;
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
	if (mItemList.size() == 0)
	{
		return NULL;
	}
	return mItemList[0];
}

LLScrollListItem* LLScrollListCtrl::getLastData() const
{
	if (mItemList.size() == 0)
	{
		return NULL;
	}
	return mItemList[mItemList.size() - 1];
}

std::vector<LLScrollListItem*> LLScrollListCtrl::getAllData() const
{
	std::vector<LLScrollListItem*> ret;
	item_list::const_iterator iter;
	for(iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item  = *iter;
		ret.push_back(item);
	}
	return ret;
}


void LLScrollListCtrl::reshape( S32 width, S32 height, BOOL called_from_parent )
{
	LLUICtrl::reshape( width, height, called_from_parent );

	S32 heading_size = (mDisplayColumnHeaders ? mHeadingHeight : 0);

	mItemListRect.setOriginAndSize(
		mBorderThickness + LIST_BORDER_PAD,
		mBorderThickness + LIST_BORDER_PAD,
		mRect.getWidth() - 2*( mBorderThickness + LIST_BORDER_PAD ) - SCROLLBAR_SIZE,
		mRect.getHeight() - 2*( mBorderThickness + LIST_BORDER_PAD ) - heading_size );

	mPageLines = mLineHeight? mItemListRect.getHeight() / mLineHeight : 0;
	mScrollbar->setVisible(mPageLines < getItemCount());
	mScrollbar->setPageSize( mPageLines );

	updateColumns();
}

// Attempt to size the control to show all items.
// Do not make larger than width or height.
void LLScrollListCtrl::arrange(S32 max_width, S32 max_height)
{
	S32 height = mLineHeight * (getItemCount() + 1);
	height = llmin( height, max_height );

	S32 width = mRect.getWidth();

	reshape( width, height );
}


LLRect LLScrollListCtrl::getRequiredRect()
{
	S32 height = mLineHeight * (getItemCount() + 1);
	S32 width = mRect.getWidth();

	return LLRect(0, height, width, 0);
}


BOOL LLScrollListCtrl::addItem( LLScrollListItem* item, EAddPosition pos )
{
	BOOL not_too_big = getItemCount() < mMaxItemCount;
	if (not_too_big)
	{
		switch( pos )
		{
		case ADD_TOP:
			mItemList.push_front(item);
			break;
	
		case ADD_SORTED:
			mSortColumn = 0;
			mSortAscending = TRUE;
			mItemList.push_back(item);
			std::sort(mItemList.begin(), mItemList.end(), SortScrollListItem(mSortColumn, mSortAscending));
			break;
	
		case ADD_BOTTOM:
			mItemList.push_back(item);
			break;
	
		default:
			llassert(0);
			mItemList.push_back(item);
			break;
		}
	
		updateLineHeight();
		mPageLines = mLineHeight ? mItemListRect.getHeight() / mLineHeight : 0;
		BOOL scrollbar_visible = mPageLines < getItemCount();
		
		if (scrollbar_visible != mScrollbar->getVisible())
		{
			mScrollbar->setVisible(mPageLines < getItemCount());
			updateColumns();
		}
		mScrollbar->setPageSize( mPageLines );
	
		mScrollbar->setDocSize( getItemCount() );

		updateMaxContentWidth(item);
	}

	return not_too_big;
}

void LLScrollListCtrl::updateMaxContentWidth(LLScrollListItem* added_item)
{
	const S32 HEADING_TEXT_PADDING = 30;
	const S32 COLUMN_TEXT_PADDING = 20;

	std::map<LLString, LLScrollListColumn>::iterator column_itor;
	for (column_itor = mColumns.begin(); column_itor != mColumns.end(); ++column_itor)
	{
		LLScrollListColumn* column = &column_itor->second;

		if (!added_item)
		{
			// update on all items
			column->mMaxContentWidth = column->mHeader ? LLFontGL::sSansSerifSmall->getWidth(column->mLabel) + mColumnPadding + HEADING_TEXT_PADDING : 0;
			item_list::iterator iter;
			for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
			{
				LLScrollListCell* cellp = (*iter)->getColumn(column->mIndex);
				if (!cellp) continue;

				column->mMaxContentWidth = llmax(LLFontGL::sSansSerifSmall->getWidth(cellp->getText()) + mColumnPadding + COLUMN_TEXT_PADDING, column->mMaxContentWidth);
			}
		}
		else
		{
			LLScrollListCell* cellp = added_item->getColumn(column->mIndex);
			if (!cellp) continue;

			column->mMaxContentWidth = llmax(LLFontGL::sSansSerifSmall->getWidth(cellp->getText()) + mColumnPadding + COLUMN_TEXT_PADDING, column->mMaxContentWidth);
		}
	}
}


// Line height is the max height of all the cells in all the items.
void LLScrollListCtrl::updateLineHeight()
{
	const S32 ROW_PAD = 2;

	mLineHeight = 0;
	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem *itemp = *iter;
		S32 num_cols = itemp->getNumColumns();
		S32 i = 0;
		for (const LLScrollListCell* cell = itemp->getColumn(i); i < num_cols; cell = itemp->getColumn(++i))
		{
			mLineHeight = llmax( mLineHeight, cell->getHeight() + ROW_PAD );
		}
	}
}

void LLScrollListCtrl::updateColumns()
{
	mColumnsIndexed.resize(mColumns.size());

	std::map<LLString, LLScrollListColumn>::iterator column_itor;
	for (column_itor = mColumns.begin(); column_itor != mColumns.end(); ++column_itor)
	{
		LLScrollListColumn *column = &column_itor->second;
		S32 new_width = column->mWidth;
		if (column->mRelWidth >= 0)
		{
			new_width = (S32)llround(column->mRelWidth*mItemListRect.getWidth());
		}
		else if (column->mDynamicWidth)
		{
			new_width = (mItemListRect.getWidth() - mTotalStaticColumnWidth) / mNumDynamicWidthColumns;
		}

		if (new_width != column->mWidth)
		{
			column->mWidth = new_width;
		}
		mColumnsIndexed[column_itor->second.mIndex] = column;
	}

	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem *itemp = *iter;
		S32 num_cols = itemp->getNumColumns();
		S32 i = 0;
		for (LLScrollListCell* cell = itemp->getColumn(i); i < num_cols; cell = itemp->getColumn(++i))
		{
			if (i >= (S32)mColumnsIndexed.size()) break;

			cell->setWidth(mColumnsIndexed[i]->mWidth);
		}
	}

	// update headers
	std::vector<LLScrollListColumn*>::iterator column_ordered_it;
	S32 left = mItemListRect.mLeft;
	LLColumnHeader* last_header = NULL;
	for (column_ordered_it = mColumnsIndexed.begin(); column_ordered_it != mColumnsIndexed.end(); ++column_ordered_it)
	{
		if ((*column_ordered_it)->mWidth <= 0)
		{
			// skip hidden columns	
		}
		LLScrollListColumn* column = *column_ordered_it;
		
		if (column->mHeader)
		{
			last_header = column->mHeader;
			S32 top = mItemListRect.mTop;
			S32 right = left + column->mWidth;

			if (column->mIndex != (S32)mColumnsIndexed.size()-1)
			{
				right += mColumnPadding;
			}
			right = llmax(left, llmin(mItemListRect.getWidth(), right));

			S32 header_width = right - left;

			last_header->reshape(header_width, mHeadingHeight);
			last_header->translate(left - last_header->getRect().mLeft, top - last_header->getRect().mBottom);
			last_header->setVisible(mDisplayColumnHeaders && header_width > 0);
			left = right;
		}
	}

	// expand last column header we encountered to full list width
	if (last_header)
	{
		S32 header_strip_width = mItemListRect.getWidth() + (mScrollbar->getVisible() ? 0 : SCROLLBAR_SIZE);
		S32 new_width = llmax(0, mItemListRect.mLeft + header_strip_width - last_header->getRect().mLeft);
		last_header->reshape(new_width, last_header->getRect().getHeight());
	}
}

void LLScrollListCtrl::setDisplayHeading(BOOL display)
{
	mDisplayColumnHeaders = display;

	updateColumns();

	setHeadingHeight(mHeadingHeight);
}

void LLScrollListCtrl::setHeadingHeight(S32 heading_height)
{
	mHeadingHeight = heading_height;

	reshape(mRect.getWidth(), mRect.getHeight());

	// Resize
	mScrollbar->reshape(SCROLLBAR_SIZE, mItemListRect.getHeight() + (mDisplayColumnHeaders ? mHeadingHeight : 0));
}

void LLScrollListCtrl::setCollapseEmptyColumns(BOOL collapse)
{
	mCollapseEmptyColumns = collapse;
}

BOOL LLScrollListCtrl::selectFirstItem()
{
	BOOL success = FALSE;

	// our $%&@#$()^%#$()*^ iterators don't let us check against the first item inside out iteration
	BOOL first_item = TRUE;

	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem *itemp = *iter;
		if( first_item && itemp->getEnabled() )
		{
			if (!itemp->getSelected())
			{
				selectItem(itemp);
			}
			success = TRUE;
		}
		else
		{
			deselectItem(itemp);
		}
		first_item = FALSE;
	}
	if (mCommitOnSelectionChange)
	{
		commitIfChanged();
	}
	return success;
}


BOOL LLScrollListCtrl::selectNthItem( S32 target_index )
{
	// Deselects all other items
	BOOL success = FALSE;
	S32 index = 0;

	target_index = llclamp(target_index, 0, (S32)mItemList.size() - 1);

	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem *itemp = *iter;
		if( target_index == index )
		{
			if( itemp->getEnabled() )
			{
				selectItem(itemp);
				success = TRUE;
			}
		}
		else
		{
			deselectItem(itemp);
		}
		index++;
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

	LLScrollListItem *cur_itemp = mItemList[index];
	mItemList[index] = mItemList[index - 1];
	mItemList[index - 1] = cur_itemp;
}


void LLScrollListCtrl::deleteSingleItem(S32 target_index)
{
	if (target_index >= (S32)mItemList.size())
	{
		return;
	}

	LLScrollListItem *itemp;
	itemp = mItemList[target_index];
	if (itemp == mLastSelected)
	{
		mLastSelected = NULL;
	}
	delete itemp;
	mItemList.erase(mItemList.begin() + target_index);
	updateMaxContentWidth(NULL);
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
	updateMaxContentWidth(NULL);
}

void LLScrollListCtrl::highlightNthItem(S32 target_index)
{
	if (mHighlightedItem != target_index)
	{
		mHighlightedItem = target_index;
		llinfos << "Highlighting item " << target_index << llendl;
	}
}

S32	LLScrollListCtrl::selectMultiple( LLDynamicArray<LLUUID> ids )
{
	item_list::iterator iter;
	S32 count = 0;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item = *iter;
		LLDynamicArray<LLUUID>::iterator iditr;
		for(iditr = ids.begin(); iditr != ids.end(); ++iditr)
		{
			if (item->getEnabled() && (item->getUUID() == (*iditr)))
			{
				selectItem(item,FALSE);
				++count;
				break;
			}
		}
		if(ids.end() != iditr) ids.erase(iditr);
	}

	if (mCommitOnSelectionChange)
	{
		commitIfChanged();
	}
	return count;
}

S32 LLScrollListCtrl::getItemIndex( LLScrollListItem* target_item )
{
	S32 index = 0;
	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem *itemp = *iter;
		if (target_item == itemp)
		{
			return index;
		}
		index++;
	}
	return -1;
}

S32 LLScrollListCtrl::getItemIndex( LLUUID& target_id )
{
	S32 index = 0;
	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem *itemp = *iter;
		if (target_id == itemp->getUUID())
		{
			return index;
		}
		index++;
	}
	return -1;
}

void LLScrollListCtrl::selectPrevItem( BOOL extend_selection)
{
	LLScrollListItem* prev_item = NULL;

	if (!getFirstSelected())
	{
		selectFirstItem();
	}
	else
	{
		item_list::iterator iter;
		for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
		{
			LLScrollListItem* cur_item = *iter;

			if (cur_item->getSelected())
			{
				if (prev_item)
				{
					selectItem(prev_item, !extend_selection);
				}
				else
				{
					reportInvalidInput();
				}
				break;
			}

			prev_item = cur_item;
		}
	}

	if ((mCommitOnSelectionChange || mCommitOnKeyboardMovement))
	{
		commitIfChanged();
	}

	mSearchString.clear();
}


void LLScrollListCtrl::selectNextItem( BOOL extend_selection)
{
	if (!getFirstSelected())
	{
		selectFirstItem();
	}
	else
	{
		item_list::iterator iter;
		for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
		{
			LLScrollListItem* item = *iter;
			if (item->getSelected())
			{
				if (++iter != mItemList.end())
				{
					LLScrollListItem *next_item = *iter;
					if (next_item)
					{
						selectItem(next_item, !extend_selection);
					}
					else
					{
						reportInvalidInput();
					}
				}
				break;
			}
		}
	}

	if ((mCommitOnSelectionChange || mCommitOnKeyboardMovement))
	{
		onCommit();
	}

	mSearchString.clear();
}



void LLScrollListCtrl::deselectAllItems(BOOL no_commit_on_change)
{
	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item = *iter;
		deselectItem(item);
	}

	if (mCommitOnSelectionChange && !no_commit_on_change)
	{
		commitIfChanged();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// "Simple" interface: use this when you're creating a list that contains only unique strings, only
// one of which can be selected at a time.

LLScrollListItem* LLScrollListCtrl::addSimpleItem(const LLString& item_text, EAddPosition pos, BOOL enabled)
{
	LLScrollListItem* item = NULL;
	if (getItemCount() < mMaxItemCount)
	{
		// simple items have their LLSD data set to their label
		item = new LLScrollListItem( LLSD(item_text) );
		item->setEnabled(enabled);
		item->addColumn( item_text, gResMgr->getRes( LLFONT_SANSSERIF_SMALL ) );
		addItem( item, pos );
	}
	return item;
}


// Selects first enabled item of the given name.
// Returns false if item not found.
BOOL LLScrollListCtrl::selectSimpleItem(const LLString& label, BOOL case_sensitive)
{
	//RN: assume no empty items
	if (label.empty())
	{
		return FALSE;
	}

	LLString target_text = label;
	if (!case_sensitive)
	{
		LLString::toLower(target_text);
	}

	BOOL found = FALSE;

	item_list::iterator iter;
	S32 index = 0;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item = *iter;
		// Only select enabled items with matching names
		LLString item_text = item->getColumn(0)->getText();
		if (!case_sensitive)
		{
			LLString::toLower(item_text);
		}
		BOOL select = !found && item->getEnabled() && item_text == target_text;
		if (select)
		{
			selectItem(item);
		}
		found = found || select;
		index++;
	}

	if (mCommitOnSelectionChange)
	{
		commitIfChanged();
	}

	return found;
}


BOOL LLScrollListCtrl::selectSimpleItemByPrefix(const LLString& target, BOOL case_sensitive)
{
	return selectSimpleItemByPrefix(utf8str_to_wstring(target), case_sensitive);
}

// Selects first enabled item that has a name where the name's first part matched the target string.
// Returns false if item not found.
BOOL LLScrollListCtrl::selectSimpleItemByPrefix(const LLWString& target, BOOL case_sensitive)
{
	BOOL found = FALSE;

	LLWString target_trimmed( target );
	S32 target_len = target_trimmed.size();
	
	if( 0 == target_len )
	{
		// Is "" a valid choice?
		item_list::iterator iter;
		for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
		{
			LLScrollListItem* item = *iter;
			// Only select enabled items with matching names
			LLScrollListCell* cellp = item->getColumn(mSearchColumn);
			BOOL select = cellp ? item->getEnabled() && ('\0' == cellp->getText()[0]) : FALSE;
			if (select)
			{
				selectItem(item);
				found = TRUE;
				break;
			}
		}
	}
	else
	{
		if (!case_sensitive)
		{
			// do comparisons in lower case
			LLWString::toLower(target_trimmed);
		}

		for (item_list::iterator iter = mItemList.begin(); iter != mItemList.end(); iter++)
		{
			LLScrollListItem* item = *iter;

			// Only select enabled items with matching names
			LLScrollListCell* cellp = item->getColumn(mSearchColumn);
			if (!cellp)
			{
				continue;
			}
			LLWString item_label = utf8str_to_wstring(cellp->getText());
			if (!case_sensitive)
			{
				LLWString::toLower(item_label);
			}
			// remove extraneous whitespace from searchable label
			LLWString trimmed_label = item_label;
			LLWString::trim(trimmed_label);
			
			BOOL select = item->getEnabled() && trimmed_label.compare(0, target_trimmed.size(), target_trimmed) == 0;

			if (select)
			{
				// find offset of matching text (might have leading whitespace)
				S32 offset = item_label.find(target_trimmed);
				cellp->highlightText(offset, target_trimmed.size());
				selectItem(item);
				found = TRUE;
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

const LLString& LLScrollListCtrl::getSimpleSelectedItem(S32 column) const
{
	LLScrollListItem* item;

	item = getFirstSelected();
	if (item)
	{
		return item->getColumn(column)->getText();
	}

	return LLString::null;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// "StringUUID" interface: use this when you're creating a list that contains non-unique strings each of which
// has an associated, unique UUID, and only one of which can be selected at a time.

LLScrollListItem* LLScrollListCtrl::addStringUUIDItem(const LLString& item_text, const LLUUID& id, EAddPosition pos, BOOL enabled, S32 column_width)
{
	LLScrollListItem* item = NULL;
	if (getItemCount() < mMaxItemCount)
	{
		item = new LLScrollListItem( enabled, NULL, id );
		item->addColumn(item_text, gResMgr->getRes(LLFONT_SANSSERIF_SMALL), column_width);
		addItem( item, pos );
	}
	return item;
}

LLScrollListItem* LLScrollListCtrl::addSimpleItem(const LLString& item_text, LLSD sd, EAddPosition pos, BOOL enabled, S32 column_width)
{
	LLScrollListItem* item = NULL;
	if (getItemCount() < mMaxItemCount)
	{
		item = new LLScrollListItem( sd );
		item->setEnabled(enabled);
		item->addColumn(item_text, gResMgr->getRes(LLFONT_SANSSERIF_SMALL), column_width);
		addItem( item, pos );
	}
	return item;
}


// Select the line or lines that match this UUID
BOOL LLScrollListCtrl::selectByID( const LLUUID& id )
{
	return selectByValue( LLSD(id) );
}

BOOL LLScrollListCtrl::setSelectedByValue(LLSD value, BOOL selected)
{
	BOOL found = FALSE;

	if (selected && !mAllowMultipleSelection) deselectAllItems(TRUE);

	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item = *iter;
		if (item->getEnabled() && (item->getValue().asString() == value.asString()))
		{
			if (selected)
			{
				selectItem(item);
			}
			else
			{
				deselectItem(item);
			}
			found = TRUE;
			break;
		}
	}

	if (mCommitOnSelectionChange)
	{
		commitIfChanged();
	}

	return found;
}

BOOL LLScrollListCtrl::isSelected(LLSD value)
{
	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item = *iter;
		if (item->getValue().asString() == value.asString())
		{
			return item->getSelected();
		}
	}
	return FALSE;
}

LLUUID LLScrollListCtrl::getStringUUIDSelectedItem()
{
	LLScrollListItem* item = getFirstSelected();

	if (item)
	{
		return item->getUUID();
	}

	return LLUUID::null;
}

LLSD LLScrollListCtrl::getSimpleSelectedValue()
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

	S32 num_page_lines = mPageLines;

	LLRect item_rect;

	LLGLSUIDefault gls_ui;
	
	{
	
		S32 cur_x = x;
		S32 cur_y = y;
		
		mDrewSelected = FALSE;

		S32 line = 0;
		LLColor4 color;
		S32 max_columns = 0;

		item_list::iterator iter;
		for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
		{
			LLScrollListItem* item = *iter;
			
			item_rect.setOriginAndSize( 
				cur_x, 
				cur_y, 
				mScrollbar->getVisible() ? mItemListRect.getWidth() : mItemListRect.getWidth() + mScrollbar->getRect().getWidth(),
				mLineHeight );

			lldebugs << mItemListRect.getWidth() << llendl;

			if (item->getSelected())
			{
				mDrewSelected = TRUE;
			}

			max_columns = llmax(max_columns, item->getNumColumns());

			LLRect bg_rect = item_rect;
			// pad background rectangle to separate it from contents
			bg_rect.stretch(LIST_BORDER_PAD, 0);

			if( mScrollLines <= line && line < mScrollLines + num_page_lines )
			{
				if( item->getSelected() && mCanSelect)
				{
					// Draw background of selected item
					LLGLSNoTexture no_texture;
					glColor4fv(mBgSelectedColor.mV);
					gl_rect_2d( bg_rect );
                    
					color = mFgSelectedColor;
				}
				else if (mHighlightedItem == line && mCanSelect)
				{
					LLGLSNoTexture no_texture;
					glColor4fv(mHighlightedColor.mV);
					gl_rect_2d( bg_rect );
					color = (item->getEnabled() ? mFgUnselectedColor : mFgDisabledColor);
				}
				else 
				{
					color = (item->getEnabled() ? mFgUnselectedColor : mFgDisabledColor);
					if (mDrawStripes && (line%2 == 0) && (max_columns > 1))
					{
						LLGLSNoTexture no_texture;
						glColor4fv(mBgStripeColor.mV);
						gl_rect_2d( bg_rect );
					}
				}

				S32 line_x = cur_x;
				{
					S32 num_cols = item->getNumColumns();
					S32 cur_col = 0;
					S32 dynamic_width = 0;
					S32 dynamic_remainder = 0;
					if(mNumDynamicWidthColumns > 0)
					{
						dynamic_width = (mItemListRect.getWidth() - mTotalStaticColumnWidth) / mNumDynamicWidthColumns;
						dynamic_remainder = (mItemListRect.getWidth() - mTotalStaticColumnWidth) % mNumDynamicWidthColumns;
					}
					for (LLScrollListCell* cell = item->getColumn(0); cur_col < num_cols; cell = item->getColumn(++cur_col))
					{
						S32 cell_width = cell->getWidth();
						if(mColumnsIndexed.size() > (U32)cur_col && mColumnsIndexed[cur_col] && mColumnsIndexed[cur_col]->mDynamicWidth)
						{							
							cell_width = dynamic_width + (--dynamic_remainder ? 1 : 0);
							cell->setWidth(cell_width);
						}
						// Two ways a cell could be hidden
						if (cell_width < 0
							|| !cell->getVisible()) continue;
						LLUI::pushMatrix();
						LLUI::translate((F32) cur_x, (F32) cur_y, 0.0f);
						S32 space_left = mItemListRect.mRight - cur_x;
						LLColor4 highlight_color = LLColor4::white;
						F32 type_ahead_timeout = LLUI::sConfigGroup->getF32("TypeAheadTimeout");

						highlight_color.mV[VALPHA] = clamp_rescale(mSearchTimer.getElapsedTimeF32(), type_ahead_timeout * 0.7f, type_ahead_timeout, 0.4f, 0.f);
						cell->drawToWidth( space_left, color, highlight_color );
						LLUI::popMatrix();
						
						cur_x += cell_width + mColumnPadding;

					}
				} 
				cur_x = line_x;			
				cur_y -= mLineHeight;
			}
			line++;
		}
	}
}


void LLScrollListCtrl::draw()
{
	if( getVisible() )
	{
		LLRect background(0, mRect.getHeight(), mRect.getWidth(), 0);
		// Draw background
		if (mBackgroundVisible)
		{
			LLGLSNoTexture no_texture;
			glColor4fv( getEnabled() ? mBgWriteableColor.mV : mBgReadOnlyColor.mV );
			gl_rect_2d(background);
		}

		drawItems();

		if (mBorder)
		{
			mBorder->setKeyboardFocusHighlight(gFocusMgr.getKeyboardFocus() == this);
		}

		LLUICtrl::draw();
	}
}

void LLScrollListCtrl::setEnabled(BOOL enabled)
{
	mCanSelect = enabled;
	setTabStop(enabled);
	mScrollbar->setTabStop(!enabled && mScrollbar->getPageSize() < mScrollbar->getDocSize());
}

BOOL LLScrollListCtrl::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	BOOL handled = FALSE;
	// Pretend the mouse is over the scrollbar
	handled = mScrollbar->handleScrollWheel( 0, 0, clicks );
	return handled;
}

BOOL LLScrollListCtrl::selectItemAt(S32 x, S32 y, MASK mask)
{
	if (!mCanSelect) return FALSE;

	BOOL selection_changed = FALSE;

	LLScrollListItem* hit_item = hitItem(x, y);
	if( hit_item )
	{
		if( mAllowMultipleSelection )
		{
			if (mask & MASK_SHIFT)
			{
				if (mLastSelected == NULL)
				{
					selectItem(hit_item);
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
								mOnMaximumSelectCallback(mCallbackUserData);
							}
							break;
						}
						LLScrollListItem *item = *itor;
                        if (item == hit_item || item == lastSelected)
						{
							selectItem(item, FALSE);
							selecting = !selecting;
						}
						if (selecting)
						{
							selectItem(item, FALSE);
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
						selectItem(hit_item, FALSE);
					}
					else
					{
						if(mOnMaximumSelectCallback)
						{
							mOnMaximumSelectCallback(mCallbackUserData);
						}
					}
				}
			}
			else
			{
				deselectAllItems(TRUE);
				selectItem(hit_item);
			}
		}
		else
		{
			selectItem(hit_item);
		}

		hit_item->handleClick(x - mBorderThickness - LIST_BORDER_PAD, 
									1, mask);

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
		//deselectAllItems(TRUE);
	}

	return selection_changed;
}


BOOL LLScrollListCtrl::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLView::childrenHandleMouseDown(x, y, mask) != NULL;

	if( !handled )
	{
		// set keyboard focus first, in case click action wants to move focus elsewhere
		setFocus(TRUE);

		// clear selection changed flag so because user is starting a selection operation
		mSelectionChanged = FALSE;

		gFocusMgr.setMouseCapture(this);
		selectItemAt(x, y, mask);
	}

	return TRUE;
}

BOOL LLScrollListCtrl::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (hasMouseCapture())
	{
		if(mask == MASK_NONE)
		{
			selectItemAt(x, y, mask);
		}
	}

	if (hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
	}

	// always commit when mouse operation is completed inside list
	// this only needs to be done for lists that don't commit on selection change
	if (!mCommitOnSelectionChange && pointInView(x,y))
	{
		mSelectionChanged = FALSE;
		onCommit();
	}

	return LLUICtrl::handleMouseUp(x, y, mask);
}

BOOL LLScrollListCtrl::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	//BOOL handled = FALSE;
	if(getVisible())
	{
		// Offer the click to the children, even if we aren't enabled
		// so the scroll bars will work.
		if (NULL == LLView::childrenHandleDoubleClick(x, y, mask))
		{
			if( mCanSelect && mOnDoubleClickCallback )
			{
				mOnDoubleClickCallback( mCallbackUserData );
			}
		}
	}
	return TRUE;
}

LLScrollListItem* LLScrollListCtrl::hitItem( S32 x, S32 y )
{
	// Excludes disabled items.
	LLScrollListItem* hit_item = NULL;

	LLRect item_rect;
	item_rect.setLeftTopAndSize( 
		mItemListRect.mLeft,
		mItemListRect.mTop,
		mItemListRect.getWidth(),
		mLineHeight );

	int num_page_lines = mPageLines;

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


BOOL LLScrollListCtrl::handleHover(S32 x,S32 y,MASK mask)
{
	BOOL	handled = FALSE;

	if (hasMouseCapture())
	{
		if(mask == MASK_NONE)
		{
			selectItemAt(x, y, mask);
		}
	}
	else if (mCanSelect)
	{
		LLScrollListItem* item = hitItem(x, y);
		if (item)
		{
			highlightNthItem(getItemIndex(item));
		}
		else
		{
			highlightNthItem(-1);
		}
	}

	handled = LLUICtrl::handleHover( x, y, mask );

	//if( !handled )
	//{
	//	// Opaque
	//	getWindow()->setCursor(UI_CURSOR_ARROW);
	//	lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << llendl;		
	//	handled = TRUE;
	//}
	return handled;
}


BOOL LLScrollListCtrl::handleKeyHere(KEY key,MASK mask, BOOL called_from_parent )
{
	BOOL handled = FALSE;

	// not called from parent means we have keyboard focus or a child does
	if (mCanSelect && !called_from_parent) 
	{
		// Ignore capslock
		mask = mask;

		if (mask == MASK_NONE)
		{
			switch(key)
			{
			case KEY_UP:
				if (mAllowKeyboardMovement || hasFocus())
				{
					// commit implicit in call
					selectPrevItem(FALSE);
					scrollToShowSelected();
					handled = TRUE;
				}
				break;
			case KEY_DOWN:
				if (mAllowKeyboardMovement || hasFocus())
				{
					// commit implicit in call
					selectNextItem(FALSE);
					scrollToShowSelected();
					handled = TRUE;
				}
				break;
			case KEY_PAGE_UP:
				if (mAllowKeyboardMovement || hasFocus())
				{
					selectNthItem(getFirstSelectedIndex() - (mScrollbar->getPageSize() - 1));
					scrollToShowSelected();
					if (mCommitOnKeyboardMovement
						&& !mCommitOnSelectionChange) 
					{
						onCommit();
					}
					handled = TRUE;
				}
				break;
			case KEY_PAGE_DOWN:
				if (mAllowKeyboardMovement || hasFocus())
				{
					selectNthItem(getFirstSelectedIndex() + (mScrollbar->getPageSize() - 1));
					scrollToShowSelected();
					if (mCommitOnKeyboardMovement
						&& !mCommitOnSelectionChange) 
					{
						onCommit();
					}
					handled = TRUE;
				}
				break;
			case KEY_HOME:
				if (mAllowKeyboardMovement || hasFocus())
				{
					selectFirstItem();
					scrollToShowSelected();
					if (mCommitOnKeyboardMovement
						&& !mCommitOnSelectionChange) 
					{
						onCommit();
					}
					handled = TRUE;
				}
				break;
			case KEY_END:
				if (mAllowKeyboardMovement || hasFocus())
				{
					selectNthItem(getItemCount() - 1);
					scrollToShowSelected();
					if (mCommitOnKeyboardMovement
						&& !mCommitOnSelectionChange) 
					{
						onCommit();
					}
					handled = TRUE;
				}
				break;
			case KEY_RETURN:
				// JC - Special case: Only claim to have handled it
				// if we're the special non-commit-on-move
				// type. AND we are visible
			  	if (!mCommitOnKeyboardMovement && mask == MASK_NONE && getVisible())
				{
					onCommit();
					mSearchString.clear();
					handled = TRUE;
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
						LLScrollListCell* cellp = getFirstSelected()->getColumn(mSearchColumn);
						if (cellp)
						{
							cellp->highlightText(0, 0);
						}
					}
				}
				else if (selectSimpleItemByPrefix(wstring_to_utf8str(mSearchString), FALSE))
				{
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

BOOL LLScrollListCtrl::handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent)
{
	if ((uni_char < 0x20) || (uni_char == 0x7F)) // Control character or DEL
	{
		return FALSE;
	}

	// perform incremental search based on keyboard input
	if (mSearchTimer.getElapsedTimeF32() > LLUI::sConfigGroup->getF32("TypeAheadTimeout"))
	{
		mSearchString.clear();
	}

	// type ahead search is case insensitive
	uni_char = LLStringOps::toLower((llwchar)uni_char);

	if (selectSimpleItemByPrefix(wstring_to_utf8str(mSearchString + (llwchar)uni_char), FALSE))
	{
		// update search string only on successful match
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

			LLScrollListCell* cellp = item->getColumn(mSearchColumn);
			if (cellp)
			{
				// Only select enabled items with matching first characters
				LLWString item_label = utf8str_to_wstring(cellp->getText());
				if (item->getEnabled() && LLStringOps::toLower(item_label[0]) == uni_char)
				{
					selectItem(item);
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

	// make sure selected item is on screen
	scrollToShowSelected();
	return TRUE;
}


void LLScrollListCtrl::reportInvalidInput()
{
	make_ui_sound("UISndBadKeystroke");
}

BOOL LLScrollListCtrl::isRepeatedChars(const LLWString& string) const
{
	if (string.empty())
	{
		return FALSE;
	}

	llwchar first_char = string[0];

	for (U32 i = 0; i < string.size(); i++)
	{
		if (string[i] != first_char)
		{
			return FALSE;
		}
	}

	return TRUE;
}

void LLScrollListCtrl::selectItem(LLScrollListItem* itemp, BOOL select_single_item)
{
	if (!itemp) return;

	if (!itemp->getSelected())
	{
		if (mLastSelected)
		{
			LLScrollListCell* cellp = mLastSelected->getColumn(mSearchColumn);
			if (cellp)
			{
				cellp->highlightText(0, 0);
			}
		}
		if (select_single_item)
		{
			deselectAllItems(TRUE);
		}
		itemp->setSelected(TRUE);
		mLastSelected = itemp;
		mSelectionChanged = TRUE;
	}
}

void LLScrollListCtrl::deselectItem(LLScrollListItem* itemp)
{
	if (!itemp) return;

	if (itemp->getSelected())
	{
		if (mLastSelected == itemp)
		{
			mLastSelected = NULL;
		}

		itemp->setSelected(FALSE);
		LLScrollListCell* cellp = itemp->getColumn(mSearchColumn);
		if (cellp)
		{
			cellp->highlightText(0, 0);
		}
		mSelectionChanged = TRUE;
	}
}

void LLScrollListCtrl::commitIfChanged()
{
	if (mSelectionChanged && !hasMouseCapture())
	{
		mSelectionChanged = FALSE;
		onCommit();
	}
}

// Called by scrollbar
//static
void LLScrollListCtrl::onScrollChange( S32 new_pos, LLScrollbar* scrollbar, void* userdata )
{
	LLScrollListCtrl* self = (LLScrollListCtrl*) userdata;
	self->mScrollLines = new_pos;
}


// First column is column 0
void  LLScrollListCtrl::sortByColumn(U32 column, BOOL ascending)
{
	if (mSortColumn != column)
	{
		mSortColumn = column;
		std::sort(mItemList.begin(), mItemList.end(), SortScrollListItem(mSortColumn, mSortAscending));
	}

	// just reverse the list if changing sort order
	if(mSortAscending != ascending)
	{
		std::reverse(mItemList.begin(), mItemList.end());
		mSortAscending = ascending;
	}
}

void LLScrollListCtrl::sortByColumn(LLString name, BOOL ascending)
{
	if (name.empty())
	{
		sortByColumn(mSortColumn, mSortAscending);
		return;
	}

	std::map<LLString, LLScrollListColumn>::iterator itor = mColumns.find(name);
	if (itor != mColumns.end())
	{
		sortByColumn((*itor).second.mIndex, ascending);
	}
}

S32 LLScrollListCtrl::getScrollPos()
{
	return mScrollbar->getDocPos();
}


void LLScrollListCtrl::setScrollPos( S32 pos )
{
	mScrollbar->setDocPos( pos );

	onScrollChange(mScrollbar->getDocPos(), mScrollbar, this);
}


void LLScrollListCtrl::scrollToShowSelected()
{
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
	S32 highest = mScrollLines + mPageLines;

	if (index < lowest)
	{
		// need to scroll to show item
		setScrollPos(index);
	}
	else if (highest <= index)
	{
		setScrollPos(index - mPageLines + 1);
	}
}

// virtual
LLXMLNodePtr LLScrollListCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	// Attributes

	node->createChild("multi_select", TRUE)->setBoolValue(mAllowMultipleSelection);

	node->createChild("draw_border", TRUE)->setBoolValue((mBorder != NULL));

	node->createChild("draw_heading", TRUE)->setBoolValue(mDisplayColumnHeaders);

	node->createChild("background_visible", TRUE)->setBoolValue(mBackgroundVisible);

	node->createChild("draw_stripes", TRUE)->setBoolValue(mDrawStripes);

	node->createChild("column_padding", TRUE)->setIntValue(mColumnPadding);

	addColorXML(node, mBgWriteableColor, "bg_writeable_color", "ScrollBgWriteableColor");
	addColorXML(node, mBgReadOnlyColor, "bg_read_only_color", "ScrollBgReadOnlyColor");
	addColorXML(node, mBgSelectedColor, "bg_selected_color", "ScrollSelectedBGColor");
	addColorXML(node, mBgStripeColor, "bg_stripe_color", "ScrollBGStripeColor");
	addColorXML(node, mFgSelectedColor, "fg_selected_color", "ScrollSelectedFGColor");
	addColorXML(node, mFgUnselectedColor, "fg_unselected_color", "ScrollUnselectedColor");
	addColorXML(node, mFgDisabledColor, "fg_disable_color", "ScrollDisabledColor");
	addColorXML(node, mHighlightedColor, "highlighted_color", "ScrollHighlightedColor");

	// Contents

	std::map<LLString, LLScrollListColumn>::const_iterator itor;
	std::vector<const LLScrollListColumn*> sorted_list;
	sorted_list.resize(mColumns.size());
	for (itor = mColumns.begin(); itor != mColumns.end(); ++itor)
	{
		sorted_list[itor->second.mIndex] = &itor->second;
	}

	std::vector<const LLScrollListColumn*>::iterator itor2;
	for (itor2 = sorted_list.begin(); itor2 != sorted_list.end(); ++itor2)
	{
		LLXMLNodePtr child_node = node->createChild("column", FALSE);
		const LLScrollListColumn *column = *itor2;

		child_node->createChild("name", TRUE)->setStringValue(column->mName);
		child_node->createChild("label", TRUE)->setStringValue(column->mLabel);
		child_node->createChild("width", TRUE)->setIntValue(column->mWidth);
	}

	return node;
}

void LLScrollListCtrl::setScrollListParameters(LLXMLNodePtr node)
{
	// James: This is not a good way to do colors. We need a central "UI style"
	// manager that sets the colors for ALL scroll lists, buttons, etc.

	LLColor4 color;
	if(node->hasAttribute("fg_unselected_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"fg_unselected_color", color);
		setFgUnselectedColor(color);
	}
	if(node->hasAttribute("fg_selected_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"fg_selected_color", color);
		setFgSelectedColor(color);
	}
	if(node->hasAttribute("bg_selected_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"bg_selected_color", color);
		setBgSelectedColor(color);
	}
	if(node->hasAttribute("fg_disable_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"fg_disable_color", color);
		setFgDisableColor(color);
	}
	if(node->hasAttribute("bg_writeable_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"bg_writeable_color", color);
		setBgWriteableColor(color);
	}
	if(node->hasAttribute("bg_read_only_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"bg_read_only_color", color);
		setReadOnlyBgColor(color);
	}
	if (LLUICtrlFactory::getAttributeColor(node,"bg_stripe_color", color))
	{
		setBgStripeColor(color);
	}
	if (LLUICtrlFactory::getAttributeColor(node,"highlighted_color", color))
	{
		setHighlightedColor(color);
	}

	if(node->hasAttribute("background_visible"))
	{
		BOOL background_visible;
		node->getAttributeBOOL("background_visible", background_visible);
		setBackgroundVisible(background_visible);
	}

	if(node->hasAttribute("draw_stripes"))
	{
		BOOL draw_stripes;
		node->getAttributeBOOL("draw_stripes", draw_stripes);
		setDrawStripes(draw_stripes);
	}
	
	if(node->hasAttribute("column_padding"))
	{
		S32 column_padding;
		node->getAttributeS32("column_padding", column_padding);
		setColumnPadding(column_padding);
	}
}

// static
LLView* LLScrollListCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("scroll_list");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	BOOL multi_select = FALSE;
	node->getAttributeBOOL("multi_select", multi_select);

	BOOL draw_border = TRUE;
	node->getAttributeBOOL("draw_border", draw_border);

	BOOL draw_heading = FALSE;
	node->getAttributeBOOL("draw_heading", draw_heading);

	BOOL collapse_empty_columns = FALSE;
	node->getAttributeBOOL("collapse_empty_columns", collapse_empty_columns);

	S32 search_column = 0;
	node->getAttributeS32("search_column", search_column);

	LLUICtrlCallback callback = NULL;

	LLScrollListCtrl* scroll_list = new LLScrollListCtrl(
		name,
		rect,
		callback,
		NULL,
		multi_select,
		draw_border);

	scroll_list->setDisplayHeading(draw_heading);
	if (node->hasAttribute("heading_height"))
	{
		S32 heading_height;
		node->getAttributeS32("heading_height", heading_height);
		scroll_list->setHeadingHeight(heading_height);
	}
	scroll_list->setCollapseEmptyColumns(collapse_empty_columns);

	scroll_list->setScrollListParameters(node);

	scroll_list->initFromXML(node, parent);

	scroll_list->setSearchColumn(search_column);

	LLSD columns;
	S32 index = 0;
	LLXMLNodePtr child;
	S32 total_static = 0, num_dynamic = 0;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("column"))
		{
			LLString labelname("");
			child->getAttributeString("label", labelname);

			LLString columnname(labelname);
			child->getAttributeString("name", columnname);

			LLString sortname(columnname);
			child->getAttributeString("sort", sortname);

			LLString imagename;
			child->getAttributeString("image", imagename);

			BOOL columndynamicwidth = FALSE;
			child->getAttributeBOOL("dynamicwidth", columndynamicwidth);

			S32 columnwidth = -1;
			child->getAttributeS32("width", columnwidth);	

			if(!columndynamicwidth) total_static += columnwidth;
			else ++num_dynamic;

			F32 columnrelwidth = 0.f;
			child->getAttributeF32("relwidth", columnrelwidth);

			LLFontGL::HAlign h_align = LLFontGL::LEFT;
			h_align = LLView::selectFontHAlign(child);

			columns[index]["name"] = columnname;
			columns[index]["sort"] = sortname;
			columns[index]["image"] = imagename;
			columns[index]["label"] = labelname;
			columns[index]["width"] = columnwidth;
			columns[index]["relwidth"] = columnrelwidth;
			columns[index]["dynamicwidth"] = columndynamicwidth;
			columns[index]["halign"] = (S32)h_align;
			index++;
		}
	}
	scroll_list->setNumDynamicColumns(num_dynamic);
	scroll_list->setTotalStaticColumnWidth(total_static);
	scroll_list->setColumnHeadings(columns);

	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("row"))
		{
			LLUUID id;
			child->getAttributeUUID("id", id);

			LLSD row;

			row["id"] = id;

			S32 column_idx = 0;
			LLXMLNodePtr row_child;
			for (row_child = child->getFirstChild(); row_child.notNull(); row_child = row_child->getNextSibling())
			{
				if (row_child->hasName("column"))
				{
					LLString value = row_child->getTextContents();

					LLString columnname("");
					row_child->getAttributeString("name", columnname);

					LLString font("");
					row_child->getAttributeString("font", font);

					LLString font_style("");
					row_child->getAttributeString("font-style", font_style);

					row["columns"][column_idx]["column"] = columnname;
					row["columns"][column_idx]["value"] = value;
					row["columns"][column_idx]["font"] = font;
					row["columns"][column_idx]["font-style"] = font_style;
					column_idx++;
				}
			}
			scroll_list->addElement(row);
		}
	}

	LLString contents = node->getTextContents();
	if (!contents.empty())
	{
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("\t\n");
		tokenizer tokens(contents, sep);
		tokenizer::iterator token_iter = tokens.begin();

		while(token_iter != tokens.end())
		{
			const char* line = token_iter->c_str();
			scroll_list->addSimpleItem(line);
			++token_iter;
		}
	}
	
	return scroll_list;
}

// LLEditMenuHandler functions

// virtual
void	LLScrollListCtrl::copy()
{
	LLString buffer;

	std::vector<LLScrollListItem*> items = getAllSelected();
	std::vector<LLScrollListItem*>::iterator itor;
	for (itor = items.begin(); itor != items.end(); ++itor)
	{
		buffer += (*itor)->getContentsCSV() + "\n";
	}
	gClipboard.copyFromSubstring(utf8str_to_wstring(buffer), 0, buffer.length());
}

// virtual
BOOL	LLScrollListCtrl::canCopy()
{
	return (getFirstSelected() != NULL);
}

// virtual
void	LLScrollListCtrl::cut()
{
	copy();
	doDelete();
}

// virtual
BOOL	LLScrollListCtrl::canCut()
{
	return canCopy() && canDoDelete();
}

// virtual
void	LLScrollListCtrl::doDelete()
{
	// Not yet implemented
}

// virtual
BOOL	LLScrollListCtrl::canDoDelete()
{
	// Not yet implemented
	return FALSE;
}

// virtual
void	LLScrollListCtrl::selectAll()
{
	// Deselects all other items
	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem *itemp = *iter;
		if( itemp->getEnabled() )
		{
			selectItem(itemp, FALSE);
		}
	}

	if (mCommitOnSelectionChange)
	{
		commitIfChanged();
	}
}

// virtual
BOOL	LLScrollListCtrl::canSelectAll()
{
	return getCanSelect() && mAllowMultipleSelection && !(mMaxSelectable > 0 && mItemList.size() > mMaxSelectable);
}

// virtual
void	LLScrollListCtrl::deselect()
{
	deselectAllItems();
}

// virtual
BOOL	LLScrollListCtrl::canDeselect()
{
	return getCanSelect();
}

void LLScrollListCtrl::addColumn(const LLSD& column, EAddPosition pos)
{
	LLString name = column["name"].asString();
	if (mColumns.empty())
	{
		mDefaultColumn = 0;
	}
	if (mColumns.find(name) == mColumns.end())
	{
		// Add column
		mColumns[name] = LLScrollListColumn(column);
		LLScrollListColumn* new_column = &mColumns[name];
		new_column->mParentCtrl = this;
		new_column->mIndex = mColumns.size()-1;

		// Add button
		if (new_column->mWidth > 0 || new_column->mRelWidth > 0 || new_column->mDynamicWidth)
		{
			if (new_column->mRelWidth >= 0)
			{
				new_column->mWidth = (S32)llround(new_column->mRelWidth*mItemListRect.getWidth());
			}
			else if(new_column->mDynamicWidth)
			{
				new_column->mWidth = (mItemListRect.getWidth() - mTotalStaticColumnWidth) / mNumDynamicWidthColumns;
			}
			S32 top = mItemListRect.mTop;
			S32 left = mItemListRect.mLeft;
			{
				std::map<LLString, LLScrollListColumn>::iterator itor;
				for (itor = mColumns.begin(); itor != mColumns.end(); ++itor)
				{
					if (itor->second.mIndex < new_column->mIndex &&
						itor->second.mWidth > 0)
					{
						left += itor->second.mWidth + mColumnPadding;
					}
				}
			}
			LLString button_name = "btn_" + name;
			S32 right = left+new_column->mWidth;
			if (new_column->mIndex != (S32)mColumns.size()-1)
			{
				right += mColumnPadding;
			}
			LLRect temp_rect = LLRect(left,top+mHeadingHeight,right,top);
			new_column->mHeader = new LLColumnHeader(button_name, temp_rect, new_column); 
			if(column["image"].asString() != "")
			{
				//new_column->mHeader->setScaleImage(false);
				new_column->mHeader->setImage(column["image"].asString());				
			}
			else
			{
				new_column->mHeader->setLabel(new_column->mLabel);
				//new_column->mHeader->setLabel(new_column->mLabel);
			}
			//RN: although it might be useful to change sort order with the keyboard,
			// mixing tab stops on child items along with the parent item is not supported yet
			new_column->mHeader->setTabStop(FALSE);
			addChild(new_column->mHeader);
			new_column->mHeader->setVisible(mDisplayColumnHeaders);


			// Move scroll to front
			removeChild(mScrollbar);
			addChild(mScrollbar);
		
		}
	}
	updateColumns();
}

// static
void LLScrollListCtrl::onClickColumn(void *userdata)
{
	LLScrollListColumn *info = (LLScrollListColumn*)userdata;
	if (!info) return;

	LLScrollListCtrl *parent = info->mParentCtrl;
	if (!parent) return;

	U32 column_index = info->mIndex;

	LLScrollListColumn* column = parent->mColumnsIndexed[info->mIndex];
	if (column->mSortingColumn != column->mName)
	{
		if (parent->mColumns.find(column->mSortingColumn) != parent->mColumns.end())
		{
			LLScrollListColumn& info_redir = parent->mColumns[column->mSortingColumn];
			column_index = info_redir.mIndex;
		}
	}

	bool ascending = true;
	if (column_index == parent->mSortColumn)
	{
		ascending = !parent->mSortAscending;
	}

	parent->sortByColumn(column_index, ascending);

	if (parent->mOnSortChangedCallback)
	{
		parent->mOnSortChangedCallback(parent->getCallbackUserData());
	}
}

std::string LLScrollListCtrl::getSortColumnName()
{
	LLScrollListColumn* column = mSortColumn >= 0 ? mColumnsIndexed[mSortColumn] : NULL;

	if (column) return column->mName;
	else return "";
}

void LLScrollListCtrl::clearColumns()
{
	std::map<LLString, LLScrollListColumn>::iterator itor;
	for (itor = mColumns.begin(); itor != mColumns.end(); ++itor)
	{
		LLColumnHeader *header = itor->second.mHeader;
		if (header)
		{
			removeChild(header);
			delete header;
		}
	}
	mColumns.clear();
}

void LLScrollListCtrl::setColumnLabel(const LLString& column, const LLString& label)
{
	std::map<LLString, LLScrollListColumn>::iterator itor = mColumns.find(column);
	if (itor != mColumns.end())
	{
		itor->second.mLabel = label;
		if (itor->second.mHeader)
		{
			itor->second.mHeader->setLabel(label);
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

void LLScrollListCtrl::setColumnHeadings(LLSD headings)
{
	mColumns.clear();
	LLSD::array_const_iterator itor;
	for (itor = headings.beginArray(); itor != headings.endArray(); ++itor)
	{
		addColumn(*itor);
	}
}

LLScrollListItem* LLScrollListCtrl::addElement(const LLSD& value, EAddPosition pos, void* userdata)
{
	// ID
	LLSD id = value["id"];

	LLScrollListItem *new_item = new LLScrollListItem(id, userdata);
	if (value.has("enabled"))
	{
		new_item->setEnabled( value["enabled"].asBoolean() );
	}

	new_item->setNumColumns(mColumns.size());

	// Add any columns we don't already have
	LLSD columns = value["columns"];
	LLSD::array_const_iterator itor;
	for (itor = columns.beginArray(); itor != columns.endArray(); ++itor)
	{
		LLString column = (*itor)["column"].asString();

		if (mColumns.size() == 0)
		{
			mDefaultColumn = 0;
		}
		std::map<LLString, LLScrollListColumn>::iterator column_itor = mColumns.find(column);
		if (column_itor == mColumns.end())
		{
			LLSD new_column;
			new_column["name"] = column;
			new_column["label"] = column;
			new_column["width"] = 0;
			addColumn(new_column);
			column_itor = mColumns.find(column);
			new_item->setNumColumns(mColumns.size());
		}

		S32 index = column_itor->second.mIndex;
		S32 width = column_itor->second.mWidth;
		LLFontGL::HAlign font_alignment = column_itor->second.mFontAlignment;

		LLSD value = (*itor)["value"];
		LLString fontname = (*itor)["font"].asString();
		LLString fontstyle = (*itor)["font-style"].asString();
		LLString type = (*itor)["type"].asString();

		const LLFontGL *font = gResMgr->getRes(fontname);
		if (!font)
		{
			font = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );
		}
		U8 font_style = LLFontGL::getStyleFromString(fontstyle);

		if (type == "icon")
		{
			LLUUID image_id = value.asUUID();
			LLImageGL* icon = LLUI::sImageProvider->getUIImageByID(image_id);
			new_item->setColumn(index, new LLScrollListIcon(icon, width, image_id));
		}
		else if (type == "checkbox")
		{
			LLCheckBoxCtrl* ctrl = new LLCheckBoxCtrl(value.asString(),
														LLRect(0, 0, width, width), "label");
			new_item->setColumn(index, new LLScrollListCheck(ctrl,width));
		}
		else
		{
			new_item->setColumn(index, new LLScrollListText(value.asString(), font, width, font_style, font_alignment));
			if (column_itor->second.mHeader && !value.asString().empty())
			{
				column_itor->second.mHeader->setHasResizableElement(TRUE);
			}
		}
	}

	S32 num_columns = mColumns.size();
	for (S32 column = 0; column < num_columns; ++column)
	{
		if (new_item->getColumn(column) == NULL)
		{
			LLScrollListColumn* column_ptr = mColumnsIndexed[column];
			new_item->setColumn(column, new LLScrollListText("", gResMgr->getRes( LLFONT_SANSSERIF_SMALL ), column_ptr->mWidth, LLFontGL::NORMAL));
		}
	}

	addItem(new_item, pos);

	return new_item;
}

LLScrollListItem* LLScrollListCtrl::addSimpleElement(const LLString& value, EAddPosition pos, const LLSD& id)
{
	LLSD entry_id = id;

	if (id.isUndefined())
	{
		entry_id = value;
	}

	LLScrollListItem *new_item = new LLScrollListItem(entry_id);

	const LLFontGL *font = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );

	new_item->addColumn(value, font, getRect().getWidth());

	addItem(new_item, pos);
	return new_item;
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

BOOL LLScrollListCtrl::operateOnSelection(EOperation op)
{
	if (op == OP_DELETE)
	{
		deleteSelectedItems();
		return TRUE;
	}
	else if (op == OP_DESELECT)
	{
		deselectAllItems();
	}
	return FALSE;
}

BOOL LLScrollListCtrl::operateOnAll(EOperation op)
{
	if (op == OP_DELETE)
	{
		clearRows();
		return TRUE;
	}
	else if (op == OP_DESELECT)
	{
		deselectAllItems();
	}
	else if (op == OP_SELECT)
	{
		selectAll();
	}
	return FALSE;
}
//virtual 
void LLScrollListCtrl::setFocus(BOOL b)
{
	mSearchString.clear();
	// for tabbing into pristine scroll lists (Finder)
	if (!getFirstSelected())
	{
		selectFirstItem();
		//onCommit(); // SJB: selectFirstItem() will call onCommit() if appropriate
	}
	LLUICtrl::setFocus(b);
}

//virtual
void LLScrollListCtrl::onFocusReceived()
{
	// forget latent selection changes when getting focus
	mSelectionChanged = FALSE;
}

//virtual 
void LLScrollListCtrl::onFocusLost()
{
	if (mIsPopup)
	{
		if (getParent())
		{
			getParent()->onFocusLost();
		}
	}
	if (hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
	}
	LLUICtrl::onFocusLost();
}

LLColumnHeader::LLColumnHeader(const LLString& label, const LLRect &rect, LLScrollListColumn* column, const LLFontGL* fontp) : 
	LLComboBox(label, rect, label, NULL, NULL), 
	mColumn(column),
	mOrigLabel(label),
	mShowSortOptions(FALSE),
	mHasResizableElement(FALSE)
{
	mListPosition = LLComboBox::ABOVE;
	setCommitCallback(onSelectSort);
	setCallbackUserData(this);
	mButton->setTabStop(FALSE);
	// require at least two frames between mouse down and mouse up event to capture intentional "hold" not just bad framerate
	mButton->setHeldDownDelay(LLUI::sConfigGroup->getF32("ColumnHeaderDropDownDelay"), 2);
	mButton->setHeldDownCallback(onHeldDown);
	mButton->setClickedCallback(onClick);
	mButton->setMouseDownCallback(onMouseDown);

	mButton->setCallbackUserData(this);

	mAscendingText = "[LOW]...[HIGH](Ascending)";
	mDescendingText = "[HIGH]...[LOW](Descending)";

	LLSD row;
	row["columns"][0]["column"] = "label";
	row["columns"][0]["value"] = mAscendingText.getString();
	row["columns"][0]["font"] = "SANSSERIF_SMALL";
	row["columns"][0]["width"] = 80;
	
	row["columns"][1]["column"] = "arrow";
	row["columns"][1]["type"] = "icon";
	row["columns"][1]["value"] = LLUI::sAssetsGroup->getString("up_arrow.tga");
	row["columns"][1]["width"] = 20;

	mList->addElement(row);

	row["columns"][0]["column"] = "label";
	row["columns"][0]["type"] = "text";
	row["columns"][0]["value"] = mDescendingText.getString();
	row["columns"][0]["font"] = "SANSSERIF_SMALL";
	row["columns"][0]["width"] = 80;
	
	row["columns"][1]["column"] = "arrow";
	row["columns"][1]["type"] = "icon";
	row["columns"][1]["value"] = LLUI::sAssetsGroup->getString("down_arrow.tga");
	row["columns"][1]["width"] = 20;

	mList->addElement(row);

	mList->reshape(llmax(mList->getRect().getWidth(), 110, mRect.getWidth()), mList->getRect().getHeight());

	// resize handles on left and right
	const S32 RESIZE_BAR_THICKNESS = 3;
	mResizeBar = new LLResizeBar( 
		"resizebar",
		LLRect( mRect.getWidth() - RESIZE_BAR_THICKNESS, mRect.getHeight(), mRect.getWidth(), 0), 
		MIN_COLUMN_WIDTH, mRect.getHeight(), LLResizeBar::RIGHT );
	addChild(mResizeBar);

	mResizeBar->setEnabled(FALSE);
}

LLColumnHeader::~LLColumnHeader()
{
}

void LLColumnHeader::draw()
{
	if( getVisible() )
	{
		mDrawArrow = !mColumn->mLabel.empty() && mColumn->mParentCtrl->getSortColumnName() == mColumn->mSortingColumn;

		BOOL is_ascending = mColumn->mParentCtrl->getSortAscending();
		mArrowImage = is_ascending ? LLUI::sImageProvider->getUIImageByID(LLUUID(LLUI::sAssetsGroup->getString("up_arrow.tga")))
			: LLUI::sImageProvider->getUIImageByID(LLUUID(LLUI::sAssetsGroup->getString("down_arrow.tga")));

		//BOOL clip = mRect.mRight > mColumn->mParentCtrl->getItemListRect().getWidth();
		//LLGLEnable scissor_test(clip ? GL_SCISSOR_TEST : GL_FALSE);

		//LLRect column_header_local_rect(-mRect.mLeft, mRect.getHeight(), mColumn->mParentCtrl->getItemListRect().getWidth() - mRect.mLeft, 0);
		//LLUI::setScissorRegionLocal(column_header_local_rect);

		// Draw children
		LLComboBox::draw();

		if (mList->getVisible())
		{
			// sync sort order with list selection every frame
			mColumn->mParentCtrl->sortByColumn(mColumn->mSortingColumn, getCurrentIndex() == 0);
		}

	}
}

BOOL LLColumnHeader::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (canResize() && mResizeBar->getRect().pointInRect(x, y))
	{
		// reshape column to max content width
		LLRect column_rect = getRect();
		column_rect.mRight = column_rect.mLeft + mColumn->mMaxContentWidth;
		userSetShape(column_rect);
	}
	else
	{
		onClick(this);
	}
	return TRUE;
}

void LLColumnHeader::setImage(const LLString &image_name)
{
	if (mButton)
	{
		mButton->setImageSelected(image_name);
		mButton->setImageUnselected(image_name);
	}
}

//static
void LLColumnHeader::onClick(void* user_data)
{
	LLColumnHeader* headerp = (LLColumnHeader*)user_data;
	if (!headerp) return;

	LLScrollListColumn* column = headerp->mColumn;
	if (!column) return;

	if (headerp->mList->getVisible())
	{
		headerp->hideList();
	}

	LLScrollListCtrl::onClickColumn(column);

	// propage new sort order to sort order list
	headerp->mList->selectNthItem(column->mParentCtrl->getSortAscending() ? 0 : 1);
}

//static
void LLColumnHeader::onMouseDown(void* user_data)
{
	// for now, do nothing but block the normal showList() behavior
	return;
}

//static
void LLColumnHeader::onHeldDown(void* user_data)
{
	LLColumnHeader* headerp = (LLColumnHeader*)user_data;
	headerp->showList();
}

void LLColumnHeader::showList()
{
	if (mShowSortOptions)
	{
		//LLSD item_val = mColumn->mParentCtrl->getFirstData()->getValue();
		mOrigLabel = mButton->getLabelSelected();

		// move sort column over to this column and do initial sort
		mColumn->mParentCtrl->sortByColumn(mColumn->mSortingColumn, mColumn->mParentCtrl->getSortAscending());

		LLString low_item_text;
		LLString high_item_text;

		LLScrollListItem* itemp = mColumn->mParentCtrl->getFirstData();
		if (itemp)
		{
			LLScrollListCell* cell = itemp->getColumn(mColumn->mIndex);
			if (cell && cell->isText())
			{
				if (mColumn->mParentCtrl->getSortAscending())
				{
					low_item_text = cell->getText();
				}
				else
				{
					high_item_text = cell->getText();
				}
			}
		}

		itemp = mColumn->mParentCtrl->getLastData();
		if (itemp)
		{
			LLScrollListCell* cell = itemp->getColumn(mColumn->mIndex);
			if (cell && cell->isText())
			{
				if (mColumn->mParentCtrl->getSortAscending())
				{
					high_item_text = cell->getText();
				}
				else
				{
					low_item_text = cell->getText();
				}
			}
		}

		LLString::truncate(low_item_text, 3);
		LLString::truncate(high_item_text, 3);

		LLString ascending_string;
		LLString descending_string;

		if (low_item_text.empty() || high_item_text.empty())
		{
			ascending_string = "Ascending";
			descending_string = "Descending";
		}
		else
		{
			mAscendingText.setArg("[LOW]", low_item_text);
			mAscendingText.setArg("[HIGH]", high_item_text);
			mDescendingText.setArg("[LOW]", low_item_text);
			mDescendingText.setArg("[HIGH]", high_item_text);
			ascending_string = mAscendingText.getString();
			descending_string = mDescendingText.getString();
		}

		S32 text_width = LLFontGL::sSansSerifSmall->getWidth(ascending_string);
		text_width = llmax(text_width, LLFontGL::sSansSerifSmall->getWidth(descending_string)) + 10;
		text_width = llmax(text_width, mRect.getWidth() - 30);

		mList->getColumn(0)->mWidth = text_width;
		((LLScrollListText*)mList->getFirstData()->getColumn(0))->setText(ascending_string);
		((LLScrollListText*)mList->getLastData()->getColumn(0))->setText(descending_string);

		mList->reshape(llmax(text_width + 30, 110, mRect.getWidth()), mList->getRect().getHeight());

		LLComboBox::showList();
	}
}

//static 
void LLColumnHeader::onSelectSort(LLUICtrl* ctrl, void* user_data)
{
	LLColumnHeader* headerp = (LLColumnHeader*)user_data;
	if (!headerp) return;

	LLScrollListColumn* column = headerp->mColumn;
	if (!column) return;
	LLScrollListCtrl *parent = column->mParentCtrl;
	if (!parent) return;

	if (headerp->getCurrentIndex() == 0)
	{
		// ascending
		parent->sortByColumn(column->mSortingColumn, TRUE);
	}
	else
	{
		// descending
		parent->sortByColumn(column->mSortingColumn, FALSE);
	}

	// restore original column header
	headerp->setLabel(headerp->mOrigLabel);
}

LLView*	LLColumnHeader::findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding)
{
	// this logic assumes dragging on right
	llassert(snap_edge == SNAP_RIGHT);

	// use higher snap threshold for column headers
	threshold = llmin(threshold, 15);

	LLRect snap_rect = getSnapRect();

	S32 snap_delta = mColumn->mMaxContentWidth - snap_rect.getWidth();

	// x coord growing means column growing, so same signs mean we're going in right direction
	if (llabs(snap_delta) <= threshold && mouse_dir.mX * snap_delta > 0 ) 
	{
		new_edge_val = snap_rect.mRight + snap_delta;
	}
	else 
	{
		LLScrollListColumn* next_column = mColumn->mParentCtrl->getColumn(mColumn->mIndex + 1);
		while (next_column)
		{
			if (next_column->mHeader)
			{
				snap_delta = (next_column->mHeader->getSnapRect().mRight - next_column->mMaxContentWidth) - snap_rect.mRight;
				if (llabs(snap_delta) <= threshold && mouse_dir.mX * snap_delta > 0 ) 
				{
					new_edge_val = snap_rect.mRight + snap_delta;
				}
				break;
			}
			next_column = mColumn->mParentCtrl->getColumn(next_column->mIndex + 1);
		}
	}

	return this;
}

void LLColumnHeader::userSetShape(const LLRect& new_rect)
{
	S32 new_width = new_rect.getWidth();
	S32 delta_width = new_width - (mRect.getWidth() + mColumn->mParentCtrl->getColumnPadding());

	if (delta_width != 0)
	{
		S32 remaining_width = delta_width;
		S32 col;
		for (col = mColumn->mIndex + 1; col < mColumn->mParentCtrl->getNumColumns(); col++)
		{
			LLScrollListColumn* columnp = mColumn->mParentCtrl->getColumn(col);
			if (!columnp) break;

			if (columnp->mHeader && columnp->mHeader->canResize())
			{
				// how many pixels in width can this column afford to give up?
				S32 resize_buffer_amt = llmax(0, columnp->mWidth - MIN_COLUMN_WIDTH);
				
				// user shrinking column, need to add width to other columns
				if (delta_width < 0)
				{
					if (!columnp->mDynamicWidth && columnp->mWidth > 0)
					{
						// statically sized column, give all remaining width to this column
						columnp->mWidth -= remaining_width;
						if (columnp->mRelWidth > 0.f)
						{
							columnp->mRelWidth = (F32)columnp->mWidth / (F32)mColumn->mParentCtrl->getItemListRect().getWidth();
						}
					}
					break;
				}
				else
				{
					// user growing column, need to take width from other columns
					remaining_width -= resize_buffer_amt;

					if (!columnp->mDynamicWidth && columnp->mWidth > 0)
					{
						columnp->mWidth -= llmin(columnp->mWidth - MIN_COLUMN_WIDTH, delta_width);
						if (columnp->mRelWidth > 0.f)
						{
							columnp->mRelWidth = (F32)columnp->mWidth / (F32)mColumn->mParentCtrl->getItemListRect().getWidth();
						}
					}

					if (remaining_width <= 0)
					{
						// width sucked up from neighboring columns, done
						break;
					}
				}
			}
		}

		// clamp resize amount to maximum that can be absorbed by other columns
		if (delta_width > 0)
		{
			delta_width -= llmax(remaining_width, 0);
		}

		// propagate constrained delta_width to new width for this column
		new_width = mRect.getWidth() + delta_width - mColumn->mParentCtrl->getColumnPadding();

		// use requested width
		mColumn->mWidth = new_width;

		// update proportional spacing
		if (mColumn->mRelWidth > 0.f)
		{
			mColumn->mRelWidth = (F32)new_width / (F32)mColumn->mParentCtrl->getItemListRect().getWidth();
		}

		// tell scroll list to layout columns again
		mColumn->mParentCtrl->updateColumns();
	}
}

void LLColumnHeader::setHasResizableElement(BOOL resizable)
{
	// for now, dynamically spaced columns can't be resized
	if (mColumn->mDynamicWidth) return;

	if (resizable != mHasResizableElement)
	{
		mHasResizableElement = resizable;

		S32 num_resizable_columns = 0;
		S32 col;
		for (col = 0; col < mColumn->mParentCtrl->getNumColumns(); col++)
		{
			LLScrollListColumn* columnp = mColumn->mParentCtrl->getColumn(col);
			if (columnp->mHeader && columnp->mHeader->canResize())
			{
				num_resizable_columns++;
			}
		}

		S32 num_resizers_enabled = 0;

		// now enable/disable resize handles on resizable columns if we have at least two
		for (col = 0; col < mColumn->mParentCtrl->getNumColumns(); col++)
		{
			LLScrollListColumn* columnp = mColumn->mParentCtrl->getColumn(col);
			if (!columnp->mHeader) continue;
			BOOL enable = num_resizable_columns >= 2 && num_resizers_enabled < (num_resizable_columns - 1) && columnp->mHeader->canResize();
			columnp->mHeader->enableResizeBar(enable);
			if (enable)
			{
				num_resizers_enabled++;
			}
		}
	}
}

void LLColumnHeader::enableResizeBar(BOOL enable)
{
	mResizeBar->setEnabled(enable);
}

BOOL LLColumnHeader::canResize()
{
	return getVisible() && (mHasResizableElement || mColumn->mDynamicWidth);
}
