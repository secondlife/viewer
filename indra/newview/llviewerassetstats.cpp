/** 
 * @file llviewerassetstats.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llviewerassetstats.h"
#include "llregionhandle.h"

#include "stdtypes.h"
#include "llvoavatar.h"

/*
 * Classes and utility functions for per-thread and per-region
 * asset and experiential metrics to be aggregated grid-wide.
 *
 * The basic metrics grouping is LLViewerAssetStats::PerRegionStats.
 * This provides various counters and simple statistics for asset
 * fetches binned into a few categories.  One of these is maintained
 * for each region encountered and the 'current' region is available
 * as a simple reference.  Each thread (presently two) interested
 * in participating in these stats gets an instance of the
 * LLViewerAssetStats class so that threads are completely
 * independent.
 *
 * The idea of a current region is used for simplicity and speed
 * of categorization.  Each metrics event could have taken a
 * region uuid argument resulting in a suitable lookup.  Arguments
 * against this design include:
 *
 *  -  Region uuid not trivially available to caller.
 *  -  Cost (cpu, disruption in real work flow) too high.
 *  -  Additional precision not really meaningful.
 *
 * By itself, the LLViewerAssetStats class is thread- and
 * viewer-agnostic and can be used anywhere without assumptions
 * of global pointers and other context.  For the viewer,
 * a set of free functions are provided in the namespace
 * LLViewerAssetStatsFF which *do* implement viewer-native
 * policies about per-thread globals and will do correct
 * defensive tests of same.
 *
 * References
 *
 * Project:
 *   <TBD>
 *
 * Test Plan:
 *   <TBD>
 *
 * Jiras:
 *   <TBD>
 *
 * Unit Tests:
 *   indra/newview/tests/llviewerassetstats_test.cpp
 *
 */


// ------------------------------------------------------
// Global data definitions
// ------------------------------------------------------
LLViewerAssetStats * gViewerAssetStatsMain(0);
LLViewerAssetStats * gViewerAssetStatsThread1(0);


// ------------------------------------------------------
// Local declarations
// ------------------------------------------------------
namespace
{

static LLViewerAssetStats::EViewerAssetCategories
asset_type_to_category(const LLViewerAssetType::EType at, bool with_http, bool is_temp);

}

// ------------------------------------------------------
// LLViewerAssetStats::PerRegionStats struct definition
// ------------------------------------------------------
void
LLViewerAssetStats::PerRegionStats::reset()
{
	for (int i(0); i < LL_ARRAY_SIZE(mRequests); ++i)
	{
		mRequests[i].mEnqueued.reset();
		mRequests[i].mDequeued.reset();
		mRequests[i].mResponse.reset();
	}
	mFPS.reset();
	
	mTotalTime = 0;
	mStartTimestamp = LLViewerAssetStatsFF::get_timestamp();
}


void
LLViewerAssetStats::PerRegionStats::merge(const LLViewerAssetStats::PerRegionStats & src)
{
	// mRegionHandle, mTotalTime, mStartTimestamp are left alone.
	
	// mFPS
	if (src.mFPS.getCount() && mFPS.getCount())
	{
		mFPS.merge(src.mFPS);
	}

	// Avatar stats - data all comes from main thread, so leave alone.

	// Requests
	for (int i = 0; i < LL_ARRAY_SIZE(mRequests); ++i)
	{
		mRequests[i].mEnqueued.merge(src.mRequests[i].mEnqueued);
		mRequests[i].mDequeued.merge(src.mRequests[i].mDequeued);
		mRequests[i].mResponse.merge(src.mRequests[i].mResponse);
	}

}


void
LLViewerAssetStats::PerRegionStats::accumulateTime(duration_t now)
{
	mTotalTime += (now - mStartTimestamp);
	mStartTimestamp = now;
}


// ------------------------------------------------------
// LLViewerAssetStats class definition
// ------------------------------------------------------
LLViewerAssetStats::LLViewerAssetStats()
	: mRegionHandle(U64(0))
{
	reset();
}


