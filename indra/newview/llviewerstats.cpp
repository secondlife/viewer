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
#include "llvfile.h"

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
#include "llviewerwindow.h"		// *TODO: remove, only used for width/height
#include "llworld.h"
#include "llfeaturemanager.h"
#include "llviewernetwork.h"
#include "llmeshrepository.h" //for LLMeshRepository::sBytesReceived
#include "llsdserialize.h"
#include "llcorehttputil.h"

namespace LLStatViewer
{

LLTrace::CountStatHandle<>	FPS("FPS", "Frames rendered"),
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
							FRAMETIME_DOUBLED("frametimedoubled", "Ratio of frames 2x longer than previous"),
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

LLTrace::CountStatHandle<F64Seconds >	
							SIM_20_FPS_TIME("sim20fpstime", "Seconds with sim FPS below 20"),
							SIM_PHYSICS_20_FPS_TIME("simphysics20fpstime", "Seconds with physics FPS below 20"),
							LOSS_5_PERCENT_TIME("loss5percenttime", "Seconds with packet loss > 5%");

SimMeasurement<>			SIM_TIME_DILATION("simtimedilation", "Simulator time scale", LL_SIM_STAT_TIME_DILATION),
							SIM_FPS("simfps", "Simulator framerate", LL_SIM_STAT_FPS),
							SIM_PHYSICS_FPS("simphysicsfps", "Simulator physics framerate", LL_SIM_STAT_PHYSFPS),
							SIM_AGENT_UPS("simagentups", "", LL_SIM_STAT_AGENTUPS),
							SIM_SCRIPT_EPS("simscripteps", "", LL_SIM_STAT_SCRIPT_EPS),
							SIM_SKIPPED_SILHOUETTE("simsimskippedsilhouettesteps", "", LL_SIM_STAT_SKIPPEDAISILSTEPS_PS),
							SIM_MAIN_AGENTS("simmainagents", "Number of avatars in current region", LL_SIM_STAT_NUMAGENTMAIN),
							SIM_CHILD_AGENTS("simchildagents", "Number of avatars in neighboring regions", LL_SIM_STAT_NUMAGENTCHILD),
							SIM_OBJECTS("simobjects", "", LL_SIM_STAT_NUMTASKS),
							SIM_ACTIVE_OBJECTS("simactiveobjects", "Number of scripted and/or mocing objects", LL_SIM_STAT_NUMTASKSACTIVE),
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

LLTrace::SampleStatHandle<>	FPS_SAMPLE("fpssample"),
							NUM_IMAGES("numimagesstat"),
							NUM_RAW_IMAGES("numrawimagesstat"),
							NUM_OBJECTS("numobjectsstat"),
							NUM_ACTIVE_OBJECTS("numactiveobjectsstat"),
							ENABLE_VBO("enablevbo", "Vertex Buffers Enabled"),
							LIGHTING_DETAIL("lightingdetail", "Lighting Detail"),
							VISIBLE_AVATARS("visibleavatars", "Visible Avatars"),
							SHADER_OBJECTS("shaderobjects", "Object Shaders"),
							DRAW_DISTANCE("drawdistance", "Draw Distance"),
							PENDING_VFS_OPERATIONS("vfspendingoperations"),
							WINDOW_WIDTH("windowwidth", "Window width"),
							WINDOW_HEIGHT("windowheight", "Window height");

LLTrace::SampleStatHandle<LLUnit<F32, LLUnits::Percent> > 
							PACKETS_LOST_PERCENT("packetslostpercentstat");

static LLTrace::SampleStatHandle<bool> 
							CHAT_BUBBLES("chatbubbles", "Chat Bubbles Enabled");

LLTrace::SampleStatHandle<F64Megabytes >	GL_TEX_MEM("gltexmemstat"),
															GL_BOUND_MEM("glboundmemstat"),
															RAW_MEM("rawmemstat"),
															FORMATTED_MEM("formattedmemstat");
LLTrace::SampleStatHandle<F64Kilobytes >	DELTA_BANDWIDTH("deltabandwidth", "Increase/Decrease in bandwidth based on packet loss"),
															MAX_BANDWIDTH("maxbandwidth", "Max bandwidth setting");

	
SimMeasurement<F64Milliseconds >	SIM_FRAME_TIME("simframemsec", "", LL_SIM_STAT_FRAMEMS),
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
	
SimMeasurement<F64Kilobytes >	SIM_UNACKED_BYTES("simtotalunackedbytes", "", LL_SIM_STAT_TOTAL_UNACKED_BYTES);
SimMeasurement<F64Megabytes >	SIM_PHYSICS_MEM("physicsmemoryallocated", "", LL_SIM_STAT_SIMPHYSICSMEMORY);

LLTrace::SampleStatHandle<F64Milliseconds >	FRAMETIME_JITTER("frametimejitter", "Average delta between successive frame times"),
																FRAMETIME_SLEW("frametimeslew", "Average delta between frame time and mean"),
																SIM_PING("simpingstat");

LLTrace::EventStatHandle<LLUnit<F64, LLUnits::Meters> > AGENT_POSITION_SNAP("agentpositionsnap", "agent position corrections");

LLTrace::EventStatHandle<>	LOADING_WEARABLES_LONG_DELAY("loadingwearableslongdelay", "Wearables took too long to load");
	
LLTrace::EventStatHandle<F64Milliseconds >	REGION_CROSSING_TIME("regioncrossingtime", "CROSSING_AVG"),
																FRAME_STACKTIME("framestacktime", "FRAME_SECS"),
																UPDATE_STACKTIME("updatestacktime", "UPDATE_SECS"),
																NETWORK_STACKTIME("networkstacktime", "NETWORK_SECS"),
																IMAGE_STACKTIME("imagestacktime", "IMAGE_SECS"),
																REBUILD_STACKTIME("rebuildstacktime", "REBUILD_SECS"),
																RENDER_STACKTIME("renderstacktime", "RENDER_SECS");
	
LLTrace::EventStatHandle<F64Seconds >	AVATAR_EDIT_TIME("avataredittime", "Seconds in Edit Appearance"),
															TOOLBOX_TIME("toolboxtime", "Seconds using Toolbox"),
															MOUSELOOK_TIME("mouselooktime", "Seconds in Mouselook"),
															FPS_10_TIME("fps10time", "Seconds below 10 FPS"),
															FPS_8_TIME("fps8time", "Seconds below 8 FPS"),
															FPS_2_TIME("fps2time", "Seconds below 2 FPS");

LLTrace::EventStatHandle<LLUnit<F32, LLUnits::Percent> > OBJECT_CACHE_HIT_RATE("object_cache_hits");

}

