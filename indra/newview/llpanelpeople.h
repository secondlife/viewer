/** 
 * @file llpanelpeople.h
 * @brief Side tray "People" panel
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

#ifndef LL_LLPANELPEOPLE_H
#define LL_LLPANELPEOPLE_H

#include <llpanel.h>

#include "llcallingcard.h" // for avatar tracker

class LLFilterEditor;
class LLTabContainer;
class LLAvatarList;
class LLGroupList;

class LLPanelPeople : public LLPanel
{
	LOG_CLASS(LLPanelPeople);
public:
	LLPanelPeople();
	virtual ~LLPanelPeople();

	/*virtual*/ BOOL 	postBuild();

	virtual void	onOpen(const LLSD& key);

	// internals
	class Updater;

private:
	bool					updateFriendList(U32 changed_mask);
	bool					updateNearbyList();
	bool					updateRecentList();
	bool					updateGroupList();
	bool					filterFriendList();
	bool					filterNearbyList();
	bool					filterRecentList();
	void					updateButtons();
	LLAvatarList*			getActiveAvatarList() const;
	LLUUID					getCurrentItemID() const;
	void					buttonSetVisible(std::string btn_name, BOOL visible);
	void					buttonSetEnabled(const std::string& btn_name, bool enabled);
	void					buttonSetAction(const std::string& btn_name, const commit_signal_t::slot_type& cb);
	void					showGroupMenu(LLMenuGL* menu);

	/*virtual*/ void		onVisibilityChange(BOOL new_visibility);

	void					reSelectedCurrentTab();

	// UI callbacks
	void					onFilterEdit(const std::string& search_string);
	void					onTabSelected(const LLSD& param);
	void					onViewProfileButtonClicked();
	void					onAddFriendButtonClicked();
	void					onAddFriendWizButtonClicked();
	void					onDeleteFriendButtonClicked();
	void					onGroupInfoButtonClicked();
	void					onChatButtonClicked();
	void					onImButtonClicked();
	void					onCallButtonClicked();
	void					onTeleportButtonClicked();
	void					onShareButtonClicked();
	void					onMoreButtonClicked();
	void					onActivateButtonClicked();
	void					onAvatarListDoubleClicked(LLAvatarList* list);
	void					onAvatarListCommitted(LLAvatarList* list);
	void					onGroupPlusButtonClicked();
	void					onGroupMinusButtonClicked();
	void					onGroupPlusMenuItemClicked(const LLSD& userdata);

	// misc callbacks
	bool					onFriendListUpdate(U32 changed_mask);
	static void				onAvatarPicked(
								const std::vector<std::string>& names,
								const std::vector<LLUUID>& ids,
								void*);

	LLFilterEditor*			mFilterEditor;
	LLTabContainer*			mTabContainer;
	LLAvatarList*			mFriendList;
	LLAvatarList*			mNearbyList;
	LLAvatarList*			mRecentList;
	LLGroupList*			mGroupList;

	LLHandle<LLView>		mGroupPlusMenuHandle;

	Updater*				mFriendListUpdater;
	Updater*				mNearbyListUpdater;
	Updater*				mRecentListUpdater;
	Updater*				mGroupListUpdater;

	std::string				mFilterSubString;

	// The vectors below contain up-to date avatar lists
	// for the corresponding tabs.
	// When the user enters a filter, it gets applied
	// to all the vectors and the result is shown in the tabs.
	// We don't need to have such a vector for the groups tab
	// since re-fetching the groups list is always fast.
	typedef std::vector<LLUUID> uuid_vector_t;
	uuid_vector_t			mNearbyVec;
	uuid_vector_t			mFriendVec;
	uuid_vector_t			mRecentVec;
};

#endif //LL_LLPANELPEOPLE_H
