/** 
 * @file llviewerim_peningtats.h
 * @brief LLViewerStats class header file
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

#ifndef LL_LLVIEWERSTATS_H
#define LL_LLVIEWERSTATS_H

#include "lltextureinfo.h"
#include "lltracerecording.h"
#include "lltrace.h"


enum ESimStatID
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

namespace LLStatViewer
{

struct SimMeasurementSampler : public LLInstanceTracker<SimMeasurementSampler, ESimStatID>
{
	SimMeasurementSampler(ESimStatID id)
	:	LLInstanceTracker<SimMeasurementSampler, ESimStatID>(id)
	{}
	virtual ~SimMeasurementSampler() {}

	virtual void sample(F64 value) = 0;
};

template<typename T = F64>
struct SimMeasurement : public LLTrace::SampleStatHandle<T>, public SimMeasurementSampler
{
	typedef SimMeasurement<T> self_t;

	SimMeasurement(const char* name, const char* description, ESimStatID stat_id)
	:	LLTrace::SampleStatHandle<T>(name, description),
		SimMeasurementSampler(stat_id)	
	{}

	using SimMeasurementSampler::getInstance;

	//friend void sample(self_t& measurement, T value)
	//{
	//	LLTrace::sample(static_cast<LLTrace::SampleStatHandle<T>& >(measurement), value);
	//}

	/*virtual*/ void sample(F64 value)
	{
		LLTrace::sample(static_cast<LLTrace::SampleStatHandle<T>& >(*this), value);
		//LLStatViewer::sample(*this, value);
	}

};

extern LLTrace::CountStatHandle<>			FPS,
											PACKETS_IN,
											PACKETS_LOST,
											PACKETS_OUT,
											TEXTURE_PACKETS,
											CHAT_COUNT,
											IM_COUNT,
											OBJECT_CREATE,
											OBJECT_REZ,
											LOGIN_TIMEOUTS,
											LSL_SAVES,
											ANIMATION_UPLOADS,
											FLY,
											TELEPORT,
											DELETE_OBJECT,
											SNAPSHOT,
											UPLOAD_SOUND,
											UPLOAD_TEXTURE,
											EDIT_TEXTURE,
											KILLED,
											FRAMETIME_DOUBLED,
											TEX_BAKES,
											TEX_REBAKES,
											NUM_NEW_OBJECTS;

extern LLTrace::CountStatHandle<LLUnit<F64, LLUnits::Kilotriangles> > TRIANGLES_DRAWN;

extern LLTrace::CountStatHandle<F64Kilobytes >	ACTIVE_MESSAGE_DATA_RECEIVED,
																	LAYERS_NETWORK_DATA_RECEIVED,
																	OBJECT_NETWORK_DATA_RECEIVED,
																	ASSET_UDP_DATA_RECEIVED,
																	TEXTURE_NETWORK_DATA_RECEIVED,
																	MESSAGE_SYSTEM_DATA_IN,
																	MESSAGE_SYSTEM_DATA_OUT;

extern LLTrace::CountStatHandle<F64Seconds >		SIM_20_FPS_TIME,
																	SIM_PHYSICS_20_FPS_TIME,
																	LOSS_5_PERCENT_TIME;

extern SimMeasurement<>						SIM_TIME_DILATION,
											SIM_FPS,
											SIM_PHYSICS_FPS,
											SIM_AGENT_UPS,
											SIM_SCRIPT_EPS,
											SIM_SKIPPED_SILHOUETTE,
											SIM_MAIN_AGENTS,
											SIM_CHILD_AGENTS,
											SIM_OBJECTS,
											SIM_ACTIVE_OBJECTS,
											SIM_ACTIVE_SCRIPTS,
											SIM_IN_PACKETS_PER_SEC,
											SIM_OUT_PACKETS_PER_SEC,
											SIM_PENDING_DOWNLOADS,
											SIM_PENDING_UPLOADS,
											SIM_PENDING_LOCAL_UPLOADS,
											SIM_PHYSICS_PINNED_TASKS,
											SIM_PHYSICS_LOD_TASKS;

extern SimMeasurement<LLUnit<F64, LLUnits::Percent> >	SIM_PERCENTAGE_SCRIPTS_RUN,
														SIM_SKIPPED_CHARACTERS_PERCENTAGE;

extern LLTrace::SampleStatHandle<>		FPS_SAMPLE,
										NUM_IMAGES,
										NUM_RAW_IMAGES,
										NUM_OBJECTS,
										NUM_ACTIVE_OBJECTS,
										ENABLE_VBO,
										LIGHTING_DETAIL,
										VISIBLE_AVATARS,
										SHADER_OBJECTS,
										DRAW_DISTANCE,
										PENDING_VFS_OPERATIONS,
										WINDOW_WIDTH,
										WINDOW_HEIGHT;

extern LLTrace::SampleStatHandle<LLUnit<F32, LLUnits::Percent> > PACKETS_LOST_PERCENT;

extern LLTrace::SampleStatHandle<F64Megabytes >	GL_TEX_MEM,
																	GL_BOUND_MEM,
																	RAW_MEM,
																	FORMATTED_MEM;
extern LLTrace::SampleStatHandle<F64Kilobytes >	DELTA_BANDWIDTH,
																	MAX_BANDWIDTH;
