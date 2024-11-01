/**
 * @file llviewerstats.cpp
 * @brief LLViewerStats class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llviewerstats.h"
#include "llviewerthrottle.h"

#include "message.h"
#include "llfloaterreg.h"
#include "llmemory.h"
#include "lltimer.h"

#include "llappviewer.h"

#include "pipeline.h"
#include "lltexturefetch.h"
#include "llviewerobjectlist.h"
#include "llviewertexturelist.h"
#include "lltexlayer.h"
#include "lltexlayerparams.h"
#include "llsurface.h"
#include "llvlmanager.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "llversioninfo.h"
#include "llfloatertools.h"
#include "lldebugview.h"
#include "llfasttimerview.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "llfeaturemanager.h"
#include "llviewernetwork.h"
#include "llmeshrepository.h" //for LLMeshRepository::sBytesReceived
#include "llperfstats.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llcorehttputil.h"
#include "llvoicevivox.h"
#include "llinventorymodel.h"
#include "lluiusage.h"
#include "lltranslate.h"
#include "llluamanager.h"
#include "scope_exit.h"

// "Minimal Vulkan" to get max API Version

// Calls
    #if defined(_WIN32)
        #define VKAPI_ATTR
        #define VKAPI_CALL __stdcall
        #define VKAPI_PTR  VKAPI_CALL
    #else
        #define VKAPI_ATTR
        #define VKAPI_CALL
        #define VKAPI_PTR
    #endif // _WIN32

// Macros
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // <variant> <-------major-------><-----------minor-----------> <--------------patch-------------->
    //      0x7          0x7F                     0x3FF                           0xFFF
    #define VK_API_VERSION_MAJOR(  version) (((uint32_t)(version) >> 22) & 0x07FU)  //  7 bits
    #define VK_API_VERSION_MINOR(  version) (((uint32_t)(version) >> 12) & 0x3FFU)  // 10 bits
    #define VK_API_VERSION_PATCH(  version) (((uint32_t)(version)      ) & 0xFFFU)  // 12 bits
    #define VK_API_VERSION_VARIANT(version) (((uint32_t)(version) >> 29) & 0x007U)  //  3 bits

    // NOTE: variant is first parameter!  This is to match vulkan/vulkan_core.h
    #define VK_MAKE_API_VERSION(variant, major, minor, patch) (0\
        | (((uint32_t)(major   & 0x07FU)) << 22) \
        | (((uint32_t)(minor   & 0x3FFU)) << 12) \
        | (((uint32_t)(patch   & 0xFFFU))      ) \
        | (((uint32_t)(variant & 0x007U)) << 29) )

    #define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

// Types
    VK_DEFINE_HANDLE(VkInstance);

    typedef enum VkResult
    {
        VK_SUCCESS = 0,
        VK_RESULT_MAX_ENUM = 0x7FFFFFFF
    } VkResult;

// Prototypes
    typedef void               (VKAPI_PTR *PFN_vkVoidFunction            )(void);
    typedef PFN_vkVoidFunction (VKAPI_PTR *PFN_vkGetInstanceProcAddr     )(VkInstance instance, const char* pName);
    typedef VkResult           (VKAPI_PTR *PFN_vkEnumerateInstanceVersion)(uint32_t* pApiVersion);

namespace LLStatViewer
{

LLTrace::CountStatHandle<>  FPS("FPS", "Frames rendered"),
                            PACKETS_IN("Packets In", "Packets received"),
                            PACKETS_LOST("packetsloststat", "Packets lost"),
                            PACKETS_OUT("packetsoutstat", "Packets sent"),
                            TEXTURE_PACKETS("texturepacketsstat", "Texture data packets received"),
                            CHAT_COUNT("chatcount", "Chat messages sent"),
                            IM_COUNT("imcount", "IMs sent"),
                            OBJECT_CREATE("objectcreate", "Number of objects created"),
                            OBJECT_REZ("objectrez", "Object rez count"),
                            LOGIN_TIMEOUTS("logintimeouts", "Number of login attempts that timed out"),
                            LSL_SAVES("lslsaves", "Number of times user has saved a script"),
                            ANIMATION_UPLOADS("animationuploads", "Animations uploaded"),
                            FLY("fly", "Fly count"),
                            TELEPORT("teleport", "Teleport count"),
                            DELETE_OBJECT("deleteobject", "Objects deleted"),
                            SNAPSHOT("snapshot", "Snapshots taken"),
                            UPLOAD_SOUND("uploadsound", "Sounds uploaded"),
                            UPLOAD_TEXTURE("uploadtexture", "Textures uploaded"),
                            EDIT_TEXTURE("edittexture", "Changes to textures on objects"),
                            KILLED("killed", "Number of times killed"),
                            TEX_BAKES("texbakes", "Number of times avatar textures have been baked"),
                            TEX_REBAKES("texrebakes", "Number of times avatar textures have been forced to rebake"),
                            NUM_NEW_OBJECTS("numnewobjectsstat", "Number of objects in scene that were not previously in cache");

LLTrace::CountStatHandle<LLUnit<F64, LLUnits::Kilotriangles> >
                            TRIANGLES_DRAWN("trianglesdrawnstat");

LLTrace::EventStatHandle<LLUnit<F64, LLUnits::Kilotriangles> >
                            TRIANGLES_DRAWN_PER_FRAME("trianglesdrawnperframestat");

LLTrace::CountStatHandle<F64Kilobytes >
                            ACTIVE_MESSAGE_DATA_RECEIVED("activemessagedatareceived", "Message system data received on all active regions"),
                            LAYERS_NETWORK_DATA_RECEIVED("layersdatareceived", "Network data received for layer data (terrain)"),
                            OBJECT_NETWORK_DATA_RECEIVED("objectdatareceived", "Network data received for objects"),
                            ASSET_UDP_DATA_RECEIVED("assetudpdatareceived", "Network data received for assets (animations, sounds) over UDP message system"),
                            TEXTURE_NETWORK_DATA_RECEIVED("texturedatareceived", "Network data received for textures"),
                            MESSAGE_SYSTEM_DATA_IN("messagedatain", "Incoming message system network data"),
                            MESSAGE_SYSTEM_DATA_OUT("messagedataout", "Outgoing message system network data");

SimMeasurement<>            SIM_TIME_DILATION("simtimedilation", "Simulator time scale", LL_SIM_STAT_TIME_DILATION),
                            SIM_FPS("simfps", "Simulator framerate", LL_SIM_STAT_FPS),
                            SIM_PHYSICS_FPS("simphysicsfps", "Simulator physics framerate", LL_SIM_STAT_PHYSFPS),
                            SIM_AGENT_UPS("simagentups", "", LL_SIM_STAT_AGENTUPS),
                            SIM_SCRIPT_EPS("simscripteps", "", LL_SIM_STAT_SCRIPT_EPS),
                            SIM_SKIPPED_SILHOUETTE("simsimskippedsilhouettesteps", "", LL_SIM_STAT_SKIPPEDAISILSTEPS_PS),
                            SIM_MAIN_AGENTS("simmainagents", "Number of avatars in current region", LL_SIM_STAT_NUMAGENTMAIN),
                            SIM_CHILD_AGENTS("simchildagents", "Number of avatars in neighboring regions", LL_SIM_STAT_NUMAGENTCHILD),
                            SIM_OBJECTS("simobjects", "", LL_SIM_STAT_NUMTASKS),
                            SIM_ACTIVE_OBJECTS("simactiveobjects", "Number of scripted and/or moving objects", LL_SIM_STAT_NUMTASKSACTIVE),
                            SIM_ACTIVE_SCRIPTS("simactivescripts", "Number of scripted objects", LL_SIM_STAT_NUMSCRIPTSACTIVE),
                            SIM_IN_PACKETS_PER_SEC("siminpps", "", LL_SIM_STAT_INPPS),
                            SIM_OUT_PACKETS_PER_SEC("simoutpps", "", LL_SIM_STAT_OUTPPS),
                            SIM_PENDING_DOWNLOADS("simpendingdownloads", "", LL_SIM_STAT_PENDING_DOWNLOADS),
                            SIM_PENDING_UPLOADS("simpendinguploads", "", LL_SIM_STAT_PENDING_UPLOADS),
                            SIM_PENDING_LOCAL_UPLOADS("simpendinglocaluploads", "", LL_SIM_STAT_PENDING_LOCAL_UPLOADS),
                            SIM_PHYSICS_PINNED_TASKS("physicspinnedtasks", "", LL_SIM_STAT_PHYSICS_PINNED_TASKS),
                            SIM_PHYSICS_LOD_TASKS("physicslodtasks", "", LL_SIM_STAT_PHYSICS_LOD_TASKS);

SimMeasurement<LLUnit<F64, LLUnits::Percent> >
                            SIM_PERCENTAGE_SCRIPTS_RUN("simpctscriptsrun", "", LL_SIM_STAT_PCTSCRIPTSRUN),
                            SIM_SKIPPED_CHARACTERS_PERCENTAGE("simsimpctsteppedcharacters", "", LL_SIM_STAT_PCTSTEPPEDCHARACTERS);

LLTrace::SampleStatHandle<> FPS_SAMPLE("fpssample"),
                            NUM_IMAGES("numimagesstat"),
                            NUM_RAW_IMAGES("numrawimagesstat"),
                            NUM_MATERIALS("nummaterials"),
                            NUM_OBJECTS("numobjectsstat"),
                            NUM_ACTIVE_OBJECTS("numactiveobjectsstat"),
                            ENABLE_VBO("enablevbo", "Vertex Buffers Enabled"),
                            VISIBLE_AVATARS("visibleavatars", "Visible Avatars"),
                            SHADER_OBJECTS("shaderobjects", "Object Shaders"),
                            DRAW_DISTANCE("drawdistance", "Draw Distance"),
                            WINDOW_WIDTH("windowwidth", "Window width"),
                            WINDOW_HEIGHT("windowheight", "Window height");

LLTrace::SampleStatHandle<LLUnit<F32, LLUnits::Percent> >
                            PACKETS_LOST_PERCENT("packetslostpercentstat");

static LLTrace::SampleStatHandle<bool>
                            CHAT_BUBBLES("chatbubbles", "Chat Bubbles Enabled");

LLTrace::SampleStatHandle<F64Megabytes > FORMATTED_MEM("formattedmemstat");

SimMeasurement<F64Milliseconds >    SIM_FRAME_TIME("simframemsec", "", LL_SIM_STAT_FRAMEMS),
                                                    SIM_NET_TIME("simnetmsec", "", LL_SIM_STAT_NETMS),
                                                    SIM_OTHER_TIME("simsimothermsec", "", LL_SIM_STAT_SIMOTHERMS),
                                                    SIM_PHYSICS_TIME("simsimphysicsmsec", "", LL_SIM_STAT_SIMPHYSICSMS),
                                                    SIM_PHYSICS_STEP_TIME("simsimphysicsstepmsec", "", LL_SIM_STAT_SIMPHYSICSSTEPMS),
                                                    SIM_PHYSICS_SHAPE_UPDATE_TIME("simsimphysicsshapeupdatemsec", "", LL_SIM_STAT_SIMPHYSICSSHAPEMS),
                                                    SIM_PHYSICS_OTHER_TIME("simsimphysicsothermsec", "", LL_SIM_STAT_SIMPHYSICSOTHERMS),
                                                    SIM_AI_TIME("simsimaistepmsec", "", LL_SIM_STAT_SIMAISTEPTIMEMS),
                                                    SIM_AGENTS_TIME("simagentmsec", "", LL_SIM_STAT_AGENTMS),
                                                    SIM_IMAGES_TIME("simimagesmsec", "", LL_SIM_STAT_IMAGESMS),
                                                    SIM_SCRIPTS_TIME("simscriptmsec", "", LL_SIM_STAT_SCRIPTMS),
                                                    SIM_SPARE_TIME("simsparemsec", "", LL_SIM_STAT_SIMSPARETIME),
                                                    SIM_SLEEP_TIME("simsleepmsec", "", LL_SIM_STAT_SIMSLEEPTIME),
                                                    SIM_PUMP_IO_TIME("simpumpiomsec", "", LL_SIM_STAT_IOPUMPTIME);

SimMeasurement<F64Kilobytes >   SIM_UNACKED_BYTES("simtotalunackedbytes", "", LL_SIM_STAT_TOTAL_UNACKED_BYTES);
SimMeasurement<F64Megabytes >   SIM_PHYSICS_MEM("physicsmemoryallocated", "", LL_SIM_STAT_SIMPHYSICSMEMORY);

LLTrace::SampleStatHandle<F64Milliseconds > FRAMETIME_JITTER("frametimejitter", "Average delta between successive frame times"),
                                            FRAMETIME("frametime", "Measured frame time"),
                                            SIM_PING("simpingstat");

LLTrace::EventStatHandle<LLUnit<F64, LLUnits::Meters> > AGENT_POSITION_SNAP("agentpositionsnap", "agent position corrections");

LLTrace::EventStatHandle<>  LOADING_WEARABLES_LONG_DELAY("loadingwearableslongdelay", "Wearables took too long to load");

LLTrace::EventStatHandle<F64Milliseconds >  REGION_CROSSING_TIME("regioncrossingtime", "CROSSING_AVG"),
                                                                FRAME_STACKTIME("framestacktime", "FRAME_SECS"),
                                                                UPDATE_STACKTIME("updatestacktime", "UPDATE_SECS"),
                                                                NETWORK_STACKTIME("networkstacktime", "NETWORK_SECS"),
                                                                IMAGE_STACKTIME("imagestacktime", "IMAGE_SECS"),
                                                                REBUILD_STACKTIME("rebuildstacktime", "REBUILD_SECS"),
                                                                RENDER_STACKTIME("renderstacktime", "RENDER_SECS");

LLTrace::EventStatHandle<F64Seconds >   AVATAR_EDIT_TIME("avataredittime", "Seconds in Edit Appearance"),
                                                            TOOLBOX_TIME("toolboxtime", "Seconds using Toolbox"),
                                                            MOUSELOOK_TIME("mouselooktime", "Seconds in Mouselook");

LLTrace::EventStatHandle<LLUnit<F32, LLUnits::Percent> > OBJECT_CACHE_HIT_RATE("object_cache_hits");

LLTrace::EventStatHandle<F64Seconds >   TEXTURE_FETCH_TIME("texture_fetch_time");

LLTrace::SampleStatHandle<LLUnit<F32, LLUnits::Percent> >  SCENERY_FRAME_PCT("scenery_frame_pct");
LLTrace::SampleStatHandle<LLUnit<F32, LLUnits::Percent> >  AVATAR_FRAME_PCT("avatar_frame_pct");
LLTrace::SampleStatHandle<LLUnit<F32, LLUnits::Percent> >  HUDS_FRAME_PCT("huds_frame_pct");
LLTrace::SampleStatHandle<LLUnit<F32, LLUnits::Percent> >  UI_FRAME_PCT("ui_frame_pct");
LLTrace::SampleStatHandle<LLUnit<F32, LLUnits::Percent> >  SWAP_FRAME_PCT("swap_frame_pct");
LLTrace::SampleStatHandle<LLUnit<F32, LLUnits::Percent> >  IDLE_FRAME_PCT("idle_frame_pct");
}

LLViewerStats::LLViewerStats()
:   mLastTimeDiff(0.0)
{
    getRecording().start();
}

LLViewerStats::~LLViewerStats()
{}

void LLViewerStats::resetStats()
{
    getRecording().reset();
}

void LLViewerStats::updateFrameStats(const F64Seconds time_diff)
{
    if (gFrameCount && mLastTimeDiff > (F64Seconds)0.0)
    {
        sample(LLStatViewer::FRAMETIME, time_diff);
        // old stats that were never really used
        F64Seconds jit = (F64Seconds)std::fabs((mLastTimeDiff - time_diff));
        sample(LLStatViewer::FRAMETIME_JITTER, jit);

        if (gFocusMgr.getAppHasFocus())
        {
            mForegroundFrameStats.push(F32(time_diff));
        }
        else
        {
            mBackgroundFrameStats.push(F32(time_diff));
        }

    }

    mLastTimeDiff = time_diff;
}

void LLViewerStats::addToMessage(LLSD &body)
{
    LLSD &misc = body["misc"];

    misc["Version"] = true;
    //TODO RN: get last value, not mean
    misc["Vertex Buffers Enabled"] = getRecording().getMean(LLStatViewer::ENABLE_VBO);

    body["AgentPositionSnaps"] = getRecording().getSum(LLStatViewer::AGENT_POSITION_SNAP).value(); //mAgentPositionSnaps.asLLSD();
    LL_INFOS() << "STAT: AgentPositionSnaps: Mean = " << getRecording().getMean(LLStatViewer::AGENT_POSITION_SNAP).value() << "; StdDev = " << getRecording().getStandardDeviation(LLStatViewer::AGENT_POSITION_SNAP).value()
            << "; Count = " << getRecording().getSampleCount(LLStatViewer::AGENT_POSITION_SNAP) << LL_ENDL;
}

// *NOTE:Mani The following methods used to exist in viewer.cpp
// Moving them here, but not merging them into LLViewerStats yet.
U32     gTotalLandIn = 0,
        gTotalLandOut = 0,
        gTotalWaterIn = 0,
        gTotalWaterOut = 0;

F32     gAveLandCompression = 0.f,
        gAveWaterCompression = 0.f,
        gBestLandCompression = 1.f,
        gBestWaterCompression = 1.f,
        gWorstLandCompression = 0.f,
        gWorstWaterCompression = 0.f;

U32Bytes                gTotalWorldData,
                                gTotalObjectData,
                                gTotalTextureData;
U32                             gSimPingCount = 0;
U32Bits             gObjectData;
F32Milliseconds     gAvgSimPing(0.f);
// rely on default initialization
U32Bytes            gTotalTextureBytesPerBoostLevel[LLViewerTexture::MAX_GL_IMAGE_CATEGORY];

extern U32  gVisCompared;
extern U32  gVisTested;

void update_statistics()
{
    LL_PROFILE_ZONE_SCOPED;

    gTotalWorldData += gVLManager.getTotalBytes();
    gTotalObjectData += gObjectData;

    // make sure we have a valid time delta for this frame
    if (gFrameIntervalSeconds > 0.f)
    {
        if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK)
        {
            record(LLStatViewer::MOUSELOOK_TIME, gFrameIntervalSeconds);
        }
        else if (gAgentCamera.getCameraMode() == CAMERA_MODE_CUSTOMIZE_AVATAR)
        {
            record(LLStatViewer::AVATAR_EDIT_TIME, gFrameIntervalSeconds);
        }
        else if (LLFloaterReg::instanceVisible("build"))
        {
            record(LLStatViewer::TOOLBOX_TIME, gFrameIntervalSeconds);
        }
    }

    LLTrace::Recording& last_frame_recording = LLTrace::get_frame_recording().getLastRecording();

    record(LLStatViewer::TRIANGLES_DRAWN_PER_FRAME, last_frame_recording.getSum(LLStatViewer::TRIANGLES_DRAWN));

    sample(LLStatViewer::ENABLE_VBO,      (F64)gSavedSettings.getBOOL("RenderVBOEnable"));
    sample(LLStatViewer::DRAW_DISTANCE,   (F64)gSavedSettings.getF32("RenderFarClip"));
    sample(LLStatViewer::CHAT_BUBBLES,    gSavedSettings.getBOOL("UseChatBubbles"));

    typedef LLTrace::StatType<LLTrace::TimeBlockAccumulator>::instance_tracker_t stat_type_t;

    record(LLStatViewer::FRAME_STACKTIME, last_frame_recording.getSum(*stat_type_t::getInstance("Frame")));

    if (gAgent.getRegion() && isAgentAvatarValid())
    {
        LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit(gAgent.getRegion()->getHost());
        if (cdp)
        {
            sample(LLStatViewer::SIM_PING, F64Milliseconds(cdp->getPingDelay()));
            gAvgSimPing = ((gAvgSimPing * gSimPingCount) + cdp->getPingDelay()) / (gSimPingCount + 1);
            gSimPingCount++;
        }
        else
        {
            sample(LLStatViewer::SIM_PING, U32Seconds(10));
        }
    }

    if (LLViewerStats::instance().getRecording().getSum(LLStatViewer::FPS))
    {
        sample(LLStatViewer::FPS_SAMPLE, LLTrace::get_frame_recording().getPeriodMeanPerSec(LLStatViewer::FPS));
    }
    add(LLStatViewer::FPS, 1);

    F64Bits layer_bits = gVLManager.getLandBits() + gVLManager.getWindBits() + gVLManager.getCloudBits();
    add(LLStatViewer::LAYERS_NETWORK_DATA_RECEIVED, layer_bits);
    add(LLStatViewer::OBJECT_NETWORK_DATA_RECEIVED, gObjectData);
    add(LLStatViewer::ASSET_UDP_DATA_RECEIVED, F64Bits(gTransferManager.getTransferBitsIn(LLTCT_ASSET)));
    gTransferManager.resetTransferBitsIn(LLTCT_ASSET);

    sample(LLStatViewer::VISIBLE_AVATARS, LLVOAvatar::sNumVisibleAvatars);
    LLWorld *world = LLWorld::getInstance(); // not LLSingleton
    if (world)
    {
        world->updateNetStats();
        world->requestCacheMisses();
    }

    // Reset all of these values.
    gVLManager.resetBitCounts();
    gObjectData = (U32Bytes)0;
//  gDecodedBits = 0;

    // Only update texture stats periodically so that they are less noisy
    {
        static const F32 texture_stats_freq = 10.f;
        static LLFrameTimer texture_stats_timer;
        if (texture_stats_timer.getElapsedTimeF32() >= texture_stats_freq)
        {
            gTotalTextureData = LLViewerStats::instance().getRecording().getSum(LLStatViewer::TEXTURE_NETWORK_DATA_RECEIVED);
            texture_stats_timer.reset();
        }
    }

    if (LLFloaterReg::instanceVisible("scene_load_stats"))
    {
        static const F32 perf_stats_freq = 1;
        static LLFrameTimer perf_stats_timer;
        if (perf_stats_timer.getElapsedTimeF32() >= perf_stats_freq)
        {
            LLStringUtil::format_map_t args;
            LLPerfStats::bufferToggleLock.lock(); // prevent toggle for a moment

            auto tot_frame_time_raw = LLPerfStats::StatsRecorder::getSceneStat(LLPerfStats::StatType_t::RENDER_FRAME);
            // cumulative avatar time (includes idle processing, attachments and base av)
            auto tot_avatar_time_raw = LLPerfStats::us_to_raw(LLVOAvatar::getTotalGPURenderTime());
            // cumulative avatar render specific time (a bit arbitrary as the processing is too.)
            // auto tot_av_idle_time_raw = LLPerfStats::StatsRecorder::getSum(AvType, LLPerfStats::StatType_t::RENDER_IDLE);
            // auto tot_avatar_render_time_raw = tot_avatar_time_raw - tot_av_idle_time_raw;
            // the time spent this frame on the "display()" call. Treated as "tot time rendering"
            auto tot_render_time_raw = LLPerfStats::StatsRecorder::getSceneStat(LLPerfStats::StatType_t::RENDER_DISPLAY);
            // sleep time is basically forced sleep when window out of focus
            auto tot_sleep_time_raw = LLPerfStats::StatsRecorder::getSceneStat(LLPerfStats::StatType_t::RENDER_SLEEP);
            // time spent on UI
            auto tot_ui_time_raw = LLPerfStats::StatsRecorder::getSceneStat(LLPerfStats::StatType_t::RENDER_UI);
            // cumulative time spent rendering HUDS
            auto tot_huds_time_raw = LLPerfStats::StatsRecorder::getSceneStat(LLPerfStats::StatType_t::RENDER_HUDS);
            // "idle" time. This is the time spent in the idle poll section of the main loop
            auto tot_idle_time_raw = LLPerfStats::StatsRecorder::getSceneStat(LLPerfStats::StatType_t::RENDER_IDLE);
            // similar to sleep time, induced by FPS limit
            //auto tot_limit_time_raw = LLPerfStats::StatsRecorder::getSceneStat(LLPerfStats::StatType_t::RENDER_FPSLIMIT);
            // swap time is time spent in swap buffer
            auto tot_swap_time_raw = LLPerfStats::StatsRecorder::getSceneStat(LLPerfStats::StatType_t::RENDER_SWAP);

            LLPerfStats::bufferToggleLock.unlock();

             auto tot_frame_time_ns = LLPerfStats::raw_to_ns(tot_frame_time_raw);
            auto tot_avatar_time_ns = LLPerfStats::raw_to_ns(tot_avatar_time_raw);
            auto tot_huds_time_ns = LLPerfStats::raw_to_ns(tot_huds_time_raw);
            // UI time includes HUD time so dedut that before we calc percentages
            auto tot_ui_time_ns = LLPerfStats::raw_to_ns(tot_ui_time_raw - tot_huds_time_raw);

            // auto tot_sleep_time_ns          = LLPerfStats::raw_to_ns( tot_sleep_time_raw );
            // auto tot_limit_time_ns          = LLPerfStats::raw_to_ns( tot_limit_time_raw );

            // auto tot_render_time_ns         = LLPerfStats::raw_to_ns( tot_render_time_raw );
            auto tot_idle_time_ns = LLPerfStats::raw_to_ns(tot_idle_time_raw);
            auto tot_swap_time_ns = LLPerfStats::raw_to_ns(tot_swap_time_raw);
            auto tot_scene_time_ns = LLPerfStats::raw_to_ns(tot_render_time_raw - tot_avatar_time_raw - tot_swap_time_raw - tot_ui_time_raw);
            // auto tot_overhead_time_ns  = LLPerfStats::raw_to_ns( tot_frame_time_raw - tot_render_time_raw - tot_idle_time_raw );

            // // remove time spent sleeping for fps limit or out of focus.
            // tot_frame_time_ns -= tot_limit_time_ns;
            // tot_frame_time_ns -= tot_sleep_time_ns;

            if (tot_frame_time_ns != 0)
            {
                auto pct_avatar_time = (tot_avatar_time_ns * 100) / tot_frame_time_ns;
                auto pct_huds_time = (tot_huds_time_ns * 100) / tot_frame_time_ns;
                auto pct_ui_time = (tot_ui_time_ns * 100) / tot_frame_time_ns;
                auto pct_idle_time = (tot_idle_time_ns * 100) / tot_frame_time_ns;
                auto pct_swap_time = (tot_swap_time_ns * 100) / tot_frame_time_ns;
                auto pct_scene_render_time = (tot_scene_time_ns * 100) / tot_frame_time_ns;
                pct_avatar_time = llclamp(pct_avatar_time, 0., 100.);
                pct_huds_time = llclamp(pct_huds_time, 0., 100.);
                pct_ui_time = llclamp(pct_ui_time, 0., 100.);
                pct_idle_time = llclamp(pct_idle_time, 0., 100.);
                pct_swap_time = llclamp(pct_swap_time, 0., 100.);
                pct_scene_render_time = llclamp(pct_scene_render_time, 0., 100.);
                if (tot_sleep_time_raw == 0)
                {
                    sample(LLStatViewer::SCENERY_FRAME_PCT, (U32)llround(pct_scene_render_time));
                    sample(LLStatViewer::AVATAR_FRAME_PCT, (U32)llround(pct_avatar_time));
                    sample(LLStatViewer::HUDS_FRAME_PCT, (U32)llround(pct_huds_time));
                    sample(LLStatViewer::UI_FRAME_PCT, (U32)llround(pct_ui_time));
                    sample(LLStatViewer::SWAP_FRAME_PCT, (U32)llround(pct_swap_time));
                    sample(LLStatViewer::IDLE_FRAME_PCT, (U32)llround(pct_idle_time));
                }
            }
            else
            {
                LL_WARNS("performance") << "Scene time 0. Skipping til we have data." << LL_ENDL;
            }
            perf_stats_timer.reset();
        }
    }
}

/*
 * The sim-side LLSD is in newsim/llagentinfo.cpp:forwardViewerStats.
 *
 * There's also a compatibility shim for the old fixed-format sim
 * stats in newsim/llagentinfo.cpp:processViewerStats.
 *
 * If you move stats around here, make the corresponding changes in
 * those locations, too.
 */
