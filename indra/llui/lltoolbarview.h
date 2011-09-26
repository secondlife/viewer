/** 
 * @file lltoolbarview.h
 * @author Merov Linden
 * @brief User customizable toolbar class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLTOOLBARVIEW_H
#define LL_LLTOOLBARVIEW_H

#include "lluictrl.h"
#include "lltoolbar.h"

class LLUICtrlFactory;

// Parent of all LLToolBar

class LLToolBarView : public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params> {};
	
	virtual ~LLToolBarView();
	/*virtual*/ BOOL postBuild();

	virtual void draw();

	// valid children for LLToolBarView are stored in this registry
	typedef LLDefaultChildRegistry child_registry_t;

protected:
	friend class LLUICtrlFactory;
	LLToolBarView(const Params&);

	void initFromParams(const Params&);

private:
	LLToolBar*	mToolbarLeft;
	LLToolBar*	mToolbarRight;
	LLToolBar*	mToolbarBottom;
};

extern LLToolBarView* gToolBarView;

#endif  // LL_LLTOOLBARVIEW_H
