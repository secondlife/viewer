/** 
 * @file lluserrelations.cpp
 * @author Phoenix
 * @date 2006-10-12
 * @brief Implementation of a simple cache of user relations.
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

#include "linden_common.h"
#include "lluserrelations.h"

// static
const U8 LLRelationship::GRANTED_VISIBLE_MASK = LLRelationship::GRANT_MODIFY_OBJECTS | LLRelationship::GRANT_MAP_LOCATION;
const LLRelationship LLRelationship::DEFAULT_RELATIONSHIP = LLRelationship(GRANT_ONLINE_STATUS, GRANT_ONLINE_STATUS, false);

LLRelationship::LLRelationship() :
    mGrantToAgent(0),
    mGrantFromAgent(0),
    mChangeSerialNum(0),
    mIsOnline(false)
{
}

LLRelationship::LLRelationship(S32 grant_to, S32 grant_from, bool is_online) :
    mGrantToAgent(grant_to),
    mGrantFromAgent(grant_from),
    mChangeSerialNum(0),
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
    mChangeSerialNum++;
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
    mChangeSerialNum++;
}

void LLRelationship::revokeRights(S32 to_agent, S32 from_agent)
{
    mGrantToAgent &= ~to_agent;
    mGrantFromAgent &= ~from_agent;
    mChangeSerialNum++;
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
