/** 
 * @file llgroupmgr.h
 * @brief Manager for aggregating all client knowledge for specific groups
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
	LLGroupMgrObserver() : mID(LLUUID::null){};
	virtual ~LLGroupMgrObserver(){};
	virtual void changed(LLGroupChange gc) = 0;
	const LLUUID& getID() { return mID; }
protected:
	LLUUID	mID;
};

class LLParticularGroupMgrObserver
{
public:
	virtual ~LLParticularGroupMgrObserver(){}
	virtual void changed(const LLUUID& group_id, LLGroupChange gc) = 0;
};

class LLGroupRoleData;

class LLGroupMemberData
{
friend class LLGroupMgrGroupData;

public:
	typedef std::map<LLUUID,LLGroupRoleData*> role_list_t;
	
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
	void clearRoles() { mRolesList.clear(); };
	role_list_t::iterator roleBegin() { return mRolesList.begin(); }
	role_list_t::iterator roleEnd() { return mRolesList.end(); }

	BOOL isInRole(const LLUUID& role_id) { return (mRolesList.find(role_id) != mRolesList.end()); }

protected:
	LLUUID	mID;
	S32		mContribution;
	U64		mAgentPowers;
	std::string	mTitle;
	std::string	mOnlineStatus;
	BOOL	mIsOwner;
	role_list_t mRolesList;
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
	typedef	std::map<LLUUID,LLGroupMemberData*> member_list_t;
	typedef	std::map<LLUUID,LLGroupRoleData*> role_list_t;
	typedef std::map<lluuid_pair,LLRoleMemberChange,lluuid_pair_less> change_map_t;
	typedef std::map<LLUUID,LLRoleData> role_data_map_t;
	member_list_t		mMembers;
	role_list_t			mRoles;

	
	change_map_t		mRoleMemberChanges;
	role_data_map_t		mRoleChanges;

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

class LLGroupMgr : public LLSingleton<LLGroupMgr>
{
	LOG_CLASS(LLGroupMgr);
	
public:
	LLGroupMgr();
	~LLGroupMgr();

	void addObserver(LLGroupMgrObserver* observer);
	void addObserver(const LLUUID& group_id, LLParticularGroupMgrObserver* observer);
	void removeObserver(LLGroupMgrObserver* observer);
	void removeObserver(const LLUUID& group_id, LLParticularGroupMgrObserver* observer);
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

	static bool parseRoleActions(const std::string& xml_filename);

	std::vector<LLRoleActionSet*> mRoleActionSets;

	static void debugClearAllGroups(void*);
	void clearGroups();
	void clearGroupData(const LLUUID& group_id);

private:
	void notifyObservers(LLGroupChange gc);
	void notifyObserver(const LLUUID& group_id, LLGroupChange gc);
	void addGroup(LLGroupMgrGroupData* group_datap);
	LLGroupMgrGroupData* createGroupData(const LLUUID &id);

	typedef std::multimap<LLUUID,LLGroupMgrObserver*> observer_multimap_t;
	observer_multimap_t mObservers;

	typedef std::map<LLUUID, LLGroupMgrGroupData*> group_map_t;
	group_map_t mGroups;

	typedef std::set<LLParticularGroupMgrObserver*> observer_set_t;
	typedef std::map<LLUUID,observer_set_t> observer_map_t;
	observer_map_t mParticularObservers;
};


#endif

