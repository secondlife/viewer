/** 
 * @file lluserrelations.cpp
 * @author Phoenix
 * @date 2006-10-12
 * @brief Implementation of a simple cache of user relations.
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
