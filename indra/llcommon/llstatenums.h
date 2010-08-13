/** 
 * @file llstatenums.h
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

#ifndef LL_LLSTATENUMS_H
#define LL_LLSTATENUMS_H

enum
{
	LL_SIM_STAT_TIME_DILATION,				// 0
	LL_SIM_STAT_FPS,
	LL_SIM_STAT_PHYSFPS,
	LL_SIM_STAT_AGENTUPS,
	LL_SIM_STAT_FRAMEMS,
	LL_SIM_STAT_NETMS,						// 5
	LL_SIM_STAT_SIMOTHERMS,
	LL_SIM_STAT_SIMPHYSICSMS,
	LL_SIM_STAT_AGENTMS,
	LL_SIM_STAT_IMAGESMS,
	LL_SIM_STAT_SCRIPTMS,					// 10
	LL_SIM_STAT_NUMTASKS,
	LL_SIM_STAT_NUMTASKSACTIVE,
	LL_SIM_STAT_NUMAGENTMAIN,
	LL_SIM_STAT_NUMAGENTCHILD,
	LL_SIM_STAT_NUMSCRIPTSACTIVE,			// 15
	LL_SIM_STAT_LSLIPS,
	LL_SIM_STAT_INPPS,
	LL_SIM_STAT_OUTPPS,
	LL_SIM_STAT_PENDING_DOWNLOADS,
	LL_SIM_STAT_PENDING_UPLOADS,			// 20
	LL_SIM_STAT_VIRTUAL_SIZE_KB,
	LL_SIM_STAT_RESIDENT_SIZE_KB,
	LL_SIM_STAT_PENDING_LOCAL_UPLOADS,
	LL_SIM_STAT_TOTAL_UNACKED_BYTES,
	LL_SIM_STAT_PHYSICS_PINNED_TASKS,		// 25
	LL_SIM_STAT_PHYSICS_LOD_TASKS,
	LL_SIM_STAT_SIMPHYSICSSTEPMS,
	LL_SIM_STAT_SIMPHYSICSSHAPEMS,
	LL_SIM_STAT_SIMPHYSICSOTHERMS,
	LL_SIM_STAT_SIMPHYSICSMEMORY,			// 30
	LL_SIM_STAT_SCRIPT_EPS,
	LL_SIM_STAT_SIMSPARETIME,
	LL_SIM_STAT_SIMSLEEPTIME,
	LL_SIM_STAT_IOPUMPTIME,
};

#endif
