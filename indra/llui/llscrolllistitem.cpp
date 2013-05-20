/** 
 * @file llscrolllistitem.cpp
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

#include "llscrolllistitem.h"

#include "llrect.h"
#include "llui.h"


//---------------------------------------------------------------------------
// LLScrollListItem
//---------------------------------------------------------------------------

LLScrollListItem::LLScrollListItem( const Params& p )
:	mSelected(FALSE),
	mHighlighted(FALSE),
	mEnabled(p.enabled),
	mUserdata(p.userdata),
	mItemValue(p.value)
{
}


LLScrollListItem::~LLScrollListItem()
{
	std::for_each(mColumns.begin(), mColumns.end(), DeletePointer());
}

void LLScrollListItem::addColumn(const LLScrollListCell::Params& p)
{
	mColumns.push_back(LLScrollListCell::create(p));
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


S32 LLScrollListItem::getNumColumns() const
{
	return mColumns.size();
}

LLScrollListCell* LLScrollListItem::getColumn(const S32 i) const
{
	if (0 <= i && i < (S32)mColumns.size())
	{
		return mColumns[i];
	} 
	return NULL;
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
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLRect bg_rect = rect;
	gl_rect_2d( bg_rect, bg_color );

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
			LLUI::translate((F32) cur_x, (F32) rect.mBottom);

			cell->draw( fg_color, highlight_color );
		}
		LLUI::popMatrix();
		
		cur_x += cell->getWidth() + column_padding;
	}
}

