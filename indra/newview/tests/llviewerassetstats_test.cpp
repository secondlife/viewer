/** 
 * @file llviewerassetstats_tut.cpp
 * @date 2010-10-28
 * @brief Test cases for some of newview/llviewerassetstats.cpp
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "linden_common.h"

#include <tut/tut.hpp>
#include <iostream>

#include "lltut.h"
#include "../llviewerassetstats.h"
#include "lluuid.h"

static const char * all_keys[] = 
{
	"get_other",
	"get_texture_temp_http",
	"get_texture_temp_udp",
	"get_texture_non_temp_http",
	"get_texture_non_temp_udp",
	"get_wearable_udp",
	"get_sound_udp",
	"get_gesture_udp"
};

static const char * resp_keys[] = 
{
	"get_other",
	"get_texture_temp_http",
	"get_texture_temp_udp",
	"get_texture_non_temp_http",
	"get_texture_non_temp_udp",
	"get_wearable_udp",
	"get_sound_udp",
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

static const LLUUID region1("4e2d81a3-6263-6ffe-ad5c-8ce04bee07e8");
static const LLUUID region2("68762cc8-b68b-4e45-854b-e830734f2d4a");

static bool
is_empty_map(const LLSD & sd)
{
	return sd.isMap() && 0 == sd.size();
}

static bool
is_single_key_map(const LLSD & sd, const std::string & key)
{
	return sd.isMap() && 1 == sd.size() && sd.has(key);
}

static bool
is_double_key_map(const LLSD & sd, const std::string & key1, const std::string & key2)
{
	return sd.isMap() && 2 == sd.size() && sd.has(key1) && sd.has(key2);
}

namespace tut
{
	struct tst_viewerassetstats_index
	{};
	typedef test_group<tst_viewerassetstats_index> tst_viewerassetstats_index_t;
	typedef tst_viewerassetstats_index_t::object tst_viewerassetstats_index_object_t;
	tut::tst_viewerassetstats_index_t tut_tst_viewerassetstats_index("tst_viewerassetstats_test");

	// Testing free functions without global stats allocated
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<1>()
	{
		// Check that helpers aren't bothered by missing global stats
		ensure("Global gViewerAssetStatsMain should be NULL", (NULL == gViewerAssetStatsMain));

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_TEXTURE, false, false);

		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_TEXTURE, false, false);

		LLViewerAssetStatsFF::record_response_main(LLViewerAssetType::AT_GESTURE, false, false, 12.3);
	}

	// Create a non-global instance and check the structure
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<2>()
	{
		ensure("Global gViewerAssetStatsMain should be NULL", (NULL == gViewerAssetStatsMain));

		LLViewerAssetStats * it = new LLViewerAssetStats();

		ensure("Global gViewerAssetStatsMain should still be NULL", (NULL == gViewerAssetStatsMain));

		LLSD sd_full = it->asLLSD();

		// Default (NULL) region ID doesn't produce LLSD results so should
		// get an empty map back from output
		ensure("Null LLSD initially", is_empty_map(sd_full));

		// Once the region is set, we will get a response even with no data collection
		it->setRegionID(region1);
		sd_full = it->asLLSD();
		ensure("Correct single-key LLSD map", is_single_key_map(sd_full, region1.asString()));

		LLSD sd = sd_full[region1.asString()];

		delete it;
			
		// Check the structure of the LLSD
		for (int i = 0; i < LL_ARRAY_SIZE(all_keys); ++i)
		{
			std::string line = llformat("Has '%s' key", all_keys[i]);
			ensure(line, sd.has(all_keys[i]));
		}

		for (int i = 0; i < LL_ARRAY_SIZE(resp_keys); ++i)
		{
			for (int j = 0; j < LL_ARRAY_SIZE(sub_keys); ++j)
			{
				std::string line = llformat("Key '%s' has '%s' key", resp_keys[i], sub_keys[j]);
				ensure(line, sd[resp_keys[i]].has(sub_keys[j]));
			}
		}
	}

	// Create a non-global instance and check some content
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<3>()
	{
		LLViewerAssetStats * it = new LLViewerAssetStats();
		it->setRegionID(region1);
		
		LLSD sd = it->asLLSD();
		ensure("Correct single-key LLSD map", is_single_key_map(sd, region1.asString()));
		sd = sd[region1.asString()];
		
		delete it;

		// Check a few points on the tree for content
		ensure("sd[get_texture_temp_http][dequeued] is 0", (0 == sd["get_texture_temp_http"]["dequeued"].asInteger()));
		ensure("sd[get_sound_udp][resp_min] is 0", (0.0 == sd["get_sound_udp"]["resp_min"].asReal()));
	}

	// Create a global instance and verify free functions do something useful
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<4>()
	{
		gViewerAssetStatsMain = new LLViewerAssetStats();
		LLViewerAssetStatsFF::set_region_main(region1);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_TEXTURE, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_TEXTURE, false, false);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_BODYPART, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_BODYPART, false, false);

		LLSD sd = gViewerAssetStatsMain->asLLSD();
		ensure("Correct single-key LLSD map", is_single_key_map(sd, region1.asString()));
		sd = sd[region1.asString()];
		
		// Check a few points on the tree for content
		ensure("sd[get_texture_non_temp_udp][enqueued] is 1", (1 == sd["get_texture_non_temp_udp"]["enqueued"].asInteger()));
		ensure("sd[get_texture_temp_udp][enqueued] is 0", (0 == sd["get_texture_temp_udp"]["enqueued"].asInteger()));
		ensure("sd[get_texture_non_temp_http][enqueued] is 0", (0 == sd["get_texture_non_temp_http"]["enqueued"].asInteger()));
		ensure("sd[get_texture_temp_http][enqueued] is 0", (0 == sd["get_texture_temp_http"]["enqueued"].asInteger()));
		ensure("sd[get_gesture_udp][dequeued] is 0", (0 == sd["get_gesture_udp"]["dequeued"].asInteger()));

		// Reset and check zeros...
		// Reset leaves current region in place
		gViewerAssetStatsMain->reset();
		sd = gViewerAssetStatsMain->asLLSD()[region1.asString()];
		
		delete gViewerAssetStatsMain;
		gViewerAssetStatsMain = NULL;

		ensure("sd[get_texture_non_temp_udp][enqueued] is reset", (0 == sd["get_texture_non_temp_udp"]["enqueued"].asInteger()));
		ensure("sd[get_gesture_udp][dequeued] is reset", (0 == sd["get_gesture_udp"]["dequeued"].asInteger()));
	}

	// Create two global instances and verify no interactions
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<5>()
	{
		gViewerAssetStatsThread1 = new LLViewerAssetStats();
		gViewerAssetStatsMain = new LLViewerAssetStats();
		LLViewerAssetStatsFF::set_region_main(region1);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_TEXTURE, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_TEXTURE, false, false);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_BODYPART, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_BODYPART, false, false);

		LLSD sd = gViewerAssetStatsThread1->asLLSD();
		ensure("Other collector is empty", is_empty_map(sd));
		sd = gViewerAssetStatsMain->asLLSD();
		ensure("Correct single-key LLSD map", is_single_key_map(sd, region1.asString()));
		sd = sd[region1.asString()];
		
		// Check a few points on the tree for content
		ensure("sd[get_texture_non_temp_udp][enqueued] is 1", (1 == sd["get_texture_non_temp_udp"]["enqueued"].asInteger()));
		ensure("sd[get_texture_temp_udp][enqueued] is 0", (0 == sd["get_texture_temp_udp"]["enqueued"].asInteger()));
		ensure("sd[get_texture_non_temp_http][enqueued] is 0", (0 == sd["get_texture_non_temp_http"]["enqueued"].asInteger()));
		ensure("sd[get_texture_temp_http][enqueued] is 0", (0 == sd["get_texture_temp_http"]["enqueued"].asInteger()));
		ensure("sd[get_gesture_udp][dequeued] is 0", (0 == sd["get_gesture_udp"]["dequeued"].asInteger()));

		// Reset and check zeros...
		// Reset leaves current region in place
		gViewerAssetStatsMain->reset();
		sd = gViewerAssetStatsMain->asLLSD()[region1.asString()];
		
		delete gViewerAssetStatsMain;
		gViewerAssetStatsMain = NULL;
		delete gViewerAssetStatsThread1;
		gViewerAssetStatsThread1 = NULL;

		ensure("sd[get_texture_non_temp_udp][enqueued] is reset", (0 == sd["get_texture_non_temp_udp"]["enqueued"].asInteger()));
		ensure("sd[get_gesture_udp][dequeued] is reset", (0 == sd["get_gesture_udp"]["dequeued"].asInteger()));
	}

    // Check multiple region collection
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<6>()
	{
		gViewerAssetStatsMain = new LLViewerAssetStats();

		LLViewerAssetStatsFF::set_region_main(region1);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_TEXTURE, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_TEXTURE, false, false);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_BODYPART, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_BODYPART, false, false);

		LLViewerAssetStatsFF::set_region_main(region2);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);
		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);
		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);
		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);

		LLSD sd = gViewerAssetStatsMain->asLLSD();

		ensure("Correct double-key LLSD map", is_double_key_map(sd, region1.asString(), region2.asString()));
		LLSD sd1 = sd[region1.asString()];
		LLSD sd2 = sd[region2.asString()];
		
		// Check a few points on the tree for content
		ensure("sd1[get_texture_non_temp_udp][enqueued] is 1", (1 == sd1["get_texture_non_temp_udp"]["enqueued"].asInteger()));
		ensure("sd1[get_texture_temp_udp][enqueued] is 0", (0 == sd1["get_texture_temp_udp"]["enqueued"].asInteger()));
		ensure("sd1[get_texture_non_temp_http][enqueued] is 0", (0 == sd1["get_texture_non_temp_http"]["enqueued"].asInteger()));
		ensure("sd1[get_texture_temp_http][enqueued] is 0", (0 == sd1["get_texture_temp_http"]["enqueued"].asInteger()));
		ensure("sd1[get_gesture_udp][dequeued] is 0", (0 == sd1["get_gesture_udp"]["dequeued"].asInteger()));

		// Check a few points on the tree for content
		ensure("sd2[get_gesture_udp][enqueued] is 4", (4 == sd2["get_gesture_udp"]["enqueued"].asInteger()));
		ensure("sd2[get_gesture_udp][dequeued] is 0", (0 == sd2["get_gesture_udp"]["dequeued"].asInteger()));
		ensure("sd2[get_texture_non_temp_udp][enqueued] is 0", (0 == sd2["get_texture_non_temp_udp"]["enqueued"].asInteger()));

		// Reset and check zeros...
		// Reset leaves current region in place
		gViewerAssetStatsMain->reset();
		sd = gViewerAssetStatsMain->asLLSD();
		ensure("Correct single-key LLSD map", is_single_key_map(sd, region2.asString()));
		sd2 = sd[region2.asString()];
		
		delete gViewerAssetStatsMain;
		gViewerAssetStatsMain = NULL;

		ensure("sd2[get_texture_non_temp_udp][enqueued] is reset", (0 == sd2["get_texture_non_temp_udp"]["enqueued"].asInteger()));
		ensure("sd2[get_gesture_udp][enqueued] is reset", (0 == sd2["get_gesture_udp"]["enqueued"].asInteger()));
	}

    // Check multiple region collection jumping back-and-forth between regions
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<7>()
	{
		gViewerAssetStatsMain = new LLViewerAssetStats();

		LLViewerAssetStatsFF::set_region_main(region1);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_TEXTURE, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_TEXTURE, false, false);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_BODYPART, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_BODYPART, false, false);

		LLViewerAssetStatsFF::set_region_main(region2);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);
		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);
		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);
		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);

		LLViewerAssetStatsFF::set_region_main(region1);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_TEXTURE, true, true);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_TEXTURE, true, true);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_BODYPART, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_BODYPART, false, false);

		LLViewerAssetStatsFF::set_region_main(region2);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);
		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);
		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);
		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_GESTURE, false, false);

		LLSD sd = gViewerAssetStatsMain->asLLSD();

		ensure("Correct double-key LLSD map", is_double_key_map(sd, region1.asString(), region2.asString()));
		LLSD sd1 = sd[region1.asString()];
		LLSD sd2 = sd[region2.asString()];
		
		// Check a few points on the tree for content
		ensure("sd1[get_texture_non_temp_udp][enqueued] is 1", (1 == sd1["get_texture_non_temp_udp"]["enqueued"].asInteger()));
		ensure("sd1[get_texture_temp_udp][enqueued] is 0", (0 == sd1["get_texture_temp_udp"]["enqueued"].asInteger()));
		ensure("sd1[get_texture_non_temp_http][enqueued] is 0", (0 == sd1["get_texture_non_temp_http"]["enqueued"].asInteger()));
		ensure("sd1[get_texture_temp_http][enqueued] is 1", (1 == sd1["get_texture_temp_http"]["enqueued"].asInteger()));
		ensure("sd1[get_gesture_udp][dequeued] is 0", (0 == sd1["get_gesture_udp"]["dequeued"].asInteger()));

		// Check a few points on the tree for content
		ensure("sd2[get_gesture_udp][enqueued] is 8", (8 == sd2["get_gesture_udp"]["enqueued"].asInteger()));
		ensure("sd2[get_gesture_udp][dequeued] is 0", (0 == sd2["get_gesture_udp"]["dequeued"].asInteger()));
		ensure("sd2[get_texture_non_temp_udp][enqueued] is 0", (0 == sd2["get_texture_non_temp_udp"]["enqueued"].asInteger()));

		// Reset and check zeros...
		// Reset leaves current region in place
		gViewerAssetStatsMain->reset();
		sd = gViewerAssetStatsMain->asLLSD();
		ensure("Correct single-key LLSD map", is_single_key_map(sd, region2.asString()));
		sd2 = sd[region2.asString()];
		
		delete gViewerAssetStatsMain;
		gViewerAssetStatsMain = NULL;

		ensure("sd2[get_texture_non_temp_udp][enqueued] is reset", (0 == sd2["get_texture_non_temp_udp"]["enqueued"].asInteger()));
		ensure("sd2[get_gesture_udp][enqueued] is reset", (0 == sd2["get_gesture_udp"]["enqueued"].asInteger()));
	}

	// Non-texture assets ignore transport and persistence flags
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<8>()
	{
		gViewerAssetStatsThread1 = new LLViewerAssetStats();
		gViewerAssetStatsMain = new LLViewerAssetStats();
		LLViewerAssetStatsFF::set_region_main(region1);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_TEXTURE, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_TEXTURE, false, false);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_BODYPART, false, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_BODYPART, false, false);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_BODYPART, false, true);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_BODYPART, false, true);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_BODYPART, true, false);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_BODYPART, true, false);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_BODYPART, true, true);
		LLViewerAssetStatsFF::record_dequeue_main(LLViewerAssetType::AT_BODYPART, true, true);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_LSL_BYTECODE, false, false);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_LSL_BYTECODE, false, true);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_LSL_BYTECODE, true, false);

		LLViewerAssetStatsFF::record_enqueue_main(LLViewerAssetType::AT_LSL_BYTECODE, true, true);

		LLSD sd = gViewerAssetStatsThread1->asLLSD();
		ensure("Other collector is empty", is_empty_map(sd));
		sd = gViewerAssetStatsMain->asLLSD();
		ensure("Correct single-key LLSD map", is_single_key_map(sd, region1.asString()));
		sd = sd[region1.asString()];
		
		// Check a few points on the tree for content
		ensure("sd[get_gesture_udp][enqueued] is 0", (0 == sd["get_gesture_udp"]["enqueued"].asInteger()));
		ensure("sd[get_gesture_udp][dequeued] is 0", (0 == sd["get_gesture_udp"]["dequeued"].asInteger()));

		ensure("sd[get_wearable_udp][enqueued] is 4", (4 == sd["get_wearable_udp"]["enqueued"].asInteger()));
		ensure("sd[get_wearable_udp][dequeued] is 4", (4 == sd["get_wearable_udp"]["dequeued"].asInteger()));

		ensure("sd[get_other][enqueued] is 4", (4 == sd["get_other"]["enqueued"].asInteger()));
		ensure("sd[get_other][dequeued] is 0", (0 == sd["get_other"]["dequeued"].asInteger()));

		// Reset and check zeros...
		// Reset leaves current region in place
		gViewerAssetStatsMain->reset();
		sd = gViewerAssetStatsMain->asLLSD()[region1.asString()];
		
		delete gViewerAssetStatsMain;
		gViewerAssetStatsMain = NULL;
		delete gViewerAssetStatsThread1;
		gViewerAssetStatsThread1 = NULL;

		ensure("sd[get_texture_non_temp_udp][enqueued] is reset", (0 == sd["get_texture_non_temp_udp"]["enqueued"].asInteger()));
		ensure("sd[get_gesture_udp][dequeued] is reset", (0 == sd["get_gesture_udp"]["dequeued"].asInteger()));
	}

}
