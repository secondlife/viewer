/** 
 * @file llpanelgroupinvite.h
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLPANELGROUPINVITE_H
#define LL_LLPANELGROUPINVITE_H

#include "llpanel.h"
#include "lluuid.h"

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
	void addUserCallback(const LLUUID& id, const std::string& full_name);
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
