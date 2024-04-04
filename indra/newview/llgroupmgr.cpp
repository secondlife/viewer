/** 
 * @file llgroupmgr.cpp
 * @brief LLGroupMgr class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

/**
 * Manager for aggregating all client knowledge for specific groups
 * Keeps a cache of group information.
 */

#include "llviewerprecompiledheaders.h"

#include "llgroupmgr.h"

#include <vector>
#include <algorithm>

#include "llappviewer.h"
#include "llagent.h"
#include "llavatarnamecache.h"
#include "llui.h"
#include "message.h"
#include "roles_constants.h"
#include "lltransactiontypes.h"
#include "llstatusbar.h"
#include "llviewerwindow.h"
#include "llpanelgroupcreate.h"
#include "llgroupactions.h"
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"
#include "lltrans.h"
#include "llviewerregion.h"
#include <boost/regex.hpp>
#include "llcorehttputil.h"
#include "lluiusage.h"


#if LL_MSVC
#pragma warning(push)   
// disable boost::lexical_cast warning
#pragma warning (disable:4702)
#endif

#include <boost/lexical_cast.hpp>

#if LL_MSVC
#pragma warning(pop)   // Restore all warnings to the previous state
#endif

const U32 MAX_CACHED_GROUPS = 20;

//
// LLRoleActionSet
//
LLRoleActionSet::LLRoleActionSet()
: mActionSetData(NULL)
{ }

LLRoleActionSet::~LLRoleActionSet()
{
	delete mActionSetData;
	std::for_each(mActions.begin(), mActions.end(), DeletePointer());
	mActions.clear();
}

//
// LLGroupMemberData
//

LLGroupMemberData::LLGroupMemberData(const LLUUID& id, 
										S32 contribution,
										U64 agent_powers,
										const std::string& title,
										const std::string& online_status,
										bool is_owner) : 
	mID(id), 
	mContribution(contribution), 
	mAgentPowers(agent_powers), 
	mTitle(title), 
	mOnlineStatus(online_status),
	mIsOwner(is_owner)
{
}

LLGroupMemberData::~LLGroupMemberData()
{
}

void LLGroupMemberData::addRole(const LLUUID& role, LLGroupRoleData* rd)
{
	mRolesList[role] = rd;
}

bool LLGroupMemberData::removeRole(const LLUUID& role)
{
	role_list_t::iterator it = mRolesList.find(role);

	if (it != mRolesList.end())
	{
		mRolesList.erase(it);
		return true;
	}

	return false;
}

//
// LLGroupRoleData
//

LLGroupRoleData::LLGroupRoleData(const LLUUID& role_id, 
								const std::string& role_name,
								const std::string& role_title,
								const std::string& role_desc,
								const U64 role_powers,
								const S32 member_count) :
	mRoleID(role_id),
	mMemberCount(member_count),
	mMembersNeedsSort(false)
{
	mRoleData.mRoleName = role_name;
	mRoleData.mRoleTitle = role_title;
	mRoleData.mRoleDescription = role_desc;
	mRoleData.mRolePowers = role_powers;
	mRoleData.mChangeType = RC_UPDATE_NONE;
}

LLGroupRoleData::LLGroupRoleData(const LLUUID& role_id, 
								LLRoleData role_data,
								const S32 member_count) :
	mRoleID(role_id),
	mRoleData(role_data),
	mMemberCount(member_count),
	mMembersNeedsSort(false)
{

}

LLGroupRoleData::~LLGroupRoleData()
{	
}

S32 LLGroupRoleData::getMembersInRole(uuid_vec_t members,
									  bool needs_sort)
{
	if (mRoleID.isNull())
	{
		// This is the everyone role, just return the size of members, 
		// because everyone is in the everyone role.
		return members.size();
	}

	// Sort the members list, if needed.
	if (mMembersNeedsSort)
	{
		std::sort(mMemberIDs.begin(), mMemberIDs.end());
		mMembersNeedsSort = false;
	}
	if (needs_sort)
	{
		// Sort the members parameter.
		std::sort(members.begin(), members.end());
	}

	// Return the number of members in the intersection.
	S32 max_size = llmin( members.size(), mMemberIDs.size() );
	uuid_vec_t in_role( max_size );
	uuid_vec_t::iterator in_role_end;
	in_role_end = std::set_intersection(mMemberIDs.begin(), mMemberIDs.end(),
									members.begin(), members.end(),
									in_role.begin());
	return in_role_end - in_role.begin();
}
			  
void LLGroupRoleData::addMember(const LLUUID& member)
{
	mMembersNeedsSort = true;
	mMemberIDs.push_back(member);
}

bool LLGroupRoleData::removeMember(const LLUUID& member)
{
	uuid_vec_t::iterator it = std::find(mMemberIDs.begin(),mMemberIDs.end(),member);

	if (it != mMemberIDs.end())
	{
		mMembersNeedsSort = true;
		mMemberIDs.erase(it);
		return true;
	}

	return false;
}

void LLGroupRoleData::clearMembers()
{
	mMembersNeedsSort = false;
	mMemberIDs.clear();
}


//
// LLGroupMgrGroupData
//

LLGroupMgrGroupData::LLGroupMgrGroupData(const LLUUID& id) : 
	mID(id), 
	mShowInList(true), 
	mOpenEnrollment(false), 
	mMembershipFee(0),
	mAllowPublish(false),
	mListInProfile(false),
	mMaturePublish(false),
	mChanged(false),
	mMemberCount(0),
	mRoleCount(0),
	mReceivedRoleMemberPairs(0),
	mMemberDataComplete(false),
	mRoleDataComplete(false),
	mRoleMemberDataComplete(false),
	mGroupPropertiesDataComplete(false),
	mPendingRoleMemberRequest(false),
	mAccessTime(0.0f),
	mPendingBanRequest(false)
{
	mMemberVersion.generate();
}

void LLGroupMgrGroupData::setAccessed()
{
	mAccessTime = (F32)LLFrameTimer::getTotalSeconds();
}

bool LLGroupMgrGroupData::getRoleData(const LLUUID& role_id, LLRoleData& role_data)
{
	role_data_map_t::const_iterator it;

	// Do we have changes for it?
	it = mRoleChanges.find(role_id);
	if (it != mRoleChanges.end()) 
	{
		if ((*it).second.mChangeType == RC_DELETE) return false;

		role_data = (*it).second;
		return true;
	}

	// Ok, no changes, hasn't been deleted, isn't a new role, just find the role.
	role_list_t::const_iterator rit = mRoles.find(role_id);
	if (rit != mRoles.end())
	{
		role_data = (*rit).second->getRoleData();
		return true;
	}

	// This role must not exist.
	return false;
}


void LLGroupMgrGroupData::setRoleData(const LLUUID& role_id, LLRoleData role_data)
{
	// If this is a newly created group, we need to change the data in the created list.
	role_data_map_t::iterator it;
	it = mRoleChanges.find(role_id);
	if (it != mRoleChanges.end())
	{
		if ((*it).second.mChangeType == RC_CREATE)
		{
			role_data.mChangeType = RC_CREATE;
			mRoleChanges[role_id] = role_data;
			return;
		}
		else if ((*it).second.mChangeType == RC_DELETE)
		{
			// Don't do anything for a role being deleted.
			return;
		}
	}

	// Not a new role, so put it in the changes list.
	LLRoleData old_role_data;
	role_list_t::iterator rit = mRoles.find(role_id);
	if (rit != mRoles.end())
	{
		bool data_change = ( ((*rit).second->mRoleData.mRoleDescription != role_data.mRoleDescription)
							|| ((*rit).second->mRoleData.mRoleName != role_data.mRoleName)
							|| ((*rit).second->mRoleData.mRoleTitle != role_data.mRoleTitle) );
		bool powers_change = ((*rit).second->mRoleData.mRolePowers != role_data.mRolePowers);
		
		if (!data_change && !powers_change)
		{
			// We are back to the original state, the changes have been 'undone' so take out the change.
			mRoleChanges.erase(role_id);
			return;
		}

		if (data_change && powers_change)
		{
			role_data.mChangeType = RC_UPDATE_ALL;
		}
		else if (data_change)
		{
			role_data.mChangeType = RC_UPDATE_DATA;
		}
		else
		{
			role_data.mChangeType = RC_UPDATE_POWERS;
		}

		mRoleChanges[role_id] = role_data;
	}
	else
	{
		LL_WARNS() << "Change being made to non-existant role " << role_id << LL_ENDL;
	}
}

bool LLGroupMgrGroupData::pendingRoleChanges()
{
	return (!mRoleChanges.empty());
}

// This is a no-op if the role has already been created.
void LLGroupMgrGroupData::createRole(const LLUUID& role_id, LLRoleData role_data)
{
	if (mRoleChanges.find(role_id) != mRoleChanges.end())
	{
		LL_WARNS() << "create role for existing role! " << role_id << LL_ENDL;
	}
	else
	{
		role_data.mChangeType = RC_CREATE;
		mRoleChanges[role_id] = role_data;
	}
}

