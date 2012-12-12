/** 
 * @file llpanelgrouproles.h
 * @brief Panel for roles information about a particular group.
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

#ifndef LL_LLPANELGROUPROLES_H
#define LL_LLPANELGROUPROLES_H

#include "llpanelgroup.h"

class LLFilterEditor;
class LLNameListCtrl;
class LLPanelGroupSubTab;
class LLPanelGroupMembersSubTab;
class LLPanelGroupRolesSubTab;
class LLPanelGroupActionsSubTab;
class LLScrollListCtrl;
class LLScrollListItem;
class LLTextEditor;

// Forward declare for friend usage.
//virtual BOOL LLPanelGroupSubTab::postBuildSubTab(LLView*);

typedef std::map<std::string,std::string> icon_map_t;

class LLPanelGroupRoles : public LLPanelGroupTab
{
public:
	LLPanelGroupRoles();
	virtual ~LLPanelGroupRoles();

	// Allow sub tabs to ask for sibling controls.
	friend class LLPanelGroupMembersSubTab;
	friend class LLPanelGroupRolesSubTab;
	friend class LLPanelGroupActionsSubTab;

	virtual BOOL postBuild();
	virtual BOOL isVisibleByAgent(LLAgent* agentp);

	
	bool handleSubTabSwitch(const LLSD& data);

	// Checks if the current tab needs to be applied, and tries to switch to the requested tab.
	BOOL attemptTransition();
	
	// Switches to the requested tab (will close() if requested is NULL)
	void transitionToTab();

	// Used by attemptTransition to query the user's response to a tab that needs to apply. 
	bool handleNotifyCallback(const LLSD& notification, const LLSD& response);
	bool onModalClose(const LLSD& notification, const LLSD& response);

	// Most of these messages are just passed on to the current sub-tab.
	virtual void activate();
	virtual void deactivate();
	virtual bool needsApply(std::string& mesg);
	virtual BOOL hasModal();
	virtual bool apply(std::string& mesg);
	virtual void cancel();
	virtual void update(LLGroupChange gc);

	virtual void setGroupID(const LLUUID& id);

protected:
	LLPanelGroupTab*		mCurrentTab;
	LLPanelGroupTab*		mRequestedTab;
	LLTabContainer*	mSubTabContainer;
	BOOL					mFirstUse;

	std::string				mDefaultNeedsApplyMesg;
	std::string				mWantApplyMesg;
};

class LLPanelGroupSubTab : public LLPanelGroupTab
{
public:
	LLPanelGroupSubTab();
	virtual ~LLPanelGroupSubTab();

	virtual BOOL postBuild();

	// This allows sub-tabs to collect child widgets from a higher level in the view hierarchy.
	virtual BOOL postBuildSubTab(LLView* root);

	virtual void setSearchFilter( const std::string& filter );

	virtual void activate();
	virtual void deactivate();

	// Helper functions
	bool matchesActionSearchFilter(std::string action);


	void setFooterEnabled(BOOL enable);

	virtual void setGroupID(const LLUUID& id);
protected:
	void buildActionsList(LLScrollListCtrl* ctrl,
								 U64 allowed_by_some,
								 U64 allowed_by_all,
						  		 LLUICtrl::commit_callback_t commit_callback,
								 BOOL show_all,
								 BOOL filter,
								 BOOL is_owner_role);
	void buildActionCategory(LLScrollListCtrl* ctrl,
									U64 allowed_by_some,
									U64 allowed_by_all,
									LLRoleActionSet* action_set,
									LLUICtrl::commit_callback_t commit_callback,
									BOOL show_all,
									BOOL filter,
									BOOL is_owner_role);

protected:
	LLPanel* mHeader;
	LLPanel* mFooter;

	LLFilterEditor*	mSearchEditor;

	std::string mSearchFilter;

	icon_map_t	mActionIcons;

	bool mActivated;

	void setOthersVisible(BOOL b);
};

class LLPanelGroupMembersSubTab : public LLPanelGroupSubTab
{
public:
	LLPanelGroupMembersSubTab();
	virtual ~LLPanelGroupMembersSubTab();

	virtual BOOL postBuildSubTab(LLView* root);

	static void onMemberSelect(LLUICtrl*, void*);
	void handleMemberSelect();

	static void onMemberDoubleClick(void*);
	void handleMemberDoubleClick();

	static void onInviteMember(void*);
	void handleInviteMember();

	static void onEjectMembers(void*);
	void handleEjectMembers();
	void sendEjectNotifications(const LLUUID& group_id, const uuid_vec_t& selected_members);

	static void onRoleCheck(LLUICtrl* check, void* user_data);
	void handleRoleCheck(const LLUUID& role_id,
						 LLRoleMemberChangeType type);

	void applyMemberChanges();
	bool addOwnerCB(const LLSD& notification, const LLSD& response);

	virtual void activate();
	virtual void deactivate();
	virtual void cancel();
	virtual bool needsApply(std::string& mesg);
	virtual bool apply(std::string& mesg);
	virtual void update(LLGroupChange gc);
	void updateMembers();

	virtual void draw();

	virtual void setGroupID(const LLUUID& id);

	void addMemberToList(LLGroupMemberData* data);
	void onNameCache(const LLUUID& update_id, LLGroupMemberData* member, const LLAvatarName& av_name);

protected:
	typedef std::map<LLUUID, LLRoleMemberChangeType> role_change_data_map_t;
	typedef std::map<LLUUID, role_change_data_map_t*> member_role_changes_map_t;

	bool matchesSearchFilter(const std::string& fullname);

	U64  getAgentPowersBasedOnRoleChanges(const LLUUID& agent_id);
	bool getRoleChangeType(const LLUUID& member_id,
						   const LLUUID& role_id,
						   LLRoleMemberChangeType& type);

	LLNameListCtrl*		mMembersList;
	LLScrollListCtrl*	mAssignedRolesList;
	LLScrollListCtrl*	mAllowedActionsList;
	LLButton*           mEjectBtn;

	BOOL mChanged;
	BOOL mPendingMemberUpdate;
	BOOL mHasMatch;

	member_role_changes_map_t mMemberRoleChangeData;
	U32 mNumOwnerAdditions;

	LLGroupMgrGroupData::member_list_t::iterator mMemberProgress;
};

class LLPanelGroupRolesSubTab : public LLPanelGroupSubTab
{
public:
	LLPanelGroupRolesSubTab();
	virtual ~LLPanelGroupRolesSubTab();

	virtual BOOL postBuildSubTab(LLView* root);

	virtual void activate();
	virtual void deactivate();
	virtual bool needsApply(std::string& mesg);
	virtual bool apply(std::string& mesg);
	virtual void cancel();
	bool matchesSearchFilter(std::string rolename, std::string roletitle);
	virtual void update(LLGroupChange gc);

	static void onRoleSelect(LLUICtrl*, void*);
	void handleRoleSelect();
	void buildMembersList();

	static void onActionCheck(LLUICtrl*, void*);
	bool addActionCB(const LLSD& notification, const LLSD& response, LLCheckBoxCtrl* check);

	static void onPropertiesKey(LLLineEditor*, void*);

	void onDescriptionKeyStroke(LLTextEditor* caller);

	static void onDescriptionCommit(LLUICtrl*, void*);

	static void onMemberVisibilityChange(LLUICtrl*, void*);
	void handleMemberVisibilityChange(bool value);

	static void onCreateRole(void*);
	void handleCreateRole();

	static void onDeleteRole(void*);
	void handleDeleteRole();

	void saveRoleChanges(bool select_saved_role);

	virtual void setGroupID(const LLUUID& id);
protected:
	void handleActionCheck(LLUICtrl* ctrl, bool force);
	LLSD createRoleItem(const LLUUID& role_id, std::string name, std::string title, S32 members);

	LLScrollListCtrl* mRolesList;
	LLNameListCtrl* mAssignedMembersList;
	LLScrollListCtrl* mAllowedActionsList;

	LLLineEditor* mRoleName;
	LLLineEditor* mRoleTitle;
	LLTextEditor* mRoleDescription;

	LLCheckBoxCtrl* mMemberVisibleCheck;
	LLButton*       mDeleteRoleButton;
	LLButton*       mCreateRoleButton;

	LLUUID	mSelectedRole;
	BOOL	mHasRoleChange;
	std::string mRemoveEveryoneTxt;
};

class LLPanelGroupActionsSubTab : public LLPanelGroupSubTab
{
public:
	LLPanelGroupActionsSubTab();
	virtual ~LLPanelGroupActionsSubTab();

	virtual BOOL postBuildSubTab(LLView* root);


	virtual void activate();
	virtual void deactivate();
	virtual bool needsApply(std::string& mesg);
	virtual bool apply(std::string& mesg);
	virtual void update(LLGroupChange gc);

	void handleActionSelect();

	virtual void setGroupID(const LLUUID& id);
protected:
	LLScrollListCtrl*	mActionList;
	LLScrollListCtrl*	mActionRoles;
	LLNameListCtrl*		mActionMembers;

	LLTextEditor*	mActionDescription;
};


#endif // LL_LLPANELGROUPROLES_H
