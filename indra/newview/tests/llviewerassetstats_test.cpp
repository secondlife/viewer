/**
 * @file llviewerassetstats_tut.cpp
 * @date 2010-10-28
 * @brief Test cases for some of newview/llviewerassetstats.cpp
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

#include "linden_common.h"

#include "../test/doctest.h"
#include <iostream>

#include "../test/lldoctest.h"
#include "../llviewerassetstats.h"
#include "lluuid.h"
#include "llsdutil.h"
#include "llregionhandle.h"
#include "lltracethreadrecorder.h"
#include "../llvoavatar.h"

namespace LLStatViewer
{
    LLTrace::SampleStatHandle<>     FPS_SAMPLE("fpssample");
}

void LLVOAvatar::getNearbyRezzedStats(std::vector<S32>& counts, F32& avg_cloud_time, S32& cloud_avatars, S32& pending_meshes, S32& control_avatars)
{
    counts.resize(3);
    counts[0] = 0;
    counts[1] = 0;
    counts[2] = 1;
    cloud_avatars = 0;
    pending_meshes = 0;
    control_avatars = 0;
}

// static
std::string LLVOAvatar::rezStatusToString(S32 rez_status)
{
    if (rez_status==0) return "cloud";
    if (rez_status==1) return "gray";
    if (rez_status==2) return "textured";
    return "unknown";
}

// static
LLViewerStats::StatsAccumulator& LLViewerStats::PhaseMap::getPhaseStats(const std::string& phase_name)
{
    static LLViewerStats::StatsAccumulator junk;
    return junk;
}

static const char * all_keys[] =
{
    "duration",
    "fps",
    "get_other_http",
    "get_other_udp",
    "get_texture_temp_http",
    "get_texture_temp_udp",
    "get_texture_non_temp_http",
    "get_texture_non_temp_udp",
    "get_wearable_http",
    "get_wearable_udp",
    "get_sound_http",
    "get_sound_udp",
    "get_gesture_http",
    "get_gesture_udp"
};

static const char * resp_keys[] =
{
    "get_other_http",
    "get_other_udp",
    "get_texture_temp_http",
    "get_texture_temp_udp",
    "get_texture_non_temp_http",
    "get_texture_non_temp_udp",
    "get_wearable_http",
    "get_wearable_udp",
    "get_sound_http",
    "get_sound_udp",
    "get_gesture_http",
    "get_gesture_udp"
};

static const char * sub_keys[] =
{
    "dequeued",
    "enqueued",
    "resp_count",
    "resp_max",
    "resp_min",
    "resp_mean"
};

static const char * mmm_resp_keys[] =
{
    "fps"
};

static const char * mmm_sub_keys[] =
{
    "count",
    "max",
    "min",
    "mean"
};

static const LLUUID region1("4e2d81a3-6263-6ffe-ad5c-8ce04bee07e8");
static const LLUUID region2("68762cc8-b68b-4e45-854b-e830734f2d4a");
static const U64 region1_handle(0x0000040000003f00ULL);
static const U64 region2_handle(0x0000030000004200ULL);
static const std::string region1_handle_str("0000040000003f00");
static const std::string region2_handle_str("0000030000004200");

#if 0
static bool
is_empty_map(const LLSD & sd)
{
    return sd.isMap() && 0 == sd.size();
}
#endif

#if 0
static bool
is_single_key_map(const LLSD & sd, const std::string & key)
{
    return sd.isMap() && 1 == sd.size() && sd.has(key);
}
#endif

static bool
is_double_key_map(const LLSD & sd, const std::string & key1, const std::string & key2)
{
    return sd.isMap() && 2 == sd.size() && sd.has(key1) && sd.has(key2);
}

#if 0
static bool
is_triple_key_map(const LLSD & sd, const std::string & key1, const std::string & key2, const std::string& key3)
{
    return sd.isMap() && 3 == sd.size() && sd.has(key1) && sd.has(key2) && sd.has(key3);
}
#endif

static bool
is_no_stats_map(const LLSD & sd)
{
    return is_double_key_map(sd, "duration", "regions");
}

static bool
is_single_slot_array(const LLSD & sd, U64 region_handle)
{
    U32 grid_x(0), grid_y(0);
    grid_from_region_handle(region_handle, &grid_x, &grid_y);

    return (sd.isArray() &&
            1 == sd.size() &&
            sd[0].has("grid_x") &&
            sd[0].has("grid_y") &&
            sd[0]["grid_x"].isInteger() &&
            sd[0]["grid_y"].isInteger() &&
            grid_x == sd[0]["grid_x"].asInteger() &&
            grid_y == sd[0]["grid_y"].asInteger());
}

static bool
is_double_slot_array(const LLSD & sd, U64 region_handle1, U64 region_handle2)
{
    U32 grid_x1(0), grid_y1(0);
    U32 grid_x2(0), grid_y2(0);
    grid_from_region_handle(region_handle1, &grid_x1, &grid_y1);
    grid_from_region_handle(region_handle2, &grid_x2, &grid_y2);

    return (sd.isArray() &&
            2 == sd.size() &&
            sd[0].has("grid_x") &&
            sd[0].has("grid_y") &&
            sd[0]["grid_x"].isInteger() &&
            sd[0]["grid_y"].isInteger() &&
            sd[1].has("grid_x") &&
            sd[1].has("grid_y") &&
            sd[1]["grid_x"].isInteger() &&
            sd[1]["grid_y"].isInteger() &&
            ((grid_x1 == sd[0]["grid_x"].asInteger() &&
              grid_y1 == sd[0]["grid_y"].asInteger() &&
              grid_x2 == sd[1]["grid_x"].asInteger() &&
              grid_y2 == sd[1]["grid_y"].asInteger()) ||
             (grid_x1 == sd[1]["grid_x"].asInteger() &&
              grid_y1 == sd[1]["grid_y"].asInteger() &&
              grid_x2 == sd[0]["grid_x"].asInteger() &&
              grid_y2 == sd[0]["grid_y"].asInteger())));
}

static LLSD
get_region(const LLSD & sd, U64 region_handle1)
{
    U32 grid_x(0), grid_y(0);
    grid_from_region_handle(region_handle1, &grid_x, &grid_y);

    for (LLSD::array_const_iterator it(sd["regions"].beginArray());
         sd["regions"].endArray() != it;
         ++it)
    {
        if ((*it).has("grid_x") &&
            (*it).has("grid_y") &&
            (*it)["grid_x"].isInteger() &&
            (*it)["grid_y"].isInteger() &&
            (*it)["grid_x"].asInteger() == grid_x &&
            (*it)["grid_y"].asInteger() == grid_y)
        {
            return *it;
        }
    }
    return LLSD();
}

TEST_SUITE("tst_viewerassetstats_test") {

struct tst_viewerassetstats_index
{

        tst_viewerassetstats_index()
        {
            LLTrace::set_master_thread_recorder(&mThreadRecorder);
        
};

TEST_CASE_FIXTURE(tst_viewerassetstats_index, "test_1")
{

        // Check that helpers aren't bothered by missing global stats
        CHECK_MESSAGE((NULL == gViewerAssetStats, "Global gViewerAssetStats should be NULL"));

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_TEXTURE, false, false);

        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_TEXTURE, false, false);

        LLViewerAssetStatsFF::record_response(LLViewerAssetType::AT_GESTURE, false, false, (U64Microseconds)12300000ULL);
    
}

TEST_CASE_FIXTURE(tst_viewerassetstats_index, "test_2")
{

        CHECK_MESSAGE((NULL == gViewerAssetStats, "Global gViewerAssetStats should be NULL"));

        LLViewerAssetStats * it = new LLViewerAssetStats();

        CHECK_MESSAGE((NULL == gViewerAssetStats, "Global gViewerAssetStats should still be NULL"));

        LLSD sd_full = it->asLLSD(false);

        // Default (NULL) region ID doesn't produce LLSD results so should
        // get an empty map back from output
        CHECK_MESSAGE(is_no_stats_map(sd_full, "Stat-less LLSD initially"));

        // Once the region is set, we will get a response even with no data collection
        it->setRegion(region1_handle);
        sd_full = it->asLLSD(false);
        CHECK_MESSAGE(is_double_key_map(sd_full, "duration", "regions", "Correct single-key LLSD map root"));
        CHECK_MESSAGE(is_single_slot_array(sd_full["regions"], region1_handle, "Correct single-slot LLSD array regions"));

        LLSD sd = sd_full["regions"][0];

        delete it;

        // Check the structure of the LLSD
        for (int i = 0; i < LL_ARRAY_SIZE(all_keys); ++i)
        {
            std::string line = llformat("Has '%s' key", all_keys[i]);
            ensure(line, sd.has(all_keys[i]));
        
}

TEST_CASE_FIXTURE(tst_viewerassetstats_index, "test_3")
{

        LLViewerAssetStats * it = new LLViewerAssetStats();
        it->setRegion(region1_handle);

        LLSD sd = it->asLLSD(false);
        CHECK_MESSAGE(is_double_key_map(sd, "regions", "duration", "Correct single-key LLSD map root"));
        CHECK_MESSAGE(is_single_slot_array(sd["regions"], region1_handle, "Correct single-slot LLSD array regions"));
        sd = sd[0];

        delete it;

        // Check a few points on the tree for content
        CHECK_MESSAGE((0 == sd["get_texture_temp_http"]["dequeued"].asInteger(, "sd[get_texture_temp_http][dequeued] is 0")));
        CHECK_MESSAGE((0.0 == sd["get_sound_udp"]["resp_min"].asReal(, "sd[get_sound_udp][resp_min] is 0")));
    
}

TEST_CASE_FIXTURE(tst_viewerassetstats_index, "test_4")
{

        gViewerAssetStats = new LLViewerAssetStats();
        LLViewerAssetStatsFF::set_region(region1_handle);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_TEXTURE, false, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_TEXTURE, false, false);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_BODYPART, false, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_BODYPART, false, false);

        LLSD sd = gViewerAssetStats->asLLSD(false);
        CHECK_MESSAGE(is_double_key_map(sd, "regions", "duration", "Correct single-key LLSD map root"));
        CHECK_MESSAGE(is_single_slot_array(sd["regions"], region1_handle, "Correct single-slot LLSD array regions"));
        sd = sd["regions"][0];

        // Check a few points on the tree for content
        CHECK_MESSAGE((1 == sd["get_texture_non_temp_udp"]["enqueued"].asInteger(, "sd[get_texture_non_temp_udp][enqueued] is 1")));
        CHECK_MESSAGE((0 == sd["get_texture_temp_udp"]["enqueued"].asInteger(, "sd[get_texture_temp_udp][enqueued] is 0")));
        CHECK_MESSAGE((0 == sd["get_texture_non_temp_http"]["enqueued"].asInteger(, "sd[get_texture_non_temp_http][enqueued] is 0")));
        CHECK_MESSAGE((0 == sd["get_texture_temp_http"]["enqueued"].asInteger(, "sd[get_texture_temp_http][enqueued] is 0")));
        CHECK_MESSAGE((0 == sd["get_gesture_udp"]["dequeued"].asInteger(, "sd[get_gesture_udp][dequeued] is 0")));

        // Reset and check zeros...
        // Reset leaves current region in place
        gViewerAssetStats->reset();
        sd = gViewerAssetStats->asLLSD(false)["regions"][region1_handle_str];

        delete gViewerAssetStats;
        gViewerAssetStats = NULL;

        CHECK_MESSAGE((0 == sd["get_texture_non_temp_udp"]["enqueued"].asInteger(, "sd[get_texture_non_temp_udp][enqueued] is reset")));
        CHECK_MESSAGE((0 == sd["get_gesture_udp"]["dequeued"].asInteger(, "sd[get_gesture_udp][dequeued] is reset")));
    
}

TEST_CASE_FIXTURE(tst_viewerassetstats_index, "test_5")
{

        gViewerAssetStats = new LLViewerAssetStats();

        LLViewerAssetStatsFF::set_region(region1_handle);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_TEXTURE, false, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_TEXTURE, false, false);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_BODYPART, false, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_BODYPART, false, false);

        LLViewerAssetStatsFF::set_region(region2_handle);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);
        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);
        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);
        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);

        LLSD sd = gViewerAssetStats->asLLSD(false);

        // std::cout << sd << std::endl;

        CHECK_MESSAGE(is_double_key_map(sd, "duration", "regions", "Correct double-key LLSD map root"));
        CHECK_MESSAGE(is_double_slot_array(sd["regions"], region1_handle, region2_handle, "Correct double-slot LLSD array regions"));
        LLSD sd1 = get_region(sd, region1_handle);
        LLSD sd2 = get_region(sd, region2_handle);
        CHECK_MESSAGE(sd1.isMap(, "Region1 is present in results"));
        CHECK_MESSAGE(sd2.isMap(, "Region2 is present in results"));

        // Check a few points on the tree for content
        ensure_equals("sd1[get_texture_non_temp_udp][enqueued] is 1", sd1["get_texture_non_temp_udp"]["enqueued"].asInteger(), 1);
        ensure_equals("sd1[get_texture_temp_udp][enqueued] is 0", sd1["get_texture_temp_udp"]["enqueued"].asInteger(), 0);
        ensure_equals("sd1[get_texture_non_temp_http][enqueued] is 0", sd1["get_texture_non_temp_http"]["enqueued"].asInteger(), 0);
        ensure_equals("sd1[get_texture_temp_http][enqueued] is 0", sd1["get_texture_temp_http"]["enqueued"].asInteger(), 0);
        ensure_equals("sd1[get_gesture_udp][dequeued] is 0", sd1["get_gesture_udp"]["dequeued"].asInteger(), 0);

        // Check a few points on the tree for content
        CHECK_MESSAGE((4 == sd2["get_gesture_udp"]["enqueued"].asInteger(, "sd2[get_gesture_udp][enqueued] is 4")));
        CHECK_MESSAGE((0 == sd2["get_gesture_udp"]["dequeued"].asInteger(, "sd2[get_gesture_udp][dequeued] is 0")));
        CHECK_MESSAGE((0 == sd2["get_texture_non_temp_udp"]["enqueued"].asInteger(, "sd2[get_texture_non_temp_udp][enqueued] is 0")));

        // Reset and check zeros...
        // Reset leaves current region in place
        gViewerAssetStats->reset();
        sd = gViewerAssetStats->asLLSD(false);
        CHECK_MESSAGE(is_double_key_map(sd, "regions", "duration", "Correct single-key LLSD map root"));
        CHECK_MESSAGE(is_single_slot_array(sd["regions"], region2_handle, "Correct single-slot LLSD array regions (p2)"));
        sd2 = sd["regions"][0];

        delete gViewerAssetStats;
        gViewerAssetStats = NULL;

        CHECK_MESSAGE((0 == sd2["get_texture_non_temp_udp"]["enqueued"].asInteger(, "sd2[get_texture_non_temp_udp][enqueued] is reset")));
        CHECK_MESSAGE((0 == sd2["get_gesture_udp"]["enqueued"].asInteger(, "sd2[get_gesture_udp][enqueued] is reset")));
    
}

TEST_CASE_FIXTURE(tst_viewerassetstats_index, "test_6")
{

        gViewerAssetStats = new LLViewerAssetStats();

        LLViewerAssetStatsFF::set_region(region1_handle);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_TEXTURE, false, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_TEXTURE, false, false);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_BODYPART, false, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_BODYPART, false, false);

        LLViewerAssetStatsFF::set_region(region2_handle);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);
        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);
        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);
        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);

        LLViewerAssetStatsFF::set_region(region1_handle);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_TEXTURE, true, true);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_TEXTURE, true, true);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_BODYPART, false, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_BODYPART, false, false);

        LLViewerAssetStatsFF::set_region(region2_handle);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);
        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);
        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);
        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_GESTURE, false, false);

        LLSD sd = gViewerAssetStats->asLLSD(false);

        CHECK_MESSAGE(is_double_key_map(sd, "duration", "regions", "Correct double-key LLSD map root"));
        CHECK_MESSAGE(is_double_slot_array(sd["regions"], region1_handle, region2_handle, "Correct double-slot LLSD array regions"));
        LLSD sd1 = get_region(sd, region1_handle);
        LLSD sd2 = get_region(sd, region2_handle);
        CHECK_MESSAGE(sd1.isMap(, "Region1 is present in results"));
        CHECK_MESSAGE(sd2.isMap(, "Region2 is present in results"));

        // Check a few points on the tree for content
        CHECK_MESSAGE((1 == sd1["get_texture_non_temp_udp"]["enqueued"].asInteger(, "sd1[get_texture_non_temp_udp][enqueued] is 1")));
        CHECK_MESSAGE((0 == sd1["get_texture_temp_udp"]["enqueued"].asInteger(, "sd1[get_texture_temp_udp][enqueued] is 0")));
        CHECK_MESSAGE((0 == sd1["get_texture_non_temp_http"]["enqueued"].asInteger(, "sd1[get_texture_non_temp_http][enqueued] is 0")));
        CHECK_MESSAGE((1 == sd1["get_texture_temp_http"]["enqueued"].asInteger(, "sd1[get_texture_temp_http][enqueued] is 1")));
        CHECK_MESSAGE((0 == sd1["get_gesture_udp"]["dequeued"].asInteger(, "sd1[get_gesture_udp][dequeued] is 0")));

        // Check a few points on the tree for content
        CHECK_MESSAGE((8 == sd2["get_gesture_udp"]["enqueued"].asInteger(, "sd2[get_gesture_udp][enqueued] is 8")));
        CHECK_MESSAGE((0 == sd2["get_gesture_udp"]["dequeued"].asInteger(, "sd2[get_gesture_udp][dequeued] is 0")));
        CHECK_MESSAGE((0 == sd2["get_texture_non_temp_udp"]["enqueued"].asInteger(, "sd2[get_texture_non_temp_udp][enqueued] is 0")));

        // Reset and check zeros...
        // Reset leaves current region in place
        gViewerAssetStats->reset();
        sd = gViewerAssetStats->asLLSD(false);
        CHECK_MESSAGE(is_double_key_map(sd, "duration", "regions", "Correct single-key LLSD map root"));
        CHECK_MESSAGE(is_single_slot_array(sd["regions"], region2_handle, "Correct single-slot LLSD array regions (p2)"));
        sd2 = get_region(sd, region2_handle);
        CHECK_MESSAGE(sd2.isMap(, "Region2 is present in results"));

        delete gViewerAssetStats;
        gViewerAssetStats = NULL;

        ensure_equals("sd2[get_texture_non_temp_udp][enqueued] is reset", sd2["get_texture_non_temp_udp"]["enqueued"].asInteger(), 0);
        ensure_equals("sd2[get_gesture_udp][enqueued] is reset", sd2["get_gesture_udp"]["enqueued"].asInteger(), 0);
    
}

TEST_CASE_FIXTURE(tst_viewerassetstats_index, "test_7")
{

        gViewerAssetStats = new LLViewerAssetStats();
        LLViewerAssetStatsFF::set_region(region1_handle);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_TEXTURE, false, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_TEXTURE, false, false);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_BODYPART, false, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_BODYPART, false, false);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_BODYPART, false, true);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_BODYPART, false, true);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_BODYPART, true, false);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_BODYPART, true, false);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_BODYPART, true, true);
        LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_BODYPART, true, true);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_LSL_BYTECODE, false, false);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_LSL_BYTECODE, false, true);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_LSL_BYTECODE, true, false);

        LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_LSL_BYTECODE, true, true);

        LLSD sd = gViewerAssetStats->asLLSD(false);
        CHECK_MESSAGE(is_double_key_map(sd, "regions", "duration", "Correct single-key LLSD map root"));
        CHECK_MESSAGE(is_single_slot_array(sd["regions"], region1_handle, "Correct single-slot LLSD array regions"));
        sd = get_region(sd, region1_handle);
        CHECK_MESSAGE(sd.isMap(, "Region1 is present in results"));

        // Check a few points on the tree for content
        CHECK_MESSAGE((0 == sd["get_gesture_udp"]["enqueued"].asInteger(, "sd[get_gesture_udp][enqueued] is 0")));
        CHECK_MESSAGE((0 == sd["get_gesture_udp"]["dequeued"].asInteger(, "sd[get_gesture_udp][dequeued] is 0")));

        CHECK_MESSAGE((2 == sd["get_wearable_http"]["enqueued"].asInteger(, "sd[get_wearable_http][enqueued] is 2")));
        CHECK_MESSAGE((2 == sd["get_wearable_http"]["dequeued"].asInteger(, "sd[get_wearable_http][dequeued] is 2")));

        CHECK_MESSAGE((2 == sd["get_wearable_udp"]["enqueued"].asInteger(, "sd[get_wearable_udp][enqueued] is 2")));
        CHECK_MESSAGE((2 == sd["get_wearable_udp"]["dequeued"].asInteger(, "sd[get_wearable_udp][dequeued] is 2")));

        CHECK_MESSAGE((2 == sd["get_other_http"]["enqueued"].asInteger(, "sd[get_other_http][enqueued] is 2")));
        CHECK_MESSAGE((0 == sd["get_other_http"]["dequeued"].asInteger(, "sd[get_other_http][dequeued] is 0")));

        CHECK_MESSAGE((2 == sd["get_other_udp"]["enqueued"].asInteger(, "sd[get_other_udp][enqueued] is 2")));
        CHECK_MESSAGE((0 == sd["get_other_udp"]["dequeued"].asInteger(, "sd[get_other_udp][dequeued] is 0")));

        // Reset and check zeros...
        // Reset leaves current region in place
        gViewerAssetStats->reset();
        sd = get_region(gViewerAssetStats->asLLSD(false), region1_handle);
        CHECK_MESSAGE(sd.isMap(, "Region1 is present in results"));

        delete gViewerAssetStats;
        gViewerAssetStats = NULL;

        ensure_equals("sd[get_texture_non_temp_udp][enqueued] is reset", sd["get_texture_non_temp_udp"]["enqueued"].asInteger(), 0);
        ensure_equals("sd[get_gesture_udp][dequeued] is reset", sd["get_gesture_udp"]["dequeued"].asInteger(), 0);
    
}

} // TEST_SUITE

