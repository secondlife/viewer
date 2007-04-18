/** 
 * @file llviewerstats.h
 * @brief LLViewerStats class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERSTATS_H
#define LL_LLVIEWERSTATS_H

#include "llstat.h"

class LLViewerStats
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

	// Simulator stats
	LLStat mSimTimeDilation;

	LLStat mSimFPS;
	LLStat mSimPhysicsFPS;
	LLStat mSimAgentUPS;
	LLStat mSimLSLIPS;

	LLStat mSimFrameMsec;
	LLStat mSimNetMsec;
	LLStat mSimSimOtherMsec;
	LLStat mSimSimPhysicsMsec;
	LLStat mSimAgentMsec;
	LLStat mSimImagesMsec;
	LLStat mSimScriptMsec;

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

	/*
	LLStat mSimCPUUsageStat;
	LLStat mSimMemTotalStat;
	LLStat mSimMemRSSStat;
	*/


	LLStat mSimPingStat;
	LLStat mUserserverPingStat;

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
		ST_LIBXUL_WIDGET_USED = 53,
		ST_WINDOW_WIDTH = 54,
		ST_WINDOW_HEIGHT = 55,
		ST_TEX_BAKES = 56,
		ST_TEX_REBAKES = 57,
		ST_LOGITECH_LCD = 58,
		
		ST_COUNT = 59
	};


	LLViewerStats();
	~LLViewerStats();

	// all return latest value of given stat
	F64 getStat(EStatType type) const;
	F64 setStat(EStatType type, F64 value);		// set the stat to value
	F64 incStat(EStatType type, F64 value = 1.f);	// add value to the stat

	void updateFrameStats(const F64 time_diff);
	
	void addToMessage() const;

	static const char *statTypeToText(EStatType type);

private:
	F64	mStats[ST_COUNT];

	F64 mLastTimeDiff;  // used for time stat updates
};

extern LLViewerStats *gViewerStats;

#endif // LL_LLVIEWERSTATS_H