LLViewerAssetStats::LLViewerAssetStats(const LLViewerAssetStats & src)
	: mRegionHandle(src.mRegionHandle),
	  mResetTimestamp(src.mResetTimestamp),
	  mPhaseStats(src.mPhaseStats),
	  mAvatarRezStates(src.mAvatarRezStates)
{
	const PerRegionContainer::const_iterator it_end(src.mRegionStats.end());
	for (PerRegionContainer::const_iterator it(src.mRegionStats.begin()); it_end != it; ++it)
	{
		mRegionStats[it->first] = new PerRegionStats(*it->second);
	}
	mCurRegionStats = mRegionStats[mRegionHandle];
}


void
LLViewerAssetStats::reset()
{
	// Empty the map of all region stats
	mRegionStats.clear();

	// If we have a current stats, reset it, otherwise, as at construction,
	// create a new one as we must always have a current stats block.
	if (mCurRegionStats)
	{
		mCurRegionStats->reset();
	}
	else
	{
		mCurRegionStats = new PerRegionStats(mRegionHandle);
	}

	// And add reference to map
	mRegionStats[mRegionHandle] = mCurRegionStats;

	// Start timestamp consistent with per-region collector
	mResetTimestamp = mCurRegionStats->mStartTimestamp;
}


void
LLViewerAssetStats::setRegion(region_handle_t region_handle)
{
	if (region_handle == mRegionHandle)
	{
		// Already active, ignore.
		return;
	}

	// Get duration for current set
	const duration_t now = LLViewerAssetStatsFF::get_timestamp();
	mCurRegionStats->accumulateTime(now);

	// Prepare new set
	PerRegionContainer::iterator new_stats = mRegionStats.find(region_handle);
	if (mRegionStats.end() == new_stats)
	{
		// Haven't seen this region_id before, create a new block and make it current.
		mCurRegionStats = new PerRegionStats(region_handle);
		mRegionStats[region_handle] = mCurRegionStats;
	}
	else
	{
		mCurRegionStats = new_stats->second;
	}
	mCurRegionStats->mStartTimestamp = now;
	mRegionHandle = region_handle;
}


void
LLViewerAssetStats::recordGetEnqueued(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));
	
	++(mCurRegionStats->mRequests[int(eac)].mEnqueued);
}
	
void
LLViewerAssetStats::recordGetDequeued(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	++(mCurRegionStats->mRequests[int(eac)].mDequeued);
}

void
LLViewerAssetStats::recordGetServiced(LLViewerAssetType::EType at, bool with_http, bool is_temp, duration_t duration)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	mCurRegionStats->mRequests[int(eac)].mResponse.record(duration);
}

void
LLViewerAssetStats::recordFPS(F32 fps)
{
	mCurRegionStats->mFPS.record(fps);
}

void
LLViewerAssetStats::recordAvatarStats()
{
	std::vector<S32> rez_counts;
	LLVOAvatar::getNearbyRezzedStats(rez_counts);
	mAvatarRezStates = rez_counts;
	mPhaseStats.clear();
	mPhaseStats["cloud"] = LLViewerStats::PhaseMap::getPhaseStats("cloud");
	mPhaseStats["cloud-or-gray"] = LLViewerStats::PhaseMap::getPhaseStats("cloud-or-gray");
}

