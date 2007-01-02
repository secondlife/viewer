/** 
 * @file llpanelgrouproles.h
 * @brief Panel for roles information about a particular group.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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

// Forward declare for friend usage.
//virtual BOOL LLPanelGroupSubTab::postBuildSubTab(LLView*);

typedef std::map<std::string,LLUUID> icon_map_t;

class LLPanelGroupRoles : public LLPanelGroupTab,
						  public LLPanelGroupTabObserver
{
public:
	LLPanelGroupRoles(const std::string& name, const LLUUID& group_id);
	virtual ~LLPanelGroupRoles();

	// Allow sub tabs to ask for sibling controls.
	friend class LLPanelGroupMembersSubTab;
	friend class LLPanelGroupRolesSubTab;
	friend class LLPanelGroupActionsSubTab;

	virtual BOOL postBuild();
	virtual BOOL isVisibleByAgent(LLAgent* agentp);

	static void* createTab(void* data);
	static void onClickSubTab(void*,bool);
	void handleClickSubTab();

	// Checks if the current tab needs to be applied, and tries to switch to the requested tab.
	BOOL attemptTransition();
	
	// Switches to the requested tab (will close() if requested is NULL)
	void transitionToTab();

	// Used by attemptTransition to query the user's response to a tab that needs to apply. 
	static void onNotifyCallback(S32 option, void* user_data);
	void handleNotifyCallback(S32 option);
	static void onModalClose(S32 option, void* user_data);

	// Most of these messages are just passed on to the current sub-tab.
	virtual LLString getHelpText() const;
	virtual void activate();
	virtual void deactivate();
	virtual bool needsApply(LLString& mesg);
	virtual BOOL hasModal();
	virtual bool apply(LLString& mesg);
	virtual void cancel();
	virtual void update(LLGroupChange gc);

	// PanelGroupTab observer trigger
	virtual void tabChanged();

protected:
	LLPanelGroupTab*		mCurrentTab;
	LLPanelGroupTab*		mRequestedTab;
	LLTabContainerCommon*	mSubTabContainer;
	BOOL					mFirstUse;
	BOOL					mIgnoreTransition;

	LLString				mDefaultNeedsApplyMesg;
	LLString				mWantApplyMesg;
};

class LLPanelGroupSubTab : public LLPanelGroupTab
{
public:
	LLPanelGroupSubTab(const std::string& name, const LLUUID& group_id);
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

	virtual void setSearchFilter( const LLString& filter );

	virtual void activate();
	virtual void deactivate();

	// Helper functions
	bool matchesActionSearchFilter(std::string action);
	void buildActionsList(LLScrollListCtrl* ctrl,
								 U64 allowed_by_some,
								 U64 allowed_by_all,
								 icon_map_t& icons,
								 void (*commit_callback)(LLUICtrl*,void*),
								 BOOL show_all,
								 BOOL filter,
								 BOOL is_owner_role);
	void buildActionCategory(LLScrollListCtrl* ctrl,
									U64 allowed_by_some,
									U64 allowed_by_all,
									LLRoleActionSet* action_set,
									icon_map_t& icons,
									void (*commit_callback)(LLUICtrl*,void*),
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

	LLString mSearchFilter;

	icon_map_t	mActionIcons;

	void setOthersVisible(BOOL b);
};

class LLPanelGroupMembersSubTab : public LLPanelGroupSubTab
{
public:
	LLPanelGroupMembersSubTab(const std::string& name, const LLUUID& group_id);
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
	static void addOwnerCB(S32 option, void* data);

	virtual void activate();
	virtual void deactivate();
	virtual void cancel();
	virtual bool needsApply(LLString& mesg);
	virtual bool apply(LLString& mesg);
	virtual void update(LLGroupChange gc);
	void updateMembers();

	virtual void draw();

protected:
	typedef std::map<LLUUID, LLRoleMemberChangeType> role_change_data_map_t;
	typedef std::map<LLUUID, role_change_data_map_t*>::iterator member_role_change_iter;
	typedef std::map<LLUUID, role_change_data_map_t*> member_role_changes_map_t;

	bool matchesSearchFilter(char* first, char* last);

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

	LLGroupMgrGroupData::member_iter mMemberProgress;
};

class LLPanelGroupRolesSubTab : public LLPanelGroupSubTab
{
public:
	LLPanelGroupRolesSubTab(const std::string& name, const LLUUID& group_id);
	virtual ~LLPanelGroupRolesSubTab();

	virtual BOOL postBuildSubTab(LLView* root);

	static void* createTab(void* data);

	virtual void activate();
	virtual void deactivate();
	virtual bool needsApply(LLString& mesg);
	virtual bool apply(LLString& mesg);
	virtual void cancel();
	bool matchesSearchFilter(std::string rolename, std::string roletitle);
	virtual void update(LLGroupChange gc);

	static void onRoleSelect(LLUICtrl*, void*);
	void handleRoleSelect();
	void buildMembersList();

	static void onActionCheck(LLUICtrl*, void*);
	void handleActionCheck(LLCheckBoxCtrl*, bool force=false);
	static void addActionCB(S32 option, void* data);

	static void onPropertiesKey(LLLineEditor*, void*);

	static void onDescriptionCommit(LLUICtrl*, void*);

	static void onMemberVisibilityChange(LLUICtrl*, void*);
	void handleMemberVisibilityChange(bool value);

	static void onCreateRole(void*);
	void handleCreateRole();

	static void onDeleteRole(void*);
	void handleDeleteRole();

	void saveRoleChanges();
protected:
	LLSD createRoleItem(const LLUUID& role_id, 
								 std::string name, 
								 std::string title, 
								 S32 members);

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
	LLPanelGroupActionsSubTab(const std::string& name, const LLUUID& group_id);
	virtual ~LLPanelGroupActionsSubTab();

	virtual BOOL postBuildSubTab(LLView* root);

	static void* createTab(void* data);

	virtual void activate();
	virtual void deactivate();
	virtual bool needsApply(LLString& mesg);
	virtual bool apply(LLString& mesg);
	virtual void update(LLGroupChange gc);

	static void onActionSelect(LLUICtrl*, void*);
	void handleActionSelect();
protected:
	LLScrollListCtrl*	mActionList;
	LLScrollListCtrl*	mActionRoles;
	LLNameListCtrl*		mActionMembers;

	LLTextEditor*	mActionDescription;
};


#endif // LL_LLPANELGROUPROLES_H