void LLGroupMgrGroupData::deleteRole(const LLUUID& role_id)
{
	role_data_map_t::iterator it;

	// If this was a new role, just discard it.
	it = mRoleChanges.find(role_id);
	if (it != mRoleChanges.end() 
		&& (*it).second.mChangeType == RC_CREATE)
	{
		mRoleChanges.erase(it);
		return;
	}

	LLRoleData rd;
	rd.mChangeType = RC_DELETE;
	mRoleChanges[role_id] = rd;
}

void LLGroupMgrGroupData::addRolePower(const LLUUID &role_id, U64 power)
{
	LLRoleData rd;
	if (getRoleData(role_id,rd))
	{	
		rd.mRolePowers |= power;
		setRoleData(role_id,rd);
	}
	else
	{
		LL_WARNS() << "addRolePower: no role data found for " << role_id << LL_ENDL;
	}
}

void LLGroupMgrGroupData::removeRolePower(const LLUUID &role_id, U64 power)
{
	LLRoleData rd;
	if (getRoleData(role_id,rd))
	{
		rd.mRolePowers &= ~power;
		setRoleData(role_id,rd);
	}
	else
	{
		LL_WARNS() << "removeRolePower: no role data found for " << role_id << LL_ENDL;
	}
}

U64 LLGroupMgrGroupData::getRolePowers(const LLUUID& role_id)
{
	LLRoleData rd;
	if (getRoleData(role_id,rd))
	{
		return rd.mRolePowers;
	}
	else
	{
		LL_WARNS() << "getRolePowers: no role data found for " << role_id << LL_ENDL;
		return GP_NO_POWERS;
	}
}

void LLGroupMgrGroupData::removeData()
{
	// Remove member data first, because removeRoleData will walk the member list
	removeMemberData();
	removeRoleData();
}

void LLGroupMgrGroupData::removeMemberData()
{
	for (member_list_t::iterator mi = mMembers.begin(); mi != mMembers.end(); ++mi)
	{
		delete mi->second;
	}
	mMembers.clear();
	mMemberDataComplete = false;
	mMemberVersion.generate();
}

void LLGroupMgrGroupData::removeRoleData()
{
	for (member_list_t::iterator mi = mMembers.begin(); mi != mMembers.end(); ++mi)
	{
		LLGroupMemberData* data = mi->second;
		if (data)
		{
			data->clearRoles();
		}
	}

	for (role_list_t::iterator ri = mRoles.begin(); ri != mRoles.end(); ++ri)
	{
		LLGroupRoleData* data = ri->second;
		delete data;
	}
	mRoles.clear();
	mReceivedRoleMemberPairs = 0;
	mRoleDataComplete = false;
	mRoleMemberDataComplete= false;
}

void LLGroupMgrGroupData::removeRoleMemberData()
{
	for (member_list_t::iterator mi = mMembers.begin(); mi != mMembers.end(); ++mi)
	{
		LLGroupMemberData* data = mi->second;
		if (data)
		{
			data->clearRoles();
		}
	}

	for (role_list_t::iterator ri = mRoles.begin(); ri != mRoles.end(); ++ri)
	{
		LLGroupRoleData* data = ri->second;
		if (data)
		{
			data->clearMembers();
		}
	}

	mReceivedRoleMemberPairs = 0;
	mRoleMemberDataComplete= false;
}

LLGroupMgrGroupData::~LLGroupMgrGroupData()
{
	removeData();
}

bool LLGroupMgrGroupData::changeRoleMember(const LLUUID& role_id, 
										   const LLUUID& member_id, 
										   LLRoleMemberChangeType rmc)
{
	role_list_t::iterator ri = mRoles.find(role_id);
	member_list_t::iterator mi = mMembers.find(member_id);

	if (ri == mRoles.end()
		|| mi == mMembers.end() )
	{
		if (ri == mRoles.end()) LL_WARNS() << "LLGroupMgrGroupData::changeRoleMember couldn't find role " << role_id << LL_ENDL;
		if (mi == mMembers.end()) LL_WARNS() << "LLGroupMgrGroupData::changeRoleMember couldn't find member " << member_id << LL_ENDL;
		return false;
	}

	LLGroupRoleData* grd = ri->second;
	LLGroupMemberData* gmd = mi->second;

	if (!grd || !gmd)
	{
		LL_WARNS() << "LLGroupMgrGroupData::changeRoleMember couldn't get member or role data." << LL_ENDL;
		return false;
	}

	if (RMC_ADD == rmc)
	{
		LL_INFOS() << " adding member to role." << LL_ENDL;
		grd->addMember(member_id);
		gmd->addRole(role_id,grd);

		//TODO move this into addrole function
		//see if they added someone to the owner role and update isOwner
		gmd->mIsOwner = (role_id == mOwnerRole) ? true : gmd->mIsOwner;
	}
	else if (RMC_REMOVE == rmc)
	{
		LL_INFOS() << " removing member from role." << LL_ENDL;
		grd->removeMember(member_id);
		gmd->removeRole(role_id);

		//see if they removed someone from the owner role and update isOwner
		gmd->mIsOwner = (role_id == mOwnerRole) ? false : gmd->mIsOwner;
	}

	lluuid_pair role_member;
	role_member.first = role_id;
	role_member.second = member_id;

	change_map_t::iterator it = mRoleMemberChanges.find(role_member);
	if (it != mRoleMemberChanges.end())
	{
		// There was already a role change for this role_member
		if (it->second.mChange == rmc)
		{
			// Already recorded this change?  Weird.
			LL_INFOS() << "Received duplicate change for "
					<< " role: " << role_id << " member " << member_id 
					<< " change " << (rmc == RMC_ADD ? "ADD" : "REMOVE") << LL_ENDL;
		}
		else
		{
			// The only two operations (add and remove) currently cancel each other out
			// If that changes this will need more logic
			if (rmc == RMC_NONE)
			{
				LL_WARNS() << "changeRoleMember: existing entry with 'RMC_NONE' change! This shouldn't happen." << LL_ENDL;
				LLRoleMemberChange rc(role_id,member_id,rmc);
				mRoleMemberChanges[role_member] = rc;
			}
			else
			{
				mRoleMemberChanges.erase(it);
			}
		}
	}
	else
	{
		LLRoleMemberChange rc(role_id,member_id,rmc);
		mRoleMemberChanges[role_member] = rc;
	}

	recalcAgentPowers(member_id);

	mChanged = true;
	return true;
}

void LLGroupMgrGroupData::recalcAllAgentPowers()
{
	LLGroupMemberData* gmd;

	for (member_list_t::iterator mit = mMembers.begin();
		 mit != mMembers.end(); ++mit)
	{
		gmd = mit->second;
		if (!gmd) continue;

		gmd->mAgentPowers = 0;
		for (LLGroupMemberData::role_list_t::iterator it = gmd->mRolesList.begin();
			 it != gmd->mRolesList.end(); ++it)
		{
			LLGroupRoleData* grd = (*it).second;
			if (!grd) continue;

			gmd->mAgentPowers |= grd->mRoleData.mRolePowers;
		}
	}
}

void LLGroupMgrGroupData::recalcAgentPowers(const LLUUID& agent_id)
{
	member_list_t::iterator mi = mMembers.find(agent_id);
	if (mi == mMembers.end()) return;

	LLGroupMemberData* gmd = mi->second;

	if (!gmd) return;

	gmd->mAgentPowers = 0;
	for (LLGroupMemberData::role_list_t::iterator it = gmd->mRolesList.begin();
		 it != gmd->mRolesList.end(); ++it)
	{
		LLGroupRoleData* grd = (*it).second;
		if (!grd) continue;

		gmd->mAgentPowers |= grd->mRoleData.mRolePowers;
	}
}

bool LLGroupMgrGroupData::isSingleMemberNotOwner()
{
	return mMembers.size() == 1 && !mMembers.begin()->second->isOwner();
}

bool packRoleUpdateMessageBlock(LLMessageSystem* msg, 
								const LLUUID& group_id,
								const LLUUID& role_id, 
								const LLRoleData& role_data, 
								bool start_message)
{
	if (start_message)
	{
		msg->newMessage("GroupRoleUpdate");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID",gAgent.getID());
		msg->addUUID("SessionID",gAgent.getSessionID());
		msg->addUUID("GroupID",group_id);
		start_message = false;
	}

	msg->nextBlock("RoleData");
	msg->addUUID("RoleID",role_id);
	msg->addString("Name", role_data.mRoleName);
	msg->addString("Description", role_data.mRoleDescription);
	msg->addString("Title", role_data.mRoleTitle);
	msg->addU64("Powers", role_data.mRolePowers);
	msg->addU8("UpdateType", (U8)role_data.mChangeType);

	if (msg->isSendFullFast())
	{
		gAgent.sendReliableMessage();
		start_message = true;
	}

	return start_message;
}

