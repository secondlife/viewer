/** 
 * @file llviewerassetstats.h
 * @brief Client-side collection of asset request statistics
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

#ifndef LL_LLVIEWERASSETSTATUS_H
#define	LL_LLVIEWERASSETSTATUS_H


#include "linden_common.h"

#include "llpointer.h"
#include "llrefcount.h"
#include "llviewerassettype.h"
#include "llviewerassetstorage.h"
#include "llsimplestat.h"
#include "llsd.h"
#include "llvoavatar.h"

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
	 * Type for the region identifier used in stats.  Currently uses
	 * the region handle's type (a U64) rather than the regions's LLUUID
	 * as the latter isn't available immediately.
	 */
	typedef U64 region_handle_t;

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
		PerRegionStats(const region_handle_t region_handle)
			: LLRefCount(),
			  mRegionHandle(region_handle)
			{
				reset();
			}

		PerRegionStats(const PerRegionStats & src)
			: LLRefCount(),
			  mRegionHandle(src.mRegionHandle),
			  mTotalTime(src.mTotalTime),
			  mStartTimestamp(src.mStartTimestamp),
			  mFPS(src.mFPS)
			{
				for (int i = 0; i < LL_ARRAY_SIZE(mRequests); ++i)
				{
					mRequests[i] = src.mRequests[i];
				}
			}

		// Default assignment and destructor are correct.
		
		void reset();

		void merge(const PerRegionStats & src);
		
		// Apply current running time to total and reset start point.
		// Return current timestamp as a convenience.
		void accumulateTime(duration_t now);
		
	public:
		region_handle_t		mRegionHandle;
		duration_t			mTotalTime;
		duration_t			mStartTimestamp;
		LLSimpleStatMMM<>	mFPS;
		
		struct prs_group
		{
			LLSimpleStatCounter			mEnqueued;
			LLSimpleStatCounter			mDequeued;
			LLSimpleStatMMM<duration_t>	mResponse;
		}
		mRequests [EVACCount];
	};

public:
	LLViewerAssetStats();
	LLViewerAssetStats(const LLViewerAssetStats &);
	// Default destructor is correct.
	LLViewerAssetStats & operator=(const LLViewerAssetStats &);			// Not defined

	// Clear all metrics data.  This leaves the currently-active region
	// in place but with zero'd data for all metrics.  All other regions
	// are removed from the collection map.
	void reset();

	// Set hidden region argument and establish context for subsequent
	// collection calls.
	void setRegion(region_handle_t region_handle);

	// Asset GET Requests
	void recordGetEnqueued(LLViewerAssetType::EType at, bool with_http, bool is_temp);
	void recordGetDequeued(LLViewerAssetType::EType at, bool with_http, bool is_temp);
	void recordGetServiced(LLViewerAssetType::EType at, bool with_http, bool is_temp, duration_t duration);

	// Frames-Per-Second Samples
	void recordFPS(F32 fps);

	// Avatar-related statistics
	void recordAvatarStats();

	// Merge a source instance into a destination instance.  This is
	// conceptually an 'operator+=()' method:
	// - counts are added
	// - minimums are min'd
	// - maximums are max'd
	// - other scalars are ignored ('this' wins)
	//
	void merge(const LLViewerAssetStats & src);
	
	// Retrieve current metrics for all visited regions (NULL region UUID/handle excluded)
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
	// &mmm_group = {
	//   count : int,
	//   min   : float,
	//   max   : float,
	//   mean  : float
	// }
	//
	// {
	//   duration: int
	//   regions: {
	//     $: {			// Keys are strings of the region's handle in hex
	//       duration:                 : int,
	//		 fps:					   : &mmm_group,
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
	//
	// @param	compact_output		If true, omits from conversion any mmm_block
	//								or stats_block that would contain all zero data.
	//								Useful for transmission when the receiver knows
	//								what is expected and will assume zero for missing
	//								blocks.
	LLSD asLLSD(bool compact_output);

protected:
	typedef std::map<region_handle_t, LLPointer<PerRegionStats> > PerRegionContainer;

	// Region of the currently-active region.  Always valid but may
	// be zero after construction or when explicitly set.  Unchanged
	// by a reset() call.
	region_handle_t mRegionHandle;

	// Pointer to metrics collection for currently-active region.  Always
	// valid and unchanged after reset() though contents will be changed.
	// Always points to a collection contained in mRegionStats.
	LLPointer<PerRegionStats> mCurRegionStats;

	// Metrics data for all regions during one collection cycle
	PerRegionContainer mRegionStats;

	// Time of last reset
	duration_t mResetTimestamp;

	// Nearby avatar stats
	std::vector<S32> mAvatarRezStates;
	LLViewerStats::phase_stats_t mPhaseStats;
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
void init();

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
void set_region_main(LLViewerAssetStats::region_handle_t region_handle);

void record_enqueue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_dequeue_main(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_response_main(LLViewerAssetType::EType at, bool with_http, bool is_temp,
						  LLViewerAssetStats::duration_t duration);

void record_fps_main(F32 fps);

void record_avatar_stats();

/**
 * Region context, event and duration loggers for Thread 1.
 */
void set_region_thread1(LLViewerAssetStats::region_handle_t region_handle);

void record_enqueue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_dequeue_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_response_thread1(LLViewerAssetType::EType at, bool with_http, bool is_temp,
						  LLViewerAssetStats::duration_t duration);

} // namespace LLViewerAssetStatsFF

#endif // LL_LLVIEWERASSETSTATUS_H
