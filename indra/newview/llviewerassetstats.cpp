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
#include "llsdparam.h"
#include "llsdutil.h"

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

	static LLTrace::CountStatHandle<> sEnqueueAssetRequestsTempTextureHTTP   ("enqueuedassetrequeststemptexturehttp", 
																	"Number of temporary texture asset http requests enqueued"),
							sEnqueueAssetRequestsTempTextureUDP    ("enqueuedassetrequeststemptextureudp", 
																	"Number of temporary texture asset udp requests enqueued"),
							sEnqueueAssetRequestsNonTempTextureHTTP("enqueuedassetrequestsnontemptexturehttp", 
																	"Number of texture asset http requests enqueued"),
							sEnqueueAssetRequestsNonTempTextureUDP ("enqueuedassetrequestsnontemptextureudp", 
																	"Number of texture asset udp requests enqueued"),
							sEnqueuedAssetRequestsWearableUdp      ("enqueuedassetrequestswearableudp", 
																	"Number of wearable asset requests enqueued"),
							sEnqueuedAssetRequestsSoundUdp         ("enqueuedassetrequestssoundudp", 
																	"Number of sound asset requests enqueued"),
							sEnqueuedAssetRequestsGestureUdp       ("enqueuedassetrequestsgestureudp", 
																	"Number of gesture asset requests enqueued"),
							sEnqueuedAssetRequestsOther            ("enqueuedassetrequestsother", 
																	"Number of other asset requests enqueued");

	static LLTrace::CountStatHandle<>* sEnqueued[EVACCount] = {		
		&sEnqueueAssetRequestsTempTextureHTTP,   
		&sEnqueueAssetRequestsTempTextureUDP,  
		&sEnqueueAssetRequestsNonTempTextureHTTP,
		&sEnqueueAssetRequestsNonTempTextureUDP,
		&sEnqueuedAssetRequestsWearableUdp,
		&sEnqueuedAssetRequestsSoundUdp,
		&sEnqueuedAssetRequestsGestureUdp,
		&sEnqueuedAssetRequestsOther            
	};
	
	static LLTrace::CountStatHandle<> sDequeueAssetRequestsTempTextureHTTP   ("dequeuedassetrequeststemptexturehttp", 
																	"Number of temporary texture asset http requests dequeued"),
							sDequeueAssetRequestsTempTextureUDP    ("dequeuedassetrequeststemptextureudp", 
																	"Number of temporary texture asset udp requests dequeued"),
							sDequeueAssetRequestsNonTempTextureHTTP("dequeuedassetrequestsnontemptexturehttp", 
																	"Number of texture asset http requests dequeued"),
							sDequeueAssetRequestsNonTempTextureUDP ("dequeuedassetrequestsnontemptextureudp", 
																	"Number of texture asset udp requests dequeued"),
							sDequeuedAssetRequestsWearableUdp      ("dequeuedassetrequestswearableudp", 
																	"Number of wearable asset requests dequeued"),
							sDequeuedAssetRequestsSoundUdp         ("dequeuedassetrequestssoundudp", 
																	"Number of sound asset requests dequeued"),
							sDequeuedAssetRequestsGestureUdp       ("dequeuedassetrequestsgestureudp", 
																	"Number of gesture asset requests dequeued"),
							sDequeuedAssetRequestsOther            ("dequeuedassetrequestsother", 
																	"Number of other asset requests dequeued");

	static LLTrace::CountStatHandle<>* sDequeued[EVACCount] = {
		&sDequeueAssetRequestsTempTextureHTTP,   
		&sDequeueAssetRequestsTempTextureUDP,  
		&sDequeueAssetRequestsNonTempTextureHTTP,
		&sDequeueAssetRequestsNonTempTextureUDP,
		&sDequeuedAssetRequestsWearableUdp,
		&sDequeuedAssetRequestsSoundUdp,
		&sDequeuedAssetRequestsGestureUdp,
		&sDequeuedAssetRequestsOther            
	};

	static LLTrace::EventStatHandle<F64Seconds >	sResponseAssetRequestsTempTextureHTTP   ("assetresponsetimestemptexturehttp",
																							"Time spent responding to temporary texture asset http requests"),
													sResponseAssetRequestsTempTextureUDP    ("assetresponsetimestemptextureudp", 
																							"Time spent responding to temporary texture asset udp requests"),
													sResponseAssetRequestsNonTempTextureHTTP("assetresponsetimesnontemptexturehttp", 
																							"Time spent responding to texture asset http requests"),
													sResponseAssetRequestsNonTempTextureUDP ("assetresponsetimesnontemptextureudp", 
																							"Time spent responding to texture asset udp requests"),
													sResponsedAssetRequestsWearableUdp      ("assetresponsetimeswearableudp", 
																							"Time spent responding to wearable asset requests"),
													sResponsedAssetRequestsSoundUdp         ("assetresponsetimessoundudp", 
																							"Time spent responding to sound asset requests"),
													sResponsedAssetRequestsGestureUdp       ("assetresponsetimesgestureudp", 
																							"Time spent responding to gesture asset requests"),
													sResponsedAssetRequestsOther            ("assetresponsetimesother", 
																							"Time spent responding to other asset requests");

	static LLTrace::EventStatHandle<F64Seconds >* sResponse[EVACCount] = {
		&sResponseAssetRequestsTempTextureHTTP,   
		&sResponseAssetRequestsTempTextureUDP,  
		&sResponseAssetRequestsNonTempTextureHTTP,
		&sResponseAssetRequestsNonTempTextureUDP,
		&sResponsedAssetRequestsWearableUdp,
		&sResponsedAssetRequestsSoundUdp,
		&sResponsedAssetRequestsGestureUdp,
		&sResponsedAssetRequestsOther            
	};
}

