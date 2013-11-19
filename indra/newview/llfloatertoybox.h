/** 
 * @file llfloatertoybox.h
 * @brief The toybox for flexibilizing the UI.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLFLOATERTOYBOX_H
#define LL_LLFLOATERTOYBOX_H

#include "llfloater.h"


class LLButton;
class LLToolBar;


class LLFloaterToybox : public LLFloater
{
public:
	LLFloaterToybox(const LLSD& key);
	virtual ~LLFloaterToybox();

	// virtuals
	BOOL postBuild();
	void draw();
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);

protected:
	void onBtnClearAll();
	void onBtnRestoreDefaults();

	void onToolBarButtonEnter(LLView* button);

public:
	LLToolBar *	mToolBar;
};

#endif // LL_LLFLOATERTOYBOX_H
