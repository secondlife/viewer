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
	mMenuVisibleLastFrame(false),
	mMenuPosition(MP_BOTTOM_LEFT)
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

	updateMenuOrigin();
}

boost::signals2::connection LLMenuButton::setMouseDownCallback( const mouse_signal_t::slot_type& cb )
{
	return LLUICtrl::setMouseDownCallback(cb);
}

void LLMenuButton::hideMenu()
{
	if(!mMenu) return;
	mMenu->setVisible(FALSE);
}

void LLMenuButton::setMenu(LLMenuGL* menu, EMenuPosition position /*MP_TOP_LEFT*/)
{
	mMenu = menu;
	mMenuPosition = position;
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

BOOL LLMenuButton::handleKeyHere(KEY key, MASK mask )
{
	if( KEY_RETURN == key && mask == MASK_NONE && !gKeyboard->getKeyRepeated(key))
	{
		// *HACK: We emit the mouse down signal to fire the callback bound to the
		// menu emerging event before actually displaying the menu. See STORM-263.
		LLUICtrl::handleMouseDown(-1, -1, MASK_NONE);

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
	LLButton::handleMouseDown(x, y, mask);
	toggleMenu();
	
	return TRUE;
}

void LLMenuButton::toggleMenu()
{
    if(!mMenu) return;

	if (mMenu->getVisible() || mMenuVisibleLastFrame)
	{
		mMenu->setVisible(FALSE);
	}
	else
	{
		mMenu->buildDrawLabels();
		mMenu->arrangeAndClear();
		mMenu->updateParent(LLMenuGL::sMenuContainer);

		updateMenuOrigin();

	    //mMenu->needsArrange(); //so it recalculates the visible elements
		LLMenuGL::showPopup(getParent(), mMenu, mX, mY);
	}
}

void LLMenuButton::updateMenuOrigin()
{
	if (!mMenu)	return;

	LLRect rect = getRect();

	switch (mMenuPosition)
	{
		case MP_TOP_LEFT:
		{
			mX = rect.mLeft;
			mY = rect.mTop + mMenu->getRect().getHeight();
			break;
		}
		case MP_BOTTOM_LEFT:
		{
			mX = rect.mLeft;
			mY = rect.mBottom;
			break;
		}
	}
}
