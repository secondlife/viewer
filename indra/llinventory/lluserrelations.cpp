/** 
 * @file lluserrelations.cpp
 * @author Phoenix
 * @date 2006-10-12
 * @brief Implementation of a simple cache of user relations.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "lluserrelations.h"

// static
const U8 LLRelationship::GRANTED_VISIBLE_MASK = LLRelationship::GRANT_MODIFY_OBJECTS | LLRelationship::GRANT_MAP_LOCATION;
const LLRelationship LLRelationship::DEFAULT_RELATIONSHIP = LLRelationship(GRANT_ONLINE_STATUS, GRANT_ONLINE_STATUS, false);

LLRelationship::LLRelationship() :
	mGrantToAgent(0),
	mGrantFromAgent(0),
	mIsOnline(false)
{
}

LLRelationship::LLRelationship(S32 grant_to, S32 grant_from, bool is_online) :
	mGrantToAgent(grant_to),
	mGrantFromAgent(grant_from),
	mIsOnline(is_online)
{
}

bool LLRelationship::isOnline() const
{
	return mIsOnline;
}

void LLRelationship::online(bool is_online)
{
	mIsOnline = is_online;
}

bool LLRelationship::isRightGrantedTo(S32 rights) const
{
	return ((mGrantToAgent & rights) == rights);
}

bool LLRelationship::isRightGrantedFrom(S32 rights) const
{
	return ((mGrantFromAgent & rights) == rights);
}

S32 LLRelationship::getRightsGrantedTo() const
{
	return mGrantToAgent;
}

S32 LLRelationship::getRightsGrantedFrom() const
{
	return mGrantFromAgent;
}

void LLRelationship::grantRights(S32 to_agent, S32 from_agent)
{
	mGrantToAgent |= to_agent;
	mGrantFromAgent |= from_agent;
}

void LLRelationship::revokeRights(S32 to_agent, S32 from_agent)
{
	mGrantToAgent &= ~to_agent;
	mGrantFromAgent &= ~from_agent;
}



/*
bool LLGrantedRights::getNextRights(
	LLUUID& agent_id,
	S32& to_agent,
	S32& from_agent) const
{
	rights_map_t::const_iterator iter = mRights.upper_bound(agent_id);
	if(iter == mRights.end()) return false;
	agent_id = (*iter).first;
	to_agent = (*iter).second.mToAgent;
	from_agent = (*iter).second.mFromAgent;
	return true;
}
*/
