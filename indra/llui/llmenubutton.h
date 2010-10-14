/** 
 * @file llbutton.h
 * @brief Header for buttons
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

#ifndef LL_LLMENUBUTTON_H
#define LL_LLMENUBUTTON_H

#include "llbutton.h"

class LLMenuGL;

class LLMenuButton
: public LLButton
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLButton::Params>
	{
		// filename for it's toggleable menu
		Optional<std::string>	menu_filename;
	
		Params();
	};	
	
	void toggleMenu();
	/*virtual*/ void draw();
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask );
	void hideMenu();
	LLMenuGL* getMenu() { return mMenu; }

protected:
	friend class LLUICtrlFactory;
	LLMenuButton(const Params&);

private:
	LLMenuGL*	mMenu;
	bool mMenuVisibleLastFrame;
};


#endif  // LL_LLMENUBUTTON_H
