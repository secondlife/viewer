/** 
 * @file llbutton.cpp
 * @brief LLButton base class
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
		setForcePressedState(TRUE);
	}

	LLButton::draw();

	setForcePressedState(FALSE);
}