LLSD
LLViewerAssetStats::asLLSD(bool compact_output)
{
	// Top-level tags
	static const LLSD::String tags[EVACCount] = 
		{
			LLSD::String("get_texture_temp_http"),
			LLSD::String("get_texture_temp_udp"),
			LLSD::String("get_texture_non_temp_http"),
			LLSD::String("get_texture_non_temp_udp"),
			LLSD::String("get_wearable_udp"),
			LLSD::String("get_sound_udp"),
			LLSD::String("get_gesture_udp"),
			LLSD::String("get_other")
		};

	// Stats Group Sub-tags.
	static const LLSD::String enq_tag("enqueued");
	static const LLSD::String deq_tag("dequeued");
	static const LLSD::String rcnt_tag("resp_count");
	static const LLSD::String rmin_tag("resp_min");
	static const LLSD::String rmax_tag("resp_max");
	static const LLSD::String rmean_tag("resp_mean");

	// MMM Group Sub-tags.
	static const LLSD::String cnt_tag("count");
	static const LLSD::String min_tag("min");
	static const LLSD::String max_tag("max");
	static const LLSD::String mean_tag("mean");

	// Avatar sub-tags
	static const LLSD::String avatar_tag("avatar");
	static const LLSD::String avatar_nearby_tag("nearby");
	static const LLSD::String avatar_phase_stats_tag("phase_stats");
	
	const duration_t now = LLViewerAssetStatsFF::get_timestamp();
	mCurRegionStats->accumulateTime(now);

	LLSD regions = LLSD::emptyArray();
	for (PerRegionContainer::iterator it = mRegionStats.begin();
		 mRegionStats.end() != it;
		 ++it)
	{
		if (0 == it->first)
		{
			// Never emit NULL UUID/handle in results.
			continue;
		}

		PerRegionStats & stats = *it->second;
		
		LLSD reg_stat = LLSD::emptyMap();
		
		for (int i = 0; i < LL_ARRAY_SIZE(tags); ++i)
		{
			PerRegionStats::prs_group & group(stats.mRequests[i]);
			
			if ((! compact_output) ||
				group.mEnqueued.getCount() ||
				group.mDequeued.getCount() ||
				group.mResponse.getCount())
			{
				LLSD & slot = reg_stat[tags[i]];
				slot = LLSD::emptyMap();
				slot[enq_tag] = LLSD(S32(stats.mRequests[i].mEnqueued.getCount()));
				slot[deq_tag] = LLSD(S32(stats.mRequests[i].mDequeued.getCount()));
				slot[rcnt_tag] = LLSD(S32(stats.mRequests[i].mResponse.getCount()));
				slot[rmin_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMin() * 1.0e-6));
				slot[rmax_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMax() * 1.0e-6));
				slot[rmean_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMean() * 1.0e-6));
			}
		}

		if ((! compact_output) || stats.mFPS.getCount())
		{
			LLSD & slot = reg_stat["fps"];
			slot = LLSD::emptyMap();
			slot[cnt_tag] = LLSD(S32(stats.mFPS.getCount()));
			slot[min_tag] = LLSD(F64(stats.mFPS.getMin()));
			slot[max_tag] = LLSD(F64(stats.mFPS.getMax()));
			slot[mean_tag] = LLSD(F64(stats.mFPS.getMean()));
		}
		U32 grid_x(0), grid_y(0);
		grid_from_region_handle(it->first, &grid_x, &grid_y);
		reg_stat["grid_x"] = LLSD::Integer(grid_x);
		reg_stat["grid_y"] = LLSD::Integer(grid_y);
		reg_stat["duration"] = LLSD::Real(stats.mTotalTime * 1.0e-6);		
		regions.append(reg_stat);
	}

	LLSD ret = LLSD::emptyMap();
	ret["regions"] = regions;
	ret["duration"] = LLSD::Real((now - mResetTimestamp) * 1.0e-6);
	LLSD avatar_info;
	avatar_info[avatar_nearby_tag] = LLSD::emptyArray();
	for (S32 rez_stat=0; rez_stat < mAvatarRezStates.size(); ++rez_stat)
	{
		std::string rez_status_name = LLVOAvatar::rezStatusToString(rez_stat);
		avatar_info[avatar_nearby_tag][rez_status_name] = mAvatarRezStates[rez_stat];
	}
	avatar_info[avatar_phase_stats_tag]["cloud"] = mPhaseStats["cloud"].getData();
	avatar_info[avatar_phase_stats_tag]["cloud-or-gray"] = mPhaseStats["cloud-or-gray"].getData();
	ret[avatar_tag] = avatar_info;
	
	return ret;
}