void send_viewer_stats(bool include_preferences)
{
    // IW 9/23/02 I elected not to move this into LLViewerStats
    // because it depends on too many viewer.cpp globals.
    // Someday we may want to merge all our stats into a central place
    // but that day is not today.

    // Only send stats if the agent is connected to a region.
    if (!gAgent.getRegion())
    {
        return;
    }

    std::string url = gAgent.getRegion()->getCapability("ViewerStats");

    if (url.empty()) {
        LL_WARNS() << "Could not get ViewerStats capability" << LL_ENDL;
        return;
    }

    LLSD body = capture_viewer_stats(include_preferences);
    LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(url, body,
        "Statistics posted to sim", "Failed to post statistics to sim");
}

LLSD capture_viewer_stats(bool include_preferences)
{
    LLViewerStats& vstats = LLViewerStats::instance();

    vstats.getRecording().pause();
    LL::scope_exit cleanup([&vstats]{ vstats.getRecording().resume(); });

    LLSD body;
    LLSD &agent = body["agent"];

    time_t ltime;
    time(&ltime);
    F32 run_time = F32(LLFrameTimer::getElapsedSeconds());

    // The first stat set must have a 0 run time if it doesn't actually
    // contain useful data in terms of FPS, etc.  We use half the
    // SEND_STATS_PERIOD seconds as the point at which these statistics become
    // valid.  Data warehouse uses a 0 value here to easily discard these
    // records with non-useful FPS values etc.
    if (run_time < (SEND_STATS_PERIOD / 2))
    {
        agent["run_time"] = 0.0f;
    }
    else
    {
        agent["run_time"] = run_time;
    }

    agent["start_time"] = S32(ltime - S32(run_time));

    agent["fg_frame_stats"] = vstats.mForegroundFrameStats.asLLSD();
    agent["fg_frame_stats"]["ofr"] = ofr(vstats.mForegroundFrameStats);
    agent["fg_frame_stats"]["fps"] = fps(vstats.mForegroundFrameStats);

    agent["bg_frame_stats"] = vstats.mBackgroundFrameStats.asLLSD();
    agent["bg_frame_stats"]["ofr"] = ofr(vstats.mBackgroundFrameStats);
    agent["bg_frame_stats"]["fps"] = fps(vstats.mBackgroundFrameStats);

    // report time the viewer has spent in the foreground
    agent["foreground_time"] = gForegroundTime.getElapsedTimeF32();
    agent["foreground_frame_count"] = (S32) gForegroundFrameCount;

    // send fps only for time app spends in foreground
    agent["fps"] = (F32)gForegroundFrameCount / gForegroundTime.getElapsedTimeF32();
    agent["version"] = LLVersionInfo::instance().getChannelAndVersion();
    std::string language = LLUI::getLanguage();
    agent["language"] = language;

    agent["sim_fps"] = ((F32) gFrameCount - gSimFrames) /
        (F32) (gRenderStartTime.getElapsedTimeF32() - gSimLastTime);

    gSimLastTime = gRenderStartTime.getElapsedTimeF32();
    gSimFrames   = (F32) gFrameCount;

    agent["agents_in_view"] = LLVOAvatar::sNumVisibleAvatars;
    agent["ping"] = gAvgSimPing.value();
    agent["meters_traveled"] = gAgent.getDistanceTraveled();
    agent["regions_visited"] = gAgent.getRegionsVisited();
    agent["mem_use"] = LLMemory::getCurrentRSS() / 1024.0;
    agent["translation"] = LLTranslate::instance().asLLSD();

    LLSD &system = body["system"];

    system["ram"] = (S32) gSysMemory.getPhysicalMemoryKB().value();
    system["os"] = LLOSInfo::instance().getOSStringSimple();
    system["cpu"] = gSysCPU.getCPUString();
    system["cpu_sse"] = gSysCPU.getSSEVersions();
    system["address_size"] = ADDRESS_SIZE;
    system["os_bitness"] = LLOSInfo::instance().getOSBitness();
    system["hardware_concurrency"] = (LLSD::Integer) std::thread::hardware_concurrency();
    unsigned char MACAddress[MAC_ADDRESS_BYTES];
    LLUUID::getNodeID(MACAddress);
    std::string macAddressString = llformat("%02x-%02x-%02x-%02x-%02x-%02x",
                                            MACAddress[0],MACAddress[1],MACAddress[2],
                                            MACAddress[3],MACAddress[4],MACAddress[5]);
    system["mac_address"] = macAddressString;
    system["serial_number"] = LLAppViewer::instance()->getSerialNumber();
    std::string gpu_desc = llformat(
        "%-6s Class %d ",
        gGLManager.mGLVendorShort.substr(0,6).c_str(),
        (S32)LLFeatureManager::getInstance()->getGPUClass())
        + gGLManager.getRawGLString();

    system["gpu"] = gpu_desc;
    system["gpu_class"] = (S32)LLFeatureManager::getInstance()->getGPUClass();
    system["gpu_memory_bandwidth"] = LLFeatureManager::getInstance()->getGPUMemoryBandwidth();
    system["gpu_vendor"] = gGLManager.mGLVendorShort;
    system["gpu_version"] = gGLManager.mDriverVersionVendorString;
    system["opengl_version"] = gGLManager.mGLVersionString;

    gGLManager.asLLSD(system["gl"]);


    S32 shader_level = 0;
    if (LLPipeline::sRenderDeferred)
    {
        if (LLPipeline::RenderShadowDetail > 0)
        {
            shader_level = 5;
        }
        else if (LLPipeline::RenderDeferredSSAO)
        {
            shader_level = 4;
        }
        else
        {
            shader_level = 3;
        }
    }
    else
    {
        shader_level = 2;
    }



    system["shader_level"] = shader_level;

    LLSD &scripts = body["scripts"];
    scripts["lua_scripts"] = LLLUAmanager::sScriptCount;
    scripts["lua_auto_scripts"] = LLLUAmanager::sAutorunScriptCount;

    LLSD &download = body["downloads"];

    download["world_kbytes"] = F64Kilobytes(gTotalWorldData).value();
    download["object_kbytes"] = F64Kilobytes(gTotalObjectData).value();
    download["texture_kbytes"] = F64Kilobytes(gTotalTextureData).value();
    download["mesh_kbytes"] = LLMeshRepository::sBytesReceived/1024.0;

    LLSD &in = body["stats"]["net"]["in"];

    in["kbytes"] = gMessageSystem->mTotalBytesIn / 1024.0;
    in["packets"] = (S32) gMessageSystem->mPacketsIn;
    in["compressed_packets"] = (S32) gMessageSystem->mCompressedPacketsIn;
    in["savings"] = (gMessageSystem->mUncompressedBytesIn -
                     gMessageSystem->mCompressedBytesIn) / 1024.0;

    LLSD &out = body["stats"]["net"]["out"];

    out["kbytes"] = gMessageSystem->mTotalBytesOut / 1024.0;
    out["packets"] = (S32) gMessageSystem->mPacketsOut;
    out["compressed_packets"] = (S32) gMessageSystem->mCompressedPacketsOut;
    out["savings"] = (gMessageSystem->mUncompressedBytesOut -
                      gMessageSystem->mCompressedBytesOut) / 1024.0;

    LLSD &fail = body["stats"]["failures"];

    fail["send_packet"] = (S32) gMessageSystem->mSendPacketFailureCount;
    fail["dropped"] = (S32) gMessageSystem->mDroppedPackets;
    fail["resent"] = (S32) gMessageSystem->mResentPackets;
    fail["failed_resends"] = (S32) gMessageSystem->mFailedResendPackets;
    fail["off_circuit"] = (S32) gMessageSystem->mOffCircuitPackets;
    fail["invalid"] = (S32) gMessageSystem->mInvalidOnCircuitPackets;
    fail["missing_updater"] = (S32) LLAppViewer::instance()->isUpdaterMissing();

    LLSD &inventory = body["inventory"];
    inventory["usable"] = gInventory.isInventoryUsable();
    LLSD& validation_info = inventory["validation_info"];
    gInventory.mValidationInfo->asLLSD(validation_info);

    body["ui"] = LLUIUsage::instance().asLLSD();

    body["stats"]["voice"] = LLVoiceVivoxStats::getInstance()->read();

    // Misc stats, two strings and two ints
    // These are not expecticed to persist across multiple releases
    // Comment any changes with your name and the expected release revision
    // If the current revision is recent, ping the previous author before overriding
    LLSD &misc = body["stats"]["misc"];

#ifdef LL_WINDOWS
    // Probe for Vulkan capability (Dave Houlton 05/2020)
    //
    // Check for presense of a Vulkan loader dll, as a proxy for a Vulkan-capable gpu.
    // False-positives and false-negatives are possible, but unlikely. We'll get a good
    // approximation of Vulkan capability within current user systems from this. More
    // detailed information on versions and extensions can come later.
    static bool vulkan_oneshot = false;
    static bool vulkan_detected = false;
    static std::string vulkan_max_api_version( "0.0" ); // Unknown/None

    if (!vulkan_oneshot)
    {
        // The 32-bit and 64-bit versions normally exist in:
        //     C:\Windows\System32
        //     C:\Windows\SysWOW64
        HMODULE vulkan_loader = LoadLibraryA("vulkan-1.dll");
        if (NULL != vulkan_loader)
        {
            vulkan_detected = true;
            vulkan_max_api_version = "1.0"; // We have at least 1.0.  See the note about vkEnumerateInstanceVersion() below.

            // We use Run-Time Dynamic Linking (via GetProcAddress()) instead of Load-Time Dynamic Linking (via directly calling vkGetInstanceProcAddr()).
            // This allows us to:
            //   a) not need the header: #include <vulkan/vulkan.h>
            //      (and not need to set the corresponding "Additional Include Directories" as long as we provide the equivalent Vulkan types/prototypes/etc.)
            //   b) not need to link to: vulkan-1.lib
            //      (and not need to set the corresponding "Additional Library Directories")
            // The former will allow Second Life to start and run even if the vulkan.dll is missing.
            // The latter will require us to:
            //   a) link with vulkan-1.lib
            //   b) cause a System Error at startup if the .dll is not found:
            //      "The code execution cannot proceed because vulkan-1.dll was not found."
            //
            // See:
            //   https://docs.microsoft.com/en-us/windows/win32/dlls/using-run-time-dynamic-linking
            //   https://docs.microsoft.com/en-us/windows/win32/dlls/run-time-dynamic-linking

            // NOTE: Technically we can use GetProcAddress() as a replacement for vkGetInstanceProcAddr()
            //       but the canonical recommendation (mandate?) is to use vkGetInstanceProcAddr().
            PFN_vkGetInstanceProcAddr pGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) GetProcAddress(vulkan_loader, "vkGetInstanceProcAddr");
            if(pGetInstanceProcAddr)
            {
                // Check for vkEnumerateInstanceVersion.  If it exists then we have at least 1.1 and can query the max API version.
                // NOTE: Each VkPhysicalDevice that supports Vulkan has its own VkPhysicalDeviceProperties.apiVersion which is separate from the max API version!
                // See: https://www.lunarg.com/wp-content/uploads/2019/02/Vulkan-1.1-Compatibility-Statement_01_19.pdf
                PFN_vkEnumerateInstanceVersion pEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion) pGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
                if(pEnumerateInstanceVersion)
                {
                    uint32_t version = VK_MAKE_API_VERSION(0,1,1,0);   // e.g. 4202631 = 1.2.135.0
                    VkResult status  = pEnumerateInstanceVersion( &version );
                    if (status != VK_SUCCESS)
                    {
                        LL_INFOS("Vulkan") << "Failed to get Vulkan version.  Assuming 1.0" << LL_ENDL;
                    }
                    else
                    {
                        // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#extendingvulkan-coreversions-versionnumbers
                        int major   = VK_API_VERSION_MAJOR  ( version );
                        int minor   = VK_API_VERSION_MINOR  ( version );
                        int patch   = VK_API_VERSION_PATCH  ( version );
                        int variant = VK_API_VERSION_VARIANT( version );

                        vulkan_max_api_version = llformat( "%d.%d.%d.%d", major, minor, patch, variant );
                        LL_INFOS("Vulkan") << "Vulkan API version: " << vulkan_max_api_version << ", Raw version: " << version << LL_ENDL;
                    }
                }
            }
            else
            {
                LL_WARNS("Vulkan") << "FAILED to get Vulkan vkGetInstanceProcAddr()!" << LL_ENDL;
            }
            FreeLibrary(vulkan_loader);
        }
        vulkan_oneshot = true;
    }

    misc["string_1"] = vulkan_detected ? llformat("Vulkan driver is detected") : llformat("No Vulkan driver detected");
    misc["VulkanMaxApiVersion"] = vulkan_max_api_version;

