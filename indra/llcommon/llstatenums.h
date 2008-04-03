/** 
 * @file llstatenums.h
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

#ifndef LL_LLSTATENUMS_H
#define LL_LLSTATENUMS_H

enum
{
	LL_SIM_STAT_TIME_DILATION,
	LL_SIM_STAT_FPS,
	LL_SIM_STAT_PHYSFPS,
	LL_SIM_STAT_AGENTUPS,
	LL_SIM_STAT_FRAMEMS,
	LL_SIM_STAT_NETMS,
	LL_SIM_STAT_SIMOTHERMS,
	LL_SIM_STAT_SIMPHYSICSMS,
	LL_SIM_STAT_AGENTMS,
	LL_SIM_STAT_IMAGESMS,
	LL_SIM_STAT_SCRIPTMS,
	LL_SIM_STAT_NUMTASKS,
	LL_SIM_STAT_NUMTASKSACTIVE,
	LL_SIM_STAT_NUMAGENTMAIN,
	LL_SIM_STAT_NUMAGENTCHILD,
	LL_SIM_STAT_NUMSCRIPTSACTIVE,
	LL_SIM_STAT_LSLIPS,
	LL_SIM_STAT_INPPS,
	LL_SIM_STAT_OUTPPS,
	LL_SIM_STAT_PENDING_DOWNLOADS,
	LL_SIM_STAT_PENDING_UPLOADS,
	LL_SIM_STAT_VIRTUAL_SIZE_KB,
	LL_SIM_STAT_RESIDENT_SIZE_KB,
	LL_SIM_STAT_PENDING_LOCAL_UPLOADS,
	LL_SIM_STAT_TOTAL_UNACKED_BYTES,
	LL_SIM_STAT_PHYSICS_PINNED_TASKS,
	LL_SIM_STAT_PHYSICS_LOD_TASKS,
	LL_SIM_STAT_SIMPHYSICSSTEPMS,
	LL_SIM_STAT_SIMPHYSICSSHAPEMS,
	LL_SIM_STAT_SIMPHYSICSOTHERMS,
	LL_SIM_STAT_SIMPHYSICSMEMORY
};

#endif