void
LLViewerAssetStats::merge(const LLViewerAssetStats & src)
{
	// mRegionHandle, mCurRegionStats and mResetTimestamp are left untouched.
	// Just merge the stats bodies

	const PerRegionContainer::const_iterator it_end(src.mRegionStats.end());
	for (PerRegionContainer::const_iterator it(src.mRegionStats.begin()); it_end != it; ++it)
	{
		PerRegionContainer::iterator dst(mRegionStats.find(it->first));
		if (mRegionStats.end() == dst)
		{
			// Destination is missing data, just make a private copy
			mRegionStats[it->first] = new PerRegionStats(*it->second);
		}
		else
		{
			dst->second->merge(*it->second);
		}
	}
}


// ------------------------------------------------------
// Global free-function definitions (LLViewerAssetStatsFF namespace)
// ------------------------------------------------------

namespace LLViewerAssetStatsFF
{

//
// Target thread is elaborated in the function name.  This could
// have been something 'templatey' like specializations iterated
// over a set of constants but with so few, this is clearer I think.
//
// As for the threads themselves... rather than do fine-grained
// locking as we gather statistics, this code creates a collector
// for each thread, allocated and run independently.  Logging
// happens at relatively infrequent intervals and at that time
// the data is sent to a single thread to be aggregated into
// a single entity with locks, thread safety and other niceties.
//
// A particularly fussy implementation would distribute the
// per-thread pointers across separate cache lines.  But that should
// be beyond current requirements.
//

// 'main' thread - initial program thread

void
set_region_main(LLViewerAssetStats::region_handle_t region_handle)
{
	if (! gViewerAssetStatsMain)
		return;

	gViewerAssetStatsMain->setRegion(region_handle);
}

void
record_enqueue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	if (! gViewerAssetStatsMain)
		return;

	gViewerAssetStatsMain->recordGetEnqueued(at, with_http, is_temp);
}

void
record_dequeue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	if (! gViewerAssetStatsMain)
		return;

	gViewerAssetStatsMain->recordGetDequeued(at, with_http, is_temp);
}

void
record_response_main(LLViewerAssetType::EType at, bool with_http, bool is_temp, LLViewerAssetStats::duration_t duration)
{
	if (! gViewerAssetStatsMain)
		return;

	gViewerAssetStatsMain->recordGetServiced(at, with_http, is_temp, duration);
}

void
record_fps_main(F32 fps)
{
	if (! gViewerAssetStatsMain)
		return;

	gViewerAssetStatsMain->recordFPS(fps);
}

void
record_avatar_stats()
{
	if (! gViewerAssetStatsMain)
		return;

	gViewerAssetStatsMain->recordAvatarStats();
}

// 'thread1' - should be for TextureFetch thread

void
set_region_thread1(LLViewerAssetStats::region_handle_t region_handle)
{
	if (! gViewerAssetStatsThread1)
		return;

	gViewerAssetStatsThread1->setRegion(region_handle);
}

void
record_enqueue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	if (! gViewerAssetStatsThread1)
		return;

	gViewerAssetStatsThread1->recordGetEnqueued(at, with_http, is_temp);
}

void
record_dequeue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	if (! gViewerAssetStatsThread1)
		return;

	gViewerAssetStatsThread1->recordGetDequeued(at, with_http, is_temp);
}

void
record_response_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp, LLViewerAssetStats::duration_t duration)
{
	if (! gViewerAssetStatsThread1)
		return;

	gViewerAssetStatsThread1->recordGetServiced(at, with_http, is_temp, duration);
}


void
init()
{
	if (! gViewerAssetStatsMain)
	{
		gViewerAssetStatsMain = new LLViewerAssetStats();
	}
	if (! gViewerAssetStatsThread1)
	{
		gViewerAssetStatsThread1 = new LLViewerAssetStats();
	}
}

void
cleanup()
{
	delete gViewerAssetStatsMain;
	gViewerAssetStatsMain = 0;

	delete gViewerAssetStatsThread1;
	gViewerAssetStatsThread1 = 0;
}


} // namespace LLViewerAssetStatsFF


// ------------------------------------------------------
// Local function definitions
// ------------------------------------------------------