void LLGroupMgrGroupData::sendRoleChanges()
{
	// Commit changes locally
	LLGroupRoleData* grd;
	role_list_t::iterator role_it;
	LLMessageSystem* msg = gMessageSystem;
	bool start_message = true;

	bool need_role_cleanup = false;
	bool need_role_data = false;
	bool need_power_recalc = false;

	// Apply all changes
	for (role_data_map_t::iterator iter = mRoleChanges.begin();
		 iter != mRoleChanges.end(); )
	{
		role_data_map_t::iterator it = iter++; // safely incrament iter
		const LLUUID& role_id = (*it).first;
		const LLRoleData& role_data = (*it).second;

		// Commit to local data set
		role_it = mRoles.find((*it).first);
		if ( (mRoles.end() == role_it 
				&& RC_CREATE != role_data.mChangeType)
			|| (mRoles.end() != role_it
				&& RC_CREATE == role_data.mChangeType))
		{
			continue;
		}
	
		// NOTE: role_it is valid EXCEPT for the RC_CREATE case
		switch (role_data.mChangeType)
		{
			case RC_CREATE:
			{
				// NOTE: role_it is NOT valid in this case
				grd = new LLGroupRoleData(role_id, role_data, 0);
				mRoles[role_id] = grd;
				need_role_data = true;
				break;
			}
			case RC_DELETE:
			{
				LLGroupRoleData* group_role_data = (*role_it).second;
				delete group_role_data;
				mRoles.erase(role_it);
				need_role_cleanup = true;
				need_power_recalc = true;
				break;
			}
			case RC_UPDATE_ALL:
				// fall through
			case RC_UPDATE_POWERS:
				need_power_recalc = true;
				// fall through
			case RC_UPDATE_DATA:
				// fall through
			default: 
			{
				LLGroupRoleData* group_role_data = (*role_it).second;
				group_role_data->setRoleData(role_data); // NOTE! might modify mRoleChanges!
				break;
			}
		}

		// Update dataserver
		start_message = packRoleUpdateMessageBlock(msg,getID(),role_id,role_data,start_message);
	}

	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}

	// If we delete a role then all the role-member pairs are invalid!
	if (need_role_cleanup)
	{
		removeRoleMemberData();
	}

	// If we create a new role, then we need to re-fetch all the role data.
	if (need_role_data)
	{
		LLGroupMgr::getInstance()->sendGroupRoleDataRequest(getID());
	}

	// Clean up change lists	
	mRoleChanges.clear();

	// Recalculate all the agent powers because role powers have now changed.
	if (need_power_recalc)
	{
		recalcAllAgentPowers();
	}
}

void LLGroupMgrGroupData::cancelRoleChanges()
{
	// Clear out all changes!
	mRoleChanges.clear();
}

void LLGroupMgrGroupData::createBanEntry(const LLUUID& ban_id, const LLGroupBanData& ban_data)
{ 
	mBanList[ban_id] = ban_data;
}

void LLGroupMgrGroupData::removeBanEntry(const LLUUID& ban_id)
{
	mBanList.erase(ban_id);
}

void LLGroupMgrGroupData::banMemberById(const LLUUID& participant_uuid)
{
	if (!mMemberDataComplete ||
		!mRoleDataComplete ||
		!(mRoleMemberDataComplete && mMembers.size()))
	{
		LL_WARNS() << "No Role-Member data yet, setting ban request to pending." << LL_ENDL;
		mPendingBanRequest = true;
		mPendingBanMemberID = participant_uuid;

		if (!mMemberDataComplete || !mMembers.size())
		{
			LLGroupMgr::getInstance()->sendCapGroupMembersRequest(mID);
		}

		if (!mRoleDataComplete)
		{
			LLGroupMgr::getInstance()->sendGroupRoleDataRequest(mID);
		}

		return;
	}
	
	LLGroupMgrGroupData::member_list_t::iterator mi = mMembers.find((participant_uuid));
	if (mi == mMembers.end())
	{
		if (!mPendingBanRequest)
		{
			mPendingBanRequest = true;
			mPendingBanMemberID = participant_uuid;
			LLGroupMgr::getInstance()->sendCapGroupMembersRequest(mID); // member isn't in members list, request reloading
		}
		else
		{
			mPendingBanRequest = false;
		}

		return;
	}

	mPendingBanRequest = false;

	LLGroupMemberData* member_data = (*mi).second;
	if (member_data && member_data->isInRole(mOwnerRole))
	{
		return; // can't ban group owner
	}

	std::vector<LLUUID> ids;
	ids.push_back(participant_uuid);

	LLGroupBanData ban_data;
	createBanEntry(participant_uuid, ban_data);
	LLGroupMgr::getInstance()->sendGroupBanRequest(LLGroupMgr::REQUEST_POST, mID, LLGroupMgr::BAN_CREATE, ids);
	LLGroupMgr::getInstance()->sendGroupMemberEjects(mID, ids);
	LLGroupMgr::getInstance()->sendGroupMembersRequest(mID);
	LLSD args;
	LLAvatarName av_name;
	LLAvatarNameCache::get(participant_uuid, &av_name);
	args["AVATAR_NAME"] = av_name.getUserName();
	args["GROUP_NAME"] = mName;
	LLNotifications::instance().add(LLNotification::Params("EjectAvatarFromGroup").substitutions(args));
}

//
// LLGroupMgr
//

LLGroupMgr::LLGroupMgr():
    mMemberRequestInFlight(false)
{
}

LLGroupMgr::~LLGroupMgr()
{
	clearGroups();
}

void LLGroupMgr::clearGroups()
{
	std::for_each(mRoleActionSets.begin(), mRoleActionSets.end(), DeletePointer());
	mRoleActionSets.clear();
	std::for_each(mGroups.begin(), mGroups.end(), DeletePairedPointer());
	mGroups.clear();
	mObservers.clear();
}

void LLGroupMgr::clearGroupData(const LLUUID& group_id)
{
	group_map_t::iterator iter = mGroups.find(group_id);
	if (iter != mGroups.end())
	{
		delete (*iter).second;
		mGroups.erase(iter);
	}
}

void LLGroupMgr::addObserver(LLGroupMgrObserver* observer) 
{ 
	if( observer->getID() != LLUUID::null )
		mObservers.insert(std::pair<LLUUID, LLGroupMgrObserver*>(observer->getID(), observer));
}

void LLGroupMgr::addObserver(const LLUUID& group_id, LLParticularGroupObserver* observer)
{
	if(group_id.notNull() && observer)
	{
		mParticularObservers[group_id].insert(observer);
	}
}

void LLGroupMgr::removeObserver(LLGroupMgrObserver* observer)
{
	if (!observer)
	{
		return;
	}
	observer_multimap_t::iterator it;
	it = mObservers.find(observer->getID());
	while (it != mObservers.end())
	{
		if (it->second == observer)
		{
			mObservers.erase(it);
			break;
		}
		else
		{
			++it;
		}
	}
}

void LLGroupMgr::removeObserver(const LLUUID& group_id, LLParticularGroupObserver* observer)
{
	if(group_id.isNull() || !observer)
	{
		return;
	}

    observer_map_t::iterator obs_it = mParticularObservers.find(group_id);
    if(obs_it == mParticularObservers.end())
        return;

    obs_it->second.erase(observer);

    if (obs_it->second.size() == 0)
    	mParticularObservers.erase(obs_it);
}

LLGroupMgrGroupData* LLGroupMgr::getGroupData(const LLUUID& id)
{
	group_map_t::iterator gi = mGroups.find(id);

	if (gi != mGroups.end())
	{
		return gi->second;
	}
	return NULL;
}

// Helper function for LLGroupMgr::processGroupMembersReply
// This reformats date strings from MM/DD/YYYY to YYYY/MM/DD ( e.g. 1/27/2008 -> 2008/1/27 )
// so that the sorter can sort by year before month before day.
static void formatDateString(std::string &date_string)
{
	using namespace boost;
	cmatch result;
	const regex expression("([0-9]{1,2})/([0-9]{1,2})/([0-9]{4})");
	if (regex_match(date_string.c_str(), result, expression))
	{
		// convert matches to integers so that we can pad them with zeroes on Linux
		S32 year	= boost::lexical_cast<S32>(result[3]);
		S32 month	= boost::lexical_cast<S32>(result[1]);
		S32 day		= boost::lexical_cast<S32>(result[2]);

		// ISO 8601 date format
		date_string = llformat("%04d/%02d/%02d", year, month, day);
	}
}

