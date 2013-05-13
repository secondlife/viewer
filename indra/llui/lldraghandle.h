/** 
 * @file lldraghandle.h
 * @brief LLDragHandle base class
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
			changeDefault(mouse_opaque, true);
			changeDefault(follows.flags, FOLLOWS_ALL);
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