LLViewerStats::LLViewerStats() 
:	mLastTimeDiff(0.0)
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
	if (getRecording().getLastValue(LLStatViewer::PACKETS_LOST_PERCENT) > F32Percent(5.0))
	{
		add(LLStatViewer::LOSS_5_PERCENT_TIME, time_diff);
	}
	
	F32 sim_fps = getRecording().getLastValue(LLStatViewer::SIM_FPS);
	if (0.f < sim_fps && sim_fps < 20.f)
	{
		add(LLStatViewer::SIM_20_FPS_TIME, time_diff);
	}
	
	F32 sim_physics_fps = getRecording().getLastValue(LLStatViewer::SIM_PHYSICS_FPS);

	if (0.f < sim_physics_fps && sim_physics_fps < 20.f)
	{
		add(LLStatViewer::SIM_PHYSICS_20_FPS_TIME, time_diff);
	}
		
	if (time_diff >= (F64Seconds)0.5)
	{
		record(LLStatViewer::FPS_2_TIME, time_diff);
	}
	if (time_diff >= (F64Seconds)0.125)
	{
		record(LLStatViewer::FPS_8_TIME, time_diff);
	}
	if (time_diff >= (F64Seconds)0.1)
	{
		record(LLStatViewer::FPS_10_TIME, time_diff);
	}

	if (gFrameCount && mLastTimeDiff > (F64Seconds)0.0)
	{
		// new "stutter" meter
		add(LLStatViewer::FRAMETIME_DOUBLED, time_diff >= 2.0 * mLastTimeDiff ? 1 : 0);

		// old stats that were never really used
		sample(LLStatViewer::FRAMETIME_JITTER, F64Milliseconds (mLastTimeDiff - time_diff));
			
		F32Seconds average_frametime = gRenderStartTime.getElapsedTimeF32() / (F32)gFrameCount;
		sample(LLStatViewer::FRAMETIME_SLEW, F64Milliseconds (average_frametime - time_diff));

		F32 max_bandwidth = gViewerThrottle.getMaxBandwidth();
		F32 delta_bandwidth = gViewerThrottle.getCurrentBandwidth() - max_bandwidth;
		sample(LLStatViewer::DELTA_BANDWIDTH, F64Bits(delta_bandwidth));
		sample(LLStatViewer::MAX_BANDWIDTH, F64Bits(max_bandwidth));
	}
	
	mLastTimeDiff = time_diff;
}

