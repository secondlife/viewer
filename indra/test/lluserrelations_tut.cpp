/** 
 * @file lluserrelations_tut.cpp
 * @author Phoenix
 * @date 2006-10-12
 * @brief Unit tests for the LLRelationship class.
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

#include <tut/tut.hpp>

#include "linden_common.h"
#include "lluserrelations.h"

namespace tut
{
    struct user_relationship
    {
        LLRelationship mRelationship;
    };
    typedef test_group<user_relationship> user_relationship_t;
    typedef user_relationship_t::object user_relationship_object_t;
    tut::user_relationship_t tut_user_relationship("relationships");

    template<> template<>
    void user_relationship_object_t::test<1>()
    {
        // Test the default construction
        ensure(
            "No granted rights to",
            !mRelationship.isRightGrantedTo(
                LLRelationship::GRANT_ONLINE_STATUS));
        ensure(
            "No granted rights from",
            !mRelationship.isRightGrantedFrom(
                LLRelationship::GRANT_ONLINE_STATUS));
        ensure("No online status",!mRelationship.isOnline());
    }

    template<> template<>
    void user_relationship_object_t::test<2>()
    {
        // Test some granting
        mRelationship.grantRights(
            LLRelationship::GRANT_ONLINE_STATUS,
            LLRelationship::GRANT_MODIFY_OBJECTS);
        ensure(
            "Granted rights to has online",
            mRelationship.isRightGrantedTo(
                LLRelationship::GRANT_ONLINE_STATUS));
        ensure(
            "Granted rights from does not have online",
            !mRelationship.isRightGrantedFrom(
                LLRelationship::GRANT_ONLINE_STATUS));
        ensure(
            "Granted rights to does not have modify",
            !mRelationship.isRightGrantedTo(
                LLRelationship::GRANT_MODIFY_OBJECTS));
        ensure(
            "Granted rights from has modify",
            mRelationship.isRightGrantedFrom(
                LLRelationship::GRANT_MODIFY_OBJECTS));
    }

    template<> template<>
    void user_relationship_object_t::test<3>()
    {
        // Test revoking
        mRelationship.grantRights(
            LLRelationship::GRANT_ONLINE_STATUS
            | LLRelationship::GRANT_MAP_LOCATION,
            LLRelationship::GRANT_ONLINE_STATUS);
        ensure(
            "Granted rights to has online and map",
            mRelationship.isRightGrantedTo(
                LLRelationship::GRANT_ONLINE_STATUS
                | LLRelationship::GRANT_MAP_LOCATION));
        ensure(
            "Granted rights from has online",
            mRelationship.isRightGrantedFrom(
                LLRelationship::GRANT_ONLINE_STATUS));

        mRelationship.revokeRights(
            LLRelationship::GRANT_MAP_LOCATION,
            LLRelationship::GRANT_NONE);
        ensure(
            "Granted rights revoked map",
            !mRelationship.isRightGrantedTo(
                LLRelationship::GRANT_ONLINE_STATUS
                | LLRelationship::GRANT_MAP_LOCATION));
        ensure(
            "Granted rights revoked still has online",
            mRelationship.isRightGrantedTo(
                LLRelationship::GRANT_ONLINE_STATUS));

        mRelationship.grantRights(
            LLRelationship::GRANT_NONE,
            LLRelationship::GRANT_MODIFY_OBJECTS);
        ensure(
            "Granted rights from still has online",
            mRelationship.isRightGrantedFrom(
                LLRelationship::GRANT_ONLINE_STATUS));
        ensure(
            "Granted rights from has full grant",
            mRelationship.isRightGrantedFrom(
                LLRelationship::GRANT_ONLINE_STATUS
                | LLRelationship::GRANT_MODIFY_OBJECTS));
        mRelationship.revokeRights(
            LLRelationship::GRANT_NONE,
            LLRelationship::GRANT_MODIFY_OBJECTS);
        ensure(
            "Granted rights from still has online",
            mRelationship.isRightGrantedFrom(
                LLRelationship::GRANT_ONLINE_STATUS));
        ensure(
            "Granted rights from no longer modify",
            !mRelationship.isRightGrantedFrom(
                LLRelationship::GRANT_MODIFY_OBJECTS)); 
    }

    template<> template<>
    void user_relationship_object_t::test<4>()
    {
        ensure("No online status", !mRelationship.isOnline());
        mRelationship.online(true);
        ensure("Online status", mRelationship.isOnline());
        mRelationship.online(false);
        ensure("No online status", !mRelationship.isOnline());
    }

/*
    template<> template<>
    void user_relationship_object_t::test<>()
    {
    }
*/
}
