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

#include "llviewerprecompiledheaders.h"

#include "llviewerassetstats.h"

#include "stdtypes.h"

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

	mTotalTime = 0;
	mStartTimestamp = LLViewerAssetStatsFF::get_timestamp();
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
{
	reset();
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
		mCurRegionStats = new PerRegionStats(mRegionID);
	}

	// And add reference to map
	mRegionStats[mRegionID] = mCurRegionStats;

	// Start timestamp consistent with per-region collector
	mResetTimestamp = mCurRegionStats->mStartTimestamp;
}


void
LLViewerAssetStats::setRegionID(const LLUUID & region_id)
{
	if (region_id == mRegionID)
	{
		// Already active, ignore.
		return;
	}

	// Get duration for current set
	const duration_t now = LLViewerAssetStatsFF::get_timestamp();
	mCurRegionStats->accumulateTime(now);

	// Prepare new set
	PerRegionContainer::iterator new_stats = mRegionStats.find(region_id);
	if (mRegionStats.end() == new_stats)
	{
		// Haven't seen this region_id before, create a new block and make it current.
		mCurRegionStats = new PerRegionStats(region_id);
		mRegionStats[region_id] = mCurRegionStats;
	}
	else
	{
		mCurRegionStats = new_stats->second;
	}
	mCurRegionStats->mStartTimestamp = now;
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
LLViewerAssetStats::recordGetServiced(LLViewerAssetType::EType at, bool with_http, bool is_temp, duration_t duration)
{
	const EViewerAssetCategories eac(asset_type_to_category(at, with_http, is_temp));

	mCurRegionStats->mRequests[int(eac)].mResponse.record(duration);
}

LLSD
LLViewerAssetStats::asLLSD()
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

	// Sub-tags.  If you add or delete from this list, mergeRegionsLLSD() must be updated.
	static const LLSD::String enq_tag("enqueued");
	static const LLSD::String deq_tag("dequeued");
	static const LLSD::String rcnt_tag("resp_count");
	static const LLSD::String rmin_tag("resp_min");
	static const LLSD::String rmax_tag("resp_max");
	static const LLSD::String rmean_tag("resp_mean");

	const duration_t now = LLViewerAssetStatsFF::get_timestamp();
	LLSD regions = LLSD::emptyMap();

	for (PerRegionContainer::iterator it = mRegionStats.begin();
		 mRegionStats.end() != it;
		 ++it)
	{
		if (it->first.isNull())
		{
			// Never emit NULL UUID in results.
			continue;
		}

		PerRegionStats & stats = *it->second;
		stats.accumulateTime(now);
		
		LLSD reg_stat = LLSD::emptyMap();
		
		for (int i = 0; i < EVACCount; ++i)
		{
			LLSD & slot = reg_stat[tags[i]];
			slot = LLSD::emptyMap();
			slot[enq_tag] = LLSD(S32(stats.mRequests[i].mEnqueued.getCount()));
			slot[deq_tag] = LLSD(S32(stats.mRequests[i].mDequeued.getCount()));
			slot[rcnt_tag] = LLSD(S32(stats.mRequests[i].mResponse.getCount()));
			slot[rmin_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMin()));
			slot[rmax_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMax()));
			slot[rmean_tag] = LLSD(F64(stats.mRequests[i].mResponse.getMean()));
		}

		reg_stat["duration"] = LLSD::Integer(stats.mTotalTime / 1000000);
		
		regions[it->first.asString()] = reg_stat;
	}

	LLSD ret = LLSD::emptyMap();
	ret["regions"] = regions;
	ret["duration"] = LLSD::Integer((now - mResetTimestamp) / 1000000);
	
	return ret;
}