#else
    misc["string_1"] = llformat("Unused");
#endif // LL_WINDOWS

    misc["string_2"] = llformat("Unused");
    misc["int_1"] = LLSD::Integer(0);
    misc["int_2"] = LLSD::Integer(0);

    LL_INFOS() << "Misc Stats: int_1: " << misc["int_1"] << " int_2: " << misc["int_2"] << LL_ENDL;
    LL_INFOS() << "Misc Stats: string_1: " << misc["string_1"] << " string_2: " << misc["string_2"] << LL_ENDL;

    body["DisplayNamesEnabled"] = gSavedSettings.getBOOL("UseDisplayNames");
    body["DisplayNamesShowUsername"] = gSavedSettings.getBOOL("NameTagShowUsernames");

    // Preferences
    if (include_preferences)
    {
        bool diffs_only = true; // only log preferences that differ from default
        body["preferences"]["settings"] = gSavedSettings.asLLSD(diffs_only);
        body["preferences"]["settings_per_account"] = gSavedPerAccountSettings.asLLSD(diffs_only);
    }

    body["MinimalSkin"] = false;


    LL_INFOS("LogViewerStatsPacket") << "Sending viewer statistics: " << body << LL_ENDL;

    // <ND> Do those lines even do anything sane in regard of debug logging?
    LL_DEBUGS("LogViewerStatsPacket") << " ";
    std::string filename("viewer_stats_packet.xml");
    llofstream of(filename.c_str());
    LLSDSerialize::toPrettyXML(body,of);
    LL_ENDL;

    // The session ID token must never appear in logs
    body["session_id"] = gAgentSessionID;

    LLViewerStats::getInstance()->addToMessage(body);
    return body;
}

