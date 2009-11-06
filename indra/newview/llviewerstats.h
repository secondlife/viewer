/** 
 * @file llviewerstats.h
 * @brief LLViewerStats class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLVIEWERSTATS_H
#define LL_LLVIEWERSTATS_H

#include "llstat.h"
#include "lltextureinfo.h"

class LLViewerStats : public LLSingleton<LLViewerStats>
{
public:
	LLStat mKBitStat;
	LLStat mLayersKBitStat;
	LLStat mObjectKBitStat;
	LLStat mAssetKBitStat;
	LLStat mTextureKBitStat;
	LLStat mVFSPendingOperations;
	LLStat mObjectsDrawnStat;
	LLStat mObjectsCulledStat;
	LLStat mObjectsTestedStat;
	LLStat mObjectsComparedStat;
	LLStat mObjectsOccludedStat;
	LLStat mFPSStat;
	LLStat mPacketsInStat;
	LLStat mPacketsLostStat;
	LLStat mPacketsOutStat;
	LLStat mPacketsLostPercentStat;
	LLStat mTexturePacketsStat;
	LLStat mActualInKBitStat;	// From the packet ring (when faking a bad connection)
	LLStat mActualOutKBitStat;	// From the packet ring (when faking a bad connection)
	LLStat mTrianglesDrawnStat;

	// Simulator stats
	LLStat mSimTimeDilation;

	LLStat mSimFPS;
	LLStat mSimPhysicsFPS;
	LLStat mSimAgentUPS;
	LLStat mSimScriptEPS;

	LLStat mSimFrameMsec;
	LLStat mSimNetMsec;
	LLStat mSimSimOtherMsec;
	LLStat mSimSimPhysicsMsec;

	LLStat mSimSimPhysicsStepMsec;
	LLStat mSimSimPhysicsShapeUpdateMsec;
	LLStat mSimSimPhysicsOtherMsec;

	LLStat mSimAgentMsec;
	LLStat mSimImagesMsec;
	LLStat mSimScriptMsec;
	LLStat mSimSpareMsec;
	LLStat mSimSleepMsec;
	LLStat mSimPumpIOMsec;

	LLStat mSimMainAgents;
	LLStat mSimChildAgents;
	LLStat mSimObjects;
	LLStat mSimActiveObjects;
	LLStat mSimActiveScripts;

	LLStat mSimInPPS;
	LLStat mSimOutPPS;
	LLStat mSimPendingDownloads;
	LLStat mSimPendingUploads;
	LLStat mSimPendingLocalUploads;
	LLStat mSimTotalUnackedBytes;

	LLStat mPhysicsPinnedTasks;
	LLStat mPhysicsLODTasks;
	LLStat mPhysicsMemoryAllocated;

	LLStat mSimPingStat;

	LLStat mNumImagesStat;
	LLStat mNumRawImagesStat;
	LLStat mGLTexMemStat;
	LLStat mGLBoundMemStat;
	LLStat mRawMemStat;
	LLStat mFormattedMemStat;

	LLStat mNumObjectsStat;
	LLStat mNumActiveObjectsStat;
	LLStat mNumNewObjectsStat;
	LLStat mNumSizeCulledStat;
	LLStat mNumVisCulledStat;

	void resetStats();
public:
	// If you change this, please also add a corresponding text label
	// in statTypeToText in llviewerstats.cpp
	enum EStatType
	{
		ST_VERSION = 0,
		ST_AVATAR_EDIT_SECONDS = 1,
		ST_TOOLBOX_SECONDS = 2,
		ST_CHAT_COUNT = 3,
		ST_IM_COUNT = 4,
		ST_FULLSCREEN_BOOL = 5,
		ST_RELEASE_COUNT= 6,
		ST_CREATE_COUNT = 7,
		ST_REZ_COUNT = 8,
		ST_FPS_10_SECONDS = 9,
		ST_FPS_2_SECONDS = 10,
		ST_MOUSELOOK_SECONDS = 11,
		ST_FLY_COUNT = 12,
		ST_TELEPORT_COUNT = 13,
		ST_OBJECT_DELETE_COUNT = 14,
		ST_SNAPSHOT_COUNT = 15,
		ST_UPLOAD_SOUND_COUNT = 16,
		ST_UPLOAD_TEXTURE_COUNT = 17,
		ST_EDIT_TEXTURE_COUNT = 18,
		ST_KILLED_COUNT = 19,
		ST_FRAMETIME_JITTER = 20,
		ST_FRAMETIME_SLEW = 21,
		ST_INVENTORY_TOO_LONG = 22,
		ST_WEARABLES_TOO_LONG = 23,
		ST_LOGIN_SECONDS = 24,
		ST_LOGIN_TIMEOUT_COUNT = 25,
		ST_HAS_BAD_TIMER = 26,
		ST_DOWNLOAD_FAILED = 27,
		ST_LSL_SAVE_COUNT = 28,
		ST_UPLOAD_ANIM_COUNT = 29,
		ST_FPS_8_SECONDS = 30,
		ST_SIM_FPS_20_SECONDS = 31,
		ST_PHYS_FPS_20_SECONDS = 32,
		ST_LOSS_05_SECONDS = 33,
		ST_FPS_DROP_50_RATIO = 34,
		ST_ENABLE_VBO = 35,
		ST_DELTA_BANDWIDTH = 36,
		ST_MAX_BANDWIDTH = 37,
		ST_LIGHTING_DETAIL = 38,
		ST_VISIBLE_AVATARS = 39,
		ST_SHADER_OBJECTS = 40,
		ST_SHADER_ENVIRONMENT = 41,
		ST_DRAW_DIST = 42,
		ST_CHAT_BUBBLES = 43,
		ST_SHADER_AVATAR = 44,
		ST_FRAME_SECS = 45,
		ST_UPDATE_SECS = 46,
		ST_NETWORK_SECS = 47,
		ST_IMAGE_SECS = 48,
		ST_REBUILD_SECS = 49,
		ST_RENDER_SECS = 50,
		ST_CROSSING_AVG = 51,
		ST_CROSSING_MAX = 52,
		ST_LIBXUL_WIDGET_USED = 53, // Unused
		ST_WINDOW_WIDTH = 54,
		ST_WINDOW_HEIGHT = 55,
		ST_TEX_BAKES = 56,
		ST_TEX_REBAKES = 57,
		
		ST_COUNT = 58
	};


	LLViewerStats();
	~LLViewerStats();

	// all return latest value of given stat
	F64 getStat(EStatType type) const;
	F64 setStat(EStatType type, F64 value);		// set the stat to value
	F64 incStat(EStatType type, F64 value = 1.f);	// add value to the stat

	void updateFrameStats(const F64 time_diff);
	
	void addToMessage(LLSD &body) const;

private:
	F64	mStats[ST_COUNT];

	F64 mLastTimeDiff;  // used for time stat updates
};

static const F32 SEND_STATS_PERIOD = 300.0f;

// The following are from (older?) statistics code found in appviewer.
void init_statistics();
void reset_statistics();
void output_statistics(void*);
void update_statistics(U32 frame_count);
void send_stats();

extern std::map<S32,LLFrameTimer> gDebugTimers;
extern std::map<S32,std::string> gDebugTimerLabel;

#endif // LL_LLVIEWERSTATS_H
