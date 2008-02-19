/** 
 * @file llfloaterchatterbox.h
 * @author Richard
 * @date 2007-05-04
 * @brief Integrated friends and group management/communication tool
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLFLOATERCHATTERBOX_H
#define LL_LLFLOATERCHATTERBOX_H

#include "llfloater.h"
#include "llstring.h"
#include "llimview.h"
#include "llimpanel.h"

class LLTabContainer;

class LLFloaterChatterBox : public LLMultiFloater, public LLUISingleton<LLFloaterChatterBox, LLFloaterChatterBox>
{
public:
	LLFloaterChatterBox(const LLSD& seed);
	virtual ~LLFloaterChatterBox();

	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/ void draw();
	/*virtual*/ void onOpen();
	/*virtual*/ void onClose(bool app_quitting);

	/*virtual*/ void removeFloater(LLFloater* floaterp);
	/*virtual*/ void addFloater(LLFloater* floaterp, 
								BOOL select_added_floater, 
								LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);

	static LLFloater* getCurrentVoiceFloater();
	
	// visibility policy for LLUISingleton
	static bool visible(LLFloater* instance, const LLSD& key)
	{
		LLFloater* floater_to_check = ((LLFloaterChatterBox*)instance)->getFloater(key);

		if (floater_to_check)
		{
			return floater_to_check->isInVisibleChain();
		}

		// otherwise use default visibility rule for chatterbox
		return VisibilityPolicy<LLFloater>::visible(instance, key);
	}

	static void show(LLFloater* instance, const LLSD& key)
	{
		LLFloater* floater_to_show = ((LLFloaterChatterBox*)instance)->getFloater(key);
		VisibilityPolicy<LLFloater>::show(instance, key);

		if (floater_to_show)
		{
			floater_to_show->open();
		}
	}

	static void hide(LLFloater* instance, const LLSD& key)
	{
		VisibilityPolicy<LLFloater>::hide(instance, key);
	}

private:
	LLFloater* getFloater(const LLSD& key)
	{
		LLFloater* floater = NULL;

		//try to show requested session
		LLUUID session_id = key.asUUID();
		if (session_id.notNull())
		{
			floater = LLIMMgr::getInstance()->findFloaterBySession(session_id);
		}

		// if TRUE, show tab for active voice channel, otherwise, just show last tab
		if (key.asBoolean())
		{
			floater = getCurrentVoiceFloater();
		}

		return floater;
	}

protected:
	LLFloater* mActiveVoiceFloater;
};


class LLFloaterMyFriends : public LLFloater, public LLUISingleton<LLFloaterMyFriends, LLFloaterMyFriends>
{
public:
	LLFloaterMyFriends(const LLSD& seed);
	virtual ~LLFloaterMyFriends();

	virtual BOOL postBuild();

	void onClose(bool app_quitting);

	static void* createFriendsPanel(void* data);
	static void* createGroupsPanel(void* data);

	// visibility policy for LLUISingleton
	static bool visible(LLFloater* instance, const LLSD& key)
	{
		LLFloaterMyFriends* floaterp = (LLFloaterMyFriends*)instance;
		return floaterp->isInVisibleChain() && floaterp->mTabs->getCurrentPanelIndex() == key.asInteger();
	}

	static void show(LLFloater* instance, const LLSD& key)
	{
		VisibilityPolicy<LLFloater>::show(instance, key);
		// garbage values in id will be interpreted as 0, or the friends tab
		((LLFloaterMyFriends*)instance)->mTabs->selectTab(key);
	}

	static void hide(LLFloater* instance, const LLSD& key)
	{
		if (visible(instance, key))
		{
			LLFloaterChatterBox::hideInstance();
		}
	}

protected:
	LLTabContainer* mTabs;
};

#endif // LL_LLFLOATERCHATTERBOX_H
