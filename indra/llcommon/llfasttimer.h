/** 
 * @file llfasttimer.h
 * @brief Declaration of a fast timer.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

#ifndef LL_LLFASTTIMER_H
#define LL_LLFASTTIMER_H

#define FAST_TIMER_ON 1

U64 get_cpu_clock_count();

class LLFastTimer
{
public:
	enum EFastTimerType
	{
		// high level
		FTM_FRAME,
		FTM_UPDATE,
		FTM_RENDER,
		FTM_SWAP,
		FTM_CLIENT_COPY,
		FTM_IDLE,
		FTM_SLEEP,

		// common messaging components
		FTM_PUMP,
		FTM_CURL,
		
		// common simulation components
		FTM_UPDATE_ANIMATION,
		FTM_UPDATE_TERRAIN,
		FTM_UPDATE_PRIMITIVES,
		FTM_UPDATE_PARTICLES,
		FTM_SIMULATE_PARTICLES,
		FTM_UPDATE_SKY,
		FTM_UPDATE_TEXTURES,
		FTM_UPDATE_WATER,
		FTM_UPDATE_CLOUDS,
		FTM_UPDATE_GRASS,
		FTM_UPDATE_TREE,
		FTM_UPDATE_AVATAR,
		
		// common render components
		FTM_RENDER_GEOMETRY,
		 FTM_RENDER_TERRAIN,
		 FTM_RENDER_SIMPLE,
		 FTM_RENDER_FULLBRIGHT,
		 FTM_RENDER_GLOW,
		 FTM_RENDER_GRASS,
		 FTM_RENDER_INVISIBLE,
		 FTM_RENDER_SHINY,
		 FTM_RENDER_BUMP,
		 FTM_RENDER_TREES,
		 FTM_RENDER_CHARACTERS,
		 FTM_RENDER_OCCLUSION,
		 FTM_RENDER_ALPHA,
         FTM_RENDER_CLOUDS,
		 FTM_RENDER_HUD,
		 FTM_RENDER_PARTICLES,
		 FTM_RENDER_WATER,
		FTM_RENDER_TIMER,
		FTM_RENDER_UI,
		FTM_RENDER_FONTS,
		
		// newview specific
		FTM_MESSAGES,
		FTM_REBUILD,
		FTM_STATESORT,
		FTM_STATESORT_DRAWABLE,
		FTM_STATESORT_POSTSORT,
		FTM_REBUILD_VBO,
		FTM_REBUILD_VOLUME_VB,
		FTM_REBUILD_BRIDGE_VB,
		FTM_REBUILD_HUD_VB,
		FTM_REBUILD_TERRAIN_VB,
		FTM_REBUILD_WATER_VB,
		FTM_REBUILD_TREE_VB,
		FTM_REBUILD_PARTICLE_VB,
		FTM_REBUILD_CLOUD_VB,
		FTM_REBUILD_GRASS_VB,
		FTM_REBUILD_NONE_VB,
		FTM_REBUILD_OCCLUSION_VB,
		FTM_POOLS,
		FTM_POOLRENDER,
		FTM_IDLE_CB,
		FTM_WORLD_UPDATE,
		FTM_UPDATE_MOVE,
		FTM_OCTREE_BALANCE,
		FTM_UPDATE_LIGHTS,
		FTM_CULL,
		FTM_CULL_REBOUND,
		FTM_FRUSTUM_CULL,
		FTM_GEO_UPDATE,
		FTM_GEO_RESERVE,
		FTM_GEO_LIGHT,
		FTM_GEO_SHADOW,
		FTM_GEN_VOLUME,
		FTM_GEN_TRIANGLES,
		FTM_GEN_FLEX,
		FTM_AUDIO_UPDATE,
		FTM_RESET_DRAWORDER,
		FTM_OBJECTLIST_UPDATE,
		FTM_AVATAR_UPDATE,
		FTM_JOINT_UPDATE,
		FTM_ATTACHMENT_UPDATE,
		FTM_LOD_UPDATE,
		FTM_REGION_UPDATE,
		FTM_CLEANUP,
		FTM_NETWORK,
		FTM_IDLE_NETWORK,
		FTM_CREATE_OBJECT,
		FTM_LOAD_AVATAR,
		FTM_PROCESS_MESSAGES,
		FTM_PROCESS_OBJECTS,
		FTM_PROCESS_IMAGES,
		FTM_IMAGE_UPDATE,
		FTM_IMAGE_CREATE,
		FTM_IMAGE_DECODE,
		FTM_IMAGE_MARK_DIRTY,
		FTM_PIPELINE,
		FTM_VFILE_WAIT,
		FTM_FLEXIBLE_UPDATE,
		FTM_OCCLUSION,
		FTM_OCCLUSION_READBACK,
		FTM_HUD_EFFECTS,
		FTM_HUD_UPDATE,
		FTM_INVENTORY,
		FTM_AUTO_SELECT,
		FTM_ARRANGE,
		FTM_FILTER,
		FTM_REFRESH,
		FTM_SORT,
		
		// Temp
		FTM_TEMP1,
		FTM_TEMP2,
		FTM_TEMP3,
		FTM_TEMP4,
		FTM_TEMP5,
		FTM_TEMP6,
		FTM_TEMP7,
		FTM_TEMP8,
		
		FTM_OTHER, // Special, used by display code
		
		FTM_NUM_TYPES
	};
	enum { FTM_HISTORY_NUM = 60 };
	enum { FTM_MAX_DEPTH = 64 };
	
public:
	LLFastTimer(EFastTimerType type)
	{
#if FAST_TIMER_ON
		mType = type;

		// These don't get counted, because they use CPU clockticks
		//gTimerBins[gCurTimerBin]++;
		//LLTimer::sNumTimerCalls++;

		U64 cpu_clocks = get_cpu_clock_count();

		sStart[sCurDepth] = cpu_clocks;
		sCurDepth++;
#endif
	};
	~LLFastTimer()
	{
#if FAST_TIMER_ON
		U64 end,delta;
		int i;

		// These don't get counted, because they use CPU clockticks
		//gTimerBins[gCurTimerBin]++;
		//LLTimer::sNumTimerCalls++;
		end = get_cpu_clock_count();

		sCurDepth--;
		delta = end - sStart[sCurDepth];
		sCounter[mType] += delta;
		sCalls[mType]++;
		// Subtract delta from parents
		for (i=0; i<sCurDepth; i++)
			sStart[i] += delta;
#endif
	}

	static void reset();
	static U64 countsPerSecond();

public:
	static int sCurDepth;
	static U64 sStart[FTM_MAX_DEPTH];
	static U64 sCounter[FTM_NUM_TYPES];
	static U64 sCalls[FTM_NUM_TYPES];
	static U64 sCountAverage[FTM_NUM_TYPES];
	static U64 sCallAverage[FTM_NUM_TYPES];
	static U64 sCountHistory[FTM_HISTORY_NUM][FTM_NUM_TYPES];
	static U64 sCallHistory[FTM_HISTORY_NUM][FTM_NUM_TYPES];
	static S32 sCurFrameIndex;
	static S32 sLastFrameIndex;
	static int sPauseHistory;
	static int sResetHistory;
	static F64 sCPUClockFrequency;
	
private:
	EFastTimerType mType;
};


#endif // LL_LLFASTTIMER_H
