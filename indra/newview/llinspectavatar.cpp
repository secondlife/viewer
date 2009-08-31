/** 
 * @file llinspectavatar.cpp
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

#include "llinspectavatar.h"

// viewer files
#include "llagentdata.h"
#include "llavataractions.h"
#include "llcallingcard.h"

// linden libraries
#include "lluictrl.h"


LLInspectAvatar::LLInspectAvatar(const LLSD& avatar_id)
:	LLFloater(avatar_id),
	mAvatarID( avatar_id.asUUID() ),
	mFirstName(),
	mLastName()
{
}

LLInspectAvatar::~LLInspectAvatar()
{
}

/*virtual*/
BOOL LLInspectAvatar::postBuild(void)
{
	getChild<LLUICtrl>("add_friend_btn")->setCommitCallback(
		boost::bind(&LLInspectAvatar::onClickAddFriend, this) );

	getChild<LLUICtrl>("view_profile_btn")->setCommitCallback(
		boost::bind(&LLInspectAvatar::onClickViewProfile, this) );

	// can't call from constructor as widgets are not built yet
	refresh();

	return TRUE;
}

void LLInspectAvatar::setAvatarID(const LLUUID &avatar_id)
{
	mAvatarID = avatar_id;
	refresh();
}

void LLInspectAvatar::refresh()
{
	// *HACK: Don't stomp data when spawning from login screen
	if (mAvatarID.isNull()) return;

	// You can't re-add someone as a friend if they are already your friend
	bool is_friend = LLAvatarTracker::instance().getBuddyInfo(mAvatarID) != NULL;
	bool is_self = (mAvatarID == gAgentID);
	childSetEnabled("add_friend_btn", !is_friend && !is_self);

	// *TODO: replace with generic
	// LLAvatarPropertiesProcessor::getInstance()->addObserver()
	// ->sendDataRequest()
	childSetValue("avatar_icon", LLSD(mAvatarID) );

	gCacheName->get(mAvatarID, FALSE,
		boost::bind(&LLInspectAvatar::nameUpdatedCallback,
			this, _1, _2, _3, _4));
}

void LLInspectAvatar::nameUpdatedCallback(
	const LLUUID& id,
	const std::string& first,
	const std::string& last,
	BOOL is_group)
{
	// Possibly a request for an older inspector
	if (id != mAvatarID) return;

	mFirstName = first;
	mLastName = last;
	std::string name = first + " " + last;

	childSetValue("user_name", LLSD(name) );
}

void LLInspectAvatar::onClickAddFriend()
{
	std::string name;
	name.assign(getFirstName());
	name.append(" ");
	name.append(getLastName());

	LLAvatarActions::requestFriendshipDialog(getAvatarID(), name);
}

void LLInspectAvatar::onClickViewProfile()
{
	LLAvatarActions::showProfile(getAvatarID());
}
