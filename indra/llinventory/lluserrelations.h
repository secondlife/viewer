/** 
 * @file lluserrelations.h
 * @author Phoenix
 * @date 2006-10-12
 * @brief Declaration of a class for handling granted rights.
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

#ifndef LL_LLUSERRELAIONS_H
#define LL_LLUSERRELAIONS_H

#include <map>
#include "lluuid.h"

/** 
 * @class LLRelationship
 *
 * This class represents a relationship between two agents, where the
 * related agent is stored and the other agent is the relationship is
 * implicit by container ownership.
 * This is merely a cache of this information used by the sim 
 * and viewer.
 *
 * You are expected to use this in a map or similar structure, eg:
 * typedef std::map<LLUUID, LLRelationship> agent_relationship_map;
 */
class LLRelationship
{
public:
	/**
	 * @brief Constructors.
	 */
	LLRelationship();
	LLRelationship(S32 grant_to, S32 grant_from, bool is_online);
	
	static const LLRelationship DEFAULT_RELATIONSHIP;

    /** 
	 * @name Status functionality
	 *
	 * I thought it would be keen to have a generic status interface,
	 * but the only thing we currently cache is online status. As this
	 * assumption changes, this API may evolve.
	 */
	//@{
	/**
	 * @brief Does this instance believe the related agent is currently
	 * online or available.
	 *
	 * NOTE: This API may be deprecated if there is any transient status
	 * other than online status, for example, away/busy/etc.
	 *
	 * This call does not check any kind of central store or make any
	 * deep information calls - it simply checks a cache of online
	 * status.
	 * @return Returns true if this relationship believes the agent is
	 * online.
	 */
	bool isOnline() const;

	/**
	 * @brief Set the online status.
	 *
	 * NOTE: This API may be deprecated if there is any transient status
	 * other than online status.
	 * @param is_online Se the online status
	 */
	void online(bool is_online);
	//@}

	/* @name Granted rights
	 */
	//@{
	/** 
	 * @brief Anonymous enumeration for specifying rights.
	 */
	enum
	{
		GRANT_NONE = 0x0,
		GRANT_ONLINE_STATUS = 0x1,
		GRANT_MAP_LOCATION = 0x2,
		GRANT_MODIFY_OBJECTS = 0x4,
	};

	/**
	 * ???
	 */
	static const U8 GRANTED_VISIBLE_MASK;

	/**
	 * @brief Check for a set of rights granted to agent.
	 *
	 * @param rights A bitfield to check for rights.
	 * @return Returns true if all rights have been granted.
	 */
	bool isRightGrantedTo(S32 rights) const;

	/**
	 * @brief Check for a set of rights granted from an agent.
	 *
	 * @param rights A bitfield to check for rights.
	 * @return Returns true if all rights have been granted.
	 */
	bool isRightGrantedFrom(S32 rights) const;

	/**
	 * @brief Get the rights granted to the other agent.
	 *
	 * @return Returns the bitmask of granted rights.
	 */
	S32 getRightsGrantedTo() const;

	/**
	 * @brief Get the rights granted from the other agent.
	 *
	 * @return Returns the bitmask of granted rights.
	 */
	S32 getRightsGrantedFrom() const;

	void setRightsTo(S32 to_agent) { mGrantToAgent = to_agent; mChangeSerialNum++; }
	void setRightsFrom(S32 from_agent) { mGrantFromAgent = from_agent; mChangeSerialNum++;}

	/**
	 * @brief Get the change count for this agent
	 *
	 * Every change to rights will increment the serial number
	 * allowing listeners to determine when a relationship value is actually new
	 *
	 * @return change serial number for relationship
	 */
	S32 getChangeSerialNum() const { return mChangeSerialNum; }

	/**
	 * @brief Grant a set of rights.
	 *
	 * Any bit which is set will grant that right if it is set in the
	 * instance. You can pass in LLGrantedRights::NONE to not change
	 * that field.
	 * @param to_agent The rights to grant to agent_id.
	 * @param from_agent The rights granted from agent_id.
	 */
	void grantRights(S32 to_agent, S32 from_agent);
	
	/** 
	 * @brief Revoke a set of rights.
	 *
	 * Any bit which is set will revoke that right if it is set in the
	 * instance. You can pass in LLGrantedRights::NONE to not change
	 * that field.
	 * @param to_agent The rights to grant to agent_id.
	 * @param from_agent The rights granted from agent_id.
	 */
	void revokeRights(S32 to_agent, S32 from_agent);
	//@}

protected:
	S32 mGrantToAgent;
	S32 mGrantFromAgent;
	S32 mChangeSerialNum;
	bool mIsOnline;
};

#endif // LL_LLUSERRELAIONS_H