void LLViewerStats::addToMessage(LLSD &body)
{
	LLSD &misc = body["misc"];
	
	misc["Version"] = TRUE;
	//TODO RN: get last value, not mean
	misc["Vertex Buffers Enabled"] = getRecording().getMean(LLStatViewer::ENABLE_VBO);
	
	body["AgentPositionSnaps"] = getRecording().getSum(LLStatViewer::AGENT_POSITION_SNAP).value(); //mAgentPositionSnaps.asLLSD();
	LL_INFOS() << "STAT: AgentPositionSnaps: Mean = " << getRecording().getMean(LLStatViewer::AGENT_POSITION_SNAP).value() << "; StdDev = " << getRecording().getStandardDeviation(LLStatViewer::AGENT_POSITION_SNAP).value() 
			<< "; Count = " << getRecording().getSampleCount(LLStatViewer::AGENT_POSITION_SNAP) << LL_ENDL;
}

// *NOTE:Mani The following methods used to exist in viewer.cpp
// Moving them here, but not merging them into LLViewerStats yet.
U32		gTotalLandIn = 0, 
		gTotalLandOut = 0,
		gTotalWaterIn = 0, 
		gTotalWaterOut = 0;

F32		gAveLandCompression = 0.f, 
		gAveWaterCompression = 0.f,
		gBestLandCompression = 1.f,
		gBestWaterCompression = 1.f,
		gWorstLandCompression = 0.f, 
		gWorstWaterCompression = 0.f;

U32Bytes				gTotalWorldData, 
								gTotalObjectData, 
								gTotalTextureData;
U32								gSimPingCount = 0;
U32Bits				gObjectData;
F32Milliseconds		gAvgSimPing(0.f);
// rely on default initialization
U32Bytes			gTotalTextureBytesPerBoostLevel[LLViewerTexture::MAX_GL_IMAGE_CATEGORY];

extern U32  gVisCompared;
extern U32  gVisTested;

LLFrameTimer gTextureTimer;

