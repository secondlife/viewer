/** 
 * @file llscrollcolumnheader.cpp
 * @brief Scroll lists are composed of rows (items), each of which 
 * contains columns (cells).
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llscrolllistcolumn.h"

#include "llbutton.h"
#include "llresizebar.h"
#include "llscrolllistcell.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lluictrlfactory.h"

const S32 MIN_COLUMN_WIDTH = 20;

// defaults for LLScrollColumnHeader param block pulled from widgets/scroll_column_header.xml
static LLWidgetNameRegistry::StaticRegistrar sRegisterColumnHeaderParams(&typeid(LLScrollColumnHeader::Params), "scroll_column_header");

//---------------------------------------------------------------------------
// LLScrollColumnHeader
//---------------------------------------------------------------------------
LLScrollColumnHeader::Params::Params()
:	column("column")
{}


LLScrollColumnHeader::LLScrollColumnHeader(const LLScrollColumnHeader::Params& p) 
:	LLButton(p), // use combobox params to steal images
	mColumn(p.column),
	mHasResizableElement(FALSE)
{
	setClickedCallback(boost::bind(&LLScrollColumnHeader::onClick, this, _2));
	
	// resize handles on left and right
	const S32 RESIZE_BAR_THICKNESS = 3;
	LLResizeBar::Params resize_bar_p;
	resize_bar_p.resizing_view(this);
	resize_bar_p.rect(LLRect(getRect().getWidth() - RESIZE_BAR_THICKNESS, getRect().getHeight(), getRect().getWidth(), 0));
	resize_bar_p.min_size(MIN_COLUMN_WIDTH);
	resize_bar_p.side(LLResizeBar::RIGHT);
	resize_bar_p.enabled(false);
	mResizeBar = LLUICtrlFactory::create<LLResizeBar>(resize_bar_p);
	addChild(mResizeBar);
}

LLScrollColumnHeader::~LLScrollColumnHeader()
{}

void LLScrollColumnHeader::draw()
{
	std::string sort_column = mColumn->mParentCtrl->getSortColumnName();
	BOOL draw_arrow = !mColumn->mLabel.empty() 
			&& mColumn->mParentCtrl->isSorted()
			// check for indirect sorting column as well as column's sorting name
			&& (sort_column == mColumn->mSortingColumn || sort_column == mColumn->mName);

	BOOL is_ascending = mColumn->mParentCtrl->getSortAscending();
	if (draw_arrow)
	{
		setImageOverlay(is_ascending ? "up_arrow.tga" : "down_arrow.tga", LLFontGL::RIGHT, LLColor4::white);
	}
	else
	{
		setImageOverlay(LLUUID::null);
	}

	// Draw children
	LLButton::draw();
}

BOOL LLScrollColumnHeader::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (canResize() && mResizeBar->getRect().pointInRect(x, y))
	{
		// reshape column to max content width
		mColumn->mParentCtrl->calcMaxContentWidth();
		LLRect column_rect = getRect();
		column_rect.mRight = column_rect.mLeft + mColumn->mMaxContentWidth;
		setShape(column_rect, true);
	}
	else
	{
		onClick(LLSD());
	}
	return TRUE;
}

void LLScrollColumnHeader::onClick(const LLSD& data)
{
	if (mColumn)
	{
		LLScrollListCtrl::onClickColumn(mColumn);
	}
}

LLView*	LLScrollColumnHeader::findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding)
{
	// this logic assumes dragging on right
	llassert(snap_edge == SNAP_RIGHT);

	// use higher snap threshold for column headers
	threshold = llmin(threshold, 10);

	LLRect snap_rect = getSnapRect();

	mColumn->mParentCtrl->calcMaxContentWidth();

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

void LLScrollColumnHeader::handleReshape(const LLRect& new_rect, bool by_user)
{
	S32 new_width = new_rect.getWidth();
	S32 delta_width = new_width - (getRect().getWidth() /*+ mColumn->mParentCtrl->getColumnPadding()*/);

	if (delta_width != 0)
	{
		S32 remaining_width = -delta_width;
		S32 col;
		for (col = mColumn->mIndex + 1; col < mColumn->mParentCtrl->getNumColumns(); col++)
		{
			LLScrollListColumn* columnp = mColumn->mParentCtrl->getColumn(col);
			if (!columnp) continue;

			if (columnp->mHeader && columnp->mHeader->canResize())
			{
				// how many pixels in width can this column afford to give up?
				S32 resize_buffer_amt = llmax(0, columnp->getWidth() - MIN_COLUMN_WIDTH);
				
				// user shrinking column, need to add width to other columns
				if (delta_width < 0)
				{
					if (columnp->getWidth() > 0)
					{
						// statically sized column, give all remaining width to this column
						columnp->setWidth(columnp->getWidth() + remaining_width);
						if (columnp->mRelWidth > 0.f)
						{
							columnp->mRelWidth = (F32)columnp->getWidth() / (F32)mColumn->mParentCtrl->getItemListRect().getWidth();
						}
						// all padding went to this widget, we're done
						break;
					}
				}
				else
				{
					// user growing column, need to take width from other columns
					remaining_width += resize_buffer_amt;

					if (columnp->getWidth() > 0)
					{
						columnp->setWidth(columnp->getWidth() - llmin(columnp->getWidth() - MIN_COLUMN_WIDTH, delta_width));
						if (columnp->mRelWidth > 0.f)
						{
							columnp->mRelWidth = (F32)columnp->getWidth() / (F32)mColumn->mParentCtrl->getItemListRect().getWidth();
						}
					}

					if (remaining_width >= 0)
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
			delta_width += llmin(remaining_width, 0);
		}

		// propagate constrained delta_width to new width for this column
		new_width = getRect().getWidth() + delta_width - mColumn->mParentCtrl->getColumnPadding();

		// use requested width
		mColumn->setWidth(new_width);

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

void LLScrollColumnHeader::setHasResizableElement(BOOL resizable)
{
	if (mHasResizableElement != resizable)
	{
		mColumn->mParentCtrl->dirtyColumns();
		mHasResizableElement = resizable;
	}
}

void LLScrollColumnHeader::updateResizeBars()
{
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

void LLScrollColumnHeader::enableResizeBar(BOOL enable)
{
	mResizeBar->setEnabled(enable);
}

BOOL LLScrollColumnHeader::canResize()
{
	return getVisible() && (mHasResizableElement || mColumn->mDynamicWidth);
}

void LLScrollListColumn::SortNames::declareValues()
{
	declare("ascending", LLScrollListColumn::ASCENDING);
	declare("descending", LLScrollListColumn::DESCENDING);
}

//
// LLScrollListColumn
//
//static 
const LLScrollListColumn::Params& LLScrollListColumn::getDefaultParams()
{
	return LLUICtrlFactory::getDefaultParams<LLScrollListColumn>();
}


LLScrollListColumn::LLScrollListColumn(const Params& p, LLScrollListCtrl* parent)
:	mWidth(0),
	mIndex (-1),
	mParentCtrl(parent),
	mName(p.name),
	mLabel(p.header.label),
	mHeader(NULL),
	mMaxContentWidth(0),
	mDynamicWidth(p.width.dynamic_width),
	mRelWidth(p.width.relative_width),
	mFontAlignment(p.halign),
	mSortingColumn(p.sort_column)
{
	if (p.sort_ascending.isProvided())
	{
		mSortDirection = p.sort_ascending() ? ASCENDING : DESCENDING;
	}
	else
	{
		mSortDirection = p.sort_direction;
	}

	setWidth(p.width.pixel_width);
}

void LLScrollListColumn::setWidth(S32 width) 
{ 
	if (!mDynamicWidth && mRelWidth <= 0.f) 
	{
		mParentCtrl->updateStaticColumnWidth(this, width);
	}
	mWidth = width;
}
