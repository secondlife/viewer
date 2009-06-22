/** 
 * @file llfasttimer.h
 * @brief Declaration of a fast timer.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_FASTTIMER_H
#define LL_FASTTIMER_H

#include "llinstancetracker.h"

#define FAST_TIMER_ON 1

U64 get_cpu_clock_count();

class LLMutex;

#include <queue>
#include "llsd.h"


class LLFastTimer
{
public:
	// stores a "named" timer instance to be reused via multiple LLFastTimer stack instances
	class NamedTimer 
	:	public LLInstanceTracker<NamedTimer>
	{
	public:
		~NamedTimer();

		enum { HISTORY_NUM = 60 };

		const std::string& getName() { return mName; }
		NamedTimer* getParent() { return mParent; }
		void setParent(NamedTimer* parent);
		S32 getDepth();
		std::string getToolTip(S32 history_index = -1);

		typedef std::vector<NamedTimer*>::const_iterator child_const_iter;
		child_const_iter beginChildren();
		child_const_iter endChildren();
		std::vector<NamedTimer*>& getChildren();

		void setCollapsed(bool collapsed) { mCollapsed = collapsed; }
		bool getCollapsed() { return mCollapsed; }

		U64 getCountAverage() { return mCountAverage; }
		U64 getCallAverage() { return mCallAverage; }

		U64 getHistoricalCount(S32 history_index = 0);
		U64 getHistoricalCalls(S32 history_index = 0);

		static NamedTimer& getRootNamedTimer();

		struct FrameState
		{
			FrameState(NamedTimer* timerp);

			U64 		mSelfTimeCounter;
			U64			mLastStartTime;		// most recent time when this timer was started
			U32 		mCalls;
			FrameState*	mParent;		// info for caller timer
			FrameState*	mLastCaller;	// used to bootstrap tree construction
			NamedTimer*	mTimer;
			U16			mActiveCount;	// number of timers with this ID active on stack
			bool		mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
		};

		FrameState& getFrameStateFast() const
		{
			return (*sTimerInfos)[mFrameStateIndex];
		}

		S32 getFrameStateIndex() const { return mFrameStateIndex; }

		FrameState& getFrameState() const;


	private: 
		friend class LLFastTimer;
		friend class NamedTimerFactory;

		//
		// methods
		//
		NamedTimer(const std::string& name);
		// recursive call to gather total time from children
		static void accumulateTimings();

		// called once per frame by LLFastTimer
		static void processFrame();

		static void buildHierarchy();
		static void resetFrame();
		static void reset();

		typedef std::vector<FrameState> info_list_t;
		static info_list_t& getFrameStateList();
		static void createFrameStateList(); // must call before any call to getFrameStateList()
		
		//
		// members
		//
		S32			mFrameStateIndex;

		std::string	mName;

		U64 		mTotalTimeCounter;

		U64 		mCountAverage;
		U64			mCallAverage;

		U64*		mCountHistory;
		U64*		mCallHistory;

		// tree structure
		NamedTimer*					mParent;				// NamedTimer of caller(parent)
		std::vector<NamedTimer*>	mChildren;
		bool						mCollapsed;				// don't show children
		bool						mNeedsSorting;			// sort children whenever child added

		static info_list_t* sTimerInfos;
	};

	// used to statically declare a new named timer
	class DeclareTimer
	{
	public:
		DeclareTimer(const std::string& name, bool open);
		DeclareTimer(const std::string& name);

		// convertable to NamedTimer::FrameState for convenient usage of LLFastTimer(declared_timer)
		operator NamedTimer::FrameState&() { return mNamedTimer.getFrameStateFast(); }
	private:
		NamedTimer& mNamedTimer;
	};

	static DeclareTimer FTM_ARRANGE;
	static DeclareTimer FTM_ATTACHMENT_UPDATE;
	static DeclareTimer FTM_AUDIO_UPDATE;
	static DeclareTimer FTM_AUTO_SELECT;
	static DeclareTimer FTM_AVATAR_UPDATE;
	static DeclareTimer FTM_CLEANUP;
	static DeclareTimer FTM_CLIENT_COPY;
	static DeclareTimer FTM_CREATE_OBJECT;
	static DeclareTimer FTM_CULL;
	static DeclareTimer FTM_CULL_REBOUND;
	static DeclareTimer FTM_FILTER;
	static DeclareTimer FTM_FLEXIBLE_UPDATE;
	static DeclareTimer FTM_FRAME;
	static DeclareTimer FTM_FRUSTUM_CULL;
	static DeclareTimer FTM_GEN_FLEX;
	static DeclareTimer FTM_GEN_TRIANGLES;
	static DeclareTimer FTM_GEN_VOLUME;
	static DeclareTimer FTM_GEO_SKY;
	static DeclareTimer FTM_GEO_UPDATE;
	static DeclareTimer FTM_HUD_EFFECTS;
	static DeclareTimer FTM_HUD_UPDATE;
	static DeclareTimer FTM_IDLE;
	static DeclareTimer FTM_IDLE_CB;
	static DeclareTimer FTM_IDLE_NETWORK;
	static DeclareTimer FTM_IMAGE_CREATE;
	static DeclareTimer FTM_IMAGE_MARK_DIRTY;
	static DeclareTimer FTM_IMAGE_UPDATE;
	static DeclareTimer FTM_INVENTORY;
	static DeclareTimer FTM_JOINT_UPDATE;
	static DeclareTimer FTM_KEYHANDLER;
	static DeclareTimer FTM_LOAD_AVATAR;
	static DeclareTimer FTM_LOD_UPDATE;
	static DeclareTimer FTM_MESSAGES;
	static DeclareTimer FTM_MOUSEHANDLER;
	static DeclareTimer FTM_NETWORK;
	static DeclareTimer FTM_OBJECTLIST_UPDATE;
	static DeclareTimer FTM_OCCLUSION_READBACK;
	static DeclareTimer FTM_OCTREE_BALANCE;
	static DeclareTimer FTM_PICK;
	static DeclareTimer FTM_PIPELINE;
	static DeclareTimer FTM_POOLRENDER;
	static DeclareTimer FTM_POOLS;
	static DeclareTimer FTM_PROCESS_IMAGES;
	static DeclareTimer FTM_PROCESS_MESSAGES;
	static DeclareTimer FTM_PROCESS_OBJECTS;
	static DeclareTimer FTM_PUMP;
	static DeclareTimer FTM_REBUILD_GRASS_VB;
	static DeclareTimer FTM_REBUILD_PARTICLE_VB;
	static DeclareTimer FTM_REBUILD_TERRAIN_VB;
	static DeclareTimer FTM_REBUILD_VBO;
	static DeclareTimer FTM_REBUILD_VOLUME_VB;
	static DeclareTimer FTM_REFRESH;
	static DeclareTimer FTM_REGION_UPDATE;
	static DeclareTimer FTM_RENDER;
	static DeclareTimer FTM_RENDER_ALPHA;
	static DeclareTimer FTM_RENDER_BLOOM;
	static DeclareTimer FTM_RENDER_BLOOM_FBO;
	static DeclareTimer FTM_RENDER_BUMP;
	static DeclareTimer FTM_RENDER_CHARACTERS;
	static DeclareTimer FTM_RENDER_FAKE_VBO_UPDATE;
	static DeclareTimer FTM_RENDER_FONTS;
	static DeclareTimer FTM_RENDER_FULLBRIGHT;
	static DeclareTimer FTM_RENDER_GEOMETRY;
	static DeclareTimer FTM_RENDER_GLOW;
	static DeclareTimer FTM_RENDER_GRASS;
	static DeclareTimer FTM_RENDER_INVISIBLE;
	static DeclareTimer FTM_RENDER_OCCLUSION;
	static DeclareTimer FTM_RENDER_SHINY;
	static DeclareTimer FTM_RENDER_SIMPLE;
	static DeclareTimer FTM_RENDER_TERRAIN;
	static DeclareTimer FTM_RENDER_TREES;
	static DeclareTimer FTM_RENDER_UI;
	static DeclareTimer FTM_RENDER_WATER;
	static DeclareTimer FTM_RENDER_WL_SKY;
	static DeclareTimer FTM_RESET_DRAWORDER;
	static DeclareTimer FTM_SHADOW_ALPHA;
	static DeclareTimer FTM_SHADOW_AVATAR;
	static DeclareTimer FTM_SHADOW_RENDER;
	static DeclareTimer FTM_SHADOW_SIMPLE;
	static DeclareTimer FTM_SHADOW_TERRAIN;
	static DeclareTimer FTM_SHADOW_TREE;
	static DeclareTimer FTM_SIMULATE_PARTICLES;
	static DeclareTimer FTM_SLEEP;
	static DeclareTimer FTM_SORT;
	static DeclareTimer FTM_STATESORT;
	static DeclareTimer FTM_STATESORT_DRAWABLE;
	static DeclareTimer FTM_STATESORT_POSTSORT;
	static DeclareTimer FTM_SWAP;
	static DeclareTimer FTM_TEMP1;
	static DeclareTimer FTM_TEMP2;
	static DeclareTimer FTM_TEMP3;
	static DeclareTimer FTM_TEMP4;
	static DeclareTimer FTM_TEMP5;
	static DeclareTimer FTM_TEMP6;
	static DeclareTimer FTM_TEMP7;
	static DeclareTimer FTM_TEMP8;
	static DeclareTimer FTM_UPDATE_ANIMATION;
	static DeclareTimer FTM_UPDATE_AVATAR;
	static DeclareTimer FTM_UPDATE_CLOUDS;
	static DeclareTimer FTM_UPDATE_GRASS;
	static DeclareTimer FTM_UPDATE_MOVE;
	static DeclareTimer FTM_UPDATE_PARTICLES;
	static DeclareTimer FTM_UPDATE_PRIMITIVES;
	static DeclareTimer FTM_UPDATE_SKY;
	static DeclareTimer FTM_UPDATE_TERRAIN;
	static DeclareTimer FTM_UPDATE_TEXTURES;
	static DeclareTimer FTM_UPDATE_TREE;
	static DeclareTimer FTM_UPDATE_WATER;
	static DeclareTimer FTM_UPDATE_WLPARAM;
	static DeclareTimer FTM_VFILE_WAIT;
	static DeclareTimer FTM_WORLD_UPDATE;

public:
	enum RootTimerMarker { ROOT };
	
	static LLMutex* sLogLock;
	static std::queue<LLSD> sLogQueue;
	static BOOL sLog;
	static BOOL sMetricLog;

	LLFastTimer(RootTimerMarker);

	LLFastTimer(NamedTimer::FrameState& timer)
	:	mFrameState(&timer)
	{
		NamedTimer::FrameState* frame_state = mFrameState;
		frame_state->mLastStartTime = get_cpu_clock_count();
		mStartSelfTime = frame_state->mLastStartTime;

		frame_state->mActiveCount++;
		frame_state->mCalls++;
		// keep current parent as long as it is active when we are
		frame_state->mMoveUpTree |= (frame_state->mParent->mActiveCount == 0);
	
		mLastTimer = sCurTimer;
		sCurTimer = this;
	}

	~LLFastTimer()
	{
#if FAST_TIMER_ON
		NamedTimer::FrameState* frame_state = mFrameState;
		U64 cur_time = get_cpu_clock_count();
		frame_state->mSelfTimeCounter += cur_time - mStartSelfTime;

		frame_state->mActiveCount--;
		LLFastTimer* last_timer = mLastTimer;
		sCurTimer = last_timer;

		// store last caller to bootstrap tree creation
		frame_state->mLastCaller = last_timer->mFrameState;

		// we are only tracking self time, so subtract our total time delta from parents
		U64 total_time = cur_time - frame_state->mLastStartTime;
		last_timer->mStartSelfTime += total_time;
#endif
	}


	// call this once a frame to reset timers
	static void nextFrame();

	// call this to reset timer hierarchy, averages, etc.
	static void reset();

	static U64 countsPerSecond();
	static S32 getLastFrameIndex() { return sLastFrameIndex; }
	static S32 getCurFrameIndex() { return sCurFrameIndex; }

	static void writeLog(std::ostream& os);

public:
	static bool 		sPauseHistory;
	static bool 		sResetHistory;
	
private:
	typedef std::vector<LLFastTimer*> timer_stack_t;
	static LLFastTimer*		sCurTimer;
	static S32				sCurFrameIndex;
	static S32				sLastFrameIndex;

	static F64				sCPUClockFrequency;
	U64						mStartSelfTime;	// start time + time of all child timers
	NamedTimer::FrameState*	mFrameState;
	LLFastTimer*			mLastTimer;
};

#endif // LL_LLFASTTIMER_H
