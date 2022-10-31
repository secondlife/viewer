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
#define LL_LLVIEWERASSETSTATUS_H


#include "linden_common.h"

#include "llpointer.h"
#include "llrefcount.h"
#include "llviewerassettype.h"
#include "llviewerassetstorage.h"
#include "llsd.h"
#include "llvoavatar.h"
#include "lltrace.h"
#include "llinitparam.h"

namespace LLViewerAssetStatsFF
{
    enum EViewerAssetCategories
    {
        EVACTextureTempHTTPGet,         //< Texture GETs - temp/baked, HTTP
        EVACTextureTempUDPGet,          //< Texture GETs - temp/baked, UDP
        EVACTextureNonTempHTTPGet,      //< Texture GETs - perm, HTTP
        EVACTextureNonTempUDPGet,       //< Texture GETs - perm, UDP
        EVACWearableHTTPGet,            //< Wearable GETs HTTP
        EVACWearableUDPGet,             //< Wearable GETs UDP
        EVACSoundHTTPGet,               //< Sound GETs HTTP
        EVACSoundUDPGet,                //< Sound GETs UDP
        EVACGestureHTTPGet,             //< Gesture GETs HTTP
        EVACGestureUDPGet,              //< Gesture GETs UDP
        EVACLandmarkHTTPGet,            //< Landmark GETs HTTP
        EVACLandmarkUDPGet,             //< Landmark GETs UDP
        EVACOtherHTTPGet,               //< Other GETs HTTP
        EVACOtherUDPGet,                //< Other GETs UDP

        EVACCount                       // Must be last
    };
}

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

class LLViewerAssetStats : public LLStopWatchControlsMixin<LLViewerAssetStats>
{
public:
    /**
     * Type for duration and other time values in the metrics.  Selected
     * for compatibility with the pre-existing timestamp on the texture
     * fetcher class, LLTextureFetch.
     */
    typedef U64Microseconds duration_t;

    /**
     * Type for the region identifier used in stats.  Currently uses
     * the region handle's type (a U64) rather than the regions's LLUUID
     * as the latter isn't available immediately.
     */
    typedef U64 region_handle_t;

    struct AssetRequestType : public LLInitParam::Block<AssetRequestType>
    {
        Mandatory<S32>  enqueued,
                        dequeued,
                        resp_count;
        Mandatory<F64>  resp_min,
                        resp_max,
                        resp_mean,
                        resp_mean_bytes;
    
        AssetRequestType();
    };

    struct FPSStats : public LLInitParam::Block<FPSStats>
    {
        Mandatory<S32>  count;
        Mandatory<F64>  min,
                        max,
                        mean;
        FPSStats();
    };

    struct RegionStats : public LLInitParam::Block<RegionStats>
    {
        Optional<AssetRequestType>  get_texture_temp_http,
                                    get_texture_temp_udp,
                                    get_texture_non_temp_http,
                                    get_texture_non_temp_udp,
                                    get_wearable_http,
                                    get_wearable_udp,
                                    get_sound_http,
                                    get_sound_udp,
                                    get_gesture_http,
                                    get_gesture_udp,
                                    get_landmark_http,
                                    get_landmark_udp,
                                    get_other_http,
                                    get_other_udp;
        Optional<FPSStats>          fps;
        Optional<S32>               grid_x,
                                    grid_y;
        Optional<F64>               duration;

        RegionStats();
    };
        
    struct AssetStats : public LLInitParam::Block<AssetStats>
    {
        Multiple<RegionStats>   regions;
        Mandatory<F64>          duration;
        
        Mandatory<LLUUID>       session_id,
                                agent_id;
        
        Mandatory<std::string>  message;
        Mandatory<S32>          sequence;
        Mandatory<bool>         initial,
                                break_;
        
        AssetStats();
    };

public:
    LLViewerAssetStats();
    LLViewerAssetStats(const LLViewerAssetStats &);

    // Default destructor is correct.
    LLViewerAssetStats & operator=(const LLViewerAssetStats &);         // Not defined

    // Clear all metrics data.  This leaves the currently-active region
    // in place but with zero'd data for all metrics.  All other regions
    // are removed from the collection map.
    void reset();

    // Set hidden region argument and establish context for subsequent
    // collection calls.
    void setRegion(region_handle_t region_handle);

    // Retrieve current metrics for all visited regions (NULL region UUID/handle excluded)
    // Uses AssetStats structure seen above
    void getStats(AssetStats& stats, bool compact_output);

    // Retrieve a single asset request type (taken from a single region)
    template <typename T>
    void getStat(LLTrace::Recording& rec, T& req, LLViewerAssetStatsFF::EViewerAssetCategories cat, bool compact_output);

    LLSD asLLSD(bool compact_output);

protected:
    void handleStart();
    void handleStop();
    void handleReset();

    typedef std::map<region_handle_t, LLTrace::Recording > PerRegionRecordingContainer;

    // Region of the currently-active region.  Always valid but may
    // be zero after construction or when explicitly set.  Unchanged
    // by a reset() call.
    region_handle_t mRegionHandle;

    // Pointer to metrics collection for currently-active region.  
    LLTrace::Recording*         mCurRecording;

    // Metrics data for all regions during one collection cycle
    PerRegionRecordingContainer mRegionRecordings;
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
extern LLViewerAssetStats * gViewerAssetStats;

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
void set_region(LLViewerAssetStats::region_handle_t region_handle);

void record_enqueue(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_dequeue(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_response(LLViewerAssetType::EType at, bool with_http, bool is_temp,
                     LLViewerAssetStats::duration_t duration, F64 bytes=0);

void record_avatar_stats();

} // namespace LLViewerAssetStatsFF

#endif // LL_LLVIEWERASSETSTATUS_H
