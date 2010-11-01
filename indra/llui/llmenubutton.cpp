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
#include "lltoggleablemenu.h"
#include "llstring.h"
#include "v4color.h"

static LLDefaultChildRegistry::Register<LLMenuButton> r("menu_button");


LLMenuButton::Params::Params()
:	menu_filename("menu_filename")
{
}


LLMenuButton::LLMenuButton(const LLMenuButton::Params& p)
:	LLButton(p),
	mIsMenuShown(false),
	mMenuPosition(MP_BOTTOM_LEFT)
{
	std::string menu_filename = p.menu_filename;

	if (!menu_filename.empty())
	{
		LLToggleableMenu* menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(menu_filename, LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance());
		if (!menu)
		{
			llwarns << "Error loading menu_button menu" << llendl;
			return;
		}

		menu->setVisibilityChangeCallback(boost::bind(&LLMenuButton::onMenuVisibilityChange, this, _2));

		mMenuHandle = menu->getHandle();

		updateMenuOrigin();
	}
}

boost::signals2::connection LLMenuButton::setMouseDownCallback( const mouse_signal_t::slot_type& cb )
{
	return LLUICtrl::setMouseDownCallback(cb);
}

void LLMenuButton::hideMenu()
{
	if(mMenuHandle.isDead()) return;

	LLToggleableMenu* menu = dynamic_cast<LLToggleableMenu*>(mMenuHandle.get());
	if (menu)
	{
		menu->setVisible(FALSE);
	}
}

LLToggleableMenu* LLMenuButton::getMenu()
{
	return dynamic_cast<LLToggleableMenu*>(mMenuHandle.get());
}

void LLMenuButton::setMenu(LLToggleableMenu* menu, EMenuPosition position /*MP_TOP_LEFT*/)
{
	if (!menu) return;

	mMenuHandle = menu->getHandle();
	mMenuPosition = position;

	menu->setVisibilityChangeCallback(boost::bind(&LLMenuButton::onMenuVisibilityChange, this, _2));
}

BOOL LLMenuButton::handleKeyHere(KEY key, MASK mask )
{
	if (mMenuHandle.isDead()) return FALSE;

	if( KEY_RETURN == key && mask == MASK_NONE && !gKeyboard->getKeyRepeated(key))
	{
		// *HACK: We emit the mouse down signal to fire the callback bound to the
		// menu emerging event before actually displaying the menu. See STORM-263.
		LLUICtrl::handleMouseDown(-1, -1, MASK_NONE);

		toggleMenu();
		return TRUE;
	}

	LLToggleableMenu* menu = dynamic_cast<LLToggleableMenu*>(mMenuHandle.get());
	if (menu && menu->getVisible() && key == KEY_ESCAPE && mask == MASK_NONE)
	{
		menu->setVisible(FALSE);
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
	if(mMenuHandle.isDead()) return;

	LLToggleableMenu* menu = dynamic_cast<LLToggleableMenu*>(mMenuHandle.get());
	if (!menu) return;

	// Store the button rectangle to toggle menu visibility if a mouse event
	// occurred inside or outside the button rect.
	menu->setButtonRect(this);

	if (!menu->toggleVisibility() && mIsMenuShown)
	{
		setForcePressedState(false);
		mIsMenuShown = false;
	}
	else
	{
		menu->buildDrawLabels();
		menu->arrangeAndClear();
		menu->updateParent(LLMenuGL::sMenuContainer);

		updateMenuOrigin();

		LLMenuGL::showPopup(getParent(), menu, mX, mY);

		setForcePressedState(true);
		mIsMenuShown = true;
	}
}

void LLMenuButton::updateMenuOrigin()
{
	if (mMenuHandle.isDead()) return;

	LLRect rect = getRect();

	switch (mMenuPosition)
	{
		case MP_TOP_LEFT:
		{
			mX = rect.mLeft;
			mY = rect.mTop + mMenuHandle.get()->getRect().getHeight();
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

void LLMenuButton::onMenuVisibilityChange(const LLSD& param)
{
	bool new_visibility = param["visibility"].asBoolean();
	bool is_closed_by_button_click = param["closed_by_button_click"].asBoolean();

	// Reset the button "pressed" state only if the menu is shown by this particular
	// menu button (not any other control) and is not being closed by a click on the button.
	if (!new_visibility && !is_closed_by_button_click && mIsMenuShown)
	{
		setForcePressedState(false);
		mIsMenuShown = false;
	}
}
