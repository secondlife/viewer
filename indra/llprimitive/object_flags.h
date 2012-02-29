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
const U32	FLAGS_USE_PHYSICS			= 0x00000001;
const U32	FLAGS_CREATE_SELECTED		= 0x00000002;
const U32	FLAGS_OBJECT_MODIFY			= 0x00000004;
const U32	FLAGS_OBJECT_COPY			= 0x00000008;
const U32	FLAGS_OBJECT_ANY_OWNER		= 0x00000010;
const U32	FLAGS_OBJECT_YOU_OWNER		= 0x00000020;
const U32	FLAGS_SCRIPTED				= 0x00000040;
const U32	FLAGS_HANDLE_TOUCH			= 0x00000080;
const U32	FLAGS_OBJECT_MOVE			= 0x00000100;
const U32	FLAGS_TAKES_MONEY			= 0x00000200;
const U32	FLAGS_PHANTOM				= 0x00000400;
const U32	FLAGS_INVENTORY_EMPTY		= 0x00000800;

const U32	FLAGS_INCLUDE_IN_SEARCH		= 0x00008000;

const U32	FLAGS_ALLOW_INVENTORY_DROP	= 0x00010000;
const U32	FLAGS_OBJECT_TRANSFER		= 0x00020000;
const U32	FLAGS_OBJECT_GROUP_OWNED	= 0x00040000;

const U32 	FLAGS_CAMERA_DECOUPLED 		= 0x00100000;
const U32	FLAGS_ANIM_SOURCE			= 0x00200000;
const U32	FLAGS_CAMERA_SOURCE			= 0x00400000;

const U32	FLAGS_OBJECT_OWNER_MODIFY	= 0x10000000;

const U32	FLAGS_TEMPORARY_ON_REZ		= 0x20000000;

const U32	FLAGS_LOCAL					= FLAGS_ANIM_SOURCE | FLAGS_CAMERA_SOURCE;

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

