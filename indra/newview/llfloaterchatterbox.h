/** 
 * @file llfloaterchatterbox.h
 * @author Richard
 * @date 2007-05-04
 * @brief Integrated friends and group management/communication tool
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	static LLFloaterMyFriends* showInstance(const LLSD& id);
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

	static LLFloaterChatterBox* showInstance(const LLSD& seed);
	static BOOL instanceVisible(const LLSD& seed);

	static LLFloater* getCurrentVoiceFloater();

protected:
	LLFloater* mActiveVoiceFloater;
};


#endif // LL_LLFLOATERCHATTERBOX_H
