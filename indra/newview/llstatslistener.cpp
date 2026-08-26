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

#if defined(LL_RENDER_BENCHMARK)
#include "llappviewer.h"
#include "llgl.h"
#include "llpanel.h"
#include "llrender.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewershadermgr.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "pipeline.h"

#include <thread>
#endif

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

#if defined(LL_RENDER_BENCHMARK)
template <typename STAT>
void setFrameSample(LLSD& frame,
                    const char* key,
                    LLTrace::Recording& period,
                    const STAT& stat)
{
    if (period.hasValue(stat))
    {
        frame[key] = (F64)period.getLastValue(stat);
    }
}

void setFrameBlockTime(LLSD& frame,
                       const char* key,
                       LLTrace::Recording& period,
                       const LLTrace::BlockTimerStatHandle& stat)
{
    if (period.hasValue(stat))
    {
        frame[key] = F64Milliseconds(period.getSum(stat)).value();
    }
}

LLSD collectRendererFrames(LLTrace::PeriodicRecording& recording, size_t num_periods)
{
    LLSD frames = LLSD::emptyArray();

    // Emit oldest-to-newest so cumulative resource counters can be differenced
    // directly. FRAME_NUMBER lets polling clients de-duplicate the rolling
    // recording window without relying on array position.
    for (size_t offset = num_periods; offset > 0; --offset)
    {
        LLTrace::Recording& period = recording.getPrevRecording(offset);
        if (!period.hasValue(LLStatViewer::FRAME_NUMBER))
        {
            continue;
        }

        LLSD frame;
        setFrameSample(frame, "frame_number", period, LLStatViewer::FRAME_NUMBER);
        setFrameSample(frame, "frame_time_ms", period, LLStatViewer::FRAMETIME);
        setFrameSample(frame, "do_frame_time_us", period, LLStatViewer::DOFRAME_TIME_US);
        setFrameSample(frame, "sim_ping_ms", period, LLStatViewer::SIM_PING);

        setFrameBlockTime(frame, "geometry_create_ms", period, LLStatViewer::RENDER_GEOMETRY_CREATE);
        setFrameBlockTime(frame, "partition_ms", period, LLStatViewer::RENDER_PARTITION);
        setFrameBlockTime(frame, "geometry_update_ms", period, LLStatViewer::RENDER_GEOMETRY_UPDATE);
        setFrameBlockTime(frame, "cull_ms", period, LLStatViewer::RENDER_CULL);
        setFrameBlockTime(frame, "shadows_ms", period, LLStatViewer::RENDER_SHADOWS);
        setFrameBlockTime(frame, "texture_work_ms", period, LLStatViewer::RENDER_TEXTURE_WORK);
        setFrameBlockTime(frame, "state_sort_ms", period, LLStatViewer::RENDER_STATE_SORT);
        setFrameBlockTime(frame, "rebuild_ms", period, LLStatViewer::RENDER_REBUILD);
        setFrameBlockTime(frame, "submission_ms", period, LLStatViewer::RENDER_SUBMISSION);
        setFrameBlockTime(frame, "lighting_ms", period, LLStatViewer::RENDER_LIGHTING);
        setFrameBlockTime(frame, "ui_ms", period, LLStatViewer::RENDER_UI);
        setFrameBlockTime(frame, "swap_ms", period, LLStatViewer::RENDER_SWAP);
        setFrameBlockTime(frame, "idle_ms", period, LLStatViewer::RENDER_IDLE);

        if (period.hasValue(LLPipeline::sStatBatchSize))
        {
            frame["draw_calls"] = (LLSD::Integer)period.getSampleCount(LLPipeline::sStatBatchSize);
            frame["batch_size_min"] = (F64)period.getMin(LLPipeline::sStatBatchSize);
            frame["batch_size_max"] = (F64)period.getMax(LLPipeline::sStatBatchSize);
            frame["batch_size_mean"] = (F64)period.getMean(LLPipeline::sStatBatchSize);
        }
        if (period.hasValue(LLStatViewer::TRIANGLES_DRAWN))
        {
            frame["ktriangles"] = (F64)period.getSum(LLStatViewer::TRIANGLES_DRAWN);
        }

        setFrameSample(frame, "texture_upload_count_total", period, LLStatViewer::TEXTURE_UPLOAD_COUNT);
        setFrameSample(frame, "texture_upload_bytes_total", period, LLStatViewer::TEXTURE_UPLOAD_BYTES);
        setFrameSample(frame, "texture_readback_count_total", period, LLStatViewer::TEXTURE_READBACK_COUNT);
        setFrameSample(frame, "texture_readback_time_us_total", period, LLStatViewer::TEXTURE_READBACK_TIME_US);
        setFrameSample(frame, "texture_wait_count_total", period, LLStatViewer::TEXTURE_WAIT_COUNT);
        setFrameSample(frame, "texture_wait_time_us_total", period, LLStatViewer::TEXTURE_WAIT_TIME_US);
        setFrameSample(frame, "shader_compile_count_total", period, LLStatViewer::SHADER_COMPILE_COUNT);
        setFrameSample(frame, "shader_compile_time_us_total", period, LLStatViewer::SHADER_COMPILE_TIME_US);
        setFrameSample(frame, "shader_bind_count_total", period, LLStatViewer::SHADER_BIND_COUNT);

        frames.append(frame);
    }
    return frames;
}

