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

void ContextMenu::show(LLView* spawning_view, const LLUUID& id, S32 x, S32 y)
{
	if (mMenu)
	{
		//preventing parent (menu holder) from deleting already "dead" context menus on exit
		LLView* parent = mMenu->getParent();
		if (parent)
		{
			parent->removeChild(mMenu);
			mMenu->setParent(NULL);
		}
		delete mMenu;
	}

	mID = id;
	mMenu = createMenu();
	mMenu->show(x, y);
	LLMenuGL::showPopup(spawning_view, mMenu, x, y);
}

//== NearbyMenu ===============================================================

LLContextMenu* NearbyMenu::createMenu()
{
	// set up the callbacks for all of the avatar menu items
	// (N.B. callbacks don't take const refs as mID is local scope)
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	registrar.add("Avatar.Profile",			boost::bind(&LLAvatarActions::showProfile,				mID));
	registrar.add("Avatar.AddFriend",		boost::bind(&LLAvatarActions::requestFriendshipDialog,	mID));
	registrar.add("Avatar.IM",				boost::bind(&LLAvatarActions::startIM,					mID));
	registrar.add("Avatar.Call",			boost::bind(&LLAvatarActions::startIM,					mID)); // *TODO: unimplemented
	registrar.add("Avatar.OfferTeleport",	boost::bind(&NearbyMenu::offerTeleport,					this));
	registrar.add("Avatar.ShowOnMap",		boost::bind(&LLAvatarActions::startIM,					mID)); // *TODO: unimplemented
	registrar.add("Avatar.Share",			boost::bind(&LLAvatarActions::startIM,					mID)); // *TODO: unimplemented
	registrar.add("Avatar.Pay",				boost::bind(&LLAvatarActions::pay,						mID));
	registrar.add("Avatar.BlockUnblock",	boost::bind(&LLAvatarActions::toggleBlock,				mID));

	enable_registrar.add("Avatar.EnableItem",	boost::bind(&NearbyMenu::enableContextMenuItem,	this, _2));
	enable_registrar.add("Avatar.CheckItem",	boost::bind(&NearbyMenu::checkContextMenuItem,	this, _2));

	// create the context menu from the XUI
	return LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
		"menu_people_nearby.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());
}

bool NearbyMenu::enableContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();

	if (item == std::string("can_block"))
	{
		std::string firstname, lastname;
		gCacheName->getName(mID, firstname, lastname);
		bool is_linden = !LLStringUtil::compareStrings(lastname, "Linden");
		bool is_self = mID == gAgentID;
		return !is_self && !is_linden;
	}
	else if (item == std::string("can_add"))
	{
		return !LLAvatarActions::isFriend(mID);
	}
	else if (item == std::string("can_delete"))
	{
		return LLAvatarActions::isFriend(mID);
	}

	return false;
}

bool NearbyMenu::checkContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();

	if (item == std::string("is_blocked"))
	{
		return LLAvatarActions::isBlocked(mID);
	}

	return false;
}

void NearbyMenu::offerTeleport()
{
	// boost::bind cannot recognize overloaded method LLAvatarActions::offerTeleport(),
	// so we have to use a wrapper.
	LLAvatarActions::offerTeleport(mID);
}

} // namespace LLPanelPeopleMenus
