/** 
 * @file llpanelgrouproles.h
 * @brief Panel for roles information about a particular group.
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

#ifndef LL_LLPANELGROUPROLES_H
#define LL_LLPANELGROUPROLES_H

#include "llpanelgroup.h"

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

class LLPanelGroupRoles : public LLPanelGroupTab,
						  public LLPanelGroupTabObserver
{
public:
	LLPanelGroupRoles(const LLUUID& group_id);
	virtual ~LLPanelGroupRoles();

	// Allow sub tabs to ask for sibling controls.
	friend class LLPanelGroupMembersSubTab;
	friend class LLPanelGroupRolesSubTab;
	friend class LLPanelGroupActionsSubTab;

	virtual BOOL postBuild();
	virtual BOOL isVisibleByAgent(LLAgent* agentp);

	static void* createTab(void* data);
	void handleClickSubTab();

	// Checks if the current tab needs to be applied, and tries to switch to the requested tab.
	BOOL attemptTransition();
	
	// Switches to the requested tab (will close() if requested is NULL)
	void transitionToTab();

	// Used by attemptTransition to query the user's response to a tab that needs to apply. 
	bool handleNotifyCallback(const LLSD& notification, const LLSD& response);
	bool onModalClose(const LLSD& notification, const LLSD& response);

	// Most of these messages are just passed on to the current sub-tab.
	virtual std::string getHelpText() const;
	virtual void activate();
	virtual void deactivate();
	virtual bool needsApply(std::string& mesg);
	virtual BOOL hasModal();
	virtual bool apply(std::string& mesg);
	virtual void cancel();
	virtual void update(LLGroupChange gc);

	// PanelGroupTab observer trigger
	virtual void tabChanged();

protected:
	LLPanelGroupTab*		mCurrentTab;
	LLPanelGroupTab*		mRequestedTab;
	LLTabContainer*	mSubTabContainer;
	BOOL					mFirstUse;
	BOOL					mIgnoreTransition;

	std::string				mDefaultNeedsApplyMesg;
	std::string				mWantApplyMesg;
};

class LLPanelGroupSubTab : public LLPanelGroupTab
{
public:
	LLPanelGroupSubTab(const LLUUID& group_id);
	virtual ~LLPanelGroupSubTab();

	virtual BOOL postBuild();

	// This allows sub-tabs to collect child widgets from a higher level in the view hierarchy.
	virtual BOOL postBuildSubTab(LLView* root) { return TRUE; }

	static void onSearchKeystroke(LLLineEditor* caller, void* user_data);
	void handleSearchKeystroke(LLLineEditor* caller);

	static void onClickSearch(void*);
	void handleClickSearch();
	static void onClickShowAll(void*);
	void handleClickShowAll();

	virtual void setSearchFilter( const std::string& filter );

	virtual void activate();
	virtual void deactivate();

	// Helper functions
	bool matchesActionSearchFilter(std::string action);
	void buildActionsList(LLScrollListCtrl* ctrl,
								 U64 allowed_by_some,
								 U64 allowed_by_all,
								 icon_map_t& icons,
						  		 LLUICtrl::commit_callback_t commit_callback,
								 BOOL show_all,
								 BOOL filter,
								 BOOL is_owner_role);
	void buildActionCategory(LLScrollListCtrl* ctrl,
									U64 allowed_by_some,
									U64 allowed_by_all,
									LLRoleActionSet* action_set,
									icon_map_t& icons,
									LLUICtrl::commit_callback_t commit_callback,
									BOOL show_all,
									BOOL filter,
									BOOL is_owner_role);

	void setFooterEnabled(BOOL enable);
protected:
	LLPanel* mHeader;
	LLPanel* mFooter;

	LLLineEditor*	mSearchLineEditor;
	LLButton*		mSearchButton;
	LLButton*		mShowAllButton;

	std::string mSearchFilter;

	icon_map_t	mActionIcons;

	void setOthersVisible(BOOL b);
};

class LLPanelGroupMembersSubTab : public LLPanelGroupSubTab
{
public:
	LLPanelGroupMembersSubTab(const LLUUID& group_id);
	virtual ~LLPanelGroupMembersSubTab();

	virtual BOOL postBuildSubTab(LLView* root);

	static void* createTab(void* data);

	static void onMemberSelect(LLUICtrl*, void*);
	void handleMemberSelect();

	static void onMemberDoubleClick(void*);
	void handleMemberDoubleClick();

	static void onInviteMember(void*);
	void handleInviteMember();

	static void onEjectMembers(void*);
	void handleEjectMembers();

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
	LLPanelGroupRolesSubTab(const LLUUID& group_id);
	virtual ~LLPanelGroupRolesSubTab();

	virtual BOOL postBuildSubTab(LLView* root);

	static void* createTab(void* data);

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

	static void onDescriptionCommit(LLUICtrl*, void*);
	static void onDescriptionFocus(LLFocusableElement*, void*);

	static void onMemberVisibilityChange(LLUICtrl*, void*);
	void handleMemberVisibilityChange(bool value);

	static void onCreateRole(void*);
	void handleCreateRole();

	static void onDeleteRole(void*);
	void handleDeleteRole();

	void saveRoleChanges();
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
	LLPanelGroupActionsSubTab(const LLUUID& group_id);
	virtual ~LLPanelGroupActionsSubTab();

	virtual BOOL postBuildSubTab(LLView* root);

	static void* createTab(void* data);

	virtual void activate();
	virtual void deactivate();
	virtual bool needsApply(std::string& mesg);
	virtual bool apply(std::string& mesg);
	virtual void update(LLGroupChange gc);

	void handleActionSelect();
protected:
	LLScrollListCtrl*	mActionList;
	LLScrollListCtrl*	mActionRoles;
	LLNameListCtrl*		mActionMembers;

	LLTextEditor*	mActionDescription;
};


#endif // LL_LLPANELGROUPROLES_H
