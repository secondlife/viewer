/** 
 * @file lllistcontextmenu.cpp
 * @brief Base class of misc lists' context menus
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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
