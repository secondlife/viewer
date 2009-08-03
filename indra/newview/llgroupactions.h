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

	/**
	 * Invokes group creation floater.
	 */
	static void create();

	/**
	 * Invokes "Leave Group" floater.
	 */
	static void leave(const LLUUID& group_id);

	/**
	 * Activate group.
	 */
	static void activate(const LLUUID& group_id);

	/**
	 * Show group information dialog.
	 */
	static void info(const LLUUID& group_id);

	/**
	 * Start group instant messaging session.
	 */
	static void startChat(const LLUUID& group_id);

	/**
	 * Offers teleport for online members of group
	 */
	static void offerTeleport(const LLUUID& group_id);
	
private:
	static bool onLeaveGroup(const LLSD& notification, const LLSD& response);
};

#endif // LL_LLGROUPACTIONS_H
