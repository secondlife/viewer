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
		//  - gestures, including animations
		//  - everything else.
		//

        EViewerAssetCategories ret(EVACOtherGet);
        switch (at)
        {
            case LLAssetType::AT_TEXTURE:
                if (is_temp)
                    ret = with_http ? EVACTextureTempHTTPGet : EVACTextureTempUDPGet;
                else
                    ret = with_http ? EVACTextureNonTempHTTPGet : EVACTextureNonTempUDPGet;
                break;
            case LLAssetType::AT_SOUND:
            case LLAssetType::AT_SOUND_WAV:
                ret = with_http ? EVACSoundHTTPGet : EVACSoundUDPGet;
                break;
            case LLAssetType::AT_CLOTHING:
            case LLAssetType::AT_BODYPART:
                ret = with_http ? EVACWearableHTTPGet : EVACWearableUDPGet;
                break;
            case LLAssetType::AT_ANIMATION:
            case LLAssetType::AT_GESTURE:
                ret = with_http ? EVACGestureHTTPGet : EVACGestureUDPGet;
                break;
            default:
                ret = EVACOtherGet;
                break;
        }
		return ret;
	}

/* Note that this is very verbose, in a way that's actually somewhat
 * risky - when adding or removing a bucket, all these arrays have to
 * be updated in parallel (although the risk is somewhat illusory,
 * because none of the names actually affect the final XML output, so
 * you just have to have the right number of distinct names). Why
 * can't we just have an array of stat objects indexed by the bucket
 * index?  Because CountStatHandle doesn't have a default constructor,
 * and is built on a big pile of template code that assumes the name
 * parameter in the constructor is useful and needed and mandatory. We
 * could replace these stat handles with something more accommodating
 * like the LLViewerAssetStats::StatsAccumulator class, but it's hard
 * to justify given how rarely this code gets changed. For now, caveat
 * developer. */

    static LLTrace::CountStatHandle<> sEnqueueAssetRequestsTempTextureHTTP   ("enqueuedassetrequeststemptexturehttp", 
                                                                              "Number of temporary texture asset http requests enqueued"),
                                      sEnqueueAssetRequestsTempTextureUDP    ("enqueuedassetrequeststemptextureudp", 
                                                                              "Number of temporary texture asset udp requests enqueued"),
                                      sEnqueueAssetRequestsNonTempTextureHTTP("enqueuedassetrequestsnontemptexturehttp", 
                                                                              "Number of texture asset http requests enqueued"),
                                      sEnqueueAssetRequestsNonTempTextureUDP ("enqueuedassetrequestsnontemptextureudp", 
                                                                              "Number of texture asset udp requests enqueued"),
                                      sEnqueuedAssetRequestsWearableHTTP      ("enqueuedassetrequestswearablehttp", 
                                                                               "Number of wearable asset http requests enqueued"),
                                      sEnqueuedAssetRequestsWearableUdp      ("enqueuedassetrequestswearableudp", 
                                                                              "Number of wearable asset udp requests enqueued"),
                                      sEnqueuedAssetRequestsSoundHTTP         ("enqueuedassetrequestssoundhttp", 
                                                                               "Number of sound asset http requests enqueued"),
                                      sEnqueuedAssetRequestsSoundUdp         ("enqueuedassetrequestssoundudp", 
                                                                              "Number of sosund asset udp requests enqueued"),
                                      sEnqueuedAssetRequestsGestureHTTP       ("enqueuedassetrequestsgesturehttp", 
                                                                               "Number of gesture asset http requests enqueued"),
                                      sEnqueuedAssetRequestsGestureUdp       ("enqueuedassetrequestsgestureudp", 
                                                                              "Number of gesture asset udp requests enqueued"),
                                      sEnqueuedAssetRequestsOther            ("enqueuedassetrequestsother", 
                                                                              "Number of other asset requests enqueued");

