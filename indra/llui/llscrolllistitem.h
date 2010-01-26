/** 
 * @file llscrolllistitem.h
 * @brief Scroll lists are composed of rows (items), each of which 
 * contains columns (cells).
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LLSCROLLLISTITEM_H
#define LLSCROLLLISTITEM_H

#include "llpointer.h"		// LLPointer<>
#include "llsd.h"
#include "v4color.h"
#include "llinitparam.h"
#include "llscrolllistcell.h"

#include <vector>

class LLCoordGL;
class LLCheckBoxCtrl;
class LLResizeBar;
class LLScrollListCtrl;
class LLScrollColumnHeader;
class LLUIImage;

//---------------------------------------------------------------------------
// LLScrollListItem
//---------------------------------------------------------------------------
class LLScrollListItem
{
	friend class LLScrollListCtrl;
public:
	struct Params : public LLInitParam::Block<Params>
	{
		Optional<bool>		enabled;
		Optional<void*>		userdata;
		Optional<LLSD>		value;
		
		Ignored				name; // use for localization tools
		Ignored				type; 
		Ignored				length; 

		Multiple<LLScrollListCell::Params> columns;

		Params()
		:	enabled("enabled", true),
			value("value"),
			name("name"),
			type("type"),
			length("length"),
			columns("columns")
		{
			addSynonym(columns, "column");
			addSynonym(value, "id");
		}
	};

	virtual ~LLScrollListItem();

	void	setSelected( BOOL b )			{ mSelected = b; }
	BOOL	getSelected() const				{ return mSelected; }

	void	setEnabled( BOOL b )			{ mEnabled = b; }
	BOOL	getEnabled() const 				{ return mEnabled; }

	void	setHighlighted( BOOL b )		{ mHighlighted = b; }
	BOOL	getHighlighted() const			{ return mHighlighted; }

	void	setUserdata( void* userdata )	{ mUserdata = userdata; }
	void*	getUserdata() const 			{ return mUserdata; }

	virtual LLUUID	getUUID() const			{ return mItemValue.asUUID(); }
	LLSD	getValue() const				{ return mItemValue; }
	
	void	setRect(LLRect rect)			{ mRectangle = rect; }
	LLRect	getRect() const					{ return mRectangle; }

	void	addColumn( const LLScrollListCell::Params& p );

	void	setNumColumns(S32 columns);

	void	setColumn( S32 column, LLScrollListCell *cell );
	
	S32		getNumColumns() const;

	LLScrollListCell *getColumn(const S32 i) const;

	std::string getContentsCSV() const;

	virtual void draw(const LLRect& rect, const LLColor4& fg_color, const LLColor4& bg_color, const LLColor4& highlight_color, S32 column_padding);

protected:
	LLScrollListItem( const Params& );

private:
	BOOL	mSelected;
	BOOL	mHighlighted;
	BOOL	mEnabled;
	void*	mUserdata;
	LLSD	mItemValue;
	std::vector<LLScrollListCell *> mColumns;
	LLRect  mRectangle;
};

#endif
