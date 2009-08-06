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
#include "llgroupmgr.h"
#include "llavataractions.h"
#include "llviewercontrol.h"

// LLGroupActions::teleport helper
//
// Method is offerTeleport should be called.
// First it checks, whether LLGroupMgr contains LLGroupMgrGroupData for this group already.
// If it's there, processMembersList can be called, which builds vector of ID's for online members and
// calls LLAvatarActions::offerTeleport.
// If LLGroupMgr doesn't contain LLGroupMgrGroupData, then ID of group should be saved in
// mID or queue, if mID is not empty. After that processQueue uses ID from mID or queue,
// registers LLGroupTeleporter as observer at LLGroupMgr and sends request for group members.
// LLGroupMgr notifies about response on this request by calling method 'changed'.
// It calls processMembersList, sets mID to null, to indicate that current group is processed,
// and calls processQueue to process remaining groups.
// The reason of calling of LLGroupMgr::addObserver and LLGroupMgr::removeObserver in
// processQueue and 'changed' methods is that LLGroupMgr notifies observers of only particular group,
// so, for each group mID should be updated and addObserver/removeObserver is called.

class LLGroupTeleporter : public LLGroupMgrObserver
{
public:
	LLGroupTeleporter() : LLGroupMgrObserver(LLUUID()) {}

	void offerTeleport(const LLUUID& group_id);

	// LLGroupMgrObserver trigger
	virtual void changed(LLGroupChange gc);
private:
	void processQueue();
	void processMembersList(LLGroupMgrGroupData* gdatap);

	std::queue<LLUUID> mGroupsQueue;
};

void LLGroupTeleporter::offerTeleport(const LLUUID& group_id)
{
	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(group_id);

	if (!gdatap || !gdatap->isMemberDataComplete())
	{
		if (mID.isNull())
			mID = group_id;
		else
			// Not null mID means that user requested next group teleport before
			// previous group is processed, so this group goes to queue
			mGroupsQueue.push(group_id);

		processQueue();
	}
	else
	{
		processMembersList(gdatap);
	}
}

// Sends request for group in mID or one group in queue
void LLGroupTeleporter::processQueue()
{
	// Get group from queue, if mID is empty
	if (mID.isNull() && !mGroupsQueue.empty())
	{
		mID = mGroupsQueue.front();
		mGroupsQueue.pop();
	}

	if (mID.notNull())
	{
		LLGroupMgr::getInstance()->addObserver(this);
		LLGroupMgr::getInstance()->sendGroupMembersRequest(mID);
	}
}

// Collects all online members of group and offers teleport to them
void LLGroupTeleporter::processMembersList(LLGroupMgrGroupData* gdatap)
{
	U32 limit = gSavedSettings.getU32("GroupTeleportMembersLimit");

	LLDynamicArray<LLUUID> ids;
	for (LLGroupMgrGroupData::member_list_t::iterator iter = gdatap->mMembers.begin(); iter != gdatap->mMembers.end(); iter++)
	{
		LLGroupMemberData* member = iter->second;
		if (!member)
			continue;

		if (member->getID() == gAgent.getID())
			// No need to teleport own avatar
			continue;

		if (member->getOnlineStatus() == "Online")
			ids.push_back(member->getID());

		if ((U32)ids.size() >= limit)
			break;
	}

	LLAvatarActions::offerTeleport(ids);
}

// LLGroupMgrObserver trigger
void LLGroupTeleporter::changed(LLGroupChange gc)
{
	if (gc == GC_MEMBER_DATA)
	{
		LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mID);

		if (gdatap && gdatap->isMemberDataComplete())
			processMembersList(gdatap);

		LLGroupMgr::getInstance()->removeObserver(this);

		// group in mID is processed
		mID.setNull();

		// process other groups in queue, if any
		processQueue();
	}
}

static LLGroupTeleporter sGroupTeleporter;

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

// static
void LLGroupActions::offerTeleport(const LLUUID& group_id)
{
	sGroupTeleporter.offerTeleport(group_id);
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
