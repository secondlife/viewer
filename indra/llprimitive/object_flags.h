/** 
 * @file object_flags.h
 * @brief Flags for object creation and transmission
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_OBJECT_FLAGS_H
#define LL_OBJECT_FLAGS_H

// downstream flags from sim->viewer
const U32   FLAGS_USE_PHYSICS          = (1U << 0);
const U32   FLAGS_CREATE_SELECTED      = (1U << 1);
const U32   FLAGS_OBJECT_MODIFY        = (1U << 2);
const U32   FLAGS_OBJECT_COPY          = (1U << 3);
const U32   FLAGS_OBJECT_ANY_OWNER     = (1U << 4);
const U32   FLAGS_OBJECT_YOU_OWNER     = (1U << 5);
const U32   FLAGS_SCRIPTED             = (1U << 6);
const U32   FLAGS_HANDLE_TOUCH         = (1U << 7);
const U32   FLAGS_OBJECT_MOVE          = (1U << 8);
const U32   FLAGS_TAKES_MONEY          = (1U << 9);
const U32   FLAGS_PHANTOM              = (1U << 10);
const U32   FLAGS_INVENTORY_EMPTY      = (1U << 11);

const U32   FLAGS_AFFECTS_NAVMESH      = (1U << 12);
const U32   FLAGS_CHARACTER            = (1U << 13);
const U32   FLAGS_VOLUME_DETECT        = (1U << 14);
const U32   FLAGS_INCLUDE_IN_SEARCH    = (1U << 15);

const U32   FLAGS_ALLOW_INVENTORY_DROP = (1U << 16);
const U32   FLAGS_OBJECT_TRANSFER      = (1U << 17);
const U32   FLAGS_OBJECT_GROUP_OWNED   = (1U << 18);
//const U32 FLAGS_UNUSED_000           = (1U << 19); // was FLAGS_OBJECT_YOU_OFFICER

const U32   FLAGS_CAMERA_DECOUPLED     = (1U << 20);
const U32   FLAGS_ANIM_SOURCE          = (1U << 21);
const U32   FLAGS_CAMERA_SOURCE        = (1U << 22);

//const U32 FLAGS_UNUSED_001           = (1U << 23); // was FLAGS_CAST_SHADOWS

//const U32 FLAGS_UNUSED_002           = (1U << 24);
//const U32 FLAGS_UNUSED_003           = (1U << 25);
//const U32 FLAGS_UNUSED_004           = (1U << 26);
//const U32 FLAGS_UNUSED_005           = (1U << 27);

const U32   FLAGS_OBJECT_OWNER_MODIFY  = (1U << 28);

const U32   FLAGS_TEMPORARY_ON_REZ     = (1U << 29);
//const U32 FLAGS_UNUSED_006           = (1U << 30); // was FLAGS_TEMPORARY
//const U32 FLAGS_UNUSED_007           = (1U << 31); // was FLAGS_ZLIB_COMPRESSED

const U32   FLAGS_LOCAL                = FLAGS_ANIM_SOURCE | FLAGS_CAMERA_SOURCE;

typedef enum e_havok_joint_type
{
	HJT_INVALID = 0,
	HJT_HINGE  	= 1,
	HJT_POINT 	= 2,
//	HJT_LPOINT 	= 3,
//	HJT_WHEEL 	= 4,
	HJT_EOF 	= 3
} EHavokJointType;

#endif