// static
void LLGroupMgr::processGroupMembersReply(LLMessageSystem* msg, void** data)
{
    LL_PROFILE_ZONE_SCOPED;

	LL_DEBUGS("GrpMgr") << "LLGroupMgr::processGroupMembersReply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group members reply for another agent!" << LL_ENDL;
		return;
	}

	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id );

	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_RequestID, request_id);

	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap || (group_datap->mMemberRequestID != request_id))
	{
		LL_WARNS() << "processGroupMembersReply: Received incorrect (stale?) group or request id" << LL_ENDL;
		return;
	}

	msg->getS32(_PREHASH_GroupData, "MemberCount", group_datap->mMemberCount );

	if (group_datap->mMemberCount > 0)
	{
		S32 contribution = 0;
		std::string online_status;
		std::string title;
		U64 agent_powers = 0;
		bool is_owner = false;

		S32 num_members = msg->getNumberOfBlocksFast(_PREHASH_MemberData);
		for (S32 i = 0; i < num_members; i++)
		{
			LLUUID member_id;

			msg->getUUIDFast(_PREHASH_MemberData, _PREHASH_AgentID, member_id, i );
			msg->getS32(_PREHASH_MemberData, _PREHASH_Contribution, contribution, i);
			msg->getU64(_PREHASH_MemberData, "AgentPowers", agent_powers, i);
			msg->getStringFast(_PREHASH_MemberData, _PREHASH_OnlineStatus, online_status, i);
			msg->getString(_PREHASH_MemberData, "Title", title, i);
			msg->getBOOL(_PREHASH_MemberData,"IsOwner",is_owner,i);

			if (member_id.notNull())
			{
				if (online_status == "Online")
				{
					static std::string localized_online(LLTrans::getString("group_member_status_online"));
					online_status = localized_online;
				}
				else
				{
					formatDateString(online_status); // reformat for sorting, e.g. 12/25/2008 -> 2008/12/25
				}
				
				//LL_INFOS() << "Member " << member_id << " has powers " << std::hex << agent_powers << std::dec << LL_ENDL;
				LLGroupMemberData* newdata = new LLGroupMemberData(member_id, 
																	contribution, 
																	agent_powers, 
																	title,
																	online_status,
																	is_owner);
#if LL_DEBUG
				LLGroupMgrGroupData::member_list_t::iterator mit = group_datap->mMembers.find(member_id);
				if (mit != group_datap->mMembers.end())
				{
					LL_INFOS() << " *** Received duplicate member data for agent " << member_id << LL_ENDL;
				}
#endif
				group_datap->mMembers[member_id] = newdata;
			}
			else
			{
				LL_INFOS() << "Received null group member data." << LL_ENDL;
			}
		}

		//if group members are loaded while titles are missing, load the titles.
		if(group_datap->mTitles.size() < 1)
		{
			LLGroupMgr::getInstance()->sendGroupTitlesRequest(group_id);
		}
	}

	group_datap->mMemberVersion.generate();

	if (group_datap->mMembers.size() ==  (U32)group_datap->mMemberCount)
	{
		group_datap->mMemberDataComplete = true;
		group_datap->mMemberRequestID.setNull();
		// We don't want to make role-member data requests until we have all the members
		if (group_datap->mPendingRoleMemberRequest)
		{
			group_datap->mPendingRoleMemberRequest = false;
			LLGroupMgr::getInstance()->sendGroupRoleMembersRequest(group_datap->mID);
		}
	}

	group_datap->mChanged = true;
	LLGroupMgr::getInstance()->notifyObservers(GC_MEMBER_DATA);
}

//static 
void LLGroupMgr::processGroupPropertiesReply(LLMessageSystem* msg, void** data)
{
    LL_PROFILE_ZONE_SCOPED;

	LL_DEBUGS("GrpMgr") << "LLGroupMgr::processGroupPropertiesReply" << LL_ENDL;
	if (!msg)
	{
		LL_ERRS() << "Can't access the messaging system" << LL_ENDL;
		return;
	}
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group properties reply for another agent!" << LL_ENDL;
		return;
	}

	LLUUID group_id;
	std::string	name;
	std::string	charter;
	bool	show_in_list = false;
	LLUUID	founder_id;
	U64		powers_mask = GP_NO_POWERS;
	S32		money = 0;
	std::string	member_title;
	LLUUID	insignia_id;
	LLUUID	owner_role;
	U32		membership_fee = 0;
	bool	open_enrollment = false;
	S32		num_group_members = 0;
	S32		num_group_roles = 0;
	bool	allow_publish = false;
	bool	mature = false;

	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id );
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_FounderID, founder_id);	
	msg->getStringFast(_PREHASH_GroupData, _PREHASH_Name, name );
	msg->getStringFast(_PREHASH_GroupData, _PREHASH_Charter, charter );
	msg->getBOOLFast(_PREHASH_GroupData, _PREHASH_ShowInList, show_in_list );
	msg->getStringFast(_PREHASH_GroupData, _PREHASH_MemberTitle, member_title );
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_InsigniaID, insignia_id );
	msg->getU64Fast(_PREHASH_GroupData, _PREHASH_PowersMask, powers_mask );
	msg->getU32Fast(_PREHASH_GroupData, _PREHASH_MembershipFee, membership_fee );
	msg->getBOOLFast(_PREHASH_GroupData, _PREHASH_OpenEnrollment, open_enrollment );
	msg->getS32Fast(_PREHASH_GroupData, _PREHASH_GroupMembershipCount, num_group_members);
	msg->getS32(_PREHASH_GroupData, "GroupRolesCount", num_group_roles);
	msg->getS32Fast(_PREHASH_GroupData, _PREHASH_Money, money);
	msg->getBOOL("GroupData", "AllowPublish", allow_publish);
	msg->getBOOL("GroupData", "MaturePublish", mature);
	msg->getUUID(_PREHASH_GroupData, "OwnerRole", owner_role);

	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->createGroupData(group_id);

	group_datap->mName = name;
	group_datap->mCharter = charter;
	group_datap->mShowInList = show_in_list;
	group_datap->mInsigniaID = insignia_id;
	group_datap->mFounderID = founder_id;
	group_datap->mMembershipFee = membership_fee;
	group_datap->mOpenEnrollment = open_enrollment;
	group_datap->mAllowPublish = allow_publish;
	group_datap->mMaturePublish = mature;
	group_datap->mOwnerRole = owner_role;
	group_datap->mMemberCount = num_group_members;
	group_datap->mRoleCount = num_group_roles + 1; // Add the everyone role.
	
	group_datap->mGroupPropertiesDataComplete = true;
	group_datap->mChanged = true;

    properties_request_map_t::iterator request = LLGroupMgr::getInstance()->mPropRequests.find(group_id);
    if (request != LLGroupMgr::getInstance()->mPropRequests.end())
    {
        LLGroupMgr::getInstance()->mPropRequests.erase(request);
    }
    else
    {
        LL_DEBUGS("GrpMgr") << "GroupPropertyResponse received with no pending request. Response was slow." << LL_ENDL;
    }
	LLGroupMgr::getInstance()->notifyObservers(GC_PROPERTIES);
}

// static
void LLGroupMgr::processGroupRoleDataReply(LLMessageSystem* msg, void** data)
{
    LL_PROFILE_ZONE_SCOPED;

	LL_DEBUGS("GrpMgr") << "LLGroupMgr::processGroupRoleDataReply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group role data reply for another agent!" << LL_ENDL;
		return;
	}

	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id );

	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_RequestID, request_id);

	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap || (group_datap->mRoleDataRequestID != request_id))
	{
		LL_WARNS() << "processGroupPropertiesReply: Received incorrect (stale?) group or request id" << LL_ENDL;
		return;
	}

	msg->getS32(_PREHASH_GroupData, "RoleCount", group_datap->mRoleCount );

	std::string	name;
	std::string	title;
	std::string	desc;
	U64		powers = 0;
	U32		member_count = 0;
	LLUUID role_id;

	U32 num_blocks = msg->getNumberOfBlocks("RoleData");
	U32 i = 0;
	for (i=0; i< num_blocks; ++i)
	{
		msg->getUUID("RoleData", "RoleID", role_id, i );
		
		msg->getString("RoleData","Name",name,i);
		msg->getString("RoleData","Title",title,i);
		msg->getString("RoleData","Description",desc,i);
		msg->getU64("RoleData","Powers",powers,i);
		msg->getU32("RoleData","Members",member_count,i);

		//there are 3 predifined roles - Owners, Officers, Everyone
		//there names are defined in lldatagroups.cpp
		//lets change names from server to localized strings
		if(name == "Everyone")
		{
			name = LLTrans::getString("group_role_everyone");
		}
		else if(name == "Officers")
		{
			name = LLTrans::getString("group_role_officers");
		}
		else if(name == "Owners")
		{
			name = LLTrans::getString("group_role_owners");
		}



        LL_DEBUGS("GrpMgr") << "Adding role data: " << name << " {" << role_id << "}" << LL_ENDL;
		LLGroupRoleData* rd = new LLGroupRoleData(role_id,name,title,desc,powers,member_count);
		group_datap->mRoles[role_id] = rd;
	}

	if (group_datap->mRoles.size() == (U32)group_datap->mRoleCount)
	{
		group_datap->mRoleDataComplete = true;
		group_datap->mRoleDataRequestID.setNull();
		// We don't want to make role-member data requests until we have all the role data
		if (group_datap->mPendingRoleMemberRequest)
		{
			group_datap->mPendingRoleMemberRequest = false;
			LLGroupMgr::getInstance()->sendGroupRoleMembersRequest(group_datap->mID);
		}
	}

	group_datap->mChanged = true;
	LLGroupMgr::getInstance()->notifyObservers(GC_ROLE_DATA);
}

