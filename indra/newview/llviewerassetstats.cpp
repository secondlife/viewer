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
LLViewerAssetStats * gViewerAssetStats(0);

// ------------------------------------------------------
// LLViewerAssetStats class definition
// ------------------------------------------------------
LLViewerAssetStats::LLViewerAssetStats()
	: mRegionHandle(U64(0))
{
	reset();
}


LLViewerAssetStats::LLViewerAssetStats(const LLViewerAssetStats & src)
:	mRegionHandle(src.mRegionHandle),
	mPhaseStats(src.mPhaseStats),
	mAvatarRezStates(src.mAvatarRezStates),
	mRegionRecordings(src.mRegionRecordings)
{
	mCurRecording = &mRegionRecordings[mRegionHandle];
}


void LLViewerAssetStats::reset()
{
	// Empty the map of all region stats
	mRegionRecordings.clear();

	// initialize new recording for current region
	mCurRecording = &mRegionRecordings[mRegionHandle];
}

void LLViewerAssetStats::setRegion(region_handle_t region_handle)
{
	if (region_handle == mRegionHandle)
	{
		// Already active, ignore.
		return;
	}

	mCurRecording->stop();
	mCurRecording = &mRegionRecordings[region_handle];
	mCurRecording->start();

	mRegionHandle = region_handle;
}

void LLViewerAssetStats::recordAvatarStats()
{
	LLVOAvatar::getNearbyRezzedStats(mAvatarRezStates);
	mPhaseStats.clear();
	mPhaseStats["cloud"] = LLViewerStats::PhaseMap::getPhaseStats("cloud");
	mPhaseStats["cloud-or-gray"] = LLViewerStats::PhaseMap::getPhaseStats("cloud-or-gray");
}

struct AssetRequestType : public LLInitParam::Block<AssetRequestType>
{
	Optional<S32>	enqueued,
					dequeued,
					resp_count;
	Optional<F64>	resp_min,
					resp_max,
					resp_mean;
	
	AssetRequestType()
	:	enqueued("enqueued"),
		dequeued("dequeued"),
		resp_count("resp_count"),
		resp_min("resp_min"),
		resp_max("resp_max"),
		resp_mean("resp_mean")
	{}
};

struct FPSStats : public LLInitParam::Block<FPSStats>
{
	Optional<S32>	count;
	Optional<F64>	min,
					max,
					mean;
	FPSStats()
	:	count("count"),
		min("min"),
		max("max"),
		mean("mean")
	{}
};

struct RegionStats : public LLInitParam::Block<RegionStats>
{
	Optional<AssetRequestType>	get_texture_temp_http,
								get_texture_temp_udp,
								get_texture_non_temp_http,
								get_texture_non_temp_udp,
								get_wearable_udp,
								get_sound_udp,
								get_gesture_udp,
								get_other;
	Optional<FPSStats>			fps;
	Mandatory<S32>				grid_x,
								grid_y;
	Mandatory<F64>				duration;

	RegionStats()
	:	get_texture_temp_http("get_texture_temp_http"),
		get_texture_temp_udp("get_texture_temp_udp"),
		get_texture_non_temp_http("get_texture_non_temp_http"),
		get_texture_non_temp_udp("get_texture_non_temp_udp"),
		get_wearable_udp("get_wearable_udp"),
		get_sound_udp("get_sound_udp"),
		get_gesture_udp("get_gesture_udp"),
		get_other("get_other"),
		fps("fps"),
		grid_x("grid_x"),
		grid_y("grid_y"),
		duration("duration")
	{}
};

struct AvatarRezState : public LLInitParam::Block<AvatarRezState>
{
	Mandatory<S32>	cloud,
					gray,
					textured;
	AvatarRezState()
	:	cloud("cloud"),
		gray("gray"),
		textured("textured")
	{}
};

struct AvatarPhaseStats : public LLInitParam::Block<AvatarPhaseStats>
{
	Mandatory<LLSD>	cloud,
					cloud_or_gray;

	AvatarPhaseStats()
	:	cloud("cloud"),
		cloud_or_gray("cloud-or-gray")
	{}
};

struct AvatarInfo : public LLInitParam::Block<AvatarInfo>
{
	Mandatory<AvatarRezState> nearby;
	Mandatory<AvatarPhaseStats> phase_stats;

