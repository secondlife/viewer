/** 
 * @file llgroupactions.cpp
 * @brief Group-related actions (join, leave, new, delete, etc)
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

#include "llgroupactions.h"

#include "llagent.h"
#include "llfloatergroupinfo.h"
#include "llfloaterreg.h"
#include "llimview.h" // for gIMMgr

// static
void LLGroupActions::search()
{
	LLFloaterReg::showInstance("search", LLSD().insert("panel", "group"));
}

// static
void LLGroupActions::create()
{
	LLFloaterGroupInfo::showCreateGroup(NULL);
}

// static
void LLGroupActions::leave(const LLUUID& group_id)
{
	if (group_id.isNull())
		return;

	S32 count = gAgent.mGroups.count();
	S32 i;
	for (i = 0; i < count; ++i)
	{
		if(gAgent.mGroups.get(i).mID == group_id)
			break;
	}
	if (i < count)
	{
		LLSD args;
		args["GROUP"] = gAgent.mGroups.get(i).mName;
		LLSD payload;
		payload["group_id"] = group_id;
		LLNotifications::instance().add("GroupLeaveConfirmMember", args, payload, onLeaveGroup);
	}
}

// static
void LLGroupActions::activate(const LLUUID& group_id)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ActivateGroup);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_GroupID, group_id);
	gAgent.sendReliableMessage();
}

// static
void LLGroupActions::info(const LLUUID& group_id)
{
	if (group_id.isNull())
		return;

	LLFloaterGroupInfo::showFromUUID(group_id);	
}

// static
void LLGroupActions::startChat(const LLUUID& group_id)
{
	if (group_id.isNull())
		return;

	LLGroupData group_data;
	if (gAgent.getGroupData(group_id, group_data))
	{
		gIMMgr->addSession(
			group_data.mName,
			IM_SESSION_GROUP_START,
			group_id);
		make_ui_sound("UISndStartIM");
	}
	else
	{
		// this should never happen, as starting a group IM session
		// relies on you belonging to the group and hence having the group data
		make_ui_sound("UISndInvalidOp");
	}
}

//-- Private methods ----------------------------------------------------------

// static
bool LLGroupActions::onLeaveGroup(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	LLUUID group_id = notification["payload"]["group_id"].asUUID();
	if(option == 0)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_LeaveGroupRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_GroupData);
		msg->addUUIDFast(_PREHASH_GroupID, group_id);
		gAgent.sendReliableMessage();
	}
	return false;
}
