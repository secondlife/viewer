/** 
 * @file llgroupactions.h
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
	static void startIM(const LLUUID& group_id);

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