// static
void LLGroupMgr::processGroupRoleMembersReply(LLMessageSystem* msg, void** data)
{
    LL_PROFILE_ZONE_SCOPED;

	LL_DEBUGS("GrpMgr") << "LLGroupMgr::processGroupRoleMembersReply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group role members reply for another agent!" << LL_ENDL;
		return;
	}

	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_RequestID, request_id);

	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_GroupID, group_id );

	U32 total_pairs;
	msg->getU32(_PREHASH_AgentData, "TotalPairs", total_pairs);

	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap || (group_datap->mRoleMembersRequestID != request_id))
	{
		LL_WARNS() << "processGroupRoleMembersReply: Received incorrect (stale?) group or request id" << LL_ENDL;
		return;
	}

	U32 num_blocks = msg->getNumberOfBlocks("MemberData");
	U32 i;
	LLUUID member_id;
	LLUUID role_id;
	LLGroupRoleData* rd = NULL;
	LLGroupMemberData* md = NULL;

	LLGroupMgrGroupData::role_list_t::iterator ri;
	LLGroupMgrGroupData::member_list_t::iterator mi;

	// If total_pairs == 0, there are no members in any custom roles.
	if (total_pairs > 0)
	{
		for (i = 0;i < num_blocks; ++i)
		{
			msg->getUUID("MemberData","RoleID",role_id,i);
			msg->getUUID("MemberData","MemberID",member_id,i);

			if (role_id.notNull() && member_id.notNull() )
			{
				rd = NULL;
				ri = group_datap->mRoles.find(role_id);
				if (ri != group_datap->mRoles.end())
				{
					rd = ri->second;
				}

				md = NULL;
				mi = group_datap->mMembers.find(member_id);
				if (mi != group_datap->mMembers.end())
				{
					md = mi->second;
				}

				if (rd && md)
				{
					LL_DEBUGS("GrpMgr") << "Adding role-member pair: " << role_id << ", " << member_id << LL_ENDL;
					rd->addMember(member_id);
					md->addRole(role_id,rd);
				}
				else
				{
					if (!rd) LL_WARNS() << "Received role data for unknown role " << role_id << " in group " << group_id << LL_ENDL;
					if (!md) LL_WARNS() << "Received role data for unknown member " << member_id << " in group " << group_id << LL_ENDL;
				}
			}
		}

		group_datap->mReceivedRoleMemberPairs += num_blocks;
	}

	if (group_datap->mReceivedRoleMemberPairs == total_pairs)
	{
		// Add role data for the 'everyone' role to all members
		LLGroupRoleData* everyone = group_datap->mRoles[LLUUID::null];
		if (!everyone)
		{
			LL_WARNS() << "Everyone role not found!" << LL_ENDL;
		}
		else
		{
			for (LLGroupMgrGroupData::member_list_t::iterator mi = group_datap->mMembers.begin();
				 mi != group_datap->mMembers.end(); ++mi)
			{
				LLGroupMemberData* data = mi->second;
				if (data)
				{
					data->addRole(LLUUID::null,everyone);
				}
			}
		}
		
        group_datap->mRoleMemberDataComplete= true;
		group_datap->mRoleMembersRequestID.setNull();
	}

	group_datap->mChanged = true;
	LLGroupMgr::getInstance()->notifyObservers(GC_ROLE_MEMBER_DATA);

	if (group_datap->mPendingBanRequest)
	{
		group_datap->banMemberById(group_datap->mPendingBanMemberID);
	}
}

// static
void LLGroupMgr::processGroupTitlesReply(LLMessageSystem* msg, void** data)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::processGroupTitlesReply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		LL_WARNS() << "Got group properties reply for another agent!" << LL_ENDL;
		return;
	}

	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_GroupID, group_id );
	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_RequestID, request_id);
	
	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap || (group_datap->mTitlesRequestID != request_id))
	{
		LL_WARNS() << "processGroupTitlesReply: Received incorrect (stale?) group" << LL_ENDL;
		return;
	}

	LLGroupTitle title;

	S32 i = 0;
	S32 blocks = msg->getNumberOfBlocksFast(_PREHASH_GroupData);
	for (i=0; i<blocks; ++i)
	{
		msg->getString("GroupData","Title",title.mTitle,i);
		msg->getUUID("GroupData","RoleID",title.mRoleID,i);
		msg->getBOOL("GroupData","Selected",title.mSelected,i);

		if (!title.mTitle.empty())
		{
			LL_DEBUGS("GrpMgr") << "LLGroupMgr adding title: " << title.mTitle << ", " << title.mRoleID << ", " << (title.mSelected ? 'Y' : 'N') << LL_ENDL;
			group_datap->mTitles.push_back(title);
		}
	}

	group_datap->mChanged = true;
	LLGroupMgr::getInstance()->notifyObservers(GC_TITLES);
}

// static
void LLGroupMgr::processEjectGroupMemberReply(LLMessageSystem* msg, void ** data)
{
	LL_DEBUGS("GrpMgr") << "processEjectGroupMemberReply" << LL_ENDL;
	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id);
	bool success;
	msg->getBOOLFast(_PREHASH_EjectData, _PREHASH_Success, success);

	// If we had a failure, the group panel needs to be updated.
	if (!success)
	{
		LLGroupActions::refresh(group_id);
	}
}

// static
void LLGroupMgr::processJoinGroupReply(LLMessageSystem* msg, void ** data)
{
	LL_DEBUGS("GrpMgr") << "processJoinGroupReply" << LL_ENDL;
	LLUUID group_id;
	bool success;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id);
	msg->getBOOLFast(_PREHASH_GroupData, _PREHASH_Success, success);

	if (success)
	{
		// refresh all group information
		gAgent.sendAgentDataUpdateRequest();

		LLGroupMgr::getInstance()->clearGroupData(group_id);
		// refresh the floater for this group, if any.
		LLGroupActions::refresh(group_id);
	}
}

// static
void LLGroupMgr::processLeaveGroupReply(LLMessageSystem* msg, void ** data)
{
	LL_DEBUGS("GrpMgr") << "processLeaveGroupReply" << LL_ENDL;
	LLUUID group_id;
	bool success;
	msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id);
	msg->getBOOLFast(_PREHASH_GroupData, _PREHASH_Success, success);

	if (success)
	{
		// refresh all group information
		gAgent.sendAgentDataUpdateRequest();

		LLGroupMgr::getInstance()->clearGroupData(group_id);
		// close the floater for this group, if any.
		LLGroupActions::closeGroup(group_id);
	}
}

// static
void LLGroupMgr::processCreateGroupReply(LLMessageSystem* msg, void ** data)
{
	LLUUID group_id;
	bool success;
	std::string message;

	msg->getUUIDFast(_PREHASH_ReplyData, _PREHASH_GroupID, group_id );

	msg->getBOOLFast(_PREHASH_ReplyData, _PREHASH_Success,	success );
	msg->getStringFast(_PREHASH_ReplyData, _PREHASH_Message, message );

	if (success)
	{
		// refresh all group information
		gAgent.sendAgentDataUpdateRequest();

		// HACK! We haven't gotten the agent group update yet, so ... um ... fake it.
		// This is so when we go to modify the group we will be able to do so.
		// This isn't actually too bad because real data will come down in 2 or 3 miliseconds and replace this.
		LLGroupData gd;
		gd.mAcceptNotices = true;
		gd.mListInProfile = true;
		gd.mContribution = 0;
		gd.mID = group_id;
		gd.mName = "new group";
		gd.mPowers = GP_ALL_POWERS;

		gAgent.mGroups.push_back(gd);

		LLPanelGroupCreate::refreshCreatedGroup(group_id);
		//FIXME
		//LLFloaterGroupInfo::closeCreateGroup();
		//LLFloaterGroupInfo::showFromUUID(group_id,"roles_tab");
	}
	else
	{
		// *TODO: Translate
		LLSD args;
		args["MESSAGE"] = message;
		LLNotificationsUtil::add("UnableToCreateGroup", args);
	}
}

LLGroupMgrGroupData* LLGroupMgr::createGroupData(const LLUUID& id)
{
	LLGroupMgrGroupData* group_datap = NULL;

	group_map_t::iterator existing_group = LLGroupMgr::getInstance()->mGroups.find(id);
	if (existing_group == LLGroupMgr::getInstance()->mGroups.end())
	{
		group_datap = new LLGroupMgrGroupData(id);
		LLGroupMgr::getInstance()->addGroup(group_datap);
	}
	else
	{
		group_datap = existing_group->second;
	}

	if (group_datap)
	{
		group_datap->setAccessed();
	}

	return group_datap;
}

bool LLGroupMgr::hasPendingPropertyRequest(const LLUUID & id)
{
    properties_request_map_t::iterator existing_req = LLGroupMgr::getInstance()->mPropRequests.find(id);
    if (existing_req != LLGroupMgr::getInstance()->mPropRequests.end())
    {
        if (gFrameTime - existing_req->second < MIN_GROUP_PROPERTY_REQUEST_FREQ)
        {
            return true;
        }
        else
        {
            LLGroupMgr::getInstance()->mPropRequests.erase(existing_req);
        }
    }
    return false;
}

void LLGroupMgr::addPendingPropertyRequest(const LLUUID& id)
{
    LLGroupMgr::getInstance()->mPropRequests[id] = gFrameTime;
}

