/** 
 * @file lluserrelations_tut.cpp
 * @author Phoenix
 * @date 2006-10-12
 * @brief Unit tests for the LLRelationship class.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include <tut/tut.h>

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