LLSD getRendererContext()
{
    LLSD viewer_info = LLAppViewer::instance()->getViewerInfo();
    LLSD context;

    context["viewer_version"] = viewer_info["VIEWER_VERSION_STR"];
    context["viewer_channel"] = viewer_info["CHANNEL"];
    context["build_type"] = viewer_info.has("BUILD_CONFIG") ? viewer_info["BUILD_CONFIG"] : LLSD("Release");
    context["os"] = viewer_info["OS_VERSION"];
    context["cpu"] = viewer_info["CPU"];
    context["logical_core_count"] = (LLSD::Integer)std::thread::hardware_concurrency();
    context["gpu_vendor"] = viewer_info["GRAPHICS_CARD_VENDOR"];
    context["gpu"] = viewer_info["GRAPHICS_CARD"];
    context["driver"] = gGLManager.mDriverVersionVendorString;
    const std::string opengl_version = viewer_info["OPENGL_VERSION"].asString();
    context["opengl_version"] = opengl_version;
    if (opengl_version.find("Core Profile") != std::string::npos)
    {
        context["opengl_profile"] = "core";
    }
    else if (opengl_version.find("Compatibility Profile") != std::string::npos)
    {
        context["opengl_profile"] = "compatibility";
    }
    else
    {
        context["opengl_profile"] = LLRender::sGLCoreProfile ? "core" : "compatibility";
    }
    LLCoordWindow backing_size;
    LLCoordWindow logical_size;
    F32 backing_scale_x = 0.f;
    F32 backing_scale_y = 0.f;
    LLWindow* window = gViewerWindow ? gViewerWindow->getWindow() : nullptr;
    if (window)
    {
        window->getSize(&backing_size);
        window->getNativeContentSize(&logical_size);
        window->getBackingScale(backing_scale_x, backing_scale_y);
    }
    context["width"] = backing_size.mX;
    context["height"] = backing_size.mY;
    context["backing_width"] = backing_size.mX;
    context["backing_height"] = backing_size.mY;
    context["logical_width"] = logical_size.mX;
    context["logical_height"] = logical_size.mY;
    context["backing_scale_x"] = backing_scale_x;
    context["backing_scale_y"] = backing_scale_y;
    context["configured_ui_scale"] = gSavedSettings.getF32("UIScaleFactor");
    const LLVector2 display_scale = gViewerWindow ? gViewerWindow->getDisplayScale() : LLVector2::zero;
    context["effective_display_scale_x"] = display_scale.mV[VX];
    context["effective_display_scale_y"] = display_scale.mV[VY];
    context["gpu_vram_mb"] = (LLSD::Integer)gGLManager.mVRAM;
    context["shader_level"] = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_DEFERRED);

    LLSD limits;
    limits["max_texture_size"] = gGLManager.mGLMaxTextureSize;
    limits["max_texture_image_units"] = gGLManager.mNumTextureImageUnits;
    limits["max_samples"] = gGLManager.mMaxSamples;
    limits["max_uniform_block_size"] = gGLManager.mMaxUniformBlockSize;
    context["gl_limits"] = limits;

    LLSD extensions = LLSD::emptyArray();
    for (const std::string& extension : gGLManager.mGLExtensions)
    {
        extensions.append(extension);
    }
    context["gl_extensions"] = extensions;

    std::string renderer = context["gpu"].asString();
    LLStringUtil::toLower(renderer);
    context["detected_backend"] = renderer.find("zink") == std::string::npos ? "native-gl" : "zink";

    LLSD settings;
    settings["AutoTuneFPS"] = gSavedSettings.getBOOL("AutoTuneFPS");
    settings["RenderAvatarMaxNonImpostors"] = (LLSD::Integer)gSavedSettings.getU32("RenderAvatarMaxNonImpostors");
    settings["RenderVSyncEnable"] = gSavedSettings.getBOOL("RenderVSyncEnable");
    settings["RenderDeferred"] = gSavedSettings.getBOOL("RenderDeferred");
    settings["RenderShadowDetail"] = gSavedSettings.getS32("RenderShadowDetail");
    settings["RenderReflectionProbeDetail"] = gSavedSettings.getS32("RenderReflectionProbeDetail");
    settings["RenderReflectionsEnabled"] = gSavedSettings.getBOOL("RenderReflectionsEnabled");
    settings["RenderFarClip"] = gSavedSettings.getF32("RenderFarClip");
    settings["RenderVolumeLODFactor"] = gSavedSettings.getF32("RenderVolumeLODFactor");
    settings["RenderQualityPerformance"] = (LLSD::Integer)gSavedSettings.getU32("RenderQualityPerformance");
    settings["RenderGLContextCoreProfile"] = LLRender::sGLCoreProfile;
    settings["RenderBenchmarkUIScale"] = gSavedSettings.getF32("RenderBenchmarkUIScale");
    settings["RenderHiDPI"] = gSavedSettings.getBOOL("RenderHiDPI");
    settings["WindowHeight"] = (LLSD::Integer)gSavedSettings.getU32("WindowHeight");
    settings["WindowMaximized"] = gSavedSettings.getBOOL("WindowMaximized");
    settings["WindowWidth"] = (LLSD::Integer)gSavedSettings.getU32("WindowWidth");
    settings["YieldTime"] = gSavedSettings.getS32("YieldTime");
    context["effective_settings"] = settings;
    context["feature_flags"] = settings;

    return context;
}
#endif
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
#if defined(LL_RENDER_BENCHMARK)
    add("normalizeRendererDisplay",
        "Apply the renderer benchmark display scale after native window attachment.",
        &LLStatsListener::normalizeRendererDisplay,
        llsd::map("reply", LLSD()));