// ------------------------------------------------------
// Global data definitions
// ------------------------------------------------------
LLViewerAssetStats * gViewerAssetStats(0);

// ------------------------------------------------------
// LLViewerAssetStats class definition
// ------------------------------------------------------
LLViewerAssetStats::LLViewerAssetStats()
:	mRegionHandle(U64(0)),
	mCurRecording(NULL)
{
	start();
}


LLViewerAssetStats::LLViewerAssetStats(const LLViewerAssetStats & src)
:	mRegionHandle(src.mRegionHandle)
{
	mRegionRecordings = src.mRegionRecordings;

	mCurRecording = &mRegionRecordings[mRegionHandle];
	
	// assume this is being passed to another thread, so make sure we have unique copies of recording data
	for (PerRegionRecordingContainer::iterator it = mRegionRecordings.begin(), end_it = mRegionRecordings.end();
		it != end_it;
		++it)
	{
		it->second.stop();
		it->second.makeUnique();
	}

	LLStopWatchControlsMixin<LLViewerAssetStats>::setPlayState(src.getPlayState());
}

void LLViewerAssetStats::handleStart()
{
	if (mCurRecording)
	{
		mCurRecording->start();
	}
}

void LLViewerAssetStats::handleStop()
{
	if (mCurRecording)
	{
		mCurRecording->stop();
	}
}

void LLViewerAssetStats::handleReset()
{
	reset();
}


void LLViewerAssetStats::reset()
{
	// Empty the map of all region stats
	mRegionRecordings.clear();

	// initialize new recording for current region
	if (mRegionHandle)
	{
		mCurRecording = &mRegionRecordings[mRegionHandle];
		mCurRecording->setPlayState(getPlayState());
	}
}

void LLViewerAssetStats::setRegion(region_handle_t region_handle)
{
	if (region_handle == mRegionHandle)
	{
		// Already active, ignore.
		return;
	}

	if (mCurRecording)
	{
		mCurRecording->pause();
	}
	if (region_handle)
	{
		mCurRecording = &mRegionRecordings[region_handle];
		mCurRecording->setPlayState(getPlayState());
	}

	mRegionHandle = region_handle;
}

