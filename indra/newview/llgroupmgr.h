/** 
 * @file llgroupmgr.h
 * @brief Manager for aggregating all client knowledge for specific groups
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLGROUPMGR_H
#define LL_LLGROUPMGR_H

#include "lluuid.h"
#include "roles_constants.h"
#include <vector>
#include <string>
#include <map>

class LLMessageSystem;

class LLGroupMgrObserver
{
public:
	LLGroupMgrObserver(const LLUUID& id) : mID(id){};
	virtual ~LLGroupMgrObserver(){};
	virtual void changed(LLGroupChange gc) = 0;
	const LLUUID& getID() { return mID; }
protected:
	LLUUID	mID;
};

class LLGroupRoleData;

class LLGroupMemberData
{
friend class LLGroupMgrGroupData;

public:
	LLGroupMemberData(const LLUUID& id, 
						S32 contribution,
						U64 agent_powers,
						const std::string& title,
						const std::string& online_status,
						BOOL is_owner);

	~LLGroupMemberData();

	const LLUUID& getID() const { return mID; }
	S32 getContribution() const { return mContribution; }
	U64	getAgentPowers() const { return mAgentPowers; }
	BOOL isOwner() const { return mIsOwner; }
	const std::string& getTitle() const { return mTitle; }
	const std::string& getOnlineStatus() const { return mOnlineStatus; }
	void addRole(const LLUUID& role, LLGroupRoleData* rd);
	bool removeRole(const LLUUID& role);
	void clearRoles() { mRoles.clear(); };
	std::map<LLUUID,LLGroupRoleData*>::iterator roleBegin() { return mRoles.begin(); }
	std::map<LLUUID,LLGroupRoleData*>::iterator roleEnd() { return mRoles.end(); }

	BOOL isInRole(const LLUUID& role_id) { return (mRoles.find(role_id) != mRoles.end()); }

protected:
	LLUUID	mID;
	S32		mContribution;
	U64		mAgentPowers;
	std::string	mTitle;
	std::string	mOnlineStatus;
	BOOL	mIsOwner;
	std::map<LLUUID,LLGroupRoleData*> mRoles;
};

struct LLRoleData
{
	LLRoleData() : mRolePowers(0), mChangeType(RC_UPDATE_NONE) { }
	LLRoleData(const LLRoleData& rd)
	: 	mRoleName(rd.mRoleName),
		mRoleTitle(rd.mRoleTitle),
		mRoleDescription(rd.mRoleDescription),
		mRolePowers(rd.mRolePowers),
		mChangeType(rd.mChangeType) { }

	std::string mRoleName;
	std::string mRoleTitle;
	std::string mRoleDescription;
	U64	mRolePowers;
	LLRoleChangeType mChangeType;
};

class LLGroupRoleData
{
friend class LLGroupMgrGroupData;

public:
	LLGroupRoleData(const LLUUID& role_id, 
					const std::string& role_name,
					const std::string& role_title,
					const std::string& role_desc,
					const U64 role_powers,
					const S32 member_count);
	
	LLGroupRoleData(const LLUUID& role_id, 
					LLRoleData role_data,
					const S32 member_count);

	~LLGroupRoleData();

	const LLUUID& getID() const { return mRoleID; }

	const std::vector<LLUUID>& getRoleMembers() const { return mMemberIDs; }
	S32 getMembersInRole(std::vector<LLUUID> members, BOOL needs_sort = TRUE);
	S32 getTotalMembersInRole() { return mMemberIDs.size(); }

	LLRoleData getRoleData() const { return mRoleData; }
	void setRoleData(LLRoleData data) { mRoleData = data; }
	
	void addMember(const LLUUID& member);
	bool removeMember(const LLUUID& member);
	void clearMembers();

	const std::vector<LLUUID>::const_iterator getMembersBegin() const
	{ return mMemberIDs.begin(); }

	const std::vector<LLUUID>::const_iterator getMembersEnd() const
	{ return mMemberIDs.end(); }


protected:
	LLGroupRoleData()
	: mMemberCount(0), mMembersNeedsSort(FALSE) {}

	LLUUID mRoleID;
	LLRoleData	mRoleData;

	std::vector<LLUUID> mMemberIDs;
	S32	mMemberCount;

private:
	BOOL mMembersNeedsSort;
};

struct LLRoleMemberChange
{
	LLRoleMemberChange() : mChange(RMC_NONE) { }
	LLRoleMemberChange(const LLUUID& role, const LLUUID& member, LLRoleMemberChangeType change)
		: mRole(role), mMember(member), mChange(change) { }
	LLRoleMemberChange(const LLRoleMemberChange& rc)
		: mRole(rc.mRole), mMember(rc.mMember), mChange(rc.mChange) { }
	LLUUID mRole;
	LLUUID mMember;
	LLRoleMemberChangeType mChange;
};

typedef std::pair<LLUUID,LLUUID> lluuid_pair;

struct lluuid_pair_less
{
	bool operator()(const lluuid_pair& lhs, const lluuid_pair& rhs) const
	{
		if (lhs.first == rhs.first)
			return lhs.second < rhs.second;
		else
			return lhs.first < rhs.first;
	}
};

typedef std::map<lluuid_pair,LLRoleMemberChange,lluuid_pair_less> change_map;

struct LLGroupTitle
{
	std::string mTitle;
	LLUUID		mRoleID;
	BOOL		mSelected;
};

class LLGroupMgr;

class LLGroupMgrGroupData
{
friend class LLGroupMgr;

public:
	LLGroupMgrGroupData(const LLUUID& id);
	~LLGroupMgrGroupData();

	const LLUUID& getID() { return mID; }

	BOOL getRoleData(const LLUUID& role_id, LLRoleData& role_data);
	void setRoleData(const LLUUID& role_id, LLRoleData role_data);
	void createRole(const LLUUID& role_id, LLRoleData role_data);
	void deleteRole(const LLUUID& role_id);
	BOOL pendingRoleChanges();

	void addRolePower(const LLUUID& role_id, U64 power);
	void removeRolePower(const LLUUID& role_id, U64 power);
	U64  getRolePowers(const LLUUID& role_id);

	void removeData();
	void removeRoleData();
	void removeMemberData();
	void removeRoleMemberData();

	bool changeRoleMember(const LLUUID& role_id, const LLUUID& member_id, LLRoleMemberChangeType rmc);
	void recalcAllAgentPowers();
	void recalcAgentPowers(const LLUUID& agent_id);

	BOOL isMemberDataComplete() { return mMemberDataComplete; }
	BOOL isRoleDataComplete() { return mRoleDataComplete; }
	BOOL isRoleMemberDataComplete() { return mRoleMemberDataComplete; }
	BOOL isGroupPropertiesDataComplete() { return mGroupPropertiesDataComplete; }

public:
	typedef	std::map<LLUUID,LLGroupMemberData*> member_list;
	typedef member_list::iterator member_iter;
	typedef	std::map<LLUUID,LLGroupRoleData*> role_list;
	typedef role_list::iterator role_iter;

	member_list			mMembers;
	role_list			mRoles;

	
	change_map mRoleMemberChanges;
	std::map<LLUUID,LLRoleData> mRoleChanges;

	std::vector<LLGroupTitle> mTitles;

	LLUUID				mID;
	LLUUID				mOwnerRole;
	std::string			mName;
	std::string			mCharter;
	BOOL				mShowInList;
	LLUUID				mInsigniaID;
	LLUUID				mFounderID;
	BOOL				mOpenEnrollment;
	S32					mMembershipFee;
	BOOL				mAllowPublish;
	BOOL				mListInProfile;
	BOOL				mMaturePublish;
	BOOL				mChanged;
	S32					mMemberCount;
	S32					mRoleCount;

protected:
	void sendRoleChanges();
	void cancelRoleChanges();

private:
	LLUUID				mMemberRequestID;
	LLUUID				mRoleDataRequestID;
	LLUUID				mRoleMembersRequestID;
	LLUUID				mTitlesRequestID;
	U32					mReceivedRoleMemberPairs;

	BOOL				mMemberDataComplete;
	BOOL				mRoleDataComplete;
	BOOL				mRoleMemberDataComplete;
	BOOL				mGroupPropertiesDataComplete;

	BOOL				mPendingRoleMemberRequest;
};

struct LLRoleAction
{
	std::string mName;
	std::string mDescription;
	std::string mLongDescription;
	U64 mPowerBit;
};

struct LLRoleActionSet
{
	LLRoleActionSet();
	~LLRoleActionSet();
	LLRoleAction* mActionSetData;
	std::vector<LLRoleAction*> mActions;
};

class LLGroupMgr
{
public:
	LLGroupMgr();
	~LLGroupMgr();

	void addObserver(LLGroupMgrObserver* observer);
	void removeObserver(LLGroupMgrObserver* observer);
	LLGroupMgrGroupData* getGroupData(const LLUUID& id);

	void sendGroupPropertiesRequest(const LLUUID& group_id);
	void sendGroupRoleDataRequest(const LLUUID& group_id);
	void sendGroupRoleMembersRequest(const LLUUID& group_id);
	void sendGroupMembersRequest(const LLUUID& group_id);
	void sendGroupTitlesRequest(const LLUUID& group_id);
	void sendGroupTitleUpdate(const LLUUID& group_id, const LLUUID& title_role_id);
	void sendUpdateGroupInfo(const LLUUID& group_id);
	void sendGroupRoleMemberChanges(const LLUUID& group_id);
	void sendGroupRoleChanges(const LLUUID& group_id);

	static void sendCreateGroupRequest(const std::string& name,
									   const std::string& charter,
									   U8 show_in_list,
									   const LLUUID& insignia,
									   S32 membership_fee,
									   BOOL open_enrollment,
									   BOOL allow_publish,
									   BOOL mature_publish);

	static void sendGroupMemberJoin(const LLUUID& group_id);
	static void sendGroupMemberInvites(const LLUUID& group_id, std::map<LLUUID,LLUUID>& role_member_pairs);
	static void sendGroupMemberEjects(const LLUUID& group_id,
									  std::vector<LLUUID>& member_ids);

	void cancelGroupRoleChanges(const LLUUID& group_id);

	static void processGroupPropertiesReply(LLMessageSystem* msg, void** data);
	static void processGroupMembersReply(LLMessageSystem* msg, void** data);
	static void processGroupRoleDataReply(LLMessageSystem* msg, void** data);
	static void processGroupRoleMembersReply(LLMessageSystem* msg, void** data);
	static void processGroupTitlesReply(LLMessageSystem* msg, void** data);
	static void processCreateGroupReply(LLMessageSystem* msg, void** data);
	static void processJoinGroupReply(LLMessageSystem* msg, void ** data);
	static void processEjectGroupMemberReply(LLMessageSystem* msg, void ** data);
	static void processLeaveGroupReply(LLMessageSystem* msg, void ** data);

	static bool parseRoleActions(const LLString& xml_filename);

	std::vector<LLRoleActionSet*> mRoleActionSets;

	static void debugClearAllGroups(void*);
	void clearGroups();
	void clearGroupData(const LLUUID& group_id);
protected:
	void notifyObservers(LLGroupChange gc);
	void addGroup(LLGroupMgrGroupData* group_datap);
	LLGroupMgrGroupData* createGroupData(const LLUUID &id);

protected:
	typedef std::multimap<LLUUID,LLGroupMgrObserver*>::iterator observer_iter;
	std::multimap<LLUUID,LLGroupMgrObserver*> mObservers;
	typedef std::map<LLUUID, LLGroupMgrGroupData*>::iterator group_iter;
	std::map<LLUUID, LLGroupMgrGroupData*> mGroups;
};

extern LLGroupMgr* gGroupMgr;

#endif