#endif
}

#if defined(LL_RENDER_BENCHMARK)
void LLStatsListener::normalizeRendererDisplay(LLSD const& evt)
{
    LLEventAPI::Response response(LLSD(), evt);
    if (!gViewerWindow || !gViewerWindow->getWindow())
    {
        return response.error("renderer window is not available");
    }

    // The factual Cocoa backing scale is only stable after the native window
    // is attached. Reflow with the window's backing dimensions because the
    // viewer's cached raw rectangle can still contain Cocoa logical points.
    LLCoordWindow backing_size;
    if (!gViewerWindow->getWindow()->getSize(&backing_size))
    {
        return response.error("renderer backing size is not available");
    }
    const LLCoordWindow requested_size(
        gSavedSettings.getU32("WindowWidth"),
        gSavedSettings.getU32("WindowHeight"));
    if ((backing_size.mX != requested_size.mX || backing_size.mY != requested_size.mY) &&
        !gViewerWindow->getWindow()->setSize(requested_size))
    {
        return response.error("renderer backing size could not be applied");
    }
    if (!gViewerWindow->getWindow()->getSize(&backing_size))
    {
        return response.error("renderer backing size is not available after resize");
    }
    gViewerWindow->reshape(backing_size.mX, backing_size.mY);
    response["accepted"] = true;
}
#endif

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
#if defined(LL_RENDER_BENCHMARK)
    stats["renderer_schema_version"] = 2;
    stats["renderer_ready"] = LLStartUp::getStartupState() == STATE_STARTED;
    stats["renderer_context"] = getRendererContext();
    stats["renderer_frames"] = collectRendererFrames(recording, num_periods);

    LLSD instrumentation;
    instrumentation["compile_time_enabled"] = true;
    instrumentation["cpu_phase_timing"] = true;
    instrumentation["resource_counters"] = true;
    instrumentation["gpu_pass_timing"] = "external-diagnostic";
    instrumentation["gpu_query_readback_in_steady_loop"] = false;
    stats["renderer_instrumentation"] = instrumentation;
#endif

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
