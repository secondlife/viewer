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
	LL_SIM_STAT_TIME_DILATION         =  0,
	LL_SIM_STAT_FPS                   =  1,
	LL_SIM_STAT_PHYSFPS               =  2,
	LL_SIM_STAT_AGENTUPS              =  3,
	LL_SIM_STAT_FRAMEMS               =  4,
	LL_SIM_STAT_NETMS                 =  5,
	LL_SIM_STAT_SIMOTHERMS            =  6,
	LL_SIM_STAT_SIMPHYSICSMS          =  7,
	LL_SIM_STAT_AGENTMS               =  8,
	LL_SIM_STAT_IMAGESMS              =  9,
	LL_SIM_STAT_SCRIPTMS              = 10,
	LL_SIM_STAT_NUMTASKS              = 11,
	LL_SIM_STAT_NUMTASKSACTIVE        = 12,
	LL_SIM_STAT_NUMAGENTMAIN          = 13,
	LL_SIM_STAT_NUMAGENTCHILD         = 14,
	LL_SIM_STAT_NUMSCRIPTSACTIVE      = 15,
	LL_SIM_STAT_LSLIPS                = 16,
	LL_SIM_STAT_INPPS                 = 17,
	LL_SIM_STAT_OUTPPS                = 18,
	LL_SIM_STAT_PENDING_DOWNLOADS     = 19,
	LL_SIM_STAT_PENDING_UPLOADS       = 20,
	LL_SIM_STAT_VIRTUAL_SIZE_KB       = 21,
	LL_SIM_STAT_RESIDENT_SIZE_KB      = 22,
	LL_SIM_STAT_PENDING_LOCAL_UPLOADS = 23,
	LL_SIM_STAT_TOTAL_UNACKED_BYTES   = 24,
	LL_SIM_STAT_PHYSICS_PINNED_TASKS  = 25,
	LL_SIM_STAT_PHYSICS_LOD_TASKS     = 26,
	LL_SIM_STAT_SIMPHYSICSSTEPMS      = 27,
	LL_SIM_STAT_SIMPHYSICSSHAPEMS     = 28,
	LL_SIM_STAT_SIMPHYSICSOTHERMS     = 29,
	LL_SIM_STAT_SIMPHYSICSMEMORY      = 30,
	LL_SIM_STAT_SCRIPT_EPS            = 31,
	LL_SIM_STAT_SIMSPARETIME          = 32,
	LL_SIM_STAT_SIMSLEEPTIME          = 33,
	LL_SIM_STAT_IOPUMPTIME            = 34,
	LL_SIM_STAT_PCTSCRIPTSRUN         = 35,
	LL_SIM_STAT_REGION_IDLE           = 36, // dataserver only
	LL_SIM_STAT_REGION_IDLE_POSSIBLE  = 37, // dataserver only
	LL_SIM_STAT_SIMAISTEPTIMEMS       = 38,
	LL_SIM_STAT_SKIPPEDAISILSTEPS_PS  = 39,
	LL_SIM_STAT_PCTSTEPPEDCHARACTERS  = 40

};

#endif
