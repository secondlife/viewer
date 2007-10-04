/** 
 * @file object_flags.h
 * @brief Flags for object creation and transmission
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

const U32	FLAGS_JOINT_HINGE			= 0x00001000;
const U32	FLAGS_JOINT_P2P				= 0x00002000;
const U32	FLAGS_JOINT_LP2P			= 0x00004000;
// const U32	FLAGS_JOINT_WHEEL		= 0x00008000;
const U32	FLAGS_INCLUDE_IN_SEARCH		= 0x00008000;

const U32	FLAGS_ALLOW_INVENTORY_DROP	= 0x00010000;
const U32	FLAGS_OBJECT_TRANSFER		= 0x00020000;
const U32	FLAGS_OBJECT_GROUP_OWNED	= 0x00040000;
//const U32	FLAGS_OBJECT_YOU_OFFICER	= 0x00080000;

const U32 	FLAGS_CAMERA_DECOUPLED 		= 0x00100000;
const U32	FLAGS_ANIM_SOURCE			= 0x00200000;
const U32	FLAGS_CAMERA_SOURCE			= 0x00400000;

const U32	FLAGS_CAST_SHADOWS			= 0x00800000;

const U32	FLAGS_OBJECT_OWNER_MODIFY	= 0x10000000;

const U32	FLAGS_TEMPORARY_ON_REZ		= 0x20000000;
const U32	FLAGS_TEMPORARY				= 0x40000000;
const U32	FLAGS_ZLIB_COMPRESSED		= 0x80000000;

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