void LLGroupMgr::notifyObservers(LLGroupChange gc)
{
	for (group_map_t::iterator gi = mGroups.begin(); gi != mGroups.end(); ++gi)
	{
		LLUUID group_id = gi->first;
		if (gi->second->mChanged)
		{
			// notify LLGroupMgrObserver
			// Copy the map because observers may remove themselves on update
			observer_multimap_t observers = mObservers;

			// find all observers for this group id
			observer_multimap_t::iterator oi = observers.lower_bound(group_id);
			observer_multimap_t::iterator end = observers.upper_bound(group_id);
			for (; oi != end; ++oi)
			{
				oi->second->changed(gc);
			}
			gi->second->mChanged = false;


			// notify LLParticularGroupObserver
		    observer_map_t::iterator obs_it = mParticularObservers.find(group_id);
		    if(obs_it == mParticularObservers.end())
		        return;

		    observer_set_t& obs = obs_it->second;
		    for (observer_set_t::iterator ob_it = obs.begin(); ob_it != obs.end(); ++ob_it)
		    {
		        (*ob_it)->changed(group_id, gc);
		    }
		}
	}
}

void LLGroupMgr::addGroup(LLGroupMgrGroupData* group_datap)
{
	while (mGroups.size() >= MAX_CACHED_GROUPS)
	{
		// LRU: Remove the oldest un-observed group from cache until group size is small enough

		F32 oldest_access = LLFrameTimer::getTotalSeconds();
		group_map_t::iterator oldest_gi = mGroups.end();

		for (group_map_t::iterator gi = mGroups.begin(); gi != mGroups.end(); ++gi )
		{
			observer_multimap_t::iterator oi = mObservers.find(gi->first);
			if (oi == mObservers.end())
			{
				if (gi->second 
						&& (gi->second->getAccessTime() < oldest_access))
				{
					oldest_access = gi->second->getAccessTime();
					oldest_gi = gi;
				}
			}
		}
		
		if (oldest_gi != mGroups.end())
		{
			delete oldest_gi->second;
			mGroups.erase(oldest_gi);
		}
		else
		{
			// All groups must be currently open, none to remove.
			// Just add the new group anyway, but get out of this loop as it 
			// will never drop below max_cached_groups.
			break;
		}
	}

	mGroups[group_datap->getID()] = group_datap;
}


void LLGroupMgr::sendGroupPropertiesRequest(const LLUUID& group_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendGroupPropertiesRequest" << LL_ENDL;
	// This will happen when we get the reply
	//LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	
    if (LLGroupMgr::getInstance()->hasPendingPropertyRequest(group_id))
    {
        LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendGroupPropertiesRequest suppressed repeat for " << group_id << LL_ENDL;
        return;
    }
    LLGroupMgr::getInstance()->addPendingPropertyRequest(group_id);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GroupProfileRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());
	msg->nextBlock("GroupData");
	msg->addUUID("GroupID",group_id);
	gAgent.sendReliableMessage();
}

void LLGroupMgr::sendGroupMembersRequest(const LLUUID& group_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendGroupMembersRequest" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	if (group_datap->mMemberRequestID.isNull())
	{
		group_datap->removeMemberData();
		group_datap->mMemberRequestID.generate();

		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("GroupMembersRequest");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID",gAgent.getID());
		msg->addUUID("SessionID",gAgent.getSessionID());
		msg->nextBlock("GroupData");
		msg->addUUID("GroupID",group_id);
		msg->addUUID("RequestID",group_datap->mMemberRequestID);
		gAgent.sendReliableMessage();
	}
}


void LLGroupMgr::sendGroupRoleDataRequest(const LLUUID& group_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendGroupRoleDataRequest" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	if (group_datap->mRoleDataRequestID.isNull())
	{
		group_datap->removeRoleData();
		group_datap->mRoleDataRequestID.generate();

		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("GroupRoleDataRequest");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID",gAgent.getID());
		msg->addUUID("SessionID",gAgent.getSessionID());
		msg->nextBlock("GroupData");
		msg->addUUID("GroupID",group_id);
		msg->addUUID("RequestID",group_datap->mRoleDataRequestID);
		gAgent.sendReliableMessage();
	}
}

void LLGroupMgr::sendGroupRoleMembersRequest(const LLUUID& group_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendGroupRoleMembersRequest" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	
	if (group_datap->mRoleMembersRequestID.isNull())
	{
		// Don't send the request if we don't have all the member or role data
		if (!group_datap->isMemberDataComplete()
			|| !group_datap->isRoleDataComplete())
		{
			// *TODO: KLW FIXME: Should we start a member or role data request?
			LL_INFOS("GrpMgr") << " Pending: " << (group_datap->mPendingRoleMemberRequest ? "Y" : "N")
				<< " MemberDataComplete: " << (group_datap->mMemberDataComplete ? "Y" : "N")
				<< " RoleDataComplete: " << (group_datap->mRoleDataComplete ? "Y" : "N") << LL_ENDL;
			group_datap->mPendingRoleMemberRequest = true;
			return;
		}

		group_datap->removeRoleMemberData();
		group_datap->mRoleMembersRequestID.generate();

		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("GroupRoleMembersRequest");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID",gAgent.getID());
		msg->addUUID("SessionID",gAgent.getSessionID());
		msg->nextBlock("GroupData");
		msg->addUUID("GroupID",group_id);
		msg->addUUID("RequestID",group_datap->mRoleMembersRequestID);
		gAgent.sendReliableMessage();
	}
}

void LLGroupMgr::sendGroupTitlesRequest(const LLUUID& group_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendGroupTitlesRequest" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	
	group_datap->mTitles.clear();
	group_datap->mTitlesRequestID.generate();

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GroupTitlesRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());
	msg->addUUID("GroupID",group_id);
	msg->addUUID("RequestID",group_datap->mTitlesRequestID);

	gAgent.sendReliableMessage();
}

void LLGroupMgr::sendGroupTitleUpdate(const LLUUID& group_id, const LLUUID& title_role_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendGroupTitleUpdate" << LL_ENDL;

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GroupTitleUpdate");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());
	msg->addUUID("GroupID",group_id);
	msg->addUUID("TitleRoleID",title_role_id);

	gAgent.sendReliableMessage();

	// Save the change locally
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);
	for (std::vector<LLGroupTitle>::iterator iter = group_datap->mTitles.begin();
		 iter != group_datap->mTitles.end(); ++iter)
	{
		if (iter->mRoleID == title_role_id)
		{
			iter->mSelected = true;
		}
		else if (iter->mSelected)
		{
			iter->mSelected = false;
		}
	}
}

// static
void LLGroupMgr::sendCreateGroupRequest(const std::string& name,
										const std::string& charter,
										U8 show_in_list,
										const LLUUID& insignia,
										S32 membership_fee,
										bool open_enrollment,
										bool allow_publish,
										bool mature_publish)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("CreateGroupRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());

	msg->nextBlock("GroupData");
	msg->addString("Name",name);
	msg->addString("Charter",charter);
	msg->addBOOL("ShowInList",show_in_list);
	msg->addUUID("InsigniaID",insignia);
	msg->addS32("MembershipFee",membership_fee);
	msg->addBOOL("OpenEnrollment",open_enrollment);
	msg->addBOOL("AllowPublish",allow_publish);
	msg->addBOOL("MaturePublish",mature_publish);

	gAgent.sendReliableMessage();
}

void LLGroupMgr::sendUpdateGroupInfo(const LLUUID& group_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendUpdateGroupInfo" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);

	LLMessageSystem* msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_UpdateGroupInfo);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());

	msg->nextBlockFast(_PREHASH_GroupData);
	msg->addUUIDFast(_PREHASH_GroupID,group_datap->getID());
	msg->addStringFast(_PREHASH_Charter,group_datap->mCharter);
	msg->addBOOLFast(_PREHASH_ShowInList,group_datap->mShowInList);
	msg->addUUIDFast(_PREHASH_InsigniaID,group_datap->mInsigniaID);
	msg->addS32Fast(_PREHASH_MembershipFee,group_datap->mMembershipFee);
	msg->addBOOLFast(_PREHASH_OpenEnrollment,group_datap->mOpenEnrollment);
	msg->addBOOLFast(_PREHASH_AllowPublish,group_datap->mAllowPublish);
	msg->addBOOLFast(_PREHASH_MaturePublish,group_datap->mMaturePublish);

	gAgent.sendReliableMessage();

	// Not expecting a response, so let anyone else watching know the data has changed.
	group_datap->mChanged = true;
	notifyObservers(GC_PROPERTIES);
}

void LLGroupMgr::sendGroupRoleMemberChanges(const LLUUID& group_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendGroupRoleMemberChanges" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = createGroupData(group_id);

	if (group_datap->mRoleMemberChanges.empty()) return;

	LLMessageSystem* msg = gMessageSystem;

	bool start_message = true;
	for (LLGroupMgrGroupData::change_map_t::const_iterator citer = group_datap->mRoleMemberChanges.begin();
		 citer != group_datap->mRoleMemberChanges.end(); ++citer)
	{
		if (start_message)
		{
			msg->newMessage("GroupRoleChanges");
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID,gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
			msg->addUUIDFast(_PREHASH_GroupID,group_id);
			start_message = false;
		}
		msg->nextBlock("RoleChange");
		msg->addUUID("RoleID",citer->second.mRole);
		msg->addUUID("MemberID",citer->second.mMember);
		msg->addU32("Change",(U32)citer->second.mChange);

		if (msg->isSendFullFast())
		{
			gAgent.sendReliableMessage();
			start_message = true;
		}
	}

	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}

	group_datap->mRoleMemberChanges.clear();

	// Not expecting a response, so let anyone else watching know the data has changed.
	group_datap->mChanged = true;
	notifyObservers(GC_ROLE_MEMBER_DATA);
}