/* static */ void
LLViewerAssetStats::mergeRegionsLLSD(const LLSD & src, LLSD & dst)
{
	// Merge operator definitions
	static const int MOP_ADD_INT(0);
	static const int MOP_MIN_REAL(1);
	static const int MOP_MAX_REAL(2);
	static const int MOP_MEAN_REAL(3);	// Requires a 'mMergeOpArg' to weight the input terms

	static const LLSD::String regions_key("regions");
	
	static const struct
		{
			LLSD::String		mName;
			int					mMergeOp;
			LLSD::String		mMergeOpArg;
		}
	key_list[] =
		{
			// Order is important below.  We modify the data in-place and
			// so operations like MOP_MEAN_REAL which need the "resp_count"
			// value for weighting must be performed before "resp_count"
			// is modified or the weight will be wrong.  Key list is
			// defined in asLLSD() and must track it.

			{ "resp_mean", MOP_MEAN_REAL, "resp_count" },
			{ "enqueued", MOP_ADD_INT, "" },
			{ "dequeued", MOP_ADD_INT, "" },
			{ "resp_count", MOP_ADD_INT, "" },
			{ "resp_min", MOP_MIN_REAL, "" },
			{ "resp_max", MOP_MAX_REAL, "" }
		};

	// Trivial checks
	if (! src.has(regions_key))
	{
		return;
	}

	if (! dst.has(regions_key))
	{
		dst[regions_key] = src[regions_key];
		return;
	}
	
	// Non-trivial cases requiring a deep merge.
	const LLSD & root_src(src[regions_key]);
	LLSD & root_dst(dst[regions_key]);
	
	const LLSD::map_const_iterator it_uuid_end(root_src.endMap());
	for (LLSD::map_const_iterator it_uuid(root_src.beginMap()); it_uuid_end != it_uuid; ++it_uuid)
	{
		if (! root_dst.has(it_uuid->first))
		{
			// src[<region>] without matching dst[<region>]
			root_dst[it_uuid->first] = it_uuid->second;
		}
		else
		{
			// src[<region>] with matching dst[<region>]
			// We have matching source and destination regions.
			// Now iterate over each asset bin in the region status.  Could iterate over
			// an explicit list but this will do as well.
			LLSD & reg_dst(root_dst[it_uuid->first]);
			const LLSD & reg_src(root_src[it_uuid->first]);

			const LLSD::map_const_iterator it_sets_end(reg_src.endMap());
			for (LLSD::map_const_iterator it_sets(reg_src.beginMap()); it_sets_end != it_sets; ++it_sets)
			{
				static const LLSD::String no_touch_1("duration");

				if (no_touch_1 == it_sets->first)
				{
					continue;
				}
				else if (! reg_dst.has(it_sets->first))
				{
					// src[<region>][<asset>] without matching dst[<region>][<asset>]
					reg_dst[it_sets->first] = it_sets->second;
				}
				else
				{
					// src[<region>][<asset>] with matching dst[<region>][<asset>]
					// Matching stats bin in both source and destination regions.
					// Iterate over those bin keys we know how to merge, leave the remainder untouched.
					LLSD & bin_dst(reg_dst[it_sets->first]);
					const LLSD & bin_src(reg_src[it_sets->first]);

					for (int key_index(0); key_index < LL_ARRAY_SIZE(key_list); ++key_index)
					{
						const LLSD::String & key_name(key_list[key_index].mName);
						
						if (! bin_src.has(key_name))
						{
							// Missing src[<region>][<asset>][<field>]
							continue;
						}

						const LLSD & src_value(bin_src[key_name]);
				
						if (! bin_dst.has(key_name))
						{
							// src[<region>][<asset>][<field>] without matching dst[<region>][<asset>][<field>]
							bin_dst[key_name] = src_value;
						}
						else
						{
							// src[<region>][<asset>][<field>] with matching dst[<region>][<asset>][<field>]
							LLSD & dst_value(bin_dst[key_name]);
					
							switch (key_list[key_index].mMergeOp)
							{
							case MOP_ADD_INT:
								// Simple counts, just add
								dst_value = dst_value.asInteger() + src_value.asInteger();
						
								break;
						
							case MOP_MIN_REAL:
								// Minimum
								dst_value = llmin(dst_value.asReal(), src_value.asReal());
								break;

							case MOP_MAX_REAL:
								// Maximum
								dst_value = llmax(dst_value.asReal(), src_value.asReal());
								break;

							case MOP_MEAN_REAL:
							    {
									// Mean
									const LLSD::String & weight_key(key_list[key_index].mMergeOpArg);
									F64 src_weight(bin_src[weight_key].asReal());
									F64 dst_weight(bin_dst[weight_key].asReal());
									F64 tot_weight(src_weight + dst_weight);
									if (tot_weight >= F64(0.5))
									{
										dst_value = (((dst_value.asReal() * dst_weight)
													  + (src_value.asReal() * src_weight))
													 / tot_weight);
									}
								}
								break;
						
							default:
								break;
							}
						}
					}
				}
			}
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
record_response_main(LLViewerAssetType::EType at, bool with_http, bool is_temp, LLViewerAssetStats::duration_t duration)
{
	if (! gViewerAssetStatsMain)
		return;

	gViewerAssetStatsMain->recordGetServiced(at, with_http, is_temp, duration);
}


// 'thread1' - should be for TextureFetch thread

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


void
merge_stats(const LLSD & src, LLSD & dst)
{
	static const LLSD::String regions_key("regions");

	// Trivial cases first
	if (! src.isMap())
	{
		return;
	}

	if (! dst.isMap())
	{
		dst = src;
		return;
	}
	
	// Okay, both src and dst are maps at this point.
	// Collector class know how to merge the regions part.
	LLViewerAssetStats::mergeRegionsLLSD(src, dst);

	// Now merge non-regions bits manually.
	const LLSD::map_const_iterator it_end(src.endMap());
	for (LLSD::map_const_iterator it(src.beginMap()); it_end != it; ++it)
	{
		if (regions_key == it->first)
			continue;

		if (dst.has(it->first))
			continue;

		dst[it->first] = it->second;
	}
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
