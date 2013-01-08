/** 
 * @file lltoggleablemenu.h
 * @brief Menu toggled by a button press
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLTOGGLEABLEMENU_H
#define LL_LLTOGGLEABLEMENU_H

#include "llmenugl.h"

class LLToggleableMenu : public LLMenuGL
{
public:
    //adding blank params to work around registration issue
    //where LLToggleableMenu was owning the LLMenuGL param
    //and menu.xml was never loaded
	struct Params : public LLInitParam::Block<Params, LLMenuGL::Params>
	{};
protected:
	LLToggleableMenu(const Params&);
	friend class LLUICtrlFactory;
public:
	~LLToggleableMenu();

	boost::signals2::connection setVisibilityChangeCallback( const commit_signal_t::slot_type& cb );

	virtual void handleVisibilityChange (BOOL curVisibilityIn);

	virtual bool addChild (LLView* view, S32 tab_group = 0);

	const LLRect& getButtonRect() const { return mButtonRect; }

	// Converts the given local button rect to a screen rect
	void setButtonRect(const LLRect& rect, LLView* current_view);
	void setButtonRect(LLView* current_view);

	// Returns "true" if menu was not closed by button click
	// and is not still visible. If menu is visible toggles
	// its visibility off.
	bool toggleVisibility();
	
protected:
	bool mClosedByButtonClick;
	LLRect mButtonRect;
	commit_signal_t*	mVisibilityChangeSignal;
};

#endif // LL_LLTOGGLEABLEMENU_H