void LLViewerAssetStats::getStats(AssetStats& stats, bool compact_output)
{
	using namespace LLViewerAssetStatsFF;

	stats.regions.setProvided();
	
	for (PerRegionRecordingContainer::iterator it = mRegionRecordings.begin(), end_it = mRegionRecordings.end();
		it != end_it;
		++it)
	{
		RegionStats& r = stats.regions.add();
		LLTrace::Recording& rec = it->second;
		if (!compact_output
			|| rec.getSum(*sEnqueued[EVACTextureTempHTTPGet]) 
			|| rec.getSum(*sDequeued[EVACTextureTempHTTPGet])
			|| rec.getSum(*sResponse[EVACTextureTempHTTPGet]).value())
		{
			r.get_texture_temp_http	.enqueued((S32)rec.getSum(*sEnqueued[EVACTextureTempHTTPGet]))
									.dequeued((S32)rec.getSum(*sDequeued[EVACTextureTempHTTPGet]))
									.resp_count((S32)rec.getSum(*sResponse[EVACTextureTempHTTPGet]).value())
									.resp_min(rec.getMin(*sResponse[EVACTextureTempHTTPGet]).value())
									.resp_max(rec.getMax(*sResponse[EVACTextureTempHTTPGet]).value())
									.resp_mean(rec.getMean(*sResponse[EVACTextureTempHTTPGet]).value());
		}
		if (!compact_output
			|| rec.getSum(*sEnqueued[EVACTextureTempUDPGet]) 
			|| rec.getSum(*sDequeued[EVACTextureTempUDPGet])
			|| rec.getSum(*sResponse[EVACTextureTempUDPGet]).value())
		{
			r.get_texture_temp_udp	.enqueued((S32)rec.getSum(*sEnqueued[EVACTextureTempUDPGet]))
									.dequeued((S32)rec.getSum(*sDequeued[EVACTextureTempUDPGet]))
									.resp_count((S32)rec.getSum(*sResponse[EVACTextureTempUDPGet]).value())
									.resp_min(rec.getMin(*sResponse[EVACTextureTempUDPGet]).value())
									.resp_max(rec.getMax(*sResponse[EVACTextureTempUDPGet]).value())
									.resp_mean(rec.getMean(*sResponse[EVACTextureTempUDPGet]).value());
		}
		if (!compact_output
			|| rec.getSum(*sEnqueued[EVACTextureNonTempHTTPGet]) 
			|| rec.getSum(*sDequeued[EVACTextureNonTempHTTPGet])
			|| rec.getSum(*sResponse[EVACTextureNonTempHTTPGet]).value())
		{
			r.get_texture_non_temp_http	.enqueued((S32)rec.getSum(*sEnqueued[EVACTextureNonTempHTTPGet]))
										.dequeued((S32)rec.getSum(*sDequeued[EVACTextureNonTempHTTPGet]))
										.resp_count((S32)rec.getSum(*sResponse[EVACTextureNonTempHTTPGet]).value())
										.resp_min(rec.getMin(*sResponse[EVACTextureNonTempHTTPGet]).value())
										.resp_max(rec.getMax(*sResponse[EVACTextureNonTempHTTPGet]).value())
										.resp_mean(rec.getMean(*sResponse[EVACTextureNonTempHTTPGet]).value());
		}

		if (!compact_output
			|| rec.getSum(*sEnqueued[EVACTextureNonTempUDPGet]) 
			|| rec.getSum(*sDequeued[EVACTextureNonTempUDPGet])
			|| rec.getSum(*sResponse[EVACTextureNonTempUDPGet]).value())
		{
			r.get_texture_non_temp_udp	.enqueued((S32)rec.getSum(*sEnqueued[EVACTextureNonTempUDPGet]))
										.dequeued((S32)rec.getSum(*sDequeued[EVACTextureNonTempUDPGet]))
										.resp_count((S32)rec.getSum(*sResponse[EVACTextureNonTempUDPGet]).value())
										.resp_min(rec.getMin(*sResponse[EVACTextureNonTempUDPGet]).value())
										.resp_max(rec.getMax(*sResponse[EVACTextureNonTempUDPGet]).value())
										.resp_mean(rec.getMean(*sResponse[EVACTextureNonTempUDPGet]).value());
		}

		if (!compact_output
			|| rec.getSum(*sEnqueued[EVACWearableUDPGet]) 
			|| rec.getSum(*sDequeued[EVACWearableUDPGet])
			|| rec.getSum(*sResponse[EVACWearableUDPGet]).value())
		{
			r.get_wearable_udp	.enqueued((S32)rec.getSum(*sEnqueued[EVACWearableUDPGet]))
								.dequeued((S32)rec.getSum(*sDequeued[EVACWearableUDPGet]))
								.resp_count((S32)rec.getSum(*sResponse[EVACWearableUDPGet]).value())
								.resp_min(rec.getMin(*sResponse[EVACWearableUDPGet]).value())
								.resp_max(rec.getMax(*sResponse[EVACWearableUDPGet]).value())
								.resp_mean(rec.getMean(*sResponse[EVACWearableUDPGet]).value());
		}

		if (!compact_output
			|| rec.getSum(*sEnqueued[EVACSoundUDPGet]) 
			|| rec.getSum(*sDequeued[EVACSoundUDPGet])
			|| rec.getSum(*sResponse[EVACSoundUDPGet]).value())
		{
			r.get_sound_udp	.enqueued((S32)rec.getSum(*sEnqueued[EVACSoundUDPGet]))
							.dequeued((S32)rec.getSum(*sDequeued[EVACSoundUDPGet]))
							.resp_count((S32)rec.getSum(*sResponse[EVACSoundUDPGet]).value())
							.resp_min(rec.getMin(*sResponse[EVACSoundUDPGet]).value())
							.resp_max(rec.getMax(*sResponse[EVACSoundUDPGet]).value())
							.resp_mean(rec.getMean(*sResponse[EVACSoundUDPGet]).value());
		}

		if (!compact_output
			|| rec.getSum(*sEnqueued[EVACGestureUDPGet]) 
			|| rec.getSum(*sDequeued[EVACGestureUDPGet])
			|| rec.getSum(*sResponse[EVACGestureUDPGet]).value())
		{
			r.get_gesture_udp	.enqueued((S32)rec.getSum(*sEnqueued[EVACGestureUDPGet]))
								.dequeued((S32)rec.getSum(*sDequeued[EVACGestureUDPGet]))
								.resp_count((S32)rec.getSum(*sResponse[EVACGestureUDPGet]).value())
								.resp_min(rec.getMin(*sResponse[EVACGestureUDPGet]).value())
								.resp_max(rec.getMax(*sResponse[EVACGestureUDPGet]).value())
								.resp_mean(rec.getMean(*sResponse[EVACGestureUDPGet]).value());
		}
			
		if (!compact_output
			|| rec.getSum(*sEnqueued[EVACOtherGet]) 
			|| rec.getSum(*sDequeued[EVACOtherGet])
			|| rec.getSum(*sResponse[EVACOtherGet]).value())
			{
			r.get_other	.enqueued((S32)rec.getSum(*sEnqueued[EVACOtherGet]))
						.dequeued((S32)rec.getSum(*sDequeued[EVACOtherGet]))
						.resp_count((S32)rec.getSum(*sResponse[EVACOtherGet]).value())
						.resp_min(rec.getMin(*sResponse[EVACOtherGet]).value())
						.resp_max(rec.getMax(*sResponse[EVACOtherGet]).value())
						.resp_mean(rec.getMean(*sResponse[EVACOtherGet]).value());
		}

		S32 fps = (S32)rec.getLastValue(LLStatViewer::FPS_SAMPLE);
		if (!compact_output || fps != 0)
		{
			r.fps	.count(fps)
					.min(rec.getMin(LLStatViewer::FPS_SAMPLE))
					.max(rec.getMax(LLStatViewer::FPS_SAMPLE))
					.mean(rec.getMean(LLStatViewer::FPS_SAMPLE));
		}
		U32 grid_x(0), grid_y(0);
		grid_from_region_handle(it->first, &grid_x, &grid_y);
		r	.grid_x(grid_x)
			.grid_y(grid_y)
			.duration(F64Microseconds(rec.getDuration()).value());
	}

	stats.duration(mCurRecording ? F64Microseconds(mCurRecording->getDuration()).value() : 0.0);
}

