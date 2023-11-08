/** 
 * @file llscrollingpanellist.h
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLSCROLLINGPANELLIST_H
#define LL_LLSCROLLINGPANELLIST_H

#include <vector>

#include "llui.h"
#include "lluictrlfactory.h"
#include "llview.h"
#include "llpanel.h"

/*
 * Pure virtual class represents a scrolling panel.
 */
class LLScrollingPanel : public LLPanel
{
public:
	LLScrollingPanel(const LLPanel::Params& params) : LLPanel(params) {}
	virtual void updatePanel(BOOL allow_modify) = 0;
};


/*
 * A set of panels that are displayed in a sequence inside a scroll container.
 */
class LLScrollingPanelList : public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool> is_horizontal;
		Optional<S32> padding;
		Optional<S32> spacing;

		Params();
	};

	LLScrollingPanelList(const Params& p);
	
	static const S32 DEFAULT_SPACING = 6;
	static const S32 DEFAULT_PADDING = 2;

	typedef std::deque<LLScrollingPanel*>	panel_list_t;

	virtual void setValue(const LLSD& value) {};

	virtual void		draw();

	void				clearPanels();
	S32					addPanel(LLScrollingPanel* panel, bool back = false);
	void				removePanel(LLScrollingPanel* panel);
	void				removePanel(U32 panel_index);
	void				updatePanels(BOOL allow_modify);
	void				rearrange();

	const panel_list_t&	getPanelList() const { return mPanelList; }
	bool				getIsHorizontal() const { return mIsHorizontal; }
	void				setPadding(S32 padding) { mPadding = padding; rearrange(); }
	void				setSpacing(S32 spacing) { mSpacing = spacing; rearrange(); }
	S32					getPadding() const { return mPadding; }
	S32					getSpacing() const { return mSpacing; }

private:
	void				updatePanelVisiblilty();

	/**
	 * Notify parent about size change, makes sense when used inside accordion
	 */
	void				notifySizeChanged();

	bool				mIsHorizontal;
	S32					mPadding;
	S32					mSpacing;

	panel_list_t		mPanelList;
};

#endif //LL_LLSCROLLINGPANELLIST_H
