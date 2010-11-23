/** 
 * @file llviewerassetstats.h
 * @brief Client-side collection of asset request statistics
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

#ifndef LL_LLVIEWERASSETSTATUS_H
#define	LL_LLVIEWERASSETSTATUS_H


#include "linden_common.h"

#include "llpointer.h"
#include "llrefcount.h"
#include "llviewerassettype.h"
#include "llviewerassetstorage.h"
#include "llsimplestat.h"
#include "llsd.h"

/**
 * @class LLViewerAssetStats
 * @brief Records performance aspects of asset access operations.
 *
 * This facility is derived from a very similar simulator-based
 * one, LLSimAssetStats.  It's function is to count asset access
 * operations and characterize response times.  Collected data
 * are binned in several dimensions:
 *
 *  - Asset types collapsed into a few aggregated categories
 *  - By simulator UUID
 *  - By transport mechanism (HTTP vs MessageSystem)
 *  - By persistence (temp vs non-temp)
 *
 * Statistics collected are fairly basic at this point:
 *
 *  - Counts of enqueue and dequeue operations
 *  - Min/Max/Mean of asset transfer operations
 *
 * This collector differs from the simulator-based on in a
 * number of ways:
 *
 *  - The front-end/back-end distinction doesn't exist in viewer
 *    code
 *  - Multiple threads must be safely accomodated in the viewer
 *
 * Access to results is by conversion to an LLSD with some standardized
 * key names.  The intent of this structure is that it be emitted as
 * standard syslog-based metrics formatting where it can be picked
 * up by interested parties.
 *
 * For convenience, a set of free functions in namespace
 * LLViewerAssetStatsFF is provided for conditional test-and-call
 * operations.
 */
class LLViewerAssetStats
{
public:
	enum EViewerAssetCategories
	{
		EVACTextureTempHTTPGet,			//< Texture GETs - temp/baked, HTTP
		EVACTextureTempUDPGet,			//< Texture GETs - temp/baked, UDP
		EVACTextureNonTempHTTPGet,		//< Texture GETs - perm, HTTP
		EVACTextureNonTempUDPGet,		//< Texture GETs - perm, UDP
		EVACWearableUDPGet,				//< Wearable GETs
		EVACSoundUDPGet,				//< Sound GETs
		EVACGestureUDPGet,				//< Gesture GETs
		EVACOtherGet,					//< Other GETs
		
		EVACCount						// Must be last
	};

	/**
	 * Type for duration and other time values in the metrics.  Selected
	 * for compatibility with the pre-existing timestamp on the texture
	 * fetcher class, LLTextureFetch.
	 */
	typedef U64 duration_t;
	
	/**
	 * @brief Collected data for a single region visited by the avatar.
	 *
	 * Fairly simple, for each asset bin enumerated above a count
	 * of enqueue and dequeue operations and simple stats on response
	 * times for completed requests.
	 */
	class PerRegionStats : public LLRefCount
	{
	public:
		PerRegionStats(const LLUUID & region_id)
			: LLRefCount(),
			  mRegionID(region_id)
			{
				reset();
			}
		// Default assignment and destructor are correct.
		
		void reset();

		// Apply current running time to total and reset start point.
		// Return current timestamp as a convenience.
		void accumulateTime(duration_t now);
		
	public:
		LLUUID mRegionID;
		duration_t mTotalTime;
		duration_t mStartTimestamp;
		
		struct
		{
			LLSimpleStatCounter			mEnqueued;
			LLSimpleStatCounter			mDequeued;
			LLSimpleStatMMM<duration_t>	mResponse;
		} mRequests [EVACCount];
	};

public:
	LLViewerAssetStats(bool qa_mode);
	// Default destructor is correct.
	LLViewerAssetStats & operator=(const LLViewerAssetStats &);			// Not defined

	// Clear all metrics data.  This leaves the currently-active region
	// in place but with zero'd data for all metrics.  All other regions
	// are removed from the collection map.
	void reset();

	// Set hidden region argument and establish context for subsequent
	// collection calls.
	void setRegionID(const LLUUID & region_id);

	// Asset GET Requests
	void recordGetEnqueued(LLViewerAssetType::EType at, bool with_http, bool is_temp);
	void recordGetDequeued(LLViewerAssetType::EType at, bool with_http, bool is_temp);
	void recordGetServiced(LLViewerAssetType::EType at, bool with_http, bool is_temp, duration_t duration);

