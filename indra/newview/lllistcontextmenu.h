/** 
 * @file lllistcontextmenu.h
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

#ifndef LL_LLLISTCONTEXTMENU_H
#define LL_LLLISTCONTEXTMENU_H

#include "llhandle.h"
#include "lluuid.h"
#include "llview.h"

class LLView;
class LLContextMenu;

/**
 * Context menu for single or multiple list items.
 * 
 * Derived classes must implement contextMenu().
 * 
 * Typical usage:
 * <code>
 * my_context_menu->show(parent_view, selected_list_items_ids, x, y);
 * </code>
 */
class LLListContextMenu
{
public:
	LLListContextMenu();
	virtual ~LLListContextMenu();

	/**
	 * Show the menu at specified coordinates.
	 *
	 * @param spawning_view View to spawn at.
	 * @param uuids An array of list items ids.
	 * @param x Horizontal coordinate in the spawn_view's coordinate frame.
	 * @param y Vertical coordinate in the spawn_view's coordinate frame.
	 */
	virtual void show(LLView* spawning_view, const uuid_vec_t& uuids, S32 x, S32 y);

	virtual void hide();

protected:
	typedef boost::function<void (const LLUUID& id)> functor_t;

	virtual LLContextMenu* createMenu() = 0;

	static LLContextMenu* createFromFile(const std::string& filename);
	static void handleMultiple(functor_t functor, const uuid_vec_t& ids);

	uuid_vec_t			mUUIDs;
	LLHandle<LLContextMenu>	mMenuHandle;
};

#endif // LL_LLLISTCONTEXTMENU_H
