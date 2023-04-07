/** 
 * @file llfloaterbump.h
 * @brief Floater showing recent bumps, hits with objects, pushes, etc.
 * @author Cory Ondrejka, James Cook
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLFLOATERBUMP_H
#define LL_LLFLOATERBUMP_H

#include "llfloater.h"
#include "llmenugl.h"

class LLMeanCollisionData;
class LLScrollListCtrl;

class LLFloaterBump 
: public LLFloater
{
	friend class LLFloaterReg;
protected:
	void add(LLScrollListCtrl* list, LLMeanCollisionData *mcd);
	void onScrollListRightClicked(LLUICtrl* ctrl, S32 x, S32 y);

public:
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	
	static LLFloaterBump* getInstance();

	void populateCollisionList();

	void startIM();
	void startCall();
	void reportAbuse();
	void showProfile();
	void addFriend();
	void inviteToGroup();
	bool enableAddFriend();
	void muteAvatar();
	void payAvatar();
	void zoomInAvatar();
	bool enableMute();

private:
	
	LLFloaterBump(const LLSD& key);
	virtual ~LLFloaterBump();

	LLScrollListCtrl* mList;
	LLHandle<LLContextMenu> mPopupMenuHandle;
	LLUUID mItemUUID;

	typedef std::map<LLUUID, std::string> uuid_map_t;
	uuid_map_t mNames;

};

#endif