	// Retrieve current metrics for all visited regions (NULL region UUID excluded)
    // Returned LLSD is structured as follows:
	//
	// &stats_group = {
	//   enqueued   : int,
	//   dequeued   : int,
	//   resp_count : int,
	//   resp_min   : float,
	//   resp_max   : float,
	//   resp_mean  : float
	// }
	//
	// {
	//   duration: int
	//   regions: {
	//     $: {
	//       duration:                 : int,
	//       get_texture_temp_http     : &stats_group,
	//       get_texture_temp_udp      : &stats_group,
	//       get_texture_non_temp_http : &stats_group,
	//       get_texture_non_temp_udp  : &stats_group,
	//       get_wearable_udp          : &stats_group,
	//       get_sound_udp             : &stats_group,
	//       get_gesture_udp           : &stats_group,
	//       get_other                 : &stats_group
	//     }
	//   }
	// }
	LLSD asLLSD();

	// Merges the "regions" maps in two LLSDs structured as per asLLSD().
	// This takes two LLSDs as returned by asLLSD() and intelligently
	// merges the metrics contained in the maps indexed by "regions".
	// The remainder of the top-level map of the LLSDs is left unchanged
	// in expectation that callers will add other information at this
	// level.  The "regions" information must be correctly formed or the
	// final result is undefined (little defensive action).
	static void mergeRegionsLLSD(const LLSD & src, LLSD & dst);

	// QA mode is established during initialization so we don't
	// touch LLSD at runtime.
	bool isQAMode() const { return mQAMode; }
	
protected:
	typedef std::map<LLUUID, LLPointer<PerRegionStats> > PerRegionContainer;

	// Region of the currently-active region.  Always valid but may
	// be a NULL UUID after construction or when explicitly set.  Unchanged
	// by a reset() call.
	LLUUID mRegionID;

	// Pointer to metrics collection for currently-active region.  Always
	// valid and unchanged after reset() though contents will be changed.
	// Always points to a collection contained in mRegionStats.
	LLPointer<PerRegionStats> mCurRegionStats;

	// Metrics data for all regions during one collection cycle
	PerRegionContainer mRegionStats;

	// Time of last reset
	duration_t mResetTimestamp;

	// QA Mode
	const bool mQAMode;
};


/**
 * Global stats collectors one for each independent thread where
 * assets and other statistics are gathered.  The globals are
 * expected to be created at startup time and then picked up by
 * their respective threads afterwards.  A set of free functions
 * are provided to access methods behind the globals while both
 * minimally disrupting visual flow and supplying a description
 * of intent.
 *
 * Expected thread assignments:
 *
 *  - Main:  main() program execution thread
 *  - Thread1:  TextureFetch worker thread
 */
extern LLViewerAssetStats * gViewerAssetStatsMain;

extern LLViewerAssetStats * gViewerAssetStatsThread1;

namespace LLViewerAssetStatsFF
{
/**
 * @brief Allocation and deallocation of globals.
 *
 * init() should be called before threads are started that will access it though
 * you'll likely get away with calling it afterwards.  cleanup() should only be
 * called after threads are shutdown to prevent races on the global pointers.
 */
void init(bool qa_mode);

void cleanup();

/**
 * We have many timers, clocks etc. in the runtime.  This is the
 * canonical timestamp for these metrics which is compatible with
 * the pre-existing timestamping in the texture fetcher.
 */
inline LLViewerAssetStats::duration_t get_timestamp()
{
	return LLTimer::getTotalTime();
}

/**
 * Region context, event and duration loggers for the Main thread.
 */
void set_region_main(const LLUUID & region_id);

void record_enqueue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_dequeue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_response_main(LLViewerAssetType::EType at, bool with_http, bool is_temp,
						  LLViewerAssetStats::duration_t duration);


/**
 * Region context, event and duration loggers for Thread 1.
 */
void set_region_thread1(const LLUUID & region_id);

void record_enqueue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_dequeue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_response_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp,
						  LLViewerAssetStats::duration_t duration);

/**
 * @brief Merge two LLSD reports from different collector instances
 *
 * Use this to merge the LLSD's from two threads.  For top-level,
 * non-region data the destination (dst) is considered authoritative
 * if the key is present in both source and destination.  For
 * regions, a numerical merge is performed when data are present in
 * both source and destination and the 'right thing' is done for
 * counts, minimums, maximums and averages.
 */
void merge_stats(const LLSD & src, LLSD & dst);

} // namespace LLViewerAssetStatsFF

#endif // LL_LLVIEWERASSETSTATUS_H