void update_statistics()
{
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
	sample(LLStatViewer::LIGHTING_DETAIL, (F64)gPipeline.getLightingDetail());
	sample(LLStatViewer::DRAW_DISTANCE,   (F64)gSavedSettings.getF32("RenderFarClip"));
	sample(LLStatViewer::CHAT_BUBBLES,    gSavedSettings.getBOOL("UseChatBubbles"));

	typedef LLTrace::StatType<LLTrace::TimeBlockAccumulator>::instance_tracker_t stat_type_t;

	F64Seconds idle_secs = last_frame_recording.getSum(*stat_type_t::getInstance("Idle"));
	F64Seconds network_secs = last_frame_recording.getSum(*stat_type_t::getInstance("Network"));

	record(LLStatViewer::FRAME_STACKTIME, last_frame_recording.getSum(*stat_type_t::getInstance("Frame")));
	record(LLStatViewer::UPDATE_STACKTIME, idle_secs - network_secs);
	record(LLStatViewer::NETWORK_STACKTIME, network_secs);
	record(LLStatViewer::IMAGE_STACKTIME, last_frame_recording.getSum(*stat_type_t::getInstance("Update Images")));
	record(LLStatViewer::REBUILD_STACKTIME, last_frame_recording.getSum(*stat_type_t::getInstance("Sort Draw State")));
	record(LLStatViewer::RENDER_STACKTIME, last_frame_recording.getSum(*stat_type_t::getInstance("Render Geometry")));
		
	LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit(gAgent.getRegion()->getHost());
	if (cdp)
	{
		sample(LLStatViewer::SIM_PING, F64Milliseconds (cdp->getPingDelay()));
		gAvgSimPing = ((gAvgSimPing * gSimPingCount) + cdp->getPingDelay()) / (gSimPingCount + 1);
		gSimPingCount++;
	}
	else
	{
		sample(LLStatViewer::SIM_PING, U32Seconds(10));
	}

	if (LLViewerStats::instance().getRecording().getSum(LLStatViewer::FPS))
	{
		sample(LLStatViewer::FPS_SAMPLE, LLTrace::get_frame_recording().getPeriodMeanPerSec(LLStatViewer::FPS));
	}
	add(LLStatViewer::FPS, 1);

	F64Bits layer_bits = gVLManager.getLandBits() + gVLManager.getWindBits() + gVLManager.getCloudBits();
	add(LLStatViewer::LAYERS_NETWORK_DATA_RECEIVED, layer_bits);
	add(LLStatViewer::OBJECT_NETWORK_DATA_RECEIVED, gObjectData);
	sample(LLStatViewer::PENDING_VFS_OPERATIONS, LLVFile::getVFSThread()->getPending());
	add(LLStatViewer::ASSET_UDP_DATA_RECEIVED, F64Bits(gTransferManager.getTransferBitsIn(LLTCT_ASSET)));
	gTransferManager.resetTransferBitsIn(LLTCT_ASSET);

	if (LLAppViewer::getTextureFetch()->getNumRequests() == 0)
	{
		gTextureTimer.pause();
	}
	else
	{
		gTextureTimer.unpause();
	}
	
	sample(LLStatViewer::VISIBLE_AVATARS, LLVOAvatar::sNumVisibleAvatars);
	LLWorld::getInstance()->updateNetStats();
	LLWorld::getInstance()->requestCacheMisses();
	
	// Reset all of these values.
	gVLManager.resetBitCounts();
	gObjectData = (U32Bytes)0;
//	gDecodedBits = 0;

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
void send_stats()
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

	LLSD body;
	std::string url = gAgent.getRegion()->getCapability("ViewerStats");

	if (url.empty()) {
		LL_WARNS() << "Could not get ViewerStats capability" << LL_ENDL;
		return;
	}
	
	LLViewerStats::instance().getRecording().pause();

	body["session_id"] = gAgentSessionID;
	
	LLSD &agent = body["agent"];
	
	time_t ltime;
	time(&ltime);
	F32 run_time = F32(LLFrameTimer::getElapsedSeconds());

	agent["start_time"] = S32(ltime - S32(run_time));

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

	// send fps only for time app spends in foreground
	agent["fps"] = (F32)gForegroundFrameCount / gForegroundTime.getElapsedTimeF32();
	agent["version"] = LLVersionInfo::getChannelAndVersion();
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

	LLSD &system = body["system"];
	
	system["ram"] = (S32) gSysMemory.getPhysicalMemoryKB().value();
	system["os"] = LLAppViewer::instance()->getOSInfo().getOSStringSimple();
	system["cpu"] = gSysCPU.getCPUString();
	system["address_size"] = ADDRESS_SIZE;
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
	system["gpu_vendor"] = gGLManager.mGLVendorShort;
	system["gpu_version"] = gGLManager.mDriverVersionVendorString;
	system["opengl_version"] = gGLManager.mGLVersionString;

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
	else if (gPipeline.canUseWindLightShadersOnObjects())
	{
		shader_level = 2;
	}
	else if (gPipeline.canUseVertexShaders())
	{
		shader_level = 1;
	}


	system["shader_level"] = shader_level;

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

	// Misc stats, two strings and two ints
	// These are not expecticed to persist across multiple releases
	// Comment any changes with your name and the expected release revision
	// If the current revision is recent, ping the previous author before overriding
	LLSD &misc = body["stats"]["misc"];

	// Screen size so the UI team can figure out how big the widgets
	// appear and use a "typical" size for end user tests.

	S32 window_width = gViewerWindow->getWindowWidthRaw();
	S32 window_height = gViewerWindow->getWindowHeightRaw();
	S32 window_size = (window_width * window_height) / 1024;
	misc["string_1"] = llformat("%d", window_size);
	misc["string_2"] = llformat("Texture Time: %.2f, Total Time: %.2f", gTextureTimer.getElapsedTimeF32(), gFrameTimeSeconds.value());

// 	misc["int_1"] = LLSD::Integer(gSavedSettings.getU32("RenderQualityPerformance")); // Steve: 1.21
// 	misc["int_2"] = LLSD::Integer(gFrameStalls); // Steve: 1.21

	F32 unbaked_time = LLVOAvatar::sUnbakedTime * 1000.f / gFrameTimeSeconds;
	misc["int_1"] = LLSD::Integer(unbaked_time); // Steve: 1.22
	F32 grey_time = LLVOAvatar::sGreyTime * 1000.f / gFrameTimeSeconds;
	misc["int_2"] = LLSD::Integer(grey_time); // Steve: 1.22

	LL_INFOS() << "Misc Stats: int_1: " << misc["int_1"] << " int_2: " << misc["int_2"] << LL_ENDL;
	LL_INFOS() << "Misc Stats: string_1: " << misc["string_1"] << " string_2: " << misc["string_2"] << LL_ENDL;

	body["DisplayNamesEnabled"] = gSavedSettings.getBOOL("UseDisplayNames");
	body["DisplayNamesShowUsername"] = gSavedSettings.getBOOL("NameTagShowUsernames");
	
	body["MinimalSkin"] = false;

	LLViewerStats::getInstance()->addToMessage(body);

	LL_INFOS("LogViewerStatsPacket") << "Sending viewer statistics: " << body << LL_ENDL;
    LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(url, body,
        "Statistics posted to sim", "Failed to post statistics to sim");
	LLViewerStats::instance().getRecording().resume();
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