	AvatarInfo()
	:	nearby("nearby"),
		phase_stats("phase_stats")
	{}
};

struct AssetStats : public LLInitParam::Block<AssetStats>
{
	Multiple<RegionStats>	regions;
	Mandatory<F64>			duration;

	AssetStats()
	:	regions("regions"),
		duration("duration")
	{}

};

//LLSD LLViewerAssetStats::asLLSD(bool compact_output)
//{
//	// Top-level tags
//	static const LLSD::String tags[LLViewerAssetStatsFF::EVACCount] = 
//		{
//			LLSD::String("get_texture_temp_http"),
//			LLSD::String("get_texture_temp_udp"),
//			LLSD::String("get_texture_non_temp_http"),
//			LLSD::String("get_texture_non_temp_udp"),
//			LLSD::String("get_wearable_udp"),
//			LLSD::String("get_sound_udp"),
//			LLSD::String("get_gesture_udp"),
//			LLSD::String("get_other")
//		};
//
//	// Stats Group Sub-tags.
//	static const LLSD::String enq_tag("enqueued");
//	static const LLSD::String deq_tag("dequeued");
//	static const LLSD::String rcnt_tag("resp_count");
//	static const LLSD::String rmin_tag("resp_min");
//	static const LLSD::String rmax_tag("resp_max");
//	static const LLSD::String rmean_tag("resp_mean");
//
//	// MMM Group Sub-tags.
//	static const LLSD::String cnt_tag("count");
//	static const LLSD::String min_tag("min");
//	static const LLSD::String max_tag("max");
//	static const LLSD::String mean_tag("mean");
//
//	// Avatar sub-tags
//	static const LLSD::String avatar_tag("avatar");
//	static const LLSD::String avatar_nearby_tag("nearby");
//	static const LLSD::String avatar_phase_stats_tag("phase_stats");
//	
//	const duration_t now = LLViewerAssetStatsFF::get_timestamp();
//	mCurRegionStats->accumulateTime(now);
//
//	LLSD regions = LLSD::emptyArray();
//	for (PerRegionContainer::iterator it = mRegionStats.begin();
//		 mRegionStats.end() != it;
//		 ++it)
//	{
//		if (0 == it->first)
//		{
//			// Never emit NULL UUID/handle in results.
//			continue;
//		}
//
//		PerRegionStats & stats = *it->second;
//		
//		LLSD reg_stat = LLSD::emptyMap();
//		
//		for (int i = 0; i < LL_ARRAY_SIZE(tags); ++i)
//		{
//			PerRegionStats::prs_group & group(stats.mRequests[i]);
//			
//			if ((! compact_output) ||
//				group.mEnqueued.getCount() ||
//				group.mDequeued.getCount() ||
//				group.mResponse.getCount())
//			{
//				LLSD & slot = reg_stat[tags[i]];
//				slot = LLSD::emptyMap();
//				slot[enq_tag] = LLSD(S32(stats.mRequests[i].mEnqueued.getCount()));
//				slot[deq_tag] = LLSD(S32(stats.mRequests[i].mDequeued.getCount()));
//				slot[rcnt_tag] = LLSD(S32(stats.mRequests[i].mResponse.getCount()));
//				slot[rmin_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMin() * 1.0e-6));
//				slot[rmax_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMax() * 1.0e-6));
//				slot[rmean_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMean() * 1.0e-6));
//			}
//		}
//
//		if ((! compact_output) || stats.mFPS.getCount())
//		{
//			LLSD & slot = reg_stat["fps"];
//			slot = LLSD::emptyMap();
//			slot[cnt_tag] = LLSD(S32(stats.mFPS.getCount()));
//			slot[min_tag] = LLSD(F64(stats.mFPS.getMin()));
//			slot[max_tag] = LLSD(F64(stats.mFPS.getMax()));
//			slot[mean_tag] = LLSD(F64(stats.mFPS.getMean()));
//		}
//		U32 grid_x(0), grid_y(0);
//		grid_from_region_handle(it->first, &grid_x, &grid_y);
//		reg_stat["grid_x"] = LLSD::Integer(grid_x);
//		reg_stat["grid_y"] = LLSD::Integer(grid_y);
//		reg_stat["duration"] = LLSD::Real(stats.mTotalTime * 1.0e-6);		
//		regions.append(reg_stat);
//	}
//
//	LLSD ret = LLSD::emptyMap();
//	ret["regions"] = regions;
//	ret["duration"] = LLSD::Real((now - mResetTimestamp) * 1.0e-6);
//	LLSD avatar_info;
//	avatar_info[avatar_nearby_tag] = LLSD::emptyArray();
//	for (S32 rez_stat=0; rez_stat < mAvatarRezStates.size(); ++rez_stat)
//	{
//		std::string rez_status_name = LLVOAvatar::rezStatusToString(rez_stat);
//		avatar_info[avatar_nearby_tag][rez_status_name] = mAvatarRezStates[rez_stat];
//	}
//	avatar_info[avatar_phase_stats_tag]["cloud"] = mPhaseStats["cloud"].asLLSD();
//	avatar_info[avatar_phase_stats_tag]["cloud-or-gray"] = mPhaseStats["cloud-or-gray"].asLLSD();
//	ret[avatar_tag] = avatar_info;
//	
//	return ret;
//}