LLTimer& LLViewerStats::PhaseMap::getPhaseTimer(const std::string& phase_name)
{
    phase_map_t::iterator iter = mPhaseMap.find(phase_name);
    if (iter == mPhaseMap.end())
    {
        LLTimer timer;
        mPhaseMap[phase_name] = timer;
    }
    LLTimer& timer = mPhaseMap[phase_name];
    return timer;
}

void LLViewerStats::PhaseMap::startPhase(const std::string& phase_name)
{
    LLTimer& timer = getPhaseTimer(phase_name);
    timer.start();
    //LL_DEBUGS("Avatar") << "startPhase " << phase_name << LL_ENDL;
}

void LLViewerStats::PhaseMap::clearPhases()
{
    //LL_DEBUGS("Avatar") << "clearPhases" << LL_ENDL;

    mPhaseMap.clear();
}

LLSD LLViewerStats::PhaseMap::asLLSD()
{
    LLSD result;
    for (phase_map_t::iterator iter = mPhaseMap.begin(); iter != mPhaseMap.end(); ++iter)
    {
        const std::string& phase_name = iter->first;
        result[phase_name]["completed"] = LLSD::Integer(!(iter->second.getStarted()));
        result[phase_name]["elapsed"] = iter->second.getElapsedTimeF32();
    }
    return result;
}

