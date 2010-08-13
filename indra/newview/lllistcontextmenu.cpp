/** 
 * @file lllistcontextmenu.cpp
 * @brief Base class of misc lists' context menus
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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


#include "llviewerprecompiledheaders.h"

#include "lllistcontextmenu.h"

// libs
#include "llmenugl.h" // for LLContextMenu

// newview
#include "llviewermenu.h" // for LLViewerMenuHolderGL

LLListContextMenu::LLListContextMenu()
:	mMenu(NULL)
{
}

LLListContextMenu::~LLListContextMenu()
{
	// do not forget delete LLContextMenu* mMenu.
	// It can have registered Enable callbacks which are called from the LLMenuHolderGL::draw()
	// via selected item (menu_item_call) by calling LLMenuItemCallGL::buildDrawLabel.
	// we can have a crash via using callbacks of deleted instance of ContextMenu. EXT-4725

	// menu holder deletes its menus on viewer exit, so we have no way to determine if instance
	// of mMenu has already been deleted except of using LLHandle. EXT-4762.
	if (!mMenuHandle.isDead())
	{
		mMenu->die();
		mMenu = NULL;
	}
}

void LLListContextMenu::show(LLView* spawning_view, const uuid_vec_t& uuids, S32 x, S32 y)
{
	if (mMenu)
	{
		//preventing parent (menu holder) from deleting already "dead" context menus on exit
		LLView* parent = mMenu->getParent();
		if (parent)
		{
			parent->removeChild(mMenu);
		}
		delete mMenu;
		mMenu = NULL;
		mUUIDs.clear();
	}

	if ( uuids.empty() )
	{
		return;
	}

	mUUIDs.resize(uuids.size());
	std::copy(uuids.begin(), uuids.end(), mUUIDs.begin());

	mMenu = createMenu();
	if (!mMenu)
	{
		llwarns << "Context menu creation failed" << llendl;
		return;
	}

	mMenuHandle = mMenu->getHandle();
	mMenu->show(x, y);
	LLMenuGL::showPopup(spawning_view, mMenu, x, y);
}

void LLListContextMenu::hide()
{
	if(mMenu)
	{
		mMenu->hide();
	}
}

// static
void LLListContextMenu::handleMultiple(functor_t functor, const uuid_vec_t& ids)
{
	uuid_vec_t::const_iterator it;
	for (it = ids.begin(); it != ids.end(); ++it)
	{
		functor(*it);
	}
}

// static
LLContextMenu* LLListContextMenu::createFromFile(const std::string& filename)
{
	return LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
		filename, LLContextMenu::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());
}

// EOF
