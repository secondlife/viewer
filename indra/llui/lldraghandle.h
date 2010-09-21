/** 
 * @file lldraghandle.h
 * @brief LLDragHandle base class
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

// A widget for dragging a view around the screen using the mouse.

#ifndef LL_DRAGHANDLE_H
#define LL_DRAGHANDLE_H

#include "llview.h"
#include "v4color.h"
#include "llrect.h"
#include "llcoord.h"

class LLTextBox;

class LLDragHandle : public LLView
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string> label;
		Optional<LLUIColor> drag_highlight_color;
		Optional<LLUIColor> drag_shadow_color;
		
		Params() 
		:	label("label"),	
			drag_highlight_color("drag_highlight_color", LLUIColorTable::instance().getColor("DefaultHighlightLight")),
			drag_shadow_color("drag_shadow_color", LLUIColorTable::instance().getColor("DefaultShadowDark"))
		{
			mouse_opaque(true);
			follows.flags(FOLLOWS_ALL);
		}
	};
	void initFromParams(const Params&);
	
	virtual ~LLDragHandle();

	virtual void setValue(const LLSD& value);

	void			setForeground(BOOL b)		{ mForeground = b; }
	BOOL			getForeground() const		{ return mForeground; }
	void			setMaxTitleWidth(S32 max_width) {mMaxTitleWidth = llmin(max_width, mMaxTitleWidth); }
	S32				getMaxTitleWidth() const { return mMaxTitleWidth; }
	void			setButtonsRect(const LLRect& rect){ mButtonsRect = rect; }
	LLRect			getButtonsRect() { return mButtonsRect; }
	void			setTitleVisible(BOOL visible);

	virtual void	setTitle( const std::string& title ) = 0;
	virtual std::string	getTitle() const = 0;

	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);

protected:
	LLDragHandle(const Params&);
	friend class LLUICtrlFactory;
	
protected:
	LLTextBox*		mTitleBox;
	
private:
	LLRect			mButtonsRect;
	S32				mDragLastScreenX;
	S32				mDragLastScreenY;
	S32				mLastMouseScreenX;
	S32				mLastMouseScreenY;
	LLCoordGL		mLastMouseDir;
	LLUIColor		mDragHighlightColor;
	LLUIColor		mDragShadowColor;
	S32				mMaxTitleWidth;
	BOOL			mForeground;

	// Pixels near the edge to snap floaters.
	static S32		sSnapMargin;
};


// Use this one for traditional top-of-window draggers
class LLDragHandleTop
: public LLDragHandle
{
protected:
	LLDragHandleTop(const Params& p) : LLDragHandle(p) {}
	friend class LLUICtrlFactory;
public:
	virtual void	setTitle( const std::string& title );
	virtual std::string getTitle() const;
	virtual void	draw();
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

private:
	void	reshapeTitleBox();
};


// Use this for left-side, vertical text draggers
class LLDragHandleLeft
: public LLDragHandle
{
protected:
	LLDragHandleLeft(const Params& p) : LLDragHandle(p) {}
	friend class LLUICtrlFactory;
public:
	virtual void	setTitle( const std::string& title );
	virtual std::string getTitle() const;
	virtual void	draw();
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

};

const S32 DRAG_HANDLE_HEIGHT = 16;
const S32 DRAG_HANDLE_WIDTH = 16;

#endif  // LL_DRAGHANDLE_H