// static initializer
//static
LLViewerStats::phase_stats_t LLViewerStats::PhaseMap::sStats;

LLViewerStats::PhaseMap::PhaseMap()
{
}

void LLViewerStats::PhaseMap::stopPhase(const std::string& phase_name)
{
    phase_map_t::iterator iter = mPhaseMap.find(phase_name);
    if (iter != mPhaseMap.end())
    {
        if (iter->second.getStarted())
        {
            // Going from started to stopped state - record stats.
            iter->second.stop();
        }
    }
}

// static
LLViewerStats::StatsAccumulator& LLViewerStats::PhaseMap::getPhaseStats(const std::string& phase_name)
{
    phase_stats_t::iterator it = sStats.find(phase_name);
    if (it == sStats.end())
    {
        LLViewerStats::StatsAccumulator new_stats;
        sStats[phase_name] = new_stats;
    }
    return sStats[phase_name];
}

// static
void LLViewerStats::PhaseMap::recordPhaseStat(const std::string& phase_name, F32 value)
{
    LLViewerStats::StatsAccumulator& stats = getPhaseStats(phase_name);
    stats.push(value);
}


bool LLViewerStats::PhaseMap::getPhaseValues(const std::string& phase_name, F32& elapsed, bool& completed)
{
    phase_map_t::iterator iter = mPhaseMap.find(phase_name);
    bool found = false;
    if (iter != mPhaseMap.end())
    {
        found = true;
        elapsed =  iter->second.getElapsedTimeF32();
        completed = !iter->second.getStarted();
        //LL_DEBUGS("Avatar") << " phase_name " << phase_name << " elapsed " << elapsed << " completed " << completed << " timer addr " << (S32)(&iter->second) << LL_ENDL;
    }
    else
    {
        //LL_DEBUGS("Avatar") << " phase_name " << phase_name << " NOT FOUND"  << LL_ENDL;
    }

    return found;
}
