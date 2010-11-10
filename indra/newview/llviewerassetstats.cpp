/** 
 * @file llviewerassetstats.cpp
 * @brief 
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

#include "llviewerassetstats.h"

#include "stdtypes.h"

/*
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
}


// ------------------------------------------------------
// LLViewerAssetStats class definition
// ------------------------------------------------------
LLViewerAssetStats::LLViewerAssetStats()
{
	reset();
}


void
LLViewerAssetStats::reset()
{
	// Empty the map of all region stats
	mRegionStats.clear();

	// If we have a current stats, reset it, otherwise, as at construction,
	// create a new one.
	if (mCurRegionStats)
	{
		mCurRegionStats->reset();
	}
	else
	{
		mCurRegionStats = new PerRegionStats(mRegionID);
	}

	// And add reference to map
	mRegionStats[mRegionID] = mCurRegionStats;
}


void
LLViewerAssetStats::setRegionID(const LLUUID & region_id)
{
	if (region_id == mRegionID)
	{
		// Already active, ignore.
		return;
	}
	
	PerRegionContainer::iterator new_stats = mRegionStats.find(region_id);
	if (mRegionStats.end() == new_stats)
	{
		// Haven't seen this region_id before, create a new block make it current.
		mCurRegionStats = new PerRegionStats(region_id);
		mRegionStats[region_id] = mCurRegionStats;
	}
	else
	{
		mCurRegionStats = new_stats->second;
	}
	mRegionID = region_id;
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
LLViewerAssetStats::recordGetServiced(LLViewerAssetType::EType at, bool with_http, bool is_temp, F64 duration)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	mCurRegionStats->mRequests[int(eac)].mResponse.record(duration);
}

const LLSD
LLViewerAssetStats::asLLSD() const
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

	// Sub-tags
	static const LLSD::String enq_tag("enqueued");
	static const LLSD::String deq_tag("dequeued");
	static const LLSD::String rcnt_tag("resp_count");
	static const LLSD::String rmin_tag("resp_min");
	static const LLSD::String rmax_tag("resp_max");
	static const LLSD::String rmean_tag("resp_mean");
	
	LLSD ret = LLSD::emptyMap();

	for (PerRegionContainer::const_iterator it = mRegionStats.begin();
		 mRegionStats.end() != it;
		 ++it)
	{
		if (it->first.isNull())
		{
			// Never emit NULL UUID in results.
			continue;
		}

		const PerRegionStats & stats = *it->second;
		
		LLSD reg_stat = LLSD::emptyMap();
		
		for (int i = 0; i < EVACCount; ++i)
		{
			LLSD & slot = reg_stat[tags[i]];
			slot = LLSD::emptyMap();
			slot[enq_tag] = LLSD(S32(stats.mRequests[i].mEnqueued.getCount()));
			slot[deq_tag] = LLSD(S32(stats.mRequests[i].mDequeued.getCount()));
			slot[rcnt_tag] = LLSD(S32(stats.mRequests[i].mResponse.getCount()));
			slot[rmin_tag] = LLSD(stats.mRequests[i].mResponse.getMin());
			slot[rmax_tag] = LLSD(stats.mRequests[i].mResponse.getMax());
			slot[rmean_tag] = LLSD(stats.mRequests[i].mResponse.getMean());
		}

		ret[it->first.asString()] = reg_stat;
	}

	return ret;
}

// ------------------------------------------------------
// Global free-function definitions (LLViewerAssetStatsFF namespace)
// ------------------------------------------------------

namespace LLViewerAssetStatsFF
{

// Target thread is elaborated in the function name.  This could
// have been something 'templatey' like specializations iterated
// over a set of constants but with so few, this is clearer I think.

void
set_region_main(const LLUUID & region_id)
{
	if (! gViewerAssetStatsMain)
		return;

	gViewerAssetStatsMain->setRegionID(region_id);
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
record_response_main(LLViewerAssetType::EType at, bool with_http, bool is_temp, F64 duration)
{
	if (! gViewerAssetStatsMain)
		return;

	gViewerAssetStatsMain->recordGetServiced(at, with_http, is_temp, duration);
}


void
set_region_thread1(const LLUUID & region_id)
{
	if (! gViewerAssetStatsThread1)
		return;

	gViewerAssetStatsThread1->setRegionID(region_id);
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
record_response_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp, F64 duration)
{
	if (! gViewerAssetStatsThread1)
		return;

	gViewerAssetStatsThread1->recordGetServiced(at, with_http, is_temp, duration);
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
	llassert_always(26 == LLViewerAssetType::AT_COUNT);

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
#if 0
			// When LLViewerAssetType::AT_COUNT == 49
			LLViewerAssetStats::EVACOtherGet,					// AT_FOLDER_ENSEMBLE_START
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
			LLViewerAssetStats::EVACOtherGet,					// AT_FOLDER_ENSEMBLE_END
			LLViewerAssetStats::EVACOtherGet,					// AT_CURRENT_OUTFIT
			LLViewerAssetStats::EVACOtherGet,					// AT_OUTFIT
			LLViewerAssetStats::EVACOtherGet					// AT_MY_OUTFITS
#endif
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