// ------------------------------------------------------
// Global free-function definitions (LLViewerAssetStatsFF namespace)
// ------------------------------------------------------

namespace LLViewerAssetStatsFF
{
	static EViewerAssetCategories asset_type_to_category(const LLViewerAssetType::EType at, bool with_http, bool is_temp)
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
		static const EViewerAssetCategories asset_to_bin_map[LLViewerAssetType::AT_COUNT] =
		{
			EVACTextureTempHTTPGet,			// (0) AT_TEXTURE
			EVACSoundUDPGet,				// AT_SOUND
			EVACOtherGet,					// AT_CALLINGCARD
			EVACOtherGet,					// AT_LANDMARK
			EVACOtherGet,					// AT_SCRIPT
			EVACWearableUDPGet,				// AT_CLOTHING
			EVACOtherGet,					// AT_OBJECT
			EVACOtherGet,					// AT_NOTECARD
			EVACOtherGet,					// AT_CATEGORY
			EVACOtherGet,					// AT_ROOT_CATEGORY
			EVACOtherGet,					// (10) AT_LSL_TEXT
			EVACOtherGet,					// AT_LSL_BYTECODE
			EVACOtherGet,					// AT_TEXTURE_TGA
			EVACWearableUDPGet,				// AT_BODYPART
			EVACOtherGet,					// AT_TRASH
			EVACOtherGet,					// AT_SNAPSHOT_CATEGORY
			EVACOtherGet,					// AT_LOST_AND_FOUND
			EVACSoundUDPGet,				// AT_SOUND_WAV
			EVACOtherGet,					// AT_IMAGE_TGA
			EVACOtherGet,					// AT_IMAGE_JPEG
			EVACGestureUDPGet,				// (20) AT_ANIMATION
			EVACGestureUDPGet,				// AT_GESTURE
			EVACOtherGet,					// AT_SIMSTATE
			EVACOtherGet,					// AT_FAVORITE
			EVACOtherGet,					// AT_LINK
			EVACOtherGet,					// AT_LINK_FOLDER
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// (30)
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// (40)
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					//
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// 
			EVACOtherGet,					// AT_MESH
			// (50)
		};

		if (at < 0 || at >= LLViewerAssetType::AT_COUNT)
		{
			return EVACOtherGet;
		}
		EViewerAssetCategories ret(asset_to_bin_map[at]);
		if (EVACTextureTempHTTPGet == ret)
		{
			// Indexed with [is_temp][with_http]
			static const EViewerAssetCategories texture_bin_map[2][2] =
			{
				{
					EVACTextureNonTempUDPGet,
					EVACTextureNonTempHTTPGet,
				},
				{
					EVACTextureTempUDPGet,
					EVACTextureTempHTTPGet,
				}
			};

			ret = texture_bin_map[is_temp][with_http];
		}
		return ret;
	}
