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

#include "llviewerprecompiledheaders.h"

// libs
#include "llmenugl.h"
#include "lluictrlfactory.h"

#include "llpanelpeoplemenus.h"

// newview
#include "llagentdata.h"			// for gAgentID
#include "llavataractions.h"
#include "llviewermenu.h"			// for gMenuHolder

namespace LLPanelPeopleMenus
{

NearbyMenu gNearbyMenu;

//== ContextMenu ==============================================================

ContextMenu::ContextMenu()
:	mMenu(NULL)
{
}

ContextMenu::~ContextMenu()
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

void ContextMenu::show(LLView* spawning_view, const std::vector<LLUUID>& uuids, S32 x, S32 y)
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
		return;

	mUUIDs.resize(uuids.size());
	std::copy(uuids.begin(), uuids.end(), mUUIDs.begin());

	mMenu = createMenu();
	mMenuHandle = mMenu->getHandle();
	mMenu->show(x, y);
	LLMenuGL::showPopup(spawning_view, mMenu, x, y);
}

void ContextMenu::hide()
{
	if(mMenu)
	{
		mMenu->hide();
	}
}

//== NearbyMenu ===============================================================

LLContextMenu* NearbyMenu::createMenu()
{
	// set up the callbacks for all of the avatar menu items
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	if ( mUUIDs.size() == 1 )
	{
		// Set up for one person selected menu

		const LLUUID& id = mUUIDs.front();
		registrar.add("Avatar.Profile",			boost::bind(&LLAvatarActions::showProfile,				id));
		registrar.add("Avatar.AddFriend",		boost::bind(&LLAvatarActions::requestFriendshipDialog,	id));
		registrar.add("Avatar.IM",				boost::bind(&LLAvatarActions::startIM,					id));
		registrar.add("Avatar.Call",			boost::bind(&LLAvatarActions::startCall,				id));
		registrar.add("Avatar.OfferTeleport",	boost::bind(&NearbyMenu::offerTeleport,					this));
		registrar.add("Avatar.ShowOnMap",		boost::bind(&LLAvatarActions::startIM,					id));	// *TODO: unimplemented
		registrar.add("Avatar.Share",			boost::bind(&LLAvatarActions::share,					id));
		registrar.add("Avatar.Pay",				boost::bind(&LLAvatarActions::pay,						id));
		registrar.add("Avatar.BlockUnblock",	boost::bind(&LLAvatarActions::toggleBlock,				id));

		enable_registrar.add("Avatar.EnableItem", boost::bind(&NearbyMenu::enableContextMenuItem,	this, _2));
		enable_registrar.add("Avatar.CheckItem",  boost::bind(&NearbyMenu::checkContextMenuItem,	this, _2));

		// create the context menu from the XUI
		return LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
			"menu_people_nearby.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());
	}
	else
	{
		// Set up for multi-selected People

		// registrar.add("Avatar.AddFriend",	boost::bind(&LLAvatarActions::requestFriendshipDialog,	mUUIDs)); // *TODO: unimplemented
		registrar.add("Avatar.IM",			boost::bind(&LLAvatarActions::startConference,			mUUIDs));
		registrar.add("Avatar.Call",		boost::bind(&LLAvatarActions::startAdhocCall,			mUUIDs));
		// registrar.add("Avatar.Share",		boost::bind(&LLAvatarActions::startIM,					mUUIDs)); // *TODO: unimplemented
		// registrar.add("Avatar.Pay",		boost::bind(&LLAvatarActions::pay,						mUUIDs)); // *TODO: unimplemented
		enable_registrar.add("Avatar.EnableItem",	boost::bind(&NearbyMenu::enableContextMenuItem,	this, _2));

		// create the context menu from the XUI
		return LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>
			("menu_people_nearby_multiselect.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());

	}
}

bool NearbyMenu::enableContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();

	// Note: can_block and can_delete is used only for one person selected menu
	// so we don't need to go over all uuids.

	if (item == std::string("can_block"))
	{
		const LLUUID& id = mUUIDs.front();
		return LLAvatarActions::canBlock(id);
	}
	else if (item == std::string("can_add"))
	{
		// We can add friends if:
		// - there are selected people
		// - and there are no friends among selection yet.

		bool result = (mUUIDs.size() > 0);

		std::vector<LLUUID>::const_iterator
			id = mUUIDs.begin(),
			uuids_end = mUUIDs.end();

		for (;id != uuids_end; ++id)
		{
			if ( LLAvatarActions::isFriend(*id) )
			{
				result = false;
				break;
			}
		}

		return result;
	}
	else if (item == std::string("can_delete"))
	{
		const LLUUID& id = mUUIDs.front();
		return LLAvatarActions::isFriend(id);
	}
	else if (item == std::string("can_call"))
	{
		return LLAvatarActions::canCall();
	}
	return false;
}

bool NearbyMenu::checkContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();
	const LLUUID& id = mUUIDs.front();

	if (item == std::string("is_blocked"))
	{
		return LLAvatarActions::isBlocked(id);
	}

	return false;
}

void NearbyMenu::offerTeleport()
{
	// boost::bind cannot recognize overloaded method LLAvatarActions::offerTeleport(),
	// so we have to use a wrapper.
	const LLUUID& id = mUUIDs.front();
	LLAvatarActions::offerTeleport(id);
}

} // namespace LLPanelPeopleMenus