//static LLTrace::CountStatHandle<> sJunkEnqueued[EVACCount];
    static LLTrace::CountStatHandle<>* sEnqueued[EVACCount] = {
 		&sEnqueueAssetRequestsTempTextureHTTP,   
 		&sEnqueueAssetRequestsTempTextureUDP,  
 		&sEnqueueAssetRequestsNonTempTextureHTTP,
 		&sEnqueueAssetRequestsNonTempTextureUDP,
 		&sEnqueuedAssetRequestsWearableHTTP,
 		&sEnqueuedAssetRequestsWearableUdp,
 		&sEnqueuedAssetRequestsSoundHTTP,
 		&sEnqueuedAssetRequestsSoundUdp,
 		&sEnqueuedAssetRequestsGestureHTTP,
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
							sDequeuedAssetRequestsWearableHTTP      ("dequeuedassetrequestswearablehttp", 
																	"Number of wearable asset http requests dequeued"),
							sDequeuedAssetRequestsWearableUdp      ("dequeuedassetrequestswearableudp", 
																	"Number of wearable asset udp requests dequeued"),
							sDequeuedAssetRequestsSoundHTTP         ("dequeuedassetrequestssoundhttp", 
																	"Number of sound asset http requests dequeued"),
							sDequeuedAssetRequestsSoundUdp         ("dequeuedassetrequestssoundudp", 
																	"Number of sound asset udp requests dequeued"),
							sDequeuedAssetRequestsGestureHTTP       ("dequeuedassetrequestsgesturehttp", 
																	"Number of gesture asset http requests dequeued"),
							sDequeuedAssetRequestsGestureUdp       ("dequeuedassetrequestsgestureudp", 
																	"Number of gesture asset udp requests dequeued"),
							sDequeuedAssetRequestsOther            ("dequeuedassetrequestsother", 
																	"Number of other asset requests dequeued");

	static LLTrace::CountStatHandle<>* sDequeued[EVACCount] = {
		&sDequeueAssetRequestsTempTextureHTTP,   
		&sDequeueAssetRequestsTempTextureUDP,  
		&sDequeueAssetRequestsNonTempTextureHTTP,
		&sDequeueAssetRequestsNonTempTextureUDP,
		&sDequeuedAssetRequestsWearableHTTP,
		&sDequeuedAssetRequestsWearableUdp,
		&sDequeuedAssetRequestsSoundHTTP,
		&sDequeuedAssetRequestsSoundUdp,
		&sDequeuedAssetRequestsGestureHTTP,
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
													sResponsedAssetRequestsWearableHTTP      ("assetresponsetimeswearablehttp", 
																							"Time spent responding to wearable asset http requests"),
													sResponsedAssetRequestsWearableUdp      ("assetresponsetimeswearableudp", 
																							"Time spent responding to wearable asset udp requests"),
													sResponsedAssetRequestsSoundHTTP         ("assetresponsetimessounduhttp", 
																							"Time spent responding to sound asset http requests"),
													sResponsedAssetRequestsSoundUdp         ("assetresponsetimessoundudp", 
																							"Time spent responding to sound asset udp requests"),
													sResponsedAssetRequestsGestureHTTP       ("assetresponsetimesgesturehttp", 
																							"Time spent responding to gesture asset http requests"),
													sResponsedAssetRequestsGestureUdp       ("assetresponsetimesgestureudp", 
																							"Time spent responding to gesture asset udp requests"),
													sResponsedAssetRequestsOther            ("assetresponsetimesother", 
																							"Time spent responding to other asset requests");

	static LLTrace::EventStatHandle<F64Seconds >* sResponse[EVACCount] = {
		&sResponseAssetRequestsTempTextureHTTP,   
		&sResponseAssetRequestsTempTextureUDP,  
		&sResponseAssetRequestsNonTempTextureHTTP,
		&sResponseAssetRequestsNonTempTextureUDP,
		&sResponsedAssetRequestsWearableHTTP,
		&sResponsedAssetRequestsWearableUdp,
		&sResponsedAssetRequestsSoundHTTP,
		&sResponsedAssetRequestsSoundUdp,
		&sResponsedAssetRequestsGestureHTTP,
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

template <typename T>
void LLViewerAssetStats::getStat(LLTrace::Recording& rec, T& req, LLViewerAssetStatsFF::EViewerAssetCategories cat, bool compact_output)
{
	using namespace LLViewerAssetStatsFF;

    if (!compact_output
        || rec.getSampleCount(*sEnqueued[cat]) 
        || rec.getSampleCount(*sDequeued[cat])
        || rec.getSampleCount(*sResponse[cat]))
    {
        req	.enqueued(rec.getSampleCount(*sEnqueued[cat]))
            .dequeued(rec.getSampleCount(*sDequeued[cat]))
            .resp_count(rec.getSampleCount(*sResponse[cat]))
            .resp_min(rec.getMin(*sResponse[cat]).value())
            .resp_max(rec.getMax(*sResponse[cat]).value())
            .resp_mean(rec.getMean(*sResponse[cat]).value());
    }
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

        getStat(rec, r.get_texture_temp_http, EVACTextureTempHTTPGet, compact_output);
        getStat(rec, r.get_texture_temp_udp, EVACTextureTempUDPGet, compact_output);
        getStat(rec, r.get_texture_non_temp_http, EVACTextureNonTempHTTPGet, compact_output);
        getStat(rec, r.get_texture_non_temp_udp, EVACTextureNonTempUDPGet, compact_output);
        getStat(rec, r.get_wearable_http, EVACWearableHTTPGet, compact_output);
        getStat(rec, r.get_wearable_udp, EVACWearableUDPGet, compact_output);
        getStat(rec, r.get_sound_http, EVACSoundHTTPGet, compact_output);
        getStat(rec, r.get_sound_udp, EVACSoundUDPGet, compact_output);
        getStat(rec, r.get_gesture_http, EVACGestureHTTPGet, compact_output);
        getStat(rec, r.get_gesture_udp, EVACGestureUDPGet, compact_output);
        getStat(rec, r.get_other, EVACOtherGet, compact_output);
        
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
	get_wearable_http("get_wearable_http"),
	get_wearable_udp("get_wearable_udp"),
	get_sound_http("get_sound_http"),
	get_sound_udp("get_sound_udp"),
	get_gesture_http("get_gesture_http"),
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