static LLTrace::Count<> sEnqueued[EVACCount] = {LLTrace::Count<>("enqueuedassetrequeststemptexturehttp", 
														"Number of temporary texture asset http requests enqueued"),
													LLTrace::Count<>("enqueuedassetrequeststemptextureudp", 
														"Number of temporary texture asset udp requests enqueued"),
													LLTrace::Count<>("enqueuedassetrequestsnontemptexturehttp", 
														"Number of texture asset http requests enqueued"),
													LLTrace::Count<>("enqueuedassetrequestsnontemptextureudp", 
														"Number of texture asset udp requests enqueued"),
													LLTrace::Count<>("enqueuedassetrequestswearableudp", 
														"Number of wearable asset requests enqueued"),
													LLTrace::Count<>("enqueuedassetrequestssoundudp", 
														"Number of sound asset requests enqueued"),
													LLTrace::Count<>("enqueuedassetrequestsgestureudp", 
														"Number of gesture asset requests enqueued"),
													LLTrace::Count<>("enqueuedassetrequestsother", 
														"Number of other asset requests enqueued")};

static LLTrace::Count<> sDequeued[EVACCount] = {LLTrace::Count<>("dequeuedassetrequeststemptexturehttp", 
													"Number of temporary texture asset http requests dequeued"),
												LLTrace::Count<>("dequeuedassetrequeststemptextureudp", 
													"Number of temporary texture asset udp requests dequeued"),
												LLTrace::Count<>("dequeuedassetrequestsnontemptexturehttp", 
													"Number of texture asset http requests dequeued"),
												LLTrace::Count<>("dequeuedassetrequestsnontemptextureudp", 
													"Number of texture asset udp requests dequeued"),
												LLTrace::Count<>("dequeuedassetrequestswearableudp", 
													"Number of wearable asset requests dequeued"),
												LLTrace::Count<>("dequeuedassetrequestssoundudp", 
													"Number of sound asset requests dequeued"),
												LLTrace::Count<>("dequeuedassetrequestsgestureudp", 
													"Number of gesture asset requests dequeued"),
												LLTrace::Count<>("dequeuedassetrequestsother", 
													"Number of other asset requests dequeued")};
static LLTrace::Measurement<LLTrace::Seconds> sResponse[EVACCount] = {LLTrace::Measurement<LLTrace::Seconds>("assetresponsetimestemptexturehttp", 
																			"Time spent responding to temporary texture asset http requests"),
																		LLTrace::Measurement<LLTrace::Seconds>("assetresponsetimestemptextureudp", 
																			"Time spent responding to temporary texture asset udp requests"),
																		LLTrace::Measurement<LLTrace::Seconds>("assetresponsetimesnontemptexturehttp", 
																			"Time spent responding to texture asset http requests"),
																		LLTrace::Measurement<LLTrace::Seconds>("assetresponsetimesnontemptextureudp", 
																			"Time spent responding to texture asset udp requests"),
																		LLTrace::Measurement<LLTrace::Seconds>("assetresponsetimeswearableudp", 
																			"Time spent responding to wearable asset requests"),
																		LLTrace::Measurement<LLTrace::Seconds>("assetresponsetimessoundudp", 
																			"Time spent responding to sound asset requests"),
																		LLTrace::Measurement<LLTrace::Seconds>("assetresponsetimesgestureudp", 
																			"Time spent responding to gesture asset requests"),
																		LLTrace::Measurement<LLTrace::Seconds>("assetresponsetimesother", 
																			"Time spent responding to other asset requests")};

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

void set_region(LLViewerAssetStats::region_handle_t region_handle)
{
	if (! gViewerAssetStats)
		return;

	gViewerAssetStats->setRegion(region_handle);
}

void record_enqueue(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	sEnqueued[int(eac)].add(1);
}

void record_dequeue(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	sDequeued[int(eac)].add(1);
}

void record_response(LLViewerAssetType::EType at, bool with_http, bool is_temp, LLViewerAssetStats::duration_t duration)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	sResponse[int(eac)].sample<LLTrace::Seconds>(duration);
}

void record_avatar_stats()
{
	if (! gViewerAssetStats)
		return;

	gViewerAssetStats->recordAvatarStats();
}

void init()
{
	if (! gViewerAssetStats)
	{
		gViewerAssetStats = new LLViewerAssetStats();
	}
}

void
cleanup()
{
	delete gViewerAssetStats;
	gViewerAssetStats = 0;
}


} // namespace LLViewerAssetStatsFF


