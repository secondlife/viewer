/** 
 * @file llpanelgroupinvite.h
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLPANELGROUPINVITE_H
#define LL_LLPANELGROUPINVITE_H

#include "llpanel.h"
#include "lluuid.h"

class LLAvatarName;

class LLPanelGroupInvite
: public LLPanel
{
public:
	LLPanelGroupInvite(const LLUUID& group_id);
	~LLPanelGroupInvite();
	
	void addUsers(uuid_vec_t& agent_ids);
	/**
	 * this callback is being used to add a user whose fullname isn't been loaded before invoking of addUsers().
	 */  
	void addUserCallback(const LLUUID& id, const LLAvatarName& av_name);
	void clear();
	void update();

	void setCloseCallback(void (*close_callback)(void*), void* data);

	virtual void draw();
	virtual BOOL postBuild();
protected:
	class impl;
	impl* mImplementation;

	BOOL mPendingUpdate;
	LLUUID mStoreSelected;
	void updateLists();
};

#endif