extern SimMeasurement<F64Milliseconds >	SIM_FRAME_TIME,
															SIM_NET_TIME,
															SIM_OTHER_TIME,
															SIM_PHYSICS_TIME,
															SIM_PHYSICS_STEP_TIME,
															SIM_PHYSICS_SHAPE_UPDATE_TIME,
															SIM_PHYSICS_OTHER_TIME,
															SIM_AI_TIME,
															SIM_AGENTS_TIME,
															SIM_IMAGES_TIME,
															SIM_SCRIPTS_TIME,
															SIM_SPARE_TIME,
															SIM_SLEEP_TIME,
															SIM_PUMP_IO_TIME;

extern SimMeasurement<F64Kilobytes >	SIM_UNACKED_BYTES;
extern SimMeasurement<F64Megabytes >	SIM_PHYSICS_MEM;


extern LLTrace::SampleStatHandle<F64Milliseconds >	FRAMETIME_JITTER,
																		FRAMETIME_SLEW,
																		SIM_PING;

extern LLTrace::EventStatHandle<LLUnit<F64, LLUnits::Meters> > AGENT_POSITION_SNAP;

extern LLTrace::EventStatHandle<>	LOADING_WEARABLES_LONG_DELAY;

extern LLTrace::EventStatHandle<F64Milliseconds >	REGION_CROSSING_TIME,
														FRAME_STACKTIME,
														UPDATE_STACKTIME,
														NETWORK_STACKTIME,
														IMAGE_STACKTIME,
														REBUILD_STACKTIME,
														RENDER_STACKTIME;

extern LLTrace::EventStatHandle<F64Seconds >	AVATAR_EDIT_TIME,
																TOOLBOX_TIME,
																MOUSELOOK_TIME,
																FPS_10_TIME,
																FPS_8_TIME,
																FPS_2_TIME;

extern LLTrace::EventStatHandle<LLUnit<F32, LLUnits::Percent> > OBJECT_CACHE_HIT_RATE;

}

class LLViewerStats : public LLSingleton<LLViewerStats>
{
	LLSINGLETON(LLViewerStats);
	~LLViewerStats();

public:
	void resetStats();

	void updateFrameStats(const F64Seconds time_diff);
	
	void addToMessage(LLSD &body);

	struct  StatsAccumulator
	{
		S32 mCount;
		F32 mSum;
		F32 mSumOfSquares;
		F32 mMinValue;
		F32 mMaxValue;
		U32 mCountOfNextUpdatesToIgnore;

		inline StatsAccumulator()
		{
			reset();
		}

		inline void push( F32 val )
		{
			if ( mCountOfNextUpdatesToIgnore > 0 )
			{
				mCountOfNextUpdatesToIgnore--;
				return;
			}
			
			mCount++;
			mSum += val;
			mSumOfSquares += val * val;
			if (mCount == 1 || val > mMaxValue)
			{
				mMaxValue = val;
			}
			if (mCount == 1 || val < mMinValue)
			{
				mMinValue = val;
			}
		}
		
		inline F32 getMean() const
		{
			return (mCount == 0) ? 0.f : ((F32)mSum)/mCount;
		}

		inline F32 getMinValue() const
		{
			return mMinValue;
		}

		inline F32 getMaxValue() const
		{
			return mMaxValue;
		}

		inline F32 getStdDev() const
		{
			const F32 mean = getMean();
			return (mCount < 2) ? 0.f : sqrt(llmax(0.f,mSumOfSquares/mCount - (mean * mean)));
		}
		
		inline U32 getCount() const
		{
			return mCount;
		}

		inline void reset()
		{
			mCount = 0;
			mSum = mSumOfSquares = 0.f;
			mMinValue = 0.0f;
			mMaxValue = 0.0f;
			mCountOfNextUpdatesToIgnore = 0;
		}
		
		inline LLSD asLLSD() const
		{
			LLSD data;
			data["mean"] = getMean();
			data["std_dev"] = getStdDev();
			data["count"] = (S32)mCount;
			data["min"] = getMinValue();
			data["max"] = getMaxValue();
			return data;
		}
	};

	// Phase tracking (originally put in for avatar rezzing), tracking
	// progress of active/completed phases for activities like outfit changing.
	typedef std::map<std::string,LLTimer>	phase_map_t;
	typedef std::map<std::string,StatsAccumulator>	phase_stats_t;
	class PhaseMap
	{
	private:
		phase_map_t mPhaseMap;
		static phase_stats_t sStats;
	public:
		PhaseMap();
		LLTimer& 		getPhaseTimer(const std::string& phase_name);
		bool 			getPhaseValues(const std::string& phase_name, F32& elapsed, bool& completed);
		void			startPhase(const std::string& phase_name);
		void			stopPhase(const std::string& phase_name);
		void			clearPhases();
		LLSD			asLLSD();
		static StatsAccumulator& getPhaseStats(const std::string& phase_name);
		static void recordPhaseStat(const std::string& phase_name, F32 value);
		phase_map_t::iterator begin() { return mPhaseMap.begin(); }
		phase_map_t::iterator end() { return mPhaseMap.end(); }
	};

	LLTrace::Recording& getRecording() { return mRecording; }
	const LLTrace::Recording& getRecording() const { return mRecording; }

private:
	LLTrace::Recording				mRecording;

	F64Seconds mLastTimeDiff;  // used for time stat updates
};

static const F32 SEND_STATS_PERIOD = 300.0f;

// The following are from (older?) statistics code found in appviewer.
void update_statistics();
void send_stats();

extern LLFrameTimer gTextureTimer;
extern U32Bytes	gTotalTextureData;
extern U32Bytes  gTotalObjectData;
extern U32Bytes  gTotalTextureBytesPerBoostLevel[] ;
#endif // LL_LLVIEWERSTATS_H
