/** 
 * @file llscrollcolumnheader.h
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

#ifndef LLSCROLLLISTCOLUMN_H
#define LLSCROLLLISTCOLUMN_H

#include "llrect.h"
#include "lluistring.h"
#include "llbutton.h"
#include "llinitparam.h"

class LLScrollListColumn;
class LLResizeBar;
class LLScrollListCtrl;

class LLScrollColumnHeader : public LLButton
{
public:
	struct Params : public LLInitParam::Block<Params, LLButton::Params>
	{
		Mandatory<LLScrollListColumn*> column;

		Params();
	};
	LLScrollColumnHeader(const Params&);
	~LLScrollColumnHeader();

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask);

	/*virtual*/ LLView*	findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding);
	/*virtual*/ void handleReshape(const LLRect& new_rect, bool by_user = false);
	
	LLScrollListColumn* getColumn() { return mColumn; }
	void setHasResizableElement(BOOL resizable);
	void updateResizeBars();
	BOOL canResize();
	void enableResizeBar(BOOL enable);

	void onClick(const LLSD& data);

private:
	LLScrollListColumn* mColumn;
	LLResizeBar*		mResizeBar;
	BOOL				mHasResizableElement;
};

/*
 * A simple data class describing a column within a scroll list.
 */
class LLScrollListColumn
{
public:
	typedef enum e_sort_direction
	{
		DESCENDING,
		ASCENDING
	} ESortDirection;

	struct SortNames
	:	public LLInitParam::TypeValuesHelper<LLScrollListColumn::ESortDirection, SortNames>
	{
		static void declareValues();
	};

	struct Params : public LLInitParam::Block<Params>
	{
		Optional<std::string>				name,
											tool_tip;
		Optional<std::string>				sort_column;
		Optional<ESortDirection, SortNames>	sort_direction;
		Optional<bool>						sort_ascending;

		struct Width : public LLInitParam::ChoiceBlock<Width>
		{
			Alternative<bool>	dynamic_width;
			Alternative<S32>		pixel_width;
			Alternative<F32>		relative_width;

			Width()
			:	dynamic_width("dynamic_width", false),
				pixel_width("width"),
				relative_width("relative_width", -1.f)
			{
				addSynonym(relative_width, "relwidth");
			}
		};
		Optional<Width>						width;

		// either an image or label is used in column header
		struct Header : public LLInitParam::ChoiceBlock<Header>
		{
			Alternative<std::string>			label;
			Alternative<LLUIImage*>			image;

			Header()
			:	label("label"),
				image("image")
			{}
		};
		Optional<Header>					header;

		Optional<LLFontGL::HAlign>			halign;

		Params()
		:	name("name"),
			tool_tip("tool_tip"),
			sort_column("sort_column"),
			sort_direction("sort_direction"),
			sort_ascending("sort_ascending", true),
			halign("halign", LLFontGL::LEFT)
		{
			// default choice to "dynamic_width"
			changeDefault(width.dynamic_width, true);

			addSynonym(sort_column, "sort");
		}
	};

	static const Params& getDefaultParams();

	//NOTE: this is default constructible so we can store it in a map.
	LLScrollListColumn(const Params& p = getDefaultParams(), LLScrollListCtrl* = NULL);

	void setWidth(S32 width);
	S32 getWidth() const { return mWidth; }

public:
	// Public data is fine so long as this remains a simple struct-like data class.
	// If it ever gets any smarter than that, these should all become private
	// with protected or public accessor methods added as needed. -MG
	std::string				mName;
	std::string				mSortingColumn;
	ESortDirection			mSortDirection;
	LLUIString				mLabel;
	F32						mRelWidth;
	BOOL					mDynamicWidth;
	S32						mMaxContentWidth;
	S32						mIndex;
	LLScrollListCtrl*		mParentCtrl;
	LLScrollColumnHeader*	mHeader;
	LLFontGL::HAlign		mFontAlignment;

private:
	S32						mWidth;
};

#endif
