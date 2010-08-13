/** 
 * @file llfloaterchatterbox.h
 * @author Richard
 * @date 2007-05-04
 * @brief Integrated friends and group management/communication tool
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLFLOATERCHATTERBOX_H
#define LL_LLFLOATERCHATTERBOX_H

#include "llfloater.h"
#include "llmultifloater.h"
#include "llstring.h"
#include "llimpanel.h"

class LLTabContainer;

class LLFloaterChatterBox : public LLMultiFloater
{
public:
	LLFloaterChatterBox(const LLSD& seed);
	virtual ~LLFloaterChatterBox();
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ void removeFloater(LLFloater* floaterp);
	/*virtual*/ void addFloater(LLFloater* floaterp, 
								BOOL select_added_floater, 
								LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);

	static LLFloaterChatterBox* getInstance(); // *TODO:Skinning Deprecate
	static LLFloater* getCurrentVoiceFloater();
	
protected:
	void onVisibilityChange ( const LLSD& new_visibility );
	
	LLFloater* mActiveVoiceFloater;
};


class LLFloaterMyFriends : public LLFloater
{
public:
	LLFloaterMyFriends(const LLSD& seed);
	virtual ~LLFloaterMyFriends();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	static LLFloaterMyFriends* getInstance(); // *TODO:Skinning Deprecate
	
	static void* createFriendsPanel(void* data);
	static void* createGroupsPanel(void* data);
};

#endif // LL_LLFLOATERCHATTERBOX_H
