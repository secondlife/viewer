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
	/*virtual*/ void	onOpen(const LLSD& key);
	/*virtual*/ bool	notifyChildren(const LLSD& info);

	// internals
	class Updater;

private:

	typedef enum e_sort_oder {
		E_SORT_BY_NAME = 0,
		E_SORT_BY_STATUS = 1,
		E_SORT_BY_MOST_RECENT = 2,
		E_SORT_BY_DISTANCE = 3,
		E_SORT_BY_RECENT_SPEAKERS = 4,
	} ESortOrder;

	// methods indirectly called by the updaters
	void					updateFriendList();
	void					updateNearbyList();
	void					updateRecentList();

	bool					isFriendOnline(const LLUUID& id);
	bool					isItemsFreeOfFriends(const std::vector<LLUUID>& uuids);
	bool 					canCall();

	void					updateButtons();
	std::string				getActiveTabName() const;
	LLUUID					getCurrentItemID() const;
	void					getCurrentItemIDs(std::vector<LLUUID>& selected_uuids) const;
	void					buttonSetVisible(std::string btn_name, BOOL visible);
	void					buttonSetEnabled(const std::string& btn_name, bool enabled);
	void					buttonSetAction(const std::string& btn_name, const commit_signal_t::slot_type& cb);
	void					showGroupMenu(LLMenuGL* menu);
	void					setSortOrder(LLAvatarList* list, ESortOrder order, bool save = true);

	void					onVisibilityChange( const LLSD& new_visibility);

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
	void					onGroupCallButtonClicked();
	void					onTeleportButtonClicked();
	void					onShareButtonClicked();
	void					onMoreButtonClicked();
	void					onActivateButtonClicked();
	void					onRecentViewSortButtonClicked();
	void					onNearbyViewSortButtonClicked();
	void					onFriendsViewSortButtonClicked();
	void					onGroupsViewSortButtonClicked();
	void					onAvatarListDoubleClicked(LLUICtrl* ctrl);
	void					onAvatarListCommitted(LLAvatarList* list);
	void					onGroupPlusButtonClicked();
	void					onGroupMinusButtonClicked();
	void					onGroupPlusMenuItemClicked(const LLSD& userdata);

	void					onFriendsViewSortMenuItemClicked(const LLSD& userdata);
	void					onNearbyViewSortMenuItemClicked(const LLSD& userdata);
	void					onGroupsViewSortMenuItemClicked(const LLSD& userdata);
	void					onRecentViewSortMenuItemClicked(const LLSD& userdata);

	//returns false only if group is "none"
	bool					isRealGroup();
	bool					onFriendsViewSortMenuItemCheck(const LLSD& userdata);
	bool					onRecentViewSortMenuItemCheck(const LLSD& userdata);
	bool					onNearbyViewSortMenuItemCheck(const LLSD& userdata);

	// misc callbacks
	static void				onAvatarPicked(
								const std::vector<std::string>& names,
								const std::vector<LLUUID>& ids);

	void					onFriendsAccordionExpandedCollapsed(LLUICtrl* ctrl, const LLSD& param, LLAvatarList* avatar_list);

	void					showAccordion(const std::string name, bool show);

	void					showFriendsAccordionsIfNeeded();

	void					onFriendListRefreshComplete(LLUICtrl*ctrl, const LLSD& param);

	void					setAccordionCollapsedByUser(LLUICtrl* acc_tab, bool collapsed);
	void					setAccordionCollapsedByUser(const std::string& name, bool collapsed);
	bool					isAccordionCollapsedByUser(LLUICtrl* acc_tab);
	bool					isAccordionCollapsedByUser(const std::string& name);

	LLFilterEditor*			mFilterEditor;
	LLTabContainer*			mTabContainer;
	LLAvatarList*			mOnlineFriendList;
	LLAvatarList*			mAllFriendList;
	LLAvatarList*			mNearbyList;
	LLAvatarList*			mRecentList;
	LLGroupList*			mGroupList;

	LLHandle<LLView>		mGroupPlusMenuHandle;
	LLHandle<LLView>		mNearbyViewSortMenuHandle;
	LLHandle<LLView>		mFriendsViewSortMenuHandle;
	LLHandle<LLView>		mGroupsViewSortMenuHandle;
	LLHandle<LLView>		mRecentViewSortMenuHandle;

	Updater*				mFriendListUpdater;
	Updater*				mNearbyListUpdater;
	Updater*				mRecentListUpdater;

	std::string				mFilterSubString;
};

#endif //LL_LLPANELPEOPLE_H