LLSD LLViewerAssetStats::asLLSD(bool compact_output)
{
	LLParamSDParser parser;
	LLSD sd;
	AssetStats stats;
	getStats(stats, compact_output);
	LLInitParam::predicate_rule_t rule = LLInitParam::default_parse_rules();
	if (!compact_output)
	{
		rule.allow(LLInitParam::EMPTY);
	}
	parser.writeSD(sd, stats, rule);
	return sd;
}

// ------------------------------------------------------
// Global free-function definitions (LLViewerAssetStatsFF namespace)
// ------------------------------------------------------

namespace LLViewerAssetStatsFF
{
void set_region(LLViewerAssetStats::region_handle_t region_handle)
{
	if (! gViewerAssetStats)
		return;

	gViewerAssetStats->setRegion(region_handle);
}

void record_enqueue(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	add(*sEnqueued[int(eac)], 1);
}

void record_dequeue(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	add(*sDequeued[int(eac)], 1);
}

void record_response(LLViewerAssetType::EType at, bool with_http, bool is_temp, LLViewerAssetStats::duration_t duration)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	record(*sResponse[int(eac)], F64Microseconds(duration));
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



LLViewerAssetStats::AssetRequestType::AssetRequestType() 
:	enqueued("enqueued"),
	dequeued("dequeued"),
	resp_count("resp_count"),
	resp_min("resp_min"),
	resp_max("resp_max"),
	resp_mean("resp_mean")
{}
	
LLViewerAssetStats::FPSStats::FPSStats() 
:	count("count"),
	min("min"),
	max("max"),
	mean("mean")
{}

LLViewerAssetStats::RegionStats::RegionStats() 
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

LLViewerAssetStats::AssetStats::AssetStats() 
:	regions("regions"),
	duration("duration"),
	session_id("session_id"),
	agent_id("agent_id"),
	message("message"),
	sequence("sequence"),
	initial("initial"),
	break_("break")
{}
