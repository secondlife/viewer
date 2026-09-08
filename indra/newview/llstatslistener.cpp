/**
 * @file llstatslistener.cpp
 * @brief EventAPI interface for querying performance statistics
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#include "llstatslistener.h"

#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llimagegl.h"
#include "llviewerstats.h"

namespace
{
template <typename STAT, typename EXTRACTOR>
LLSD collectPeriodArray(LLTrace::PeriodicRecording& recording,
                        size_t num_periods,
                        const STAT& stat,
                        EXTRACTOR extractor)
{
    LLSD values = LLSD::emptyArray();
    for (size_t i = 1; i <= num_periods; ++i)
    {
        LLTrace::Recording& period = recording.getPrevRecording(i);
        if (period.hasValue(stat))
        {
            values.append((F64)extractor(period, stat));
        }
    }
    return values;
}

template <typename STAT, typename EXTRACTOR>
void setPeriodArray(LLSD& out,
                    const char* key,
                    LLTrace::Recording& last,
                    LLTrace::PeriodicRecording& recording,
                    size_t num_periods,
                    const STAT& stat,
                    EXTRACTOR extractor)
{
    if (last.hasValue(stat))
    {
        out[key] = collectPeriodArray(recording, num_periods, stat, extractor);
    }
}
}

LLStatsListener::LLStatsListener()
    : LLEventAPI("LLStats", "Query performance statistics")
{
    add("getPerfData",
        "Get performance data from the frame recording buffer, plus texture memory\n"
        "and inventory loading timing information.\n"
        "Reply contains [\"stats\"] with nested group maps.",
        &LLStatsListener::getPerfData,
        llsd::map("reply", LLSD()));
}

void LLStatsListener::getPerfData(LLSD const & evt)
{
    LLEventAPI::Response response(LLSD(), evt);

    // get_frame_recording() is a PeriodicRecording with 200 periods
    LLTrace::PeriodicRecording& recording = LLTrace::get_frame_recording();
    LLTrace::Recording& last = recording.getLastRecording();

    size_t num_periods = recording.getNumRecordedPeriods();
    F64 total_duration = recording.getDuration().value();

    LLSD stats;
    stats["total_periods_duration"] = total_duration;
    stats["num_periods"] = (LLSD::Integer)num_periods;

    LLSD frametime;

    setPeriodArray(frametime, "fps", last, recording, num_periods, LLStatViewer::FPS,
        [](LLTrace::Recording& period, const auto& stat) { return period.getPerSec(stat); });

    setPeriodArray(frametime, "frame_time_ms", last, recording, num_periods, LLStatViewer::FRAMETIME,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });

    setPeriodArray(frametime, "do_frame_time_us", last, recording, num_periods, LLStatViewer::DOFRAME_TIME_US,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });

    setPeriodArray(frametime, "frame_time_jitter_ms", last, recording, num_periods, LLStatViewer::FRAMETIME_JITTER,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });

    // Normalized jitter - scalar samples updated per frame
    // report the current (last) value since they are session/period rolling stats
    if (last.hasValue(LLStatViewer::NOTRMALIZED_FRAMETIME_JITTER_SESSION))
    {
        frametime["normalized_jitter_session"] = last.getLastValue(LLStatViewer::NOTRMALIZED_FRAMETIME_JITTER_SESSION);
    }
    if (last.hasValue(LLStatViewer::NORMALIZED_FRAMTIME_JITTER_PERIOD))
    {
        frametime["normalized_jitter_period"] = last.getLastValue(LLStatViewer::NORMALIZED_FRAMTIME_JITTER_PERIOD);
    }
    if (last.hasValue(LLStatViewer::NFTV))
    {
        frametime["normalized_frametime_variation"] = last.getLastValue(LLStatViewer::NFTV);
    }

    // Jitter event minute counters: running avg per minute and count in last completed minute
    // These are already minute-window aggregates, so sending single value, not arrays
    if (last.hasValue(LLStatViewer::FRAMETIME_JITTER_EVENTS_PER_MINUTE))
    {
        frametime["frame_time_jitter_events_per_minute"] = (F64)last.getLastValue(LLStatViewer::FRAMETIME_JITTER_EVENTS_PER_MINUTE);
    }
    if (last.hasValue(LLStatViewer::FRAMETIME_JITTER_EVENTS_LAST_MINUTE))
    {
        frametime["frame_time_jitter_events_last_minute"] = (F64)last.getLastValue(LLStatViewer::FRAMETIME_JITTER_EVENTS_LAST_MINUTE);
    }

    // Jitter percentiles / cumulative
    setPeriodArray(frametime, "jitter_cumulative_ms", last, recording, num_periods, LLStatViewer::FRAMETIME_JITTER_CUMULATIVE,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });
    setPeriodArray(frametime, "jitter_99th_ms", last, recording, num_periods, LLStatViewer::FRAMETIME_JITTER_99TH,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });
    setPeriodArray(frametime, "jitter_95th_ms", last, recording, num_periods, LLStatViewer::FRAMETIME_JITTER_95TH,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });
    setPeriodArray(frametime, "jitter_stddev_ms", last, recording, num_periods, LLStatViewer::FRAMETIME_JITTER_STDDEV,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });

    // Frame time percentiles
    setPeriodArray(frametime, "frametime_99th_ms", last, recording, num_periods, LLStatViewer::FRAMETIME_99TH,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });
    setPeriodArray(frametime, "frametime_95th_ms", last, recording, num_periods, LLStatViewer::FRAMETIME_95TH,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });
    setPeriodArray(frametime, "frametime_stddev_ms", last, recording, num_periods, LLStatViewer::FRAMETIME_STDDEV,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });

    // Frametime jitter event count
    setPeriodArray(frametime, "frametime_jitter_events", last, recording, num_periods, LLStatViewer::FRAMETIME_JITTER_EVENTS,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });

    stats["frametime"] = frametime;

    LLSD other;

    // Packet loss
    setPeriodArray(other, "packet_loss_percent", last, recording, num_periods, LLStatViewer::PACKETS_LOST_PERCENT,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });

    // Sim ping
    setPeriodArray(other, "sim_ping_ms", last, recording, num_periods, LLStatViewer::SIM_PING,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });

    // Triangle rendering
    setPeriodArray(other, "ktris_per_frame", last, recording, num_periods, LLStatViewer::TRIANGLES_DRAWN,
        [](LLTrace::Recording& period, const auto& stat) { return period.getSum(stat); });
    setPeriodArray(other, "ktris_per_sec", last, recording, num_periods, LLStatViewer::TRIANGLES_DRAWN,
        [](LLTrace::Recording& period, const auto& stat) { return period.getPerSec(stat); });

    // Object counts
    setPeriodArray(other, "num_objects", last, recording, num_periods, LLStatViewer::NUM_OBJECTS,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });
    setPeriodArray(other, "num_active_objects", last, recording, num_periods, LLStatViewer::NUM_ACTIVE_OBJECTS,
        [](LLTrace::Recording& period, const auto& stat) { return period.getMean(stat); });

    stats["other"] = other;

    // texture memory usage
    LLSD memory;
    memory["texture_bytes_alloc_mb"] = (F64)LLImageGL::getTextureBytesAllocated() / 1024.0 / 512.0;
    stats["memory"] = memory;

    LLSD inventory;
    // library skeleton cache load duration
    inventory["skeleton_load_time_library_seconds"] = gInventory.getLibrarySkeletonLoadTime();
    // agent skeleton cache load duration
    inventory["skeleton_load_time_agent_seconds"] = gInventory.getAgentSkeletonLoadTime();
    // initial recursive inventory/library background fetch duration
    inventory["initial_fetch_time_seconds"] = LLInventoryModelBackgroundFetch::instance().getInitialFetchDuration();
    inventory["fetch_completed"] = LLInventoryModelBackgroundFetch::instance().isEverythingFetched();
    stats["inventory"] = inventory;

    response["stats"] = stats;
}