//static
void LLGroupMgr::sendGroupMemberJoin(const LLUUID& group_id)
{

	LLUIUsage::instance().logCommand("Group.Join");

	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_JoinGroupRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_GroupData);
	msg->addUUIDFast(_PREHASH_GroupID, group_id);

	gAgent.sendReliableMessage();
}

// member_role_pairs is <member_id,role_id>
// static
void LLGroupMgr::sendGroupMemberInvites(const LLUUID& group_id, std::map<LLUUID,LLUUID>& member_role_pairs)
{
	bool start_message = true;
	LLMessageSystem* msg = gMessageSystem;

	for (std::map<LLUUID,LLUUID>::iterator it = member_role_pairs.begin();
		 it != member_role_pairs.end(); ++it)
	{
		if (start_message)
		{
			msg->newMessage("InviteGroupRequest");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID",gAgent.getID());
			msg->addUUID("SessionID",gAgent.getSessionID());
			msg->nextBlock("GroupData");
			msg->addUUID("GroupID",group_id);
			start_message = false;
		}

		msg->nextBlock("InviteData");
		msg->addUUID("InviteeID",(*it).first);
		msg->addUUID("RoleID",(*it).second);

		if (msg->isSendFull())
		{
			gAgent.sendReliableMessage();
			start_message = true;
		}
	}

	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}
}

//static
void LLGroupMgr::sendGroupMemberEjects(const LLUUID& group_id,
									   uuid_vec_t& member_ids)
{
	bool start_message = true;
	LLMessageSystem* msg = gMessageSystem;

	LLGroupMgrGroupData* group_datap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!group_datap) return;

	for (uuid_vec_t::iterator it = member_ids.begin();
		 it != member_ids.end(); ++it)
	{
		LLUUID& ejected_member_id = (*it);

		// Can't use 'eject' to leave a group.
		if (ejected_member_id == gAgent.getID()) continue;

		// Make sure they are in the group, and we need the member data
		LLGroupMgrGroupData::member_list_t::iterator mit = group_datap->mMembers.find(ejected_member_id);
		if (mit != group_datap->mMembers.end())
		{
			// Add them to the message
			if (start_message)
			{
				msg->newMessage("EjectGroupMemberRequest");
				msg->nextBlock("AgentData");
				msg->addUUID("AgentID",gAgent.getID());
				msg->addUUID("SessionID",gAgent.getSessionID());
				msg->nextBlock("GroupData");
				msg->addUUID("GroupID",group_id);
				start_message = false;
			}
			
			msg->nextBlock("EjectData");
			msg->addUUID("EjecteeID",ejected_member_id);

			if (msg->isSendFull())
			{
				gAgent.sendReliableMessage();
				start_message = true;
			}

			LLGroupMemberData* member_data = (*mit).second;

			// Clean up groupmgr
			for (LLGroupMemberData::role_list_t::iterator rit = member_data->roleBegin();
				 rit != member_data->roleEnd(); ++rit)
			{
				if ((*rit).first.notNull() && (*rit).second!=0)
				{
					(*rit).second->removeMember(ejected_member_id);
				}
			}
			
			group_datap->mMembers.erase(ejected_member_id);
			
			// member_data was introduced and is used here instead of (*mit).second to avoid crash because of invalid iterator
			// It becomes invalid after line with erase above. EXT-4778
			delete member_data;
		}
	}

	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}

	group_datap->mMemberVersion.generate();
}

void LLGroupMgr::getGroupBanRequestCoro(std::string url, LLUUID groupId)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("groupMembersRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::string finalUrl = url + "?group_id=" + groupId.asString();

    LLSD result = httpAdapter->getAndSuspend(httpRequest, finalUrl);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("GrpMgr") << "Error receiving group member data " << LL_ENDL;
        return;
    }

    if (result.has("ban_list"))
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        // group ban data received
        processGroupBanRequest(result);
    }
}

void LLGroupMgr::postGroupBanRequestCoro(std::string url, LLUUID groupId,
    U32 action, uuid_vec_t banList, bool update)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("groupMembersRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);
    LLCore::HttpOptions::ptr_t httpOptions(new LLCore::HttpOptions);

    httpOptions->setFollowRedirects(false);

    httpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_LLSD_XML);


    std::string finalUrl = url + "?group_id=" + groupId.asString();

    LLSD postData = LLSD::emptyMap();
    postData["ban_action"] = (LLSD::Integer)action;
    // Add our list of potential banned residents to the list
    postData["ban_ids"] = LLSD::emptyArray();
    LLSD banEntry;

    uuid_vec_t::const_iterator it = banList.begin();
    for (; it != banList.end(); ++it)
    {
        banEntry = (*it);
        postData["ban_ids"].append(banEntry);
    }

    LL_WARNS() << "post: " << ll_pretty_print_sd(postData) << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, finalUrl, postData, httpOptions, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("GrpMgr") << "Error posting group member data " << LL_ENDL;
        return;
    }

    if (result.has("ban_list"))
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        // group ban data received
        processGroupBanRequest(result);
    }

    if (update)
    {
        getGroupBanRequestCoro(url, groupId);
    }
}

void LLGroupMgr::sendGroupBanRequest(	EBanRequestType request_type, 
										const LLUUID& group_id, 
										U32 ban_action, /* = BAN_NO_ACTION */
										const std::vector<LLUUID> &ban_list) /* = std::vector<LLUUID>() */
{
	LLViewerRegion* currentRegion = gAgent.getRegion();
	if(!currentRegion)
	{
		LL_WARNS("GrpMgr") << "Agent does not have a current region. Uh-oh!" << LL_ENDL;
		return;
	}

	// Check to make sure we have our capabilities
	if(!currentRegion->capabilitiesReceived())
	{
		LL_WARNS("GrpMgr") << " Capabilities not received!" << LL_ENDL;
		return;
	}

	// Get our capability
	std::string cap_url =  currentRegion->getCapability("GroupAPIv1");
	if(cap_url.empty())
	{
		return;
	}

    U32 action = ban_action & ~BAN_UPDATE;
    bool update = ((ban_action & BAN_UPDATE) == BAN_UPDATE);

    switch (request_type)
    {
    case REQUEST_GET:
        LLCoros::instance().launch("LLGroupMgr::getGroupBanRequestCoro",
            boost::bind(&LLGroupMgr::getGroupBanRequestCoro, this, cap_url, group_id));
        break;
    case REQUEST_POST:
        LLCoros::instance().launch("LLGroupMgr::postGroupBanRequestCoro",
            boost::bind(&LLGroupMgr::postGroupBanRequestCoro, this, cap_url, group_id, 
            action, ban_list, update));
        break;
    case REQUEST_PUT:
    case REQUEST_DEL:
        break;
    }
}

void LLGroupMgr::processGroupBanRequest(const LLSD& content)
{
	// Did we get anything in content?
	if(!content.size())
	{
		LL_WARNS("GrpMgr") << "No group member data received." << LL_ENDL;
		return;
	}

	LLUUID group_id = content["group_id"].asUUID();
	
	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(group_id);
	if (!gdatap)
		return;

	gdatap->clearBanList();
	LLSD::map_const_iterator i		= content["ban_list"].beginMap();
	LLSD::map_const_iterator iEnd	= content["ban_list"].endMap();
	for(;i != iEnd; ++i)
	{
		const LLUUID ban_id(i->first);
		LLSD ban_entry(i->second);
		
		LLGroupBanData ban_data;
		if(ban_entry.has("ban_date"))
		{
			ban_data.mBanDate = ban_entry["ban_date"].asDate();
			// TODO: Ban Reason
		}

		gdatap->createBanEntry(ban_id, ban_data);
	}

	gdatap->mChanged = true;
	LLGroupMgr::getInstance()->notifyObservers(GC_BANLIST);
}

void LLGroupMgr::groupMembersRequestCoro(std::string url, LLUUID groupId)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("groupMembersRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);

    mMemberRequestInFlight = true;

    LLSD postData = LLSD::emptyMap();
    postData["group_id"] = groupId;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("GrpMgr") << "Error receiving group member data " << LL_ENDL;
        mMemberRequestInFlight = false;
        return;
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    LLGroupMgr::processCapGroupMembersRequest(result);
    mMemberRequestInFlight = false;
}

void LLGroupMgr::sendCapGroupMembersRequest(const LLUUID& group_id)
{
    static U32 lastGroupMemberRequestFrame = 0;

	// Have we requested the information already this frame?
    // Todo: make this per group, we can invite to one group and simultaneously be checking another one
    if ((lastGroupMemberRequestFrame == gFrameCount) || (mMemberRequestInFlight))
		return;
	
	LLViewerRegion* currentRegion = gAgent.getRegion();
	// Thank you FS:Ansariel!
	if(!currentRegion)
	{
		LL_WARNS("GrpMgr") << "Agent does not have a current region. Uh-oh!" << LL_ENDL;
		return;
	}

	// Check to make sure we have our capabilities
	if(!currentRegion->capabilitiesReceived())
	{
		LL_WARNS("GrpMgr") << " Capabilities not received!" << LL_ENDL;
		return;
	}

	// Get our capability
	std::string cap_url =  currentRegion->getCapability("GroupMemberData");

	// Thank you FS:Ansariel!
	if(cap_url.empty())
	{
		LL_INFOS("GrpMgr") << "Region has no GroupMemberData capability.  Falling back to UDP fetch." << LL_ENDL;
		sendGroupMembersRequest(group_id);
		return;
	}

    LLGroupMgrGroupData* group_datap = createGroupData(group_id); //make sure group exists
    group_datap->mMemberRequestID.generate(); // mark as pending

    lastGroupMemberRequestFrame = gFrameCount;

    LLCoros::instance().launch("LLGroupMgr::groupMembersRequestCoro",
        boost::bind(&LLGroupMgr::groupMembersRequestCoro, this, cap_url, group_id));
}


