/** 
 * @file llregionflags.h
 * @brief Flags that are sent in the statistics message region_flags field.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLREGIONFLAGS_H
#define LL_LLREGIONFLAGS_H

// Can you be hurt here? Should health be on?
const U64 REGION_FLAGS_ALLOW_DAMAGE				= (1 << 0);

// Can you make landmarks here?
const U64 REGION_FLAGS_ALLOW_LANDMARK			= (1 << 1);

// Do we reset the home position when someone teleports away from here?
const U64 REGION_FLAGS_ALLOW_SET_HOME			= (1 << 2);

// Do we reset the home position when someone teleports away from here?
const U64 REGION_FLAGS_RESET_HOME_ON_TELEPORT	= (1 << 3);

// Does the sun move?
const U64 REGION_FLAGS_SUN_FIXED				= (1 << 4);

// Can't change the terrain heightfield, even on owned parcels,
// but can plant trees and grass.
const U64 REGION_FLAGS_BLOCK_TERRAFORM			= (1 << 6);

// Can't release, sell, or buy land.
const U64 REGION_FLAGS_BLOCK_LAND_RESELL		= (1 << 7);

// All content wiped once per night
const U64 REGION_FLAGS_SANDBOX					= (1 << 8);
const U64 REGION_FLAGS_SKIP_COLLISIONS			= (1 << 12); // Pin all non agent rigid bodies
const U64 REGION_FLAGS_SKIP_SCRIPTS				= (1 << 13);
const U64 REGION_FLAGS_SKIP_PHYSICS				= (1 << 14); // Skip all physics
const U64 REGION_FLAGS_EXTERNALLY_VISIBLE		= (1 << 15);
const U64 REGION_FLAGS_ALLOW_RETURN_ENCROACHING_OBJECT = (1 << 16);
const U64 REGION_FLAGS_ALLOW_RETURN_ENCROACHING_ESTATE_OBJECT = (1 << 17);
const U64 REGION_FLAGS_BLOCK_DWELL				= (1 << 18);

// Is flight allowed?
const U64 REGION_FLAGS_BLOCK_FLY				= (1 << 19);	

// Is direct teleport (p2p) allowed?
const U64 REGION_FLAGS_ALLOW_DIRECT_TELEPORT	= (1 << 20);

// Is there an administrative override on scripts in the region at the
// moment. This is the similar skip scripts, except this flag is
// presisted in the database on an estate level.
const U64 REGION_FLAGS_ESTATE_SKIP_SCRIPTS		= (1 << 21);

const U64 REGION_FLAGS_RESTRICT_PUSHOBJECT		= (1 << 22);

const U64 REGION_FLAGS_DENY_ANONYMOUS			= (1 << 23);

const U64 REGION_FLAGS_ALLOW_PARCEL_CHANGES		= (1 << 26);

const U64 REGION_FLAGS_ALLOW_VOICE = (1 << 28);

const U64 REGION_FLAGS_BLOCK_PARCEL_SEARCH = (1 << 29);
const U64 REGION_FLAGS_DENY_AGEUNVERIFIED	= (1 << 30);

const U64 REGION_FLAGS_DEFAULT = REGION_FLAGS_ALLOW_LANDMARK |
								 REGION_FLAGS_ALLOW_SET_HOME |
                                 REGION_FLAGS_ALLOW_PARCEL_CHANGES |
                                 REGION_FLAGS_ALLOW_VOICE;


const U64 REGION_FLAGS_PRELUDE_SET = REGION_FLAGS_RESET_HOME_ON_TELEPORT;
const U64 REGION_FLAGS_PRELUDE_UNSET = REGION_FLAGS_ALLOW_LANDMARK 
									   | REGION_FLAGS_ALLOW_SET_HOME;

const U64 REGION_FLAGS_ESTATE_MASK = REGION_FLAGS_EXTERNALLY_VISIBLE
									 | REGION_FLAGS_SUN_FIXED
									 | REGION_FLAGS_DENY_ANONYMOUS
									 | REGION_FLAGS_DENY_AGEUNVERIFIED;

inline BOOL is_prelude( U64 flags )
{
	// definition of prelude does not depend on fixed-sun
	return 0 == (flags & REGION_FLAGS_PRELUDE_UNSET)
		   && 0 != (flags & REGION_FLAGS_PRELUDE_SET);
}

inline U64 set_prelude_flags(U64 flags)
{
	// also set the sun-fixed flag
	return ((flags & ~REGION_FLAGS_PRELUDE_UNSET)
			| (REGION_FLAGS_PRELUDE_SET | REGION_FLAGS_SUN_FIXED));
}

inline U64 unset_prelude_flags(U64 flags)
{
	// also unset the fixed-sun flag
	return ((flags | REGION_FLAGS_PRELUDE_UNSET) 
			& ~(REGION_FLAGS_PRELUDE_SET | REGION_FLAGS_SUN_FIXED));
}

// Region protocols
const U64 REGION_PROTOCOLS_AGENT_APPEARANCE_SERVICE = (1 << 0);

// estate constants. Need to match first few etries in indra.estate table.
const U32 ESTATE_ALL = 0; // will not match in db, reserved key for logic
const U32 ESTATE_MAINLAND = 1;
const U32 ESTATE_ORIENTATION = 2;
const U32 ESTATE_INTERNAL = 3;
const U32 ESTATE_SHOWCASE = 4;
const U32 ESTATE_TEEN = 5;
const U32 ESTATE_LAST_LINDEN = 5; // last linden owned/managed estate

// for EstateOwnerRequest, setaccess message
const U32 ESTATE_ACCESS_ALLOWED_AGENTS	= 1 << 0;
const U32 ESTATE_ACCESS_ALLOWED_GROUPS	= 1 << 1;
const U32 ESTATE_ACCESS_BANNED_AGENTS	= 1 << 2;
const U32 ESTATE_ACCESS_MANAGERS		= 1 << 3;

//maximum number of access list entries we can fit in one packet
const S32 ESTATE_ACCESS_MAX_ENTRIES_PER_PACKET = 63;

// for reply to "getinfo", don't need to forward to all sims in estate
const U32 ESTATE_ACCESS_SEND_TO_AGENT_ONLY = 1 << 4;

const U32 ESTATE_ACCESS_ALL = ESTATE_ACCESS_ALLOWED_AGENTS
							  | ESTATE_ACCESS_ALLOWED_GROUPS
							  | ESTATE_ACCESS_BANNED_AGENTS
							  | ESTATE_ACCESS_MANAGERS;

// for EstateOwnerRequest, estateaccessdelta message
const U32 ESTATE_ACCESS_APPLY_TO_ALL_ESTATES		= 1 << 0;
const U32 ESTATE_ACCESS_APPLY_TO_MANAGED_ESTATES	= 1 << 1;

const U32 ESTATE_ACCESS_ALLOWED_AGENT_ADD			= 1 << 2;
const U32 ESTATE_ACCESS_ALLOWED_AGENT_REMOVE		= 1 << 3;
const U32 ESTATE_ACCESS_ALLOWED_GROUP_ADD			= 1 << 4;
const U32 ESTATE_ACCESS_ALLOWED_GROUP_REMOVE		= 1 << 5;
const U32 ESTATE_ACCESS_BANNED_AGENT_ADD			= 1 << 6;
const U32 ESTATE_ACCESS_BANNED_AGENT_REMOVE			= 1 << 7;
const U32 ESTATE_ACCESS_MANAGER_ADD					= 1 << 8;
const U32 ESTATE_ACCESS_MANAGER_REMOVE				= 1 << 9;
const U32 ESTATE_ACCESS_NO_REPLY                                  = 1 << 10;

const S32 ESTATE_MAX_MANAGERS = 10;
const S32 ESTATE_MAX_ACCESS_IDS = 500;	// max for access, banned
const S32 ESTATE_MAX_GROUP_IDS = (S32) ESTATE_ACCESS_MAX_ENTRIES_PER_PACKET;

// 'Sim Wide Delete' flags
const U32 SWD_OTHERS_LAND_ONLY		= (1 << 0);
const U32 SWD_ALWAYS_RETURN_OBJECTS = (1 << 1);
const U32 SWD_SCRIPTED_ONLY			= (1 << 2);

#endif


