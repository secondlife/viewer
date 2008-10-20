 /** 
 * @file llscrolllistctrl.cpp
 * @brief LLScrollListCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include <algorithm>

#include "linden_common.h"
#include "llstl.h"
#include "llboost.h"

#include "llscrolllistctrl.h"

#include "indra_constants.h"

#include "llcheckboxctrl.h"
#include "llclipboard.h"
#include "llfocusmgr.h"
#include "llrender.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llstring.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llcontrol.h"
#include "llkeyboard.h"
#include "llresizebar.h"

const S32 MIN_COLUMN_WIDTH = 20;
const S32 LIST_SNAP_PADDING = 5;

static LLRegisterWidget<LLScrollListCtrl> r("scroll_list");

// local structures & classes.
struct SortScrollListItem
{
	SortScrollListItem(const std::vector<std::pair<S32, BOOL> >& sort_orders)
	:	mSortOrders(sort_orders)
	{}

	bool operator()(const LLScrollListItem* i1, const LLScrollListItem* i2)
	{
		// sort over all columns in order specified by mSortOrders
		S32 sort_result = 0;
		for (sort_order_t::const_reverse_iterator it = mSortOrders.rbegin();
			 it != mSortOrders.rend(); ++it)
		{
			S32 col_idx = it->first;
			BOOL sort_ascending = it->second;

			const LLScrollListCell *cell1 = i1->getColumn(col_idx);
			const LLScrollListCell *cell2 = i2->getColumn(col_idx);
			S32 order = sort_ascending ? 1 : -1; // ascending or descending sort for this column?
			if (cell1 && cell2)
			{
				sort_result = order * LLStringUtil::compareDict(cell1->getValue().asString(), cell2->getValue().asString());
				if (sort_result != 0)
				{
					break; // we have a sort order!
				}
			}
		}

		return sort_result < 0;
	}

	typedef std::vector<std::pair<S32, BOOL> > sort_order_t;
	const sort_order_t& mSortOrders;
};


//
// LLScrollListIcon
//
LLScrollListIcon::LLScrollListIcon(LLUIImagePtr icon, S32 width)
	: LLScrollListCell(width),
	  mIcon(icon),
	  mColor(LLColor4::white)
{
}

LLScrollListIcon::LLScrollListIcon(const LLSD& value, S32 width)
	: LLScrollListCell(width),
	mColor(LLColor4::white)
{
	setValue(value);
}


LLScrollListIcon::~LLScrollListIcon()
{
}

void LLScrollListIcon::setValue(const LLSD& value)
{
	if (value.isUUID())
	{
		// don't use default image specified by LLUUID::null, use no image in that case
		LLUUID image_id = value.asUUID();
		mIcon = image_id.notNull() ? LLUI::sImageProvider->getUIImageByID(image_id) : LLUIImagePtr(NULL);
	}
	else
	{
		std::string value_string = value.asString();
		if (LLUUID::validate(value_string))
		{
			setValue(LLUUID(value_string));
		}
		else if (!value_string.empty())
		{
			mIcon = LLUI::getUIImage(value.asString());
		}
		else
		{
			mIcon = NULL;
		}
	}
}


void LLScrollListIcon::setColor(const LLColor4& color)
{
	mColor = color;
}

S32	LLScrollListIcon::getWidth() const 
{
	// if no specified fix width, use width of icon
	if (LLScrollListCell::getWidth() == 0 && mIcon.notNull())
	{
		return mIcon->getWidth();
	}
	return LLScrollListCell::getWidth();
}


void LLScrollListIcon::draw(const LLColor4& color, const LLColor4& highlight_color)	 const
{
	if (mIcon)
	{
		mIcon->draw(0, 0, mColor);
	}
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
		setWidth(width);
	}
	else
	{
		setWidth(rect.getWidth()); //check_box->getWidth();
	}
}

LLScrollListCheck::~LLScrollListCheck()
{
	delete mCheckBox;
}

void LLScrollListCheck::draw(const LLColor4& color, const LLColor4& highlight_color) const
{
	mCheckBox->draw();
}

BOOL LLScrollListCheck::handleClick()
{ 
	if (mCheckBox->getEnabled())
	{
		mCheckBox->toggle();
	}
	// don't change selection when clicking on embedded checkbox
	return TRUE; 
}

//
// LLScrollListSeparator
//
LLScrollListSeparator::LLScrollListSeparator(S32 width) : LLScrollListCell(width)
{
}

//virtual 
S32 LLScrollListSeparator::getHeight() const
{
	return 5;
}


void LLScrollListSeparator::draw(const LLColor4& color, const LLColor4& highlight_color) const
{
	//*FIXME: use dynamic item heights and make separators narrow, and inactive
	gl_line_2d(5, 8, llmax(5, getWidth() - 5), 8, color);
}

//
// LLScrollListText
//
U32 LLScrollListText::sCount = 0;

LLScrollListText::LLScrollListText( const std::string& text, const LLFontGL* font, S32 width, U8 font_style, LLFontGL::HAlign font_alignment, LLColor4& color, BOOL use_color, BOOL visible)
:	LLScrollListCell(width),
	mText( text ),
	mFont( font ),
	mColor(color),
	mUseColor(use_color),
	mFontStyle( font_style ),
	mFontAlignment( font_alignment ),
	mVisible( visible ),
	mHighlightCount( 0 ),
	mHighlightOffset( 0 )
{
	sCount++;

	// initialize rounded rect image
	if (!mRoundedRectImage)
	{
		mRoundedRectImage = LLUI::sImageProvider->getUIImage("rounded_square.tga");
	}
}
//virtual 
void LLScrollListText::highlightText(S32 offset, S32 num_chars)
{
	mHighlightOffset = offset;
	mHighlightCount = num_chars;
}

//virtual 
BOOL LLScrollListText::isText() const
{
	return TRUE;
}

//virtual 
BOOL LLScrollListText::getVisible() const
{
	return mVisible;
}

//virtual 
S32 LLScrollListText::getHeight() const
{
	return llround(mFont->getLineHeight());
}


LLScrollListText::~LLScrollListText()
{
	sCount--;
}

S32	LLScrollListText::getContentWidth() const
{
	return mFont->getWidth(mText.getString());
}


void LLScrollListText::setColor(const LLColor4& color)
{
	mColor = color;
	mUseColor = TRUE;
}

void LLScrollListText::setText(const LLStringExplicit& text)
{
	mText = text;
}

//virtual
void LLScrollListText::setValue(const LLSD& text)
{
	setText(text.asString());
}

//virtual 
const LLSD LLScrollListText::getValue() const		
{ 
	return LLSD(mText.getString()); 
}


void LLScrollListText::draw(const LLColor4& color, const LLColor4& highlight_color) const
{
	LLColor4 display_color;
	if (mUseColor)
	{
		display_color = mColor;
	}
	else
	{
		display_color = color;
	}

	if (mHighlightCount > 0)
	{
		S32 left = 0;
		switch(mFontAlignment)
		{
		case LLFontGL::LEFT:
			left = mFont->getWidth(mText.getString(), 0, mHighlightOffset);
			break;
		case LLFontGL::RIGHT:
			left = getWidth() - mFont->getWidth(mText.getString(), mHighlightOffset, S32_MAX);
			break;
		case LLFontGL::HCENTER:
			left = (getWidth() - mFont->getWidth(mText.getString())) / 2;
			break;
		}
		LLRect highlight_rect(left - 2, 
				llround(mFont->getLineHeight()) + 1, 
				left + mFont->getWidth(mText.getString(), mHighlightOffset, mHighlightCount) + 1, 
				1);
		mRoundedRectImage->draw(highlight_rect, highlight_color);
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
		start_x = (F32)getWidth();
		break;
	case LLFontGL::HCENTER:
		start_x = (F32)getWidth() * 0.5f;
		break;
	}
	mFont->render(mText.getWString(), 0, 
						start_x, 2.f,
						display_color,
						mFontAlignment,
						LLFontGL::BOTTOM, 
						mFontStyle,
						string_chars, 
						getWidth(),
						&right_x, 
						FALSE, 
						TRUE);
}


LLScrollListItem::~LLScrollListItem()
{
	std::for_each(mColumns.begin(), mColumns.end(), DeletePointer());
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

std::string LLScrollListItem::getContentsCSV() const
{
	std::string ret;

	S32 count = getNumColumns();
	for (S32 i=0; i<count; ++i)
	{
		ret += getColumn(i)->getValue().asString();
		if (i < count-1)
		{
			ret += ", ";
		}
	}

	return ret;
}

void LLScrollListItem::draw(const LLRect& rect, const LLColor4& fg_color, const LLColor4& bg_color, const LLColor4& highlight_color, S32 column_padding)
{
	// draw background rect
	LLRect bg_rect = rect;
	{
		LLGLSNoTexture no_texture;
		gGL.color4fv(bg_color.mV);
		gl_rect_2d( bg_rect );
	}

	S32 cur_x = rect.mLeft;
	S32 num_cols = getNumColumns();
	S32 cur_col = 0;

	for (LLScrollListCell* cell = getColumn(0); cur_col < num_cols; cell = getColumn(++cur_col))
	{
		// Two ways a cell could be hidden
		if (cell->getWidth() < 0
			|| !cell->getVisible()) continue;

		LLUI::pushMatrix();
		{
			LLUI::translate((F32) cur_x, (F32) rect.mBottom, 0.0f);

			cell->draw( fg_color, highlight_color );
		}
		LLUI::popMatrix();
		
		cur_x += cell->getWidth() + column_padding;
	}
}


void LLScrollListItem::setEnabled(BOOL b)
{
	mEnabled = b;
}

//---------------------------------------------------------------------------
// LLScrollListItemComment
//---------------------------------------------------------------------------
LLScrollListItemComment::LLScrollListItemComment(const std::string& comment_string, const LLColor4& color)
: LLScrollListItem(FALSE),
	mColor(color)
{
	addColumn( comment_string, LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF_SMALL ) );
}

void LLScrollListItemComment::draw(const LLRect& rect, const LLColor4& fg_color, const LLColor4& bg_color, const LLColor4& highlight_color, S32 column_padding)
{
	LLScrollListCell* cell = getColumn(0);
	if (cell)
	{
		// Two ways a cell could be hidden
		if (cell->getWidth() < 0
			|| !cell->getVisible()) return;

		LLUI::pushMatrix();
		{
			LLUI::translate((F32)rect.mLeft, (F32)rect.mBottom, 0.0f);

			// force first cell to be width of entire item
			cell->setWidth(rect.getWidth());
			cell->draw( mColor, highlight_color );
		}
		LLUI::popMatrix();
	}
}

//---------------------------------------------------------------------------
// LLScrollListItemSeparator
//---------------------------------------------------------------------------
LLScrollListItemSeparator::LLScrollListItemSeparator()
: LLScrollListItem(FALSE)
{
	LLScrollListSeparator* cell = new LLScrollListSeparator(0);
	setNumColumns(1);
	setColumn(0, cell);
}

void LLScrollListItemSeparator::draw(const LLRect& rect, const LLColor4& fg_color, const LLColor4& bg_color, const LLColor4& highlight_color, S32 column_padding)
{
	//TODO* move LLScrollListSeparator::draw into here and get rid of it
	LLScrollListCell* cell = getColumn(0);
	if (cell)
	{
		// Two ways a cell could be hidden
		if (cell->getWidth() < 0
			|| !cell->getVisible()) return;

		LLUI::pushMatrix();
		{
			LLUI::translate((F32)rect.mLeft, (F32)rect.mBottom, 0.0f);

			// force first cell to be width of entire item
			cell->setWidth(rect.getWidth());
			cell->draw( fg_color, highlight_color );
		}
		LLUI::popMatrix();
	}
}

//---------------------------------------------------------------------------
// LLScrollListCtrl
//---------------------------------------------------------------------------

LLScrollListCtrl::LLScrollListCtrl(const std::string& name, const LLRect& rect,
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
	mNeedsScroll(FALSE),
	mCanSelect(TRUE),
	mDisplayColumnHeaders(FALSE),
	mColumnsDirty(FALSE),
	mMaxItemCount(INT_MAX), 
	mMaxContentWidth(0),
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
	mBorderThickness( 2 ),
	mOnDoubleClickCallback( NULL ),
	mOnMaximumSelectCallback( NULL ),
	mOnSortChangedCallback( NULL ),
	mHighlightedItem(-1),
	mBorder(NULL),
	mSearchColumn(0),
	mNumDynamicWidthColumns(0),
	mTotalStaticColumnWidth(0),
	mSorted(TRUE),
	mDirty(FALSE),
	mOriginalSelection(-1),
	mDrewSelected(FALSE)
{
	mItemListRect.setOriginAndSize(
		mBorderThickness,
		mBorderThickness,
		getRect().getWidth() - 2 * mBorderThickness,
		getRect().getHeight() - 2 * mBorderThickness );

	updateLineHeight();

	mPageLines = mLineHeight? (mItemListRect.getHeight()) / mLineHeight : 0;

	// Init the scrollbar
	LLRect scroll_rect;
	scroll_rect.setOriginAndSize( 
		getRect().getWidth() - mBorderThickness - SCROLLBAR_SIZE,
		mItemListRect.mBottom,
		SCROLLBAR_SIZE,
		mItemListRect.getHeight());
	mScrollbar = new LLScrollbar( std::string("Scrollbar"), scroll_rect,
								  LLScrollbar::VERTICAL,
								  getItemCount(),
								  mScrollLines,
								  mPageLines,
								  &LLScrollListCtrl::onScrollChange, this );
	mScrollbar->setFollowsRight();
	mScrollbar->setFollowsTop();
	mScrollbar->setFollowsBottom();
	mScrollbar->setEnabled( TRUE );
	// scrollbar is visible only when needed
	mScrollbar->setVisible(FALSE);
	addChild(mScrollbar);

	// Border
	if (show_border)
	{
		LLRect border_rect( 0, getRect().getHeight(), getRect().getWidth(), 0 );
		mBorder = new LLViewBorder( std::string("dlg border"), border_rect, LLViewBorder::BEVEL_IN, LLViewBorder::STYLE_LINE, 1 );
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
	updateLayout();
	mDirty = FALSE; 
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

S32 LLScrollListCtrl::getFirstSelectedIndex() const
{
	S32 CurSelectedIndex = 0;
	item_list::const_iterator iter;
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

// returns first matching item
LLScrollListItem* LLScrollListCtrl::getItem(const LLSD& sd) const
{
	std::string string_val = sd.asString();

	item_list::const_iterator iter;
	for(iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item  = *iter;
		// assumes string representation is good enough for comparison
		if (item->getValue().asString() == string_val)
		{
			return item;
		}
	}
	return NULL;
}


void LLScrollListCtrl::reshape( S32 width, S32 height, BOOL called_from_parent )
{
	LLUICtrl::reshape( width, height, called_from_parent );

	updateLayout();
}

void LLScrollListCtrl::updateLayout()
{
	// reserve room for column headers, if needed
	S32 heading_size = (mDisplayColumnHeaders ? mHeadingHeight : 0);
	mItemListRect.setOriginAndSize(
		mBorderThickness,
		mBorderThickness,
		getRect().getWidth() - 2 * mBorderThickness,
		getRect().getHeight() - (2 * mBorderThickness ) - heading_size );

	// how many lines of content in a single "page"
	mPageLines = mLineHeight? mItemListRect.getHeight() / mLineHeight : 0;
	BOOL scrollbar_visible = getItemCount() > mPageLines;
	if (scrollbar_visible)
	{
		// provide space on the right for scrollbar
		mItemListRect.mRight = getRect().getWidth() - mBorderThickness - SCROLLBAR_SIZE;
	}

	mScrollbar->reshape(SCROLLBAR_SIZE, mItemListRect.getHeight() + (mDisplayColumnHeaders ? mHeadingHeight : 0));
	mScrollbar->setPageSize( mPageLines );
	mScrollbar->setDocSize( getItemCount() );
	mScrollbar->setVisible(scrollbar_visible);

	dirtyColumns();
}

// Attempt to size the control to show all items.
// Do not make larger than width or height.
void LLScrollListCtrl::fitContents(S32 max_width, S32 max_height)
{
	S32 height = llmin( getRequiredRect().getHeight(), max_height );
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


BOOL LLScrollListCtrl::addItem( LLScrollListItem* item, EAddPosition pos, BOOL requires_column )
{
	BOOL not_too_big = getItemCount() < mMaxItemCount;
	if (not_too_big)
	{
		switch( pos )
		{
		case ADD_TOP:
			mItemList.push_front(item);
			setSorted(FALSE);
			break;
	
		case ADD_SORTED:
			{
				// sort by column 0, in ascending order
				std::vector<sort_column_t> single_sort_column;
				single_sort_column.push_back(std::make_pair(0, TRUE));

				mItemList.push_back(item);
				std::stable_sort(
					mItemList.begin(), 
					mItemList.end(), 
					SortScrollListItem(single_sort_column));
				
				// ADD_SORTED just sorts by first column...
				// this might not match user sort criteria, so flag list as being in unsorted state
				setSorted(FALSE);
				break;
			}	
		case ADD_BOTTOM:
			mItemList.push_back(item);
			setSorted(FALSE);
			break;
	
		default:
			llassert(0);
			mItemList.push_back(item);
			setSorted(FALSE);
			break;
		}
	
		// create new column on demand
		if (mColumns.empty() && requires_column)
		{
			LLSD new_column;
			new_column["name"] = "default_column";
			new_column["label"] = "";
			new_column["dynamicwidth"] = TRUE;
			addColumn(new_column);
		}

		updateLineHeightInsert(item);

		updateLayout();
	}

	return not_too_big;
}

// NOTE: This is *very* expensive for large lists, especially when we are dirtying the list every frame
//  while receiving a long list of names.
// *TODO: Use bookkeeping to make this an incramental cost with item additions
void LLScrollListCtrl::calcColumnWidths()
{
	const S32 HEADING_TEXT_PADDING = 30;
	const S32 COLUMN_TEXT_PADDING = 20;

	mMaxContentWidth = 0;

	S32 max_item_width = 0;

	ordered_columns_t::iterator column_itor;
	for (column_itor = mColumnsIndexed.begin(); column_itor != mColumnsIndexed.end(); ++column_itor)
	{
		LLScrollListColumn* column = *column_itor;
		if (!column) continue;

		// update column width
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

		// update max content width for this column, by looking at all items
		column->mMaxContentWidth = column->mHeader ? LLFontGL::sSansSerifSmall->getWidth(column->mLabel) + mColumnPadding + HEADING_TEXT_PADDING : 0;
		item_list::iterator iter;
		for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
		{
			LLScrollListCell* cellp = (*iter)->getColumn(column->mIndex);
			if (!cellp) continue;

			column->mMaxContentWidth = llmax(LLFontGL::sSansSerifSmall->getWidth(cellp->getValue().asString()) + mColumnPadding + COLUMN_TEXT_PADDING, column->mMaxContentWidth);
		}

		max_item_width += column->mMaxContentWidth;
	}

	mMaxContentWidth = max_item_width;
}

const S32 SCROLL_LIST_ROW_PAD = 2;

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
			mLineHeight = llmax( mLineHeight, cell->getHeight() + SCROLL_LIST_ROW_PAD );
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
		mLineHeight = llmax( mLineHeight, cell->getHeight() + SCROLL_LIST_ROW_PAD );
	}
}


void LLScrollListCtrl::updateColumns()
{
	calcColumnWidths();

	// propagate column widths to individual cells
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

	// update column headers
	std::vector<LLScrollListColumn*>::iterator column_ordered_it;
	S32 left = mItemListRect.mLeft;
	LLColumnHeader* last_header = NULL;
	for (column_ordered_it = mColumnsIndexed.begin(); column_ordered_it != mColumnsIndexed.end(); ++column_ordered_it)
	{
		if ((*column_ordered_it)->mWidth < 0)
		{
			// skip hidden columns
			continue;
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
			last_header->translate(
				left - last_header->getRect().mLeft,
				top - last_header->getRect().mBottom);
			last_header->setVisible(mDisplayColumnHeaders && header_width > 0);
			left = right;
		}
	}

	//FIXME: stretch the entire last column if it is resizable (gestures windows shows truncated text in last column)
	// expand last column header we encountered to full list width
	if (last_header)
	{
		S32 new_width = llmax(0, mItemListRect.mRight - last_header->getRect().mLeft);
		last_header->reshape(new_width, last_header->getRect().getHeight());
		last_header->setVisible(mDisplayColumnHeaders && new_width > 0);
	}
}

void LLScrollListCtrl::setDisplayHeading(BOOL display)
{
	mDisplayColumnHeaders = display;

	updateLayout();
}

void LLScrollListCtrl::setHeadingHeight(S32 heading_height)
{
	mHeadingHeight = heading_height;

	updateLayout();

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
			mOriginalSelection = 0;
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

// Deselects all other items
// virtual
BOOL LLScrollListCtrl::selectNthItem( S32 target_index )
{
	return selectItemRange(target_index, target_index);
}

// virtual
BOOL LLScrollListCtrl::selectItemRange( S32 first_index, S32 last_index )
{
	if (mItemList.empty())
	{
		return FALSE;
	}

	S32 listlen = (S32)mItemList.size();
	first_index = llclamp(first_index, 0, listlen-1);
	
	if (last_index < 0)
		last_index = listlen-1;
	else
		last_index = llclamp(last_index, first_index, listlen-1);

	BOOL success = FALSE;
	S32 index = 0;
	for (item_list::iterator iter = mItemList.begin(); iter != mItemList.end(); )
	{
		LLScrollListItem *itemp = *iter;
		if(!itemp)
		{
			iter = mItemList.erase(iter);
			continue ;
		}
		
		if( index >= first_index && index <= last_index )
		{
			if( itemp->getEnabled() )
			{
				selectItem(itemp, FALSE);
				success = TRUE;				
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
	if (target_index < 0 || target_index >= (S32)mItemList.size())
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

void LLScrollListCtrl::highlightNthItem(S32 target_index)
{
	if (mHighlightedItem != target_index)
	{
		mHighlightedItem = target_index;
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

S32 LLScrollListCtrl::getItemIndex( LLScrollListItem* target_item ) const
{
	S32 index = 0;
	item_list::const_iterator iter;
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

S32 LLScrollListCtrl::getItemIndex( const LLUUID& target_id ) const
{
	S32 index = 0;
	item_list::const_iterator iter;
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
		// select last item
		selectNthItem(getItemCount() - 1);
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

			// don't allow navigation to disabled elements
			prev_item = cur_item->getEnabled() ? cur_item : prev_item;
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
	LLScrollListItem* next_item = NULL;

	if (!getFirstSelected())
	{
		selectFirstItem();
	}
	else
	{
		item_list::reverse_iterator iter;
		for (iter = mItemList.rbegin(); iter != mItemList.rend(); iter++)
		{
			LLScrollListItem* cur_item = *iter;

			if (cur_item->getSelected())
			{
				if (next_item)
				{
					selectItem(next_item, !extend_selection);
				}
				else
				{
					reportInvalidInput();
				}
				break;
			}

			// don't allow navigation to disabled items
			next_item = cur_item->getEnabled() ? cur_item : next_item;
		}
	}

	if (mCommitOnKeyboardMovement)
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
// Use this to add comment text such as "Searching", which ignores column settings of list

LLScrollListItem* LLScrollListCtrl::addCommentText(const std::string& comment_text, EAddPosition pos)
{
	LLScrollListItem* item = NULL;
	if (getItemCount() < mMaxItemCount)
	{
		// always draw comment text with "enabled" color
		item = new LLScrollListItemComment( comment_text, mFgUnselectedColor );
		addItem( item, pos, FALSE );
	}
	return item;
}

LLScrollListItem* LLScrollListCtrl::addSeparator(EAddPosition pos)
{
	LLScrollListItem* item = new LLScrollListItemSeparator();
	addItem(item, pos, FALSE);
	return item;
}

// Selects first enabled item of the given name.
// Returns false if item not found.
BOOL LLScrollListCtrl::selectItemByLabel(const std::string& label, BOOL case_sensitive)
{
	// ensure that no stale items are selected, even if we don't find a match
	deselectAllItems(TRUE);
	//RN: assume no empty items
	if (label.empty())
	{
		return FALSE;
	}

	std::string target_text = label;
	if (!case_sensitive)
	{
		LLStringUtil::toLower(target_text);
	}

	BOOL found = FALSE;

	item_list::iterator iter;
	S32 index = 0;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item = *iter;
		// Only select enabled items with matching names
		std::string item_text = item->getColumn(0)->getValue().asString();
		if (!case_sensitive)
		{
			LLStringUtil::toLower(item_text);
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


BOOL LLScrollListCtrl::selectItemByPrefix(const std::string& target, BOOL case_sensitive)
{
	return selectItemByPrefix(utf8str_to_wstring(target), case_sensitive);
}

// Selects first enabled item that has a name where the name's first part matched the target string.
// Returns false if item not found.
BOOL LLScrollListCtrl::selectItemByPrefix(const LLWString& target, BOOL case_sensitive)
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
			BOOL select = cellp ? item->getEnabled() && ('\0' == cellp->getValue().asString()[0]) : FALSE;
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
			LLWStringUtil::toLower(target_trimmed);
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
			LLWString item_label = utf8str_to_wstring(cellp->getValue().asString());
			if (!case_sensitive)
			{
				LLWStringUtil::toLower(item_label);
			}
			// remove extraneous whitespace from searchable label
			LLWString trimmed_label = item_label;
			LLWStringUtil::trim(trimmed_label);
			
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

const std::string LLScrollListCtrl::getSelectedItemLabel(S32 column) const
{
	LLScrollListItem* item;

	item = getFirstSelected();
	if (item)
	{
		return item->getColumn(column)->getValue().asString();
	}

	return LLStringUtil::null;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// "StringUUID" interface: use this when you're creating a list that contains non-unique strings each of which
// has an associated, unique UUID, and only one of which can be selected at a time.

LLScrollListItem* LLScrollListCtrl::addStringUUIDItem(const std::string& item_text, const LLUUID& id, EAddPosition pos, BOOL enabled, S32 column_width)
{
	LLScrollListItem* item = NULL;
	if (getItemCount() < mMaxItemCount)
	{
		item = new LLScrollListItem( enabled, NULL, id );
		item->addColumn(item_text, LLResMgr::getInstance()->getRes(LLFONT_SANSSERIF_SMALL), column_width);
		addItem( item, pos );
	}
	return item;
}

// Select the line or lines that match this UUID
BOOL LLScrollListCtrl::selectByID( const LLUUID& id )
{
	return selectByValue( LLSD(id) );
}

BOOL LLScrollListCtrl::setSelectedByValue(const LLSD& value, BOOL selected)
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

BOOL LLScrollListCtrl::isSelected(const LLSD& value) const 
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
	return FALSE;
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
	S32 num_page_lines = mPageLines + 1;

	LLRect item_rect;

	LLGLSUIDefault gls_ui;
	
	{
		LLLocalClipRect clip(mItemListRect);

		S32 cur_y = y;
		
		mDrewSelected = FALSE;

		S32 line = 0;
		S32 max_columns = 0;

		LLColor4 highlight_color = LLColor4::white;
		F32 type_ahead_timeout = LLUI::sConfigGroup->getF32("TypeAheadTimeout");
		highlight_color.mV[VALPHA] = clamp_rescale(mSearchTimer.getElapsedTimeF32(), type_ahead_timeout * 0.7f, type_ahead_timeout, 0.4f, 0.f);

		item_list::iterator iter;
		for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
		{
			LLScrollListItem* item = *iter;
			
			item_rect.setOriginAndSize( 
				x, 
				cur_y, 
				mItemListRect.getWidth(),
				mLineHeight );

			//llinfos << item_rect.getWidth() << llendl;

			if (item->getSelected())
			{
				mDrewSelected = TRUE;
			}

			max_columns = llmax(max_columns, item->getNumColumns());

			LLColor4 fg_color;
			LLColor4 bg_color(LLColor4::transparent);

			if( mScrollLines <= line && line < mScrollLines + num_page_lines )
			{
				fg_color = (item->getEnabled() ? mFgUnselectedColor : mFgDisabledColor);
				if( item->getSelected() && mCanSelect)
				{
					bg_color = mBgSelectedColor;
					fg_color = (item->getEnabled() ? mFgSelectedColor : mFgDisabledColor);
				}
				else if (mHighlightedItem == line && mCanSelect)
				{
					bg_color = mHighlightedColor;
				}
				else 
				{
					if (mDrawStripes && (line % 2 == 0) && (max_columns > 1))
					{
						bg_color = mBgStripeColor;
					}
				}

				if (!item->getEnabled())
				{
					bg_color = mBgReadOnlyColor;
				}

				item->draw(item_rect, fg_color, bg_color, highlight_color, mColumnPadding);

				cur_y -= mLineHeight;
			}
			line++;
		}
	}
}


void LLScrollListCtrl::draw()
{
	// if user specifies sort, make sure it is maintained
	if (needsSorting() && !isSorted())
	{
		sortItems();
	}

	if (mNeedsScroll)
	{
		scrollToShowSelected();
		mNeedsScroll = FALSE;
	}
	LLRect background(0, getRect().getHeight(), getRect().getWidth(), 0);
	// Draw background
	if (mBackgroundVisible)
	{
		LLGLSNoTexture no_texture;
		gGL.color4fv( getEnabled() ? mBgWriteableColor.mV : mBgReadOnlyColor.mV );
		gl_rect_2d(background);
	}

	if (mColumnsDirty)
	{
		updateColumns();
		mColumnsDirty = FALSE;
	}

	drawItems();

	if (mBorder)
	{
		mBorder->setKeyboardFocusHighlight(gFocusMgr.getKeyboardFocus() == this);
	}

	LLUICtrl::draw();
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

BOOL LLScrollListCtrl::handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen)
{
	S32 column_index = getColumnIndexFromOffset(x);
	LLScrollListColumn* columnp = getColumn(column_index);

	if (columnp == NULL) return FALSE;

	BOOL handled = FALSE;
	// show tooltip for full name of hovered item if it has been truncated
	LLScrollListItem* hit_item = hitItem(x, y);
	if (hit_item)
	{
		LLScrollListCell* hit_cell = hit_item->getColumn(column_index);
		if (!hit_cell) return FALSE;
		//S32 cell_required_width = hit_cell->getContentWidth();
		if (hit_cell 
			&& hit_cell->isText())
		{

			S32 rect_left = getColumnOffsetFromIndex(column_index) + mItemListRect.mLeft;
			S32 rect_bottom = getRowOffsetFromIndex(getItemIndex(hit_item));
			LLRect cell_rect;
			cell_rect.setOriginAndSize(rect_left, rect_bottom, rect_left + columnp->mWidth, mLineHeight);
			// Convert rect local to screen coordinates
			localPointToScreen( 
				cell_rect.mLeft, cell_rect.mBottom, 
				&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
			localPointToScreen(
				cell_rect.mRight, cell_rect.mTop, 
				&(sticky_rect_screen->mRight), &(sticky_rect_screen->mTop) );

			msg = hit_cell->getValue().asString();
		}
		handled = TRUE;
	}

	// otherwise, look for a tooltip associated with this column
	LLColumnHeader* headerp = columnp->mHeader;
	if (headerp && !handled)
	{
		headerp->handleToolTip(x, y, msg, sticky_rect_screen);
		handled = !msg.empty();
	}

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
							if (hit_item == lastSelected)
							{
								// stop selecting now, since we just clicked on our last selected item
								selecting = FALSE;
							}
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
	BOOL handled = childrenHandleMouseDown(x, y, mask) != NULL;

	if( !handled )
	{
		// set keyboard focus first, in case click action wants to move focus elsewhere
		setFocus(TRUE);

		// clear selection changed flag because user is starting a selection operation
		mSelectionChanged = FALSE;

		handleClick(x, y, mask);
	}

	return TRUE;
}

BOOL LLScrollListCtrl::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (hasMouseCapture())
	{
		// release mouse capture immediately so 
		// scroll to show selected logic will work
		gFocusMgr.setMouseCapture(NULL);
		if(mask == MASK_NONE)
		{
			selectItemAt(x, y, mask);
			mNeedsScroll = TRUE;
		}
	}

	// always commit when mouse operation is completed inside list
	if (mItemListRect.pointInRect(x,y))
	{
		mDirty |= mSelectionChanged;
		mSelectionChanged = FALSE;
		onCommit();
	}

	return LLUICtrl::handleMouseUp(x, y, mask);
}

BOOL LLScrollListCtrl::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	//BOOL handled = FALSE;
	BOOL handled = handleClick(x, y, mask);

	if (!handled)
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

BOOL LLScrollListCtrl::handleClick(S32 x, S32 y, MASK mask)
{
	// which row was clicked on?
	LLScrollListItem* hit_item = hitItem(x, y);
	if (!hit_item) return FALSE;

	// get appropriate cell from that row
	S32 column_index = getColumnIndexFromOffset(x);
	LLScrollListCell* hit_cell = hit_item->getColumn(column_index);
	if (!hit_cell) return FALSE;

	// if cell handled click directly (i.e. clicked on an embedded checkbox)
	if (hit_cell->handleClick())
	{
		// if item not currently selected, select it
		if (!hit_item->getSelected())
		{
			selectItemAt(x, y, mask);
			gFocusMgr.setMouseCapture(this);
			mNeedsScroll = TRUE;
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
				}
			}
			//FIXME: find a better way to signal cell changes
			onCommit();
		}
		// eat click (e.g. do not trigger double click callback)
		return TRUE;
	}
	else
	{
		// treat this as a normal single item selection
		selectItemAt(x, y, mask);
		gFocusMgr.setMouseCapture(this);
		mNeedsScroll = TRUE;
		// do not eat click (allow double click callback)
		return FALSE;
	}
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

	// allow for partial line at bottom
	S32 num_page_lines = mPageLines + 1;

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
		width = (*iter)->mWidth + mColumnPadding;
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
		column_offset += (*iter)->mWidth + mColumnPadding;
	}

	// when running off the end, return the rightmost pixel
	return mItemListRect.mRight;
}

S32 LLScrollListCtrl::getRowOffsetFromIndex(S32 index)
{
	S32 row_bottom = ((mItemListRect.mTop - (index - mScrollLines)) * mLineHeight) 
						- mLineHeight;
	return row_bottom;
}


BOOL LLScrollListCtrl::handleHover(S32 x,S32 y,MASK mask)
{
	BOOL	handled = FALSE;

	if (hasMouseCapture())
	{
		if(mask == MASK_NONE)
		{
			selectItemAt(x, y, mask);
			mNeedsScroll = TRUE;
		}
	}
	else 
	if (mCanSelect)
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

	return handled;
}


BOOL LLScrollListCtrl::handleKeyHere(KEY key,MASK mask )
{
	BOOL handled = FALSE;

	// not called from parent means we have keyboard focus or a child does
	if (mCanSelect) 
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
					mNeedsScroll = TRUE;
					handled = TRUE;
				}
				break;
			case KEY_DOWN:
				if (mAllowKeyboardMovement || hasFocus())
				{
					// commit implicit in call
					selectNextItem(FALSE);
					mNeedsScroll = TRUE;
					handled = TRUE;
				}
				break;
			case KEY_PAGE_UP:
				if (mAllowKeyboardMovement || hasFocus())
				{
					selectNthItem(getFirstSelectedIndex() - (mScrollbar->getPageSize() - 1));
					mNeedsScroll = TRUE;
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
					mNeedsScroll = TRUE;
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
					mNeedsScroll = TRUE;
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
					mNeedsScroll = TRUE;
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
			  	if (!mCommitOnKeyboardMovement && mask == MASK_NONE)
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
				else if (selectItemByPrefix(wstring_to_utf8str(mSearchString), FALSE))
				{
					mNeedsScroll = TRUE;
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

BOOL LLScrollListCtrl::handleUnicodeCharHere(llwchar uni_char)
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

	if (selectItemByPrefix(wstring_to_utf8str(mSearchString + (llwchar)uni_char), FALSE))
	{
		// update search string only on successful match
		mNeedsScroll = TRUE;
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
				LLWString item_label = utf8str_to_wstring(cellp->getValue().asString());
				if (item->getEnabled() && LLStringOps::toLower(item_label[0]) == uni_char)
				{
					selectItem(item);
					mNeedsScroll = TRUE;
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
	if (mSelectionChanged)
	{
		mDirty = TRUE;
		mSelectionChanged = FALSE;
		onCommit();
	}
}

struct SameSortColumn
{
	SameSortColumn(S32 column) : mColumn(column) {}
	S32 mColumn;

	bool operator()(std::pair<S32, BOOL> sort_column) { return sort_column.first == mColumn; }
};

BOOL LLScrollListCtrl::setSort(S32 column, BOOL ascending)
{
	sort_column_t new_sort_column(column, ascending);

	if (mSortColumns.empty())
	{
		mSortColumns.push_back(new_sort_column);
		return TRUE;
	}
	else
	{	
		// grab current sort column
		sort_column_t cur_sort_column = mSortColumns.back();
		
		// remove any existing sort criterion referencing this column
		// and add the new one
		mSortColumns.erase(remove_if(mSortColumns.begin(), mSortColumns.end(), SameSortColumn(column)), mSortColumns.end()); 
		mSortColumns.push_back(new_sort_column);

		// did the sort criteria change?
		return (cur_sort_column != new_sort_column);
	}
}

// Called by scrollbar
//static
void LLScrollListCtrl::onScrollChange( S32 new_pos, LLScrollbar* scrollbar, void* userdata )
{
	LLScrollListCtrl* self = (LLScrollListCtrl*) userdata;
	self->mScrollLines = new_pos;
}


void LLScrollListCtrl::sortByColumn(const std::string& name, BOOL ascending)
{
	std::map<std::string, LLScrollListColumn>::iterator itor = mColumns.find(name);
	if (itor != mColumns.end())
	{
		sortByColumnIndex((*itor).second.mIndex, ascending);
	}
}

// First column is column 0
void  LLScrollListCtrl::sortByColumnIndex(U32 column, BOOL ascending)
{
	if (setSort(column, ascending))
	{
		sortItems();
	}
}

void LLScrollListCtrl::sortItems()
{
	// do stable sort to preserve any previous sorts
	std::stable_sort(
		mItemList.begin(), 
		mItemList.end(), 
		SortScrollListItem(mSortColumns));

	setSorted(TRUE);
}

// for one-shot sorts, does not save sort column/order
void LLScrollListCtrl::sortOnce(S32 column, BOOL ascending)
{
	std::vector<std::pair<S32, BOOL> > sort_column;
	sort_column.push_back(std::make_pair(column, ascending));

	// do stable sort to preserve any previous sorts
	std::stable_sort(
		mItemList.begin(), 
		mItemList.end(), 
		SortScrollListItem(sort_column));
}

void LLScrollListCtrl::dirtyColumns() 
{ 
	mColumnsDirty = TRUE; 

	// need to keep mColumnsIndexed up to date
	// just in case someone indexes into it immediately
	mColumnsIndexed.resize(mColumns.size());

	std::map<std::string, LLScrollListColumn>::iterator column_itor;
	for (column_itor = mColumns.begin(); column_itor != mColumns.end(); ++column_itor)
	{
		LLScrollListColumn *column = &column_itor->second;
		mColumnsIndexed[column_itor->second.mIndex] = column;
	}
}


S32 LLScrollListCtrl::getScrollPos() const
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
	// don't scroll automatically when capturing mouse input
	// as that will change what is currently under the mouse cursor
	if (hasMouseCapture())
	{
		return;
	}

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

	std::map<std::string, LLScrollListColumn>::const_iterator itor;
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
	std::string name("scroll_list");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	BOOL multi_select = FALSE;
	node->getAttributeBOOL("multi_select", multi_select);

	BOOL draw_border = TRUE;
	node->getAttributeBOOL("draw_border", draw_border);

	BOOL draw_heading = FALSE;
	node->getAttributeBOOL("draw_heading", draw_heading);

	S32 search_column = 0;
	node->getAttributeS32("search_column", search_column);

	S32 sort_column = -1;
	node->getAttributeS32("sort_column", sort_column);

	BOOL sort_ascending = TRUE;
	node->getAttributeBOOL("sort_ascending", sort_ascending);

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

	scroll_list->setScrollListParameters(node);

	scroll_list->initFromXML(node, parent);

	scroll_list->setSearchColumn(search_column);

	if (sort_column >= 0)
	{
		scroll_list->sortByColumnIndex(sort_column, sort_ascending);
	}

	LLSD columns;
	S32 index = 0;
	LLXMLNodePtr child;
	S32 total_static = 0;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("column"))
		{
			std::string labelname("");
			child->getAttributeString("label", labelname);

			std::string columnname(labelname);
			child->getAttributeString("name", columnname);

			std::string sortname(columnname);
			child->getAttributeString("sort", sortname);
		
			BOOL sort_ascending = TRUE;
			child->getAttributeBOOL("sort_ascending", sort_ascending);

			std::string imagename;
			child->getAttributeString("image", imagename);

			BOOL columndynamicwidth = FALSE;
			child->getAttributeBOOL("dynamicwidth", columndynamicwidth);

			S32 columnwidth = -1;
			child->getAttributeS32("width", columnwidth);	

			std::string tooltip;
			child->getAttributeString("tool_tip", tooltip);

			if(!columndynamicwidth) total_static += llmax(0, columnwidth);

			F32 columnrelwidth = 0.f;
			child->getAttributeF32("relwidth", columnrelwidth);

			LLFontGL::HAlign h_align = LLFontGL::LEFT;
			h_align = LLView::selectFontHAlign(child);

			columns[index]["name"] = columnname;
			columns[index]["sort"] = sortname;
			columns[index]["sort_ascending"] = sort_ascending;
			columns[index]["image"] = imagename;
			columns[index]["label"] = labelname;
			columns[index]["width"] = columnwidth;
			columns[index]["relwidth"] = columnrelwidth;
			columns[index]["dynamicwidth"] = columndynamicwidth;
			columns[index]["halign"] = (S32)h_align;
			columns[index]["tool_tip"] = tooltip;
			
			index++;
		}
	}
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
					std::string value = row_child->getTextContents();

					std::string columnname("");
					row_child->getAttributeString("name", columnname);

					std::string font("");
					row_child->getAttributeString("font", font);

					std::string font_style("");
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

	std::string contents = node->getTextContents();
	if (!contents.empty())
	{
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("\t\n");
		tokenizer tokens(contents, sep);
		tokenizer::iterator token_iter = tokens.begin();

		while(token_iter != tokens.end())
		{
			const std::string& line = *token_iter;
			scroll_list->addSimpleElement(line);
			++token_iter;
		}
	}
	
	return scroll_list;
}

// LLEditMenuHandler functions

// virtual
void	LLScrollListCtrl::copy()
{
	std::string buffer;

	std::vector<LLScrollListItem*> items = getAllSelected();
	std::vector<LLScrollListItem*>::iterator itor;
	for (itor = items.begin(); itor != items.end(); ++itor)
	{
		buffer += (*itor)->getContentsCSV() + "\n";
	}
	gClipboard.copyFromSubstring(utf8str_to_wstring(buffer), 0, buffer.length());
}

// virtual
BOOL	LLScrollListCtrl::canCopy() const
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
BOOL	LLScrollListCtrl::canCut() const
{
	return canCopy() && canDoDelete();
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
BOOL	LLScrollListCtrl::canSelectAll() const
{
	return getCanSelect() && mAllowMultipleSelection && !(mMaxSelectable > 0 && mItemList.size() > mMaxSelectable);
}

// virtual
void	LLScrollListCtrl::deselect()
{
	deselectAllItems();
}

// virtual
BOOL	LLScrollListCtrl::canDeselect() const
{
	return getCanSelect();
}

void LLScrollListCtrl::addColumn(const LLSD& column, EAddPosition pos)
{
	std::string name = column["name"].asString();
	// if no column name provided, just use ordinal as name
	if (name.empty())
	{
		std::ostringstream new_name;
		new_name << mColumnsIndexed.size();
		name = new_name.str();
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
				mNumDynamicWidthColumns++;
				new_column->mWidth = (mItemListRect.getWidth() - mTotalStaticColumnWidth) / mNumDynamicWidthColumns;
			}
			S32 top = mItemListRect.mTop;
			S32 left = mItemListRect.mLeft;
			{
				std::map<std::string, LLScrollListColumn>::iterator itor;
				for (itor = mColumns.begin(); itor != mColumns.end(); ++itor)
				{
					if (itor->second.mIndex < new_column->mIndex &&
						itor->second.mWidth > 0)
					{
						left += itor->second.mWidth + mColumnPadding;
					}
				}
			}
			std::string button_name = "btn_" + name;
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

			new_column->mHeader->setToolTip(column["tool_tip"].asString());

			//RN: although it might be useful to change sort order with the keyboard,
			// mixing tab stops on child items along with the parent item is not supported yet
			new_column->mHeader->setTabStop(FALSE);
			addChild(new_column->mHeader);
			new_column->mHeader->setVisible(mDisplayColumnHeaders);

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

	S32 column_index = info->mIndex;

	LLScrollListColumn* column = parent->mColumnsIndexed[info->mIndex];
	bool ascending = column->mSortAscending;
	if (column->mSortingColumn != column->mName
		&& parent->mColumns.find(column->mSortingColumn) != parent->mColumns.end())
	{
		LLScrollListColumn& info_redir = parent->mColumns[column->mSortingColumn];
		column_index = info_redir.mIndex;
	}

	// if this column is the primary sort key, reverse the direction
	sort_column_t cur_sort_column;
	if (!parent->mSortColumns.empty() && parent->mSortColumns.back().first == column_index)
	{
		ascending = !parent->mSortColumns.back().second;
	}

	parent->sortByColumnIndex(column_index, ascending);

	if (parent->mOnSortChangedCallback)
	{
		parent->mOnSortChangedCallback(parent->getCallbackUserData());
	}
}

std::string LLScrollListCtrl::getSortColumnName()
{
	LLScrollListColumn* column = mSortColumns.empty() ? NULL : mColumnsIndexed[mSortColumns.back().first];

	if (column) return column->mName;
	else return "";
}

BOOL LLScrollListCtrl::needsSorting()
{
	return !mSortColumns.empty();
}

void LLScrollListCtrl::clearColumns()
{
	std::map<std::string, LLScrollListColumn>::iterator itor;
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
	mSortColumns.clear();
}

void LLScrollListCtrl::setColumnLabel(const std::string& column, const std::string& label)
{
	std::map<std::string, LLScrollListColumn>::iterator itor = mColumns.find(column);
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
	S32 col_index = 0 ;
	for (itor = columns.beginArray(); itor != columns.endArray(); ++itor)
	{
		if (itor->isUndefined())
		{
			// skip unused columns in item passed in
			continue;
		}
		std::string column = (*itor)["column"].asString();

		LLScrollListColumn* columnp = NULL;

		// empty columns strings index by ordinal
		if (column.empty())
		{
			std::ostringstream new_name;
			new_name << col_index;
			column = new_name.str();
		}

		std::map<std::string, LLScrollListColumn>::iterator column_itor;
		column_itor = mColumns.find(column);
		if (column_itor != mColumns.end()) 
		{
			columnp = &column_itor->second;
		}

		// create new column on demand
		if (!columnp)
		{
			LLSD new_column;
			new_column["name"] = column;
			new_column["label"] = column;
			// if width supplied for column, use it, otherwise 
			// use adaptive width
			if (itor->has("width"))
			{
				new_column["width"] = (*itor)["width"];
			}
			else
			{
				new_column["dynamicwidth"] = true;
			}
			addColumn(new_column);
			columnp = &mColumns[column];
			new_item->setNumColumns(mColumns.size());
		}

		S32 index = columnp->mIndex;
		S32 width = columnp->mWidth;
		LLFontGL::HAlign font_alignment = columnp->mFontAlignment;
		LLColor4 fcolor = LLColor4::black;
		
		LLSD value = (*itor)["value"];
		std::string fontname = (*itor)["font"].asString();
		std::string fontstyle = (*itor)["font-style"].asString();
		std::string type = (*itor)["type"].asString();
		
		if ((*itor).has("font-color"))
		{
			LLSD sd_color = (*itor)["font-color"];
			fcolor.setValue(sd_color);
		}
		
		BOOL has_color = (*itor).has("color");
		LLColor4 color = ((*itor)["color"]);
		BOOL enabled = !(*itor).has("enabled") || (*itor)["enabled"].asBoolean() == true;

		const LLFontGL *font = LLResMgr::getInstance()->getRes(fontname);
		if (!font)
		{
			font = LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF_SMALL );
		}
		U8 font_style = LLFontGL::getStyleFromString(fontstyle);

		if (type == "icon")
		{
			LLScrollListIcon* cell = new LLScrollListIcon(value, width);
			if (has_color)
			{
				cell->setColor(color);
			}
			new_item->setColumn(index, cell);
		}
		else if (type == "checkbox")
		{
			LLCheckBoxCtrl* ctrl = new LLCheckBoxCtrl(std::string("check"),
													  LLRect(0, width, width, 0), std::string(" "));
			ctrl->setEnabled(enabled);
			ctrl->setValue(value);
			LLScrollListCheck* cell = new LLScrollListCheck(ctrl,width);
			if (has_color)
			{
				cell->setColor(color);
			}
			new_item->setColumn(index, cell);
		}
		else if (type == "separator")
		{
			LLScrollListSeparator* cell = new LLScrollListSeparator(width);
			if (has_color)
			{
				cell->setColor(color);
			}
			new_item->setColumn(index, cell);
		}
		else
		{
			LLScrollListText* cell = new LLScrollListText(value.asString(), font, width, font_style, font_alignment, fcolor, TRUE);
			if (has_color)
			{
				cell->setColor(color);
			}
			new_item->setColumn(index, cell);
			if (columnp->mHeader && !value.asString().empty())
			{
				columnp->mHeader->setHasResizableElement(TRUE);
			}
		}

		col_index++;
	}

	// add dummy cells for missing columns
	for (column_map_t::iterator column_it = mColumns.begin(); column_it != mColumns.end(); ++column_it)
	{
		S32 column_idx = column_it->second.mIndex;
		if (new_item->getColumn(column_idx) == NULL)
		{
			LLScrollListColumn* column_ptr = &column_it->second;
			new_item->setColumn(column_idx, new LLScrollListText(LLStringUtil::null, LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF_SMALL ), column_ptr->mWidth, LLFontGL::NORMAL));
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

	LLScrollListItem *new_item = new LLScrollListItem(entry_id);

	const LLFontGL *font = LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF_SMALL );

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


// virtual 
BOOL	LLScrollListCtrl::isDirty() const		
{
	BOOL grubby = mDirty;
	if ( !mAllowMultipleSelection )
	{
		grubby = (mOriginalSelection != getFirstSelectedIndex());
	}
	return grubby;
}

// Clear dirty state
void LLScrollListCtrl::resetDirty()
{
	mDirty = FALSE;
	mOriginalSelection = getFirstSelectedIndex();
}


//virtual
void LLScrollListCtrl::onFocusReceived()
{
	// forget latent selection changes when getting focus
	mSelectionChanged = FALSE;
	LLUICtrl::onFocusReceived();
}

//virtual 
void LLScrollListCtrl::onFocusLost()
{
	if (hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
	}
	LLUICtrl::onFocusLost();
}

LLColumnHeader::LLColumnHeader(const std::string& label, const LLRect &rect, LLScrollListColumn* column, const LLFontGL* fontp) : 
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

	mAscendingText = std::string("[LOW]...[HIGH](Ascending)"); // *TODO: Translate
	mDescendingText = std::string("[HIGH]...[LOW](Descending)"); // *TODO: Translate

	mList->reshape(llmax(mList->getRect().getWidth(), 110, getRect().getWidth()), mList->getRect().getHeight());

	// resize handles on left and right
	const S32 RESIZE_BAR_THICKNESS = 3;
	mResizeBar = new LLResizeBar( 
		std::string("resizebar"),
		this,
		LLRect( getRect().getWidth() - RESIZE_BAR_THICKNESS, getRect().getHeight(), getRect().getWidth(), 0), 
		MIN_COLUMN_WIDTH, S32_MAX, LLResizeBar::RIGHT );
	addChild(mResizeBar);

	mResizeBar->setEnabled(FALSE);
}

LLColumnHeader::~LLColumnHeader()
{
}

void LLColumnHeader::draw()
{
	BOOL draw_arrow = !mColumn->mLabel.empty() && mColumn->mParentCtrl->isSorted() && mColumn->mParentCtrl->getSortColumnName() == mColumn->mSortingColumn;

	BOOL is_ascending = mColumn->mParentCtrl->getSortAscending();
	mButton->setImageOverlay(is_ascending ? "up_arrow.tga" : "down_arrow.tga", LLFontGL::RIGHT, draw_arrow ? LLColor4::white : LLColor4::transparent);
	mArrowImage = mButton->getImageOverlay();

	//BOOL clip = getRect().mRight > mColumn->mParentCtrl->getItemListRect().getWidth();
	//LLGLEnable scissor_test(clip ? GL_SCISSOR_TEST : GL_FALSE);

	//LLRect column_header_local_rect(-getRect().mLeft, getRect().getHeight(), mColumn->mParentCtrl->getItemListRect().getWidth() - getRect().mLeft, 0);
	//LLUI::setScissorRegionLocal(column_header_local_rect);

	// Draw children
	LLComboBox::draw();

	if (mList->getVisible())
	{
		// sync sort order with list selection every frame
		mColumn->mParentCtrl->sortByColumn(mColumn->mSortingColumn, getCurrentIndex() == 0);
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

void LLColumnHeader::setImage(const std::string &image_name)
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

		std::string low_item_text;
		std::string high_item_text;

		LLScrollListItem* itemp = mColumn->mParentCtrl->getFirstData();
		if (itemp)
		{
			LLScrollListCell* cell = itemp->getColumn(mColumn->mIndex);
			if (cell && cell->isText())
			{
				if (mColumn->mParentCtrl->getSortAscending())
				{
					low_item_text = cell->getValue().asString();
				}
				else
				{
					high_item_text = cell->getValue().asString();
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
					high_item_text = cell->getValue().asString();
				}
				else
				{
					low_item_text = cell->getValue().asString();
				}
			}
		}

		LLStringUtil::truncate(low_item_text, 3);
		LLStringUtil::truncate(high_item_text, 3);

		std::string ascending_string;
		std::string descending_string;

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
		text_width = llmax(text_width, getRect().getWidth() - 30);

		mList->getColumn(0)->mWidth = text_width;
		((LLScrollListText*)mList->getFirstData()->getColumn(0))->setText(ascending_string);
		((LLScrollListText*)mList->getLastData()->getColumn(0))->setText(descending_string);

		mList->reshape(llmax(text_width + 30, 110, getRect().getWidth()), mList->getRect().getHeight());

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
	S32 delta_width = new_width - (getRect().getWidth() /*+ mColumn->mParentCtrl->getColumnPadding()*/);

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
		new_width = getRect().getWidth() + delta_width - mColumn->mParentCtrl->getColumnPadding();

		// use requested width
		mColumn->mWidth = new_width;

		// update proportional spacing
		if (mColumn->mRelWidth > 0.f)
		{
			mColumn->mRelWidth = (F32)new_width / (F32)mColumn->mParentCtrl->getItemListRect().getWidth();
		}

		// tell scroll list to layout columns again
		// do immediate update to get proper feedback to resize handle
		// which needs to know how far the resize actually went
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
	// for now, dynamically spaced columns can't be resized
	if (!mColumn->mDynamicWidth)
	{
		mResizeBar->setEnabled(enable);
	}
}

BOOL LLColumnHeader::canResize()
{
	return getVisible() && (mHasResizableElement || mColumn->mDynamicWidth);
}