void LLGroupMgr::processCapGroupMembersRequest(const LLSD& content)
{
	// Did we get anything in content?
	if(!content.size())
	{
		LL_DEBUGS("GrpMgr") << "No group member data received." << LL_ENDL;
		return;
	}

	LLUUID group_id = content["group_id"].asUUID();

	LLGroupMgrGroupData* group_datap = getGroupData(group_id);
	if(!group_datap)
	{
		LL_WARNS("GrpMgr") << "Received incorrect, possibly stale, group or request id" << LL_ENDL;
		return;
	}

	// If we have no members, there's no reason to do anything else
	S32	num_members	= content["member_count"];
	if (num_members < 1)
	{
		LL_INFOS("GrpMgr") << "Received empty group members list for group id: " << group_id.asString() << LL_ENDL;
		// Set mMemberDataComplete for correct handling of empty responses. See MAINT-5237
		group_datap->mMemberDataComplete = true;
		group_datap->mChanged = true;
		LLGroupMgr::getInstance()->notifyObservers(GC_MEMBER_DATA);
		return;
	}
	
	group_datap->mMemberCount = num_members;

	LLSD	member_list	= content["members"];
	LLSD	titles		= content["titles"];
	LLSD	defaults	= content["defaults"];

	std::string online_status;
	std::string title;
	S32			contribution;
	U64			member_powers;
	// If this is changed to a bool, make sure to change the LLGroupMemberData constructor
	bool		is_owner;

	// Compute this once, rather than every time.
	U64	default_powers	= llstrtou64(defaults["default_powers"].asString().c_str(), NULL, 16);

	LLSD::map_const_iterator member_iter_start	= member_list.beginMap();
	LLSD::map_const_iterator member_iter_end	= member_list.endMap();
	for( ; member_iter_start != member_iter_end; ++member_iter_start)
	{
		// Reset defaults
		online_status	= "unknown";
		title			= titles[0].asString();
		contribution	= 0;
		member_powers	= default_powers;
		is_owner		= false;

		const LLUUID member_id(member_iter_start->first);
		LLSD member_info = member_iter_start->second;
		
		if(member_info.has("last_login"))
		{
			online_status = member_info["last_login"].asString();
			if(online_status == "Online")
				online_status = LLTrans::getString("group_member_status_online");
			else
				formatDateString(online_status);
		}

		if(member_info.has("title"))
			title = titles[member_info["title"].asInteger()].asString();

		if(member_info.has("powers"))
			member_powers = llstrtou64(member_info["powers"].asString().c_str(), NULL, 16);

		if(member_info.has("donated_square_meters"))
			contribution = member_info["donated_square_meters"];

		if(member_info.has("owner"))
			is_owner = true;

		LLGroupMemberData* data = new LLGroupMemberData(member_id, 
			contribution, 
			member_powers, 
			title,
			online_status,
			is_owner);

		LLGroupMemberData* member_old = group_datap->mMembers[member_id];
		if (member_old && group_datap->mRoleMemberDataComplete)
		{
			LLGroupMemberData::role_list_t::iterator rit = member_old->roleBegin();
			LLGroupMemberData::role_list_t::iterator end = member_old->roleEnd();

			for ( ; rit != end; ++rit)
			{
				data->addRole((*rit).first,(*rit).second);
			}
		}
		else
		{
			group_datap->mRoleMemberDataComplete = false;
		}

		group_datap->mMembers[member_id] = data;
	}

	group_datap->mMemberVersion.generate();

	// Technically, we have this data, but to prevent completely overhauling
	// this entire system (it would be nice, but I don't have the time), 
	// I'm going to be dumb and just call services I most likely don't need 
	// with the thought being that the system might need it to be done.
	// 
	// TODO:
	// Refactor to reduce multiple calls for data we already have.
	if(group_datap->mTitles.size() < 1)
		sendGroupTitlesRequest(group_id);


	group_datap->mMemberDataComplete = true;
	group_datap->mMemberRequestID.setNull();
	// Make the role-member data request
	if (group_datap->mPendingRoleMemberRequest || !group_datap->mRoleMemberDataComplete)
	{
		group_datap->mPendingRoleMemberRequest = false;
		sendGroupRoleMembersRequest(group_id);
	}

	group_datap->mChanged = true;
	notifyObservers(GC_MEMBER_DATA);

}


void LLGroupMgr::sendGroupRoleChanges(const LLUUID& group_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::sendGroupRoleChanges" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = getGroupData(group_id);

	if (group_datap && group_datap->pendingRoleChanges())
	{
		group_datap->sendRoleChanges();
	
		// Not expecting a response, so let anyone else watching know the data has changed.
		group_datap->mChanged = true;
		notifyObservers(GC_ROLE_DATA);
	}
}

void LLGroupMgr::cancelGroupRoleChanges(const LLUUID& group_id)
{
	LL_DEBUGS("GrpMgr") << "LLGroupMgr::cancelGroupRoleChanges" << LL_ENDL;
	LLGroupMgrGroupData* group_datap = getGroupData(group_id);

	if (group_datap) group_datap->cancelRoleChanges();
}

//static
bool LLGroupMgr::parseRoleActions(const std::string& xml_filename)
{
	LLXMLNodePtr root;

	bool success = LLUICtrlFactory::getLayeredXMLNode(xml_filename, root);	
	
	if (!success || !root || !root->hasName( "role_actions" ))
	{
		LL_ERRS() << "Problem reading UI role_actions file: " << xml_filename << LL_ENDL;
		return false;
	}

	LLXMLNodeList role_list;

	root->getChildren("action_set", role_list, false);
	
	for (LLXMLNodeList::iterator role_iter = role_list.begin(); role_iter != role_list.end(); ++role_iter)
	{
		LLXMLNodePtr action_set = role_iter->second;

		LLRoleActionSet* role_action_set = new LLRoleActionSet();
		LLRoleAction* role_action_data = new LLRoleAction();
		
		// name=
		std::string action_set_name;
		if (action_set->getAttributeString("name", action_set_name))
		{
			LL_DEBUGS("GrpMgr") << "Loading action set " << action_set_name << LL_ENDL;
			role_action_data->mName = action_set_name;
		}
		else
		{
			LL_WARNS() << "Unable to parse action set with no name" << LL_ENDL;
			delete role_action_set;
			delete role_action_data;
			continue;
		}
		// description=
		std::string set_description;
		if (action_set->getAttributeString("description", set_description))
		{
			role_action_data->mDescription = set_description;
		}
		// long description=
		std::string set_longdescription;
		if (action_set->getAttributeString("longdescription", set_longdescription))
		{
			role_action_data->mLongDescription = set_longdescription;
		}

		// power mask=
		U64 set_power_mask = 0;

		LLXMLNodeList action_list;
		LLXMLNodeList::iterator action_iter;

		action_set->getChildren("action", action_list, false);

		for (action_iter = action_list.begin(); action_iter != action_list.end(); ++action_iter)
		{
			LLXMLNodePtr action = action_iter->second;

			LLRoleAction* role_action = new LLRoleAction();

			// name=
			std::string action_name;
			if (action->getAttributeString("name", action_name))
			{
				LL_DEBUGS("GrpMgr") << "Loading action " << action_name << LL_ENDL;
				role_action->mName = action_name;
			}
			else
			{
				LL_WARNS() << "Unable to parse action with no name" << LL_ENDL;
				delete role_action;
				continue;
			}
			// description=
			std::string description;
			if (action->getAttributeString("description", description))
			{
				role_action->mDescription = description;
			}
			// long description=
			std::string longdescription;
			if (action->getAttributeString("longdescription", longdescription))
			{
				role_action->mLongDescription = longdescription;
			}
			// description=
			S32 power_bit = 0;
			if (action->getAttributeS32("value", power_bit))
			{
				if (0 <= power_bit && power_bit < 64)
				{
					role_action->mPowerBit = 0x1LL << power_bit;
				}
			}

			set_power_mask |= role_action->mPowerBit;
	
			role_action_set->mActions.push_back(role_action);
		}

		role_action_data->mPowerBit = set_power_mask;
		role_action_set->mActionSetData = role_action_data;

		LLGroupMgr::getInstance()->mRoleActionSets.push_back(role_action_set);
	}
	return true;
}

// static
void LLGroupMgr::debugClearAllGroups(void*)
{
	LLGroupMgr::getInstance()->clearGroups();
	LLGroupMgr::parseRoleActions("role_actions.xml");
}


