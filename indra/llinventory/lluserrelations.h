/** 
 * @file lluserrelations.h
 * @author Phoenix
 * @date 2006-10-12
 * @brief Declaration of a class for handling granted rights.
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
