/** 
 * @file llscrolllistitem.cpp
 * @brief Scroll lists are composed of rows (items), each of which 
 * contains columns (cells).
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llscrolllistitem.h"

#include "llrect.h"
#include "llresmgr.h"		// LLFONT_SANSSERIF_SMALL
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
			LLUI::translate((F32) cur_x, (F32) rect.mBottom, 0.0f);

			cell->draw( fg_color, highlight_color );
		}
		LLUI::popMatrix();
		
		cur_x += cell->getWidth() + column_padding;
	}
}

