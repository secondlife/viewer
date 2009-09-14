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
#include "llfloaterreg.h"
#include "llgroupmgr.h"
#include "llimview.h" // for gIMMgr
#include "llsidetray.h"

#include "llcommandhandler.h"

//
// Globals
//

class LLGroupHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLGroupHandler() : LLCommandHandler("group", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		if (tokens.size() < 1)
		{
			return false;
		}

		if (tokens[0].asString() == "create")
		{
			LLGroupActions::createGroup();
			return true;
		}

		if (tokens.size() < 2)
		{
			return false;
		}

		if (tokens[0].asString() == "list")
		{
			if (tokens[1].asString() == "show")
			{
				LLFloaterReg::showInstance("contacts", "groups");
				return true;
			}
            return false;
		}

		LLUUID group_id;
		if (!group_id.set(tokens[0], FALSE))
		{
			return false;
		}

		if (tokens[1].asString() == "about")
		{
			if (group_id.isNull())
				return true;

			LLGroupActions::show(group_id);

			return true;
		}
		return false;
	}
};
LLGroupHandler gGroupHandler;

// static
void LLGroupActions::search()
{
	LLFloaterReg::showInstance("search", LLSD().insert("panel", "group"));
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

bool	isGroupUIVisible()
{
	LLPanel* panel = LLSideTray::getInstance()->findChild<LLPanel>("panel_group_info_sidetray");
	if(!panel)
		return false;
	return panel->getVisible();
}

// static
void LLGroupActions::show(const LLUUID& group_id)
{
	if (group_id.isNull())
		return;

	LLSD params;
	params["group_id"] = group_id;
	params["open_tab_name"] = "panel_group_info_sidetray";

	LLSideTray::getInstance()->showPanel("panel_group_info_sidetray", params);
}

void LLGroupActions::refresh_notices()
{
	if(!isGroupUIVisible())
		return;

	LLSD params;
	params["group_id"] = LLUUID::null;
	params["open_tab_name"] = "panel_group_info_sidetray";
	params["action"] = "refresh_notices";

	LLSideTray::getInstance()->showPanel("panel_group_info_sidetray", params);
}

//static 
void LLGroupActions::refresh(const LLUUID& group_id)
{
	if(!isGroupUIVisible())
		return;

	LLSD params;
	params["group_id"] = group_id;
	params["open_tab_name"] = "panel_group_info_sidetray";
	params["action"] = "refresh";

	LLSideTray::getInstance()->showPanel("panel_group_info_sidetray", params);
}

//static 
void LLGroupActions::createGroup()
{
	LLSD params;
	params["group_id"] = LLUUID::null;
	params["open_tab_name"] = "panel_group_info_sidetray";
	params["action"] = "create";

	LLSideTray::getInstance()->showPanel("panel_group_info_sidetray", params);

}
//static
void LLGroupActions::closeGroup(const LLUUID& group_id)
{
	if(!isGroupUIVisible())
		return;

	LLSD params;
	params["group_id"] = group_id;
	params["open_tab_name"] = "panel_group_info_sidetray";
	params["action"] = "close";

	LLSideTray::getInstance()->showPanel("panel_group_info_sidetray", params);
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

// static
bool LLGroupActions::isAvatarMemberOfGroup(const LLUUID& group_id, const LLUUID& avatar_id)
{
	if(group_id.isNull() || avatar_id.isNull())
	{
		return false;
	}

	LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(group_id);
	if(!group_data)
	{
		return false;
	}

	if(group_data->mMembers.end() == group_data->mMembers.find(avatar_id))
	{
		return false;
	}

	return true;
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