namespace
{

LLViewerAssetStats::EViewerAssetCategories
asset_type_to_category(const LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	// For statistical purposes, we divide GETs into several
	// populations of asset fetches:
	//  - textures which are de-prioritized in the asset system
	//  - wearables (clothing, bodyparts) which directly affect
	//    user experiences when they log in
	//  - sounds
	//  - gestures
	//  - everything else.
	//
	llassert_always(50 == LLViewerAssetType::AT_COUNT);

	// Multiple asset definitions are floating around so this requires some
	// maintenance and attention.
	static const LLViewerAssetStats::EViewerAssetCategories asset_to_bin_map[LLViewerAssetType::AT_COUNT] =
		{
			LLViewerAssetStats::EVACTextureTempHTTPGet,			// (0) AT_TEXTURE
			LLViewerAssetStats::EVACSoundUDPGet,				// AT_SOUND
			LLViewerAssetStats::EVACOtherGet,					// AT_CALLINGCARD
			LLViewerAssetStats::EVACOtherGet,					// AT_LANDMARK
			LLViewerAssetStats::EVACOtherGet,					// AT_SCRIPT
			LLViewerAssetStats::EVACWearableUDPGet,				// AT_CLOTHING
			LLViewerAssetStats::EVACOtherGet,					// AT_OBJECT
			LLViewerAssetStats::EVACOtherGet,					// AT_NOTECARD
			LLViewerAssetStats::EVACOtherGet,					// AT_CATEGORY
			LLViewerAssetStats::EVACOtherGet,					// AT_ROOT_CATEGORY
			LLViewerAssetStats::EVACOtherGet,					// (10) AT_LSL_TEXT
			LLViewerAssetStats::EVACOtherGet,					// AT_LSL_BYTECODE
			LLViewerAssetStats::EVACOtherGet,					// AT_TEXTURE_TGA
			LLViewerAssetStats::EVACWearableUDPGet,				// AT_BODYPART
			LLViewerAssetStats::EVACOtherGet,					// AT_TRASH
			LLViewerAssetStats::EVACOtherGet,					// AT_SNAPSHOT_CATEGORY
			LLViewerAssetStats::EVACOtherGet,					// AT_LOST_AND_FOUND
			LLViewerAssetStats::EVACSoundUDPGet,				// AT_SOUND_WAV
			LLViewerAssetStats::EVACOtherGet,					// AT_IMAGE_TGA
			LLViewerAssetStats::EVACOtherGet,					// AT_IMAGE_JPEG
			LLViewerAssetStats::EVACGestureUDPGet,				// (20) AT_ANIMATION
			LLViewerAssetStats::EVACGestureUDPGet,				// AT_GESTURE
			LLViewerAssetStats::EVACOtherGet,					// AT_SIMSTATE
			LLViewerAssetStats::EVACOtherGet,					// AT_FAVORITE
			LLViewerAssetStats::EVACOtherGet,					// AT_LINK
			LLViewerAssetStats::EVACOtherGet,					// AT_LINK_FOLDER
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// (30)
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// (40)
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					//
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// 
			LLViewerAssetStats::EVACOtherGet,					// AT_MESH
																// (50)
		};
	
	if (at < 0 || at >= LLViewerAssetType::AT_COUNT)
	{
		return LLViewerAssetStats::EVACOtherGet;
	}
	LLViewerAssetStats::EViewerAssetCategories ret(asset_to_bin_map[at]);
	if (LLViewerAssetStats::EVACTextureTempHTTPGet == ret)
	{
		// Indexed with [is_temp][with_http]
		static const LLViewerAssetStats::EViewerAssetCategories texture_bin_map[2][2] =
			{
				{
					LLViewerAssetStats::EVACTextureNonTempUDPGet,
					LLViewerAssetStats::EVACTextureNonTempHTTPGet,
				},
				{
					LLViewerAssetStats::EVACTextureTempUDPGet,
					LLViewerAssetStats::EVACTextureTempHTTPGet,
				}
			};

		ret = texture_bin_map[is_temp][with_http];
	}
	return ret;
}

} // anonymous namespace
