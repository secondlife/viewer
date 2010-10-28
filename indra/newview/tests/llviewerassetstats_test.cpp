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

static const char * all_keys[] = 
{
	"get_other",
	"get_texture",
	"get_wearable",
	"get_sound",
	"get_gesture"
};

static const char * resp_keys[] = 
{
	"get_other",
	"get_texture",
	"get_wearable",
	"get_sound",
	"get_gesture"
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
		ensure("Global gViewerAssetStats should be NULL", (NULL == gViewerAssetStats));

		LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_TEXTURE);

		LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_TEXTURE);

		LLViewerAssetStatsFF::record_response(LLViewerAssetType::AT_GESTURE, 12.3);
	}

	// Create a non-global instance and check the structure
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<2>()
	{
		ensure("Global gViewerAssetStats should be NULL", (NULL == gViewerAssetStats));

		LLViewerAssetStats * it = new LLViewerAssetStats();

		ensure("Global gViewerAssetStats should still be NULL", (NULL == gViewerAssetStats));
		
		LLSD sd = it->asLLSD();
		
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
		
		LLSD sd = it->asLLSD();
		
		delete it;

		// Check a few points on the tree for content
		ensure("sd[get_texture][dequeued] is 0", (0 == sd["get_texture"]["dequeued"].asInteger()));
		ensure("sd[get_sound][resp_min] is 0", (0.0 == sd["get_sound"]["resp_min"].asReal()));
	}

	// Create a global instance and verify free functions do something useful
	template<> template<>
	void tst_viewerassetstats_index_object_t::test<4>()
	{
		gViewerAssetStats = new LLViewerAssetStats();

		LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_TEXTURE);
		LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_TEXTURE);

		LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_BODYPART);
		LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_BODYPART);

		LLSD sd = gViewerAssetStats->asLLSD();
		
		// Check a few points on the tree for content
		ensure("sd[get_texture][enqueued] is 1", (1 == sd["get_texture"]["enqueued"].asInteger()));
		ensure("sd[get_gesture][dequeued] is 0", (0 == sd["get_gesture"]["dequeued"].asInteger()));

		// Reset and check zeros...
		gViewerAssetStats->reset();
		sd = gViewerAssetStats->asLLSD();
		
		delete gViewerAssetStats;
		gViewerAssetStats = NULL;

		ensure("sd[get_texture][enqueued] is reset", (0 == sd["get_texture"]["enqueued"].asInteger()));
		ensure("sd[get_gesture][dequeued] is reset", (0 == sd["get_gesture"]["dequeued"].asInteger()));
	}

}
