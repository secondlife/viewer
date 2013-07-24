/** 
 * @file llgroupactions.h
 * @brief Group-related actions (join, leave, new, delete, etc)
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLGROUPACTIONS_H
#define LL_LLGROUPACTIONS_H

#include "llsd.h"
#include "lluuid.h"

/**
 * Group-related actions (join, leave, new, delete, etc)
 */
class LLGroupActions
{
public:
	/**
	 * Invokes group search floater.
	 */
	static void search();

	/// Join a group.  Assumes LLGroupMgr has data for that group already.
	static void join(const LLUUID& group_id);

	/**
	 * Invokes "Leave Group" floater.
	 */
	static void leave(const LLUUID& group_id);

	/**
	 * Activate group.
	 */
	static void activate(const LLUUID& group_id);

	/**
	 * Show group information panel.
	 */
	static void show(const LLUUID& group_id);

	/**
	 * Show group inspector floater.
	 */
	static void inspect(const LLUUID& group_id);

	/**
	 * Refresh group information panel.
	 */
	static void refresh(const LLUUID& group_id);

	/**
	 * Refresh group notices panel.
	 */
	static void refresh_notices();

	/**
	 * Refresh group information panel.
	 */
	static void createGroup();

	/**
	 * Close group information panel.
	 */
	static void closeGroup		(const LLUUID& group_id);

	/**
	 * Start group instant messaging session.
	 */
	static LLUUID startIM(const LLUUID& group_id);

	/**
	 * End group instant messaging session.
	 */
	static void endIM(const LLUUID& group_id);

	/// Returns if the current user is a member of the group
	static bool isInGroup(const LLUUID& group_id);

	/**
	 * Start a group voice call.
	 */
	static void startCall(const LLUUID& group_id);

	/**
	 * Returns true if avatar is in group.
	 *
	 * Note that data about group members is loaded from server.
	 * If data has not been loaded yet, function will return inaccurate result.
	 * See LLGroupMgr::sendGroupMembersRequest
	 */
	static bool isAvatarMemberOfGroup(const LLUUID& group_id, const LLUUID& avatar_id);
	
private:
	static bool onJoinGroup(const LLSD& notification, const LLSD& response);
	static bool onLeaveGroup(const LLSD& notification, const LLSD& response);
};

#endif // LL_LLGROUPACTIONS_H
