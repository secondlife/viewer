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

class LLTabContainerCommon;

class LLFloaterMyFriends : public LLFloater, public LLUISingleton<LLFloaterMyFriends>
{
public:
	LLFloaterMyFriends(const LLSD& seed);
	virtual ~LLFloaterMyFriends();

	virtual BOOL postBuild();

	void onClose(bool app_quitting);

	// override LLUISingleton behavior
	static LLFloaterMyFriends* showInstance(const LLSD& id = LLSD());
	static void hideInstance(const LLSD& id);
	static BOOL instanceVisible(const LLSD& id);

	static void* createFriendsPanel(void* data);
	static void* createGroupsPanel(void* data);

protected:
	LLTabContainerCommon* mTabs;
};

class LLFloaterChatterBox : public LLMultiFloater, public LLUISingleton<LLFloaterChatterBox>
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
								LLTabContainerCommon::eInsertionPoint insertion_point = LLTabContainerCommon::END);

	static LLFloaterChatterBox* showInstance(const LLSD& seed = LLSD());
	static BOOL instanceVisible(const LLSD& seed);

	static LLFloater* getCurrentVoiceFloater();

protected:
	LLFloater* mActiveVoiceFloater;
};


#endif // LL_LLFLOATERCHATTERBOX_H
