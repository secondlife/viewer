/** 
 * @file llpanelpeoplemenus.h
 * @brief Menus used by the side tray "People" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLPANELPEOPLEMENUS_H
#define LL_LLPANELPEOPLEMENUS_H

#include "llavatarlistitem.h"

namespace LLPanelPeopleMenus
{

/**
 * Base context menu.
 */
class ContextMenu : public LLAvatarListItem::ContextMenu
{
public:
	ContextMenu();
	virtual ~ContextMenu();

	/**
	 * Show the menu at specified coordinates.
	 *
	 * @param  uuids - an array of avatar or group ids
	 */
	/*virtual*/ void show(LLView* spawning_view, const std::vector<LLUUID>& uuids, S32 x, S32 y);

	virtual void hide();

protected:

	virtual LLContextMenu* createMenu() = 0;

	std::vector<LLUUID>	mUUIDs;
	LLContextMenu*		mMenu;
	LLHandle<LLView>	mMenuHandle;
};

/**
 * Menu used in the nearby people list.
 */
class NearbyMenu : public ContextMenu
{
public:
	/*virtual*/ LLContextMenu* createMenu();
private:
	bool enableContextMenuItem(const LLSD& userdata);
	bool checkContextMenuItem(const LLSD& userdata);
	void offerTeleport();
};

extern NearbyMenu gNearbyMenu;

} // namespace LLPanelPeopleMenus

#endif // LL_LLPANELPEOPLEMENUS_H
