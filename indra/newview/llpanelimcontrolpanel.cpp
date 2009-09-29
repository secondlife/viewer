/** 
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llpanelimcontrolpanel.h"

#include "llavataractions.h"
#include "llavatariconctrl.h"
#include "llbutton.h"
#include "llgroupactions.h"

LLPanelIMControlPanel::LLPanelIMControlPanel()
{
}

LLPanelIMControlPanel::~LLPanelIMControlPanel()
{
}

BOOL LLPanelIMControlPanel::postBuild()
{
	childSetAction("view_profile_btn", boost::bind(&LLPanelIMControlPanel::onViewProfileButtonClicked, this));
	childSetAction("add_friend_btn", boost::bind(&LLPanelIMControlPanel::onAddFriendButtonClicked, this));
	childSetAction("call_btn", boost::bind(&LLPanelIMControlPanel::onCallButtonClicked, this));
	childSetAction("share_btn", boost::bind(&LLPanelIMControlPanel::onShareButtonClicked, this));
	childSetEnabled("add_friend_btn", !LLAvatarActions::isFriend(getChild<LLAvatarIconCtrl>("avatar_icon")->getAvatarId()));

	return TRUE;
}

void LLPanelIMControlPanel::onViewProfileButtonClicked()
{
	LLAvatarActions::showProfile(getChild<LLAvatarIconCtrl>("avatar_icon")->getAvatarId());
}

void LLPanelIMControlPanel::onAddFriendButtonClicked()
{
	LLAvatarIconCtrl* avatar_icon = getChild<LLAvatarIconCtrl>("avatar_icon");
	std::string full_name = avatar_icon->getFirstName() + " " + avatar_icon->getLastName();
	LLAvatarActions::requestFriendshipDialog(avatar_icon->getAvatarId(), full_name);
}

void LLPanelIMControlPanel::onCallButtonClicked()
{
	// *TODO: Implement
}

void LLPanelIMControlPanel::onShareButtonClicked()
{
	// *TODO: Implement
}

void LLPanelIMControlPanel::setID(const LLUUID& avatar_id)
{
	// Disable "Add friend" button for friends.
	childSetEnabled("add_friend_btn", !LLAvatarActions::isFriend(avatar_id));

	getChild<LLAvatarIconCtrl>("avatar_icon")->setValue(avatar_id);
}



BOOL LLPanelGroupControlPanel::postBuild()
{
	childSetAction("group_info_btn", boost::bind(&LLPanelGroupControlPanel::onGroupInfoButtonClicked, this));
	childSetAction("call_btn", boost::bind(&LLPanelGroupControlPanel::onCallButtonClicked, this));

	return TRUE;
}

void LLPanelGroupControlPanel::onGroupInfoButtonClicked()
{
	LLGroupActions::show(mGroupID);
}


void LLPanelGroupControlPanel::onCallButtonClicked()
{
	// *TODO: Implement
}


void LLPanelGroupControlPanel::setID(const LLUUID& id)
{
	mGroupID = id;
}
