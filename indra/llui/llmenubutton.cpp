/** 
 * @file llbutton.cpp
 * @brief LLButton base class
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

#include "linden_common.h"

#include "llmenubutton.h"

// Linden library includes
#include "llmenugl.h"
#include "llstring.h"
#include "v4color.h"

static LLDefaultChildRegistry::Register<LLMenuButton> r("menu_button");


LLMenuButton::Params::Params()
:	menu_filename("menu_filename")
{
}


LLMenuButton::LLMenuButton(const LLMenuButton::Params& p)
:	LLButton(p),
	mMenu(NULL),
	mMenuVisibleLastFrame(false)
{
	std::string menu_filename = p.menu_filename;

	if (!menu_filename.empty())
	{
		mMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>(menu_filename, LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance());
		if (!mMenu)
		{
			llwarns << "Error loading menu_button menu" << llendl;
		}
	}
}

void LLMenuButton::toggleMenu()
{
    if(!mMenu)
		return;

	if (mMenu->getVisible() || mMenuVisibleLastFrame)
	{
		mMenu->setVisible(FALSE);
	}
	else
	{
	    LLRect rect = getRect();
		//mMenu->needsArrange(); //so it recalculates the visible elements
		LLMenuGL::showPopup(getParent(), mMenu, rect.mLeft, rect.mBottom);
	}
}


void LLMenuButton::hideMenu() 
{ 
	if(!mMenu)
		return;
	mMenu->setVisible(FALSE); 
}


BOOL LLMenuButton::handleKeyHere(KEY key, MASK mask )
{
	if( KEY_RETURN == key && mask == MASK_NONE && !gKeyboard->getKeyRepeated(key))
	{
		toggleMenu();
		return TRUE;
	}

	if (mMenu && mMenu->getVisible() && key == KEY_ESCAPE && mask == MASK_NONE)
	{
		mMenu->setVisible(FALSE);
		return TRUE;
	}
	
	return FALSE;
}

BOOL LLMenuButton::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (hasTabStop() && !getIsChrome())
	{
		setFocus(TRUE);
	}

	toggleMenu();
	
	if (getSoundFlags() & MOUSE_DOWN)
	{
		make_ui_sound("UISndClick");
	}

	return TRUE;
}

void LLMenuButton::draw()
{
	//we save this off so next frame when we try to close it by 
	//button click, and it hides menus before we get to it, we know
	mMenuVisibleLastFrame = mMenu && mMenu->getVisible();
	
	if (mMenuVisibleLastFrame)
	{
		setForcePressedState(true);
	}

	LLButton::draw();

	setForcePressedState(false);
}

