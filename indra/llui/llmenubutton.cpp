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

void LLMenuButton::MenuPositions::declareValues()
{
	declare("topleft", MP_TOP_LEFT);
	declare("topright", MP_TOP_RIGHT);
	declare("bottomleft", MP_BOTTOM_LEFT);
}

LLMenuButton::Params::Params()
:	menu_filename("menu_filename"),
	position("menu_position", MP_BOTTOM_LEFT)
{
	addSynonym(position, "position");
}


LLMenuButton::LLMenuButton(const LLMenuButton::Params& p)
:	LLButton(p),
	mIsMenuShown(false),
	mMenuPosition(p.position),
	mOwnMenu(false)
{
	std::string menu_filename = p.menu_filename;

	setMenu(menu_filename, mMenuPosition);
	updateMenuOrigin();
}

LLMenuButton::~LLMenuButton()
{
	cleanup();
}

boost::signals2::connection LLMenuButton::setMouseDownCallback( const mouse_signal_t::slot_type& cb )
{
	return LLUICtrl::setMouseDownCallback(cb);
}

void LLMenuButton::hideMenu()
{
	LLToggleableMenu* menu = getMenu();
	if (menu)
	{
		menu->setVisible(FALSE);
	}
}

LLToggleableMenu* LLMenuButton::getMenu()
{
	return dynamic_cast<LLToggleableMenu*>(mMenuHandle.get());
}

void LLMenuButton::setMenu(const std::string& menu_filename, EMenuPosition position /*MP_TOP_LEFT*/)
{
	if (menu_filename.empty())
	{
		return;
	}

	LLToggleableMenu* menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(menu_filename, LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance());
	if (!menu)
	{
		llwarns << "Error loading menu_button menu" << llendl;
		return;
	}

	setMenu(menu, position, true);
}

void LLMenuButton::setMenu(LLToggleableMenu* menu, EMenuPosition position /*MP_TOP_LEFT*/, bool take_ownership /*false*/)
{
	if (!menu) return;

	cleanup(); // destroy the previous memnu if we own it

	mMenuHandle = menu->getHandle();
	mMenuPosition = position;
	mOwnMenu = take_ownership;

	menu->setVisibilityChangeCallback(boost::bind(&LLMenuButton::onMenuVisibilityChange, this, _2));
}

BOOL LLMenuButton::handleKeyHere(KEY key, MASK mask )
{
	if (!getMenu()) return FALSE;

	if( KEY_RETURN == key && mask == MASK_NONE && !gKeyboard->getKeyRepeated(key))
	{
		// *HACK: We emit the mouse down signal to fire the callback bound to the
		// menu emerging event before actually displaying the menu. See STORM-263.
		LLUICtrl::handleMouseDown(-1, -1, MASK_NONE);

		toggleMenu();
		return TRUE;
	}

	LLToggleableMenu* menu = getMenu();
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
	if (mValidateSignal && !(*mValidateSignal)(this, LLSD()))
	{
		return;
	}

	LLToggleableMenu* menu = getMenu();
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
	LLToggleableMenu* menu = getMenu();
	if (!menu) return;

	LLRect rect = getRect();

	switch (mMenuPosition)
	{
		case MP_TOP_LEFT:
		{
			mX = rect.mLeft;
			mY = rect.mTop + menu->getRect().getHeight();
			break;
		}
		case MP_TOP_RIGHT:
		{
			const LLRect& menu_rect = menu->getRect();
			mX = rect.mRight - menu_rect.getWidth();
			mY = rect.mTop + menu_rect.getHeight();
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

void LLMenuButton::cleanup()
{
	if (mMenuHandle.get() && mOwnMenu)
	{
		mMenuHandle.get()->die();
	}
}
