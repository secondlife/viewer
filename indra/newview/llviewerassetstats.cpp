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

namespace LLTrace
{
// This little bit of shimmery is to allow the creation of
// default-constructed stat and event handles so we can make arrays of
// the things.

// The only sensible way to use this function is to immediately make a
// copy of the contents, since it always returns the same pointer.
const char *makeNewAutoName()
{
    static char name[64];
    static S32 auto_namer_number = 0;
    snprintf(name,64,"auto_name_%d",auto_namer_number);
    auto_namer_number++;
    return name;
}

template <typename T = F64>
class DCCountStatHandle:
        public CountStatHandle<T>
{
public:
    DCCountStatHandle(const char *name = makeNewAutoName(), const char *description=NULL):
        CountStatHandle<T>(name,description)
    {
    }
};

template <typename T = F64>
class DCEventStatHandle:
        public EventStatHandle<T>
{
public:
    DCEventStatHandle(const char *name = makeNewAutoName(), const char *description=NULL):
        EventStatHandle<T>(name,description)
    {
    }
};

}

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

        EViewerAssetCategories ret;
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
            case LLAssetType::AT_LANDMARK:
                ret = with_http ? EVACLandmarkHTTPGet : EVACLandmarkUDPGet;
                break;
            default:
                ret = with_http ? EVACOtherHTTPGet : EVACOtherUDPGet;
                break;
        }
        return ret;
    }

    static LLTrace::DCCountStatHandle<> sEnqueued[EVACCount];
    static LLTrace::DCCountStatHandle<> sDequeued[EVACCount];
    static LLTrace::DCEventStatHandle<> sBytesFetched[EVACCount];
    static LLTrace::DCEventStatHandle<F64Seconds > sResponse[EVACCount];
}

// ------------------------------------------------------
// Global data definitions
// ------------------------------------------------------
LLViewerAssetStats * gViewerAssetStats(0);

// ------------------------------------------------------
// LLViewerAssetStats class definition
// ------------------------------------------------------
LLViewerAssetStats::LLViewerAssetStats()
:   mRegionHandle(U64(0)),
    mCurRecording(NULL)
{
    start();
}


LLViewerAssetStats::LLViewerAssetStats(const LLViewerAssetStats & src)
:   mRegionHandle(src.mRegionHandle)
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
        || rec.getSampleCount(sEnqueued[cat]) 
        || rec.getSampleCount(sDequeued[cat])
        || rec.getSampleCount(sResponse[cat]))
    {
        req .enqueued(rec.getSampleCount(sEnqueued[cat]))
            .dequeued(rec.getSampleCount(sDequeued[cat]))
            .resp_count(rec.getSampleCount(sResponse[cat]))
            .resp_min(rec.getMin(sResponse[cat]).value())
            .resp_max(rec.getMax(sResponse[cat]).value())
            .resp_mean(rec.getMean(sResponse[cat]).value())
            .resp_mean_bytes(rec.getMean(sBytesFetched[cat]));
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
        getStat(rec, r.get_landmark_http, EVACLandmarkHTTPGet, compact_output);
        getStat(rec, r.get_landmark_udp, EVACLandmarkUDPGet, compact_output);
        getStat(rec, r.get_other_http, EVACOtherHTTPGet, compact_output);
        getStat(rec, r.get_other_udp, EVACOtherUDPGet, compact_output);
        
        S32 fps = (S32)rec.getLastValue(LLStatViewer::FPS_SAMPLE);
        if (!compact_output || fps != 0)
        {
            r.fps   .count(fps)
                    .min(rec.getMin(LLStatViewer::FPS_SAMPLE))
                    .max(rec.getMax(LLStatViewer::FPS_SAMPLE))
                    .mean(rec.getMean(LLStatViewer::FPS_SAMPLE));
        }
        U32 grid_x(0), grid_y(0);
        grid_from_region_handle(it->first, &grid_x, &grid_y);
        r   .grid_x(grid_x)
            .grid_y(grid_y)
            .duration(F64Seconds(rec.getDuration()).value());
    }

    stats.duration(mCurRecording ? F64Seconds(mCurRecording->getDuration()).value() : 0.0);
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

    add(sEnqueued[int(eac)], 1);
}

void record_dequeue(LLViewerAssetType::EType at, bool with_http, bool is_temp)
{
    const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

    add(sDequeued[int(eac)], 1);
}

void record_response(LLViewerAssetType::EType at, bool with_http, bool is_temp, LLViewerAssetStats::duration_t duration, F64 bytes)
{
    const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

    record(sResponse[int(eac)], F64Seconds(duration));
    record(sBytesFetched[int(eac)], bytes);
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
:   enqueued("enqueued"),
    dequeued("dequeued"),
    resp_count("resp_count"),
    resp_min("resp_min"),
    resp_max("resp_max"),
    resp_mean("resp_mean"),
    resp_mean_bytes("resp_mean_bytes")
{}
    
LLViewerAssetStats::FPSStats::FPSStats() 
:   count("count"),
    min("min"),
    max("max"),
    mean("mean")
{}

LLViewerAssetStats::RegionStats::RegionStats() 
:   get_texture_temp_http("get_texture_temp_http"),
    get_texture_temp_udp("get_texture_temp_udp"),
    get_texture_non_temp_http("get_texture_non_temp_http"),
    get_texture_non_temp_udp("get_texture_non_temp_udp"),
    get_wearable_http("get_wearable_http"),
    get_wearable_udp("get_wearable_udp"),
    get_sound_http("get_sound_http"),
    get_sound_udp("get_sound_udp"),
    get_gesture_http("get_gesture_http"),
    get_gesture_udp("get_gesture_udp"),
    get_landmark_http("get_landmark_http"),
    get_landmark_udp("get_landmark_udp"),
    get_other_http("get_other_http"),
    get_other_udp("get_other_udp"),
    fps("fps"),
    grid_x("grid_x"),
    grid_y("grid_y"),
    duration("duration")
{}

LLViewerAssetStats::AssetStats::AssetStats() 
:   regions("regions"),
    duration("duration"),
    session_id("session_id"),
    agent_id("agent_id"),
    message("message"),
    sequence("sequence"),
    initial("initial"),
    break_("break")
{}
